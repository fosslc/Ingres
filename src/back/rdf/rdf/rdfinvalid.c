/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<st.h>
#include	<pc.h>
#include	<me.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<cs.h>
#include	<cx.h>
#include	<lg.h>
#include	<scf.h>
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
#include	<ex.h>
#include	<rdfint.h>

/**
**
**  Name: RDFINVALID.C - Invalidate a table information block currently in
**			 the cache.
**
**  Description:
**	This file contains the routine for releasing a table information
**	record from the hash/cache table.
**
**	rdf_invalidate - Invalidate a table information block currently in
**			 the cache.
**
**  History:
**	12-jan-87 (puree)
**	    initial version.
**	31-jan-91 (teresa)
**	    add statistic keeping logic
**      20-dec-91 (teresa)
**          added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**      23-jan-92 (tersea)
**          SYBIL: change criteria for clearing QT hash table to be if caller
**                 is PSF; rdi_hash->rdv_rdesc_hash; rdi_qthash->rdv_qtree_hash
**	13-mar-92 (teresa)
**	    major changes for SYBIL's new invalidation algorithm.
**	16-mar-92 (teresa)
**	    prototypes
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	24-mar-93 (smc)
**	    Cast function parameters to match prototype declarations.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**      02-feb-1996 (ramra01)
**          Destroy the ULH object before releasing the memory stream.
**          This is based on an earlier fix in the 6.4 codeline to 
**          resolve concurrency issues with rdf.
**	07-feb-1997 (canor01)
**	    If there are multiple servers in the installation, signal
**	    the RDF cache synchronization event.
**      23-jul-1998 (rigka01)
**          In rdf_inv_alert,
**          set SCE_NOORIG so that alert does not get sent to alert
**          originator
**	14-aug-1998 (nanpr01)
**	    Since we are looking at uptr without a semaphore protection,
**	    it is possible in a concurrent environment to see it nil. In that
**	    case we must retry before spitting out RD0045 error.
**	03-sep-1998 (nanpr01)
**	    Another shot at the RD0045 problem to protect it rather than
**	    retry logic.
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
**	30-may-2002 (devjo01)
**	    Cache coherency problems in a cluster.
**	14-Jan-2004 (schka24)
**	    Avoid using a partition ID in db-tab-index.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror(). Use new form of uleFormat().
**      21-Nov-2008 (hanal04) Bug 121248
**          If we free the rdf_info)blk and the caller supplied the address
**          of a referencing pointer, null the content of that pointer so
**          that the caller can not continue to reference freed memory.
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
*/

/* forward declaration */
static DB_STATUS rdf_inv_alert( RDR_RB *rdr_rb );
static bool rdf_ismultiserver( void );

/* infoblk_invalidate  -- Invalidate the info block pointed by 	rdf_info_blk */
static DB_STATUS infoblk_invalidate(
	RDF_GLOBAL	*global,
	RDR_INFO	*infoblk);

/* name_invalidate - invalidate the object by name */
static DB_STATUS name_invalidate(
	RDF_GLOBAL	*global,
	DB_TAB_ID	*tabid);

/*{
** Name: rdf_invalidate - Invalidate a table information block currently in
**			  the cache.
**
** External call format:    status = rdf_call(RDF_INVALIDATE, &rdf_cb);
**
** Description:
**	This function removes the specified table information block from
**	the RDF cache.  The table removed becomes immediately inaccessible
**	to all subsequent references.  The actual removal of the data area
**	occurs when the reference count (the current user count) becomes
**	zero.
**
**	There are many ways to invalidate table information in RDF cache:
**
**	    1)	The user who obtains access to a specific table info block
**		can invalidate the block using the rdf_cb.rdf_info_blk
**		pointer.  The pointer must be obtained by an earlier call
**		to the rdf_getdesc.  THIS IS THE PREFERED WAY because no 
**		additional search needed.  At the completion of the function,
**		the information block is unfixed.  (This works only for non-
**		procedures.  See option #2 for DB procedures).
**
**	    2)  Invalidate a specified DB procedure infoblk.  This is currently
**		the only way to invalidate a DB Procedrue cache object.  The
**		caller must have the object fixed and supply a pointer to that
**		object in rdf_rb.rdr_procedure.  The caller must also specify
**		that this a a procedure by setting bit RDR_PROCEDURE in 
**		rdf_rb.rdr_types_mask.  Of course, the user must also specify
**		the unique database identifier in rdf_rb.rdr_unique_dbid
**		and the rdr_rb.rdr_fcb.  At the completion of the function,
**		the procedure information block is unfixed.
**
**	    3)	The user who does not have access to the table info block
**		can invalidate it through the table id (plus database id).
**		To use this option, the rdf_cb.rdf_info_blk must be set to NULL
**		and the rdf_rb.rdr_tabid and rdf_rb.rdr_unique_dbid fields
**		must be correctly initialized, and rdr_rb.rdr_2types_mask must
**		NOT have the following bits set:  RDR2_CLASS or RDR2_ALIAS.
**		Also, rdr_db_id must not be NULL.
**		The information block is not unfixed (since the user does not 
**		access to it to begin with).  Futhermore, if the user specifies
**		the RDR2_KILLTREES bit in rdr_rb.rdr_2types_mask, then RDF
**		will build class names for any integrities, permits or rules
**		on the cache and invalidate all cached trees assocaited with the
**		specified table id.
**
**	    4)  Remove a single QTREE cached integrity/permit/rule tree by
**		specifying the alias to that object.  An alias is comprised of
**		a) to specify a permit/integrity: 
**		    unique table identifier (i4)
**		    integrity/rule sequence number (i4)
**		    tree_type (i4 - DB_PROTECT, DB_INTG)
**		    database id (i4)
**		b) to specify a rule:
**		    rule name   (DB_MAXNAME)
**		    rule owner  (DB_MAXNAME)
**		    database id (i4)
**		To use this option, the rdf_cb.rdf_info_blk must be set to NULL
**		and the RDR2_ALIAS bit must be set in rdf_rb.rdr_2types_mask,
**		and the rdr_rb.rdr_unique_dbid and rdr_rb.rdr_fcb must be 
**		supplied.  In addition, the following fields must be correctly 
**		initialized to describe the object:
**		For Permits/integrities;
**		    rdr_tabid (unique table id of object receiving permit/integ)
**		    rdr_types_mask (bit RDR_PROTECT for protect or 
**				    bit RDR_INTEGRITIES for integrity)
**		    rdr_sequence (permit/integrity identifier: 1st=1, 2nd=2,etc)
**		for Rules:
**		    rdr_name.rdr_rlname (contains rule name)
**		    rdr_owner (contains rule owner)
**		    rdr_types_mask (bit RDR_RULE for rule)
**		The caller is responsible to unfix the QTREE cache object via
**		an RDF_UNFIX call.  The order of the RDF_INVALIDATE and RDF_UNFIX
**		calls do not matter.
**	    
**	    5)  Remove all cached integrity or permit or rule trees by 
**		specifying the class that these objects are cached in.
**		The class specifier is comprised of:
**		    unique table identifier (i4)
**		    tree_type (i4 - DB_RULE, DB_PROTECT, DB_INTG)
**		    database id (i4)
**		To use this option, the rdf_cb.rdf_info_blk must be set to NULL
**		and the RDR2_CLASS bit must be set in rdf_rb.rdr_2types_mask,
**		and the rdr_rb.rdr_unique_dbid and rdr_rb.rdr_fcb must be 
**		supplied.  In addition, the following fields must be correctly 
**		initialized to describe the object:
**		For Permits/integrities;
**		    rdr_tabid (unique table id of object receiving permit/integ)
**		    rdr_types_mask (bit RDR_PROTECT for protect or 
**				    bit RDR_INTEGRITIES for integrity or
**				    bit RDR_RULE for rule)
**		The caller is responsible to unfix any fixed QTREE cache objects
**		via RDF_UNFIX calls.  The order of the RDF_INVALIDATE and 
**		RDF_UNFIX calls do not matter.
**
**	    6)	The entire QTREE cache (contains integrity, permit and 
**		rule trees and DB procedures) can be invalidated at once by 
**		setting both the rdf_cb.rdf_info_blk and the rdf_rb.rdr_db_id
**		to NULL and setting the RDR2_QTREE_KILL bit in 
**		rdr_rb.rdr_2types_mask.  The caller is responsible to assure
**		that any objects they have fixed on that cache are unfixed via
**		a rdf_call with the RDF_UNFIX operation code.  
**		NOTE: this operation is quite expensive and should only be used
**		      when there is no other option.
**	
**
**          7)  The entire ldbdesc cache can be invalidated by setting both the
**              the rdf_cb.rdf_info_blk and the .rdf_rb.rdr_db_id to NULL, and
**              setting the RDR2_INVL_LDBDESC bit in rdr_rb.rdr_2types_mask.
**              NOTE: this operation is quite expensive and should only be used
**                    when there is no other option, or as a result of trace
**                    point processing.
**
**          8)  The entire defaults cache can be invalidated by setting both the
**              the rdf_cb.rdf_info_blk and the .rdf_rb.rdr_db_id to NULL, and
**              setting the RDR2_INVL_DEFAULT bit in rdr_rb.rdr_2types_mask.
**              NOTE: this operation is quite expensive and should only be used
**                    when there is no other option, or as a result of trace
**                    point processing.
**
**          9)  The entire cache can be invalidated at once by setting both
**		the rdf_cb.rdf_info_blk and the .rdf_rb.rdr_db_id to NULL. 
**		If this option is used, the user (or users) must issue the
**		rdf_unfix for each info block acquired.  The order of the
**		calls is not important.  THIS IS NOT THE PREFERRED WAY TO
**		CLEAN UP THE RDF CACHE, AS IT IS VERY EXPENSIVE!
**
** Inputs:
**
**      rdf_cb                          
**	    .rdf_info_blk		Pointer to the table info block to be
**					invalidated.  If NULL, RDF will use 
**					da id + table id to locate the info
**					block.
**
**	    .rdf_rb			RDF request block containing:
**		.rdr_fcb		Pointer to RDF facility control block
**
**		.rdr_db_id		If the rdr_info_blk is NULL, this field
**					specifies the DMF database id.  If this
**					field is also NULL, THE ENTIRE CACHE
**					WILL BE INVALIDATED.
**
**		.rdr_unique_dbid	Unique (ie infodb) db id used when
**					invalidating things by id.
**
**		.rdr_tabid		Table id if the info block is to be
**					located by id.
**
**		.rdr_procedure		Pointer to fixed procedure infoblk
**					(used only to invalidate procedures)
**
**		.rdr_types_mask		Bitmap instructions to specify that the
**					invalidate target is a rule (RDR_RULE),
**					integrity (RDR_INTEGRITIES) or a pemit
**					(RDR_PERMITS).  Or, if a procedure cache
**					object is specified, RDR_PROCEDURE.
**
**		.rdr_2types_mask	Bitmap instructions for invalidating
**					permit/integrity or rule trees.
**					RDR2_ALIAS -> invalidate a single tree
**					RDR2_CLASS -> invalidate all assocaited 
**						      trees (in all in a class)
**		.rdr_name		name of target object (a union)
**		    .rdr_rlname		name of the target rule to invalidate
**		.rdr_sequence		unique identifier for a permit or
**					integrity from DB_PROTECTION.dbp_permno
**					or DB_INTEGRITY.dbi_number
**
** Outputs:
**      rdf_cb                          
**					
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**		.rdf_info_blk		Pointer to the table information
**					block may be cleared to null.
** History:
**	12-jan-87 (puree)
**	    initial version.
**	11-may-87 (puree)
**	    add field rdr_unique_dbid for cache id.
**	29-aug-88 (seputis)
**	    rewritten for resource management, procedures, casting problems
**	30-sep-88 (seputis)
**	    pass stream id to rdu_dstream
**	15-dec-89 (teg)
**	    add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**	    routines to use.
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	22-aug-90 (teresa)
**	    assure that error status returned when clearing object desc cache
**	    and query tree cache for PSF.  It was possible to get error cleaing
**	    object desc cache, successfully clear query tree cache and then
**	    return a success status to PSF.
**	31-jan-91 (teresa)
**	    add statistic keeping logic
**	16-mar-92 (teresa)
**	    added logic to invalidate QTREE cache object and secondary index
**	    objects.  Rewrote to break logic into applicable subroutines.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil project.
**	16-jul-92 (teresa)
**	    prototypes
**	6-aug-92 (teresa)
**	    changed interface for procedure invalidation to accept name and
**	    owner of procedure
**	24-feb-93 (teresa)
**	    Added support to invalidate the DEFAULTS cache.
**	07-feb-1997 (canor01)
**	    If there are multiple servers in the installation, signal
**	    the RDF cache synchronization event.
**	31-Aug-2006 (jonj)
**	    ...But don't broadcast invalidates of Temp Tables - they can't
**	    possibly exist elsewhere.
**	    When invalidating a specific table on behalf of an alert
**	    from another server, dmf_call(DMT_INVALIDATE_TCB)
**	    to release any cached TCBs for the table.
*/

DB_STATUS
rdf_invalidate(	RDF_GLOBAL	*global,
		RDF_CB		*rdfcb)
{
    RDR_INFO	    *infoblk;
    RDI_FCB	    *rdi_fcb;
    DB_STATUS	    status = E_DB_OK;
    RDF_ULHOBJECT   *rdf_ulhobject;
    struct {
	i4		db_id;
	DB_DBP_NAME	prc_name;
	DB_OWN_NAME	prc_owner;
    } obj_alias;		    /* structure used to define an alias
				    ** by name to the object */

    /*
    ** Future Consideration:
    ** Currently rdf obj is invalidated/refereshed for DDL pst_showtab().
    ** Perhaps invalidating rdf cache for the database upon
    ** rollback/force abort if DDL was performed in transaction.
    ** May have to be done inside DMF. Invalidating entire cache may
    ** be expensive and overkill.  Also need to think about concurrency
    ** issues (e.g. RDF_GETINFO doesn't fix anything, what if someone
    ** just did a getinfo and someone else zaps the rdf cache?)
    */

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */
    rdi_fcb = (RDI_FCB *)(rdfcb->rdf_rb.rdr_fcb);
    global->rdfcb = rdfcb;

    /*
    ** rdi_fcb will be NULL only if the call originates outside
    ** of a facility.  At the moment, that means triggered from 
    ** an event. 
    **
    ** Not true - NULL really means "don't broadcast".
    */

    CLRDBERR(&rdfcb->rdf_error);

    do /* one-time loop to break out of */
    {
        if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
        {
	    /* case 2 - invalidate the procedure cache entry */
	    DB_STATUS	proc_status;
    
	    /* set up the procedure hash table */
	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
					
	    if (rdfcb->rdf_rb.rdr_procedure)
	    {   
	        /* procedure is fixed, so use ptr to it */
	        global->rdf_ulhcb.ulh_object = 
			(ULH_OBJECT *)rdfcb->rdf_rb.rdr_procedure->rdf_ulhptr;
	    }
	    else
	    {   /* procedure is not fixed, so fix it and get ptr */

	        /* Try to get descriptor by alias */
	        obj_alias.db_id = rdfcb->rdf_rb.rdr_unique_dbid;
	        STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_prcname, 
				    obj_alias.prc_name);
	        STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner,
				    obj_alias.prc_owner);
	        status = ulh_getalias(&global->rdf_ulhcb, (u_char *)&obj_alias, 
				      sizeof(obj_alias));
	        if (DB_FAILURE_MACRO(status))
	        {
		    /* we could not find the procedure object on the cache */
		    if((global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
		        &&
		    (global->rdf_ulhcb.ulh_error.err_code != E_UL0121_NO_ALIAS))
		    {
		        rdu_ferror(global, status, 
			           &global->rdf_ulhcb.ulh_error,
			           E_RD012D_ULH_ACCESS,0);
		        break;
		    }
		    /* object was not on the cache, so no need to invalidate it */
		    status = E_DB_OK;
		    break;
	        }		
	    }

	    proc_status = ulh_destroy(&global->rdf_ulhcb, (u_char *)NULL, 0);
	    if (DB_FAILURE_MACRO(proc_status))
	    {
	        status = proc_status;
	        rdu_ferror(global, status, 
		           &global->rdf_ulhcb.ulh_error, 
		           E_RD0040_ULH_ERROR,0);
	    }
	    rdfcb->rdf_rb.rdr_procedure = NULL;
	    Rdi_svcb->rdvstat.rds_i2_p_invalid++;
        }
        else if (rdfcb->rdf_info_blk != NULL)
        {    
	    /*  Case 1: Invalidate the info block pointed by rdf_info_blk */
	    status = invalidate_relcache(global, rdfcb, rdfcb->rdf_info_blk, 
				         (DB_TAB_ID *) NULL);
	    /* indicate infoblk has been unfixed by clearing the ptr. */
            if(rdfcb->caller_ref && (*rdfcb->caller_ref == rdfcb->rdf_info_blk))
                *rdfcb->caller_ref = NULL;
	    rdfcb->rdf_info_blk = NULL;
        }
        else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_ALIAS)
        {
	    /* Case 4: Destroy a specified tree */
	    i4	tree_type;
	    char	alias_name[ULH_MAXNAME];
	    i4	alias_len=0;
    
	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT)
	    tree_type = DB_PROT;
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	        tree_type = DB_INTG;
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_RULE)
	        tree_type = DB_RULE;
	    else
	    {
	        status = E_DB_ERROR;
	        rdu_ierror(global, status, E_RD0148_BAD_QTREE_ALIAS);	    
	        break;
	    }

	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	    global->rdf_ulhcb.ulh_object = (ULH_OBJECT *) NULL;
	    rdu_qtalias(global, tree_type, alias_name, &alias_len);
	    status = ulh_getalias(&global->rdf_ulhcb, (u_char *)alias_name,
				    alias_len);
	    if (DB_FAILURE_MACRO(status))
	    {
	        /* the object did not exist on the cache.  This is not an error
	        ** unless there was an unexpected error code returned */

	        if((global->rdf_ulhcb.ulh_error.err_code == E_UL0109_NFND)
		    ||
		    (global->rdf_ulhcb.ulh_error.err_code == E_UL0121_NO_ALIAS))
	        {
		    /* the object was not on the cache to invalidate.  This is NOT
		    ** an error */
		    status = E_DB_OK;
    
		    /* update statistics */
		    Rdi_svcb->rdvstat.rds_i3_flushed++;
	        }
	        else
	        {
		    rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		        E_RD012D_ULH_ACCESS,0);
		    break;
	        }
	    }
	    else
	    {
	        /* ok, we got the tree, now destroy it */
	        status = ulh_destroy(&global->rdf_ulhcb, (u_char *) NULL, 0);
	        /* update statistics */
	        if (DB_SUCCESS_MACRO(status))
		    Rdi_svcb->rdvstat.rds_i5_tree_invalid++;
    
	    }
        }
        else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_CLASS)
        {	
	    i4	tree_type;
	    u_i4	baseid;
    
	    /* Case 5: Destroy all trees of the specified type (permit, rule or
	    **	   integrity) that are associated with this object 
	    */
	    if (rdfcb->rdf_rb.rdr_types_mask & RDR_PROTECT)
	        tree_type = DB_PROT;
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	        tree_type = DB_INTG;
	    else if (rdfcb->rdf_rb.rdr_types_mask & RDR_RULE)
	        tree_type = DB_RULE;
	    else
	    {
	        status = E_DB_ERROR;
	        rdu_ierror(global, status, E_RD0147_BAD_QTREE_CLASS);	    
	        break;
	    }
	    status = rdr_killtree(global, tree_type, 0);
    
        }
        else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_QTREE_KILL )
        {
	    /* case 6: invalidate the qtree cache */
            if (rdi_fcb == NULL)
            {
	        status = E_DB_ERROR;
	        rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	        return(status);
            }
	    if (rdi_fcb->rdi_fac_id == DB_PSF_ID)
	    {   /* this cache is only defined for the parser and not the
	        ** optimizer to check that it exists */
	        global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
					 /* set up the procedure hash table */
	        status = ulh_clean(&global->rdf_ulhcb);
	        if (DB_FAILURE_MACRO(status))
		    rdu_ferror(global, status, 
			        &global->rdf_ulhcb.ulh_error,
			        E_RD0040_ULH_ERROR,0);
	        else
		    Rdi_svcb->rdvstat.rds_i6_qtcache_invalid++;
	    }
        }
        else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_INVL_LDBDESC )
        {
            /* case 7: invalidate the entire LDBdesc cache.  But only do this if
	    **	   this is a distributed thread and the LDBdesc cache has been
	    **	   defined. Detect existance of LDBdesc cache by a non-zero
	    **	   rdv_dist_hashid.
	    */
	    if (Rdi_svcb->rdv_dist_hashid)
	    {
	        /* cache exists, so invalidate it */
	        global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_dist_hashid;
	        status = ulh_clean(&global->rdf_ulhcb);
	        if (DB_FAILURE_MACRO(status))
		    rdu_ferror(global, status,
			        &global->rdf_ulhcb.ulh_error,
			        E_RD0040_ULH_ERROR,0);
	        else
		    Rdi_svcb->rdvstat.rds_i8_cache_invalid++;
	    }
	    else
	        /* make this a harmless no-op */
	        status=E_DB_OK;
        }
        else if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_INVL_DEFAULT )
        {
            /* case 8: invalidate the entire DEFAULTS cache.  But only do this if
	    **	   this is a non-distributed thread and the defaults cache has 
	    **	   been defined. Detect existance of DEFAULTS cache by a 
	    **	   non-zero rdv_def_hashid.
	    */
	    if (Rdi_svcb->rdv_def_hashid)
	    {
	        /* cache exists, so invalidate it */
	        global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_def_hashid;
	        status = ulh_clean(&global->rdf_ulhcb);
	        if (DB_FAILURE_MACRO(status))
		    rdu_ferror(global, status,
			        &global->rdf_ulhcb.ulh_error,
			        E_RD0040_ULH_ERROR,0);
	        else
		    Rdi_svcb->rdvstat.rds_i8_cache_invalid++;
	    }
	    else
	        /* make this a harmless no-op */
	        status=E_DB_OK;
        }
        else if (rdfcb->rdf_rb.rdr_db_id != NULL)
        {
	    /* Case 3: invalidate the info block specified by unique db id
	    ** + table id */
	    status = invalidate_relcache(global, rdfcb, (RDR_INFO *) NULL,
				        &rdfcb->rdf_rb.rdr_tabid);
	    /* indicate infoblk has been unfixed by clearing the ptr. */
            if(rdfcb->caller_ref && (*rdfcb->caller_ref == rdfcb->rdf_info_blk))
                *rdfcb->caller_ref = NULL;
	    rdfcb->rdf_info_blk = NULL;

	    /* 
	    ** If a temp table, don't broadcast alert; this
	    ** table can't possibly exist elsewhere.
	    */
	    if ( rdfcb->rdf_rb.rdr_tabid.db_tab_base < 0 )
		rdi_fcb = NULL;
	    /*
	    **
	    ** If triggered by an alert from another server (rdr_db_id == 1)
	    ** call dm2t to release its cached TCB, if any.
	    */
	    else if ( rdi_fcb == NULL && rdfcb->rdf_rb.rdr_db_id == (PTR)1 )
	    {
		DMT_CB	dmt_cb;

		dmt_cb.type = DMT_TABLE_CB;
		dmt_cb.length = sizeof(DMT_CB);
		dmt_cb.dmt_unique_dbid = rdfcb->rdf_rb.rdr_unique_dbid;
		dmt_cb.dmt_id = rdfcb->rdf_rb.rdr_tabid;
		(VOID)dmf_call(DMT_INVALIDATE_TCB, &dmt_cb);
	    }
        }
        else
        {   /*  Case 9: Invalidate the entire cache */
    
	    DB_STATUS   qt_status = E_DB_OK;
    
	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	    status = ulh_clean(&global->rdf_ulhcb);
	    if (DB_FAILURE_MACRO(status))
	        rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error, 
		    E_RD0040_ULH_ERROR,0);
	    else
	        Rdi_svcb->rdvstat.rds_i7_cache_invalid++;

            if (rdi_fcb == NULL)
            {
	        status = E_DB_ERROR;
	        rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	        return(status);
            }
	    if (rdi_fcb->rdi_fac_id == DB_PSF_ID)
	    {   /* this cache is only defined for the parser and not the
	        ** optimizer to check that it exists */
	        global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
					   /* set up the procedure hash table */
	        qt_status = ulh_clean(&global->rdf_ulhcb);
	        if (DB_FAILURE_MACRO(qt_status))
		    rdu_ferror(global, qt_status, 
			        &global->rdf_ulhcb.ulh_error,
			        E_RD0040_ULH_ERROR,0);
	        else
		    Rdi_svcb->rdvstat.rds_i6_qtcache_invalid++;

	        /* preserve the worst of the errors, in the case where there were
	        ** errors clearing the main cache and also errors clearing the
	        ** Query Tree cache.
	        */
	        if (qt_status > status)
		    status = qt_status;
	    }
        }
    } while (FALSE);

    /*
    ** if invalidate was successful, and the call was not as a
    ** result of an event or is explicitly not to be broadcast
    ** (rdi_fcb == NULL) inform all other servers in
    ** the installation that they need to coordinate their caches
    */
    if ( status == E_DB_OK && rdi_fcb != NULL )
    {
	status = rdf_inv_alert( &rdfcb->rdf_rb );
    }

    return(status);
}

/*{
** Name: infoblk_invalidate  -- Invalidate the info block pointed by
**				rdf_info_blk
**
** Description:
**	This routine invalidates a fixed infoblk by calling ulh_destroy().
**	If the infoblk was a private copy, it also releases the memory for
**	the private infoblk by calling rdu_dstream().
**
** Inputs:
**	global			    thread local variables memory area
**	infoblk			    Cache object ptr
**	    .stream_id		    memory stream identifier used to allocate
**					cache object
**	    .rdr_object		    book keeping info on cache object
**		.rdf_state	    indicates state of the object
**		.rdf_ulhptr	    ptr to ulh control block
**		.rdf_sysinfoblk	    ptr to system copy of cache object
** Outputs:
**	 svcb		    	    RDF Server Control Block
**	    .rdvstat	    	    RDF statistics 
**		.rds_i1_obj_invalid object invalidation statistics
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	03-apr-92 (teresa)
**	    Initial creation.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil project.
**	16-jul-92 (teresa)
**	    prototypes
**	30-jul-97 (nanpr01)
**	    Set the streamid to NULL after the memory is deallocated.
**	07-Aug-1997 (jenjo02)
**	    Set the streamid to NULL BEFORE the memory is deallocated.
**	15-Mar-2004 (schka24)
**	    Comment update (no code).
**	6-Feb-2008 (kibro01) b114324
**	    Added rdu_drop_attachments to close attachments from this infoblk
**	19-Oct-2009 (wanfr01) B122755
**	    rdu_drop_attachments should pass 0 to req_sessions, not 1.
**	    It is compared against rdf_shared_sessions, which is the number
**	    of sessions sharing the object.  You don't want to destroy the
**	    object if someone else (even if it's just one) is using it
*/
static DB_STATUS
infoblk_invalidate( RDF_GLOBAL	*global,
		    RDR_INFO	*infoblk)
{

	RDR_INFO	    *sysinfoblk;
	DB_STATUS	    status,temp_status;
        bool		    is_private;
	PTR		    stream_id;
	ULH_RCB		    *ulh_rcb;
	RDF_ULHOBJECT       *old_glob_object;

	sysinfoblk = ((RDF_ULHOBJECT *)infoblk->rdr_object)->rdf_sysinfoblk;

	/* Set ulh cb object pointer by traverse through master info block,
	** in case our object is private (no ulhptr in private object).
	*/
	global->rdf_ulhcb.ulh_object = 
		((RDF_ULHOBJECT *)sysinfoblk->rdr_object)->rdf_ulhptr;

        is_private = ((RDF_ULHOBJECT *)infoblk->rdr_object)->rdf_state == 
                                       RDF_SPRIVATE;

	ulh_rcb = &global->rdf_ulhcb;

	old_glob_object = global->rdf_ulhobject;
	global->rdf_ulhobject = (RDF_ULHOBJECT *)sysinfoblk->rdr_object;
        if ( global->rdf_ulhobject &&
             ulh_rcb->ulh_hashid == Rdi_svcb->rdv_rdesc_hashid
           )
        {
	    status = rdu_drop_attachments(global, infoblk, 0);
	}
	global->rdf_ulhobject = old_glob_object;

	/* call ULH to invalidate and unfix this infoblk */
	temp_status = ulh_destroy(&global->rdf_ulhcb, (u_char *)NULL, 0);

	/* 
	** if this is a private infoblk, then destroy the memory stream 
	*/

        if (is_private)
        {
            /* this is a private copy so destroy the stream */
	    stream_id = (PTR)infoblk->stream_id;
	    infoblk->stream_id = NULL;
            status = rdu_dstream(global, (PTR)stream_id,
                                 (RDF_STATE *)NULL);
        }
        else
        {
            status = E_DB_OK;
        }

	if (DB_FAILURE_MACRO(temp_status))
	{
	    status = temp_status;
	    rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error, 
		E_RD0040_ULH_ERROR,0);
	}

	/* update statistics */
	Rdi_svcb->rdvstat.rds_i1_obj_invalid++;

	return(status);
}

/*{
** Name:    NAME_INVALIDATE - invalidate the object by name
**
** Description:
**	Invalidate the cache object for the specified table identifier.
**
** Inputs:
**	global			    thread local variables memory area
**	tabid			    table unique identifier (used to buld name)
**
** Outputs:
**	global			    thread local variables memory area
**	    .rdf_ulhcb		    ULH Control Block
**		.ulh_object	    ptr to ULH object (ptr is nulled)
**	svcb		    	    RDF Server Control Block
**	    .rdvstat	            RDF statistics 
**	        .rds_i1_obj_invalid object invalidation statistics
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	03-apr-92 (teresa)
**	    Initial creation.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil project.
**	16-jul-92 (teresa)
**	    prototypes
*/
static DB_STATUS
name_invalidate(RDF_GLOBAL	*global,
		DB_TAB_ID	*tabid)
{

	DB_STATUS	    status;


	global->rdf_ulhcb.ulh_object = (ULH_OBJECT *) NULL;
	rdu_setname(global, tabid);
	status = ulh_destroy(&global->rdf_ulhcb,
	    (u_char *)&global->rdf_objname[0], 
	    global->rdf_namelen);
	if (DB_FAILURE_MACRO(status))
	{
	    if (global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
		rdu_ferror( global, status, 
			    &global->rdf_ulhcb.ulh_error, 
			    E_RD0040_ULH_ERROR,0);
	    else
	    {
		/* its ok if object is not on the cache */
		status = E_DB_OK;
	    }
	}
	else
	    Rdi_svcb->rdvstat.rds_i1_obj_invalid++;
	return status;
}

/*{
** Name:    rdr_killtree - kill a tree class
**
** Description:
**	this routine constructs the class name for a tree class and then
**	calls a ULH_DEST_CLASS to destroy all cache objects in that tree's
**	class.
** Inputs:
**	tree_type		    Type of tree:
**					protection tree: DB_PROT
**					integrity tree:  DB_INTG
**					rule tree:	 DB_RULE
**	id			    identifier of protection or integrity tree
**				    (unique identifier for given table id receiving
**				    the tree)
**	svcb		    	    server control block
**	    .rdv_qtree_hashid       hash id for Qtree cache
**
** Outputs:
**	svcb		    	    RDF Server Control Block
**	    .rdvstat	    	    RDF statistics 
**		.rds_i1_obj_invalid object invalidation statistics
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	03-apr-92 (teresa)
**	    Initial creation.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil project.
**	16-jul-92 (teresa)
**	    prototypes
*/
DB_STATUS
rdr_killtree(
	RDF_GLOBAL	*global,
        i4		tree_type,
        u_i4		id)
{	
    char	class_name[ULH_MAXNAME];
    i4		class_len=0;
    RDI_FCB	*rdi_fcb = (RDI_FCB *)global->rdfcb->rdf_rb.rdr_fcb;
    DB_STATUS	status;

    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
    global->rdf_ulhcb.ulh_object = (ULH_OBJECT *) NULL;
    rdu_qtclass(global, id, tree_type, class_name, &class_len);
    status = ulh_dest_class(&global->rdf_ulhcb, (u_char *)class_name,
	class_len);
    if (DB_FAILURE_MACRO(status))
    {
	if (global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
	    rdu_ferror(global, status, 
			&global->rdf_ulhcb.ulh_error, 
			E_RD0040_ULH_ERROR,0);
	else
	    /* it is OK if the class is not currently on the cache */
	    status = E_DB_OK;
    }
    else
	Rdi_svcb->rdvstat.rds_i4_class_invalid++;
    return(status);
}

/*{
** Name: invalidate_relcache - invalidate a relation cache entry and any 
**				associated objects
**
** Description:
**	invalidate a relation cache entry and any associated objects
**
** Inputs:
**	global			    thread local variables memory area
**	rdfcb			    RDF Control Block
**	infoblk			    ptr to a cache object
**	obj_id			    unique table identifier for cache object
** Outputs:
**	global			    thread local variables memory area
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	03-apr-92 (teresa)
**	    Initial creation.
**	06-jul-92 (teresa)
**	    fix concurrancy bug where RDF could not handle ulh_access not
**	    finding the cache object because it had been flushed to make space
**	    for another object.
**	07-jul-92 (teresa)
**	    update statistics reporting for sybil project.
**	16-jul-92 (teresa)
**	    prototypes
**	08-Nov-93 (teresa)
**	    Handle ICL bug 56471.  This bug is caused because repeat queries
**	    must invalidate the rdf objects associated with a query and refresh
**	    with current data (there is no retry mechanism for repeat queries
**	    because SCF must always get the repeat query text from the FE and
**	    cannot keep track of the fact that QEF invalidated an object in the
**	    query plan).  This leads to concurrency problems where one thread
**	    has began to build an infoblk and has gotten the relation data but
**	    has not completed the index data.  The second thread comes along and
**	    finds the partially completed infoblk on the cache and invalidates
**	    because rdr2_types_mask.RDR2_REFRESH is set.  (This problem would
**	    not exist if we could hold semaphores accross DMF calls.)  Anyhow,
**	    the second thread finds there are secondary indexes from the
**	    relation information (iirelation.relidxcount),  so it tries to walk
**	    the array of iiindex tuples that either has not been allocated or
**	    has been allocated but has not yet been fully populated.  In either
**	    case, bug 56471 AVs when it comes to an invalid pointer in array
**	    infoblk->rdr_indx[].  Ideally this should be fixed by changing 
**	    rdu_cache() to realize this is a partially built infoblk and ignore
**	    the RDR2_REFRESH instruction and build a private infoblk.  But this
**	    gets tricky and may be more destabilizing that theoraputic.  So, we
**	    handle invalidation of partially created infoblks here by being aware
**	    that the infoblk->rdr_indx array may not be fully initialized and
**	    testing for valid addresses before trying to dereference them.  We
**	    cannot afford to get rid of walking infoblk->rdr_indx[] all together
**	    because it may be that there is stale secondary index infoblks on
**	    the cache that cause QEF to invalidate repeat querry plans.  So we
**	    walk as much as we have and do the best that we can do with it...
**	    ALSO, the logic to walk the index array was incorrect and unduely
**	    complicated by trying to save the index info in a static array in
**	    case RDF had been asked to invalidate from tableid instead of from
**	    fixed infoblk.  RDF was saving this because it had to unfix the 
**	    infoblk before dealing with the secondary indexes.  This was due to
**	    RDF's error cleanup routine rdf_release() [in rdfcall.c] being unable
**	    to handle two objects fixed at the same time.  So, I changed the 
**	    cleanup logic to handle a fixed relation infoblk (by adding field
**	    global->rdf_fix_toinvalidate and bit RDF_INVAL_FIXED to RDF_RTYPES.
**	    Now RDF does not have to unfix the infoblk before invalidating any
**	    associated secondary indexes.
**
** History:
**	04-feb-1996 (canor01)
**	    Remove unused reference to RDI_FCB.
**	14-aug-1998 (nanpr01)
**	    Since we are looking at uptr without a semaphore protection,
**	    it is possible in a concurrent environment to see it nil. In that
**	    case we must retry before spitting out RD0045 error.
**	03-sep-1998 (nanpr01)
**	    Another shot at the RD0045 problem to protect it rather than
**	    retry logic.
**      27-Jan-2010 (hanal04) Bug 123180
**          Prevent E_RD012C errors. Do not unset RDF_RULH until we have 
**          release the ULH object.
**          
*/
DB_STATUS
invalidate_relcache(
	    RDF_GLOBAL	*global,
	    RDF_CB	*rdfcb,
	    RDR_INFO	*infoblk,
	    DB_TAB_ID	*obj_id)
{

    	bool		    ck_index = FALSE;
	RDF_ULHOBJECT	    *rdf_ulhobject;
	u_i4		    baseid;
	DB_STATUS	    ulh_status,status;

	/* specify relation cache */
	global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;

    	if (! infoblk)
	{
	    /* 
	    ** If the caller did not fix the infoblk, then go ahead and fix it
	    ** so we can extract information that we may need
	    */
	    _VOID_ rdu_setname(global, obj_id);
	    status = ulh_access(&global->rdf_ulhcb,
				(u_char *)&global->rdf_objname[0],
				global->rdf_namelen);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (global->rdf_ulhcb.ulh_error.err_code == E_UL0109_NFND)
		{
		    /* the object has already been flushed from the cache --
		    ** probably ULH removed it from LRU to make room for another
		    ** cache object.
		    **
		    ** So, there is no work to do.  Just exit.
		    */
		    Rdi_svcb->rdvstat.rds_i3_flushed++;
		    return (E_DB_OK);
		}
		else
		{
		    /* unantisipated ULH error.  Report and Abort */
		    rdu_ferror( global, status, 
				&global->rdf_ulhcb.ulh_error,
				E_RD0142_ULH_ACCESS,0);
		    return(status);
		}
	    }
	    /* mark resource as fixed and keep track of it in local structures
	    ** in case recovery [rdf_release()] needs to clean it up later 
	    */
	    if  (global->rdf_ulhcb.ulh_object) 
	    {
		/* get the semaphore so that we see ulh_uptr consistent */
		global->rdf_resources |= RDF_RULH;
		status = rdu_gsemaphore(global);
		rdf_ulhobject = 
		    (RDF_ULHOBJECT *)global->rdf_ulhcb.ulh_object->ulh_uptr;
		/* 
		** Now release it because it is fixed and going 
		** to be removed
		*/
		status = rdu_rsemaphore(global);

		global->rdf_ulhobject = rdf_ulhobject; 
						    /* save ptr to object */
		if (rdf_ulhobject && rdf_ulhobject->rdf_sysinfoblk)
		{
		    global->rdf_resources |= RDF_INVAL_FIXED;
		    global->rdf_fix_toinvalidate =
					    rdf_ulhobject->rdf_sysinfoblk;
		    infoblk = rdf_ulhobject->rdf_sysinfoblk;
		    global->rdfcb->rdf_info_blk = infoblk; 
					    /* rdf error handling
					    ** may need this initialized */
		}
		else
		{
		    /* 
		    ** should have sys_infoblk. This case is unlikely. But
		    ** if we hit hit, then act like the object has been 
		    ** flushed from cache, and release the fixed infoblk 
		    */
		    global->rdf_init |= RDF_IULH;
		    ulh_status = rdu_rulh(global);
		    global->rdf_init &= ~RDF_IULH;
		    global->rdf_resources &= ~RDF_RULH;
		    if (DB_FAILURE_MACRO(ulh_status))
		    {
			rdu_ferror( global, ulh_status, 
				&global->rdf_ulhcb.ulh_error,
				E_RD011A_ULH_RELEASE, 0);
			if (DB_SUCCESS_MACRO(status))
			    status = ulh_status;
		    }
		    Rdi_svcb->rdvstat.rds_i3_flushed++;
		    return(status);
		}
		global->rdf_init &= ~RDF_IULH;
		global->rdf_resources &= ~RDF_RULH;
	    }
	    else
	    {
		/* we got back a weird ulh object, so treat it like an error
		** DO NOT TRY TO FREE RESOURCE, OR WE WILL AV
		*/
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0045_ULH_ACCESS);
	    	return (status);
	    }
	}
	else
	    /* use the caller supplied infoblk data */
	    rdf_ulhobject = (RDF_ULHOBJECT *) infoblk->rdr_object;
	    
	do	    /* control loop */
	{
	    DMT_TBL_ENTRY	*relptr;

	    relptr = infoblk->rdr_rel;
	    if (relptr)
	    {
		/* 
		** the object exists, and has relation info 
		*/

		if (rdfcb->rdf_rb.rdr_2types_mask & RDR2_KILLTREES)
		{
		    /* if there are permit, integrity or rule trees, zap them */
		    /* This test for tab-index is probably formulaic,
		    ** for sure we want to exclude partitions
		    */
		    baseid = (relptr->tbl_id.db_tab_index>0)  ?
				relptr->tbl_id.db_tab_index :
				relptr->tbl_id.db_tab_base;


		    if (relptr->tbl_status_mask & DMT_PROTECTION)
		    {
			status = rdr_killtree(global, DB_PROT, baseid);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		    if (relptr->tbl_status_mask & DMT_INTEGRITIES)
		    {
			status = rdr_killtree(global, DB_INTG, baseid);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		    if (relptr->tbl_status_mask & DMT_RULE)
		    {
			status = rdr_killtree(global, DB_RULE, baseid);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		} /* end if KILLTREES */

		global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;

	    }
	    else
	    {
		/* the object was not on the cache and we just got it, or
		** the caller somehow was given access to an infoblk that was
		** not correctly initialized.  Unfix this object and invalidate
		** it, and return.
		**
		**  There is an assumption here that RDF never gives PSF/OPF
		**  an infoblk ptr without atleast RELATION information in it.
		*/

		global->rdf_ulhobject = rdf_ulhobject; /* save ptr to object */
		if (rdf_ulhobject)
		    global->rdfcb->rdf_info_blk = rdf_ulhobject->rdf_sysinfoblk;
		global->rdf_init &= ~RDF_IULH;
		ulh_status = rdu_rulh(global);
		if (DB_FAILURE_MACRO(ulh_status))
		{
		    rdu_ferror( global, ulh_status, 
				&global->rdf_ulhcb.ulh_error,
				E_RD011A_ULH_RELEASE, 0);
		    if (DB_SUCCESS_MACRO(status))
			status = ulh_status;
		}
		/* unset ulh resource */
		global->rdf_resources &= ~RDF_RULH;  
		global->rdf_ulhobject = NULL;
		global->rdfcb->rdf_info_blk = NULL;
		return (status);
	    }

	    /* assure we are pointing back to the relation cache */
	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	    /* do any requested traces before zapping this infoblk */
	    rdu_master_infodump(infoblk, global, RDFINVALID,0);

	    /* loop through secondary indexes (if any) and assure they are not
	    ** on the cache --
	    ** call invalidate_relcache to invalidate any secondary index 
	    ** assocated with this table that may exist on the cache.  It is 
	    ** NOT an error condition if the secondary index object is NOT 
	    ** cached.
	    */

		/* determine whether or not to check for secondary indexes
		** to remove from the cache based on the following critieria:
		** 1.  table is not a secondary index
		** 2.  table is not a view
		** 3.  index count > 0
		** 4.  Memory has been allocated to hold the index array.
		*/		
	    if (
		   ((relptr->tbl_status_mask & (DMT_VIEW | DMT_IDX))==0)
		   && 
		   (infoblk->rdr_no_index != 0)
		   &&
		   (infoblk->rdr_indx)
		)
	    {
		register i4	i;
		for (i = 0; i < infoblk->rdr_no_index;  i++)
		{   
		    DB_TAB_ID	    idx_id;

		    if (infoblk->rdr_indx[i])
		    {
			/* invalidate the cache entry for this secondary index
			** if it exists 
			*/
			global->rdf_init &= ~RDF_INAME;
			STRUCT_ASSIGN_MACRO(infoblk->rdr_indx[i]->idx_id,
					    idx_id);
			status = name_invalidate(global, &idx_id);
			if (DB_FAILURE_MACRO(status))
			    return(status);
		    }
		    else
			/* the array of index ids was not completely built, and
			** we have hit the end, so stop processing  (bug 56471)
			**/
			break;

		} /* end loop */
	    }

	    /* now destroy the object */
	    if (global->rdf_resources & RDF_INVAL_FIXED)
	    {
		/* the infoblk_invalidate() call will unfix the infoblk that
		** we have fixed, so clear away the details that we have fixed
		** it for our earlier processing.
		*/
		global->rdf_resources &= ~RDF_INVAL_FIXED;
		global->rdf_fix_toinvalidate = 0;
	    }
	    global->rdf_init &= ~RDF_IULH;
	    status = infoblk_invalidate(global, infoblk);

	} while (0); /* end control loop */

	/*
	** if we broke on on an error condition having fixed an infoblk, then
	** rdf_release() will unfix this on the way out...
	*/
	return(status);
}

/*
** rdf_inv_alert - raise an alert on rdf_invalidate
**
** Description:
**      Raises the RDF cache synchronization event when there are
**	multiple servers in an installation.
**
** Inputs:
**      rdr_rb                      RDR_RB
**
** Outputs:
**	None
**
** Side Effects:
**      Raises RDF_INV_ALERT_NAME event, which should cause other
**	servers in installation to do the same rdf_invalidate.
**
** History:
**	07-feb-1997 (canor01)
**	    Created.
**      23-jul-1998 (rigka01)
**          set SCE_NOORIG so that alert does not get sent to alert
**          originator
**	17-Nov-2005 (schka24)
**	    Our session ID can't possibly be meaningful on other servers,
**	    send them "no session".
*/
static DB_STATUS
rdf_inv_alert( RDR_RB *rdr_rb )
{
    DB_STATUS	status;
    SCF_CB      scf_cb;
    SCF_ALERT   scfa;
    DB_ALERT_NAME aname;
    DB_DATA_VALUE tm;                                   /* Timestamp */
    DB_DATE     tm_date;
    PID	        pid;
    i4		dbms_servers;
    DB_SQLSTATE sqlstate;
    char	msg_buffer[DB_ERR_SIZE];
    i4	msg_buf_length;
    i4     ule_error;
# define RDF_SCE_DATASIZE  sizeof(PID)+sizeof(RDR_RB)
    char	rdf_passed_data[RDF_SCE_DATASIZE];

    /* 
    ** if this is the only server in the installation, 
    ** no need for further action.
    ** (Do not count the recovery server.)
    */
    if ( !rdf_ismultiserver() )
	return ( E_DB_OK );

    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_RDF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_alert_parms = &scfa;
 
    MEfill(sizeof(DB_ALERT_NAME), 0, &aname.dba_alert);
    STmove( RDF_INV_ALERT_NAME, ' ', sizeof(DB_NAME), 
	    aname.dba_alert.db_name );
    scfa.scfa_name = &aname;
    scfa.scfa_text_length = RDF_SCE_DATASIZE;
    PCpid(&pid);
    MECOPY_CONST_MACRO( &pid, sizeof(PID), rdf_passed_data );
    MEcopy( rdr_rb, sizeof(RDR_RB), rdf_passed_data+sizeof(PID) );
    scfa.scfa_user_text = (PTR) rdf_passed_data;
    scfa.scfa_flags = SCE_NOORIG; 
 
    /* this alert does not need the time stamp, so save a call to adf */
    MEfill(sizeof(tm_date), 0, &tm_date);
    scfa.scfa_when = &tm_date;

    status = scf_call( SCE_RAISE, &scf_cb );

    if ( status != E_DB_OK )
    {
	if ( scf_cb.scf_error.err_code == E_SC028C_XSEV_EALLOCATE )
        {
            uleFormat( NULL, E_RD018A_EVENT_FAIL, NULL, ULE_LOG, &sqlstate, 
			msg_buffer, (i4) (sizeof(msg_buffer)-1), 
			&msg_buf_length, &ule_error, 0);

	    status = E_DB_OK;
	}
    }

    return ( E_DB_OK );
}

/* rdf_ismultiserver - are multiple dbms servers active in installation? */
static bool
rdf_ismultiserver()
{
    LG_PROCESS      lpb;
    i4		    count = 0;
    i4         length = 0;
    CL_ERR_DESC     sys_err;
    STATUS	    status;

    if ( CXcluster_enabled() )
    {
	/* Assume existence of remote servers in a cluster */
	return ( TRUE );
    }

    for (;;)
    {
        status = LGshow(LG_N_PROCESS, (PTR)&lpb, sizeof(lpb), &length, &sys_err);
        if ( length == 0 || status != OK )
	    break;
        /* a DBMS server will usually be PR_FCT or PR_SLAVE */
        if ( (lpb.pr_type & PR_FCT) || (lpb.pr_type & PR_SLAVE) )
	    count++;
	if ( count > 1 )
	    break;
    }

    if ( count > 1)
	return ( TRUE );
    else
	return ( FALSE );
}
