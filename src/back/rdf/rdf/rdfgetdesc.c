/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
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
#include	<ex.h>
#include	<rdfint.h>
#include	<tr.h>
#include	<psfparse.h>
#include	<rdftrace.h>
#include	<cui.h>
#include	<st.h>

/* prototype definitions */

/* rdu_synonym - Read a synonym from iisynonym catalog */
static DB_STATUS rdu_synonym(   RDF_GLOBAL          *global,
                                DB_TAB_ID           *table_id);

/* rdu_cache - find the ULH object in the cache */
static DB_STATUS rdu_cache( RDF_GLOBAL	*global,
			    RDF_BUILD   *build,
			    bool	*refreshed);

/* rdu_relbld   - build the relation relation descriptor */
static DB_STATUS rdu_relbld(RDF_GLOBAL  *global,
			    RDF_BUILD   *build,
			    bool        *refreshed);

/* rdu_bldattr  - build attributes descriptor */
static DB_STATUS rdu_bldattr(   RDF_GLOBAL  *global,
				RDF_BUILD   *build,
				bool        *refreshed);

/* rdu_key_ary_bld - Build the primary key array. */
static DB_STATUS rdu_key_ary_bld(RDF_GLOBAL *global);

/* rdu_getbucket - Hash an attribute name. */
static i4  rdu_getbucket(DB_ATT_NAME *name);

/* rdu_hash_att_bld - Build the hash table of attributes information. */
static DB_STATUS rdu_hash_att_bld(RDF_GLOBAL *global);

/* rdu_bldindx  - build the index descriptor */
static rdu_bldindx( RDF_GLOBAL	    *global,
		    RDF_BUILD	    *build,
		    bool	    *refreshed);

/*rdu_default_bld - Add defaults to the attributes descriptor */
static DB_STATUS rdu_default_bld(RDF_GLOBAL *global );


/**
**
**  Name: RDFGETDESC.C - Request description of a table.
**
**  Description:
**	This file contains the routines for requesting relation, attributes
**	and index information of a table.
**
**	rdf_gdesc - Request description of a table.
**	rdu_key_ary_bld - Build the primary key array.
** 	rdu_hash_att_bld - Build the hash table of attributes information.
** 	rdu_getbucket - Hash an attribute name.
**
**  History:    
**      03-apr-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	21-sep-87 (puree)
**	    modify ULH interface for new object handling protocol.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	16-jul-92 (teresa)
**	    prototypes
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use appropriate tid attribute descriptor, depending upon the
**	    case translation semantics for regular identifiers for the session.
**      23-jun-93 (shailaja)
**          Fixed compiler warnings on trigraph sequences.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of me.h.
**      23-jan-97 (cohmi01)
**	    Remove checksum calls from those places that were placing
**	    it in rdr_checksum, this is now done in rdu_rsemaphore(), 
**	    before releasing mutex to ensure that checksum is recomputed 
**	    during same mutex interval that the infoblock was altered in. 
**	    (Bug 80245).
**	19-feb-1997 (cohmi01)
**	    Call rdu_xlock() before changing the sys_infoblk for DDB info
**	    to ensure proper concurrent behaviour & that it gets re-checksumed.
**	28-May-1997 (shero03)
**	    Reverse the meaning of RDF_CHECK_SUM
**      26-mar-1998 (sarjo01)
**          Bug 77364: In rdf_gdesc(), be sure that dd_t8_cols_pp is not null
**          before deciding to avoiding the rdd_ldbsearch().
**	22-jul-1998 (shust01)
**	    Initialized rdf_ulhobject->rdf_sysinfoblk to NULL after rdf_ulhobject
**	    is allocated in case allocation of rdf_sysinfoblk fails (due to lack
**	    of RDF memory, for example).  That way, we won't try  to use/free up
**	    rdf_sysinfoblk later, causing a SEGV. Bug 92059. 
**	22-Nov-1999 (jenjo02)
**	    In rdu_bldindx(), handle disparity in the number of indexes expected 
**	    and the number created by DMT_SHOW, expecially the zero index case!
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
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
**	21-feb-2001 (rigka01)
**	    Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**	    user_default(). info block indicates default information is
**	    available but pointer to the info is null. 
**      20-aug-2002 (huazh01)
**          On rdf_gdesc(), request a semaphore before updating
**          rdf_ulhobject->rdf_shared_sessions. This fixes b107625.
**          INGSRV 1551.
**    17-April-2003 (inifa01) bugs 110096 & 109815 INGREP 133.
**          E_DM004D and E_RD002B should not be returned to the user or 
**          printed to the error log.  Call rdu_ferror() here to suppress
**          E_DM004D.
**	29-Dec-2003 (schka24)
**	    Include partitioning stuff in "physical info" request,
**	    rename RDR_BLD_KEY to RDR_BLD_PHYS.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*{
** Name: rdu_gsemaphore	- get a semaphore on the ULH object
**
** Description:
**      This routine will get a semaphore on the ULH object if it is not
**      already obtained.
**
** Inputs:
**      global                          ptr to RDF global state variable
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
**      20-oct-87 (seputis)
**	    initial creation
**	18-apr-90 (teg)
**	    interface to rdu_ferror changed for fix to bug 4390
**	16-jul-92 (teresa)
**	    prototypes
**	15-Aug-1997 (jenjo02)
**	    Call CL directly instead of going thru SCF.
[@history_template@]...
*/
DB_STATUS
rdu_gsemaphore( RDF_GLOBAL *global)
{
    DB_STATUS	    status;

    if (global->rdf_resources & RDF_RSEMAPHORE)
	return(E_DB_OK);		    /* semaphore is already held */

    if (global->rdf_resources & RDF_RULH)
	status = CSp_semaphore(TRUE, &global->rdf_ulhcb.ulh_object->ulh_usem);
    else if (global->rdf_ulhobject)
	/* Traverse through the master info block and thence to the master
	** ULH object.  Private objects don't have rdf_ulhptr filled in!
	*/
	status = CSp_semaphore(TRUE,
	    &((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
		rdr_object)->rdf_ulhptr->ulh_usem);
    else
    {   /* expecting ULH object to be accessible */
	status = E_DB_SEVERE;
	rdu_ierror(global, status, E_RD012C_ULH_SEMAPHORE);
	return(status);
    }

    if (status)
    {
	DB_ERROR	localDBerror;
	
	SETDBERR(&localDBerror, 0, status);
	rdu_ferror(global, E_DB_ERROR, &localDBerror, E_RD0034_RELEASE_SEMAPHORE,0);
	status = E_DB_ERROR;
    }
    else
	global->rdf_resources |= RDF_RSEMAPHORE; /* mark semaphore has being
					    ** held */
    return(status);
}

/*{
** Name: rdu_rsemaphore	- release semaphore on ULH object
**
** Description:
**      This routine will release the semaphore on the ULH object if it
**      is held.
**
** Inputs:
**      global                          ptr to RDF global state variable
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
**      20-oct-87 (seputis)
**          initial creation
**	18-apr-90 (teg)
**	    interface to rdu_ferror changed for fix to bug 4390
**	16-jul-92 (teresa)
**	    prototypes
**      23-jan-97 (cohmi01)
**	    Compute checksum and store in sys_infoblock before releasing 
**	    mutex to ensure that checksum is recomputed during the 
**	    same mutex interval that the infoblock was altered in. (Bug 80245)
**	15-Aug-1997 (jenjo02)
**	    Call CL directly instead of going thru SCF.
[@history_template@]...
*/
DB_STATUS
rdu_rsemaphore( RDF_GLOBAL  *global)
{
    DB_STATUS           status;
    i4		v1, v2;

    if (!(global->rdf_resources & RDF_RSEMAPHORE))
	return(E_DB_OK);		    /* if semphore is not held then no
                                            ** need to call SCF */
    if (ult_check_macro(&Rdi_svcb->rdf_trace,
					RDU_CHECKSUM, &v1, &v2)) 
    {
	if (global->rdf_ulhobject && global->rdf_ulhobject->rdf_sysinfoblk)
        {
	    RDR_INFO	*sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
	    sys_infoblk->rdr_checksum = rdu_com_checksum(sys_infoblk);
        }
        else
    	    TRdisplay("rdu_rsem lacks ptrs to checksum\n");
    }

    if (global->rdf_resources & RDF_RULH)
	status = CSv_semaphore(&global->rdf_ulhcb.ulh_object->ulh_usem);
    else if (global->rdf_ulhobject)
	/* Traverse through the master info block and thence to the master
	** ULH object.  Private objects don't have rdf_ulhptr filled in!
	*/
	status = CSv_semaphore(
	    &((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
		rdr_object)->rdf_ulhptr->ulh_usem);
    else
    {   /* object may have been destroyed, ignore its semaphore */
	status = E_DB_OK;
    }

    if (status)
    {
	DB_ERROR	localDBerror;
	
	SETDBERR(&localDBerror, 0, status);
	rdu_ferror(global, E_DB_ERROR, &localDBerror, E_RD0034_RELEASE_SEMAPHORE,0);
	status = E_DB_ERROR;
    }
    else
        global->rdf_resources &= (~RDF_RSEMAPHORE); /* mark semaphore as not being
                                            ** held */

    return(status);
}

/*{
** Name: rdu_synonym	- Read a synonym from iisynonym catalog
**
** Description:
**      This routine will read a single synonym tuple.  It will copy
**	the table id from the synonym table into the caller supplied
**	DB_TAB_ID variable table_id.  If no synonym is found, it will set 
**	both elements of table_id to false.
**
** Inputs:
**      global                          RDF state information
**	  .rdfcb			User RDF request block
**	     .rdr_types_mask		DMT_M_TABLE | RDR_BY_NAME
**	     .rdr_tabname		Table name
**	     .rdr_owner			Table Owner owner
**
** Outputs:
**	table_id
**	    .db_tab_base		Base Table Id if synonym found,
**					 remains unchanged if no synonym
**	    .db_tab_index		Index Table Id if synonym found,
**					 remains unchanged if no synonym
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-apr-90 (teg)
**          Written for SYNONYMS
**	23-may-90 (teg)
**	    initialized qrym_alloc.  This is required because we are picking
**	    up the bugfix from 6.3/02 for 8011.
**	16-jul-92 (teresa)
**	    prototypes
**	30-mar-93 (teresa)
**	    check session control block instead of server control block for
**	    RDS_DONT_USE_IISYNONYM (previously RDV_DONT_USE_IISYNONYM)
**	23-mar-99 (thaju02)
**	    If table is a session temporary (denoted by '$Sess' owner),
**	    and the DMF show call returns with nonexistent table, then
**	    return error. Avoid checking/locking iisynonym to see
**	    if name is a synonym name, since we can not create a synonym
**	    on a session temporary (see psl_valid_session_schema_use()
**	    in pslsgram.yi). (B96047)
**	1-Jan-2004 (schka24)
**	    Delete "buffer", not used and took up stack space.
*/
static DB_STATUS
rdu_synonym( RDF_GLOBAL	    *global,
	     DB_TAB_ID	    *table_id)
{
    bool		eof_found;	/* To pass to rdu_qget */
    QEF_DATA		qefdata;	/* For QEU extraction call */
    RDD_QDATA		rdqef;		/* For rdu_qopen to use */
    RDF_CB		*rdfcb;
    DB_STATUS		status, end_status;
    DB_IISYNONYM        syn_tup;	/* Holds 1 iisynonym tuple */
    i4             v1=0;		/* setup for checking trace info */
    i4             v2=0;		/* setup for checking trace info */
    bool		rd0020 = FALSE;	/* setup for checking trace info */

    rd0020 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0020, &v1,&v2);
    if ( (rd0020) || 
	 (global->rdf_sess_cb->rds_special_condt & RDS_DONT_USE_IISYNONYM) ||
	 (STscompare(DB_SESS_TEMP_OWNER, 5,
		(PTR)(global->rdfcb->rdf_rb.rdr_owner.db_own_name), 5) == 0)
       )
    {
	/* this trace point skips checking iisynonym, so return as thought
	** we checked iisynonym and did not find a tuple. */
	status = E_DB_WARN;
	rdu_ierror(global, status, E_RD0002_UNKNOWN_TBL);
	status = E_DB_ERROR;
	return (status);
    }

    rdfcb = global->rdfcb;

    ++Rdi_svcb->rdvstat.rds_r16_syn;

    /* Validate parameters to request */
    if ( (rdfcb->rdf_rb.rdr_types_mask & (RDR_BY_NAME | DMT_M_TABLE)) == 0)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* Use the user data with a local RDF/QEF data request block */
    qefdata.dt_next = NULL;
    qefdata.dt_size = sizeof(DB_IISYNONYM);
    qefdata.dt_data = (PTR) &syn_tup;
    rdqef.qrym_data = &qefdata;
    rdqef.qrym_alloc= 1;
    rdqef.qrym_cnt  = 1;
    rdfcb->rdf_rb.rdr_rec_access_id = NULL;
    rdfcb->rdf_rb.rdr_qtuple_count = 0;

    /* now go open the iisynonym table and look for a synonym */
    status = rdu_qopen(global, RD_IISYNONYM, (char *) 0,
		       sizeof(DB_IISYNONYM), (PTR)&rdqef, 
		       0 /* 0 datasize means RDD_QDATA is set up */ );
    if (DB_FAILURE_MACRO(status))
	return(status);
    /* Fetch the tuple */
    status = rdu_qget(global, &eof_found);
    if ( !DB_FAILURE_MACRO(status))
    {
	if ( (global->rdf_qeucb.qeu_count == 0) || eof_found)
	{
	    /* NOTE:  we do NOT want to log this message, just
	    **	      populate rdf_rf.rdf_error.err_code with
	    **	      E_RD0002_UNKNOWN_TABLE, so status must be
	    **	      warning for the call to rdu_ierror.  However,
	    **	      we want to report an error to PSF, so we set
	    **	      status to E_DB_ERROR after the rdu_ierror call
	    **	      has completed.
	    */
	    status = E_DB_WARN;
	    rdu_ierror(global, status, E_RD0002_UNKNOWN_TBL);
	    status = E_DB_ERROR;
	    /* update statistics indicating that synonym not found */
	    Rdi_svcb->rdvstat.rds_z16_syn++;
	    
	}
	else
	{
	    Rdi_svcb->rdvstat.rds_b16_syn++;
	    table_id->db_tab_base= syn_tup.db_syntab_id.db_tab_base;
	    table_id->db_tab_index=syn_tup.db_syntab_id.db_tab_index;
	}
    }
    end_status = rdu_qclose(global); 	/* Close file */
    if (end_status > status)
	status = end_status;
    return(status);
} /* rdu_synonym */

/*{
** Name: rdu_shwdmfcall	- call DMF show call routine for table
**
** Description:
**
**      Initialize DMF control block if needed and perform DMF show call.
**	
**	If the DMF show call is asking for iirelation information by name and
**	the call fails with a not found error, then go see
**	it the name is really a synonym name by calling rdu_synonym.  If
**	rdu_synonym finds a synonym, then we need to see if we already have
**	the table that the synonym resolves to on the RDF cache.  If so,
**	handle it like when we find an alias.  If not, then do another
**	DMF show call asking for the information by table id.
**
** Inputs:
**      global                          ptr to RDF global state variable
**	dmf_mask		        the dmf show operation requested
**
** Outputs:
**      flagmask                        indicates the following (for synonyms):
**	    RDU_1SYN			    requested name was a synonym
**	    RDU_2CACHED			    requested synonym was already on
**						the RDF cache, and has been
**						fixed via ulh_access call.
**	table_id			contains the table id if request was
**					    RDR_BY_NAME and the name was a
**					    synonym
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    the synonyms table may be read.
**
** History:
**      19-oct-87 (seputis)
**          initial creation
**      14-sep-88 (seputis)
**          release semaphores over DMF calls
**	06-mar-90 (teg)
**	    modify to handle synonyms
**	08-mar-90 (teg)
**	    recoginze E_DM013A_INDEX_COUNT_MISMATCH as a recoverable error
**	    and therefore do NOT report error returned from DMF.
**	18-apr-90 (teg)
**	    interface to rdu_ferror changed for fix to bug 4390
**	07-nov-91 (teresa)
**	    picked up ingres63p change 260973:
**	    03-feb-91 (teresa)
**		pre-integrated seputis change to conditionally set
**		timeout value for DMF calls.
**	16-jul-92 (teresa)
**	    prototypes
**	21-may-93 (teresa)
**	    fixed type casting for prototyped call to ulh routines, and fixed
**	    instance where rdf was passing an (RDF_CB **) instead of an 
**	    (RDF_CB *) to invalidate_relcache().
**    17-April-2003 (inifa01) bugs 110096 & 109815 INGREP 133.
**          E_DM004D and E_RD002B should not be returned to the user or 
**          printed to the error log.  Call rdu_ferror() here to suppress
**          E_DM004D. 
**	16-Mar-2004 (schka24)
**	    Always set table ID when synonym is located (prevent passing
**	    zero tabid to dmt-show's later).
**	7-Feb-2008 (kibro01) b119744
**	    Initialise temp_global
[@history_template@]...
*/
DB_STATUS
rdu_shwdmfcall(	RDF_GLOBAL         *global,
		i4		    dmf_mask,
		RDF_SYVAR	   *flagmask,
		DB_TAB_ID	   *table_id,
		bool		   *refreshed)
{
    DB_STATUS           status;
    DMT_SHW_CB		*dmt_shw_cb;
    bool		scf_semaphore;	    /* TRUE if user has semaphore on
                                            ** ULH object */
    DB_STATUS		scf_status;	    /* scf status */

    scf_status = E_DB_OK;

    /* Set up dmt_show call input parameters. */
    dmt_shw_cb = &global->rdf_dmtshwcb;

/*  semaphore can safely be held across DMT show calls since DMF does not
**  hold locks on system catalog pages, thus no possibility of deadlocks
**  - this is not true for extended system catalogs however 
**  - WRONG DMF does hold locks on system catalog pages, in particular
**  if a table is being modified, the DMF show call will be blocked
*/
    if (global->rdf_resources & RDF_RSEMAPHORE)
    {
	scf_status = rdu_rsemaphore(global);    /* release semaphore if it is 
						** held */
	if (DB_FAILURE_MACRO(scf_status))
	    return (scf_status);
	scf_semaphore = TRUE;
    }
    else
	scf_semaphore = FALSE;
    dmt_shw_cb->dmt_flags_mask = dmf_mask;
    if (!(global->rdf_init & RDF_ISHWDMF))
    {	/* initialize the control block if it is not setup */
	dmt_shw_cb->length = sizeof(DMT_SHW_CB);
	dmt_shw_cb->type = DMT_SH_CB;
	dmt_shw_cb->dmt_db_id = global->rdfcb->rdf_rb.rdr_db_id;
	dmt_shw_cb->dmt_session_id = global->rdfcb->rdf_rb.rdr_session_id;
	STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tabid, dmt_shw_cb->dmt_tab_id);
	if (global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_TIMEOUT)
	{   /* initialize structure to send timeout info to DMF */
	    global->rdf_timeout.char_id = DMT_C_TIMEOUT_LOCK;
	    global->rdf_timeout.char_value = RDF_TIMEOUT; /* default timeout value */
	    dmt_shw_cb->dmt_char_array.data_address = (PTR)&global->rdf_timeout;
	    dmt_shw_cb->dmt_char_array.data_in_size = sizeof(global->rdf_timeout);
	    dmt_shw_cb->dmt_char_array.data_out_size = 0;
	}
	else
	{
	    dmt_shw_cb->dmt_char_array.data_address = NULL; /* no default
				    ** requested */
	}
	global->rdf_init |= RDF_ISHWDMF; /* indicate control block is
				    ** initialized for the next call */
    }
    status = dmf_call(DMT_SHOW, dmt_shw_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (( dmt_shw_cb->error.err_code == E_DM0054_NONEXISTENT_TABLE ) &&
	    ( dmf_mask == (DMT_M_NAME | DMT_M_TABLE) ) )
	{
	    if ( (flagmask == 0) || (table_id == 0) )
	    {
		status = E_DB_ERROR;
		rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
		return(status);
	    }
	    *flagmask = 0;
		
	    /* we were getting iirelation information by owner.name, and could
	    ** not find it.  See if the request was actually for a synonym 
	    */
	    status = rdu_synonym( global, table_id);
	    if ( DB_FAILURE_MACRO(status) )
	    {
		/* update statistics since table not found in catalogs */
		{
		    RDF_TYPES	mask;

		    mask = global->rdfcb->rdf_rb.rdr_types_mask &
			   (RDR_ATTRIBUTES | RDR_INDEXES | RDR_HASH_ATT | 
			    RDR_BLD_PHYS);
		    if (mask)
			Rdi_svcb->rdvstat.rds_z1_core++;
		    else 
			/* it was an existance check */
			Rdi_svcb->rdvstat.rds_z0_exists++;
		}


		return(status);
	    }
	    else if (table_id->db_tab_base)
	    {
		/* we found a synonym, so go process it.  Start by seeing if
		** the base table the synonym resolves to is already on the
		** cache.
		*/

		*flagmask |= RDU_1SYN;
		STRUCT_ASSIGN_MACRO( *table_id, global->rdfcb->rdf_rb.rdr_tabid);
		STRUCT_ASSIGN_MACRO( *table_id, dmt_shw_cb->dmt_tab_id);
		(VOID) rdu_setname (global, table_id);
		status = ulh_access(&global->rdf_ulhcb, 
				    (unsigned char *) &global->rdf_objname[0],
				    global->rdf_namelen);
		if (DB_FAILURE_MACRO(status))
		{
		    if (global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
		    {
			/* unanticipated ULH error.  Report and Abort */
			rdu_ferror( global, status, 
				    &global->rdf_ulhcb.ulh_error,
				    E_RD0142_ULH_ACCESS,0);
			return(status);
		    }		
		}
		else
		{ 
		    RDF_ULHOBJECT	*rdf_ulhobj;
		    /* we found the rdf cache entry.  See if relation info is
		    ** on the cache entry.  If not, treat it as though the
		    ** entry did not exist
		    */
		    global->rdf_resources |= RDF_RULH;  /* mark ulh resource
						        ** as being fixed */
		    Rdi_svcb->rdvstat.rds_f0_relation++;
		    *flagmask |= RDU_2CACHED;
		    rdf_ulhobj=
		        (RDF_ULHOBJECT *)global->rdf_ulhcb.ulh_object->ulh_uptr;
		    if (rdf_ulhobj && rdf_ulhobj->rdf_sysinfoblk
			&& rdf_ulhobj->rdf_sysinfoblk->rdr_rel)
		    {   
			if ((global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH) == 0)
			{
			    /* We have the table info already and we've set
			    ** the table ID corresponding to the synonym.
			    ** It's extremely unlikely if not impossible that
			    ** the caller will end up doing a relbld, but
			    ** just in case, make sure that it looks like
			    ** we did a show:
			    */
			    MEcopy(rdf_ulhobj->rdf_sysinfoblk->rdr_rel,
				dmt_shw_cb->dmt_table.data_in_size,
				dmt_shw_cb->dmt_table.data_address);
			    dmt_shw_cb->dmt_table.data_out_size =
					dmt_shw_cb->dmt_table.data_in_size;
			    return (E_DB_OK);
			}
			else
			{
			    RDF_GLOBAL	temp_global = {0};
			    DB_STATUS	inval_stat;

			    /* invalidate this cache entry and behave as though 
			    ** the entry is not found
			    */
			    *refreshed =  TRUE;
	
			    temp_global.rdf_sess_cb = global->rdf_sess_cb;
			    temp_global.rdf_sess_id = global->rdf_sess_id;
			    temp_global.rdf_init=0;
			    temp_global.rdfcb = global->rdfcb;
			    temp_global.rdf_resources = 0;
			    temp_global.rdf_ulhcb.ulh_object = 
						global->rdf_ulhcb.ulh_object;
			    inval_stat = invalidate_relcache( &temp_global, 
						    global->rdfcb,
						    rdf_ulhobj->rdf_sysinfoblk,
						    (DB_TAB_ID *) NULL);
			    if (DB_FAILURE_MACRO(inval_stat))
			    {
				/* release any resources held by failed 
				** invalidate_relcache 
				*/
				rdf_release( &temp_global, &inval_stat);
				return inval_stat;
			    }
			    /* mark ulh resource as not in use */
			    global->rdf_resources &= ~RDF_RULH;  
			    *flagmask &= ~RDU_2CACHED;
			    rdf_ulhobj=(RDF_ULHOBJECT *) 0;
			    /* update statistics*/
			    Rdi_svcb->rdvstat.rds_u11_shared++; 
			}
		    } /* if have rel info */
		} /* endif base table cache entry found */

		/* either the base object did not exist, or it existed,
		** but did not have the relation information we need. */

		MEfill(sizeof(DB_ERROR),'\0',(PTR)&dmt_shw_cb->error);
		dmt_shw_cb->dmt_flags_mask = DMT_M_TABLE;
		status = dmf_call(DMT_SHOW, dmt_shw_cb);

	    } /* endif we found a synonym */

	} /* dmf_show call was a potential synonym candidate */

    } /* endif original DMT SHOW call returned a failure status */

    if (scf_semaphore)
    {
	scf_status = rdu_gsemaphore(global); /* get the SCF semaphore if it was
					** was held before */
	if (DB_FAILURE_MACRO(scf_status))
	    return(scf_status);	    /* return with SCF status, since it has been
                                    ** reported already */
    }
    if (DB_FAILURE_MACRO(status))
    {	/* report error, unless it is a recoverable error.  Currently,
	** the only recoverable error is:
	**	E_DM013A_INDEX_COUNT_MISMATCH	
	*/
	
	if (dmt_shw_cb->error.err_code == E_DM004D_LOCK_TIMER_EXPIRED)
	{
	    /* Suppress printing of E_DM004D_LOCK_TIMER_EXPIRED and mapped
	    ** rdf error E_RD002B so that user is only returned E_US125E
	    ** in psf.
	    */
	    rdu_ferror(global, status, &dmt_shw_cb->error,
	                E_RD0060_DMT_SHOW,E_DM004D_LOCK_TIMER_EXPIRED);
	} 
	else if (dmt_shw_cb->error.err_code != E_DM013A_INDEX_COUNT_MISMATCH)
	{
	    rdu_ferror(global, status, &dmt_shw_cb->error, 
		       E_RD0060_DMT_SHOW,0);
	}
    }
    return(status);
}

/*{
** Name: rdu_cache	- find the ULH object in the cache
**
** Description:
**      Routine to call ULH and find or create the object associated with 
**      the table ID.
**
** Inputs:
**      global                          ptr to RDF global state variable
**	build				internal counter for statistics
** Outputs:
**	build				internal counter for statistics
**					    RDF_BUILT_FROM_CATALOGS or
**					    RDF_ALREADY_EXISTS
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-oct-87 (seputis)
**          initial creation
**	09-apr-90 (teg)
**	    add synonym support
**	18-apr-90 (teg)
**	    interface to rdu_ferror changed for fix to bug 4390
**	04-may-90 (teg)
**	    change RDF to treat E_UL0119_ALIAS_MEM as not an error if it
**	    is defining an alias to a synonym.
**	05-feb-91 (teresa)
**	    improved RDF statistics reporting.
**	23-jan-92 (teresa)
**	    SYBIL Changes:  rdi_hashid->rdv_rdesc_hashid; eleminate rdi_alias
**	08-may-92 (teresa)
**	    SYBIL changes:  implement rdr2_refresh logic
**	16-jul-92 (teresa)
**	    prototypes
**	21-may-93 (teresa)
**	    fixed type casting for newly prototyped ulh calls.
**	18-feb-94 (teresa)
**	    fix bug 59336 - Have RDF validate the checksum when taking an 
**	    existing infoblk off of the cache.
**	15-mar-94 (teresa)
**	    Fix bug 60609 -- When distributed, RDF must assure that it's found
**	    the correct type of object on the cache.  We could be asked for
**	    a REGPROC when the cache contains a table/view or vica versa.
**	05-Jun-1997 (shero03)
**	    Only validate the checksum if the RDF_CHECKSUM is turned on.
**	16-Mar-2004 (schka24)
**	    The show-dmf routine will always fill in the proper tabid for
**	    synonyms, don't do it here (stops "control lock on table id 0"
**	    messages in dmt-show).
**	7-Feb-2008 (kibro01) b119744
**	    Initialise temp_global
**	19-Feb-2008 (kibro01) b119744
**	    Initialise second (and only other) temp_global
**	19-oct-2009 (wanfr01) Bug 122755
**	    rdf_shared_sessions must be changed under rdf_gsemaphore protection
**      22-Jan-2009 (hanal04) Bug 123169
**          If rdu_shwdmfcall() invalidates the object (sets refreshed to
**          TRUE) make sure we update ulh_fixed accordingly.
[@history_template@]...
*/

static DB_STATUS
rdu_cache(  RDF_GLOBAL	*global,
	    RDF_BUILD	*build,
	    bool	*refreshed)
{
    DB_TAB_ID	    tab_id;
    RDF_SYVAR	    syn_info;
    DB_STATUS	    status;
    RDF_CB	    *rdfcb;
    typedef struct _OBJ_ALIAS {
	i4		db_id;
	DB_TAB_NAME	tab_name;
	DB_OWN_NAME	tab_owner;
    } OBJ_ALIAS;		    /* structure used to define an alias
				    ** by name to the object */
    OBJ_ALIAS		obj_alias;  /* alias to name PSF asked for */
    OBJ_ALIAS		tbl_alias;  /* table name if PSF asked for a synonym*/
    bool		bad_cksum = FALSE;
    i4		v1,v2;
    bool            dmfshw_refresh = FALSE;

    /* the value of tab_id will be reset by rdu_shwdmfcall if the request is
    ** really for a synonym.  If it remains 0, then either:
    **	1) the request was NOT by name (RDR_BY_NAME)
    **	2) the request was by name (RDR_BY_NAME), but the name was not a synonym
    */
    tab_id.db_tab_base = 0;
    tab_id.db_tab_index = 0;
    syn_info = 0;

    rdfcb = global->rdfcb;
    /* obtain access to a cache object */
    if (!(global->rdf_init & RDF_IULH))
    {	/* init the ULH control block if necessary, only need the server
        ** level hashid obtained at server startup */
	global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	global->rdf_init |= RDF_IULH;
    }
    if (rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME)
    {
	bool	    ulh_fixed;

	ulh_fixed = FALSE;
	/* Try to get descriptor by alias */
	obj_alias.db_id = rdfcb->rdf_rb.rdr_unique_dbid;
	STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_tabname, 
						obj_alias.tab_name);
	STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner, obj_alias.tab_owner);
	status = ulh_getalias(&global->rdf_ulhcb, (unsigned char *) &obj_alias, 
	    sizeof(obj_alias));
	if (DB_FAILURE_MACRO(status))
	{
	    if((global->rdf_ulhcb.ulh_error.err_code != E_UL0109_NFND)
		&&
		(global->rdf_ulhcb.ulh_error.err_code != E_UL0121_NO_ALIAS))
	    {
		rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
		    E_RD012D_ULH_ACCESS,0);
		return(status);
	    }
	}
	else
	{	/* need to find table ID */
	    RDF_ULHOBJECT	    *rdf_ulhobject;
	    global->rdf_resources |= RDF_RULH;  /* mark ulh resource
					** as being fixed */
	    rdf_ulhobject = (RDF_ULHOBJECT *) global->rdf_ulhcb.
		ulh_object->ulh_uptr;
	    ulh_fixed = TRUE;
	    Rdi_svcb->rdvstat.rds_f0_relation++;	/* update statistics */
	    
	    if (rdf_ulhobject && rdf_ulhobject->rdf_sysinfoblk
		&& rdf_ulhobject->rdf_sysinfoblk->rdr_rel)
	    {   
                /* we have a cache object that has atleast some data in
                ** it and CHECKSUM is turned on - 
		** validate the checksum.  If the checksum fails,
                ** then we have memory corruption.  Attempt to recover
                ** by:
                **      1.  Increment ctr
                **      2.  If ctr not max, then invalidate this obj
                **      3.  If ctr hits max, then invalidate whole cache.
                ** We must have semaphore protection to validate the checksum.
		**
		** However, if we have been asked to refresh this infoblk, then
		** do not bother to validate the checksum.
                */

                if ( !(rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH) &&
                       ult_check_macro(&Rdi_svcb->rdf_trace,
					RDU_CHECKSUM, &v1, &v2) )
	        {
		    
		    global->rdf_ulhobject = rdf_ulhobject;
                    status = rdu_gsemaphore(global);
                    if (DB_FAILURE_MACRO(status))
                        return(status);

                    if (rdf_ulhobject->rdf_sysinfoblk->rdr_checksum !=
                               rdu_com_checksum(rdf_ulhobject->rdf_sysinfoblk))
                    {
                        bad_cksum = TRUE;
                        if( ++Rdi_svcb->rdv_bad_cksum_ctr >
                                                             RDV_MAX_BAD_CKSUMS)
                        {
                            Rdi_svcb->rdv_bad_cksum_ctr = 0;

			    /* invalidate the entire cache */
			    status = ulh_clean(&global->rdf_ulhcb);
			    if (DB_FAILURE_MACRO(status))
			    {
				rdu_ferror(global, status, 
				       &global->rdf_ulhcb.ulh_error, 
				       E_RD0040_ULH_ERROR,0);
				return(status);
			    }
                            rdu_ierror(global,E_DB_ERROR, E_RD0101_FLUSH_CACHE);
			    return (E_DB_ERROR);
                        }
                    }

                    status = rdu_rsemaphore(global);
                    if (DB_FAILURE_MACRO(status))
                        return(status);
                }

		/* see if RDF is requested to flush existing cache entries 
		** or if there has been a checksum error (indicating memroy
		** corruption
		*/
		if ( 
		     (rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH)
		   ||
		     bad_cksum
	           )
		{
		    RDF_GLOBAL	temp_global = {0};
		    DB_STATUS	inval_stat;

		    /* invalidate this cache entry and behave as though we've
		    ** not found this entry 
		    */
		    *refreshed =  TRUE;			
		    temp_global.rdf_sess_cb = global->rdf_sess_cb;
		    temp_global.rdf_sess_id = global->rdf_sess_id;
		    temp_global.rdf_init=0;
		    temp_global.rdfcb = global->rdfcb;
		    temp_global.rdf_resources = 0;
		    temp_global.rdf_ulhcb.ulh_object = 
						   global->rdf_ulhcb.ulh_object;
		    inval_stat = invalidate_relcache (&temp_global, 
						rdfcb,
						rdf_ulhobject->rdf_sysinfoblk,
						(DB_TAB_ID *) NULL);
		    if (DB_FAILURE_MACRO(inval_stat))
		    {
			/* 
			** release any resources held by failed 
			** invalidate_relcache 
			*/
			rdf_release( &temp_global, &inval_stat);
			return inval_stat;
		    }

		    /* mark ulh resource as not in use */
		    global->rdf_resources &= ~RDF_RULH;  
		    ulh_fixed = FALSE;

		    /* raise consistency check if this was a bad checksum */
		    if (bad_cksum)
		    {
			rdu_ierror(global, E_DB_ERROR, E_RD010D_BAD_CKSUM);
			return (E_DB_ERROR);
		    }
		}
		else
		{
		    if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
		    {
			DD_OBJ_DESC	*obj;

			/* 
			** We are distributed:
			** bug 60609 -- there is a slim chance that we can be
			** asked to provide info on a regproc:
			**       (global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
			** but we have actually found a table/view by the same
			** owner.name, since regprocs (in 6.5) share the same
			** namespace as tables/views.  The inverse is also true,
			** we could have found a regproc when looking for a
			** table or view.  We need to validate that the object 
			** is of the correct type.
			*/
			if (obj=rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc)
			{

			    if (
			        ((global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
			         &&
			         (obj->dd_o9_tab_info.dd_t3_tab_type !=
				  DD_5OBJ_REG_PROC)
			        )
				||
			        (!(global->rdfcb->rdf_rb.rdr_instr& RDF_REGPROC)
			        &&
			        (obj->dd_o9_tab_info.dd_t3_tab_type ==
				 DD_5OBJ_REG_PROC)
				)
			       )
			    {
				/* here we have found the desired owner.name on
				** the cache, but it is for the wrong type.
				** Either we have been asked for a table/view 
				** and found a REGPROC instead, or we have been
				** asked for a regproc and found a table/view
				** instead.
				** In either case, unfix the object and pretend
				** like we did not find it.  Since we know that
				** namespace is shared and the wrong type of 
				** object was found, we know that querying the
				** catalogs will not yeild the correct object.
				** So we have an enhancement to simply return
				**	E_RD0002_UNKNOWN_TBL
				** and not bother with any more catalog queries.
				*/
				status = E_DB_WARN;
				rdu_ierror(global, status, 
					   E_RD0002_UNKNOWN_TBL);
				status = E_DB_ERROR;

				/*
				** RDF must increment rdf_shared_sessions 'cuz
				** rdf_release will decrement it when it 
				** releases this infoblk.  Yes, I know this is
				** kludgy, but the alternative is even messier..
				*/

				status = rdu_gsemaphore(global); 
				if (DB_FAILURE_MACRO(status))
				    return(E_DB_ERROR);
				CSadjust_counter(&rdf_ulhobject->rdf_shared_sessions, 1);
				rdu_rsemaphore(global); 
				return(E_DB_ERROR);
			    }  /* endif wrong type */

			}   /* endif star the object is built enough to check */
		    }	/* end this is for a distributed object */


		    /* if the relation descriptor information is already
		    ** available then get table ID, since it is required
		    ** for any subsequent show calls to DMF, if it is not
		    ** available then a show call needs to be done , any
		    ** other sequence may cause semaphore and locking
		    ** problems */
		    *build = RDF_ALREADY_EXISTS;
		    STRUCT_ASSIGN_MACRO(rdf_ulhobject->rdf_sysinfoblk->
			rdr_rel->tbl_id, global->rdf_dmtshwcb.dmt_tab_id);
		    STRUCT_ASSIGN_MACRO(rdf_ulhobject->rdf_sysinfoblk->rdr_rel
			->tbl_id, rdfcb->rdf_rb.rdr_tabid);
		    return(status);
		}
	    }
	}
	if (rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	{   
	    /* STAR:  This is a distributed thread, so call distirubted logic
	    **	      to get the object/table info into rdf_tobj and rdf_trel 
	    */

	    global->rdf_ddrequests.rdd_types_mask = RDD_OBJECT;
    	    status = rdd_getobjinfo(global);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* There are three possible scopes of search space:
		**  1.  search only table name space
		**  2.  search only registered procedure name space
		**  3,  Search  both table and registered procedure name space.
		**
		**  If we are not searching both name spaces, just return an
		**  error.  Otherwise, we've just searched the table name
		**  space, so now search the registered procedure name space.
		*/
		if (global->rdfcb->rdf_rb.rdr_instr & RDF_FULLSEARCH)
		{
		    RDF_INSTR old;

		    old = global->rdfcb->rdf_rb.rdr_instr;
		    global->rdfcb->rdf_rb.rdr_instr = (old & RDF_REGPROC) ?
			(old & ~RDF_REGPROC) : (old | RDF_REGPROC);
        	    status = rdd_getobjinfo(global);
		    global->rdfcb->rdf_rb.rdr_instr = old;
		    if (DB_FAILURE_MACRO(status))
			return(status);
		}
		else
		    return(status);


	    }
	    STRUCT_ASSIGN_MACRO(global->rdf_trel.tbl_id, 
				rdfcb->rdf_rb.rdr_tabid);
	    global->rdf_init |= RDF_ITBL;   /* rdf_trel contains table info */

	    *build = RDF_BUILT_FROM_CATALOGS;
	    if (Rdi_svcb->rdvstat.rds_n0_tables < MAXI4)
	    {
		DMT_TBL_ENTRY   *tmp_rel= &global->rdf_trel;
		double temp;

		/*
		** update statistics on average number per table and average 
		** number of indexes per table.  Note, if we are left running 
		** so long that the number of tables counter (rds_n0_table) is 
		** about to overflow, we simply stop calculating avg 
		** index/table and avg col/table statistics 
		*/

		temp = ( Rdi_svcb->rdvstat.rds_n1_avg_col * 
			(float) Rdi_svcb->rdvstat.rds_n0_tables ) +
			(float) tmp_rel->tbl_attr_count;
		Rdi_svcb->rdvstat.rds_n1_avg_col = temp / 
			((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		temp = ( Rdi_svcb->rdvstat.rds_n2_avg_idx * 
			(float) Rdi_svcb->rdvstat.rds_n0_tables )  +
			(float) tmp_rel->tbl_index_count;
		Rdi_svcb->rdvstat.rds_n2_avg_idx = temp /
			((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		Rdi_svcb->rdvstat.rds_n0_tables++;
	    }
	}
	else
	{
	    DMT_SHW_CB      *dmt_shw_cb;
	    DMT_TBL_ENTRY   *tmp_rel;

	    dmt_shw_cb = &global->rdf_dmtshwcb;
	    tmp_rel = &global->rdf_trel;
	    /*
	    ** Obtain the table id by a direct call to DMF by name
	    */
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_name.rdr_tabname, 
		dmt_shw_cb->dmt_name);
	    STRUCT_ASSIGN_MACRO(rdfcb->rdf_rb.rdr_owner, 
		dmt_shw_cb->dmt_owner);
	    dmt_shw_cb->dmt_table.data_address = (char *)tmp_rel;
	    dmt_shw_cb->dmt_table.data_in_size = sizeof(*tmp_rel);

	    status = rdu_shwdmfcall(global, 
			    (i4)(DMT_M_NAME | DMT_M_TABLE),
			    &syn_info,&tab_id,&dmfshw_refresh);
	    if (DB_FAILURE_MACRO(status))
		return(status);		   /* error should have been reported */

            if(dmfshw_refresh == TRUE)
            {
                /* rdu_shwdmfcall() invalidated the object. If it was fixed
                ** it's not anymore.
                */
                *refreshed = dmfshw_refresh;
                ulh_fixed = FALSE;
            }

	    /* check for invalid secondary index, which sometimes occurs 
	    ** because the iirel_idx is out of synch with iirelation, and
	    ** the TIDp field points to the wrong iirelation tuple.
	    ** If the request is NOT by synonym name, validate that:
	    **	 the iirelation tuple looked up by name is actually 
	    **	 returned -- ie, contains the correct table/owner names .
	    ** If we have found a synonym, DO NOT do this check, as we
	    ** are guarenteed that table name will not be the same as
	    ** synonym name.  (The table id returned from
	    ** the rdu_shwdmfcall routine will be non-zero if this is
	    ** a synonym, and zero otherwise.)
	    */
	    if ( tab_id.db_tab_base == 0)
	    {
		/* this was not a synonym request */
		if (MEcmp((PTR)&global->rdf_trel.tbl_name,
		    (PTR)&rdfcb->rdf_rb.rdr_name.rdr_tabname, 
		    sizeof(rdfcb->rdf_rb.rdr_name.rdr_tabname))
		    ||
		    MEcmp((PTR)&global->rdf_trel.tbl_owner,
		    (PTR)&rdfcb->rdf_rb.rdr_owner,
		    sizeof(rdfcb->rdf_rb.rdr_owner))
		    )
		{
		    rdu_ierror(global, E_DB_SEVERE, E_RD0134_INVALID_SECONDARY);
		    return(E_DB_SEVERE);
		}
		/* save the table id in the rdfcb and dmt_shw_cb for future
		** table access */
		STRUCT_ASSIGN_MACRO(tmp_rel->tbl_id, rdfcb->rdf_rb.rdr_tabid);
		STRUCT_ASSIGN_MACRO(tmp_rel->tbl_id, dmt_shw_cb->dmt_tab_id);
	    }
	    /* if it was a synonym, tabids are set inside rdu-shwdmfcall */

	    /* global's trel contains table info */
	    global->rdf_init |= RDF_ITBL;   

	    *build = RDF_BUILT_FROM_CATALOGS;
	    if (Rdi_svcb->rdvstat.rds_n0_tables < MAXI4)
	    {
		double temp;

		/*
		** update statistics on average number per table and average 
		** number of indexes per table.  Note, if we are left running 
		** so long that the number of tables counter (rds_n0_table) is 
		** about to overflow, we simply stop calculating avg 
		** index/table and avg col/table statistics 
		*/

		temp = ( Rdi_svcb->rdvstat.rds_n1_avg_col * 
			(float) Rdi_svcb->rdvstat.rds_n0_tables ) +
			(float) tmp_rel->tbl_attr_count;
		Rdi_svcb->rdvstat.rds_n1_avg_col = temp / 
			((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		temp = ( Rdi_svcb->rdvstat.rds_n2_avg_idx * 
			(float) Rdi_svcb->rdvstat.rds_n0_tables )  +
			(float) tmp_rel->tbl_index_count;
		Rdi_svcb->rdvstat.rds_n2_avg_idx = temp /
			((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		Rdi_svcb->rdvstat.rds_n0_tables++;
	    }
	}

	/* do not fix ULH object if it has already been fixed by
	** the ULH alias call */
	if (ulh_fixed)
	    return(status);

	if (syn_info & RDU_2CACHED)
	{
	    /* the synonym name is new, but the base table that it resolves to
	    ** is already on the RDF cache.  So, just make another ulh_alias 
	    ** for this synonym 
	    */
	    status = ulh_define_alias(&global->rdf_ulhcb, 
				      (unsigned char *)&obj_alias, 
				      sizeof(obj_alias));
	    if (DB_FAILURE_MACRO(status))
	    {
		if (global->rdf_ulhcb.ulh_error.err_code==E_UL0119_ALIAS_MEM)
		{
		    /* warn that RDF could not make an alias to the synonym,
		    ** so preformance on the synonym will be degraded. This
		    ** is not an error condition, since RDF can always get the
		    ** synonym name from dbms catalog iisynonym and find the
		    ** entry on the cache. */
		    rdu_warn (global, E_RD0202_ALIAS_DEGRADED);
		    status = E_DB_OK;
		}
		else 
		if (global->rdf_ulhcb.ulh_error.err_code != E_UL0120_DUP_ALIAS)
		{
		    rdu_ferror(global, status, 
				&global->rdf_ulhcb.ulh_error,
				E_RD0045_ULH_ACCESS,0);
		}
		else
		    status = E_DB_OK;
	    }
	    return(status);
	}
    }

    /* initialize the ULH object name */
    (VOID) rdu_setname(global, &global->rdfcb->rdf_rb.rdr_tabid);
 
    /* Obtain cache object if it exists, else create a new one */
    status = ulh_create(&global->rdf_ulhcb, 
			(unsigned char *) &global->rdf_objname[0], 
			global->rdf_namelen, ULH_DESTROYABLE, 0);
    if (DB_SUCCESS_MACRO(status))
    {

        RDF_ULHOBJECT	*rdf_ulhobject;

	Rdi_svcb->rdvstat.rds_f0_relation++;	/* update statistics */
	rdf_ulhobject =(RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->ulh_uptr;

        if (
	    rdf_ulhobject 
	   && 
	    rdf_ulhobject->rdf_sysinfoblk 
	   &&
	    rdf_ulhobject->rdf_sysinfoblk->rdr_rel
	   )
	{
	    /* we have found an object on the cache for which atleast the
	    ** relation info (maybe all of it) has already been built.
	    ** If we have been asked to refresh the object (RDR2_REFRESH), we
	    ** must invalidate it and recreate an empty object.  If we have
	    ** not been asked to refresh it, then we need to validate it
	    ** by checking the checksum.  Checksum validation must be performed
	    ** under semaphore protection.  If the checksum does not validate,
	    ** 1. We increment a server wide bad checksum counter
	    ** 2. If the counter hits the max tollerance, we reset to zero and
	    **    invalidate the whole cache.
	    ** 3. If the counter does not hit the max tollerance, we invalidate
	    **    this particular object.
	    */
	    if ( !(rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH) )
            {
		global->rdf_ulhobject = rdf_ulhobject;
                status = rdu_gsemaphore(global);
                if (DB_FAILURE_MACRO(status))
                    return(status);
		if(
                   ult_check_macro(&Rdi_svcb->rdf_trace,
				    RDU_CHECKSUM, &v1, &v2)
                  &&
                   rdf_ulhobject->rdf_sysinfoblk->rdr_checksum !=
                                 rdu_com_checksum(rdf_ulhobject->rdf_sysinfoblk)
                  )
		{
		     
		     global->rdf_resources |= RDF_BAD_CHECKSUM;
		     bad_cksum = TRUE;
		     status = rdu_rsemaphore(global);
		     if (DB_FAILURE_MACRO(status))
			 return(status);
                     if( ++Rdi_svcb->rdv_bad_cksum_ctr > RDV_MAX_BAD_CKSUMS)
                     {
                         Rdi_svcb->rdv_bad_cksum_ctr = 0;
			 status = ulh_clean(&global->rdf_ulhcb);
			 if (DB_FAILURE_MACRO(status))
			 {
			     rdu_ferror(global, status,
				       &global->rdf_ulhcb.ulh_error,
				       E_RD0040_ULH_ERROR,0);
			     return(status);
			 } 
	    	     	 status = E_DB_ERROR;
	    		 rdu_ierror(global, status, E_RD0101_FLUSH_CACHE);
	    	     	 return(status);
                     }
                }	
		status = rdu_rsemaphore(global);
                if (DB_FAILURE_MACRO(status))
                    return(status);
             }

	    if (
		 (rdfcb->rdf_rb.rdr_2types_mask & RDR2_REFRESH)
	       ||
		 bad_cksum
               )
	    {
		RDF_GLOBAL		temp_global = {0};
		DB_STATUS		inval_stat;

		/* invalidate this cache entry and behave as though we've
		** not found this entry 
		*/
		*refreshed =  TRUE;
	    
		temp_global.rdf_sess_cb = global->rdf_sess_cb;
		temp_global.rdf_sess_id = global->rdf_sess_id;
		temp_global.rdf_init=0;
		temp_global.rdfcb = global->rdfcb;
		temp_global.rdf_resources = 0;
		temp_global.rdf_ulhcb.ulh_object = global->rdf_ulhcb.ulh_object;
		inval_stat = invalidate_relcache( &temp_global, rdfcb,
						  rdf_ulhobject->rdf_sysinfoblk,
						  (DB_TAB_ID *) NULL);
		if (DB_FAILURE_MACRO(inval_stat))
		{
		    /* 
		    ** release any resources held by failed 
		    ** invalidate_relcache 
		    */
		    rdf_release( &temp_global, &inval_stat);
		    return inval_stat;
		}

		/* if we had a back checksum, raise a consistency point and
		** abort statement.
		*/
		if (bad_cksum)
		{	
		    rdu_ierror(global, E_DB_ERROR, E_RD010D_BAD_CKSUM);
		    return (E_DB_ERROR);
		}

		/* now recreate an empty ulh object */
		status = ulh_create(&global->rdf_ulhcb, 
				    (unsigned char *)&global->rdf_objname[0],
				    global->rdf_namelen, ULH_DESTROYABLE, 0);
		if (DB_FAILURE_MACRO(status))
		{
		    rdu_ferror(global, status, 
			       &global->rdf_ulhcb.ulh_error,
			       E_RD0045_ULH_ACCESS,0);
		    return (status);
		}
       	    }  /* endif -- we have been asked to refresh or found a bad cksum */
	}  /* end if -- ulh_create returned an already created object */

	global->rdf_resources |= RDF_RULH; /* mark ULH object as being
				    ** fixed */
	/* if we are creating a cache entry for a synonym, we also need
	** to create an alias for the real table name.  This is necessary 
	** because: 1) it prevents objects from getting on the RDF cache more
	** than once and 2) view definitions involving synonyms resolve to the
	** real table name in the query text -- which means that queries to a
	** view built on the synonym name will actually request the table name.
	**
	** We build the alias for the real name first. This is because it is
	** a fatal error if we cannot build an alias for the table name, but it
	** is only a performance degration if we cannot build an alias for the
	** synonym name.
	*/
	if (rdfcb->rdf_rb.rdr_types_mask & RDR_BY_NAME)
	{
	    if (tab_id.db_tab_base)
	    {
		/* PSF asked for a synonym, so 
		** build an alais to the real table/owner name
		*/
		tbl_alias.db_id = rdfcb->rdf_rb.rdr_unique_dbid;
		STRUCT_ASSIGN_MACRO(global->rdf_trel.tbl_name,
				    tbl_alias.tab_name);
		STRUCT_ASSIGN_MACRO(global->rdf_trel.tbl_owner,
				    tbl_alias.tab_owner);

		status = ulh_define_alias(&global->rdf_ulhcb, 
					  (unsigned char *) &tbl_alias, 
					  sizeof(tbl_alias));
		if (DB_FAILURE_MACRO(status))
		{
		    if (global->rdf_ulhcb.ulh_error.err_code != 
			     E_UL0120_DUP_ALIAS)
		    {
			rdu_ferror(global, status, 
				    &global->rdf_ulhcb.ulh_error,
				    E_RD0045_ULH_ACCESS,0);
			return(status);
		    }
		    else
			status = E_DB_OK;
		}

	    } /* endif we must build an alias for the table name */	

	    status = ulh_define_alias(&global->rdf_ulhcb, 
				      (unsigned char *) &obj_alias, 
				      sizeof(obj_alias));
	    if (DB_FAILURE_MACRO(status))
	    {
		if ( (global->rdf_ulhcb.ulh_error.err_code==E_UL0119_ALIAS_MEM)
		     &&
		     (tab_id.db_tab_base))
		{
		    /* warn that RDF could not make an alias to the synonym,
		    ** so preformance on the synonym will be degraded. This
		    ** is not an error condition, since RDF can always get the
		    ** synonym name from dbms catalog iisynonym and find the
		    ** entry on the cache. */
		    rdu_warn (global, E_RD0202_ALIAS_DEGRADED);
		    status = E_DB_OK;
		}
		else if (global->rdf_ulhcb.ulh_error.err_code != 
			 E_UL0120_DUP_ALIAS)
		{
		    rdu_ferror(global, status, 
				&global->rdf_ulhcb.ulh_error,
				E_RD0045_ULH_ACCESS,0);
		    return(status);
		}
		else
		{
		    /* this was probably caused by concurent activity, where
		    ** another thread was building about the same time that
		    ** this thread was.
		    */
		    status = E_DB_OK;
		}
	    } /* endif alias define failed */
	} /* we need to define an alias cuz object was requested by name */
    } /* endif ulh create succeeded */
    else
	rdu_ferror(global, status, &global->rdf_ulhcb.ulh_error,
	    E_RD0045_ULH_ACCESS,0);
    return (status);		    /* some other ULH error occurred so
				    ** report it and return */
}

/*{
** Name: rdu_relbld	- build the relation relation descriptor
**
** Description:
**      Routine will obtain the relation relation descriptor from DMF
**      and initialize the RDR_REL structure for the caller.
**
** Inputs:
**      global                          ptr to RDF state descriptor
**
** Outputs:
**	build				RDF_BUILT_FROM_CATALOGS if cache obj
**					  is successfully created
**					RDF_ALREADY_EXISTS if cache object
**					  already exists and does not need to
**					  be built.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-oct-87 (seputis)
**          initial creation
**	31-jan-91 (teresa)
**	    improve statistics reporting
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-94 (teresa)
**	    Fix bug 59336 -- need to recalculate checksum each time we
**	    update the infoblk.
**	15-Mar-2004 (schka24)
**	    Delete duplicated code; protect setting of sysinfo state.
[@history_template@]...
*/
static DB_STATUS
rdu_relbld( RDF_GLOBAL  *global,
	    RDF_BUILD	*build,
	    bool	*refreshed)
{
    DB_STATUS		status;
    DMT_TBL_ENTRY	*rel_ptr;

    status = rdu_malloc(global, (i4)sizeof(*rel_ptr), (PTR *)&rel_ptr);
    if (DB_FAILURE_MACRO(status))
	return(status);

    if (global->rdf_init & RDF_ITBL)
    {	/* the table descriptor has already been looked by name so just copy
        ** it to the newly allocated buffer */
	MEcopy((PTR)&global->rdf_trel, sizeof(*rel_ptr), (PTR)rel_ptr);
    }
    else
    {
	/* we only get here in the event that the request was by table id (not
	** by name) and the object was not alreaddy on the cache.   This could
	** occur if the object was flushed off the cache by ULH to make room
	** for another object, and possibly by walking some trees (for a
	** procedure or rule)
	*/

	if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	{   
	    /* If this is a distributed thread, hanle it here */
	    global->rdf_ddrequests.rdd_types_mask = RDD_OBJECT;
    	    status = rdd_getobjinfo(global);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    else
	    {
		*build =  RDF_BUILT_FROM_CATALOGS;
		if (Rdi_svcb->rdvstat.rds_n0_tables < MAXI4)
		{
		    double avg;
		
		/*
		** update statistics on average number per table and average number
		** of indexes per table.  Note, if we are left running so long that 
		** the number of tables counter (rds_n0_table) is about to overflow,
		** we simply stop calculating avg index/table and avg col/table 
		** statistics 
		*/

		    avg = ( Rdi_svcb->rdvstat.rds_n1_avg_col * 
			    (float) Rdi_svcb->rdvstat.rds_n0_tables ) +
			    (float) rel_ptr->tbl_attr_count;
		    Rdi_svcb->rdvstat.rds_n1_avg_col = avg / 
			    ((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		    avg = ( Rdi_svcb->rdvstat.rds_n2_avg_idx * 
			    (float) Rdi_svcb->rdvstat.rds_n0_tables )  +
			    (float) rel_ptr->tbl_index_count;
		    Rdi_svcb->rdvstat.rds_n2_avg_idx = avg /
			    ((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		    Rdi_svcb->rdvstat.rds_n0_tables++;
		}
	    }
	    STRUCT_ASSIGN_MACRO(global->rdf_trel, *rel_ptr);
	}
	else
	{
	    DMT_SHW_CB		*dmt_shw_cb;
	    dmt_shw_cb = &global->rdf_dmtshwcb;
	    dmt_shw_cb->dmt_table.data_in_size = sizeof(*rel_ptr);
	    dmt_shw_cb->dmt_table.data_address = (PTR)rel_ptr;

	    /* Get the relation information */
	    status = rdu_shwdmfcall(global, (i4)DMT_M_TABLE,
				    (RDF_SYVAR *) NULL, (DB_TAB_ID *) NULL,
				    refreshed);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    else 
	    {
		*build =  RDF_BUILT_FROM_CATALOGS;
		if (Rdi_svcb->rdvstat.rds_n0_tables < MAXI4)
		{
		    double avg;
		
		/*
		** update statistics on average number per table and average number
		** of indexes per table.  Note, if we are left running so long that 
		** the number of tables counter (rds_n0_table) is about to overflow,
		** we simply stop calculating avg index/table and avg col/table 
		** statistics 
		*/

		    avg = ( Rdi_svcb->rdvstat.rds_n1_avg_col * 
			    (float) Rdi_svcb->rdvstat.rds_n0_tables ) +
			    (float) rel_ptr->tbl_attr_count;
		    Rdi_svcb->rdvstat.rds_n1_avg_col = avg / 
			    ((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		    avg = ( Rdi_svcb->rdvstat.rds_n2_avg_idx * 
			    (float) Rdi_svcb->rdvstat.rds_n0_tables )  +
			    (float) rel_ptr->tbl_index_count;
		    Rdi_svcb->rdvstat.rds_n2_avg_idx = avg /
			    ((float) Rdi_svcb->rdvstat.rds_n0_tables + 1);
		    Rdi_svcb->rdvstat.rds_n0_tables++;
		}
	    }
	}
    }

    {
	RDR_INFO	*sys_infoblk;

	sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
	if (global->rdf_resources & RDF_RUPDATE)
	{   /* relation descriptor has been allocated from the ULH memory stream
	    ** since there is an exclusive update lock on the object.
	    ** Protect sysinfo block state change in case someone is
	    ** making a private copy at this instant.
	    */
	    status = rdu_gsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    sys_infoblk->rdr_no_attr = rel_ptr->tbl_attr_count;
	    sys_infoblk->rdr_no_index = rel_ptr->tbl_index_count;
	    sys_infoblk->rdr_invalid = FALSE;
	    sys_infoblk->rdr_rel = rel_ptr;
	    sys_infoblk->rdr_types |= RDR_RELATION;

	    /* Assign object desc from tobj for distributed case */
	    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	    {
		/* If RDR_CHECK_EXIST, this means PSF calls for a partial 
		** descriptor.  Mark the info block as invalid so that it won't
		** be reused at later requests. Note that we also invalidate 
		** infoblk for RDR_REFRESH */
		if (global->rdfcb->rdf_rb.rdr_types_mask & 
		    (RDR_CHECK_EXIST | RDR_REFRESH)
		   )
		    sys_infoblk->rdr_invalid = TRUE;

		STRUCT_ASSIGN_MACRO(global->rdf_tobj, 
				    *sys_infoblk->rdr_obj_desc);
		if (!(rel_ptr->tbl_status_mask & DMT_VIEW)
		    &&
		    (global->rdf_tobj.dd_o9_tab_info.dd_t3_tab_type != DD_0OBJ_NONE)
		    &&
		    !(global->rdfcb->rdf_rb.rdr_types_mask & RDR_REMOVE)
		   )
		    sys_infoblk->rdr_ldb_id = 
		    global->rdf_tobj.dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc.dd_l5_ldb_id;
		sys_infoblk->rdr_types |= RDR_DST_INFO;
		
	    }
	    status = rdu_rsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	if (sys_infoblk != global->rdfcb->rdf_info_blk)
	{   /* user has a private info block which must also be updated */
	    RDR_INFO	*usr_infoblk;
	    usr_infoblk = global->rdfcb->rdf_info_blk;
	    usr_infoblk->rdr_no_attr = rel_ptr->tbl_attr_count;
	    usr_infoblk->rdr_no_index = rel_ptr->tbl_index_count;
	    usr_infoblk->rdr_invalid = FALSE;
	    usr_infoblk->rdr_rel = rel_ptr;
	    usr_infoblk->rdr_types |= RDR_RELATION;
	    /* If RDR_CHECK_EXIST, this means PSF calls for a partial 
	    ** descriptor.  Mark the info block as invalid so that it won't be 
	    ** reused at later requests. Note that we also invalidate infoblk 
	    ** for RDR_REFRESH */
	    if (global->rdfcb->rdf_rb.rdr_types_mask & 
		(RDR_CHECK_EXIST | RDR_REFRESH)
	       )
		usr_infoblk->rdr_invalid = TRUE;
	    /* Assign object desc from tobj for distributed case */
	    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	    {
		STRUCT_ASSIGN_MACRO(global->rdf_tobj, *usr_infoblk->rdr_obj_desc);
		if (!(rel_ptr->tbl_status_mask & DMT_VIEW))
		    usr_infoblk->rdr_ldb_id = 
		    global->rdf_tobj.dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc.dd_l5_ldb_id;
		usr_infoblk->rdr_types |= RDR_DST_INFO;

	    }
	}
    }
    return(E_DB_OK);
}

/*{
** Name: rdu_bldattr	- build attributes descriptor
**
** Description:
**      Routine will build and add the attributes descriptor to the info
**      control block
**
**	If we end up doing a DMT SHOW against the table, ask for the
**	physical partition info as well.  This isn't really the right
**	place for it, but there's no point in doing extra show's, as
**	they are not terribly cheap.  The typical sequence on a table
**	that isn't in the RDF cache at all is a show-by-name for the
**	base table info, then another show-by-ID for everything else.
**	(Two shows are needed as we need the table info to know how
**	big the other areas will be.  Unfortunately DMT SHOW can't
**	just build the areas for us.)
**
** Inputs:
**      global                          ptr to RDF state variable
**	build				value will be: RDF_BUILT_FROM_CATALOGS
**					 or RDF_ALREADY_EXISTS
**					 or RDF_NOTFOUND
** Outputs:
**	build				Contents may remain unchanged.  If they
**					are changed, they will be set to:
**					  RDF_BUILT_FROM_CATALOGS if cache obj
**					  is successfully created
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-oct-87 (seputis)
**          initial creation
**      19-oct-88 (seputis)
**          fixed race condition
**	1-mar-90 (teg)
**	    speeded up a little by removing unnecessary dereference and using
**	    local varaible which already had the value.
**	31-jan-91 (teresa)
**	    improve statistics reporting
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-94 (teresa)
**	    Fix bug 59336 -- need to recalculate checksum each time we
**	    update the infoblk.
**	11-nov-1998 (nanpr01)
**	    Allocate a single chunk rather than allocating small pieces of
**	    memory separately.
**	18-Jan-2004 (schka24)
**	    Ask for the PP array if we're doing a show.
**	27-Jul-2004 (schka24)
**	    When we were generating a private infoblk, the master copy's
**	    pparray wasn't being copied.  Remove this code anyway and
**	    send the private infoblk back thru dmt-show.
[@history_template@]...
*/
static DB_STATUS
rdu_bldattr(	RDF_GLOBAL         *global,
		RDF_BUILD	   *build,
		bool		    *refreshed)
{
    DMT_ATT_ENTRY   **attr;		/* ptr to DMT_ATT_ENTRY ptr array */
    DMT_PHYS_PART   *pp_array;		/* Pointer to PP array */
    DMT_SHW_CB	    *dmt_shw_cb;	/* DMF SHOW control block */
    DD_COLUMN_DESC  **attrmap = NULL;	/* ptr to DD_COLUMN_DESC ptr array
					** for mapped columns in distributed
					** mode. */
    DB_STATUS	    status;
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    bool	    updating_master;

    usr_infoblk = global->rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    updating_master = (usr_infoblk == sys_infoblk);

    /* If we're here, the user infoblk must want attribute (and pp-array)
    ** info filled in.
    ** RDF used to copy the master attribute info pointer to the private
    ** copy.  This could only happen if someone fetch attr info into the
    ** master copy in the tiny window between making our private copy,
    ** and getting here.  Since having user infoblk's pointing to master
    ** stream data is dangerous anyway, I've removed the confusing code.
    **
    ** FIXME propagate this fix to other "build" routines here, I don't
    ** have time to do it right now and the pparray thing is the real fix
    */

    /* use the usr_infoblk since it is guaranteed to have rdr_rel defined
    ** whereas the sys_infoblk does not */
    if (usr_infoblk->rdr_no_attr == 0)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD013B_BAD_ATTR_COUNT);
	return(status);
    }
    status = rdu_malloc(global, 
	(i4)(((usr_infoblk->rdr_no_attr + 1) * sizeof(PTR)) +
	      (sizeof(DMT_ATT_ENTRY) * usr_infoblk->rdr_no_attr)), 
	(PTR *)&attr);		/* allocate memory for array of pointers
				    ** to attribute descriptors */
    if (DB_FAILURE_MACRO(status))
	return(status);
    attr[DB_IMTID] = (DMT_ATT_ENTRY *) NULL; /* initialize the TID attribute */


    {
	i4		    att_cnt;
	DMT_ATT_ENTRY   *attr_entry;

	attr_entry = (DMT_ATT_ENTRY *)((char *)attr + 
			    ((usr_infoblk->rdr_no_attr + 1) * sizeof(PTR)));
	for (att_cnt = 1; att_cnt <= usr_infoblk->rdr_no_attr; att_cnt++)
	{
	    attr[att_cnt] = &attr_entry[att_cnt - 1];
	}
    }

    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {   
	/* this is a distributed thread */
	global->rdf_ddrequests.rdd_types_mask = RDD_ATTRIBUTE;
	global->rdf_ddrequests.rdd_attr = attr;
	status = rdd_getobjinfo(global);   /* get attribute information
					   ** from CDB */	    
	if (DB_FAILURE_MACRO(status))
	{
	    /* unable to retrieve column info is a serious problem. we need to
	    ** destroy the object from the cache. */
	    global->rdf_resources |= RDF_CLEANUP;
	    return(status);
	}

	/* if register with refresh, promote schema check errors to caller*/
	if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
	    usr_infoblk->rdr_local_info = global->rdf_local_info;

	/* If distributed flag was set and attributed were mapped 
	** then allocate memory for mapped columns */
	if (usr_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b)
	{ 
	    /* allocate memory for mapped columns */
	    status = rdu_malloc(global, 
		(i4)(((usr_infoblk->rdr_no_attr + 1) * sizeof(PTR)) +
		       (usr_infoblk->rdr_no_attr * sizeof(DD_COLUMN_DESC))),
		(PTR *)&attrmap);
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    /* initialize the TID attribute */
	    attrmap[DB_IMTID] = (DD_COLUMN_DESC *)NULL;

	    {   
		i4		    att_cnt;
		DD_COLUMN_DESC  *attrmap_entry;	

		attrmap_entry = (DD_COLUMN_DESC *)((char *)attrmap +
			    ((usr_infoblk->rdr_no_attr + 1) * sizeof(PTR)));
		for (att_cnt = 1; att_cnt <= usr_infoblk->rdr_no_attr; 
		     att_cnt++)
		{
		    attrmap[att_cnt] = &attrmap_entry[att_cnt - 1];
		}
	    }
	    global->rdf_ddrequests.rdd_types_mask = RDD_MAPPEDATTR;
	    global->rdf_ddrequests.rdd_mapattr = attrmap;
	    status = rdd_getobjinfo(global);    /* get mapped attribute info
						** from CDB */	    
	    if (DB_FAILURE_MACRO(status))
	    {  
	       /* unable to retrieve column info is a serious problem. 
	       ** We need to destroy the object from the cache. */
	       global->rdf_resources |= RDF_CLEANUP;
	       return(status);
	    }
	    else
		*build =  RDF_BUILT_FROM_CATALOGS;
	}
    }
    else
    {
	/* Normal, non-STAR case */

	i4		ppsize;
	i4		show_flags;

	show_flags = DMT_M_ATTR | DMT_M_COUNT_UPD;
	dmt_shw_cb = &global->rdf_dmtshwcb;
	dmt_shw_cb->dmt_phypart_array.data_address = NULL;
	dmt_shw_cb->dmt_phypart_array.data_in_size = 0;

	/* If we're doing a show of a partitioned table, ask for the
	** physical partition array too.  This isn't really the right
	** place to do it, but extra dmt-show calls are to be eschewed.
	*/
	if (usr_infoblk->rdr_rel->tbl_status_mask & DMT_IS_PARTITIONED)
	{
	    show_flags = show_flags | DMT_M_PHYPART;
	    ppsize = sizeof(DMT_PHYS_PART) * usr_infoblk->rdr_rel->tbl_nparts;
	    status = rdu_malloc(global, ppsize,
		    &dmt_shw_cb->dmt_phypart_array.data_address);
	    dmt_shw_cb->dmt_phypart_array.data_in_size = ppsize;
	}
	dmt_shw_cb->dmt_attr_array.ptr_address = (PTR)(attr);
	if (usr_infoblk->rdr_no_attr == 0)
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD013B_BAD_ATTR_COUNT);
	    return(status);
	}
	dmt_shw_cb->dmt_attr_array.ptr_in_count = usr_infoblk->rdr_no_attr + 1;
	dmt_shw_cb->dmt_attr_array.ptr_size = sizeof(DMT_ATT_ENTRY); 
	status = rdu_shwdmfcall(global, show_flags,
			    (RDF_SYVAR *) NULL, (DB_TAB_ID *) NULL,
			    refreshed);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	else
	    *build =  RDF_BUILT_FROM_CATALOGS;
    }

    /* Store the info into the user info block (might be the same as the
    ** master info block).
    */
    if (updating_master)
    {
	status = rdu_gsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    usr_infoblk->rdr_attr = attr;
    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	if (usr_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b)
	    usr_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t8_cols_pp = attrmap;
    }
    else
    {
	/* Update latest counts: */
	usr_infoblk->rdr_rel->tbl_record_count = dmt_shw_cb->dmt_reltups;
	usr_infoblk->rdr_rel->tbl_page_count = dmt_shw_cb->dmt_relpages;
	usr_infoblk->rdr_pp_array = (DMT_PHYS_PART *) dmt_shw_cb->dmt_phypart_array.data_address;
    }
    usr_infoblk->rdr_types |= RDR_ATTRIBUTES;
    if (updating_master)
    {
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }

    return(E_DB_OK);
}

/**
**	
** Name: rdu_key_ary_bld - Build the primary key array.
**
**	Internal call format:	rdu_key_ary_bld(global);
**
** Description:
**      This function builds the primary key array. Each entry of the array
**	is the attribute number which composes of the primary key associated
**	with the table.
**	
** Inputs:
**
**	global				RDF request-state block
**
** Outputs:
**      rdf_cb                          
**		.rdf_info_blk		
**					
**			.rdr_keys	pointer to the primary key of a table.
**	Returns:
**		DB_STATUS
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      10-apr-86 (ac)    
**          written
**      21-aug-86 (seputis)
**          add 1 to rdr_no_attr so that null termination has storage
**          modified to allocate exact storage for array rather than worst case
**      27-oct-87 (seputis)
**          modified to use RDF state variable
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-94 (teresa)
**	    Fix bug 59336 -- need to recalculate checksum each time we
**	    update the infoblk.
**	2-Jan-2004 (schka24)
**	    Don't set bld-phys flag here, will do after reading partitioning
**	    catalogs.  (Don't set the flag when we're half done!)
**	15-Oct-2007 (kschendel) b144021
**	    Simplify code, remove pseudo-optimizations.
[@history_template@]...
*/
static DB_STATUS
rdu_key_ary_bld(RDF_GLOBAL *global)
{
    RDR_INFO		*sys_infoblk;
    RDR_INFO		*usr_infoblk;
    DB_STATUS		status;
    RDD_KEY_ARRAY	*rdr_keys;	    /* ptr to key structure */
        
    usr_infoblk = global->rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;

    /* Use the information in the cache as long as it is available.  However,
    ** there are three cases we can not use the info in the cache.  First, if
    ** global->rdf_resources indicates RDF_REFRESH, meaning, RDR_REFRESH is
    ** requested from PSF for register with refresh, we will bring back
    ** all necessary information for a private infoblock and mark sys_infoblk
    ** as invalid. Second, if RDF_BAD_CHECKSUM is set, meaning, cached object
    ** is trashed or nolonger valid, we need to bring back object info
    ** from database.  Third case is that RDR_CHECK_EXIST is set, means cached
    ** object will be partially filled and sys_infoblk will be marked as invalid
    ** as previous cases.  */

    if ((sys_infoblk != usr_infoblk)
	&&
	(sys_infoblk->rdr_attr == usr_infoblk->rdr_attr)
	&&
	(sys_infoblk->rdr_keys)
	&&
        !(global->rdfcb->rdf_rb.rdr_types_mask & (RDR_CHECK_EXIST | RDR_REFRESH))
	&&
        !(global->rdf_resources & RDF_BAD_CHECKSUM)
	)
    {	/*
	** check for case in which a private descriptor is being built
	** but the attributes of the shared descriptor is being used
	** so that the keys of the shared descriptor can be used */
	rdr_keys = sys_infoblk->rdr_keys;
    }
    else
    {
	i4		max_key_seq;	    /* maximum key sequence number
					    ** for a consistency check */
	i4		no_of_key;
	DMT_ATT_ENTRY 	**attr;
	DMT_ATT_ENTRY 	**attpp;

	if ((global->rdf_resources & RDF_RUPDATE)
	    &&
	    (sys_infoblk != usr_infoblk)
	    &&
	    (sys_infoblk->rdr_attr == usr_infoblk->rdr_attr)
	    )
	{   /* since this is a private control block, and we have an update
	    ** lock, memory will be allocated from the shared memory stream
	    ** this will be placed in the shared control block, so make
	    ** sure the attribute arrays are equal, since the key table
	    ** points to element in the attribute arrays, this will be true
	    ** if any request for rdr_attr never a separate call from rdr_keys
	    */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0113_KEYS);
	    return(status);
	}
	attr = usr_infoblk->rdr_attr;
	/* Count 'em first to see how much memory is needed.
	** Remember that attrs are counted one origin.
	** "attr" is an array of pointers, not an array of attrs.
	*/
	no_of_key = 0;
	max_key_seq = 0;
	for (attpp = &attr[usr_infoblk->rdr_no_attr];
	     attpp >= &attr[1];
	     --attpp)
	{
	    if ((*attpp)->att_key_seq_number > 0)
	    {
		++ no_of_key;
		if ((*attpp)->att_key_seq_number > max_key_seq)
		    max_key_seq = (*attpp)->att_key_seq_number;
	    }
	}
	if (max_key_seq != no_of_key)
	{
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0113_KEYS);
					/* inconsistent attribute key info */
	    return (status);
	}
	status = rdu_malloc(global, (i4) (sizeof(RDD_KEY_ARRAY) + 
		sizeof(i4) * (no_of_key + 1)), 
		(PTR *)&rdr_keys);  /* add 1 for null terminator PTR field */

	rdr_keys->key_array = (i4 *)(((char *)rdr_keys) + 
					    sizeof(RDD_KEY_ARRAY));
	for (attpp = &attr[usr_infoblk->rdr_no_attr];
	     attpp >= &attr[1];
	     --attpp)
	{
	    if ((*attpp)->att_key_seq_number > 0)
	    {
		rdr_keys->key_array[(*attpp)->att_key_seq_number-1] =
				(*attpp)->att_number;
	    }
	}
	rdr_keys->key_array[no_of_key] = 0;
	rdr_keys->key_count = no_of_key;
	if (global->rdf_resources & RDF_RUPDATE)
	{   /* rdr_keys needs to be consistent prior to making it visible in the
	    ** system descriptor */
	    sys_infoblk->rdr_keys = rdr_keys;
	}
    }
    if (sys_infoblk != usr_infoblk)
    {	/* user has a private info block which must also be updated */
	usr_infoblk->rdr_keys = rdr_keys;
    }
    return(E_DB_OK);
}


/**
**	
** Name: rdu_getbucket - Hash an attribute name.
**
**	Internal call format:	rdu_getbucket(attr_name)
**
** Description:
**      This function hash an attribute name into a hash bucket.
**	
** Inputs:
**
**	attribute name
**
** Outputs:
**	    none
**	Returns:
**	    hash bucket number
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      10-apr-86 (ac)    
**          written
**	18-mar-90 (teg)
**	    use unsigned char for MOD operation so that Kanji can work.
**	16-jul-92 (teresa)
**	    prototypes
*/
static i4
rdu_getbucket(DB_ATT_NAME *name)
{
    register i4	bucket = 0;
    register i4  	i;
    register u_char	*c;

    c = (u_char *)name->db_att_name;
    for (i=0; i < sizeof (name->db_att_name); i++)
    {
	    bucket += *c;
	    c++;
    }
    bucket %= RDD_SIZ_ATTHSH;
    return(bucket);
}

/**
**	
** Name: rdu_hash_att_bld - Build the hash table of attributes information.
**
**	Internal call format:	rdu_hash_att_bld(global);
**
** Description:
**      This function builds the hash table of attribute information.
**	The hash table of attribute information is built for PSF for 
**	speeding up range variable attributes lookup.
**	
** Inputs:
**
**	global				RDF request-state block
**
** Outputs:
**      rdf_cb                          
**		.sys_infoblk		
**					
**			.rdr_atthsh	pointer to the hash table of attribute
**					information.
**	Returns:
**		DB_STATUS
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      10-apr-86 (ac)    
**          written
**      11-nov-87 (seputis)
**          re-written for resource management
**	05-jun-91 (teresa)
**	    fix bug 36478 by removing tid from hash table if we are building
**	    the hash table for a view.
**	16-jul-92 (teresa)
**	    prototypes
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use appropriate tid attribute descriptor, depending upon the
**	    case translation semantics for regular identifiers for the session.
**	17-feb-94 (teresa)
**	    Fix bug 59336 -- need to recalculate checksum each time we
**	    update the infoblk.
**	15-Mar-2004 (schka24)
**	    Semaphore protect the system info block state changes.
[@history_template@]...
*/
static DB_STATUS
rdu_hash_att_bld(RDF_GLOBAL *global)
{
    RDR_INFO		*sys_infoblk;
    RDR_INFO		*usr_infoblk;
    DB_STATUS		status;
    RDD_ATTHSH		*atthshp;
    RDD_BUCKET_ATT 	*atthshp_entry;

    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    usr_infoblk = global->rdfcb->rdf_info_blk;

    /* Use the information in the cache as long as it is available.  However,
    ** there are three cases we can not use the info in the cache.  First, if
    ** global->rdf_resources indicates RDF_REFRESH, meaning, RDR_REFRESH is
    ** requested from PSF for register with refresh, we will bring back
    ** all necessary information for a private infoblock and mark sys_infoblk
    ** as invalid. Second, if RDF_BAD_CHECKSUM is set, meaning, cached object
    ** is trashed or nolonger valid, we need to bring back object info
    ** from database.  Third case is that RDR_CHECK_EXIST is set, means cached
    ** object will be partially filled and sys_infoblk will be marked as invalid
    ** as previous cases.  */

    if ((sys_infoblk != usr_infoblk)
	&&
	(sys_infoblk->rdr_attr == usr_infoblk->rdr_attr)
	&&
	(sys_infoblk->rdr_atthsh)
	&&
        !(global->rdfcb->rdf_rb.rdr_types_mask & (RDR_CHECK_EXIST | RDR_REFRESH))
	&&
        !(global->rdf_resources & RDF_BAD_CHECKSUM)
	)
    {	/*
	** check for case in which a private descriptor is being built
	** but the attributes of the shared descriptor is being used
	** so that the hash table of the shared descriptor can be used */
	atthshp = sys_infoblk->rdr_atthsh;
    }
    else
    {
	i4		attno;
	DMT_ATT_ENTRY 	**attrpp;		/* get around a compiler bug */
	DMT_ATT_ENTRY   *tidattp;		/* ptr to global descriptor used
						** for TID attribute */
	if ((global->rdf_resources & RDF_RUPDATE)
	    &&
	    (sys_infoblk != usr_infoblk)
	    &&
	    (sys_infoblk->rdr_attr == usr_infoblk->rdr_attr)
	    )
	{   /* since this is a private control block, and we have an update
	    ** lock, memory will be allocated from the shared memory stream
	    ** this will be placed in the shared control block, so make
	    ** sure the attribute arrays are equal, since the hash table
	    ** points to element in the attribute arrays, this will be true
	    ** if any request for rdr_attr never a separate call from rdr_atthsh
	    */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0114_HASH);
	    return(status);
	}
	attrpp = usr_infoblk->rdr_attr;
	tidattp = ((*global->rdf_sess_cb->rds_dbxlate & CUI_ID_REG_U) ?
		   ((RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb))->rdi_utiddesc :
		   ((RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb))->rdi_tiddesc);

	if (usr_infoblk->rdr_rel->tbl_status_mask & DMT_VIEW)
	    status = rdu_malloc(global, (i4)(sizeof(RDD_ATTHSH) +
		(sizeof(RDD_BUCKET_ATT) * usr_infoblk->rdr_no_attr)), 
		(PTR *)&atthshp);
	else
	    status = rdu_malloc(global, (i4)(sizeof(RDD_ATTHSH) +
		(sizeof(RDD_BUCKET_ATT) * (usr_infoblk->rdr_no_attr + 1))), 
		(PTR *)&atthshp);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	/*  First clear the hash table. */
	MEfill(sizeof(RDD_ATTHSH), (u_char)0, (PTR)atthshp);

	atthshp_entry = (RDD_BUCKET_ATT *)((char *)atthshp + 
						sizeof(RDD_ATTHSH));
	/* Build the hash table. */
	for (attno = 0; 
             attno <= usr_infoblk->rdr_no_attr; attno++, attrpp++)
	{
	    RDD_BUCKET_ATT	    *next;
	    i4			    bucket;

	    if (attno == DB_IMTID)
	    {
		/* 
		** see if this is a view.  We do not want to make a tid for a
		** view, as the concept is not supported.  This is a fix for
		** bug 36478 (E_OP0889 consistency check) 
		*/
		if (usr_infoblk->rdr_rel->tbl_status_mask & DMT_VIEW)
		    continue;

		bucket = rdu_getbucket(&tidattp->att_name);
	    }
	    else
		bucket = rdu_getbucket(&(*attrpp)->att_name);

	    /* 
	    ** Always insert in the beginning of the bucket 
	    ** linked list.
	    */
	    next = atthshp->rdd_atthsh_tbl[bucket];
	    atthshp->rdd_atthsh_tbl[bucket] = atthshp_entry++;
						/* allocate these buckets in 
						** pieces so that there is no
						** requirement for large 
						** contiguous blocks of 
						** memory */
	    if (attno == DB_IMTID)
		atthshp->rdd_atthsh_tbl[bucket]->attr = tidattp;
	    else
		atthshp->rdd_atthsh_tbl[bucket]->attr = *attrpp;
	    atthshp->rdd_atthsh_tbl[bucket]->next_attr = next;
	}
	if (global->rdf_resources & RDF_RUPDATE)
	{   /* rdr_keys needs to be consistent prior to making it visible in the
	    ** system descriptor */
	    status = rdu_gsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    sys_infoblk->rdr_atthsh = atthshp;
	    sys_infoblk->rdr_types |= RDR_HASH_ATT;
	    status = rdu_rsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
    }
    if (sys_infoblk != usr_infoblk)
    {	/* user has a private info block which must also be updated */
	usr_infoblk->rdr_atthsh = atthshp;
	usr_infoblk->rdr_types |= RDR_HASH_ATT;
    }
    return(E_DB_OK);
}

/*{
** Name: rdu_bldindx	- build the index descriptor
**
** Description:
**      build the secondary index descriptor for the base relation
**
** Inputs:
**      global                          ptr to RDF state variable
**	build				may be RDF_NOTFOUND, RDF_ALREADY_EXISTS
**					 or RDF_BUILT_FROM_CATALOGS
** Outputs:
**	build				may not be modified.  If modified, will
**					be set to either:
**					RDF_ALREADY_EXISTS if cache object
**					  already exists and does not need to
**					  be built.
**					RDF_BUILT_FROM_CATALOGS if cache obj
**					  is successfully created
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-oct-87 (seputis)
**          initial creation
**      6-nov-88 (seputis)
**          refine RD0116 consistency check
**      19-oct-88 (seputis)
**          fixed race condition
**	28-feb-90 (teg)
**	    Fixed high concurency bug where RDF was checking sys_infoblk's
**	    rdr_rel for index count.  You cannot assume that the rdr_rel has
**	    been filled in at this time, so check the usr_infoblk->rdr_rel
**	    instead.
**	08-mar-90 (teg)
**	    added recovery if dmt_show fails because of
**	    E_DM013A_INDEX_COUNT_MISMATCH.  This means that the relation
**	    descriptor is probably outdated.  Pretend that there is no index
**	    information.  QEF should detect a timestamp mismatch and invalidate
**	    whatever query plan OPF comes up with.
**	31-jan-91 (teresa)
**	    improve statistics reporting
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-94 (teresa)
**	    Fix bug 59336 -- need to recalculate checksum each time we
**	    update the infoblk.
**	22-Nov-1999 (jenjo02)
**	    Handle disparity in the number of indexes we expect and the
**	    number created by DMT_SHOW, expecially the zero index case!
**	15-Mar-2004 (schka24)
**	    Semaphore protect sysinfo block state changes.
[@history_template@]...
*/
static DB_STATUS
rdu_bldindx(	RDF_GLOBAL	    *global,
		RDF_BUILD	    *build,
		bool		    *refreshed)
{
    DB_STATUS	    status;
    DMT_IDX_ENTRY   **indx;	    /* ptr to DMT_IDX_ENTRY ptr array */
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    i4		    num_indexes;

    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    usr_infoblk = global->rdfcb->rdf_info_blk;
    
    if ( (num_indexes = usr_infoblk->rdr_no_index) <= 0 )
    {
	if (usr_infoblk->rdr_rel->tbl_status_mask & DMT_IDXD)
	{	/* check for no indexes */
	    status = E_DB_ERROR;
	    rdu_ierror( global, status, E_RD0116_0INDEX);
	}
	else
	    status = E_DB_OK;
	return(status);
    }
    /* Use the information in the cache as long as it is available.  However,
    ** there are three cases we can not use the info in the cache.  First, if
    ** global->rdf_resources indicates RDF_REFRESH, meaning, RDR_REFRESH is
    ** requested from PSF for register with refresh, we will bring back
    ** all necessary information for a private infoblock and mark sys_infoblk
    ** as invalid. Second, if RDF_BAD_CHECKSUM is set, meaning, cached object
    ** is trashed or nolonger valid, we need to bring back object info
    ** from database.  Third case is that RDR_CHECK_EXIST is set, means cached
    ** object will be partially filled and sys_infoblk will be marked as invalid
    ** as previous cases.  */
    if ( (sys_infoblk->rdr_indx)
	&&
        !(global->rdfcb->rdf_rb.rdr_types_mask & (RDR_CHECK_EXIST | RDR_REFRESH))
	&&
        !(global->rdf_resources & RDF_BAD_CHECKSUM)
       )
    {	/* system cache has hash array this must be a case of a
        ** private relation descriptor being built, make a consistency
	** check that this is the case */
	if ((sys_infoblk == usr_infoblk)
	    ||
	    global->rdfcb->rdf_info_blk->rdr_indx)
	{
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD0114_HASH);
	    return(status);
	}
	indx = sys_infoblk->rdr_indx;
	if (*build != RDF_BUILT_FROM_CATALOGS)
	    *build = RDF_ALREADY_EXISTS;
    }
    else
    {
	status = rdu_malloc(global, 
	    (i4) ((num_indexes * sizeof(PTR)) +
		   (sizeof(DMT_IDX_ENTRY) * num_indexes)) ,
	    (PTR *)&indx);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	{
	    i4		    idx_cnt;
    	    DMT_IDX_ENTRY   *indx_entry;   /* ptr to DMT_IDX_ENTRY ptr array */
	
	    indx_entry = (DMT_IDX_ENTRY *)((char *)indx + 
				(num_indexes * sizeof(PTR)));
	    for (idx_cnt = 0; idx_cnt < num_indexes; idx_cnt++)
	    {
		indx[idx_cnt] = &indx_entry[idx_cnt];
	    }
	}

	if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	{   
	    /* This is a distributed thread */
	    global->rdf_ddrequests.rdd_types_mask = RDD_INDEX;
	    global->rdf_ddrequests.rdd_indx = indx;

    	    status = rdd_getobjinfo(global);   /* Inquire index information
					       ** from CDB */	    
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    else
		*build =  RDF_BUILT_FROM_CATALOGS;
	}
	else
	{
	    DMT_SHW_CB	*dmt_shw_cb;
	    dmt_shw_cb = &global->rdf_dmtshwcb;
	    dmt_shw_cb->dmt_index_array.ptr_address = (PTR)(indx);
	    dmt_shw_cb->dmt_index_array.ptr_in_count = num_indexes;
	    dmt_shw_cb->dmt_index_array.ptr_out_count = 0;
	    dmt_shw_cb->dmt_index_array.ptr_size = sizeof(DMT_IDX_ENTRY);

	    status = rdu_shwdmfcall(global, (i4)DMT_M_INDEX,
				(RDF_SYVAR *) NULL, (DB_TAB_ID *) NULL,
				refreshed);

	    if ( DB_FAILURE_MACRO(status) &&
		 dmt_shw_cb->error.err_code != E_DM013A_INDEX_COUNT_MISMATCH )
	    {
		return(status);
	    }

	    /*
	    ** If DM013A returned, there are now more indexes defined
	    ** on the table than we knew about when the DMT_SHOW was
	    ** done on behalf of rdu_relbld(), and DMT_SHOW has created
	    ** no entries.
	    **
	    ** Note that may also now be fewer, or zero, indexes on
	    ** the table. 
	    **
	    ** We'll handle any mismatch by accepting the number that
	    ** DMT_SHOW has constructed, and update our counts with
	    ** the real value to match the number of DMT_IDX_ENTRY
	    ** structures constructed.
	    */

	    /* Return the actual number of indexes built by DMT_SHOW */
	    num_indexes = dmt_shw_cb->dmt_index_array.ptr_out_count;

	    *build =  RDF_BUILT_FROM_CATALOGS;
	}

	if ( global->rdf_resources & RDF_RUPDATE || sys_infoblk == usr_infoblk )
	{   /* rdr_indx needs to be consistent prior to making it visible in the
	    ** system descriptor */
	    status = rdu_gsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    
	    /* Make tbl_index_count AND rdr_no_index consistent with reality */
	    if ( (sys_infoblk->rdr_rel->tbl_index_count = sys_infoblk->rdr_no_index
			= num_indexes) > 0 )
	    {
		sys_infoblk->rdr_indx = indx;
		sys_infoblk->rdr_types |= RDR_INDEXES;
	    }
	    else
	    {
		sys_infoblk->rdr_indx = (DMT_IDX_ENTRY **)NULL;
		sys_infoblk->rdr_types &= ~(RDR_INDEXES);
		sys_infoblk->rdr_rel->tbl_status_mask &= ~(DMT_IDXD);
	    }
	    status = rdu_rsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}

    }

    if (sys_infoblk != usr_infoblk)
    {	/* user has a private info block which must also be updated */

	/* Make tbl_index_count AND rdr_no_index consistent with reality */
	if ( (usr_infoblk->rdr_rel->tbl_index_count = usr_infoblk->rdr_no_index
		    = num_indexes) > 0 )
	{
	    usr_infoblk->rdr_indx = indx;
	    usr_infoblk->rdr_types |= RDR_INDEXES;
	}
	else
	{
	    usr_infoblk->rdr_indx = (DMT_IDX_ENTRY **)NULL;
	    usr_infoblk->rdr_types &= ~(RDR_INDEXES);
	    usr_infoblk->rdr_rel->tbl_status_mask &= ~(DMT_IDXD);
	}
    }
    return(E_DB_OK);
}

/*{
** Name: rdf_gdesc - Request description of a table.
**
**	External call format:	status = rdf_call(RDF_GETDESC, &rdf_cb);
**
** Description:
**      This function requests relation, attribute and index information of
**	a table. Any combination of the above information can be requested
**	by setting the appropriate bits in the rdr_types_mask. For example,
**
**	Request relation, attributes and indexes information by table id:
**
**	rdr_types_mask = RDR_RELATION | RDR_ATTRIBUTES | RDR_INDEXES;
**
**	Request relation, attributes and indexes information by table name 
**	This call incurs very high overhead and must be used only when a call
**	by id is not feasible.
**
**	rdr_types_mask = RDR_RELATION | RDR_ATTRIBUTES | RDR_INDEXES | 
**			    RDR_BY_NAME;
**
**	Request attributes information by table id: and build a hash table 
**	of attributes:
**
**	rdr_types_mask = RDR_ATTRIBUTES;
**
**	In addition to RDR_ATTRIBUTES one can also ask for a couple of
**	additional items:  for a hash table of attribute names, include
**
**	rdr_types_mask = RDR_ATTRIBUTES | RDR_HASH_ATT;
**	
**	For physical storage info, including the base table structure
**	keys and partitioning info if the table is partitioned:
**
**	rdr_types_mask = RDR_ATTRIBUTES | RDR_BLD_PHYS;
**
**	For defaults in addition to the attribute stuff:
**
**	rdr_types_mask = RDR_ATTRIBUTES
**	rdr_2types_mask= RDR2_DEFAULT
**
**	Request indexes information only by table id :
**
**	rdr_types_mask = RDR_INDEXES;
**
**	    NOTE: RDF will assume that RDR_INDEXES is requested if RDR_RELATION
**		  is requested.  This is because the iirelation tuple contains
**		  the number of secondary indexes.  That number could change 
**		  without invalidating the relation object in the cache (if
**		  someone does a create index or modify command).  So, RDF will
**		  always pick up the index information that is in synch with the
**		  iirelation information.
**
**	Asking for physical partition information by table ID is invalid.
**	All partition info is carried by the master table.
**
**	Requesting information by table id is the preferred way.  RDF
**	maintains internal cache using table id and database id as the
**	hash key.  Information requested by name will be directly passed
**	over to DMF even if the information may have been in the cache
**	already.
**
**	If RDF successfully acquires the information for the client, the
**	information block in the cache is locked in shared mode.  The
**	client must issue the rdf_unfix() function to release the lock
**	and system resources associated with the block.
**
**	RDF does not validate information in its cache against DMF copies.
**	It is the client's responsibility to validate the information and
**	issue the rdf_invalidate() to remove an invalid information from
**	RDF cache.
**
** Inputs:
**
**      rdf_cb                          
**		.rdf_rb
**			.rdr_db_id	Database id.
**			.rdr_session_id	Session id.
**			.rdr_tabid	Table id.
**			.rdr_name.rdr_tabname Table name.
**			.rdr_owner	Table owner.
**			.rdr_types_mask	Type of information requested.
**					The masks can be:
**					RDR_RELATION
**					RDR_ATTRIBUTES
**					RDR_INDEXES
**					RDR_BY_NAME
**					RDR_HASH_ATT
**					RDR_BLD_PHYS
**			.rdr_2types_mask Type of information requested.
**					The masks can be:
**					RDR2_DEFAULT
**			.rdr_fcb	A pointer to the structure which 
**					contains the memory allocation and
**					caching control information. All
**					the table information will be allocated 
**					in the memory pool specified by the
**					poolid.
**					
**		.rdf_info_blk		Pointer to the requested table 
**					information block. NULL if the requested
**					table information block doesn't exist
**					yet. This is for storing all the
**					information related to one table 
**					together.
**
** Outputs:
**      rdf_cb                          
**		.rdf_rb
**			.rdr_tabid	Table id if request table information
**					by RDR_BY_NAME.
**
**		.rdf_info_blk		Pointer to the requested table 
**					information block. The pointer field is 
**					NULL if the corresponding information is
**					not requested.
**			.rdr_rel	Pointer to table entry.
**			.rdr_attr	Pointer to the array of pointers to
**					attribute entry.
**			.rdr_indx	Pointer to array of pointers to index
**					entry.
**			.rdr_keys	Pointer to the primary key of a table
**					if RDR_BLD_PHYS is requested.
**			.rdr_no_attr	Number of attributes in the table.
**			.rdr_no_index	Number of indexes on the table.
**			.rdr_atthsh 	Pointer to the hash table of attribute
**					information if RDR_HASH_ATT is 
**					requested.
**	
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0001_NO_MORE_MEM
**					Out of memory.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of 
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of 
**					memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned 
**					above.
**				E_RD0002_UNKNOWN_TBL
**					Table doesn't exist.
**				E_RD000C_USER_INTR
**					User interrupt.
**				E_RD000D_USER_ABORT
**					User abort.
**				E_RD000E_DMF_ERROR
**					Whatever error returned from DMF not
**					mentioned above.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
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
** Side Effects:
**		none
** History:
**	28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version
**	11-may-87 (puree)
**	    add field rdr_unique_dbid for cache id.
**	21-sep-87 (puree)
**	    modify ULH interface for new object handling protocol.
**      19-oct-87 (seputis)
**          added resource handling code, fixed bugs, added readonly
**	27-oct-89 (teg)
**	    changed logic to get index info when getting relation info, if any
**	    index info exists and we do not already have it.  This is part of
**	    the fix to bug 5446.  Also added logic to handle RDI_INFO traces
**	    (RD0001 through RD0012)
**	15-dec-89 (teg)
**	    add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**	    routines to use.
**	28-feb-90 (teg)
**	    add a few consistency checks to assure we dont try to get
**	    information in the wrong order.
**	31-jan-91 (teresa)
**	    improve statistics reporting
**	12-may-92 (teresa)
**	    Sybil:  set RDR_ATTRIBUTES if distributed and this is a query that
**	    requires schema checking (i.e., RDR_CHECK_EXIST and RDR_USERTAB are 
**	    not set).
**	22-may-92 (teresa)
**	    Sybil: changed interface to rdu_xlock.  Set new parameter to true to
**	    indicate the object is being fixed by the first time by this thread.
**	    Also initialized rdf_starptr.
**	16-jul-92 (teresa)
**	    prototypes
**	01-dec-92 (teresa)
**	    added support for REGPROCs.
**	25-feb-93 (teresa)
**	    add support for defaults for FIPS constraints.
**	12-mar-93 (teresa)
**	    move rdf_shared_sessions increment logic from rdu_xlock call to
**	    rdu_gdesc()
**	16-feb-94 (teresa)
**	    Calculate checksum whether or not trace point to validate
**	    checksum is set.
**	19-feb-1997 (cohmi01)
**	    Call rdu_xlock() before changing the sys_infoblk for DDB info
**	    to ensure proper concurrent behaviour & that it gets re-checksumed.
**	20-jun-1997 (nanpr01)
**	    Fix star segv problem on createdb.
**	22-jul-1998 (shust01)
**	    Initialized rdf_ulhobject->rdf_sysinfoblk to NULL after rdf_ulhobject
**	    is allocated in case allocation of rdf_sysinfoblk fails (due to lack
**	    of RDF memory, for example).  That way, we won't try  to use/free up
**	    rdf_sysinfoblk later, causing a SEGV. Bug 92059. 
**	07-sep-1998 (nanpr01)
**	    Shared session counter was not getting incremented for the else
**	    part of the code where it is possible that we did not see the 
**	    object, then get the semaphore and then found the object on 
**	    recheck.
**	07-nov-1998 (nanpr01)
**	    Initialize the rdf_ulhobject fields. rdu_malloc allocated memory
**	    by calling ulm_palloc which is not guaranteed to be initialized.
**	    Caller is supposed to do initialization. Prior to this change, 
**	    these fields were not used by mistake and caused segvs.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**      20-aug-2002 (huazh01)
**          Semaphore protection on rdf_ulhobject->rdf_shared_sessions.
**          This fixes bug 107625. INGSRV 1551.
**	30-Dec-2004 (schka24)
**	    Build partition info if asking for RDR_BLD_PHYS.
**	8-Aug-2005 (schka24)
**	    Revise fix for 107625 to use thread-safe increment.
**	6-Oct-2005 (schka24)
**	    Above change missed one place, fix.
**	6-jan-06 (dougi)
**	    Add support for RDR2_COLCOMPARE for loading column compoarison
**	    statistics.
**	10-Sep-2008 (jonj)
**	    Use CLRDBERR, SETDBERR to value RDF_CCB's rdf_error.
**	19-oct-2009 (wanfr01) Bug 122755
**	    rdf_session_count needs to be incremented if rdf_release  is going
**	    to decrement it.  rdf_shared_sessions must be modified under
**	    rdu_gsemaphore protection to prevent concurrent destruction.
*/
DB_STATUS
rdf_gdesc(  RDF_GLOBAL	*global,
	    RDF_CB	*rdfcb)
{	
	bool relation_only;		/* TRUE if only table info wanted */
	RDF_TYPES	requests;
	RDF_TYPES	req2;
	RDF_TYPES	mask2;
	DB_STATUS	status;
	RDF_TYPES	build=RDF_NOTFOUND;
	bool		refreshed=FALSE;
	RDF_ULHOBJECT	*rdf_ulhobject;	/* root of table descriptor object 
					** stored in ULH */
        bool session_cnt_needed=FALSE;

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */
    global->rdfcb = rdfcb;
    CLRDBERR(&rdfcb->rdf_error);
    status = E_DB_OK;

    /* Check input parameter. */
    {
	RDF_TYPES	types1;
	RDF_TYPES	types2;

	types1 = rdfcb->rdf_rb.rdr_types_mask;
	types2 = rdfcb->rdf_rb.rdr_2types_mask;
	if (((RDI_FCB *)(rdfcb->rdf_rb.rdr_fcb)) == NULL ||
	    rdfcb->rdf_rb.rdr_db_id == NULL ||
	    rdfcb->rdf_rb.rdr_unique_dbid == 0 ||
	    rdfcb->rdf_rb.rdr_session_id == 0 ||
	    rdfcb->rdf_rb.rdr_fcb == NULL ||
	    (((types1 & (RDR_RELATION | RDR_ATTRIBUTES | RDR_INDEXES)) == 0L) &&
	      ((types2 & RDR2_DEFAULT)==0)
	    )
	    ||
	    (	(types1 & (RDR_HASH_ATT | RDR_BLD_PHYS)) 
		&& 
		((types1 & RDR_ATTRIBUTES) == 0L)
	    )
	    ||
	    (	(types2 & RDR2_DEFAULT) 
		&& 
		((types1 & RDR_ATTRIBUTES) == 0L)
	    )
	    ||
	    (	(types1 & RDR_BY_NAME) == 0
		&& (types1 & (RDR_RELATION | RDR_INDEXES)) != 0
		&& rdfcb->rdf_rb.rdr_tabid.db_tab_index < 0
	    )
	   )
	{
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	    return(status);
	}
    }

    /* force a cache flush of object on register with refresh command to
    ** assure that rdd_alterdate and rdd_schemachk logic is executed.
    */
    if (rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
	rdfcb->rdf_rb.rdr_2types_mask |= RDR2_REFRESH;


    /* increment request statistics before we start looking for
    ** anything.  This way, we can see how many disk accesses
    ** we make where we do not find what we want */
    requests = rdfcb->rdf_rb.rdr_types_mask 
	&
	(   RDR_RELATION | RDR_ATTRIBUTES | RDR_INDEXES | 
	    RDR_HASH_ATT | RDR_BLD_PHYS
	);
    relation_only = (rdfcb->rdf_rb.rdr_types_mask
		& (RDR_ATTRIBUTES | RDR_INDEXES | RDR_HASH_ATT | RDR_BLD_PHYS)
	) == 0;
    req2 = rdfcb->rdf_rb.rdr_2types_mask & (RDR2_DEFAULT | RDR2_COLCOMPARE);

    if (! relation_only)
	Rdi_svcb->rdvstat.rds_r1_core++;
    else if (requests & RDR_RELATION)
	Rdi_svcb->rdvstat.rds_r0_exists++;

    /*
    ** If the user does not have his copy of the info block, try to
    ** get the system info block.  The info block may have to be 
    ** created if it does not exist. If the user already has his copy
    ** of the info block, we need to check if it has everything he 
    ** wants.
    */

    /* get access to the ULH object */
    if (rdfcb->rdf_info_blk != (RDR_INFO *) NULL)
    {   /* object is already available in the info block */
	/* This will be the private one if the info block is private */
	rdf_ulhobject = (RDF_ULHOBJECT *)(rdfcb->rdf_info_blk->rdr_object);
	/* note: we intentionally ignore the RDR2_REFRESH request if the
	**	 caller already has the infoblk, so no special checks for
	**	 refreshing a fixed infoblk as performed here.  RDR2_REFRESH
	**	 refers only to using infoblks that already exist on the cache
	**	 when the user first tries to access them.
	*/
    }
    else
    {
	status = rdu_cache(global,&build,&refreshed); /* now that the table ID 
				** is available, either create or get access
				** to the respective ULH object */
	if (DB_FAILURE_MACRO(status))
 	{
	    RDF_ULHOBJECT       *ulhobj = 0; 
	    DB_STATUS	local_status;

	    /* Bug 122755: Error occurred in rdu_cache - this will cause rdf_release to 
	    ** decrease rdf_shared_sessions, so it needs to be increased here.
	    */
	    if (global->rdf_resources & RDF_RULH)
	    {		
		if (global->rdfcb->rdf_info_blk)
		    ulhobj =
			(RDF_ULHOBJECT *)global->rdfcb->rdf_info_blk->rdr_object;
		else if (global->rdf_ulhcb.ulh_object)
		{
		    ulhobj = (RDF_ULHOBJECT *)
			global->rdf_ulhcb.ulh_object->ulh_uptr;
		}

		if (ulhobj)
		{
		    local_status = rdu_gsemaphore(global); 
		    if (DB_FAILURE_MACRO(local_status))
			return(local_status);

		    CSadjust_counter(&ulhobj->rdf_shared_sessions, 1);
		    local_status = rdu_rsemaphore(global);
		    if (DB_FAILURE_MACRO(local_status))
			return(local_status);
		}
	    }
	    return(status);
	}
	rdf_ulhobject = (RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->
		ulh_uptr;
    }

    /*  We can be here for two cases:
    **	    - we have successfully created an empty 
    **	      ULH object, or
    **	    - we have successfully gained access to an object.
    */

    if (!rdf_ulhobject)		
    {	/* object newly created or it has no info block */
	status = rdu_gsemaphore(global); /* get a semaphore on the
					 ** ulh object in order to update
                                	 ** the object status 
					 */
	if (DB_FAILURE_MACRO(status))
	    return(status);
	rdf_ulhobject = (RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->
	    ulh_uptr;		/* re-fetch the ULH object under semaphore
				** protection in order to check if another
				** thread is attempting to create this object
				*/
	global->rdf_ulhobject = rdf_ulhobject; /* save ptr
				** to the resource which is to be updated */
	if (rdf_ulhobject == NULL)
	{   
	    /* 
	    ** another thread is not attempting to create this object
	    ** so allocate some memory for the object descriptor 
	    */
	    global->rdf_resources |= RDF_RUPDATE; /* ULH object has been
				** successfully inited, to be updated by
				** this thread */
	    status = rdu_malloc(global, (i4)sizeof(*rdf_ulhobject), 
		(PTR *)&rdf_ulhobject);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    rdf_ulhobject->rdf_ulhptr = global->rdf_ulhcb.ulh_object; /* save
				** ptr to ULH object for unfix commands */
	    rdf_ulhobject->rdf_state = RDF_SUPDATE; /* indicate that this
				** object is being updated */
	    rdf_ulhobject->rdf_shared_sessions = 0; /* init to one session
						    ** at the beginning */
	    /* Initialize other fields */
	    rdf_ulhobject->rdf_sintegrity = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_sprotection = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_ssecalarm = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_srule = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_skeys = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_sprocedure_parameter = RDF_SNOTINIT;
	    rdf_ulhobject->rdf_procedure = NULL;
	    rdf_ulhobject->rdf_defaultptr = NULL;

	    global->rdf_ulhobject = rdf_ulhobject; /* save ptr
				** to the resource which is to be updated */
	    /* initialize rdf_sysinfoblk in case next rdu_malloc fails */
	    rdf_ulhobject->rdf_sysinfoblk = NULL;
	    status = rdu_malloc(global, 
		(i4)sizeof(*rdf_ulhobject->rdf_sysinfoblk), 
		(PTR *)&rdf_ulhobject->rdf_sysinfoblk);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    rdu_init_info(global, rdf_ulhobject->rdf_sysinfoblk,
		global->rdf_ulhcb.ulh_object->ulh_streamid);
            /* If distributed, allocate memory for object descriptor. */
            if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
	    {
		status = rdu_malloc(global, 
		    (i4)sizeof(*rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc), 
		    (PTR *)&rdf_ulhobject->rdf_sysinfoblk->rdr_obj_desc);
		if (DB_FAILURE_MACRO(status))
		    return(status);
            }
	    rdf_ulhobject->rdf_sysinfoblk->rdr_object = (PTR)rdf_ulhobject; /*
				** have this object point back to itself
				** initially, so it can be found if the user
                                ** calls RDF with this object again */
	    global->rdf_ulhcb.ulh_object->ulh_uptr = (PTR)rdf_ulhobject; 
				/* save this object after marking 
				** its' status as being updated */
	    /* initialize the ldbdesc ptr -- it may already exist if the
	    ** refresh path has been taken */
	    rdf_ulhobject->rdf_starptr = global->rdf_d_ulhobject;
	}
        else
        {
            /* 
            ** Another thread has finished building the object while we
            ** were waiting for semaphore. Need to bump up session count
            **/
	    session_cnt_needed = TRUE;

        }
    }

     /* We fixed the object in rdu_cache(), so up the fix count */
    if ((rdfcb->rdf_info_blk == (RDR_INFO *) NULL) || (session_cnt_needed))
    {
	status = rdu_gsemaphore(global); 
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if ((rdfcb->rdf_info_blk == (RDR_INFO *) NULL) && (session_cnt_needed))
	    CSadjust_counter(&rdf_ulhobject->rdf_shared_sessions, 2);
	else
	    CSadjust_counter(&rdf_ulhobject->rdf_shared_sessions, 1);
	status = rdu_rsemaphore(global); 
	if (DB_FAILURE_MACRO(status))
	    return(status);

    }

    {	/* check if all requested components exist in the cache */
	RDR_INFO	*sys_infoblk;
	RDR_INFO	*usr_infoblk;

	sys_infoblk = rdf_ulhobject->rdf_sysinfoblk; /* this must be available
                                   ** since it was added under semaphore
                                   ** protection */
	if (sys_infoblk->rdr_invalid)
	{   /* consistency check to see if ULH has marked this object as
            ** deleted */
	    status = E_DB_ERROR;
	    rdu_ierror(global, status, E_RD000F_INFO_OUT_OF_DATE);
	    return(status);
	}

	if (!rdfcb->rdf_info_blk)
	    rdfcb->rdf_info_blk = sys_infoblk;	/* use the system info block
						** if the user info block is 
						** not defined yet */
	usr_infoblk = rdfcb->rdf_info_blk;

	/* control loop */
	do
	{
	    /* force attribute info for distributed queries that require schema
	    ** checking -- i.e., queries besides create/register and remove
	    ** (For Sybil, schema checking was moved from relation operations to
	    ** attribute operations so that iidd_columns only be read once.)
	    */
	    if ( (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS) &&
		 (rdfcb->rdf_rb.rdr_types_mask & RDR_RELATION) &&
		 (!(rdfcb->rdf_rb.rdr_types_mask&(RDR_CHECK_EXIST|RDR_USERTAB))
		  || (rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
	         )
		  && !(global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	       )
		requests |= RDR_ATTRIBUTES;

	    if ( ( (usr_infoblk->rdr_types & requests) == requests )
		 &&
		 ( (usr_infoblk->rdr_2_types & req2) == req2 )
	       )
	    {
		/* everything requested was already on the cache */

		if (usr_infoblk->rdr_types & RDR_DST_INFO)
		{
		    /* get the ldb descriptor if this is not a view */
		    if ((usr_infoblk->rdr_rel->tbl_status_mask & DMT_VIEW) == 0)
		    {
			/* 
			** We're a distributed thread, so get ptr to 
			** LDBdescriptor and place in sys_infoblk.
			** This operation requires that we get the semaphore
			** for concurrency & to ensure that we re-checksum.
			*/
	    		global->rdf_ulhobject = rdf_ulhobject; 
					/* save ptr to ulh object
					** descriptor to update */
			status = rdu_xlock(global, &usr_infoblk);
			if (DB_FAILURE_MACRO(status))
			    return(status);
			status = rdd_ldbsearch(global, 
			    &sys_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p,
			    global->rdfcb->rdf_rb.rdr_unique_dbid,
			    sys_infoblk->rdr_ldb_id);
			if (DB_FAILURE_MACRO(status))
			    break;
		    }
		    /* assure that LDBdesc cache ulh object ptr is not lost */
		    global->rdf_ulhobject = rdf_ulhobject;
		    global->rdf_ulhobject->rdf_starptr =global->rdf_d_ulhobject;
		}
		build = RDF_ALREADY_EXISTS;
		/* FIXME should do an abbreviated dmt-show to get just
		** the latest reltups/relpages or some reasonable estimate
		*/
		break;
	    }

	    global->rdf_ulhobject = rdf_ulhobject; /* save ptr to ulh object
					** descriptor to update */
	    status = rdu_xlock(global, &usr_infoblk); /* get an exclusive
					** lock to update the system descriptor
                                        */
	    if (DB_FAILURE_MACRO(status))
		return(status);

	    /* just in case!  if sys infoblk is invalidated at this moment,
            ** meaning, this is a RDR_REFRESH or checksum error case. Make
            ** sure we put enough info in the private info block */
	    if ((sys_infoblk->rdr_invalid)    /* sys infoblk got invalidated */
		&& 
		(sys_infoblk != usr_infoblk)  /* it is a private infoblk */
               )
	    {
		/* always retrieve relation info */
		requests |= RDR_RELATION;
		/* we should not do the following because sysinfo block may
		** be trashed already. Do it anyway just in case. */
		requests |= ((sys_infoblk->rdr_types)
			     &
			     (RDR_ATTRIBUTES |
			      RDR_BLD_PHYS   |
			      RDR_HASH_ATT   |
			      RDR_INDEXES
			     ));
	    }
    
	    /* If distributed, validate ldb descriptor. */
	    if ( (usr_infoblk->rdr_types & RDR_DST_INFO) &&
		 (usr_infoblk->rdr_obj_desc->dd_o6_objtype != DD_3OBJ_VIEW) &&
		 !(usr_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p) 
	       )
	    {
		/* This should only happen when the relation descriptor was
		** unfixed, or if we are creating a private infoblk in
		** concurrent conditions 
		*/

		if (global->rdf_tobj.dd_o9_tab_info.dd_t9_ldb_p &&
                    global->rdf_tobj.dd_o9_tab_info.dd_t8_cols_pp)
		{
		    /* desired object is already in memory from rdu_cache call 
		    ** Use it.  Also, set up the pointer to the LDBdesc cache
		    ** object for subsequent calls.
		    */
		    STRUCT_ASSIGN_MACRO(global->rdf_tobj, 
					*usr_infoblk->rdr_obj_desc);
		    
		}
		else
		{
		    status = rdd_ldbsearch(global, 
			&sys_infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p,
			global->rdfcb->rdf_rb.rdr_unique_dbid,
			sys_infoblk->rdr_ldb_id);
		    if (DB_FAILURE_MACRO(status))
			break;
		}
	    }

	    if ((requests & RDR_RELATION) && relation_only &&
		(usr_infoblk->rdr_types & RDR_RELATION)
	       )
	    {
		/* item is already on the cache, and relation is all we
		** are looking for */
		if (build == RDF_NOTFOUND)
		    build = RDF_ALREADY_EXISTS;
	    }
	    else if ((requests & RDR_RELATION) 
		     && 
		     !(usr_infoblk->rdr_types & RDR_RELATION)
		    )
	    {
		/* we need to get the iirelation info and put on cache */
		status = rdu_relbld(global, &build, &refreshed);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    else
	    {
		/* we need RDR_RELATION info for any subsequent requests.  If
		** we do not have this, report an error */
		if ( !(usr_infoblk->rdr_types & RDR_RELATION) )
		{
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD013F_MISSING_RELINFO);
		    return(status);
		}
	    }
	    /* assure that LDBdesc cache ulh object ptr is not lost */
	    if ((global->rdf_ulhobject->rdf_starptr==NULL) && 
		(global->rdf_d_ulhobject)
	       )
		global->rdf_ulhobject->rdf_starptr =global->rdf_d_ulhobject;
	    if ((requests & RDR_ATTRIBUTES) 
		&&
		!(usr_infoblk->rdr_types & RDR_ATTRIBUTES))
	    {
		status = rdu_bldattr(global, &build, &refreshed);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    else
		build = (build == RDF_NOTFOUND) ? RDF_ALREADY_EXISTS : build;

	    if ((requests & RDR_BLD_PHYS)
		&&
		!(usr_infoblk->rdr_types & RDR_BLD_PHYS))
	    {
		/* we need RDR_ATTRIBUTES info for this request.  If
		** we do not have this, report an error */
		if ( !(usr_infoblk->rdr_types & RDR_ATTRIBUTES) )
		{
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0140_BAD_RELKEY_REQ);
		    return(status);		    
		}
		status = rdu_key_ary_bld(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = rdp_build_partdef(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    if ((requests & RDR_HASH_ATT)
		&&
		!(usr_infoblk->rdr_types & RDR_HASH_ATT))
	    {
		/* we need RDR_ATTRIBUTES info for this request.  If
		** we do not have this, report an error */
		if ( !(usr_infoblk->rdr_types & RDR_ATTRIBUTES) )
		{
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0141_BAD_ATTHASH_REQ);
		    return(status);		    
		}
		status = rdu_hash_att_bld(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    if ((req2 & RDR2_DEFAULT)
		&&
		!(usr_infoblk->rdr_2_types & RDR2_DEFAULT))
	    {
		/* we need RDR_ATTRIBUTES info for this request.  If
		** we do not have this, report an error */
		if ( !(usr_infoblk->rdr_types & RDR_ATTRIBUTES) )
		{
		    status = E_DB_ERROR;
		    rdu_ierror(global, status, E_RD0140_BAD_RELKEY_REQ);
		    return(status);		    
                }
		status = rdu_default_bld(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    if ((req2 & RDR2_COLCOMPARE)
/* && FALSE */
		&&
		!(usr_infoblk->rdr_2_types & RDR2_COLCOMPARE))
	    {
		/* Not much to do - just call func to load column
		** comparison statistics. */
		status = rdu_bld_colcompare(global);
		if (DB_FAILURE_MACRO(status))
		    return(status);
		status = E_DB_OK;
	    }

	    /* a side effect of bug 5446 is that certain types of operations
	    ** modify the number of secondary indices.  These operations
	    ** (create index, modify table, etc) go throught parser (who does
	    ** not care about 2ndary indices) but not through OPF (who does
	    ** care about secondary indices).  Therefore, the iirelation desc
	    ** may be out of date because it has old information about the number
	    ** of secondary indices, but OPF cannot determine this.  That would
	    ** result in an error returned from DMT_SHOW.  So, whenever we get
	    ** relation information, get and cache the index infomation that
	    ** goes along with the iirelation information.  If this info is 
	    ** out of date, QEF will detect a timestamp descrepancy, and will
	    ** invalidate the query plan.  Then we will get the new iirelation
	    ** tuple and new secondary index info.
	    ** 
	    **	But if its a REGPROC, then do not get index info, since there
	    **  will not be any.
	    */
	    if ( (requests & ( RDR_RELATION | RDR_INDEXES))
		&& 
		!(usr_infoblk->rdr_types & RDR_INDEXES)
		&&
		!(global->rdfcb->rdf_rb.rdr_instr & RDF_REGPROC)
	       )
	    {
		status = rdu_bldindx(global, &build, &refreshed);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }
	    else
		build = (build == RDF_NOTFOUND) ? RDF_ALREADY_EXISTS : build;
	    
	} while (0);   /* end control loop */


	/* now do statistic reporting if we found or built cache entry */
	if (build != RDF_NOTFOUND)
	{
	    if (! relation_only)
	    {
		/* update statistics on obtaining core catalog info */
		if (build == RDF_ALREADY_EXISTS)
		    Rdi_svcb->rdvstat.rds_c1_core++;
		else
		    Rdi_svcb->rdvstat.rds_b1_core++;
	    }
	    else
	    {
		/* update statistics on existance checking */
		if (build == RDF_ALREADY_EXISTS)
		    Rdi_svcb->rdvstat.rds_c0_exists++;
		else
		    Rdi_svcb->rdvstat.rds_b0_exists++;
		
	    }
	    /* update statistics on cache refresh */
	    if (refreshed)
		Rdi_svcb->rdvstat.rds_x1_relation++;
	}

	/* handle any RDI_INFO trace requests. */
	rdu_master_infodump(usr_infoblk, global, RDFGETDESC, 0);
    }
    return(status);
}

/*{
** Name: rdu_default_bld	- Add Defaults to attributes descriptor
**
** Description:
**      This routine obtains a defaults descriptor for each attribute in the
**	relation and attaches them to the attributes descriptor.  It 
**	accomplishes this by calling routine rdd_defsearch().
**
**      FIX_ME:
**	There is a concurrancy/sempahore problem here.  Techincally RDF needs to
**	release the semaphore each time that it calls rdd_defsearch(), since
**	rdd_defsearch() may call QEF/DMF to read iidefault tuples.  However,
**	if we release the semaphore, we could get into race conditions where 
**	more than one thread fixes the same DEFAULTS cache object but the object
**	only appears one time in the DMF_ATTR_ENTRY and is therefore only 
**	unfixed one time.  The solution is probably to have rdu_default_bld()
**	release the semaphore, call rdd_defsearch(), retake the semaphore via
**	an XLOCK call (so a private copy is made if another thread has taken
**	update of the sys_infoblk), then update the usr_infoblk.  This could
**	drastically increase the chance of one thread starting with update of
**	the sys_infoblk and ending up finishing on the update of a private
**	infoblk.  The holding a semaphore across QEF/DMF calls needs to be 
**	fixed, but I want to make sure that we understand concurrancy issues
**	so we don't generate any concurrancy bugs (which are very tough to 
**	debug).
**
**
** Inputs:
**      global                          ptr to RDF state variable
**	    .rdfcb			RDF control block
**		.rdf_info_blk		Infoblk for this cache object
**		    .
** Outputs:
**	global
**	    
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-feb-93 (teresa)
**          initial creation
**      21-feb-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          Do not set sys_infoblk->rdr_2_types |= RDR2_DEFAULT when 
** 	    attrs for private and system info blocks are different
[@history_template@]...
*/
static DB_STATUS
rdu_default_bld(RDF_GLOBAL         *global )
{
    DMT_ATT_ENTRY   **attr;		/* ptr to DMT_ATT_ENTRY ptr array */
    DB_STATUS	    status;
    RDR_INFO	    *sys_infoblk;
    RDR_INFO	    *usr_infoblk;
    i4		    att_cnt;

    usr_infoblk = global->rdfcb->rdf_info_blk;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;

    /*
    ** if the attribute descriptor is not available or if it has no attributes,
    ** raise an error
    */
    if (!usr_infoblk->rdr_attr)
    {
            status = E_DB_SEVERE;
            rdu_ierror(global, status, E_RD0112_ATTRIBUTE);
            return(status);
    }
    else if (usr_infoblk->rdr_no_attr == 0)
    {

        status = E_DB_ERROR;
        rdu_ierror(global, status, E_RD013B_BAD_ATTR_COUNT);
        return(status);
    }

    attr = usr_infoblk->rdr_attr;
    for (att_cnt = 1; att_cnt <= usr_infoblk->rdr_no_attr; att_cnt++)
    {
        RDD_DEFAULT  *def_ptr; /*ptr to cache object returned by rdd_defsearch*/

        if (attr[att_cnt]->att_default == NULL)
        {
            /* Some other thread has not beaten us to updating the default, so
            ** update it
            */
            status=rdd_defsearch( global, attr[att_cnt], &def_ptr);
            if (status != E_DB_OK)
                return(status);
            else
            {
                /* indicate that we are building defaults info.  The rdf_release
                ** routine needs to know this so it knows to unfix any fixed
                ** attribute default
                */
                global->rdf_resources |= RDF_ATTDEF_BLD;
                attr[att_cnt]->att_default = (PTR) def_ptr;
            }
        }
    }

    if (sys_infoblk != global->rdfcb->rdf_info_blk)
    {
        usr_infoblk->rdr_2_types |= RDR2_DEFAULT;
        if (global->rdf_ulhobject->rdf_sysinfoblk->rdr_attr == 
	    usr_infoblk->rdr_attr)
            if (global->rdf_resources & RDF_RUPDATE)
                sys_infoblk->rdr_2_types |= RDR2_DEFAULT;
    }
    else /* infoblks same so attr's same */
        if (global->rdf_resources & RDF_RUPDATE) /* necessary? */
                sys_infoblk->rdr_2_types |= RDR2_DEFAULT;
    return(E_DB_OK);
}

