/*
** Copyright (c) 2003, 2008 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<bt.h>
#include	<cm.h>
#include	<st.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<er.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<cs.h>
#include	<pc.h>
#include	<lk.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<adfops.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rqf.h>
#include	<rdf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<tr.h>
#include	<rdftrace.h>

/*
** Name: RDFPART.c - RDF functions for partitioned table catalogs
**
** Description:
**	This file contains RDF utilities that process partitioned table
**	catalogs.  The catalogs are stored in a normalized form, with
**	any constant values stored as uninterpreted text.  The in-memory
**	form is an DB_PART_DEF, which contains a header and one descriptor
**	per dimension.  Each descriptor points to partition names, and
**	defines the distribution scheme for that dimension.
**
**	All of this work happens when someone requests the physical
**	storage info (RDR_BLD_PHYS) for the master table (the one that
**	is partitioned).  The individual physical partitions are tables,
**	but RDF doesn't care about them -- the partitions are physical
**	objects, not (generally) logical or syntax objects.
**
**	The primary functions defined here are:
**	rdp_build_partdef -- build a partitioned table definition from the
**		system catalogs;
**	rdp_copy_partdef -- make an external copy of a partition definition;
**	rdp_finish_partdef -- "Finish" and validate a partition definition;
**	rdp_part_compat -- Check partition / storage structure compatibility;
**	rdp_update_partdef -- reverse-engineer a partition definition into
**		system catalog entries, and update the catalogs;
**
**	As a general note, these routines are rather careless about
**	cleaning up after themselves when an error occurs.  As long
**	as RDF utilities like rdu_qopen are used, the global RDF
**	request-state block tracks resources in use.  When the
**	rdf-call is finished, any in-use resources are closed off.
**
**	This file is organized with external routines first, then
**	local ones, with each section in alphabetic order.  Please
**	keep it this way so that it's easier to find things.
**
** History:
**	31-Dec-2003 (schka24)
**	    Initial burst of frantic typing.
**	2-Feb-2004 (schka24)
**	    Make finish-partdef externally callable; change memory allocator
**	    interface to partdef copier.
**	5-Aug-2004 (schka24)
**	    Export the struct-compat checker for proper unique checking.
**	25-Oct-2005 (hanje04)
**	    Add prototype for rdp_write_partname() to prevent compiler
**	    errors with GCC 4.0 due to conflict with implicit declaration.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror(). Use new form of uleFormat().
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      20-aug-2010 (stial01)
**          rdp_build_partdef() Fixed E_UL0005_NOMEM/rdu_mrecover err handling.
*/


/* Local prototype definitions */

/* Finish breaks tables after reading raw iidistcol data */
static DB_STATUS rdp_finish_breaktables(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def,
	char errbuf[]);

/* Issue an internal inconsistency errmsg */
static void rdp_inconsistent(
	RDF_GLOBAL *global,
	char *catalog,
	char *str);

/* Read the iidistcol partitioning columns catalog */
static DB_STATUS rdp_read_distcol(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def,
	char *qef_area,
	i4 qef_size,
	i4 no_cols,
	DMT_ATT_ENTRY **mast_att_list);

/* Read the iidistscheme partitioning dimensions catalog */
static DB_STATUS rdp_read_distscheme(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def,
	char *qef_area,
	i4 qef_size);

/* Read the iidistval partition break values catalog */
static DB_STATUS rdp_read_distval(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def,
	char *qef_area,
	i4 qef_size);

/* Read the iipartname partition names catalog */
static DB_STATUS rdp_read_partname(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def,
	char *qef_area,
	i4 qef_size);

/* Sort a dimension's breaks table */
static DB_STATUS rdp_sort_breaks(
	RDF_GLOBAL *global,
	DB_PART_DIM *dim_ptr,
	char errbuf[]);

/* Set partition flag if master table storage structure is direct-plan
** compatible with the partitioning scheme.
*/
static void rdp_struct_compat(
	RDF_GLOBAL *global,
	i4 storage_type,
	i4 nkeys,
	i4 *key_array,
	DB_PART_DEF *part_def);

/* Scan a (multicolumn) breaks value from text to internal form */
static DB_STATUS rdp_text_to_internal(
	RDF_GLOBAL *global,
	DB_PART_DIM *dim_ptr,
	DB_PART_BREAKS *break_ptr,
	char errbuf[]);

/* Compute size of dimension's break value */
static i4 rdp_val_size(DB_PART_DIM *dim_ptr);

/* Validate a dimension's breaks table after all setup */
static DB_STATUS rdp_validate_breaks(
	RDF_GLOBAL *global,
	DB_PART_DIM *dim_ptr,
	char errbuf[]);

/* Write out the iidistcol partitioning columns catalog */
static DB_STATUS rdp_write_distcol(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def);

/* Write out the iidistscheme partitioning dimensions catalog */
static DB_STATUS rdp_write_distscheme(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def);

/* Write out the iidistval partition break values catalog */
static DB_STATUS rdp_write_distval(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def);

/* Write out the iipartnames partition names catalog */
static DB_STATUS rdp_write_partname(
	RDF_GLOBAL *global,
	DB_PART_DEF *part_def);

/* Local defines */

/* We need guesses for the size of the breaks tables (for each
** dimension), and for a break text-value holder chunk.  These guesses
** should not be too small or too large;  either way wastes memory.
** The TPC-H will probably run to about 80+ values (all in one dimension),
** which is about 1K of DB_PART_BREAKS -- hardly a problem!
*/

#define BREAKS_MEM_GUESS	100 * (sizeof(DB_PART_BREAKS))
#define TEXTP_MEM_GUESS		150 * sizeof(DB_TEXT_STRING *)

/* The text is a little trickier to guess at, but the same 100 date
** values should end up taking about 1500 bytes
** *Important* This must be large enough for any single text value.
** The catalog is defined with a value entry of about 1500 bytes.
*/

#define TEXT_MEM_GUESS		2000


/* Data items defined here, if any */

/* The breaks table sorter uses a simple shell-sort.  Define the
** initial increments table in the usual Knuth manner, where H[1] = 1,
** and H[n] = H[n-1]*3 + 1.
*/
static const i4 incrTable[] = {
	1, 4, 13, 40, 121, 364, 1093, 3280, 9841, 29524, 88573, MAXI4
};

/* 
** Name: rdp_build_partdef - Build partitioned table definition
**
** Description:
**	This routine determines whether or not the table that RDF is
**	describing is a partitioned table (master).  If it is, the
**	partition catalogs are read and a DB_PART_DEF structure is
**	constructed.
**
**	The catalogs that we need are the ones describing the partitioning
**	scheme: iidistscheme, iidistcol, iidistval, and iipartname.
**
**	The DB_PART_DEF structure and everything that it points to
**	are built from rdu_malloc'ed memory, meaning that it all
**	comes out of the same memory stream.  We don't know ahead of
**	time how large the break values might be, as we don't know
**	how many they are nor how large the individual values are.
**	(The translated values might be something fixed like i4's, but
**	there's no a priori way to know the total text size unless we
**	were to track it in the catalogs.)  So, we'll simply guess,
**	and reallocate larger pieces as needed.
**
**	After building the partition stuff, we point the user and/or
**	system info block to it.  Normally they are the same thing;
**	if the master (system) copy was x-locked, the user infoblk
**	is a private copy.  (and the partition-stuff will come from the
**	same private stream, in the usual RDF manner.)  We're xlocked
**	at this point, so we either own the master or have a private copy.
**	Note that the actual state change must still be mutexed, in case
**	someone else decides to make a private copy just at that instant.
**
**	The catalogs are read using QEF/QEU functions.  It's not clear
**	what advantage this gives over direct DMF access, but it's what
**	the rest of RDF does.  Whatever.
**
** Input:
**	call as part of an RDF_GETDESC operation, master infoblk xlocked
**
**	global			RDF request-state block
**	    .rdf_cb		External RDF request block
**		.rdf_rb		RDF request info
**		.rdr_info_blk	RDF (result) info block
**
** Output:
**	rdr_info_blk.rdr_parts points to RDR_PART_DEF, or NULL
**
** Side Effects:
**	calls QEF to read catalogs, so can block and/or take locks.
**
** History:
**	31-Dec-2003 (schka24)
**	    Written.
**	15-Mar-2004 (schka24)
**	    Mutex state changes to master info block.
*/

DB_STATUS
rdp_build_partdef(RDF_GLOBAL *global)
{

    bool updating_master;		/* TRUE if updating master infoblk */
    DB_PART_DEF *part_def;		/* Ptr to partition stuff being built */
    DB_PART_DIM *part_dim;		/* Ptr to partition dimension entry */
    DB_STATUS status;			/* Routine call status */
    DB_STATUS toss_status;		/* Ignored, tossed */
    DMT_TBL_ENTRY *master_dmt_tbl;	/* Master-table definition */
    i4 ndims;				/* Partition dimensions */
    i4 nkeys;				/* Number of storage structure keys */
    i4 psize;				/* Memory allocation bytes needed */
    i4 qef_size;			/* Size of qef_area buffer */
    i4 *key_array;			/* Pointer to key attr #'s */
    PTR pptr;				/* Returned memory address */
    PTR qef_area;			/* Memory area for reading rows */
    RDR_INFO *user_info;		/* User RDF infoblk, might be same
					** as master, or might be private
					*/
    ULM_RCB temp_ulm_rcb;		/* Request block for temporary stream */
    i4					mrecover_cnt = 0;

    user_info = global->rdfcb->rdf_info_blk;
    master_dmt_tbl = user_info->rdr_rel;
    updating_master = (user_info == global->rdf_ulhobject->rdf_sysinfoblk);
    if (! (master_dmt_tbl->tbl_status_mask & DMT_IS_PARTITIONED))
    {
	/* State-change the info block, needs mutex if master */
	if (updating_master)
	{
	    status = rdu_gsemaphore(global);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
	user_info->rdr_parts = NULL;	/* Make sure nothing there */
	user_info->rdr_types |= RDR_BLD_PHYS;	/* We have the phys info */
	status = E_DB_OK;
	if (updating_master)
	{
	    status = rdu_rsemaphore(global);
	}
	return (status);
    }

    /* Looks like we have work to do... */
    /* Note: other RDF builders copy from the master info block if we're
    ** doing a private info block, and it looks like the master is there,
    ** and various other conditions hold.  I'm not going to bother with
    ** that, because a) it's racy unless you hold a semaphore, and b)
    ** it's such a rare case that the extra work is OK.
    */

    /* If we're holding the object semaphore, release it.  By the time
    ** we get here, we should have done an xlock, so that we either own
    ** the master copy of the object, or have a private copy.
    ** Can't be holding the semaphore over QEF/DMF calls.
    */
    if (global->rdf_resources & RDF_RSEMAPHORE)
    {
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status)) return (status);
    }

    /* We'll need to sling some temporary memory around, create a temp
    ** RDF stream for it.  (Who knows where rdu_malloc is pointing to...)
    */
    do
    {
	temp_ulm_rcb.ulm_poolid = Rdi_svcb->rdv_poolid;
	temp_ulm_rcb.ulm_memleft = &Rdi_svcb->rdv_memleft;
	temp_ulm_rcb.ulm_facility = DB_RDF_ID;
	temp_ulm_rcb.ulm_blocksize = 0;	/* No preferred block size */
	temp_ulm_rcb.ulm_streamid_p = &temp_ulm_rcb.ulm_streamid;
	/* No other thread will see this stream, can be private */
	temp_ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;

	/* The iidistval catalog has pretty big rows, so don't allocate a
	** buffer on the stack -- use the temporary stream.
	** iidistval is the biggest, for the others it will simply allow
	** reading more rows at once.
	** 10 rows at a time is plenty.  Add a little extra to allow for alignment.
	*/
	qef_size = 10 * (sizeof(QEF_DATA) + sizeof(DB_IIDISTVAL)) + sizeof(PTR);
	temp_ulm_rcb.ulm_psize = qef_size;

	status = ulm_openstream(&temp_ulm_rcb);
	if (DB_SUCCESS_MACRO(status))
	    break;

	mrecover_cnt++;
	/* Garbage-collect and try again */
	TRdisplay("%@ [%x] Garbage-collecting RDF: part openstream err %d %d psize %d (poolid %x, memleft %d try %d)\n",
	    global->rdf_sess_id, temp_ulm_rcb.ulm_error.err_code,
	    global->rdf_ulmcb.ulm_error.err_code,
	    temp_ulm_rcb.ulm_psize, temp_ulm_rcb.ulm_poolid, 
	    Rdi_svcb->rdv_memleft, mrecover_cnt);

	/* Pass rdu_mrecover the ULM_RCB we used for ulm_openstream */
	status = rdu_mrecover(global, &temp_ulm_rcb, status, E_RD0118_ULMOPEN);

    } while (DB_SUCCESS_MACRO(status));

    if (DB_FAILURE_MACRO(status))
    {
	/* Report error, return */
	rdu_ferror(global,status, &temp_ulm_rcb.ulm_error,
		    E_RD0001_NO_MORE_MEM, 0);
	return (status);
    }

    qef_area = temp_ulm_rcb.ulm_pptr;

    /* Errors from this point on should goto cleanup to release the
    ** temporary stream.
    */

    /* Allocate the partition definition and its dimensions.
    ** The DB_PART_DEF includes one dimension.
    */

    ndims = master_dmt_tbl->tbl_ndims;
    psize = sizeof(DB_PART_DEF) + (ndims - 1) * sizeof(DB_PART_DIM);
    status = rdu_malloc(global, psize, (PTR *)&part_def);
    if (DB_FAILURE_MACRO(status)) goto cleanup;
    part_def->nphys_parts = master_dmt_tbl->tbl_nparts;
    part_def->ndims = ndims;
    part_def->part_flags = 0;

    /* Read in the iidistscheme rows; one for each dimension. */
    status = rdp_read_distscheme(global, part_def, qef_area, qef_size);
    if (DB_FAILURE_MACRO(status)) goto cleanup;

    /* Read in the partition names for all dimensions */
    status = rdp_read_partname(global, part_def, qef_area, qef_size);
    if (DB_FAILURE_MACRO(status)) goto cleanup;

    /* Read in the distribution column info if there are any */
    status = rdp_read_distcol(global, part_def, qef_area, qef_size,
		user_info->rdr_no_attr, user_info->rdr_attr);
    if (DB_FAILURE_MACRO(status)) goto cleanup;

    /* Read in break table values for all dimensions */
    status = rdp_read_distval(global, part_def, qef_area, qef_size);
    if (DB_FAILURE_MACRO(status)) goto cleanup;

    /* Check for the overall distribution scheme being compatible with
    ** the master table storage structure.
    ** If no keys, don't need, leave the flag off.
    */
    if (user_info->rdr_keys != NULL && user_info->rdr_keys->key_count > 0)
    {
	nkeys = user_info->rdr_keys->key_count;
	key_array = user_info->rdr_keys->key_array;
	rdp_struct_compat(global,master_dmt_tbl->tbl_storage_type,
		nkeys,key_array,part_def);
    }

    /* The DB_PART_DEF is now all set up.  Point the user info block(s)
    ** to it.  This might actually be the master info block.
    */
    if (updating_master)
    {
	status = rdu_gsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    user_info->rdr_parts = part_def;
    user_info->rdr_types |= RDR_BLD_PHYS;	/* We have the phys info */
    if (updating_master)
    {
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }
    status = E_DB_OK;

/* Here if error to clean up; return status in "status" */
cleanup:

    /* Terminate the temporary memory stream used for the row buffers */
    toss_status = ulm_closestream(&temp_ulm_rcb);

    return (status);
} /* rdp_build_partdef */

/*
** Name: rdp_copy_partdef - Make an external copy of a partition definition
**
** Description:
**	The partition definition built by rdp_build_partdef belongs to
**	RDF, and is guaranteed to exist only as long as it's "fixed"
**	by whoever originally called the RDF_GETDESC function.  Typical
**	fix lifetimes are the duration of query parse, or query optimization;
**	RDF things are not normally fixed during query execution.
**	This means that query execution cannot reliably use RDF's
**	copy of the partition definition (unless it were to fix it, which
**	seems unreasonable).
**
**	Query execution may also need special versions of the partition
**	definition.  For instance, it may be necessary to compute the
**	physical partition number, or dimension partition sequence, of
**	some tuple partway up the query plan.  It is unlikely that the
**	partitioning column offsets at that point in the QP will exactly
**	match the original column offsets, which are computed for the
**	unmodified master table row.
**
**	So, other facilities may find it very useful to obtain a private
**	copy of a partition definition.  That's what this routine is
**	for.  It's called directly from the rdf_call dispatcher.
**
**	Callers may or may not need some of the information in the
**	partition definition.  The optional bits are the partition name
**	array, and the uninterpreted break value texts.  Both of these are
**	omitted by default;  callers can ask for them by including the
**	RDF_PCOPY_NAMES and RDF_PCOPY_TEXTS flags in the rdr_instr flag
**	word of the rdf-call request block.
**
**	Some callers (PSF, for instance) need fancier memory allocation
**	than just ulm calls.  So, the caller is expected to pass the
**	address of a memory allocation routine in rdr_partcopy_mem.
**	The routine will be called with the first param set to
**	rdr_partcopy_cb, which can be anything the caller provides.
**	The second param will be the size in bytes needed.  The
**	third parameter is where the address of the allocated memory
**	is returned.  Thus:
**	    db_status = (*rdr_partcopy_mem)(rdr_partcopy_cb, size, &ptr);
**
**	Partition definitions are not usually very big;  perhaps a few
**	hundred bytes without any LIST or RANGE dimensions, or a few K
**	with one or two LISTs or RANGEs.
**
**	The new partition copy is guaranteed to live entirely within a
**	single allocated piece of memory.  The size of that piece as well
**	as its address is returned.  This way, a caller can do one copy
**	from RDF, and then make as many private copies as it likes.
**	(Of course it's up to the caller to adjust the pointers inside
**	the partition definition, but that's still probably cheaper than
**	repeating the entire copy-from-RDF hassle over and over.)
**
**	A request to copy a nonexistent partition definition is OK as
**	long as the rest of the request is OK, and the table is not
**	partitioned;  in this case we just return a null pointer.
**
**	NOTE: While this function was originally intended to copy the
**	RDF version of a partition definition, one can use it to copy
**	an arbitrary partition definition.  Simply pass an RDF_CB that
**	points to an RDR_INFO with rdr_parts filled in, and the
**	RDR_BLD_PHYS flag set in the rdr_types flags.  If you change
**	this routine, please make sure that there are no other references
**	to any RDF stuff.  If you have a partition definition already
**	copied by this routine, it's easier to re-copy it by hand;  but
**	if you have a partition definition that's all over the place
**	(such as one built in bits and pieces by the parser), this routine
**	is probably the best option for copying.
**
** Inputs:
**	global		RDF request state block
**	rdfcb		RDF_CB rdf-call info block
**	    .rdf_rb		Request block, with caller flags
**		.rdr_partcopy_mem
**		.rdr_partcopy_cb  both filled in
**	    ->rdf_info_blk	Valid info block from a previous RDF_GETDESC
**				call (must have partition info filled in)
**				(Guaranteed that the only things in here
**				that we look at are rdr_parts and rdr_types.)
**
** Outputs:
**	Returns the usual DB_STATUS, logs messages if error
**	rdfcb.rdf_rb.rdr_newpart will point to the new partition def copy
**		    .rdr_newpartsize will contain total size
**
** Side Effects:
**	Allocates memory using caller supplied allocator routine
**
** History:
**	9-Jan-2004 (schka24)
**	    Written.
**	28-Jan-2004 (schka24)
**	    Minor tweak and doc change to allow copying from non-RDF
**	    partition def, with minimal trickery by the caller.
**	6-Apr-2004 (schka24)
**	    Better hygiene when not copying names (null out name ptr).
*/

DB_STATUS
rdp_copy_partdef(RDF_GLOBAL *global, RDF_CB *rdfcb)
{

    bool copy_names;			/* TRUE to copy partition names */
    bool copy_texts;			/* TRUE to copy value text */
    DB_PART_BREAKS *break_ptr;		/* Old break table entry pointer */
    DB_PART_BREAKS *new_break;		/* New break table entry pointer */
    DB_PART_DEF *new_def;		/* New partition definition pointer */
    DB_PART_DEF *part_def;		/* Partition definition pointer */
    DB_PART_DIM *dim_ptr;		/* Individual dimension pointer */
    DB_PART_DIM *new_dim;		/* New copy dimension pointer */
    DB_STATUS status;			/* Call error status */
    i4 def_size;			/* Size of definition+dimensions */
    i4 entry_size;			/* Size needed for one break */
    i4 psize;				/* Total size needed */
    i4 size;				/* Copy size */
    i4 val_size;			/* Value entry size */
    PTR next_addr;			/* Where in new copy next thing goes */
    PTR text_addr;			/* Where next text string goes */
    RDR_INFO *user_info;		/* Passed in RDF info area */

    /* Make sure call is OK */
    /* Caller must have passed an allocator routine;
    ** also must have passed an info block that contains valid physical
    ** info in it.
    */
    user_info = rdfcb->rdf_info_blk;
    if (rdfcb->rdf_rb.rdr_partcopy_mem == NULL
      || user_info == NULL
      || ! (user_info->rdr_types & RDR_BLD_PHYS))
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* See if the table is partitioned at all, if not just return. */
    if (user_info->rdr_parts == NULL)
    {
	rdfcb->rdf_rb.rdr_newpart = NULL;
	rdfcb->rdf_rb.rdr_newpartsize = 0;
	return (E_DB_OK);
    }
    part_def = user_info->rdr_parts;

    /* Figure how how much memory we'll need, a suitably tedious process. */

    def_size = sizeof(DB_PART_DEF) + (part_def->ndims - 1) * sizeof(DB_PART_DIM);
    psize = def_size;
    copy_names = (rdfcb->rdf_rb.rdr_instr & RDF_PCOPY_NAMES) != 0;
    copy_texts = (rdfcb->rdf_rb.rdr_instr & RDF_PCOPY_TEXTS) != 0;

    /* Work out what each dimension needs.
    ** Memory is added up differently here than it is when reading in
    ** the definition;  be somewhat paranoid about alignment.
    */
    for (dim_ptr = &part_def->dimension[part_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr)
    {
	/* Partition names for dimension (optional) */
	if (copy_names)
	{
	    psize = psize + dim_ptr->nparts * sizeof(DB_NAME);
	    psize = (psize + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	}
	/* Partitioning column info (if any, ncols might be zero) */
	psize = psize + dim_ptr->ncols * sizeof(DB_PART_LIST);
	psize = (psize + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	if (dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	{
	    /* Include the breaks table */
	    entry_size = sizeof(DB_PART_BREAKS) + dim_ptr->value_size;
	    if (copy_texts)
	    {
		/* Include ncols pointers to raw text, and the text size */
		entry_size = entry_size + dim_ptr->ncols * sizeof(DB_TEXT_STRING *);
		psize = psize + dim_ptr->text_total_size;
		psize = (psize + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	    }
	    entry_size = (entry_size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	    psize = psize + entry_size * dim_ptr->nbreaks;
	}
    }

    /* Allocate one big chunk, and copy stuff in. */
    status = (*rdfcb->rdf_rb.rdr_partcopy_mem)(rdfcb->rdf_rb.rdr_partcopy_cb,
		psize, &new_def);
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }

    MEcopy(part_def, def_size, new_def);
    next_addr = (PTR) new_def + def_size;
    /* Run new and old dimensions side by side */
    for (dim_ptr = &part_def->dimension[part_def->ndims-1],
	    new_dim = &new_def->dimension[new_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr, --new_dim)
    {
	new_dim->partnames = NULL;
	if (copy_names)
	{
	    size = dim_ptr->nparts * sizeof(DB_NAME);
	    new_dim->partnames = (DB_NAME *) next_addr;
	    MEcopy(dim_ptr->partnames, size, next_addr);
	    size = (size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	    next_addr = next_addr + size;
	}
	if (dim_ptr->ncols > 0)
	{
	    size = dim_ptr->ncols * sizeof(DB_PART_LIST);
	    new_dim->part_list = (DB_PART_LIST *) next_addr;
	    MEcopy(dim_ptr->part_list, size, next_addr);
	    size = (size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
	    next_addr = next_addr + size;
	}
	if (dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	{
	    /* Have to fiddle with copying a break table.
	    ** If we're doing text, arrange to put all the text at the
	    ** end, because I don't want to deal with the alignment issues
	    ** (I don't want to align after every text string).
	    ** Breaks table, then values and text pointers, then text.
	    ** If text, ncols text pointers after each value.
	    */
	    new_dim->part_breaks = (DB_PART_BREAKS *) next_addr;
	    next_addr = next_addr + dim_ptr->nbreaks * sizeof(DB_PART_BREAKS);
	    val_size = dim_ptr->value_size;
	    if (copy_texts)
		text_addr = next_addr + val_size * dim_ptr->nbreaks
				      + sizeof(DB_TEXT_STRING *) * dim_ptr->ncols * dim_ptr->nbreaks;
	    for (break_ptr = &dim_ptr->part_breaks[dim_ptr->nbreaks-1],
	    	    new_break = &new_dim->part_breaks[dim_ptr->nbreaks-1];
		 break_ptr >= &dim_ptr->part_breaks[0];
		 --break_ptr, --new_break)
	    {
		/* Don't bother MEcopying the breaks table, we have to
		** fix a good bit of it up anyway
		*/
		new_break->oper = break_ptr->oper;
		new_break->partseq = break_ptr->partseq;
		new_break->val_text = NULL;
		new_break->val_base = next_addr;
		next_addr = next_addr + val_size;
		MEcopy(break_ptr->val_base, val_size, new_break->val_base);
		if (copy_texts)
		{
		    /* Copying the text is a PITA; each one must be copied
		    ** individually, as they are not necessarily all
		    ** together in the original.
		    */
		    DB_TEXT_STRING **text_pp, **new_text_pp;
		    i4 j;
		    i4 len;

		    text_pp = break_ptr->val_text;
		    new_text_pp = (DB_TEXT_STRING **) next_addr;
		    next_addr = next_addr + sizeof(DB_TEXT_STRING *) * dim_ptr->ncols;
		    new_break->val_text = new_text_pp;
		    for (j = 0; j < dim_ptr->ncols; ++j)
		    {
			/* varchar length, plus count, plus null indicator */
			len = (*text_pp)->db_t_count + sizeof(u_i2) + 1;
			MEcopy(*text_pp, len, text_addr);
			len = (len + sizeof(u_i2)-1) & (~(sizeof(u_i2)-1));
			*new_text_pp++ = (DB_TEXT_STRING *) text_addr;
			text_addr = text_addr + len;
			++ text_pp;
		    }
		}
	    } /* break for */
	    /* If copying texts, skip past the actual text area */
	    if (copy_texts)
	    {
		size = (dim_ptr->text_total_size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));
		next_addr = next_addr + size;
		/* Double check my arithmetic */
		if (next_addr < text_addr)
		{
		    rdu_ierror(global, E_DB_ERROR, E_RD014C_PCOPY_ERROR);
		    return (E_DB_ERROR);
		}
	    }
	} /* if list or range */
    } /* dim for */
    /* Verify that we didn't screw up */
    if ((PTR) new_def + psize < next_addr)
    {
	rdu_ierror(global, E_DB_ERROR, E_RD014C_PCOPY_ERROR);
	return (E_DB_ERROR);
    }

    /* Note that the new copy is all in one piece;  this might be useful
    ** to callers who want to copy it around further.  Or not.  Whatever.
    */
    new_def->part_flags |= DB_PARTF_ONEPIECE;

    /* Give new definition to caller */
    rdfcb->rdf_rb.rdr_newpart = new_def;
    rdfcb->rdf_rb.rdr_newpartsize = psize;

    return (E_DB_OK);
} /* rdp_copy_partdef */

/* Name: rdp_finish_partdef - Finish/validate a partition definition
**
** Description:
**	This routine implements the RDF_PART_FINISH call.  A new
**	partition definition exists, but needs to be validated.
**	The parser calls this function when it's done parsing a new
**	partition definition.
**
**	Most (ok, all) of the activity centers around LIST and RANGE
**	dimensions.  These two have user-supplied partitioning constant
**	values, which determine what rows go where.  Each value
**	has its breaks table entry mapping the value to the target
**	partition.  There is a canonical order for the breaks table,
**	and various validations to be done (such as ensuring complete
**	domain coverage for RANGE).  The partitioning values arrive as
**	text, and need to be converted into the internal format
**	partitioning value.  All of this is done here.
**
**	Memory is allocated for the internal form partitioning values,
**	the caller will probably want to pass a ulm RCB and set the
**	RDF_USE_CALLER_ULM_RCB flag in rdr_instr.
**
**	If something turns out to be wrong with the partition definition,
**	a suitable error message detail is prepared in the caller supplied
**	rdr_errbuf.  The caller can wrap this in a descriptive message.
**
** Inputs:
**	global			RDF request state block
**	    .rdf_fulmcb		Caller supplied ULM request block
**	rdfcb			RDF call control block
**	    .rdf_rb		RDF call request block
**		.rdr_instr	set RDF_USE_CALLER_ULM_RCB (optional)
**		.rdf_fac_ulmrcb	caller sets this ULM RCB (optional)
**		.rdr_newpart	Address of DB_PART_DEF to check out
**		.rdr_errbuf	Caller supplied error message buffer
**
** Outputs:
**	Returns OK or error status; error message string in rdr_errbuf
**
** History:
**	2-Feb-2004 (schka24)
**	    Finally got the parser to where this is needed.
*/

DB_STATUS
rdp_finish_partdef(RDF_GLOBAL *global, RDF_CB *rdfcb)
{

    DB_STATUS status;			/* Called routine status */

    /* Validate the call;  there must be a partition definition to
    ** check, and a place to put error description strings.
    */
    if (rdfcb->rdf_rb.rdr_newpart == NULL
      || rdfcb->rdf_rb.rdr_errbuf == NULL)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* Call the finisher-upper */

    status = rdp_finish_breaktables(global, rdfcb->rdf_rb.rdr_newpart,
		rdfcb->rdf_rb.rdr_errbuf);

    return (status);
} /* rdp_finish_partdef */

/*
** Name: rdp_part_compat - Check partition / storage structure compatibility
**
** Description:
**	In some situations, we need to know whether the partitioning
**	scheme and the storage structure are "compatible", in this sense:
**	given a particular storage structure key, can we tell what
**	partitions the rows with that key are in?  This sort of calculation
**	is normally done against the current table storage structure
**	when a partitioning definition is read in.  But, if a MODIFY
**	is changing either the storage structure or the partitioning
**	scheme, it needs to recalculate the struct/partitioning flags.
**	That's what this RDF-call does.
**
**	Fortunately the normal struct/partition compatibility checker
**	was written to accept all the stuff it needs as parameters...
**
**	The caller should supply a partition definition in rdr_newpart,
**	the storage structure involved in rdr_storage_type, and the
**	address of an RDD_KEY_ARRAY thing in rdr_keys.  (The key array is
**	just a list of storage structure key column numbers in order.) 
**
**	Upon return, the relevant DB_PARTF_xxx flags will be set or
**	cleared in the partition definition part_flags:
**	DB_PARTF_KEY_ONE_PART if all rows for a key are in one physical
**	partition;
**	DB_PARTF_KEY_N_PARTS if all rows for a key are in N>1 physical
**	partitions, and we can figure out which.
**
** Inputs:
**	global			RDF request state area
**	rdfcb			Caller supplied request block
**	    .rdr_newpart	Pointer to partition definition
**	    .rdr_storage_type	Storage structure type (DB_xxx_STORE)
**	    .rdr_keys		Key array list header
**
** Outputs:
**	Returns E_DB_xxx status (error only if bad parameter)
**	Sets part_flags in the partition definition.
**
** History:
**	5-Aug-2004 (schak24)
**	    Expose compatibility checker to the outside world.
*/

DB_STATUS
rdp_part_compat(RDF_GLOBAL *global, RDF_CB *rdfcb)
{

    DB_PART_DEF *part_def;		/* Partition definition to check */
    DB_STATUS status;			/* The usual return status */
    RDD_KEY_ARRAY *key_header;		/* Key info header */

    /* Make sure caller passed needed info */
    part_def = rdfcb->rdf_rb.rdr_newpart;
    key_header = rdfcb->rdf_rb.rdr_keys;
    if (part_def == NULL || key_header == NULL)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }
    /* Assume no flags should be set */
    part_def->part_flags &= ~(DB_PARTF_KEY_ONE_PART | DB_PARTF_KEY_N_PARTS);

    /* "No keys" shouldn't happen, but basically that means leave the
    ** compat flags off.
    */
    if (key_header->key_count == 0
      || rdfcb->rdf_rb.rdr_storage_type == DB_HEAP_STORE)
	return (E_DB_OK);

    /* Call the checker to turn on flags that should be set */
    rdp_struct_compat(global, rdfcb->rdf_rb.rdr_storage_type,
		key_header->key_count, key_header->key_array, part_def);

    return (E_DB_OK);
} /* rdp_part_compat */

/*
** Name: rdp_update_partdef - Update partition catalogs with new definition
**
** Description:
**	This routine implements the RDF_PART_UPDATE rdf-call.  Someone has
**	constructed a new or perhaps modified partition definition, and
**	wants it written out.
**
**	I did not piggyback this onto RDF_UPDATE partly because RDF_UPDATE
**	is already squirrely enough, with a bazillion flags and special
**	cases;  but mostly because RDF_UPDATE is more concerned with
**	separate database objects like views, permits, DB procedures, and
**	the like.  A partition definition is a semi-physical thing and
**	not a qrymod thing.
**
**	The machinery here is simple enough.  We disassemble a partition
**	definition into tuples, doing pretty much the inverse of what was
**	done to read it in.  The various tuples are written out to the
**	partition catalogs in the same order we read them (iidistscheme
**	first, then iipartname, iidistcol, iidistval).  The relnparts
**	and relnpartlevels columns in iirelation are maintained by DMF
**	and are not updated here.
**
**	Partition updating makes numerous assumptions, mostly boiling
**	down to "the caller is not doing anything stupid".  The new
**	definition must be complete, and must include all partition names
**	and uninterpreted value texts.  The new definition must be valid;
**	the caller must check, we assume it's OK if we get this far!
**	We do NOT assume that the partition definition being read in
**	belongs to RDF;  it may in fact have been constructed by another
**	facility, such as the parser.  In fact there is no assumption that
**	RDF knows about any previous partition definition at all.
**
**	As a special case, the new partition definition can have ndims == 0,
**	which is equivalent to "NOPARTITIONS".  Any old partition definition is
**	deleted and no new one is written.
**
**	We assume that the external facility has done something to protect
**	against other sessions doing an update, or working with an obsolete
**	partition definition.  (That "something" is almost certainly an
**	X control lock on the master table.)
**
**	We assume that the external facility will invalidate the table's
**	definition out of RDF after completing whatever it's doing.
**	No attempt is made to update the RDF copy of the partition
**	definition (if there even is one!) with the new definition.
**
**	The caller (or some higher level) is responsible for transaction
**	control.  A read/write transaction must be in progress before calling
**	this routine.
**
** Inputs:
**	global			RDF request state area
**	rdfcb			rdf-call request block
**	    .rdf_rb		Request block, with caller flags, ulm-rcb
**		.rdr_tabid	Table ID of master table
**		.rdr_newpart	Points to new partition info to write out
**		.rdr_db_id	Database ID
**	    ->rdf_info_blk	not used
**
** Outputs:
**	Returns the usual DB_STATUS
**
** Side Effects:
**	Writes rows via QEF, DMF;  can block, will take locks
**
** History:
**	9-Jan-2004 (schka24)
**	    Initial version.
**	11-May-2006 (kschendel)
**	    I forgot to init the lock mode, so it was being passed as
**	    LK_N.  Apparently it mostly worked, but DMF wasn't doing the
**	    auto-row-lock thing for catalogs because of this.
*/

DB_STATUS
rdp_update_partdef(RDF_GLOBAL *global, RDF_CB *rdfcb)
{

    DB_PART_DEF *part_def;		/* Partition definition to write out */
    DB_STATUS status;			/* The usual return status */

    /* Make sure caller passed a new partition definition */
    part_def = rdfcb->rdf_rb.rdr_newpart;
    if (part_def == NULL)
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return (status);
    }

    /* RDF QEU utilities no help here! use QEU directly. */
    /* Note that global's QEU_CB starts out zeroed thanks to rdf-call.
    ** Initialize a few things that remain constant throughout; for instance,
    ** all of the catalogs involved use a (partial) key of the master table ID.
    */
    global->rdf_qeucb.qeu_key = &global->rdf_keyptrs[0];
    global->rdf_keyptrs[0] = &global->rdf_keys[0];
    global->rdf_keyptrs[1] = &global->rdf_keys[1];
    global->rdf_qeucb.qeu_klen = 2;	/* Master table id columns */
    global->rdf_keys[0].attr_operator = DMR_OP_EQ;
    global->rdf_keys[0].attr_value = (PTR) &rdfcb->rdf_rb.rdr_tabid.db_tab_base;
    global->rdf_keys[1].attr_operator = DMR_OP_EQ;
    global->rdf_keys[1].attr_value = (PTR) &rdfcb->rdf_rb.rdr_tabid.db_tab_index;
    global->rdf_qeucb.qeu_access_mode = DMT_A_WRITE;
    global->rdf_qeucb.qeu_db_id = rdfcb->rdf_rb.rdr_db_id;
    global->rdf_qeucb.qeu_d_id = global->rdf_sess_id;
    global->rdf_qeucb.qeu_lk_mode = DMT_IX;

    /* Write out the various pieces of the definition, same order as we
    ** read them in (for deadlock avoidance).
    */
    status = rdp_write_distscheme(global, part_def);
    if (DB_SUCCESS_MACRO(status))
	status = rdp_write_partname(global, part_def);
    if (DB_SUCCESS_MACRO(status))
	status = rdp_write_distcol(global, part_def);
    if (DB_SUCCESS_MACRO(status))
	status = rdp_write_distval(global, part_def);

    return (status);
} /* rdp_update_partdef */

/*
** Name: rdp_finish_breaktables - Construct the breaks table(s) from raw data
**
** Description:
**	After the iidistval rows are read in, the breaks table(s) for the
**	various partitioning dimensions exists;  however there is still
**	much to do.  There are two primary chores:
**
**	1. The uninterpreted partitioning value texts have to be
**	translated into an internal-form partitioning value.
**
**	2. The result needs to be sorted into ascending order and
**	validated.  Besides simplifying validation, ascending order
**	allows the runtime to use binary search for partition
**	number translation.
**
**	If something goes wrong, an error-string buffer is returned
**	instead of issuing the error, so that the caller can decide
**	what to do with the error.
**
**	FOR LATER: a raw-text breaks table might be a reasonable way
**	for the parser to supply a trial partitioning scheme for
**	checking, if so then this routine should be made accessible
**	via rdf-call ...
**
** Inputs:
**	global		RDF_GLOBAL * RDF request state area
**	part_def	DB_PART_DEF * Pointer to the partition definition
**	errbuf		pointer to error area
**
** Outputs:
**	Returns DB_STATUS
**	errbuf		Filled in with asciz error string if error
**
** History:
**	2-Jan-2004 (schka24)
**	    Written.
**	20-Jan-2004 (schka24)
**	    Value size now stored into the dimension info.
**	4-Feb-2004 (schka24)
**	    Allow call if no breaks in the partition def (just return).
**	28-Aug-2007 (kschendel) 122041
**	    Zero out value-size if no breaks (only for clean opf traces).
*/

static DB_STATUS
rdp_finish_breaktables(RDF_GLOBAL *global, DB_PART_DEF *part_def,
	char errbuf[])
{

    DB_PART_BREAKS *break_ptr;		/* Pointer to a value entry */
    DB_PART_DIM *dim_ptr;		/* Pointer to a dimension entry */
    DB_STATUS status;			/* Routine call status */
    i4 dim;				/* A dimension index */
    i4 psize;				/* Memory size needed */
    i4 val_size;			/* Values needed for a dimension */
    PTR val_area;			/* Base of value holder area */
    PTR value_ptr;			/* Pointer to one value */

    errbuf[0] = '\0';			/* In case no error */

    /* The first order of business is to determine how much memory
    ** will be needed for all of the internal-format partition values.
    ** We can figure this out by looking at the dimension's DB_PART_LIST
    ** which describes a canonical internal-format value via the
    ** val_offset member.  The largest val_offset plus its length is
    ** the length of a single internal partition value;  times the
    ** number of breaks table entries for that dimension;  added
    ** up for all dimensions.
    */

    psize = 0;
    for (dim_ptr = &part_def->dimension[part_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr)
    {
	dim_ptr->value_size = 0;
	if (dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	{
	    val_size = rdp_val_size(dim_ptr);
	    psize = psize + val_size * dim_ptr->nbreaks;
	    /* save the value size we computed */
	    dim_ptr->value_size = val_size;
	}
    }

    /* If nothing to do (external call), just return */
    if (psize == 0)
	return (E_DB_OK);

    status = rdu_malloc(global, psize, &val_area);
    if (DB_FAILURE_MACRO(status))
    {
	/* RDF issued error in this case */
	return (status);
    }

    /* Clear out the value area, only because of DEFAULT entries.  The value
    ** for these is meaningless, but until we manage to get everything checked,
    ** it's possible that the sort- and validate-breaks routines will attempt
    ** to do a compare against a DEFAULT entry value.  There has to be something
    ** rational in there.  Zero is always rational for pretty much any datatype,
    ** as far as I know...
    */
    MEfill(psize, 0, val_area);

    /* Go back through and hand out value memory to the various break
    ** table entries; and fill in the values.
    ** After the internal partition values are all computed, we can
    ** sort the breaks table into its proper form, and check it out.
    */

    value_ptr = val_area;
    for (dim = 0; dim < part_def->ndims; ++dim)
    {
	dim_ptr = &part_def->dimension[dim];
	if (dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	{
	    for (break_ptr = &dim_ptr->part_breaks[dim_ptr->nbreaks-1];
		 break_ptr >= &dim_ptr->part_breaks[0];
		 --break_ptr)
	    {
		break_ptr->val_base = value_ptr;
		value_ptr = value_ptr + dim_ptr->value_size;
		/* Value for DEFAULT is meaningless, convert others */
		if (break_ptr->oper != DBDS_OP_DFLT)
		{
		    status = rdp_text_to_internal(global, dim_ptr, break_ptr, errbuf);
		    if (DB_FAILURE_MACRO(status))
			return (status);
		}
	    }
	    /* Sort and validate the breaks table */
	    status = rdp_sort_breaks(global, dim_ptr, errbuf);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    status = rdp_validate_breaks(global, dim_ptr, errbuf);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	} /* if range or list */
    } /* for dimptr */

    return (E_DB_OK);
} /* rdp_finish_breaktables */

/*
** Name: rdp_inconsistent - issue catalog-inconsistency errmsg
**
** Description:
**	This routine issues an inconsistent-catalog message.
**
**	The RDF error message utilities are wimpy, and don't appear
**	to allow any sort of parameterization.  Since we're trying
**	to do better than "error" these days, and I don't have time
**	to fix the error utilities, this routine formats and outputs
**	the message itself.
**
**	The caller specifies the catalog being read, and a short
**	descriptive message string.
**
** Inputs:
**	global		RDF request state block
**	catalog		String giving the catalog name
**	str		Additional descriptive string
**
** Outputs:
**	none
**
** Side Effects:
**	Issues error via SCF
**
*/

static void
rdp_inconsistent(RDF_GLOBAL *global, char *catalog, char *str)
{

    char msg_buffer[DB_ERR_SIZE];	/* Where the message goes */
    DB_SQLSTATE sqlstate;		/* generated sqlstate thing */
    DB_STATUS status;			/* The usual return status */
    i4 msg_buf_length;			/* Length of error in msg_buffer */
    i4 ule_error;			/* Formatting error status */

    /* Give caller a generic, parameter-less error saying RDF has
    ** reported the error.  (The associated msg is a little stupid but... )
    */
    SETDBERR(&global->rdfcb->rdf_error, 0, E_RD0025_USER_ERROR);

    status = uleFormat(NULL, E_RD014B_BAD_PARTCAT, NULL, ULE_LOG, &sqlstate,
		msg_buffer, sizeof(msg_buffer)-1, &msg_buf_length,
		&ule_error, 4,
		STlen(catalog), catalog,
		STlen(str), str,
		sizeof(DB_OWN_NAME), global->rdfcb->rdf_info_blk->rdr_rel->tbl_owner,
		sizeof(DB_TAB_NAME), global->rdfcb->rdf_info_blk->rdr_rel->tbl_name
		);
    if (status != E_DB_OK)
    {
	STprintf(msg_buffer, "ULE error %x, RDF message %x cannot be found\n",
		ule_error, E_RD014B_BAD_PARTCAT);
	TRdisplay("%s",msg_buffer);
	msg_buf_length = STlen(msg_buffer);
    }
    else
	msg_buffer[msg_buf_length] = '\0'; /* Ensure null-terminated */

    /* Output via SCF */
    (void) rdu_sccerror(E_DB_ERROR, &sqlstate, E_RD014B_BAD_PARTCAT,
		msg_buffer, msg_buf_length);

} /* rdp_inconsistent */

/*
** Name: rdp_read_distcol - read in iidistcol rows
**
** Description:
**	This routine reads in the distribution columns for all
**	the dimensions.  These are stored as DB_PART_LIST entries.
**
**	Each iidistcol row contains the levelno (dimension) and column
**	number.  Since the total number of DB_PART_LIST entries to
**	build is known ahead of time, we can allocate them all at
**	once, and fill them in as we read rows.
**
** Inputs:
**	global		RDF_GLOBAL * RDF request state area
**	part_def	DB_PART_DEF * pointer to partition definition
**	qef_area	char * buffer for reading rows
**	qef_size	i4 size of qef_area in bytes
**	no_cols		i4 number of base table columns (for consistency
**			check)
**	mast_att_list	DB_ATT_ENTRY ** array of pointers to column
**			definitions for the master table
**
** Outputs:
**	Returns DB_STATUS
**
** Side Effects:
**	QEF/DMF called, this routine can take locks and/or block
**
** History:
**	1-Jan-2004 (schka24)
**	    Written
**	19-Mar-2004 (schka24)
**	    Might not be any columns, fix.
**	16-nov-2006 (dougi)
*/

static DB_STATUS
rdp_read_distcol(RDF_GLOBAL *global, DB_PART_DEF *part_def,
	char *qef_area, i4 qef_size, i4 no_cols,
	DMT_ATT_ENTRY **mast_att_list)
{

    bool no_more_rows;			/* End of input indicator */
    DB_IIDISTCOL *row_ptr;		/* Address of catalog row */
    DB_PART_DIM *dim_ptr;		/* Pointer to a dimension entry */
    DB_PART_LIST *db_part_ptr;		/* Pointer to individual entry */
    DB_PART_LIST *list_area_base;	/* Base of the DB_PART_LIST array */
    DB_STATUS status;			/* Return status holder */
    DB_STATUS toss_status;		/* Ignore return value */
    DMT_ATT_ENTRY *att_ptr;		/* Pointer to column definition */
    i4 i;
    i4 rows_read;			/* Number of rows processed */
    i4 total_cols;			/* Number of columns involved total */
    i4 val_offset;			/* Partitioning value offset */
    QEF_DATA *qef_db;			/* Pointer to QEF row descriptor */

    /* Before reading the iidistcol rows, figure out how much space
    ** is needed, and allocate it.  Each column entry builds a DB_PART_LIST,
    ** and there are sum(dim_ptr->ncols) of them.
    */
    total_cols = 0;
    for (dim_ptr = &part_def->dimension[part_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr)
	total_cols = total_cols + dim_ptr->ncols;

    if (total_cols == 0)
	return (E_DB_OK);		/* They're all AUTOMATIC */

    status = rdu_malloc(global, total_cols * sizeof(DB_PART_LIST), (PTR *) &list_area_base);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Give each dimension its section of the list array.
    ** Careful, not all dimensions necessarily use value dependent schemes!
    */
    i = 0;
    for (dim_ptr = &part_def->dimension[0];
	 dim_ptr <= &part_def->dimension[part_def->ndims-1];
	 ++dim_ptr)
    {
	if (dim_ptr->ncols != 0)
	{
	    dim_ptr->part_list = &list_area_base[i];
	    i = i + dim_ptr->ncols;
	}
    }

    /* Now open and read the catalog */
    status = rdu_qopen(global, RD_IIDISTCOL, NULL,
			sizeof(DB_IIDISTCOL), qef_area, qef_size);
    if (DB_FAILURE_MACRO(status))
	return (status);
    rows_read = 0;
    for (;;)
    {
	status = rdu_qget(global, &no_more_rows);
	if (DB_FAILURE_MACRO(status))
	{
	    toss_status = rdu_qclose(global);
	    return (status);
	}
	qef_db = global->rdf_qeucb.qeu_output;
	for (i = 0; i < global->rdf_qeucb.qeu_count; ++i)
	{
	    ++ rows_read;
	    row_ptr = (DB_IIDISTCOL *) (qef_db->dt_data);
	    if (row_ptr->levelno <= 0 || row_ptr->levelno > part_def->ndims)
	    {
		rdp_inconsistent(global,"iidistcol","invalid levelno");
		return (E_DB_ERROR);
	    }
	    dim_ptr = &part_def->dimension[row_ptr->levelno - 1];
	    if (row_ptr->colseq <= 0 || row_ptr->colseq > dim_ptr->ncols)
	    {
		rdp_inconsistent(global,"iidistcol","invalid colseq");
		return (E_DB_ERROR);
	    }
	    /* All we'll do here is fill in the column number.
	    ** We'll leave all the offset calculations for later.
	    */
	    dim_ptr->part_list[row_ptr->colseq-1].att_number = row_ptr->attid;
	    qef_db = qef_db->dt_next;
	}
	if (no_more_rows) break;
    } /* qget loop */

    /* No more rows, do final checks and calculations */
    if (rows_read != total_cols)
    {
	rdp_inconsistent(global,"iidistcol","wrong # of rows");
	return (E_DB_ERROR);
    }
    status = rdu_qclose(global);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Post-process the DB_PART_LIST entries.
    ** Fill in the type/precision/length stuff from the master table.
    ** The row-offset also comes from the master table column info.
    ** The val-offset is calculated based on an aligned layout of just
    ** the dimension's partitioning columns.
    */
    for (dim_ptr = &part_def->dimension[0];
	 dim_ptr <= &part_def->dimension[part_def->ndims-1];
	 ++dim_ptr)
    {
	if (dim_ptr->ncols > 0)
	{
	    val_offset = 0;		/* New value area per dimension */
	    /* Important note: I tend to run for loops in whichever way
	    ** seems convenient, but this one must run from 0 to N-1, or
	    ** rdp_val_size has to change.
	    */
	    for (db_part_ptr = &dim_ptr->part_list[0];
		 db_part_ptr <= &dim_ptr->part_list[dim_ptr->ncols-1];
		 ++db_part_ptr)
	    {
		/* Note mast-att-list points to an array of pointers,
		** not an array of entries
		*/
		if (db_part_ptr->att_number > no_cols)
		{
		    /* Partitioning col has bigger number than last base
		    ** table column. Consistency error. */
		    rdp_inconsistent(global,"iidistcol","att_number too large");
		    return (E_DB_ERROR);
		}
		att_ptr = mast_att_list[db_part_ptr->att_number];
		/* Copy stuff from master attribute */
		db_part_ptr->type = att_ptr->att_type;
		db_part_ptr->precision = att_ptr->att_prec;
		db_part_ptr->length = att_ptr->att_width;
		db_part_ptr->row_offset = att_ptr->att_offset;
		/* Fill in the offset within a value string for this dim */
		/* Double-align everything.  This is perhaps somewhat wasteful,
		** but it makes life easier, since it's the worst case.
		*/
		val_offset = (val_offset + (DB_ALIGN_SZ-1)) &
				( ~(DB_ALIGN_SZ-1) );
		db_part_ptr->val_offset = val_offset;
		val_offset = val_offset + db_part_ptr->length;
	    }
	} /* ncols > 0 */
    } /* for dim_ptr */

    return (E_DB_OK);
} /* rdp_read_distcol */

/*
** Name: rdp_read_distscheme - read in iidistscheme rows
**
** Description:
**	The first step in reading the partitioning catalogs is to read
**	iidistscheme.  This catalog has one entry per dimension, and
**	gives the distribution rule, number of partitions, and number
**	of columns making up the partition selector.
**
**	The caller has already allocated the partition definition
**	area, so our job is pretty simple.
**
** Inputs:
**	global		RDF_GLOBAL * RDF request state area
**	part_def	DB_PART_DEF * pointer to partition definition
**	qef_area	char * buffer for reading rows
**	qef_size	i4 size of qef_area in bytes
**
** Outputs:
**	Returns DB_STATUS
**	DB_PART_DIM entries in the partition definition (partially)
**	filled in.
**
** Side Effects:
**	QEF/DMF called, this routine can take locks and/or block
**
** History:
**	1-Jan-2004 (schka24)
**	    Written
*/

static DB_STATUS
rdp_read_distscheme(RDF_GLOBAL *global, DB_PART_DEF *part_def,
	char *qef_area, i4 qef_size)
{

    bool no_more_rows;			/* End of input indicator */
    DB_IIDISTSCHEME *row_ptr;		/* Pointer to an input row */
    DB_PART_DIM *dim_ptr;		/* Pointer to a dimension entry */
    DB_STATUS status;			/* Return status holder */
    DB_STATUS toss_status;		/* Ignore return value */
    i4 i;
    i4 rows_read;			/* Number of rows processed */
    i4 stride;				/* Dimensions "array stride" */
    QEF_DATA *qef_db;			/* Pointer to QEF row descriptor */

    status = rdu_qopen(global, RD_IIDISTSCHEME, NULL,
			sizeof(DB_IIDISTSCHEME), qef_area, qef_size);
    if (DB_FAILURE_MACRO(status))
	return (status);
    rows_read = 0;
    for (;;)
    {
	status = rdu_qget(global, &no_more_rows);
	if (DB_FAILURE_MACRO(status))
	{
	    toss_status = rdu_qclose(global);
	    return (status);
	}
	qef_db = global->rdf_qeucb.qeu_output;
	for (i = 0; i < global->rdf_qeucb.qeu_count; ++i)
	{
	    ++ rows_read;
	    row_ptr = (DB_IIDISTSCHEME *) (qef_db->dt_data);
	    if (row_ptr->levelno <= 0 || row_ptr->levelno > part_def->ndims)
	    {
		rdp_inconsistent(global,"iidistscheme","invalid levelno");
		return (E_DB_ERROR);
	    }
	    if (row_ptr->distrule != DBDS_RULE_AUTOMATIC && row_ptr->ncols <= 0)
	    {
		rdp_inconsistent(global,"iidistscheme","no partition columns");
		return (E_DB_ERROR);
	    }
	    dim_ptr = &part_def->dimension[row_ptr->levelno - 1];
	    dim_ptr->distrule = row_ptr->distrule;
	    dim_ptr->ncols = row_ptr->ncols;
	    dim_ptr->nparts = row_ptr->nparts;
	    /* Initialize other stuff in the dimension */
	    dim_ptr->nbreaks = 0;		/* Don't know yet */
	    dim_ptr->text_total_size = 0;
	    dim_ptr->partnames = NULL;		/* Will read soon */
	    dim_ptr->part_list = NULL;		/* May stay NULL */
	    dim_ptr->part_breaks = NULL;	/* Ditto */
	    /* We do the stride below */
	    qef_db = qef_db->dt_next;
	}
	if (no_more_rows) break;
    } /* qget loop */

    /* No more rows, do final checks and calculate stride */
    if (rows_read != part_def->ndims)
    {
	rdp_inconsistent(global,"iidistscheme","wrong # of rows");
	return (E_DB_ERROR);
    }
    status = rdu_qclose(global);
    if (DB_FAILURE_MACRO(status))
	return (status);
    /* The "stride" of a dimension is the number of physical partitions
    ** making up one entry (or logical partition) at that dimension.
    ** The innermost dimension has stride 1 and we simply multiply by
    ** the size of each dimension as we work to the outermost.
    */
    dim_ptr = &part_def->dimension[part_def->ndims - 1];
    stride = 1;
    do
    {
	dim_ptr->stride = stride;
	stride = stride * dim_ptr->nparts;
    } while (--dim_ptr >= &part_def->dimension[0]);

    return (E_DB_OK);
} /* rdp_read_distscheme */

/*
** Name: rdp_read_distval - read in iidistval rows
**
** Description:
**	This routine reads in the break table values for all the dimensions.
**	This is probably the most complicated part of reading
**	the partitioning catalogs.
**
**	RANGE and LIST distributions need a set of break table values.
**	To compute a partition number, a row value is compared against
**	the break table.  For LIST, we're looking for an exact-match;
**	for RANGE, we're looking for the range that contains the value.
**	(There will always be such a range for properly constructed
**	break tables.)
**
**	Reading the break values out of iidistval is a little trickier
**	than reading the other partitioning catalogs.  Part of the reason
**	is that we don't know how many break entries to expect.
**	(There's no "nbreaks" in the catalogs, which makes a few of
**	the partition maintenance operations a tiny bit cleaner.)
**	In addition, we're storing the value two different ways:  as
**	the original text, and as an internal value string.  There's
**	no way to know how big the texts will be, even if we knew
**	how many values to expect.
**
**	This is handled by guessing at a reasonable size for various
**	tables.  We use three general areas:  someplace to put breaks
**	entries, someplace to put the text pointer arrays, and someplace
**	to put text.  The breaks table and text-pointer arrays are
**	order sensitive, and if they fill up, a larger area is
**	allocated and used.  If a text value area fills up, we just
**	allocate another one.  Unused space in any of the memory
**	chunks is just wasted, it's not that much memory anyway.
**	
**	Each dimension gets its own memory piece for a breaks table,
**	and another memory piece for the text pointer arrays.
**	table.  If a pointer array fills up, it's a pain, since we
**	have to relocate existing pointers in the breaks entries.
**
**	After reading everything in, considerable post-processing is
**	done.  The text values are converted to internal format
**	(at least we'll know how much room it takes!).  Then each
**	table is sorted into ascending order and validated.
**
**	By the way, I considered using our temporary memory stream
**	while reading the rows, and then copying everything into
**	RDF's regular stream once we know how much we want.  Since
**	we're only talking about a few K I didn't bother.
**
**	The iidistval catalog is somewhat de-normalized;  a fully
**	normalized approach would split it into iidistval (a header
**	with the oper and partseq), and iidistvalcol (the N-th
**	column element of the value).  I'm not about to do that,
**	because it's just more work for little gain.  We will
**	assume that oper and partseq are all the same for all
**	column elements of a given valseq!
**
** Inputs:
**	global		RDF_GLOBAL * RDF request state area
**	part_def	DB_PART_DEF * pointer to partition definition
**	qef_area	char * buffer for reading rows
**	qef_size	i4 size of qef_area in bytes
**
** Outputs:
**	Returns DB_STATUS
**
** Side Effects:
**	QEF/DMF called, this routine can take locks and/or block
**
** History:
**	1-Jan-2004 (schka24)
**	    Written
**	15-Jun-2006 (kschendel)
**	    D'oh!  Text-pointer array expand forgot to move the existing
**	    entries!  How did I not test that one....?
*/

static DB_STATUS
rdp_read_distval(RDF_GLOBAL *global, DB_PART_DEF *part_def,
	char *qef_area, i4 qef_size)
{

    /* The things dimensioned DBDS_MAX_LEVELS have one entry per
    ** partitioning dimension that use range or list.  (Which will
    ** likely be just one dimension...)
    */

    bool need_vals;			/* TRUE if we need to do this */
    bool no_more_rows;			/* End of input indicator */
    char errbuf[DB_ERR_SIZE];		/* Place for lower-level errors */
    DB_IIDISTVAL *row_ptr;		/* Pointer to a catalog row */
    DB_PART_BREAKS *break_ptr;		/* Pointer to a value entry */
    DB_PART_DIM *dim_ptr;		/* Pointer to a dimension entry */
    DB_STATUS status;			/* Return status holder */
    DB_STATUS toss_status;		/* Ignore return value */
    DB_TEXT_STRING **textp_arrays[DBDS_MAX_LEVELS]; /* Text pointer array areas */
    DB_TEXT_STRING *text_putter;	/* Where next text value goes */
    i4 break_entries[DBDS_MAX_LEVELS];	/* # of entries in breaks/array area */
    i4 break_size[DBDS_MAX_LEVELS];	/* Current size of breaks area */
    i4 i;
    i4 dim;				/* Level/dimension index */
    i4 rows_read;			/* Number of rows processed */
    i4 text_left;			/* Bytes of text area left */
    i4 text_len;			/* Length of row's text value */
    i4 textp_entries[DBDS_MAX_LEVELS];	/* # of pointers left in textp */
    i4 textp_index;			/* Catalog row's textp array index */
    i4 textp_size[DBDS_MAX_LEVELS];	/* Current size of textp area */
    QEF_DATA *qef_db;			/* Pointer to QEF row descriptor */
    u_i2 nchars;			/* Actual chars in text value */

    /* Allocate initial guess sizes for the breaks tables and the
    ** value text areas.  These really aren't too big anyway...
    */
    need_vals = FALSE;
    for (dim = 0; dim < part_def->ndims; ++dim)
    {
	dim_ptr = &part_def->dimension[dim];
	if (dim_ptr->distrule == DBDS_RULE_LIST || dim_ptr->distrule == DBDS_RULE_RANGE)
	{
	    need_vals = TRUE;
	    status = rdu_malloc(global, BREAKS_MEM_GUESS,
			(PTR *) &dim_ptr->part_breaks);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    /* Zero out the breaks so that we don't see a garbage
	    ** pointer if we have to expand the text ptr area (below)
	    */
	    MEfill(BREAKS_MEM_GUESS, 0, dim_ptr->part_breaks);
	    break_size[dim] = BREAKS_MEM_GUESS;
	    /* Each entry is a DB_PART_BREAKS */
	    break_entries[dim] = BREAKS_MEM_GUESS / sizeof(DB_PART_BREAKS);
	    status = rdu_malloc(global, TEXTP_MEM_GUESS * dim_ptr->ncols,
			(PTR *) &textp_arrays[dim]);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	    textp_size[dim] = TEXTP_MEM_GUESS * dim_ptr->ncols;
	    textp_entries[dim] = textp_size[dim] / sizeof(DB_TEXT_STRING *);
	}
    }
    /* If no range/list, nothing more to do */
    if (! need_vals)
	return (E_DB_OK);

    /* Can use one memory area for all text, regardless of dimension or
    ** value sequence or anything
    */
    status = rdu_malloc(global, TEXT_MEM_GUESS, (PTR *) &text_putter);
    if (DB_FAILURE_MACRO(status))
	return (status);
    text_left = TEXT_MEM_GUESS;

    /* Now open and read the catalog */
    status = rdu_qopen(global, RD_IIDISTVAL, NULL,
			sizeof(DB_IIDISTVAL), qef_area, qef_size);
    if (DB_FAILURE_MACRO(status))
	return (status);
    rows_read = 0;
    for (;;)
    {
	status = rdu_qget(global, &no_more_rows);
	if (DB_FAILURE_MACRO(status))
	{
	    toss_status = rdu_qclose(global);
	    return (status);
	}
	qef_db = global->rdf_qeucb.qeu_output;
	for (i = 0; i < global->rdf_qeucb.qeu_count; ++i)
	{
	    ++ rows_read;
	    row_ptr = (DB_IIDISTVAL *) (qef_db->dt_data);
	    if (row_ptr->levelno <= 0 || row_ptr->levelno > part_def->ndims)
	    {
		rdp_inconsistent(global,"iidistval","invalid levelno");
		return (E_DB_ERROR);
	    }
	    dim = row_ptr->levelno - 1;
	    dim_ptr = &part_def->dimension[dim];
	    if (row_ptr->colseq <= 0 || row_ptr->colseq > dim_ptr->ncols)
	    {
		rdp_inconsistent(global,"iidistval","invalid colseq");
		return (E_DB_ERROR);
	    }
	    /* See if we have room for this db-part-break entry */
	    while (row_ptr->valseq > break_entries[dim])
	    {
		DB_PART_BREAKS *old_breaks;
		i4 old_size;			/* Original area size */

		/* Need to expand; expand by the initial guess */
		old_breaks = dim_ptr->part_breaks;
		old_size = break_size[dim];
		break_size[dim] = old_size + BREAKS_MEM_GUESS;
		status = rdu_malloc(global, break_size[dim], (PTR *) &dim_ptr->part_breaks);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		break_entries[dim] = break_size[dim] / sizeof(DB_PART_BREAKS);
		MEcopy(old_breaks, old_size, dim_ptr->part_breaks);
		/* Keep the new part clean, no garbage text ptrs */
		MEfill(BREAKS_MEM_GUESS,0,(PTR)(dim_ptr->part_breaks)+old_size);
		/* No way to return the old chunk - brilliant... */
	    }
	    /* See if we have room for this row's text-pointer */
	    textp_index = (row_ptr->valseq-1) * dim_ptr->ncols + row_ptr->colseq-1;
	    while (textp_index >= textp_entries[dim])
	    {
		DB_TEXT_STRING **old_textp;
		i4 old_size;			/* Original area size */

		/* Need to expand, what a PITA - have to redo pointers */
		old_textp = textp_arrays[dim];
		old_size = textp_size[dim];
		textp_size[dim] = old_size + TEXTP_MEM_GUESS;
		status = rdu_malloc(global, textp_size[dim], (PTR *) &textp_arrays[dim]);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		textp_entries[dim] = textp_size[dim] / sizeof(DB_TEXT_STRING *);
		MEcopy(old_textp, old_size, textp_arrays[dim]);
		for (break_ptr = &dim_ptr->part_breaks[break_entries[dim]-1];
		     break_ptr >= &dim_ptr->part_breaks[0];
		     --break_ptr)
		{
		    /* This IF is why we mefill the breaks to zero! */
		    if (break_ptr->val_text != NULL)
			break_ptr->val_text = (DB_TEXT_STRING **) (
				((PTR) break_ptr->val_text - (PTR) old_textp) + (PTR) textp_arrays[dim] );
		}
	    }
	    /* See if we have room for this row's actual text value.
	    ** Include the length itself and a null indicator.
	    ** Warning, rowptr->value is not aligned!
	    ** The value for a DEFAULT entry is meaningless, but to
	    ** simplify the code, we'll just store a zero-length string.
	    */
	    I2ASSIGN_MACRO(row_ptr->value[0],nchars);
	    if (row_ptr->oper == DBDS_OP_DFLT) nchars = 0;
	    text_len = nchars + sizeof(u_i2) + 1;
	    if (text_len > text_left)
	    {
		/* Grab another piece.  Assumes TEXT_MEM_GUESS is large
		** enough for any value!
		*/
		status = rdu_malloc(global, TEXT_MEM_GUESS, (PTR *) &text_putter);
		if (DB_FAILURE_MACRO(status))
		    return (status);
		text_left = TEXT_MEM_GUESS;
	    }
	    /* Make sure that the operator code is OK */
	    if (row_ptr->oper < 0 || row_ptr->oper > DBDS_OP_DFLT)
	    {
		rdp_inconsistent(global,"iidistval","invalid oper code");
		return (E_DB_ERROR);
	    }
	    /* Sheesh!  After all that baloney, we can now actually store
	    ** the row data somewhere...
	    */
	    break_ptr = &dim_ptr->part_breaks[row_ptr->valseq-1];
	    break_ptr->oper = row_ptr->oper;
	    break_ptr->partseq = row_ptr->partseq;
	    *(textp_arrays[dim] + textp_index) = text_putter;
	    /* textp_index is this column element, break points to the FIRST
	    ** column element text;  adjust the index back to the first
	    */
	    break_ptr->val_text = textp_arrays[dim] + (textp_index - (row_ptr->colseq - 1));
	    /* Count gets text length only, no overhead */
	    text_putter->db_t_count = nchars;
	    MEcopy(&row_ptr->value[2], nchars, &text_putter->db_t_text[0]);
	    /* We're going to pretend that the text is a varchar(n)
	    ** where N = the actual length, plus null indicator - move the indicator
	    */
	    text_putter->db_t_text[nchars] = row_ptr->value[DB_MAX_DIST_VAL_LENGTH+2];
	    /* Bump up the text-putter, making it word-aligned,
	    ** and being careful to adjust text-left by the aligned
	    ** size!
	    */
	    text_len = (text_len + sizeof(u_i2)-1) & (~(sizeof(u_i2)-1));
	    text_putter = (DB_TEXT_STRING *) ((PTR) text_putter + text_len);
	    text_left = text_left - text_len;
	    dim_ptr->text_total_size = dim_ptr->text_total_size + text_len;

	    /* Keep track of the number of break entries per dimension, by
	    ** watching valseq.  (The number of entries is NOT the number
	    ** of rows read!)
	    */
	    if (dim_ptr->nbreaks < row_ptr->valseq)
		dim_ptr->nbreaks = row_ptr->valseq;

	    qef_db = qef_db->dt_next;
	} /* for rows */
	if (no_more_rows) break;
    } /* qget loop */

    /* No more rows */
    /* Would be nice to check that we got all the expected rows,
    ** without any holes or missing column values.  Could probably
    ** do this with the textp arrays if we zeroed them first and
    ** then went looking for null pointers.  Todo not-today...
    */
    status = rdu_qclose(global);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /* Post-process the breaks tables */
    status = rdp_finish_breaktables(global, part_def, errbuf);
    if (DB_FAILURE_MACRO(status))
    {
	rdp_inconsistent(global,"iidistval",errbuf);
	return (status);
    }

    return (E_DB_OK);
} /* rdp_read_distval */

/*
** Name: rdp_read_partname - read in iipartname rows
**
** Description:
**	This routine reads in the partition names for all the dimensions.
**
**	Each name row is labeled with its dimension (levelno) and the
**	partition within that dimension (partseq), so it's simple
**	enough to allocate space for all the names and read them in.
**
** Inputs:
**	global		RDF_GLOBAL * RDF request state area
**	part_def	DB_PART_DEF * pointer to partition definition
**	qef_area	char * buffer for reading rows
**	qef_size	i4 size of qef_area in bytes
**
** Outputs:
**	Returns DB_STATUS
**
** Side Effects:
**	QEF/DMF called, this routine can take locks and/or block
**
** History:
**	1-Jan-2004 (schka24)
**	    Written
*/

static DB_STATUS
rdp_read_partname(RDF_GLOBAL *global, DB_PART_DEF *part_def,
	char *qef_area, i4 qef_size)
{

    bool no_more_rows;			/* End of input indicator */
    DB_IIPARTNAME *row_ptr;		/* Pointer to catalog row */
    DB_NAME *name_area_base;		/* Start of all the names */
    DB_PART_DIM *dim_ptr;		/* Pointer to a dimension entry */
    DB_STATUS status;			/* Return status holder */
    DB_STATUS toss_status;		/* Ignore return value */
    i4 i;
    i4 nnames;				/* Number of names to read */
    i4 psize;				/* Size of name area */
    i4 rows_read;			/* Number of rows processed */
    QEF_DATA *qef_db;			/* Pointer to QEF row descriptor */

    /* Before reading the partition names, figure out how much space
    ** is needed, and allocate it.  Each name will go into a DB_NAME,
    ** and there are sum(dim_ptr->nparts) of them.
    */
    nnames = 0;
    for (dim_ptr = &part_def->dimension[part_def->ndims-1];
	 dim_ptr >= &part_def->dimension[0];
	 --dim_ptr)
	nnames = nnames + dim_ptr->nparts;

    psize = nnames * sizeof(DB_NAME);
    status = rdu_malloc(global, psize, (PTR *) &name_area_base);
    if (DB_FAILURE_MACRO(status))
	return (status);
    /* Space-fill just in case of size discrepancies with the catalog */
    MEfill(psize, ' ', name_area_base);

    /* Give each dimension its section of the name array */
    i = 0;
    for (dim_ptr = &part_def->dimension[0];
	 dim_ptr <= &part_def->dimension[part_def->ndims-1];
	 ++dim_ptr)
    {
	dim_ptr->partnames = &name_area_base[i];
	i = i + dim_ptr->nparts;
    }

    /* Now open and read the catalog */
    status = rdu_qopen(global, RD_IIPARTNAME, NULL,
			sizeof(DB_IIPARTNAME), qef_area, qef_size);
    if (DB_FAILURE_MACRO(status))
	return (status);
    rows_read = 0;
    for (;;)
    {
	status = rdu_qget(global, &no_more_rows);
	if (DB_FAILURE_MACRO(status))
	{
	    toss_status = rdu_qclose(global);
	    return (status);
	}
	qef_db = global->rdf_qeucb.qeu_output;
	for (i = 0; i < global->rdf_qeucb.qeu_count; ++i)
	{
	    ++ rows_read;
	    row_ptr = (DB_IIPARTNAME *) (qef_db->dt_data);
	    if (row_ptr->levelno <= 0 || row_ptr->levelno > part_def->ndims)
	    {
		rdp_inconsistent(global,"iipartname","invalid levelno");
		return (E_DB_ERROR);
	    }
	    dim_ptr = &part_def->dimension[row_ptr->levelno - 1];
	    if (row_ptr->partseq <= 0 || row_ptr->partseq > dim_ptr->nparts)
	    {
		rdp_inconsistent(global,"iipartname","invalid partseq");
		return (E_DB_ERROR);
	    }
	    MEcopy((void *) &row_ptr->partname, sizeof(DB_NAME),
		(void *) &dim_ptr->partnames[row_ptr->partseq-1]);
	    qef_db = qef_db->dt_next;
	}
	if (no_more_rows) break;
    } /* qget loop */

    /* No more rows, do final checks */
    if (rows_read != nnames)
    {
	rdp_inconsistent(global,"iipartname","wrong # of rows");
	return (E_DB_ERROR);
    }
    status = rdu_qclose(global);
    if (DB_FAILURE_MACRO(status))
	return (status);

    return (E_DB_OK);
} /* rdp_read_partname */

/* Name: rdp_sort_breaks - Sort a breaks table into proper order
**
** Description:
**
**	For runtime efficiency, as well as ease of validation, each
**	break values table is sorted into ascending value order.
**	If any two entries have the same value, they will be ordered
**	by the "oper" code, which will put LT/LE before GT/GE entries.
**
**	As an initial step, any entry with the operation DEFAULT is placed
**	at the very end.  There should only be one of these, but someone
**	else will check that.  The DEFAULT entry has a garbage value and
**	does not participate in sorting.
**
**	Obviously, the breaks table has to have its values converted
**	to internal format before this routine can run.
**
**	The world doesn't really need another embedded sorter, but
**	in this case the comparison needs enough context outside of the
**	items actually being sorted to make it inconvenient to use a
**	generic sort.
**
** Inputs:
**	global			RDF global request state
**	dim_ptr			A partition dimension info pointer
**	errbuf			Place to put ADF error messages (DB_ERR_SIZE)
**
** Outputs:
**	Returns DB_STATUS
**	errbuf			Holds any ADF message, in asciz
**
** History:
**	4-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
rdp_sort_breaks(RDF_GLOBAL *global, DB_PART_DIM *dim_ptr, char errbuf[])
{
    ADF_CB *adfcb;			/* Ptr to session ADF control block */
    char *save_aderrmsg;		/* Saved adf error buffer */
    DB_PART_BREAKS *breaksBase;		/* Base of breaks being sorted */
    DB_PART_BREAKS curBreak;		/* Current table entry */
    DB_PART_BREAKS *newPtr, *ptr;	/* Sorting pointers */
    DB_PART_BREAKS *stopAddr;		/* Where to stop this sort pass */
    DB_PART_LIST *part_list;		/* Array of component descriptors */
    DB_STATUS status;			/* The usual return status */
    i4 cmpResult;			/* Comparison result */
    i4 count;				/* Number of entries to sort */
    i4 increment;			/* Shell-sort increment */
    i4 j;
    i4 save_adebuflen;			/* Saved adf error buf length */
    const i4 *incrPtr;		/* Increments table pointer */

    /* Before starting, see if there's anything to sort... */
    count = dim_ptr->nbreaks;
    if (count <= 1)
	return (E_DB_OK);

    adfcb = (ADF_CB *) global->rdf_sess_cb->rds_adf_cb;
    /* Arrange to see formatted ADF error strings */
    save_aderrmsg = adfcb->adf_errcb.ad_errmsgp;
    save_adebuflen = adfcb->adf_errcb.ad_ebuflen;
    adfcb->adf_errcb.ad_errmsgp = &errbuf[0];
    adfcb->adf_errcb.ad_ebuflen = DB_ERR_SIZE;
    adfcb->adf_errcb.ad_emsglen = 0;

    breaksBase = dim_ptr->part_breaks;

    /* First, do the DEFAULT entry fudge:  it goes at the end and is
    ** not sorted with the rest.
    ** For the present, only LIST dimensions have default entries.
    */
    if (dim_ptr->distrule == DBDS_RULE_LIST) {
	stopAddr = &breaksBase[count-1];	/* Addr of last entry */
	ptr = breaksBase;
	do
	{
	    if (ptr->oper == DBDS_OP_DFLT) {
		/* Found it, stick it at the end */
		STRUCT_ASSIGN_MACRO(*stopAddr, curBreak);
		STRUCT_ASSIGN_MACRO(*ptr, *stopAddr);
		STRUCT_ASSIGN_MACRO(curBreak, *ptr);
		break;
	    }
	} while (++ptr < stopAddr);
	/* If we didn't find it, someone else will figure it out... */
	/* Sort the remaining n-1 entries */
	--count;
    }

    /* I'm rather partial to shellsorts myself.  This is a traditional
    ** Knuth shellsort with increments set so that H[x] is about count/3.
    ** In addition to being short, easy to code, and better than N-squared,
    ** shellsort (a.k.a dimishing-increment sort) is quite well-behaved
    ** for already sorted input.
    */

    /* Find the initial increment */
    incrPtr = &incrTable[0];
    while (count > *(incrPtr + 2))
	++ incrPtr;

    /* Do an insertion sort at each increment down to 1. */
    do
    {
	j = *incrPtr;
	increment = j;
	while (j < count)
	{
	    /* Pick up j'th entry, sort into position among the preceding
	    ** elements, only looking at each increment'th one.
	    */
	    ptr = &breaksBase[j++];
	    STRUCT_ASSIGN_MACRO(*ptr, curBreak);
	    stopAddr = &breaksBase[increment];
	    do
	    {
		newPtr = ptr - increment;
		/* "if (curBreak >= *newPtr) break" */
		status = adt_vvcmp(adfcb, dim_ptr->ncols, dim_ptr->part_list,
			curBreak.val_base, newPtr->val_base, &cmpResult);
		if (DB_FAILURE_MACRO(status)) goto error;
		if (cmpResult > 0) break;
		/* If values are equal, sort on opers */
		if (cmpResult == 0 && curBreak.oper >= newPtr->oper) break;
		/* Ok, curBreak goes before *newPtr, keep looking */
		STRUCT_ASSIGN_MACRO(*newPtr, *ptr);	/* Shuffle entry up */
		ptr = newPtr;			/* Back up to preceding position */
	    } while (ptr >= stopAddr);
	    STRUCT_ASSIGN_MACRO(curBreak, *ptr);  /* Install entry at proper position */
	}
    } while (--incrPtr >= &incrTable[0]);

    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
    return (E_DB_OK);

/* Error from the comparision, handled out-of-line to keep the inner
** loop nice and neat!
*/
error:
    /* Make sure null-terminated string... */
    if (adfcb->adf_errcb.ad_emsglen <= 0)
	STcopy("unknown ADF error",errbuf);
    else
	errbuf[adfcb->adf_errcb.ad_emsglen] = '\0';
    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
    return (status);
} /* rdp_sort_breaks */

/*
** Name: rdp_struct_compat - Check for structure and partitioning compatibility
**
** Description:
**	A "direct query plan" is one where the partitioning scheme is
**	essentially an extension of the master table storage structure;
**	the query plan can choose a partition from a predicate or row
**	value, and be assured that only that partition can contain the
**	row(s) with the given structure key values.
**
**	When a direct plan is not available, the query will have to
**	inspect possibly many partitions to find the qualifying rows.
**
**	This routine compares the partitioning scheme and the storage
**	structure, and if they are direct-plan-compatible, sets the
**	appropriate flag(s) in the partition definition.  The flags
**	involved are:
**
**	DB_PARTF_KEY_ONE_PART: set if rows with a given structure key
**	are guaranteed to all be in the same physical partition.
**	This is true if the overall partitioning column set, considering
**	all dimensions, is identical to or a subset of the key columns.
**	(Ordering doesn't matter.  If the key tells us all we need to
**	know at all dimensions, then the key uniquely identifies one
**	physical partition.)  Also, there may not be any AUTOMATIC
**	dimensions.
**
**	DB_PARTF_KEY_N_PARTS: set if one-part isn't true, but rows with
**	a given key can be found in some subset of partitions; and it's
**	reasonable to figure out which ones.
**	This is true if we can figure out a specific logical partition
**	at any dimension along the line.  Again, this means that the
**	partition column set must be a subset of the key columns, and
**	the dimension rule must not be AUTOMATIC..
**
**	There's one other case that could potentially be marked KEY_N_PARTS:
**	if the partitioning scheme is RANGE and the partitioning key is
**	an extension of the structure key (ie the structure key a left-
**	prefix of the partitioning key).  We don't bother with this case
**	because it implies that the storage structure is less selective
**	than the range partitioning!  which is surely not sensible or useful.
**
**	Another property that this routine could figure out, but doesn't,
**	is whether the partitioning scheme is similar to the storage
**	structure type (ie range and tree structures; list/hash and hash
**	structure).  At the moment, though, nobody cares.
**
**
** Inputs:
**	global			RDF request state area
**	storage_type		DMT_xxx_TYPE master structure type
**				(or DB_xxx_STORE, they are the same)
**				(notused for now)
**	nkeys			Number of storage structure key columns
**	key_array		Key column-number array
**	part_def		A completed partition definition area
**				with the relevant part_flags CLEAR
**
** Outputs:
**	none (other than setting the flags).  This routine does not
**	detect or issue any errors.
**
** History:
**	4-Jan-2004 (schka24)
**	    written at the end of the first RDF typing spree.
**	5-Aug-2004 (schka24)
**	    Revise significantly: it turns out that we care about key-in-
**	    partition-ness, but don't care about partition vs structure
**	    similarity.  Revise header doc to match.
*/

static void
rdp_struct_compat(RDF_GLOBAL *global, i4 storage_type, i4 nkeys,
	i4 *key_array, DB_PART_DEF *part_def)
{

    bool any_auto;			/* TRUE if any automatic dimensions */
    bool key_n_parts;			/* TRUE if we have key_n_parts */
    DB_COLUMN_BITMAP dim_map;		/* One dimension's columns */
    DB_COLUMN_BITMAP key_map;		/* Structure key columns */
    DB_COLUMN_BITMAP overall_map;	/* All partitioning columns */
    DB_PART_DIM *dim_ptr;		/* Partitioning dimension pointer */
    DB_PART_LIST *part_list_ptr;	/* Partition column list entry */
    i4 i;

    /* Since we don't care about column orderings, it makes sense to
    ** use bitmaps to check the columns we have.
    ** First, light the bits for the storage structure key columns.
    */

    DB_COLUMN_BITMAP_INIT(key_map.db_domset, i, 0);
    for (i = 0; i < nkeys; ++i)
    {
	BTset(key_array[i], (char *) &key_map);
    }

    /* Init the partitioning maps and flags. */

    DB_COLUMN_BITMAP_INIT(overall_map.db_domset, i, 0);
    any_auto = FALSE;
    key_n_parts = FALSE;

    /* Run through the partitioning dimensions.  For each dimension, see
    ** if the dimension allows "key_n_part"-ness, and include the dimension's
    ** columns in the overall bitmap.
    */

    for (dim_ptr = &part_def->dimension[0];
	 dim_ptr <= &part_def->dimension[part_def->ndims-1];
	 ++ dim_ptr)
    {
	if (dim_ptr->distrule == DBDS_RULE_AUTOMATIC)
	{
	    /* Auto dimensions negate the possibility of key-1-part-ness.
	    ** If we already have key-n-part-ness, we can stop here.
	    ** Otherwise just truck on to the next dimension, looking
	    ** for key-n-part-ness.
	    */
	    any_auto = TRUE;
	    if (key_n_parts) break;
	}
	else
	{
	    /* Build a map of this dimension's partitioning columns. */
	    DB_COLUMN_BITMAP_INIT(dim_map.db_domset, i, 0);
	    for (part_list_ptr = &dim_ptr->part_list[dim_ptr->ncols-1];
		 part_list_ptr >= &dim_ptr->part_list[0];
		 -- part_list_ptr)
	    {
		BTset(part_list_ptr->att_number, (char *) &dim_map);
	    }

	    /* If we don't already have key-n-part-ness, see if this
	    ** dimension supplies it.  NOTE: column numbers are 1-origin,
	    ** hence the +1.
	    */
	    if (! key_n_parts)
	    {
		key_n_parts = BTsubset((char *) &dim_map, (char *) &key_map,
				DB_MAX_COLS + 1);
	    }

	    /* Include this dimension's columns into the overall */
	    BTor(DB_MAX_COLS+1, (char *) &dim_map, (char *) &overall_map);
	}
    }

    /* If there's no AUTOMATIC dimensions, we might have key-1-part-ness */
    if (! any_auto)
    {
	if (BTsubset((char *) &overall_map, (char *) &key_map, DB_MAX_COLS+1))
	{
	    part_def->part_flags |= DB_PARTF_KEY_ONE_PART;
	    key_n_parts = FALSE;	/* Don't light this too */
	}
    }

    /* If we discovered key-n-part-ness, say so */
    if (key_n_parts)
	part_def->part_flags |= DB_PARTF_KEY_N_PARTS;

} /* rdp_struct_compat */

/*
** Name: rdp_text_to_internal - Convert textual break value to internal form
** 
** Description:
**	After reading in the break table value catalog iidistval, we
**	have a bunch of uninterpreted textual values.  The catalogs are
**	done this way partly to avoid the need for a faked-out union
**	type in the catalog, and partly so that utilities can echo back
**	the partition definition the way it was entered (including
**	dates, floats, other weird stuff).  Dates are a slight exception
**	to the rule, as they are converted into the universal
**	yyyy_mm_dd format to isolate them from the date-format setting.
**
**	Textual representations aren't going to do the runtime any
**	good, though, so we need to convert the text to an internal
**	form with ints, float, etc.  This routine is called to convert
**	a single (possibly multi-column) break table value entry
**	from text to internal form.  The internal value has been allocated
**	and laid out, all we need to do is invoke the proper
**	coercion routine.
**
**	Note that we are not concerned with datatype compatibility at
**	this point.  All that gets checked out at parse time.  If we
**	run into some kind of datatype conversion error, it's too bad.
**
** Inputs:
**	global		RDF request state block
**	dim_ptr		Pointer to dimension info area
**	break_ptr	Pointer to DB_PART_BREAKS for value to convert
**	errbuf		Where to put any low-level error strings (DB_ERR_SIZE)
**
** Outputs:
**	Returns DB_STATUS
**	errbuf		Holds asciz error message if any
**
** History:
**	4-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
rdp_text_to_internal(RDF_GLOBAL *global,
	DB_PART_DIM *dim_ptr, DB_PART_BREAKS *break_ptr, char errbuf[])
{

    ADF_CB *adfcb;			/* Ptr to session ADF control block */
    char *save_aderrmsg;		/* Saved adf error buffer */
    DB_DATA_VALUE result_dv;		/* Internal form result descriptor */
    DB_DATA_VALUE text_dv;		/* A longtext value descriptor */
    DB_PART_LIST *db_part_ptr;		/* Value column info */
    DB_STATUS status;			/* The usual return status */
    DB_TEXT_STRING **val_text;		/* Value-text array pointer */
    i4 abstype;				/* Result type w/o nullability */
    i4 i;
    i4 save_adebuflen;			/* Saved adf error buf length */

    adfcb = (ADF_CB *) global->rdf_sess_cb->rds_adf_cb;
    /* Arrange to see formatted ADF error strings */
    save_aderrmsg = adfcb->adf_errcb.ad_errmsgp;
    save_adebuflen = adfcb->adf_errcb.ad_ebuflen;
    adfcb->adf_errcb.ad_errmsgp = &errbuf[0];
    adfcb->adf_errcb.ad_ebuflen = DB_ERR_SIZE;
    adfcb->adf_errcb.ad_emsglen = 0;
    db_part_ptr = dim_ptr->part_list;
    val_text = break_ptr->val_text;	/* Array of text ptrs for value */
    text_dv.db_prec = 0;
    for (i = 0; i < dim_ptr->ncols; i++)
    {
	/* Set up the source descriptor as a nullable longtext.
	** The source is stored as a varchar, but longtext has the
	** same form, and according to an ADF comment, there are
	** coercions from longtext to anything...
	** The length allows for the count bytes and null.
	*/
	text_dv.db_datatype = -DB_LTXT_TYPE;
	text_dv.db_data = (PTR) *val_text;
	text_dv.db_length = (*val_text)->db_t_count + sizeof(u_i2) + 1;
	/* Set up the result descriptor from the db-part-list entry */
	result_dv.db_datatype = db_part_ptr->type;
	result_dv.db_data = break_ptr->val_base + db_part_ptr->val_offset;
	result_dv.db_length = db_part_ptr->length;
	result_dv.db_prec = db_part_ptr->precision;

	/* If the result is a number with a potential decimal point,
	** do a hack to allow for II_DECIMAL.  (This is almost the same hack
	** found in defaults processing in the parser.)  Basically
	** we convert the last , or . seen into the current decimal point
	** setting for the session.
	*/
	abstype = abs(result_dv.db_datatype);
	if (abstype == DB_FLT_TYPE || abstype == DB_DEC_TYPE
	    || abstype == DB_MNY_TYPE)
	{
	    u_char decpt;
	    u_char *cp = &(*val_text)->db_t_text[0];
	    i4 len = (*val_text)->db_t_count;

	    decpt = '.';
	    if (adfcb->adf_decimal.db_decspec) decpt = (u_char) adfcb->adf_decimal.db_decimal;
	    cp = cp + len - 1;			/* Move to the end */

	    /* This is going to be slow on double-byte but it doesn't
	    ** happen very often.
	    ** FIXME now that double-byte is default maybe do forwards.
	    */
	    while (--len >= 0)
	    {
		if (*cp == '.' || *cp == ',')
		{
		    *cp = decpt;
		    break;
		}
		CMprev(cp,&(*val_text)->db_t_text[0]);
	    }
	}

	/* Now do the actual coercion */
	status = adc_cvinto(adfcb, &text_dv, &result_dv);
	if (DB_FAILURE_MACRO(status))
	{
	    /* Make sure null-terminated string... */
	    if (adfcb->adf_errcb.ad_emsglen <= 0)
		STcopy("unknown ADF error",errbuf);
	    else
		errbuf[adfcb->adf_errcb.ad_emsglen] = '\0';
	    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
	    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
	    return (status);
	}

	/* Advance to next column, repeat */
	++ db_part_ptr;
	++ val_text;
    }
    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
    return (E_DB_OK);
} /* rdp_text_to_internal */

/*
** Name: rdp_val_size - Compute internal format value size for a dimension
**
** Description:
**	In a few places, we need to know how big the internal-format value
**	for one partitioning dimension is.  It's the val-offset plus length
**	for the last component, aligned to ALIGN_RESTRICT (i.e. DB_ALIGN_SZ).
**
**	This must only be called for RANGE or LIST dimensions, which are
**	the only ones that have breaks tables and their associated values.
**
**	(Actually, as things have evolved, there's only one place that
**	calls this any more, but I'll leave it split out for now.)
**
** Inputs:
**	dim_ptr		Partition dimension descriptor pointer
**
** Outputs:
**	Returns the aligned total length of the value area
**
** History:
**	9-jan-2004 (schka24)
**	    Written.
*/

static i4
rdp_val_size(DB_PART_DIM *dim_ptr)
{
    DB_PART_LIST *part_list_ptr;	/* Dimension column descriptions */
    i4 val_size;

    /* we always allocate offsets from start to end, so largest is at the end */
    part_list_ptr = &dim_ptr->part_list[dim_ptr->ncols-1];
    val_size = part_list_ptr->val_offset + part_list_ptr->length;
    /* Double-align the total length of a partitioning value */
    val_size = (val_size + (DB_ALIGN_SZ-1)) & (~(DB_ALIGN_SZ-1));

    return (val_size);
} /* rdp_val_size */

/*
** Name: rdp_validate_breaks - Validate a dimension's break table
**
** Description:
**	This routine performs various checks on a partition breaks
**	table to ensure that it's OK.  If something wrong is found,
**	a textual message is produced describing the error.
**	(The error message is returned to the caller, rather than
**	being issued to the user, as the caller may want to wrap
**	the message in more explanation.)  We use a message buffer,
**	rather than returning some sort of message ID code, because
**	some errors might be ADF errors, instead of validation errors.
**
**	Validation for LIST distributions and RANGE distributions are
**	rather different.
**	For LIST:
**		- The last entry must have the DEFAULT operator
**		- No other entries may be DEFAULT
**		- All other entries must be the = operator
**		- Non-DEFAULT entries must have distinct values
**
**	For RANGE:
**		- The first entry must be a < or <=
**		- The last entry must be a > or >=
**		  (implying that there has to be at least two entries)
**		- Any pair of entries that change comparison direction
**		  must share the same break value
**		- Any pair of entries with the same break value must have
**		  complementary comparisons (i.e. < with >=, >= with <,
**		  and so on).
**		- All entries must be <, <=, >, or >=.
**	The intent of the range rules is that the value domain must be
**	completely covered (the first two rules), and there must not
**	be any gaps or overlaps (the second two rules).  The last rule
**	mainly verifies that nobody has been fiddling with the operator
**	codes in the catalogs!
**
**	This routine is to be called after the breaks table has been
**	sorted into order, via rdp-sort-breaks.  It is only to be called
**	for LIST or RANGE distributions (they are the only ones with
**	a breaks value table).
**
** Inputs:
**	global			RDF request state block
**	dim_ptr			Pointer to one dimension description
**	errbuf			Where to put any error string
**
** Outputs:
**	Returns the usual DB_STATUS
**	errbuf			If error, asciz descriptive string here
**
** Side Effects:
**	none
**
** History:
**	4-Jan-2004 (schka24)
**	    Written.
*/

static DB_STATUS
rdp_validate_breaks(RDF_GLOBAL *global, DB_PART_DIM *dim_ptr, char errbuf[])
{


    ADF_CB *adfcb;			/* Ptr to session ADF control block */
    CL_ERR_DESC tosscl;			/* Ignored status holder */
    char *save_aderrmsg;		/* Saved adf error buffer */
    DB_PART_BREAKS *break_ptr;		/* Ptr to a breaks table entry */
    DB_PART_BREAKS *prev_break;		/* Previous breaks table entry ptr */
    DB_PART_LIST *part_list;		/* Array of component descriptors */
    DB_STATUS status;			/* The usual return status */
    i4 cmpResult;			/* Comparison result */
    i4 count;				/* Number of entries to sort */
    i4 errcode;				/* Message code if any */
    i4 errlen;				/* Returned error string length */
    i4 oper;				/* An entry's operator code */
    i4 save_adebuflen;			/* Saved adf error buf length */

    adfcb = (ADF_CB *) global->rdf_sess_cb->rds_adf_cb;
    /* Arrange to see formatted ADF error strings */
    save_aderrmsg = adfcb->adf_errcb.ad_errmsgp;
    save_adebuflen = adfcb->adf_errcb.ad_ebuflen;
    adfcb->adf_errcb.ad_errmsgp = &errbuf[0];
    adfcb->adf_errcb.ad_ebuflen = DB_ERR_SIZE;
    adfcb->adf_errcb.ad_emsglen = 0;

    errcode = 0;			/* Start out OK */

    if (dim_ptr->distrule == DBDS_RULE_LIST)
    {
	/* Do the LIST checks */

	/* Just something to break out of */
	do
	{
	    /* Last entry must be DEFAULT */
	    break_ptr = &dim_ptr->part_breaks[dim_ptr->nbreaks-1];
	    if (break_ptr->oper != DBDS_OP_DFLT)
	    {
		errcode = S_RDF000_LIST_DEFAULT;
		break;
	    }
	    /* No other DEFAULTS;  no duplicates */
	    while (--break_ptr >= &dim_ptr->part_breaks[0])
	    {
		if (break_ptr->oper == DBDS_OP_DFLT)
		{
		    errcode = S_RDF001_ONE_DEFAULT;
		    break;
		}
		if (break_ptr->oper != DBDS_OP_EQ)
		{
		    errcode = S_RDF002_LIST_EQ;
		    break;
		}
		if (break_ptr > &dim_ptr->part_breaks[0])
		{
		    prev_break = break_ptr - 1;
		    status = adt_vvcmp(adfcb, dim_ptr->ncols, dim_ptr->part_list,
			break_ptr->val_base, prev_break->val_base, &cmpResult);
		    if (DB_FAILURE_MACRO(status)) goto error;
		    if (cmpResult == 0)
		    {
			errcode = S_RDF003_LIST_UNIQUE;
			break;
		    }
		}
		if (errcode != 0) break;	/* Got something, quit */
	    } /* while */
	} while (0); /* fake loop */
    }
    else
    {
	/* Must be RANGE */

	/* By the way, note that rdp_sort_breaks has arranged so that
	** entries with identical partition values sort in oper order,
	** so that < and <= precede > and >=.
	*/

	/* Just something to break out of */
	do
	{
	    /* First entry must be < or <= */
	    oper = dim_ptr->part_breaks->oper;
	    if (oper != DBDS_OP_LT && oper != DBDS_OP_LTEQ)
	    {
		errcode = S_RDF004_RNG_FIRST_LT;
		break;
	    }
	    /* Last entry must be > or >= */
	    break_ptr = &dim_ptr->part_breaks[dim_ptr->nbreaks-1];
	    if (break_ptr->oper != DBDS_OP_GT && break_ptr->oper != DBDS_OP_GTEQ)
	    {
		errcode = S_RDF005_RNG_LAST_GT;
		break;
	    }
	    while (break_ptr > &dim_ptr->part_breaks[0])
	    {
		/* Check for a valid operator */
		oper = break_ptr->oper;
		if (oper != DBDS_OP_LT && oper != DBDS_OP_LTEQ
		  && oper != DBDS_OP_GT && oper != DBDS_OP_GTEQ)
		{
		    errcode = S_RDF006_RANGE_OPER;
		    break;
		}
		/* See if this entry and previous one are equal */
		prev_break = break_ptr - 1;
		status = adt_vvcmp(adfcb, dim_ptr->ncols, dim_ptr->part_list,
			break_ptr->val_base, prev_break->val_base, &cmpResult);
		if (DB_FAILURE_MACRO(status)) goto error;
		if (cmpResult == 0)
		{
		    /* Entries are equal, operators must be complementary */
		    /* oper code XOR 4 gives the complementary code */
		    if ((oper ^ 4) != prev_break->oper)
		    {
			errcode = S_RDF007_RANGE_GAP_OVERLAP;
			break;
		    }
		}
		else
		{
		    /* Entries are not equal, operators must be in the
		    ** same "direction".  Trimming away the 1 bit makes
		    ** LT/LTEQ and GT/GTEQ the same.
		    */
		    if ((oper & (~1)) != (prev_break->oper & (~1)))
		    {
			errcode = S_RDF007_RANGE_GAP_OVERLAP;
			break;
		    }
		}

		/* Done checking this pair */
		if (errcode != 0) break;	/* Give up if error */
		-- break_ptr;
	    } /* while */
	} while (0); /* fake loop */
    } /* if */

    if (errcode != 0)
    {
	/* We got a problem, retrieve the message string.
	** ule_format is inappropriate because it insists on sticking
	** a timestamp and other formatting poop into the message.
	*/
	status = ERslookup(errcode, NULL, ER_TEXTONLY, NULL,
			errbuf, DB_ERR_SIZE, 1, &errlen, &tosscl, 0, NULL);
	if (status != E_DB_OK)
	    STprintf(errbuf, "partition validation error %d (failed ERslookup)",
			errcode);
	errbuf[errlen] = '\0';
	return (E_DB_ERROR);
    }
    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
    return (E_DB_OK);

/* Error from ADF, handled out-of-line to keep the inner
** loops semi-readable.
*/
error:
    /* Make sure null-terminated string... */
    if (adfcb->adf_errcb.ad_emsglen <= 0)
	STcopy("unknown ADF error",errbuf);
    else
	errbuf[adfcb->adf_errcb.ad_emsglen] = '\0';
    adfcb->adf_errcb.ad_errmsgp = save_aderrmsg;
    adfcb->adf_errcb.ad_ebuflen = save_adebuflen;
    return (status);
} /* rdp_validate_breaks */

/*
** Name: rdp_write_distcol -- Write iidistcol distribution column rows.
**
** Description:
**	This routine updates the iidistcol catalog with the new partition
**	definition.  It does do by brute force:  any old iidistcol rows for
**	the partition master table are deleted, and new ones are inserted.
**
**	Catalog rows are deleted and inserted using basic qeu functions.
**	The caller has partially prepared the QEU_CB in the rdf request state
**	block, we just need to include the catalog ID.
**
** **** FIXME crappy errmsgs, fix later (all of rdf...)
**
** Inputs:
**	global			RDF request state area
**	    ->rdf_qeucb		QEF utility request block, partially completed
**	part_def		Pointer to new DB_PART_DEF definition
**
** Outputs:
**	Returns standard DB_STATUS
**
** Side Effects:
**	Calls QEF/DMF, so will take locks, may block
**
** History:
**	12-Jan-2004 (schka24)
**	    Pump out the update side of things.
*/

static DB_STATUS
rdp_write_distcol(RDF_GLOBAL *global, DB_PART_DEF *part_def)
{

    DB_IIDISTCOL new_row;		/* Generated row to write */
    DB_PART_DIM *dim_ptr;		/* Partition dimension pointer */
    DB_STATUS status;			/* Standard routine status */
    i4 col;				/* Column sequence index */
    i4 dim;				/* Dimension index */
    QEF_DATA qedata;			/* QEF data descriptor */

    /* Finish a few things in the QEU request block */
    global->rdf_qeucb.qeu_tab_id.db_tab_base = DM_B_DISTCOL_TAB_ID;
    global->rdf_qeucb.qeu_tab_id.db_tab_index = DM_I_DISTCOL_TAB_ID;
    global->rdf_keys[0].attr_number = DM_1_DISTCOL_KEY;
    global->rdf_keys[1].attr_number = DM_2_DISTCOL_KEY;
    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIDISTCOL);
    global->rdf_qeucb.qeu_acc_id = NULL;
    global->rdf_qeucb.qeu_flag = QEU_SHOW_STAT | QEU_BYPASS_PRIV;
    status = qef_call(QEU_OPEN, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0077_QEU_OPEN,0);
	return (status);
    }
    /* Track open status so magic RDF error handling helps us */
    global->rdf_resources |= RDF_RQEU;

    /* Delete existing rows.  qeu-klen is set to delete all rows that match
    ** the master table ID.
    */
    global->rdf_qeucb.qeu_flag = 0;
    status = qef_call(QEU_DELETE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD007A_QEU_DELETE,0);
	return (status);
    }

    /* Format and append the new rows (if there are any!) */
    for (dim=0; dim < part_def->ndims; dim++)
    {
	dim_ptr = &part_def->dimension[dim];
	for (col = 0; col < dim_ptr->ncols; col++)
	{
	    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tabid, new_row.master_tabid);
	    new_row.levelno = dim+1;
	    new_row.colseq = col+1;
	    new_row.attid = dim_ptr->part_list[col].att_number;
    
	    qedata.dt_data = (PTR) &new_row;
	    qedata.dt_size = sizeof(DB_IIDISTCOL);
	    qedata.dt_next = NULL;
	    global->rdf_qeucb.qeu_input = &qedata;
	    global->rdf_qeucb.qeu_count = 1;
	    status = qef_call(QEU_APPEND, &global->rdf_qeucb);
	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror(global, status, &global->rdf_qeucb.error,
			    E_RD007B_QEU_APPEND,0);
		return (status);
	    }
	}
    }

    /* Done with this catalog, close QEF */
    status = qef_call(QEU_CLOSE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0079_QEU_CLOSE, 0);
    global->rdf_resources &= ~(RDF_RQEU);
    return (status);

} /* rdp_write_distcol */

/*
** Name: rdp_write_distscheme -- Write iidistscheme distribution scheme rows.
**
** Description:
**	This routine updates the iidistscheme catalog with the new partition
**	definition.  It does do by brute force:  any old iidistscheme rows for
**	the partition master table are deleted, and new ones are inserted.
**
**	Catalog rows are deleted and inserted using basic qeu functions.
**	The caller has partially prepared the QEU_CB in the rdf request state
**	block, we just need to include the catalog ID.
**
** **** FIXME crappy errmsgs, fix later (all of rdf...)
**
** Inputs:
**	global			RDF request state area
**	    ->rdf_qeucb		QEF utility request block, partially completed
**	part_def		Pointer to new DB_PART_DEF definition
**
** Outputs:
**	Returns standard DB_STATUS
**
** Side Effects:
**	Calls QEF/DMF, so will take locks, may block
**
** History:
**	12-Jan-2004 (schka24)
**	    Pump out the update side of things.
*/

static DB_STATUS
rdp_write_distscheme(RDF_GLOBAL *global, DB_PART_DEF *part_def)
{

    DB_IIDISTSCHEME new_row;		/* Generated row to write */
    DB_PART_DIM *dim_ptr;		/* Partition dimension pointer */
    DB_STATUS status;			/* Standard routine status */
    i4 dim;				/* Dimension index */
    QEF_DATA qedata;			/* QEF data descriptor */

    /* Finish a few things in the QEU request block */
    global->rdf_qeucb.qeu_tab_id.db_tab_base = DM_B_DSCHEME_TAB_ID;
    global->rdf_qeucb.qeu_tab_id.db_tab_index = DM_I_DSCHEME_TAB_ID;
    global->rdf_keys[0].attr_number = DM_1_DSCHEME_KEY;
    global->rdf_keys[1].attr_number = DM_2_DSCHEME_KEY;
    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIDISTSCHEME);
    global->rdf_qeucb.qeu_acc_id = NULL;
    global->rdf_qeucb.qeu_flag = QEU_SHOW_STAT | QEU_BYPASS_PRIV;
    status = qef_call(QEU_OPEN, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0077_QEU_OPEN,0);
	return (status);
    }
    /* Track open status so magic RDF error handling helps us */
    global->rdf_resources |= RDF_RQEU;

    /* Delete existing rows.  qeu-klen is set to delete all rows that match
    ** the master table ID.
    */
    global->rdf_qeucb.qeu_flag = 0;
    status = qef_call(QEU_DELETE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD007A_QEU_DELETE,0);
	return (status);
    }

    /* Format and append the new rows (if there are any!) */
    for (dim=0; dim < part_def->ndims; dim++)
    {
	dim_ptr = &part_def->dimension[dim];
	STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tabid, new_row.master_tabid);
	new_row.levelno = dim+1;
	new_row.ncols = dim_ptr->ncols;
	new_row.nparts = dim_ptr->nparts;
	new_row.distrule = dim_ptr->distrule;

	qedata.dt_data = (PTR) &new_row;
	qedata.dt_size = sizeof(DB_IIDISTSCHEME);
	qedata.dt_next = NULL;
	global->rdf_qeucb.qeu_input = &qedata;
	global->rdf_qeucb.qeu_count = 1;
	status = qef_call(QEU_APPEND, &global->rdf_qeucb);
	if (DB_FAILURE_MACRO(status))
	{
	    rdu_ferror(global, status, &global->rdf_qeucb.error,
			E_RD007B_QEU_APPEND,0);
	    return (status);
	}
    }

    /* Done with this catalog, close QEF */
    status = qef_call(QEU_CLOSE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0079_QEU_CLOSE, 0);
    global->rdf_resources &= ~(RDF_RQEU);
    return (status);

} /* rdp_write_distscheme */

/*
** Name: rdp_write_distval -- Write iidistval distribution break value rows.
**
** Description:
**	This routine updates the iidistval catalog with the new partition
**	definition.  It does do by brute force:  any old iidistval rows for
**	the partition master table are deleted, and new ones are inserted.
**
**	Catalog rows are deleted and inserted using basic qeu functions.
**	The caller has partially prepared the QEU_CB in the rdf request state
**	block, we just need to include the catalog ID.
**
**	The iidistval catalog holds uninterpreted value text, not internal
**	format data.  For this to work, the new partition definition has to
**	include the value texts.  We're not going to attempt to coerce internal
**	form break values to character strings, nor should there be need to.
**
** **** FIXME crappy errmsgs, fix later (all of rdf...)
**
** Inputs:
**	global			RDF request state area
**	    ->rdf_qeucb		QEF utility request block, partially completed
**	part_def		Pointer to new DB_PART_DEF definition
**
** Outputs:
**	Returns standard DB_STATUS
**
** Side Effects:
**	Calls QEF/DMF, so will take locks, may block
**
** History:
**	12-Jan-2004 (schka24)
**	    Pump out the update side of things.
*/

static DB_STATUS
rdp_write_distval(RDF_GLOBAL *global, DB_PART_DEF *part_def)
{

    DB_IIDISTVAL new_row;		/* Generated row to write */
    DB_PART_BREAKS *break_ptr;		/* Pointer to breaks table entry */
    DB_PART_DIM *dim_ptr;		/* Pointer to partition dimension */
    DB_STATUS status;			/* Standard routine status */
    DB_TEXT_STRING **text_pp;		/* Pointer to text pointer array */
    DB_TEXT_STRING *val_text;		/* Pointer to a text string */
    i4 col;				/* Column sequence index */
    i4 dim;				/* Dimension index */
    i4 val;				/* Break value index */
    QEF_DATA qedata;			/* QEF data descriptor */

    /* Finish a few things in the QEU request block */
    global->rdf_qeucb.qeu_tab_id.db_tab_base = DM_B_DISTVAL_TAB_ID;
    global->rdf_qeucb.qeu_tab_id.db_tab_index = DM_I_DISTVAL_TAB_ID;
    global->rdf_keys[0].attr_number = DM_1_DISTVAL_KEY;
    global->rdf_keys[1].attr_number = DM_2_DISTVAL_KEY;
    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIDISTVAL);
    global->rdf_qeucb.qeu_acc_id = NULL;
    global->rdf_qeucb.qeu_flag = QEU_SHOW_STAT | QEU_BYPASS_PRIV;
    status = qef_call(QEU_OPEN, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0077_QEU_OPEN,0);
	return (status);
    }
    /* Track open status so magic RDF error handling helps us */
    global->rdf_resources |= RDF_RQEU;

    /* Delete existing rows.  qeu-klen is set to delete all rows that match
    ** the master table ID.
    */
    global->rdf_qeucb.qeu_flag = 0;
    status = qef_call(QEU_DELETE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD007A_QEU_DELETE,0);
	return (status);
    }

    /* Format and append the new rows (if there are any!) */
    for (dim=0; dim < part_def->ndims; dim++)
    {
	dim_ptr = &part_def->dimension[dim];
	/* Skip this dimension if it doesn't use a breaks table */
	if (dim_ptr->distrule != DBDS_RULE_LIST && dim_ptr->distrule != DBDS_RULE_RANGE)
	    continue;

	/* Go through the values, for each value go through the columns.  This is
	** the row key order.  It doesn't really matter, since DMF will do the right
	** thing anyway, but perhaps this will minimize its work.
	*/
	for (val = 0; val < dim_ptr->nbreaks; val++)
	{
	    break_ptr = &dim_ptr->part_breaks[val];
	    text_pp = break_ptr->val_text;
	    /* Texts can be left out of partition definitions, but not this one. */
	    if (text_pp == NULL)
	    {
		rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
		return (E_DB_ERROR);
	    }
	    for (col = 0; col < dim_ptr->ncols; col++)
	    {
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tabid, new_row.master_tabid);
		new_row.levelno = dim+1;
		new_row.colseq = col+1;
		new_row.valseq = val+1;
		new_row.partseq = break_ptr->partseq;
		new_row.oper = break_ptr->oper;
		/* Assign the value */
		val_text = *text_pp++;
		if (val_text->db_t_count > DB_MAX_DIST_VAL_LENGTH)
		{
		    rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
		    return (E_DB_ERROR);
		}
		/* Value string, with null indicator at the end */
		MEcopy(val_text, val_text->db_t_count + sizeof(u_i2),
			&new_row.value[0]);
		new_row.value[DB_MAX_DIST_VAL_LENGTH+2] =
			val_text->db_t_text[val_text->db_t_count];
	
		qedata.dt_data = (PTR) &new_row;
		qedata.dt_size = sizeof(DB_IIDISTVAL);
		qedata.dt_next = NULL;
		global->rdf_qeucb.qeu_input = &qedata;
		global->rdf_qeucb.qeu_count = 1;
		status = qef_call(QEU_APPEND, &global->rdf_qeucb);
		if (DB_FAILURE_MACRO(status))
		{
		    rdu_ferror(global, status, &global->rdf_qeucb.error,
				E_RD007B_QEU_APPEND,0);
		    return (status);
		}
	    } /* columns in a value */
	} /* values */
    } /* dimensions */

    /* Done with this catalog, close QEF */
    status = qef_call(QEU_CLOSE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0079_QEU_CLOSE, 0);
    global->rdf_resources &= ~(RDF_RQEU);
    return (status);

} /* rdp_write_distval */

/*
** Name: rdp_write_partname -- Write iipartname partition name rows.
**
** Description:
**	This routine updates the iipartname catalog with the new partition
**	definition.  It does do by brute force:  any old iipartname rows for
**	the partition master table are deleted, and new ones are inserted.
**
**	Catalog rows are deleted and inserted using basic qeu functions.
**	The caller has partially prepared the QEU_CB in the rdf request state
**	block, we just need to include the catalog ID.
**
** **** FIXME crappy errmsgs, fix later (all of rdf...)
**
** Inputs:
**	global			RDF request state area
**	    ->rdf_qeucb		QEF utility request block, partially completed
**	part_def		Pointer to new DB_PART_DEF definition
**
** Outputs:
**	Returns standard DB_STATUS
**
** Side Effects:
**	Calls QEF/DMF, so will take locks, may block
**
** History:
**	12-Jan-2004 (schka24)
**	    Pump out the update side of things.
*/

static DB_STATUS
rdp_write_partname(RDF_GLOBAL *global, DB_PART_DEF *part_def)
{

    DB_IIPARTNAME new_row;		/* Generated row to write */
    DB_PART_DIM *dim_ptr;		/* Partition dimension pointer */
    DB_STATUS status;			/* Standard routine status */
    i4 dim;				/* Dimension index */
    i4 seq;				/* Partition name index */
    QEF_DATA qedata;			/* QEF data descriptor */

    /* Finish a few things in the QEU request block */
    global->rdf_qeucb.qeu_tab_id.db_tab_base = DM_B_PNAME_TAB_ID;
    global->rdf_qeucb.qeu_tab_id.db_tab_index = DM_I_PNAME_TAB_ID;
    global->rdf_keys[0].attr_number = DM_1_PNAME_KEY;
    global->rdf_keys[1].attr_number = DM_2_PNAME_KEY;
    global->rdf_qeucb.qeu_tup_length = sizeof(DB_IIPARTNAME);
    global->rdf_qeucb.qeu_acc_id = NULL;
    global->rdf_qeucb.qeu_flag = QEU_SHOW_STAT | QEU_BYPASS_PRIV;
    status = qef_call(QEU_OPEN, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0077_QEU_OPEN,0);
	return (status);
    }
    /* Track open status so magic RDF error handling helps us */
    global->rdf_resources |= RDF_RQEU;

    /* Delete existing rows.  qeu-klen is set to delete all rows that match
    ** the master table ID.
    */
    global->rdf_qeucb.qeu_flag = 0;
    status = qef_call(QEU_DELETE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD007A_QEU_DELETE,0);
	return (status);
    }

    /* Format and append the new rows (if there are any!) */
    for (dim=0; dim < part_def->ndims; dim++)
    {
	dim_ptr = &part_def->dimension[dim];
	/* Names can be left out of partition definitions, but not this one. */
	if (dim_ptr->partnames == NULL)
	{
	    rdu_ierror(global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
	    return (E_DB_ERROR);
	}
	for (seq = 0; seq < dim_ptr->nparts; seq++)
	{
	    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_rb.rdr_tabid, new_row.master_tabid);
	    new_row.levelno = dim+1;
	    new_row.partseq = seq+1;
	    MEcopy(&dim_ptr->partnames[seq], sizeof(DB_NAME), &new_row.partname);
    
	    qedata.dt_data = (PTR) &new_row;
	    qedata.dt_size = sizeof(DB_IIPARTNAME);
	    qedata.dt_next = NULL;
	    global->rdf_qeucb.qeu_input = &qedata;
	    global->rdf_qeucb.qeu_count = 1;
	    status = qef_call(QEU_APPEND, &global->rdf_qeucb);
	    if (DB_FAILURE_MACRO(status))
	    {
		rdu_ferror(global, status, &global->rdf_qeucb.error,
			    E_RD007B_QEU_APPEND,0);
		return (status);
	    }
	}
    }

    /* Done with this catalog, close QEF */
    status = qef_call(QEU_CLOSE, &global->rdf_qeucb);
    if (DB_FAILURE_MACRO(status))
	rdu_ferror(global, status, &global->rdf_qeucb.error,
		E_RD0079_QEU_CLOSE, 0);
    global->rdf_resources &= ~(RDF_RQEU);
    return (status);

} /* rdp_write_partname */
