/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<cs.h>
#include	<nm.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<cm.h>
#include	<si.h>

/* prototypes */

/* rdf_rcluster	- retrieve cluster information. */
static DB_STATUS rdf_rcluster(	RDF_GLOBAL	*global,
				ULM_RCB		*ulmrcb);
/**
**
**  Name: RDFSTART.C - Start up RDF for the server.
**
**  Description:
**	This file contains the routine for starting up RDF for the server. 
**
**	rdf_startup - Start up RDF for the server.
**
**
**  History:
**      15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**	05-apr-90 (teg)
**	    added logic to include synonyms in cache size calculation.
**	31-jan-91 (teresa)
**	    zerofill SVCB after allocating it.  This assures that all running
**	    counters start at zero, and is part of the mini-project to improve
**	    RDF statistics reporting.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of nm.h
**	 7-mar-1996 (nick)
**	    Specifying large RDF table counts would cause us to fail in
**	    rdf_startup() when allocating the defaults cache. #73850
**	15-Aug-1997 (jenjo02)
**	    Call CL directly for semaphores instead of going thru SCF.
**	    Removed rdf_scfcb, rdf_fscfcb.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	23-Sep-1997 (jenjo02)
**	    Initialize rdv_global_sem. This was not being done and causes fits
**	    on platforms which demand that the underlying mutexes be initialized,
**	    plus it's just good form.
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
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	11-Jun-2010 (kiria01) b123908
**	    Init ulm_streamid_p for ulm_openstream to fix potential segvs.
**/

/*{
** Name: rdf_startup - Start up RDF for the server.
**
**	External call format:	status = rdf_call(RDF_STARTUP, &rdf_ccb)
**
** Description:
**
**	This routine starts up RDF for the server. It should be invoked only
**	once during the life of the server.
**
**	This routine requests memory from SCF for the RDF Server Control
**	block (svcb).  That server control block remains for the life of the
**	DBMS server.
**
**      This routine allocates cache memory space from ULM.  This memory
**	contains four ulh hash tables.  One is used for relations, one is 
**	used for querry trees, one is used for defaults  and the other is
**	for distributed objects. The Memory Pool id and the hash table id 
**	for each hash table is placed into the server control block.  
**
**	RDF uses the rdf_cachesize parameter to determine memory pool size,
**	which it rounds up to the nearest 32K block (32 * 1024).
**	It uses the following input parameters to set hard limits:
**
**	    num objects on relation cache:  rdf_max_tbl or RDF_MTRDF (default)
**	    num objects on qtree cache:	    rdf_max_tbl or RDF_MTRDF (default)
**	    num synonyms on relation cache: rdf_num_synonym or
**						RDF_SYNCNT * number objects
**	    num objects on defaults cache:  rdf_colavg or RDF_COLDEFS
**	    num objects on LDBdesc cache:   rdf_maxddb * rdf_avgldb
**		where rdf_maxddb defaults to Rdi_svcb->rdv_max_sessions and
**		rdf_avgldb defaults to RDF_AVGLDBS
**
**	RDF initialized the following caches:
**		relation
**		qtree
**		defaults    (ldb only)
**		ldbdesc	    (star only)
**
**	RDF saves away the initial memory pool size, and the hard limits and the
**	startup parameters so it may report them to the user at server shutdown
**	time.
**		
**	Finally, if all memory allocation/initialization is successful, RDF
**	clears up all the RDF trace flags for the server.
**	
** Inputs:
**
**      rdf_ccb			
**	    .rdf_max_tbl	    max number of tables allowed in cache
**	    .rdf_colavg		    avg number of columns per table
**	    .rdf_cachesiz	    approx number of bytes to allocate for cache
**	    .rdf_num_synonym	    number of synonyms per table
**	    .rdf_maxddb		    max # of DDB objects RDF may cache at once.
**	    .rdf_avgldb		    avg # ldbs per ddb
** Outputs:
**      rdf_ccb                          
**	    .rdf_error		RDF Error control block
**		.err_code	    Error code.  Has the following possible values:
**				    E_RD0000_OK - Operation succeed.
**				    E_RD0003_BAD_PARAMETER - bad parameter
**				    E_RD0020_INTERNAL_ERR - RDF internal error.
**				    E_RD0129_RDF_STARTUP - unable to get memory 
**							   from SCF for SVCB
**				    E_RD025A_ULMSTART - failure from ulm_startup
**							to allocate memory pool
**				    E_RD0040_ULH_ERROR - failure initializing 
**							 hash table
**	    .rdf_server		Pointer to Server Control Block (SVCB)
**		.rdv_memleft	    amount of unused memory in pool
**		.rdv_poolid	    Memory pool id from ulm_startup
**		.rdv_state	    indicates RDF state during initialization:
**				    NULL		 - no memory allocated.
**				    RDV_1_RELCACHE_INIT  - allocated mem pool.
**				    RDV_3_RELHASH_INIT   - initialized relation
**							   hash table
**				    RDV_4_QTREEHASH_INIT - initialized qtree
**							   hash table
**				    RDV_2_LDBCACHE_INIT  - initialized distrib
**							   access hash table
**				    RDV_5_DEFAULTS_INIT  - initialized default
**							   hash table
**		.rdv_rdesc_hashid   descriptor hash bucket header
**		.rdv_qtree_hashid   qtree hash bucket header
**		.rdv_dist_hashid    ldbdesc hash bucket header
**		.rdv_def_hashid	    defaults hash bucket header
**		.rdf_trace	    rdf trace vector
**		.rdv_pool_size      Size of rdf cache pool in bytes
**		.rdv_in0_memory	    save input parameter for memory pool size
**		.rdv_in1_max_tables save input parameter for number objs in relcache
**		.rdv_in2_table_columns save input parameter for max defaults
**		.rdv_in3_table_synonyms save input parameter for synonyms
**		.rdv_in4_cache_ddbs  save input parameter for max ddbs
**		.rdv_in5_average_ldbs	save input parameter for avg ldbs/ddb
**		.rdv_bad_cksum_ctr  bad checksum counter
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_ccb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**	16-Oct-89 (teg)
**	    added logic to allocate a memory pool and set up two hash table
**	    lists.  This is part of the solution to bug 5446.
**	13-dec-89 (teg)
**	    added logic to make cache size user programable.
**	05-apr-90 (teg)
**	    added logic to consider synonyms in cache size calculation
**      18-apr-90 (teg)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	31-jan-91 (teresa)
**	    zerofill SVCB after allocating it.  This assures that all running
**	    counters start at zero, and is part of the mini-project to improve
**	    RDF statistics reporting.
**	12-mar-92 (teresa)
**	    change ulh_init call for QTREE cache to specifiy classes.
**	16-jul-92 (teresa)
**	    added prototypes
**	29-jul-92 (teresa)
**	    return size of RDF_SESS_CB to scf.
**	03-dec-92 (teresa)
**	    correct stack calculation for LDBdesc cache: the number of entries
**	    on the cache should be (num_ddbs * num_ldbs), not num_ddbs.
**	12-feb-93 (teresa)
**	    simplified RDF cache tuning.
**	25-mar-93 (dianeh)
**	    Fixed the assignment to Rdi_svcb->rdv_memleft.
**	11-may-1993 (teresa)
**	    surpress having any messages printed to user by setting the
**	    surpress flag on calls to rdu_ferror().
**	21-may-93 (teresa)
**	    assure that RDF is never allowed to start up with less than the
**	    functional minimum amount of memory it needs to complete startup
**	    and connect to a single session -- use RDF_MIN_MEMORY constant.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast in assignment to Rdi_svcb->owner which has changed
**	    type to PTR.
**	23-feb-94 (teresa)
**	    Fix bug 59336.  Initialize bad checksum counter.
**      12-May-1994 (daveb)
**          Add call to rdf_mo_init().
**	 7-mar-1996 (nick)
**	    If (rdf_max_tbl / RDF_BUCKETS) > rdf_colavg we'd fail at server 
**	    startup.  Calculations of hash density were hosed. #73850
**	23-Sep-1997 (jenjo02)
**	    Initialize rdv_global_sem. This was not being done and causes fits
**	    on platforms which demand that the underlying mutexes be initialized,
**	    plus it's just good form.
*/
DB_STATUS
rdf_startup(RDF_GLOBAL	*global,
	    RDF_CCB	*rdf_ccb)
{
    DB_STATUS	    status;
    ULM_RCB         *ulmrcb = &global->rdf_ulmcb; /* ULM request cb */
    ULH_RCB         *ulhrcb = &global->rdf_ulhcb; /* ULH request cb */
    i4              objcnt;
    i4              density;
    int		    increment;
    i4	    num_synonyms;
    i4	    num_defs=0;
    i4	    num_ddb=0;
    i4	    num_ldb=0;
    i4	    num_ldbdesc_entries;

    global->rdfccb = rdf_ccb;
    CLRDBERR(&rdf_ccb->rdf_error);
    if (ULH_MAXNAME < (sizeof(DB_TAB_ID) + sizeof(PTR)))
    {	/* this assertion needs to be made for RDF hashing to work */
	status = E_DB_FATAL;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    /* set up server control block pointers.
    */
    rdf_ccb->rdf_server = (PTR)Rdi_svcb;

    /* return the size of RDF's Server Control Block */
    rdf_ccb->rdf_sesize = sizeof(RDF_SESS_CB);

    /*
    ** Fill in header information.
    */
    Rdi_svcb->length = sizeof(RDI_SVCB);
    Rdi_svcb->type = RDI_CB_TYPE;
    Rdi_svcb->owner = (PTR)DB_RDF_ID;
    Rdi_svcb->ascii_id = RDI_SCB_ID;

    /* Save offset to any session's RDF_SESS_CB */
    Rdi_svcb->rdv_rdscb_offset = rdf_ccb->rdf_rdscb_offset;

    /* Prime the rdv_global_sem semaphore */
    CSw_semaphore(&Rdi_svcb->rdv_global_sem, CS_SEM_SINGLE, "RDF SVCB sem");

    /* determine hard limits for relation and qtree cache, which are used
    ** both by Local and Star servers 
    */
    objcnt = (rdf_ccb->rdf_max_tbl > 0) ? rdf_ccb->rdf_max_tbl : RDF_MTRDF;
    num_synonyms = (rdf_ccb->rdf_num_synonym > 0) ?  /* did usr specify value?*/
		    rdf_ccb->rdf_num_synonym :	         /* use suppied value*/
		    RDF_SYNCNT * objcnt;		 /* else use default */

    /* save input parameters for statistics reporting */
    Rdi_svcb->rdv_in1_max_tables	= (rdf_ccb->rdf_max_tbl > 0) ? 
				   rdf_ccb->rdf_max_tbl : RD_NOTSPECIFIED;
    Rdi_svcb->rdv_in3_table_synonyms= (rdf_ccb->rdf_num_synonym > 0) ?
				   rdf_ccb->rdf_num_synonym : RD_NOTSPECIFIED;

    
    /* initialize the bad checksum counter to zero */
    Rdi_svcb->rdv_bad_cksum_ctr = 0;

    /*
    ** if user supplied a specific cache size, use it.
    ** Else use default.
    */
    Rdi_svcb->rdv_memleft = (rdf_ccb->rdf_cachesiz>0) ? 
	    rdf_ccb->rdf_cachesiz : RDF_MEMORY;

    /* force rdf memory to functional minimal level if it is below that */
    Rdi_svcb->rdv_memleft = (Rdi_svcb->rdv_memleft < RDF_MIN_MEMORY) ?
			     RDF_MIN_MEMORY : 
			     Rdi_svcb->rdv_memleft;


    /* 
    ** round cache size up to 32K because we use 32K block 
    */
    if (Rdi_svcb->rdv_memleft%RDF_BLKSIZ != 0)
	Rdi_svcb->rdv_memleft = ((Rdi_svcb->rdv_memleft/RDF_BLKSIZ)+1)*RDF_BLKSIZ;

    /* copy initial cache size to SVCB for statistics reporting */
    Rdi_svcb->rdv_pool_size = Rdi_svcb->rdv_memleft;
    Rdi_svcb->rdv_in0_memory	= (rdf_ccb->rdf_cachesiz>0) ? 
				   rdf_ccb->rdf_cachesiz : RD_NOTSPECIFIED;

    /* 
    ** Compute ulh table density - try to keep a max of 
    ** RDF_BUCKETS number of hash buckets 
    */
    increment = (objcnt % RDF_BUCKETS == 0) ? 0 : 1;
    density = (objcnt / RDF_BUCKETS) + increment;

    /*
    ** Allocate memory space for table descriptor cache.
    ** This cache will contains two hash tables.  One is for
    ** relation descriptors and the other is for qtree.
    */ 
    ulmrcb->ulm_facility = DB_RDF_ID;    /* rdf's id */
    ulmrcb->ulm_sizepool = Rdi_svcb->rdv_memleft;
    ulmrcb->ulm_blocksize = 0;
    status = ulm_startup(ulmrcb);  /* call the ulm startup */
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &ulmrcb->ulm_error,
		E_RD025A_ULMSTART,ulmrcb->ulm_error.err_code);
	return(status);
    }

    /* 1. Relation description cache memory allocation completed OK */
    Rdi_svcb->rdv_state |= RDV_1_RELCACHE_INIT;
    Rdi_svcb->rdv_poolid = ulmrcb->ulm_poolid;	    /* pool id */

    /* 
    ** Call ULH to initialize a hash table for table descriptor.
    ** NOTE:  we have to specify 1 more synonym per table than the user
    **	      specified because RDF will always build an alias to the table
    **	      name.  Each synonym is merely a ulh alias, so we require more
    **	      alias' than the user requests.  (Ie, the user request does not
    **	      include the alias for each table's name.)
    */
    status = ulh_init(  ulhrcb, (i4)DB_RDF_ID, 
		        (PTR)Rdi_svcb->rdv_poolid, &Rdi_svcb->rdv_memleft, 
		        (i4)objcnt, (i4)0, (i4)density, 
		        ULH_ALIAS_OK,
		        (i4) (num_synonyms + objcnt) 
		      );
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &ulhrcb->ulh_error, 
		   E_RD0040_ULH_ERROR, ulhrcb->ulh_error.err_code);
	return(status);
    }

    /* Save table descriptor hash table id in server control block */
    Rdi_svcb->rdv_rdesc_hashid = ulhrcb->ulh_hashid;
    Rdi_svcb->rdv_cnt0_rdesc = objcnt;
    Rdi_svcb->rdv_state |= RDV_3_RELHASH_INIT;

    /* 
    ** Call ULH to initialize a hash table for qtree.
    */
    status = ulh_init(ulhrcb, (i4)DB_RDF_ID,
	    (PTR)Rdi_svcb->rdv_poolid, &Rdi_svcb->rdv_memleft, (i4)objcnt, 
	    (i4)objcnt, (i4)density, ULH_ALIAS_OK, (i4) objcnt);
				    /* alias will be needed for
				    ** PSF procedures and rules/integrities/
				    ** permits, but synonyms are not
				    ** allowed for the QTREE hash table */
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &ulhrcb->ulh_error, 
		    E_RD0040_ULH_ERROR, ulhrcb->ulh_error.err_code);
	return(status);
    }

    /* Save QTREE hash table id in server control block */
    Rdi_svcb->rdv_qtree_hashid = ulhrcb->ulh_hashid;	
    Rdi_svcb->rdv_cnt1_qtree = objcnt;
    Rdi_svcb->rdv_state |= RDV_4_QTREEHASH_INIT;

    if (global->rdfccb->rdf_distrib & DB_2_DISTRIB_SVR)
    {
	/* 
	** This server supports distributed, so initialize the distributed
	** portions of the cache 
	*/

	num_ldb = (rdf_ccb->rdf_avgldb > 0) ? rdf_ccb->rdf_avgldb : RDF_AVGLDBS;
	num_ddb = (rdf_ccb->rdf_maxddb > 0) ? rdf_ccb->rdf_maxddb
					    : Rdi_svcb->rdv_max_sessions;
	num_ldbdesc_entries = num_ldb * num_ddb;

	/* save input values for statistics reporting */
	Rdi_svcb->rdv_in4_cache_ddbs    = (rdf_ccb->rdf_maxddb > 0) ? 
				       rdf_ccb->rdf_maxddb : RD_NOTSPECIFIED;
	Rdi_svcb->rdv_in5_average_ldbs  = (rdf_ccb->rdf_avgldb > 0) ? 
				       rdf_ccb->rdf_avgldb : RD_NOTSPECIFIED;

	/* this cache is not used for star */
	Rdi_svcb->rdv_in2_table_columns = 0;	
	
	/* try and keep below RDF_STARBUCKETS number of buckets */
	increment = ((num_ldbdesc_entries % RDF_STARBUCKETS) == 0) ? 0 : 1;
	density = (num_ldbdesc_entries / RDF_STARBUCKETS) + increment;

	/* 
	** Call ULH to initialize a hash table for Distirbuted Access
	*/
	status = ulh_init(ulhrcb, (i4)DB_RDF_ID,
		(PTR)Rdi_svcb->rdv_poolid, &Rdi_svcb->rdv_memleft, 
		(i4)num_ldbdesc_entries,  (i4)0, (i4)density, 
		0, 0);  /* no aliases for this hash table */
	if (DB_FAILURE_MACRO(status))
	{
	    rdu_ferror(global, status, &ulhrcb->ulh_error, 
			E_RD0040_ULH_ERROR, ulhrcb->ulh_error.err_code);
	    return(status);
	}
	/* Save LDBdesc hash table id in server control block */
	Rdi_svcb->rdv_dist_hashid = ulhrcb->ulh_hashid;	
	Rdi_svcb->rdv_cnt2_dist = num_ldbdesc_entries;
	Rdi_svcb->rdv_state |= RDV_2_LDBCACHE_INIT;

	/* create another shared stream to cache cluster information */
	ulmrcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
	ulmrcb->ulm_flags = ULM_SHARED_STREAM;
	ulmrcb->ulm_streamid_p = NULL;

	status = ulm_openstream(ulmrcb); 
	if (DB_FAILURE_MACRO(status))
	{
	    rdu_ferror(global, status, 
		    &ulmrcb->ulm_error, E_RD0118_ULMOPEN, 
		    ulmrcb->ulm_error.err_code);
	    return(status);
	}

	Rdi_svcb->rdv_cls_streamid = ulmrcb->ulm_streamid;
	status = rdf_rcluster(global, ulmrcb);
	/* is cluster info retrieval OK */
	if (DB_FAILURE_MACRO(status))
	{
	    /* 
	    ** if cluster info is not available, return OK anyway.
	    ** however, mark the count to zero 
	    */
	    status = E_DB_OK;
	    Rdi_svcb->rdv_num_nodes = 0;
	    Rdi_svcb->rdv_cluster_p = (RDD_CLUSTER_INFO *)NULL;
	}
    }
    else
    {
	/* 
	** set up DEFAULTS cache.  Note - this cache is allocated only
	** when we are not doing distributed 
	**
	** Call ULH to initialize a hash table for Defaults cache.
	** NOTE:  Aliases are not allowed for this cache.  Also, the number
	**	  of objects allowed on this cache is assumed to be
	**		(objcnt * num_atts) /  5
	**	  or 20% of the attributes allowed on the cache at 1 time.
	*/
	num_defs = (rdf_ccb->rdf_colavg > 0) ? 
			rdf_ccb->rdf_colavg : RDF_COLDEFS;

	/* save input values for statistics reporting */

	Rdi_svcb->rdv_in2_table_columns = (rdf_ccb->rdf_colavg > 0) ? 
				       rdf_ccb->rdf_colavg : RD_NOTSPECIFIED;

	/* these are used only for star */
	Rdi_svcb->rdv_in4_cache_ddbs    = 0;
	Rdi_svcb->rdv_in5_average_ldbs  = 0;

	/* 
	** Compute ulh table density - try to keep a max of RDF_BUCKETS number
	** of hash buckets 
	*/
	increment = (num_defs % RDF_BUCKETS == 0) ? 0 : 1;
	density = (num_defs / RDF_BUCKETS) + increment;

	status = ulh_init(  ulhrcb, (i4)DB_RDF_ID, 
			    (PTR)Rdi_svcb->rdv_poolid, &Rdi_svcb->rdv_memleft, 
			    (i4)num_defs, (i4)0, (i4)density, 
			    (i4)0, (i4) 0
			  );
	if (DB_FAILURE_MACRO(status))
	{
	    rdu_ferror(global, status, &ulhrcb->ulh_error,
		       E_RD0040_ULH_ERROR, ulhrcb->ulh_error.err_code);
	    return(status);
	}

	/* Save table descriptor hash table id in server control block */
	Rdi_svcb->rdv_def_hashid = ulhrcb->ulh_hashid;
	Rdi_svcb->rdv_cnt3_default = num_defs;
	Rdi_svcb->rdv_state |= RDV_5_DEFAULTS_INIT;

    }
    if (global->rdfccb->rdf_distrib & DB_1_LOCAL_SVR)
	Rdi_svcb->rdv_distrib |= DB_1_LOCAL_SVR;

    rdf_mo_init( Rdi_svcb );

    /*
    ** Clear all trace flags for the server.
    */
	/* #fix_me -- need to change RDF_NB and RDF_NVP and RDF_NVA0 to 
	**	      appropriate constants for the session */
    ult_init_macro(&Rdi_svcb->rdf_trace, RDF_NB, RDF_NVP, RDF_NVAO);
    return(status);
}

/*{
** Name: rdf_rcluster	- retrieve cluster information.
**
** Description:
**      This function reads II_CONFIG:cluster.cnf to retrieve cluster 
**	information and cache it in the server control block.
**
**	This routine is highly dependend on the format of the file it is
**	trying to read, and IF THE FILE FORMAT CHANGES, THIS ROUTINE WILL
**	NEED TO BE CHANGED ACCORDINGLY.
**	(Note: IIBUILD and DMFCSP work together to build the cluster.cnf
**	       file.  It is only changes to IIBUILD (or whatever replaces it)
**	       and DMFCSP that can cause thif file to change format.)
**
**	Each record in this file is currently 128 bytes long and has the
**	following format:
**
**	    [0]	    - non-zero if this record contains a cluster name, zero if 
**		      it does not.
**	    [1-7]   - contains non-ASCII data that Star is not interested in.
**	    [8-23]  - contains cluster name in ASCII.
**	    [24-27] - contains non-ASCII data that is meaningful to DMF
**		      but not to Star.
**	    [28+]   - contains ASCII pathname to error log file for this cluster
**		      node.
**
**	Star only cares about cluster names, which are currently begin at buf[8]
**	and contain 16 characters.  It reads the whole record into buf and tests
**	to see if this record contains a cluster name.  If not, it throws away 
**	the record and gets the next.  If the record contains a clusteer name,
**	RDF extracts the cluster name and converts it to lower case.
**
**	The following limitations exist and could byte STAR on any new releases:
**	1.  Star currently reads the cluster name into a buffer DB_MAXNAME
**	    bytes long.  If the name contains characters which require more
**	    than 1 byte, the cluster names still cannot exceed DB_MAXNAME 
**	    characters of storage or Star will break.
**	2.  The file record size is assumed to be 128 bytes.  If this changes,
**	    Star must be changed accordingly (constant CLSTR_BUF_SIZE).
**	3.  Star will not read more than MAX_RECORDS (current value is 28 from
**	    the most recent iibuild I could find) number of records from the 
**	    cluster.cnf file.  If iibuild/dmfcsp change to allow more records,
**	    then up the MAX_RECORDS constant accordingly.
**	4.  Star assumes the cluster name starts at an offset of 
**	    CLSTR_NAME_START (currently =8) from the beginning of the record.
**	    If the offset changes, then CLSTR_NAME_START must be changed
**	    accordingly.
**	5.  Star assumes the cluster name will not exceed CLSTR_NAME_LEN 
**	    characters.  For ASCII, each byte is 1 character, but this is not 
**	    true for all character sets, so be careful that the CLSTR_NAME_LEN 
**	    characters does not exceed DB_MAXNAME bytes!)  The current value of 
**	    CLSTR_NAME_LEN is 16.  If ii_config:cluster.cnf is changed to allow
**	    longer cluster names
**
** Inputs:
**
** Outputs:
**	none
**
**	Returns:
**	    0 if the input argument is a prime number
**	    1 otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-mar-89 (mings)
**	    created
**	18-apr-89 (mings)
**	    cluster node name will be blanked and not null terminalted
**	23-jan-92 (teresa)
**	    changes for SYBIL:  added logic to determine if we are trying to
**	    read more nodes into memory than we've allocated memory for.  If so,
**	    then stop reading additional nodes so we do not trample memory.
**	    Added two new input parameters: num_cls (max number of node entries
**	    that there is room to hold) and svcb (pointer to server control blk)
**	16-jul-92 (teresa)
**	    added prototypes
**	1-dec-92 (teresa)
**	    replaced hardwired constants "8" and "16"  and "24" with constants
**	    CLSTR_NAME_START, CLSTR_NAME_LEN and MAX_RECORDS.  Also upped 
**	    MAX_RECORDS from 24 to 28, since that's the current iibuild limit.
**	    Renamed BUF_SIZE constant to CLSTR_BUF_SIZE.  Finally, replaced
**	    incrementing buffers with calls to CMnext.
**	    (I also documented all the stuff I've learned by research so I don't
**	    have to go through all this research next time I need to examine
**	    this routine.)
**	12-feb-93 (teresa)
**	    removed num_cls input parameter as part of simplify-tuning project.
**	11-may-1993 (teresa)
**	    surpress having any messages printed to user by setting the
**	    surpress flag on calls to rdu_ferror().
*/

/*------------ Constants used to read file II_CONFIG:cluster.cnf -------------*/
#define	    CLSTR_BUF_SIZE  128	    /* read buffer size */
#define	    MAX_RECORDS	    28	    /* max number of CLSTR entries in file;
				    ** controlled by iibuild */
#define	    CLSTR_NAME_START 8	    /*offset to CLSTR name start in record */
#define	    CLSTR_NAME_LEN   16	    /* max # characters allowed in a CLSTR name 
				    ** NOTE: THIS MUST NOT EXCEEDE DB_MAXNAME
				    **	     and size must not exceed DB_MAXNAME
				    **	     bytes.
				    */
/*----------------------------------------------------------------------------*/

static DB_STATUS
rdf_rcluster(	RDF_GLOBAL	*global,
		ULM_RCB		*ulmrcb)
{

    DB_STATUS		status;
    FILE		*fptr;
    RDD_CLUSTER_INFO	*temp_p = (RDD_CLUSTER_INFO *)NULL;
    bool		not_eof = TRUE;
    i4		count, i;
    char		buf[CLSTR_BUF_SIZE];
    char		*from_buf,*to_buf;

    /* initialize cluster info in server control block */

    Rdi_svcb->rdv_num_nodes = 0;
    Rdi_svcb->rdv_cluster_p = (RDD_CLUSTER_INFO *)NULL;

    /* clear cpu and net cost */
    Rdi_svcb->rdv_node_p = (DD_NODELIST *)NULL;
    Rdi_svcb->rdv_net_p = (DD_NETLIST *)NULL;
    Rdi_svcb->rdv_cost_set = RDV_0_COST_NOT_SET;
    
    /* open II_CONFIG:cluster.cnf */

    if (NMf_open("r", "cluster.cnf", &fptr) == E_DB_OK)
    {
	do
	{
	    MEfill(CLSTR_BUF_SIZE, (u_char)0, (PTR)buf); 

	    /* read 128 bytes from II_CONFIG:cluster.cnf */
	    status = SIread(fptr, CLSTR_BUF_SIZE, &count, buf);

	    /* is eof or bad block */
	    if (status == ENDFILE || count < MAX_RECORDS)
	    {
		status = E_DB_OK;
		not_eof = FALSE;
	    }
	    else if (DB_FAILURE_MACRO(status))
		 {
		    /* read error */
		    not_eof = FALSE;
		 }
	    else if (!(buf[0]))
		 {
		    /* this block contains no information at all, 
		    ** skip it and continue */
		    continue;
		 }
	    else
	    {
		Rdi_svcb->rdv_num_nodes++;	/* count number of nodes read */

		/* we just read in a valid block, allocate RDD_CLUSTER_INFO and
		** store the node name. */
		ulmrcb->ulm_psize = sizeof(RDD_CLUSTER_INFO);
		status = ulm_palloc(ulmrcb);
		if (DB_FAILURE_MACRO(status))
		{
		    rdu_ferror(global, status, &ulmrcb->ulm_error,
			       E_RD0127_ULM_PALLOC,ulmrcb->ulm_error.err_code);
		    not_eof = FALSE;
		}
		
		/* is this the first block */
		if (temp_p)
		{
		    /* no, this is not the first block. link it to the previous
		    ** node info. */
		    temp_p->rdd_1_nextnode = 
					   (RDD_CLUSTER_INFO *)ulmrcb->ulm_pptr;
		}
		else
		{
		    /* 
		    ** this is the first node so save it in the server control 
		    ** block 
		    */
		    Rdi_svcb->rdv_cluster_p = (RDD_CLUSTER_INFO *)ulmrcb->ulm_pptr;
		}

		/*
		** Currently the size of the node name in the cluster.cnf
		** file is 16 chars and the buf its read into is DB_NODE_MAXNAME
		** So blank fill the buffer 
		*/
		temp_p = (RDD_CLUSTER_INFO *)ulmrcb->ulm_pptr;
		MEfill(DB_NODE_MAXNAME, (u_char)' ', temp_p->rdd_2_node); 

		/* 
		** convert cluster node name to lower case --
		** The line format is 8 non-ASCII characters, followed
		** by the CLSTR name (currently 16 ascii characters), 
		** followed by additional ascii and non-ascii information.
		*/
		from_buf = &buf[CLSTR_NAME_START];
		to_buf = &temp_p->rdd_2_node[0];
	        for (i = 0; i < CLSTR_NAME_LEN; i++)
		{
		    CMtolower(from_buf, to_buf);
		    CMnext(from_buf);
		    CMnext(to_buf);
		}
		Rdi_svcb->rdv_num_nodes++;	/* keep a count on valid nodes */
	    }
	} while (not_eof);
	SIclose(fptr);
	
    }
    else
    {
	/* we couldn't find II_CONFIG:cluster.cnf */
	status = E_DB_ERROR;
    }
    
    return(status);
}
