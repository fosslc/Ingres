/*
**Copyright (c) 2010 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <tm.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <copy.h>
#include    <qsf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <dmacb.h>
#include    <gwf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <qeuqcb.h>
#include    <qefscb.h>
#include    <rdf.h>
#include    <tm.h>
#include    <usererror.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>

/*
** NO_OPTIM = rs4_us5 ris_u64 i64_aix
*/

/**
**
**  Name: QEUS.C - set utility functions for QEF
**
**  Description:
**      These are the external entry points for functions that
**  provide query-like and transaction dependent operations.
**  These functions don't fit into the other QEF catagories
**  (transaction, control or query). 
**
**          qeu_dbu     - perform a DBU operation
**          qeu_query   - run a static query
**	    qeu_create_table - Create a user table
**
**
**
**  History:
**      30-apr-86 (daved)    
**          written
**	19-jan-87 (rogerk)
**	    Wrote copy routines.
**	14-mar-87 (daved)
**	    make transaction readonly for copy from and
**	    for update if copy into
**	17-apr-86 (rogerk)
**	    In qeu_r_copy, don't error if all rows aren't appended, as some
**	    may have been duplicates.
**	17-apr-86 (rogerk)
**	    Use qet_begin in qeu_b_copy instead of qeu_begin.  We want to
**	    start a user transaction rather than an internal transaction.
**	19-nov-87 (puree)
**	    Set savepoint pointer in qeu_b_copy before using it.
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc
**	06-jan-88 (puree)
**	    modify qeu_dbu for special transaction state and error handling
**	    for PSY.
**	10-feb-88 (puree)
**	    Converted ulm_openstream to qec_mopen.  Rename qec_palloc to 
**	    qec_malloc.
**	11-feb-88 (puree)
**	    Modify qeu_dbu to put an exclusive lock on the table to be
**	    destroyed.
**	17-feb-88 (puree)
**	    Fix bug setting DMF error code in qeu_dbu.  Also, check for
**	    error openning a view.
**	24-mar-88 (puree)
**	    Add code to validate copy table in qeu_b_copy.  Return
**	    E_QE0023_INVALID_QUERY if table is not found or if the timestamp
**	    does not match.
**	08-aug-88 (puree)
**	    Modify qeu_b_copy and qeu_dbu to recognize E_DM0114_FILE_NOT_FOUND.
**	11-aug-88 (puree)
**	    Clean up lint errors
**	29-aug-88 (puree)
**	    Clean up qeu_r_copy
**	05-oct-88 (puree)
**	    modified qeu_dbu due to change in user error 5102 format by PSF.
**	24-oct-88 (puree)
**	    fix string length for message 5102.
**	09-nov-88 (puree)
**	    modify qeu_dbu for reporting invalid location name.
**	21-nov-88 (puree)
**	    modify qeu_dbu for setting user error in forced abort.
**	25-dec-88 (carl)
**	    Modified for Titan transaction considerations
**	07-mar-89 (neil)
**	    Added support to drop rule applied to a table being dropped.
**	13-mar-89 (carl)
**	    Added code to commit CDB association to avoid deadlocks
**	01-apr-89 (carl)
**	    Modified qeu_dbu to process REGISTER WITH REFRESH
**	19-apr-89 (paul)
**	    Changed qeq_query calling sequence for handling cursors.
**	    Must update qeq_query call in this module.
**      26-jul-89 (neil)
**          One more update to qeq_query call (qef_intstate).
**      01-aug-89 (jennifer)
**          Added code to audit copy operations.
**	08-nov-89 (neil)
**	    Clear flags for qeu_dprot in qeu_dbu.
**	04-may-90 (carol)
**	    Add cascading drops for comments.
**      16-may-90 (jennifer)
**          Add code to support empty table option.
**	10-aug-90 (carl)
**	    removed all references to IIQE_c29_ddl_info
**	05-oct-90 (davebf)
**	    Don't close all cursors for error in copy begin
**	19-nov-90 (rachael)
**	    Bugfix for 8498. Added check for caller == QEF_INTERNAL after the
**	    dmf_call in qeu_dbu.  The qeu_cb->qeu_eflag was not being set by 
**	    qea_dmu.
**	10-dec-90 (neil)
**	    Alerters: Clear flags for qeu_dprot in qeu_dbu.
**	02-feb-90 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	12-jun-91 (rickh)
**	    Set dmu_gw_id for DESTROY operation.  Random noise on the stack
**	    isn't good enough.
**	16-oct-91 (bryanp)
**	    Part of 6.4 -> 6.5 merge changes. Must initialize new dmt_char_array
**	    field for DMT_SHOW calls.
**	08-Jan-1992 (rmuth)
**	    Before we do a fast-load we perform some checks to see if there
**	    is any data in the table. One of the checks is to make sure there
**	    is less than 16 pages in the table as we do not want to scan a
**	    large table to see if there is any data. The file allocation
**	    project added atleast two new pages to each table, a FHDR and one
**	    or more FMAPS depending on the size of the table. These need to
**	    be taken into account in the above check otherwise the code
**	    will reject a fast-load into an empty default hash table, which 
**	    has 18 pages. For all other table structures the number of pages
**	    is < 18 so increase the check size to 18 pages.
**      20-apr-92 (schang)
**          fix GW bug 42825
**	15-jun-92 (davebf)
**	    Sybil Merge
**	20-nov-92 (rickh)
**	    New argument for qeu_dbu.  qeu_dbu now exposes the ID of the
**	    table it creates.
**      08-dec-92 (anitap)
**          Added #include <psfparse.h> for CREATE SCHEMA.
**	17-feb-93 (rickh)
**	    When modifying a base table, be sure you recreate the persistent
**	    indices.  Also, when creating a table, make sure you populate
**	    IIINTEGRITY as necessary.
**	12-mar-93 (rickh)
**	    Made qeu_tableOpen and qeu_findOrMakeDefaultID external.
**	22-mar-93 (rickh)
**	    System generated indices (e.g., those generated on behalf of
**	    constraints) should not report a row count.
**	30-mar-93 (rickh)
**	    Zero out some more qeu_cb fields in qeu_updateDependentIDs.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	12-apr-93 (rickh)
**	    DMU_CB list in createIndices is a linked list, not an array.
**	23-apr-93 (anitap)
**	    Added support in qeu_dbu() for the creation of implicit schemas.
**	26-apr-1993 (rmuth)
**	    COPY with ROW_ESTIMATE = n, place the estimated number of rows
**	    into the dmr_cb.
**	27-apr-93 (anitap)
**	    Removed changes for implicit schema. Will integrate after more
**	    testing. 
**      08-jun-93 (rblumer)
**          added code in qeu_query for new flag from PSF: QEU_WANT_USER_XACT.
**          Also added externs and removed unused variables to get rid of
**          compiler warnings. 
**	9-jun-93 (rickh)
**	    Added some return arguments to qeu_dbu.  The point was to
**	    isolate the QEU_CB as a read-only argument.  This in turn was
**	    done because sometimes the QEU_CB may have been compiled into
**	    code space by OPC.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (anitap)
**	    Added extra argument to qea_schema() in qeu_dbu().
**	    Do not create implict schema for temp tables in qeu_dbu().
**	    Change assignment of flag to IMPSCH_FOR_TABLE.
**	    Added arguments to qea_reserveID() in qeu_findOrMakeDefaultID().
**	26-jul-1993 (rmuth)
**	    Enhance the COPY command to allow the following with clauses,
**	    ( FILLFACTOR, LEAFFILL, NONLEAFFILL, ALLOCATION, EXTEND, 
**	    MINPAGES and MAXPAGES). 
**	    Add <usererror.h>
**	3-aug-93 (rickh)
**	    When creating a unique index on behalf of a UNIQUE CONSTRAINT, emit
**	    a CONSTRAINT error if duplicate rows prevent you from creating
**	    the index.
**	19-aug-93 (anitap)
**	    Initialized qef_cb->qef_dbid in qeu_findOrMakeDefaultID().
**      14-sep-93 (smc)
**          Fixed up bad casts of dmu_table_name & qeu_d_id.
**	13-sep-93 (rblumer)
**	    in qeu_query, remove code for obsolete QEU_WANT_USER_XACT flag.
**          Fixed some compiler warnings due to argument mismatches
**          (dereference dmu_table_name, rather than using address of struct).
**	21-sep-93 (robf)
**          Assigned "err" in more places on error, caused spurious
**	    E_QE0018_ILLEGAL_CB errors when unset.
**	6-oct-93 (stephenb)
**	    Set tablename in qeuq_cb in qeu_dbu() so that we can audit 
**	    drop temp table.
**      17-Sep-1993 (fred)
**          Remove unconditional setting of dmu_flags_mask in qeu_dbu().
**      30-sep-93 (anitap)
**          Create implicit schema for journaled tables. Fix for bug 53525.
**          Changes in qeu_dbu(). Also fixed compiler warning.
**	17-jan-94 (rickh)
**	    Make sure errors from recreation of persistent indices are stuffed
**	    into the correct error variable.
**	24-jan-1994 (gmanning)
**	    Change variable errno to err_num because of macro conflict under NT.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	14-feb-94 (rickh)
**	    When recreating persistent indices, store the new index id, NOT
**	    the base table id (for later poking into the IIDBDPENDS tuple).
**	    This fixes bug 58850:  if a base table with a constraint was
**	    modified, then subsequent dropping of the constraint would also
**	    clobber the base table.
**       02-oct-1994 (Bin Li)
**       Within qeu_dbu function in qeus.c file, after the dmf_call,
**       if error 'BAD_LOCATION' is encountered, check the opcode to see
**       whether it is equal to DMU_RELOCATE. If it is , do the
**       Mecopt function to copy the dmu_cb->error to error_block.
**	20-jun-94 (jnash)
**	    When fast-loading heap tables, pass "sync at close" flag to 
**	    DMT_OPEN to make use of fsync performance optimizations.  File 
**	    synchronization will take place during DMT_CLOSE.  
**      19-dec-94 (sarjo01) from 22-Aug-94 (johna)
**          set dmt_cb.dmt_mustlock. Bug 58681
**      21-aug-95 (tchdi01)
**          Added processing of the E_DM0181_PROD_MODE_ERR error for the
**          production mode project; do not look for database object
**          that depend on temporary tables in production mode -- there
**          cannot be any. This means cascading is unnecessary when
**          dropping a temporary table
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
**      19-apr-1996 (stial01)
**          Added case E_DM0157_NO_BMCACHE_BUFFERS
**	10-june-1996 (angusm)
**		Ensure 'out of locks' E_DM004B gets propagated up correctly
**		(bug 68281)
**      27-jun-96 (sarjo01)
**          Bug 75899:  qeu_b_copy(): make sure that qeu_copy->qeu_count is
**	    reset to 0 before leaving here. If row_estimate was given in
**	    the COPY FROM query, it is passed in qeu_copy->qeu_count, which
**	    also serves as the running row count field. In the case of bulk
**          copy this field was getting cleared somewhere else, but not in
**	    the case of non-bulk copy, causing the final rows added count
** 	    to include the row estimate. 
**	23-jul-96 (pchang)
**	   Do not perform cascading destruction of temporary lightweight 
**	   tables.  This is unnecessary since currently there cannot be any
**	   database object that is dependent on a temporary table (B77717).
**      25-jul-96 (ramra01)
**          Alter Table add/drop column restrict/cascade implementation.
**	12-sep-96 (pchang)
**	   Do not attempt to recreate persistent indices when reorganizing a
**	   base table to change locations since secondary indices need not be 
**	   dropped in so doing (B78270).  
**	26-sep-96 (pchang)
**	   Extend the above fix to the following modify operations:
**	   MERGE, ADD_EXTEND, TABLE_DEBUG (B78206). 
**	14-nov-1996 (nanpr01)
**	    Disallow modify of temporary tables to diff page sizes.
**	    map the error message E_DM0183_CANT_MOD_TEMP_TABLE.
**	21-nov-1996 (shero03)
**	    Add support for RTree and thereby fix bug B79469
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support 
**	    <transaction access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	20-May-1998 (kinpa04)
**	    Turned off rebuild_pindex for the modify of the flags for
**	    DMU_LOG_INCONSISTENT & DMU_PHYS_INCONSISTENT.
**	28-oct-1998 (somsa01)
**	    In qeq_dbu() and qeu_b_copy(), in the case of Global Session
**	    Temporary Tables, make sure that its locks reside on the session
**	    lock list.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    In qeu_b_copy(), refined check for a Global Session Temporary
**	    Table. Also, removed unecessary extra code for using DMT_SHOW,
**	    as well as moving up the original call to it. In qeu_dbu(),
**	    refined check for a Global Session Temporary Table.  (Bug #94059)
**      28-Jan-1999 (hanal04)
**          In qeu_dbu() E_DM0187 should be reported as a BAD OLDLOCATION
**          error. b92641.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**      18-mar-1999 (stial01)
**          qeu_dbu() Fixed error handling for DMU_PINDEX_TABLE.
**	21-mar-1999 (nanpr01)
**	    Support raw location.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      11-aug-1999 (rigka01)
**          In que_b_copy, do not disallow options to be specified with a
**          no fast load copy because the fix to bug 94487 requires
**          that min_pages and max_pages be allowed.  Bug #98350.
**	19-apr-2001 (rigka01) bug #100019
**	    Restrict fix to bug 98350 to hash tables just as fix to bug 94487 
**	    only applies to hash tables.
**      25-apr-2001 (horda03)
**         Do not rebuild persistent indices if the modify command hasn't
**         dropped the index (e.g. a MODIFY tab TO READONLY). B104553.
**      31-aug-1999 (hweho01)
**          Added NO_OPTIM for ris_u64 to eliminate error during createdb. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Deleted qef_dmf_id, use qef_sess_id instead.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	    Deleted redeclaration of dmf_call(), qsf_call().
**      18-Apr-2001 (hweho01)
**          Removed the prototype declaration of qef_error(), since it
**          is defined in qefprotos.h header file.
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**      19-apr-2001 (rigka01) bug #100019
**          Restrict fix to bug 98350 to hash tables just as fix to bug 94487
**          only applies to hash tables.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      21-oct-2002 (horda03) Bug 77895
**          For a COPY FROM, replace the attribute information supplied
**          in the tuple provided by the FE with the default value for
**          all attributes not specified.
**      13-jan-2003 (huazh01)
**          In qeu_dqrymod_objects(), drop the comments on the table/columns, 
**          though the user does not specify the 'cascade' option. 
**          This fixes bug 109442/INGSRV 2054. 
**	11-Feb-2004 (schka24)
**	    Support partitioned table creation.
**      30-mar-2004 (stial01 for inkdo01)
**	    Fixed error handling for create table.
**	8-Apr-2004 (schka24)
**	    Slice out copy stuff.
**      14-apr-2004 (stial01)
**	    Maximum btree key size varies by pagetype, pagesize (SIR 108315)
**      17-may-2004 (stial01)
**          Fixing error handling for create table again.
**      21-jun-2004 (stial01)
**          Fixing error handling for create table AGAIN.
**	10-Mar-2005 (schka24)
**	    Persistent index didn't get the right global-ness if base table
**	    is changing partitioning status (becoming partitioned or becoming
**	    non-partitioned).
**	10-may-2005 (raodi01) bug110709 INGSRV2470
**	    Added handling of code when a temporary table is modified with
**	    key compression.
**	30-Aug-2005 (thaju02)
**	    Modified setupDmuChar(). Share DMT_SYS_MAINTAINED characteristic
**	    with partitions. (B115134)
**      03-Feb-2006 (horda03) Bug 112560/INGSRV2880
**          Indicate to qeu_d_check_conix() whether or not dealing with a
**          Temp table.
**	19-nov-2007 (smeke01) b119490
**	    Fix for bug b119490.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/


FUNC_EXTERN VOID qed_u10_trap();

/*
** forward declarations
*/
static  DB_STATUS
qeu_reg_table_text(
QEU_CB  *qeu_cb,
DMU_CB  *dmu_cb,
QEF_CB  *qef_cb);

static DB_STATUS
qeu_qrytext_to_tuple(
QEU_CB	    *qeu_cb,
i4	    mode,
RDD_QDATA   *data,
DB_QRY_ID   *qryid,
ULM_RCB     *ulm);

static  DB_STATUS
qeu_tuple_to_iiqrytext(
QEU_CB	    *qeu_cb,
QEUQ_CB	    *qeuq_cb,
QEF_CB	    *qef_cb);

static  DB_STATUS
qeu_remove_from_iiqrytext(
i4	    gw,
DB_QRY_ID   *key,
QEUQ_CB	    *qeuq_cb,
QEF_CB	    *qef_cb,
i4	    *error);

static DB_STATUS qeu_modify_prep(
	QEU_CB *qeucb,
	DMU_CB *dmucb,
	ULM_RCB *ulm,
	DM_DATA *pp_array,
	i4 *err_code);

static DB_STATUS qeu_modify_finish(
	QEF_CB *qef_cb,
	DMU_CB *dmucb,
	DM_DATA *pp_array);

static DB_STATUS
collectPersistentIndices(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    i4			baseTableID,
    i4			changing_part,
    DMU_CB		**indexDMU_CBs,	/* linked list of DMU_CBs for
					** recreating the persistent indices */
    DB_TAB_ID		**oldIndexIDs,
    i4			*returnedNumberOfPersistentIndices
);

static DB_STATUS
lookupTableInfo(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*baseTableID,
    DMT_SHW_CB		**tableCB
);

static DB_STATUS
lookupTableEntry(
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*tableID,
    DMT_SHW_CB		*tableCB,
    DMT_TBL_ENTRY	*tableEntry
);

static DB_STATUS
buildPersistentIndexDMU_CB(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    DMU_CB		*dmu_cb,
    DMT_SHW_CB		*baseTableDMT_SHW_CB,
    i4			changing_part,
    i4			indexNumber,
    DMT_SHW_CB		*indexDMT_SHW_CB
);

static DB_STATUS
initCharacteristics(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    i4			pass,
    DM_DATA		*dm_data
);

static VOID
addCharacteristic(
    i4			pass,
    DM_DATA		*dm_data,
    i4			characteristic,
    i4			value
);

static	DB_STATUS
buildDM_PTR(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    i4		numberOfObjects,
    i4			objectSize,
    DM_PTR		*dm_ptr
);

static DB_STATUS
getMemory(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    i4			pieceSize,
    PTR			*pieceAddress
);

static DB_STATUS
qeu_updateDependentIDs(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    DB_TAB_ID		*oldDependentIDs,
    DB_TAB_ID		*newDependentIDs,
    i4			numberOfIDs,
    i4			dependentObjectType
);

static	DB_STATUS
dropDependentViews(
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*indexIDs,
    i4			numberOfIDs
);

static DB_STATUS
getNewIndexIDs(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    DMU_CB		*indexDMU_CBs,
    i4			numberOfIndices,
    DB_TAB_ID		**newIndexIDs
);

static DB_STATUS
createDefaultTuples(
    QEF_RCB		*qef_rcb,
    DMU_CB		*dmu_cb
);

static	bool
suppress_rowcount(
    i4	opcode,
    DMU_CB	*dmu
);

static DB_STATUS copyPartDef(
	QEU_CB		*qeucb,
	DMU_CB		*dmucb,
	ULM_RCB		*ulm,
	i4		*err_code);

static DB_STATUS locPtrToArray(
	DM_PTR		*dmPtrLoc,
	DM_DATA		*dmDataLoc,
	ULM_RCB		*ulm);

static bool sameLoc(
	DM_DATA		*dmDataLoc,
	DM_PTR		*dmPtrLoc);

static DB_STATUS newPPcharArray(
	DMU_CB		*dmucb,
	ULM_RCB		*ulm,
	i4		nparts,
	DM_DATA		*pp_desc);

static DB_STATUS partdefCopyMem(
	ULM_RCB		*ulm,
	i4		psize,
	void		*pptr);

static DB_STATUS setupAttrInfo(
	DMU_CB		*dmucb,
	DMT_SHW_CB	*dmtshow,
	i4		natts,
	ULM_RCB		*ulm,
	i4		*err_code);

static void setupDmuChar(
	DMU_CB		*dmucb,
	DMT_SHW_CB	*dmtshow);

static i4 createTblErrXlate(
	DMU_CB		*dmucb,
	char		*tbl_name,
	i4		dmf_errcode,
	DB_STATUS	status);


/* definitions needed for the characteristics compiler */

#define	NUMBER_OF_PASSES	2

#define ADD_CHARACTERISTIC( characteristic, value )	\
	addCharacteristic( pass, &dmu_cb->dmu_char_array, \
			   characteristic, value );

/* Value passed around in "changing_part" */

#define NO_CHANGE		0
#define BECOMING_PARTITIONED	1
#define BECOMING_NONPARTITIONED	2

/* short hand for common code sprawls */

#define	GET_MEMORY( size, pieceAddress )	\
	status = getMemory( qef_rcb, ulm, size, ( PTR * ) pieceAddress ); \
	if ( status != E_DB_OK )	return( status );

#define	QEF_ERROR( errorCode )	\
            error = errorCode;	\
	    qef_error( error, 0L, status, &error, &qef_rcb->error, 0 );


/* useful for defining a code block to break out of */

#define	BEGIN_CODE_BLOCK	for ( ; ; ) {
#define	EXIT_CODE_BLOCK		break;
#define	END_CODE_BLOCK		break; }


/*
** {
** Name: QEU_DBU        - perform DBU operation
**
**  External QEF call:	    status = qef_call(QEU_DBU, &qeu_cb, caller);
**
** Description:
**      A DBU operation is performed for the client.  There are three
**	of clients to qeu_dbu.  The list of client, flags settings, and
**	their semantics are described below:
**
**	    client   caller      qeu_eflag    transaction & error handling
**
**	    SCF   QEF_EXTERNAL  QEF_EXTERNAL  handle errors and transaction
**					      state.
**	    PSY   QEF_EXTERNAL  QEF_INTERNAL  handle errors but leave the
**					      transaction state to be handled
**					      by the client. For a forced
**					      abort error, however, the user's
**					      transaction will be aborted and
**					      qeu_dbu returns E_QE0025.
**	    QEF	  QEF_INTERNAL  QEF_INTERNAL  handle neither the error nor
**					      the transaction state.
** Inputs:
**	caller			    designate caller type
**		QEF_INTERNAL	    called by other QEF routine
**		QEF_EXTERNAL	    called by external facility
**      qeu_cb
**	    .qeu_eflag		    designate error handling semantics
**				    for user errors.
**		QEF_INTERNAL	    return error code.
**		QEF_EXTERNAL	    send message to user, except for forced
**				    abort errors.
**	    .qeu_db_id		    database id
**	    .qeu_d_id		    dmf session id
**          .qeu_d_op		    id for DMF operation
**          .qeu_d_cb		    control block for DMF operation or zero.
**	    .qeu_qp		    QSF name for DMF control block. This is used
**				    if the qeu_d_cb is zero and if 
**				    qeu_qso is zero.
**	    .qeu_qso		    QSF object id. This is used if 
**				    qeu_d_cb is zero.
**	create statements require
**		.dmu_name	    name of table
**		.dmu_owner	    owner
**		.dmu_location	    location where table resides
**		.dmu_attr_array.ptr_in_count number of entries
**		.dmu_attr_array.ptr_address ptr to an area used to pass
**					ptrs of type DMU_CHAR_ENTRY
**		.dmu_attr_array.ptr_size    size of an entry
**		<dmu_attr_array> are of type DMF_ATTR_ENTRY
**		attr_name
**		attr_type
**		attr_size
**		attr_precision
**		attr_collID
**		attr_flags_mask
**		.dmu_char_array.data_address
**		.dmu_char_array.data_in_size
**		<dmu_char_array> is of type DMU_CHAR_ENTRY
**		char_id
**		char_value
**		
**	destroy statements require
**		.dmu_tbl_id	    internal name of table
**	index statements require
**		.dmu_tbl_id	    internal name of base relation
**		.dmu_name	    external name of index
**		.dmu_location	    location where table resides
**		.dmu_key_array.ptr_in_count
**		.dmu_key_array.ptr_address
**		.dmu_key_array.ptr_size
**		<dmu_key_array> is of type DMU_KEY_ENTRY
**		key_attr_name
**		key_order
**		.dmu_char_array.data_address
**		.dmu_char_array.data_in_size
**		<dmu_char_array> is of type DMU_CHAR_ENTRY
**		char_id
**		char_value
**	modify statements require
**		.dmu_key_array.ptr_in_count
**		.dmu_key_array.ptr_address
**		.dmu_key_array.ptr_size
**		<dmu_key_array> is of type DMU_KEY_ENTRY
**		key_attr_name
**		key_order
**		.dmu_char_array.data_address
**		.dmu_char_array.data_in_size
**		<dmu_char_array> is of type DMU_CHAR_ENTRY
**		char_id
**		char_value
**	relocate statements require
**		.dmu_tbl_id	    internal name of relation
**		.dmu_location	    location where table resides
**		
**
** Outputs:
**	row_count		    rowcount for index or modify
**
**      error_block->err_code       One of the following
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE001D_FAIL
**				    E_QE0025_USER_ERROR
**				    E_QE0028_BAD_TABLE_NAME
**				    E_QE0029_BAD_OWNER_NAME
**				    E_QE002A_DEADLOCK
**				    E_QE002B_TABLE_EXISTS
**				    E_QE002C_BAD_ATTR_TYPE
**				    E_QE002D_BAD_ATTR_NAME
**				    E_QE002E_BAD_ATTR_SIZE
**				    E_QE002F_BAD_ATTR_PRECISION
**				    E_QE0030_BAD_LOCATION_NAME
**				    E_QE0031_NONEXISTENT_TABLE
**				    E_QE0034_LOCK_QUOTA_EXCEEDED
**				    E_QE0035_LOCK_RESOURCE_BUSY
**				    E_QE0036_LOCK_TIMER_EXPIRED
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-may-86 (daved)
**          written
**	17-apr-87 (rogerk)
**	    Changed to use qet_begin instead of qeu_btran so that a user
**	    transaction will be started instead of an internal one.
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc.
**	06-jan-87 (puree)
**	    Modified error and transaction handling to support PSY.
**	11-feb-88 (puree)
**	    Put an exclusive lock on the table to be destroyed by openning
**	    and closing it.
**	17-feb-88 (puree)
**	    Fix bug setting error code from DMF.  Also, check for error
**	    openning the table when the table to be destroyed is a view.
**	08-aug-88 (puree)
**	    Watch for E_DM0114_FILE_NOT_FOUND after dmf call
**	20-sep-88 (puree)
**	    Fix bug setting error code from DMF.
**	05-oct-88 (puree)
**	    modified qeu_dbu due to change in user error 5102 format by PSF.
**	24-oct-88 (puree)
**	    fix string length for message 5102.
**	09-nov-88 (puree)
**	    modify qeu_dbu to report the invalid location name.
**	21-nov-88 (puree)
**	    remove the statement that set err= E_QE0025 for a force abort.
**	    If the eflag is set to QEF_INTERNAL, the E_QE002A must be seen
**	    by the caller.
**	07-mar-89 (neil)
**	    Modified table destruction to remove all rules applied to table.
**	13-mar-89 (carl)
**	    Added code to commit CDB association to avoid deadlocks
**	01-apr-89 (carl)
**	    Modified to process REGISTER WITH REFRESH
**	07-oct-89 (carl)
**	    Modified qeu_dbu to call qel_u3_lock_objbase to avoid STAR 
**	    deadlocks
**	08-nov-89 (neil)
**	    Clear flags before qeu_dprot to avoid collisions with other objects.
**	24-apr-90 (andre)
**	    Drop synonyms when dropping a table.
**	04-may-90 (carol)
**	    Add cascading drops for comments.
**	21-jun-90 (linda, bryanp)
**	    Changed dmt_open flag to DMT_A_OPEN_NOACCESS rather than
**	    DMT_A_WRITE for destroy table operation, relevant only to gateway
**	    tables where the foreign data file need not exist (this access code
**	    is translated to a WRITE access code at lower levels of DMF for
**	    non-Gateway tables).
**	22-jun-90 (linda)
**	    Fill in qeuq_cb status mask from DMT_SHOW status mask, so that
**	    views can be identified directly rather than by checking
**	    db_qry_high_time and db_qry_low_time (gateway tables have non-zero
**	    values for these as do views).
**	13-nov-90 (rachael)
**	    Bugfix for 8498. Added check for caller == QEF_INTERNAL after the
**	    dmf_call in qeu_dbu.  The qeu_cb->qeu_eflag was not being set by 
**	    qea_dmu.
**	10-dec-90 (neil)
**	    Alerters: Clear flags before qeu_dprot to avoid collisions with
**	    other objects.
**	12-jun-91 (rickh)
**	    Set dmu_gw_id for DESTROY operation.  Random noise on the stack
**	    isn't good enough.
**	16-oct-91 (bryanp)
**	    Part of 6.4 -> 6.5 merge changes. Must initialize new dmt_char_array
**	    field for DMT_SHOW calls.
**	23-jun-92 (andre)
**	    when asked to destroy a table or a view, call qeu_d_cascade() (some
**	    time in the future we will have to distinguish cascading vs.
**	    restricted descrution - until that happens, destruction will always
**	    be cascading); we will no longer try to call qeu_dview() for views
**	    and qeu_dqrymod_objects() for base tables - this is so because we
**	    are interested in existence of objects besides views which may
**	    depend on a table
**	08-sep-92 (andre)
**	    if dropping a temporary table, tell qeu_d_cascade() about it
**	20-nov-92 (rickh)
**	    Added action and dsh arguments to qeu_dbu.  qeu_dbu now exposes
**	    the ID of the table it creates.
**	08-dec-92 (barbara)
**	    Address of QEU_DDL_INFO is stored in QEU_CB.qeu_ddl_info
**	    instead of overloading qeu_qso.
**	11-feb-93 (andre)
**	    added code to recognize and handle QEU_DROP_BASE_TBL_ONLY - a new
**	    mask defined over QEU_CB.qeu_flag - ONLY when processing a request
**	    to destroy a LOCAL BASE TABLE.  When set, it will instruct us to
**	    destroy specified base table without regard for any object that may
**	    depend on it.  Naturally, it will be used only in very restricted
**	    circumstances (in particular, 6.5 UPGRADEDB needs to destroy and
**	    recreate IIDBDEPENDS whose structure has changed between 6.4 and
**	    6.5.  Since IIDBDEPENDS itself is heavily used in qeu_d_cascade(),
**	    it is simpler to have the table destroyed here without ever going to
**	    qeu_d_cascade().)
**	    Note that QEU_DROP_BASE_TBL_ONLY will not be recognized for views since
**	    there is more to dropping a view than dropping its description from
**	    IIRELATION and IIATTRIBUTES.
**	17-feb-93 (rickh)
**	    When modifying a base table, be sure you recreate the persistent
**	    indices.  Also, when creating a table, make sure you populate
**	    IIINTEGRITY as necessary.
**	22-mar-93 (rickh)
**	    System generated indices (e.g., those generated on behalf of
**	    constraints) should not report a row count.
**	23-apr-93 (anitap)
**	    Added support for the creation of implicit schemas for CREATE TABLE.
**	09-jun-93 (rickh)
**	    Added some return arguments to qeu_dbu.  The point was to
**	    isolate the QEU_CB as a read-only argument.  This in turn was
**	    done because sometimes the QEU_CB may have been compiled into
**	    code space by OPC.
**	07-jul-93 (anitap)
**	    Added extra argument to qea_schema() in qeu_dbu().
**	    Do not create implict schema for temp tables.
**	    Changed assignment of flag to IMPSCH_FOR_TABLE.
**	3-aug-93 (rickh)
**	    When creating a unique index on behalf of a UNIQUE CONSTRAINT, emit
**	    a CONSTRAINT error if duplicate rows prevent you from creating
**	    the index.  While I was in here, I trimmed the trailing white space
**	    off table names in other error messages.
**	10-sep-93 (andre)
** 	    before calling  qeu_d_cascade(), set QEU_FORCE_QP_INVALIDATION to 
**	    ensure that it does its best to invalidate QPs dependent on the 
**	    object being dropped and any objects that depend on it 
**	6-oct-93 (stephenb)
**	    Set tablename in qeuq_cb so that we can audit drop temp table.
**      17-Sep-1993 (fred)
**          Remove unconditional setting of dmu_flags_mask to 0.  This 
**          field should be correctly initialized by whoever is 
**          providing the control block (PSF), and the appropriate 
**          value is NOT always zero.  In particular, large object 
**          support allows non-zero values for relocate/modify 
**          operations to cause the extension tables to be relocated 
**          and/or modified either instead of or in addition to the 
**          base table.
**      30-sep-93 (anitap)
**          Create implicit schema for journaled tables. Fix for bug 53525.
**	    Also fixed compiler warning.
**	08-oct-93 (andre)
**	    qeu_d_cascade() expects one more parameter - an address of a 
**	    DMT_TBL_ENTRY describing table/view/index being dropped; for all 
**	    other types of objects, NULL must be passed
**	27-oct-93 (andre)
**	    when creating gateway tables/indexes, dm2u_create() will compute 
**	    text id and return it inside dmu_cb->dmu_qry_id (as opposed to 
**	    having us compute it here and pass it down to DMF.)  This change is
**	    required because in order to avoid duplicate text ids we decided 
** 	    to use a derivative of object id as the text id and DMF, having 
** 	    computed the object id, must populate text id in IIRELATION tuple.
**	    This also means that text of registrations must be stored AFTER the
**	    call to DMF to create the IIRELATION tuples representing the table
**	    or index.
**	17-jan-94 (rickh)
**	    Make sure errors from recreation of persistent indices are stuffed
**	    into the correct error variable.
**      14-sep-95 (tchdi01)
**          Do not cascade destruction of a temporary table in production mode.
**          Process production mode related errors returned from the called
**          facilities.
**      19-apr-96 (stial01)
**          Added case E_DM0157_NO_BMCACHE_BUFFERS
**	10-june-1996 (angusm)
**		Level above expects QEF error in qeu_cb: however, DM004B is
**		passed on qef_rcb and gets overwritten. Slip DM004B into
**		qeu_cb to pass up correctly. (bug 68281)
**	23-jul-96 (pchang)
**	   Do not perform cascading destruction of temporary lightweight 
**	   tables.  This is unnecessary since currently there cannot be any
**	   database object that is dependent on a temporary table (B77717).
**	25-jul-96 (ramra01)
**	    Alter Table add/drop column restrict/cascade implementation.
**	12-sep-96 (pchang)
**	   Do not attempt to recreate persistent indices when reorganizing a
**	   base table to change locations since secondary indices need not be 
**	   dropped in so doing (B78270).  
**	26-sep-96 (pchang)
**	   Extend the above fix to the following modify operations:
**	   MERGE, ADD_EXTEND, TABLE_DEBUG (B78206). 
**	08-nov-96 (nanpr01)
**	   map the TUPLE_TOO_WIDE error message correctly.
**	05-dec-1996 (nanpr01)
**	   Bug 79484 : Previous bug fix of 78270 cause this. We have to
**	   check if the data_address is null or not before assigning.
**	20-dec-1996 (nanpr01)
**	   Bug 79853 : comment on columns are not getting dropped.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support 
**	    <transaction access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	26-mar-1997 (nanpr01)
**	    Initialized the field for Alter Table drop col with ref integ.
**	28-oct-1998 (somsa01)
**	    In the case of Global Session Temporary Tables, make sure that
**	    its locks reside on the session lock list.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    Refined check for a Global Session Temporary Table.  (Bug #94059)
**      28-Jan-1999 (hanal04)
**          E_DM0187 should be reported as a BAD OLDLOCATION error. b92641.
**      18-mar-1999 (stial01)
**          qeu_dbu() Fixed error handling for DMU_PINDEX_TABLE.
**	12-may-1999 (thaju02)
**	   If dmf_call fails with error E_DM0065, pass error up via
**	   qeu_cb->error. (B96940)
**      25-apr-2001 (horda03)
**         Do not rebuild persistent indices if the modify command hasn't
**         dropped the index (e.g. a MODIFY tab TO READONLY). B104553.
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**         Prevent a user index from being dropped if a constraint is
**         using the user index.
**         Prevent a modify from taking place if the table/index is
**         being used by a constraint.
**         qeu_d_check_conix() is used to see if a table/index is
**         being used by a constraint. 
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	11-Feb-2004 (schka24)
**	    Break off create table guts for partitioning.
**	    Don't reference rowcount after (parallel index) dmu stream is
**	    returned.
**	26-Feb-2004 (schka24)
**	    More partitioning changes, for MODIFY this time.
**	10-Apr-2004 (gupsh01)
**	    Added support for alter table alter column.
**	30-apr-2004 (thaju02)
**	    Set DMU_PIND_CHAINED if persistent index DMU_CBs are linked to 
**	    modify DMU_CB.
**	1-Jun-2004 (schka24)
**	    Split-up of create table missed the fact that register table
**	    goes thru the same path, insert iiqrytext rows after create.
**	    (Otherwise remove of registration fails.)  It's not clear to
**	    me what good the iiqrytext rows are, but I guess something
**	    looks at them.
**	22-jun-2004 (thaju02)
**	    Online modify and alter table drop col results in deadlock,
**	    involving table lock and tbl control locks. If DMT_LK_ONLN_NEEDED, 
**	    request shared online modify lock. (B112536)
**	22-jul-2004 (thaju02)
**	    DMT_LK_ONLN_NEEDED does not apply to temp tables. (B112728)
**      28-dec-2004 (huazh01)
**         add trace point qe61, which allows user to modify a table
**         with constraint dependency.
**         b112978, INGSRV3004.
**	10-Mar-2005 (schka24)
**	    Rebuild persistent indexes as GLOBAL indexes if we're making
**	    the base table partitioned (and was non-partitioned).
**	20-mar-2006 (toumi01)
**	    Add support for DMU_NODEPENDENCY_CHECK, equivalent to t.p. qe61.
**	15-Aug-2006 (jonj)
**	    Avoid "rebuild_pindex" if the table being operated on is itself an
**	    index. Be sure to check all dmu cb's whenever there is a list of
**	    them.
**      07-jun-2007 (huazh01)
**          Add config parameter qef_no_dependency_chk to the fix to b112978.
**	19-nov-2007 (smeke01) b119490
**	    Set rebuild_pindexes to TRUE only for DMU_MODIFY_TABLE variants 
**	    'to truncated' and 'to <structure>'. Previously, due to a change 
**	    made during 111488, all DMU_MODIFY_TABLE variants set 
**	    rebuild_pindexes to TRUE, resulting in bug b119490. 
**	22-May-2008 (coomi01) Bug 120413	    
**          Put back call to createDefaultTuples to allow default tuple to
**	    be specified for 'alter table alter column'.
**	4-Jun-2009 (kschendel) b122118
**	    Avoid pointless trial open for temp tables.
**	14-Jul-2009 (thaju02) (B122300)
**	    In reporting E_DM007D, note the errdmu's table name, rather 
**	    than that of the first dmu block. 
**      15-Feb-2010 (hanal04) Bug 123292
**          Initialise qef_rcb.error.
**	12-Apr-2010 (gupsh01) SIR 123444
**	    Added support for alter table rename table/column.
**	22-Apr-2010 (kschendel) SIR 123485
**	    Use padded copy of standard savepoint name in the QEF SCB.
**
*/
DB_STATUS
qeu_dbu(
QEF_CB		*qef_cb,
QEU_CB		*qeu_cb,	/* READONLY argument */
i4		tabid_count,
DB_TAB_ID	*table_id,
i4		*row_count,
DB_ERROR	*error_block,
i4		caller )
{
    DB_STATUS	    status   = E_DB_OK;
    DB_STATUS	    ret_stat = E_DB_OK;
    DB_STATUS	    stat     = E_DB_OK;
    QSF_RCB	    qsf_rcb;
    i4	    	    err = E_QE0000_OK;
    QEF_RCB	    *oqef_rcb;
    bool	    qsf_obj = FALSE;	/* TRUE if holding QSF object */
    bool	    tran_started = FALSE;
    bool	    table_opened = FALSE;
    QEU_CB	    *temp_cb, *end;
    DMU_CB	    dmu_cb, *dmu_ptr = &dmu_cb, *tdmu_ptr, *errdmu_ptr;	
    DMT_CB	    dmt_cb, *dmt_ptr = &dmt_cb;
    QEF_RCB	    qef_rcb;
    i4	    	    opcode = qeu_cb->qeu_d_op;
    bool            createDfltTuple = (qeu_cb->qeu_flag & QEU_DFLT_VALUE_PRESENT) != 0;
    bool	    TempTable = (qeu_cb->qeu_flag & QEU_TMP_TBL) != 0;
    bool	    just_drop_base_table =
			(qeu_cb->qeu_flag & QEU_DROP_BASE_TBL_ONLY) != 0;
    bool	    rebuild_pindex = FALSE;	/* rebuild persistent index */
    DMU_CHAR_ENTRY  chr;
    QED_DDL_INFO    *ddl_p = &qef_rcb.qef_r3_ddb_req.qer_d7_ddl_info;
    DB_TAB_ID	    *tableID;
    DMU_CB	    *persistentIndexDMU_CBs = (DMU_CB *)0;
    DB_TAB_ID	    *oldPersistentIndexIDs;
    DB_TAB_ID	    *newPersistentIndexIDs;
    i4		    numberOfPersistentIndices;
    ULM_RCB         ulm;
    GLOBALREF	    QEF_S_CB	  *Qef_s_cb;
    bool	    memoryStreamOpened = FALSE;
    i4		    flag = IMPSCH_FOR_TABLE;	
    QEUQ_CB	    *qeuq_cbs = (QEUQ_CB *)NULL;
    DMU_CHAR_ENTRY  *char_entry;			
    i4	    	    char_count;
    i4	    	    i, NumofIndices, j;
    bool	    dmu_atbl_addcol  = FALSE;
    bool	    dmu_atbl_dropcol = FALSE;
    bool	    dmu_atbl_cascade = FALSE;
    DM_DATA	    pp_array;
    DB_PART_DEF	    *full_part_def;	/* Original partition def */
    i4		    full_partdef_size;	/* Size of same */
    i4		    val_1, val_2;
    bool	    dmu_atbl_altcol  = FALSE;
    bool	    dmu_atbl_rentab  = FALSE;
    bool	    dmu_atbl_rencol  = FALSE;
    i4		    dummy1, dummy2;
    DMF_ATTR_ENTRY	**dmu_attr;
    char		column_name[DB_ATT_MAXNAME +1];


    *row_count = -1;	    /* initialize to illegal value */

    pp_array.data_address = NULL;
    pp_array.data_in_size = 0;

    if (opcode == QED_RLINK)	    /* distributed */
	goto ddb_refresh;	    /* here until new PSF interface is struck */

    if ((opcode == DMU_PINDEX_TABLE) && 
	((qeu_cb->qef_next == NULL) || 
	 (qeu_cb->qef_next != qeu_cb->qeu_prev)))
        /* if multiple qeu_cb was passed from parser */
        for (NumofIndices = 1, temp_cb = qeu_cb->qef_next, 
	     end = qeu_cb->qeu_prev; 
	     temp_cb != end; temp_cb = temp_cb->qef_next)
            NumofIndices++;
    else
	NumofIndices = 1;
 
    /* get control block from QSF if not passed in here */
    if (qeu_cb->qeu_d_cb == 0)
    {
        qsf_rcb.qsf_next = 0;
	qsf_rcb.qsf_prev = 0;
	qsf_rcb.qsf_length = sizeof(QSF_RCB);
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_owner = (PTR)DB_QEF_ID;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_sid = qef_cb->qef_ses_id;

	qsf_rcb.qsf_lk_state = QSO_SHLOCK;
	if (qeu_cb->qeu_qso)
	{
	    qsf_rcb.qsf_obj_id.qso_handle = qeu_cb->qeu_qso;
	    /* access QSF by handle */
	    if (status = qsf_call(QSO_LOCK, &qsf_rcb))
	    {
		err = qsf_rcb.qsf_error.err_code;
		goto exit;
	    }
	}
	else
	{
	    /* get object by name */
	    qsf_rcb.qsf_obj_id.qso_type = QSO_QP_OBJ;
	    MEcopy((PTR)&qeu_cb->qeu_qp, sizeof (DB_CURSOR_ID), 
		(PTR) qsf_rcb.qsf_obj_id.qso_name);
	    qsf_rcb.qsf_obj_id.qso_lname = sizeof (DB_CURSOR_ID);
	    if (status = qsf_call(QSO_GETHANDLE, &qsf_rcb))
	    {
		err = qsf_rcb.qsf_error.err_code;
		goto exit;
	    }
	}

	qsf_obj = TRUE;	/* we have successfully claimed the object */

	/* 
	** QP is locked now
	** we copy the structure because it is a shared object and
	** we cannot write on it.
	*/
	if (opcode == DMT_ALTER)
	{
	    STRUCT_ASSIGN_MACRO(*((DMT_CB *) qsf_rcb.qsf_root), dmt_cb);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*((DMU_CB *) qsf_rcb.qsf_root), dmu_cb);
	}
    }
    else
    {
	/*
	** We cannot write on the DM?_CB because it may be a read-only
	** object to this routine.
	*/
	if (opcode == DMT_ALTER)
	{
	    STRUCT_ASSIGN_MACRO(*((DMT_CB *) qeu_cb->qeu_d_cb), dmt_cb);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(*((DMU_CB *) qeu_cb->qeu_d_cb), dmu_cb);
	}
    }

ddb_refresh:

    qef_rcb.qef_eflag = qeu_cb->qeu_eflag;
    qef_rcb.qef_flag        = 0;
    CLRDBERR(&qef_rcb.error);
    qef_rcb.qef_sess_id     = qeu_cb->qeu_d_id;
    qef_rcb.qef_db_id       = qeu_cb->qeu_db_id;
    qef_rcb.qef_cb		= qef_cb;

    /* insure that a transaction exists */
    qeu_cb->qeu_flag = 0;
    if ((caller == QEF_EXTERNAL) && (qef_cb->qef_stat == QEF_NOTRAN))
    {
	oqef_rcb = qef_cb->qef_rcb;
	qef_cb->qef_rcb = &qef_rcb;
	qef_rcb.qef_eflag = qeu_cb->qeu_eflag;

	qef_rcb.qef_modifier    = QEF_SSTRAN;
	qef_rcb.qef_flag        = 0;
	qef_rcb.qef_sess_id     = qeu_cb->qeu_d_id;
	qef_rcb.qef_db_id       = qeu_cb->qeu_db_id;
	qef_rcb.qef_cb		= qef_cb;

	status = qet_begin(qef_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    qef_cb->qef_rcb = oqef_rcb;
	    goto exit;
	}
	tran_started = TRUE;
	qef_cb->qef_rcb = oqef_rcb;
    }


    /* distributed session processing */
    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    { 
	QES_DDB_SES	*dds_p;
	QED_DDL_INFO    *i_ddl_p;


	qef_rcb.qef_cb = qef_cb;		/* must initialize */
	dds_p = & qef_cb->qef_c2_ddb_ses;
	switch (opcode)
	{
	case DMU_CREATE_TABLE:
	case DMU_DESTROY_TABLE:
	case QED_RLINK:

	    /* set up *ddl_p */

	    i_ddl_p = (QED_DDL_INFO *) qeu_cb->qeu_ddl_info;
	    STRUCT_ASSIGN_MACRO(*i_ddl_p, *ddl_p);

	    break;
	default:
	    MEfill(sizeof (*ddl_p), '\0', (PTR) ddl_p);
	    break;
	}

	/* lock to synchronize */

	status = qel_u3_lock_objbase(& qef_rcb);
	if (status)
	    goto exit;

	switch (qeu_cb->qeu_d_op)
	{
	case DMU_CREATE_TABLE:

	    status = qeu_d1_cre_tab(& qef_rcb); 
	    if (status)
		qed_u10_trap();

	    break;

	case DMU_DESTROY_TABLE:

	    status = qeu_d2_des_tab(& qef_rcb); 
	    break;

	case QED_RLINK:

	    dds_p->qes_d7_ses_state = QES_4ST_REFRESH;
						/* to signal REFRESH to
						** DROP and CREATE LINK */
	    status = qel_d0_des_lnk(& qef_rcb); 
	    if (status == E_DB_OK)
		status = qel_c0_cre_lnk(& qef_rcb); 
						/* recreate the link */
	    if (status)
		qed_u10_trap();
	    break;

	case QEU_DVIEW:
	case QED_DLINK:
	    {
		STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, ddl_p->qed_d7_obj_id);

		if (qeu_cb->qeu_d_op == QEU_DVIEW)
		    status = qeu_20_des_view(& qef_rcb);  
		else
		    status = qel_d0_des_lnk(& qef_rcb); 
	    }
	    if (status)
		qed_u10_trap();

	    break;

	case DMT_ALTER:
	    {
		dmt_ptr = & dmt_cb;

		status = qeu_d5_alt_tab(& qef_rcb, dmt_ptr); 
	    }
	    if (status)
		qed_u10_trap();

	    break;

	default:
	    status = qed_u1_gen_interr(& qef_rcb.error);
	    if (status)
		qed_u10_trap();
	    break;

	} /* end of switch */

	err = qef_rcb.error.err_code;		/* assume error */

	if (status == E_DB_OK  && qef_cb->qef_stat != QEF_NOTRAN)
	{
	    /* send message to commit CDB association to avoid deadlocks */

	    status = qed_u11_ddl_commit(& qef_rcb);
	    err = qef_rcb.error.err_code;
	}
	dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset session state */
	goto exit;
    }	/* end if distributed */

    /* non-distributed session processing */

    if (opcode == DMT_ALTER)
    {
	dmt_ptr->type = DMT_TABLE_CB;
	dmt_ptr->length = sizeof(DMT_CB);
	dmt_ptr->dmt_db_id = qeu_cb->qeu_db_id;
	dmt_ptr->dmt_flags_mask = 0;
	dmt_ptr->dmt_tran_id = qef_cb->qef_dmt_id;
	status = dmf_call(opcode, dmt_ptr);
	err = dmt_ptr->error.err_code;
    }
    else
    {
	if ( (opcode == DMU_ALTER_TABLE) &&
	     (char_entry = (DMU_CHAR_ENTRY*)
			   dmu_ptr->dmu_char_array.data_address) &&
	     (char_count = dmu_ptr->dmu_char_array.data_in_size
			   / sizeof(DMU_CHAR_ENTRY) ) )

	{
	   for (i = 0; i < char_count; i++)
	   {
	       switch (char_entry[i].char_id)
	       {
		   case DMU_ALTER_TYPE:

			dmu_atbl_addcol =
				(char_entry[i].char_value == DMU_C_ADD_ALTER);
			dmu_atbl_dropcol = 
				(char_entry[i].char_value == DMU_C_DROP_ALTER);
			dmu_atbl_altcol =
				(char_entry[i].char_value == DMU_C_ALTCOL_ALTER);
			dmu_atbl_rencol =
				(char_entry[i].char_value == DMU_C_ALTCOL_RENAME);
			dmu_atbl_rentab =
				(char_entry[i].char_value == DMU_C_ALTTBL_RENAME);
                        break;

		   case DMU_CASCADE:

			dmu_atbl_cascade =
					(char_entry[i].char_value == DMU_C_ON);
                        break;
		   default:

			continue;

		}
	    }
	}

	if (NumofIndices > 1)
	{
	     DMU_CB	*currentDMU_CB = (DMU_CB *) NULL;

	    /* get a memory stream in which to build DMU_CBs */
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	    if ((status = qec_mopen(&ulm)) != E_DB_OK)
	    {
		err = ulm.ulm_error.err_code;
		goto exit;
	    }
	    memoryStreamOpened = TRUE;

	    status = getMemory(&qef_rcb, &ulm, NumofIndices*sizeof(DMU_CB), 
				(PTR *)&dmu_ptr); 
	    if ( status != E_DB_OK )	
		goto exit;
	    tdmu_ptr = dmu_ptr;
            for (j = 1, temp_cb = qeu_cb; j <= NumofIndices;
         	 temp_cb = temp_cb->qef_next, j++)
	    {
		STRUCT_ASSIGN_MACRO(*((DMU_CB *) temp_cb->qeu_d_cb), *tdmu_ptr);
	        tdmu_ptr->q_next 	= 0;
		if (currentDMU_CB != (DMU_CB *) NULL)
		{
		    tdmu_ptr->q_prev = currentDMU_CB->q_next;
		    currentDMU_CB->q_next = (PTR) tdmu_ptr;
		}
		else
	            tdmu_ptr->q_prev 	= 0;
		currentDMU_CB = tdmu_ptr;

	        tdmu_ptr->type		= DMU_UTILITY_CB;
	        tdmu_ptr->length	= sizeof(DMU_CB);
	        tdmu_ptr->dmu_db_id	= qeu_cb->qeu_db_id;
                tdmu_ptr->dmu_tran_id = qef_cb->qef_dmt_id;
		tdmu_ptr++;
	    }
	}
	else
	{
	    dmu_ptr->q_next 	= 0;
	    dmu_ptr->q_prev 	= 0;
	    dmu_ptr->type		= DMU_UTILITY_CB;
	    dmu_ptr->length		= sizeof(DMU_CB);
	    dmu_ptr->dmu_db_id	= qeu_cb->qeu_db_id;
            dmu_ptr->dmu_tran_id = qef_cb->qef_dmt_id;
	}

	/*
	** if destroying a table, we need to delete other catalog
	** information for this table. Specifically, view, protect, comment,
	** integrity, rule, alarm and statistics information.  Issue a DMT_SHOW
	** to look at the table status bits.
	*/

	if ( (opcode == DMU_DESTROY_TABLE) || (dmu_atbl_dropcol) )
	{
	    DMT_SHW_CB		dmt_show;
	    DMT_TBL_ENTRY	dmt_tbl_entry;
	    QEUQ_CB		qeuq_cb;
	    DB_QMODE		obj_type;

	    /* This trial open may have some relevance for locking;
	    ** but it's essentially pointless for temporary tables.
	    */
	    if (!TempTable)
	    {
		MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
		dmt_cb.type         = DMT_TABLE_CB;
		dmt_cb.length       = sizeof(DMT_CB);
		dmt_cb.dmt_db_id    = qeu_cb->qeu_db_id;
		STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, dmt_cb.dmt_id);

		dmt_cb.dmt_flags_mask |= DMT_LK_ONLN_NEEDED;
		dmt_cb.dmt_lock_mode   = DMT_X;
		dmt_cb.dmt_update_mode = DMT_U_DIRECT;
		dmt_cb.dmt_access_mode = DMT_A_OPEN_NOACCESS;
		dmt_cb.dmt_sequence	   = qef_cb->qef_stmt;
		dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
		status = dmf_call(DMT_OPEN, &dmt_cb);

		dmt_cb.dmt_flags_mask &= ~DMT_LK_ONLN_NEEDED;

		if (status == E_DB_OK ||
		    dmt_cb.error.err_code == E_DM0114_FILE_NOT_FOUND)
		{
		    table_opened = TRUE;
		    status = dmf_call(DMT_CLOSE, &dmt_cb);
		    if (status != E_DB_OK)
		    {
			err = dmt_cb.error.err_code;
			goto exit;
		    }
		    table_opened = FALSE;
		}
		else if (dmt_cb.error.err_code == E_DM009E_CANT_OPEN_VIEW)
		{
		    status = E_DB_OK;
		}
		else
		{
		    err = dmt_cb.error.err_code;
		    goto exit;
		}
	    } /* trial open/close */

	    /* Format the DMT_SHOW request block */
	    dmt_show.type = DMT_SH_CB;
	    dmt_show.length = sizeof(DMT_SHW_CB);
	    dmt_show.dmt_char_array.data_in_size = 0;
	    dmt_show.dmt_char_array.data_out_size  = 0;
	    STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, dmt_show.dmt_tab_id);
	    dmt_show.dmt_flags_mask = DMT_M_TABLE;
	    dmt_show.dmt_db_id	= qeu_cb->qeu_db_id;
	    dmt_show.dmt_session_id = qeu_cb->qeu_d_id;
	    dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	    dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	    dmt_show.dmt_char_array.data_address = (PTR)NULL;
	    dmt_show.dmt_char_array.data_in_size = 0;
	    dmt_show.dmt_char_array.data_out_size  = 0;

	    status = dmf_call(DMT_SHOW, &dmt_show);

	    if (status != E_DB_OK &&
		dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
	    {
		err = dmt_show.error.err_code;
		goto exit;
	    }

	    status = E_DB_OK;

	    dmu_ptr->dmu_gw_id = dmt_tbl_entry.tbl_relgwid;

	    qeuq_cb.qeuq_next		=
		qeuq_cb.qeuq_prev	= 0;
	    qeuq_cb.qeuq_length		= sizeof (qeuq_cb);
	    qeuq_cb.qeuq_type		= QEUQCB_CB;
	    qeuq_cb.qeuq_owner		= (PTR)DB_QEF_ID;
	    qeuq_cb.qeuq_ascii_id	= QEUQCB_ASCII_ID;
	    qeuq_cb.qeuq_db_id		= qeu_cb->qeu_db_id;
	    qeuq_cb.qeuq_d_id		= qeu_cb->qeu_d_id;
	    qeuq_cb.qeuq_eflag		= qeu_cb->qeu_eflag;
	    qeuq_cb.qeuq_rtbl		= &dmu_ptr->dmu_tbl_id;
	    qeuq_cb.qeuq_status_mask	= dmt_tbl_entry.tbl_status_mask;
	    qeuq_cb.qeuq_qid.db_qry_high_time =
		qeuq_cb.qeuq_qid.db_qry_low_time = 0;
	    qeuq_cb.qeuq_flag_mask = (TempTable) ? QEU_DROP_TEMP_TABLE : 0;

	    /*
	    **  For a REMOVE TABLE command, we need to remove the text of the
	    **  REGISTER TABLE that created this table from iiqrytext; and
	    **  similarly for a REMOVE INDEX/REGISTER INDEX.  A
	    **  DMU_DESTROY_TABLE for a Gateway object is a REMOVE
	    **  TABLE/INDEX, with associated iiqrytext.
	    */
	    if (!TempTable && dmt_tbl_entry.tbl_status_mask & DMT_GATEWAY)
	    {
		i4	    rem_err;

		ret_stat = qeu_remove_from_iiqrytext(dmu_ptr->dmu_gw_id,
		    &dmt_tbl_entry.tbl_qryid, &qeuq_cb, qef_cb, &rem_err);
		if (DB_FAILURE_MACRO(ret_stat))
		{
		    err = rem_err;
		    goto exit;
		}
	    }

	    /* Do not allow an index to be dropped if a constraint
	    ** is using it.
	    ** Base tables are dropped along with dependant constraints,
	    ** to block drop of base table change the check below to
	    ** ~dmt_tbl_entry.tbl_status_mask & DMT_VIEW
	    */
	    if ( dmt_tbl_entry.tbl_status_mask & DMT_IDX )
	    {
                DB_TAB_ID indexid;
                bool      found_one = FALSE;
		i4        error;
 
                if (!memoryStreamOpened)
                {
                    /* get a memory stream in which to build DMU_CBs */
                    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
                    if ((status = qec_mopen(&ulm)) != E_DB_OK)
                    {
                        err = ulm.ulm_error.err_code;
                        goto exit;
                    }
                    memoryStreamOpened = TRUE;
                }

                STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, indexid);

		/* Check for a dependent constraint */
                status = qeu_d_check_conix(qef_cb, &qeuq_cb,
		      &ulm, &indexid, TempTable, &error, &found_one);

                if ((status != E_DB_OK) || (found_one))
                {
                    if(status != E_DB_OK)
                    {
                        err = error;
                    }
                    else
                    {
			/* Dependent constraint found */
	                status = E_DB_ERROR;
                        qef_error(I_QE2037_CONSTRAINT_TAB_IDX_ERR, 0L,
                             status, &error, error_block, 3,
                             (sizeof("DROP/DESTROY") - 1), "DROP/DESTROY",
			     (sizeof("drop") - 1), "drop",
                             qec_trimwhite(DB_TAB_MAXNAME,
                                    dmu_ptr->dmu_table_name.db_tab_name),
                             dmu_ptr->dmu_table_name.db_tab_name);
			err = E_QE0025_USER_ERROR;
                    }
                    goto exit;
                }
		status = E_DB_OK;
	    }

	    /*
	    ** If the object is a temporary lightweight table OR 
	    ** if the object is NOT a view and the caller has requested that we
	    ** drop it without regard for any objects that may depend on it, we
	    ** will do it right here; otherwise, qeu_d_cascade() will be asked
	    ** to perform the cascading destruction
	    */
	    if ( TempTable || (just_drop_base_table && 
				~dmt_tbl_entry.tbl_status_mask & DMT_VIEW) )
	    {
		DMU_CB	dmucb, *dmu_cb_p = &dmucb;
		
		dmu_cb_p->length = sizeof(DMU_CB);
		dmu_cb_p->type = DMU_UTILITY_CB;
		dmu_cb_p->dmu_flags_mask = 0;
		dmu_cb_p->dmu_tbl_id.db_tab_base =
		    dmu_ptr->dmu_tbl_id.db_tab_base;
		dmu_cb_p->dmu_tbl_id.db_tab_index =
		    dmu_ptr->dmu_tbl_id.db_tab_index;
		dmu_cb_p->dmu_db_id = qeu_cb->qeu_db_id;
		dmu_cb_p->dmu_tran_id = qef_cb->qef_dmt_id;
		if (TempTable)
		{
		    chr.char_id = DMU_TEMP_TABLE;
		    chr.char_value = DMU_C_ON;
		    dmu_cb_p->dmu_char_array.data_address = (char *) &chr;
		    dmu_cb_p->dmu_char_array.data_in_size =
			sizeof(DMU_CHAR_ENTRY);
		}
		else
		{
		    dmu_cb_p->dmu_char_array.data_address = 0;
		    dmu_cb_p->dmu_char_array.data_in_size = 0;
		}
		
		ret_stat = dmf_call(DMU_DESTROY_TABLE, dmu_cb_p);
		if (DB_FAILURE_MACRO(ret_stat))
		{
		    /* ignore nonexistent tables */
		    if (dmu_cb_p->error.err_code == E_DM0054_NONEXISTENT_TABLE)
		    {
			ret_stat = E_DB_OK;
		    }
		    else
		    {
			err = dmu_cb_p->error.err_code;
			goto exit;
		    }
		}
		/* Invalidate this table out of RDF, it's gone.  Don't
		** need to do this for session temps, DMF did it.  So
		** the only other case is base-table-only delete, which
		** I think is just an upgradedb thing.  Still, it's probably
		** wise to keep RDF up to date.
		*/
		if (! TempTable)
		{
		    RDF_CB rdfcb;

		    qeu_rdfcb_init((PTR) &rdfcb, qef_cb);
		    STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, rdfcb.rdf_rb.rdr_tabid);
		    /* Unclear what to do on error, so ignore error */
		    ret_stat = rdf_call(RDF_INVALIDATE, &rdfcb);
		    if (ret_stat != E_DB_OK)
			TRdisplay("%@ RDF invalidate error %d on (%d,%d)\n",
				ret_stat,dmu_ptr->dmu_tbl_id.db_tab_base,
				dmu_ptr->dmu_tbl_id.db_tab_index);
		}
	    }
	    else
	    {
		DMF_ATTR_ENTRY	**dmu_attr;
		/* 
		** destroy the table or view and all view depending upon this
		** one. We also need to test for possible permits on this
		** table. Until then, always go through dview.
		**
		**  23-jun-92 (andre)
		**	    will always call qeu_d_cascade()
		*/
		qeuq_cb.qeuq_tabname = 
		    ((DMU_CB *)qeu_cb->qeu_d_cb)->dmu_table_name;
		STRUCT_ASSIGN_MACRO(dmt_tbl_entry.tbl_qryid, qeuq_cb.qeuq_qid);

		/*
		** set QEU_FORCE_QP_INVALIDATION to indicate to qeu_d_cascade()
		** that QPs dependent on the object being dropped and any 
		** objects that depend on it must be invalidated
		*/
		qeuq_cb.qeuq_flag_mask |= QEU_FORCE_QP_INVALIDATION;

		if (dmt_tbl_entry.tbl_status_mask & DMT_VIEW)
		{
		    obj_type = (DB_QMODE) DB_VIEW;
		}
		else if (dmt_tbl_entry.tbl_status_mask & DMT_IDX)
		{
		    obj_type = (DB_QMODE) DB_INDEX;
		}
		else
		{
		    obj_type = (DB_QMODE) DB_TABLE;

		    if (dmu_atbl_dropcol)
		    {
		       dmu_attr = (DMF_ATTR_ENTRY **)
		       		  dmu_ptr->dmu_attr_array.ptr_address;
    		       qeuq_cb.qeuq_ano	= dmu_attr[0]->attr_size;
    		       qeuq_cb.qeuq_aid	= dmu_attr[0]->attr_type;
		       qeuq_cb.qeuq_flag_mask |= QEU_DROP_COLUMN;
		       qeuq_cb.qeuq_keydropped = FALSE;
		       qeuq_cb.qeuq_keypos = 0;
		       qeuq_cb.qeuq_keyattid = 0;

    		       if (dmu_atbl_cascade)
			  qeuq_cb.qeuq_flag_mask |= QEU_DROP_CASCADE;
		    }

		}
		if (ret_stat = qeu_d_cascade(qef_cb, &qeuq_cb, obj_type,
				   &dmt_tbl_entry))
		{
		    err = qeuq_cb.error.err_code;
		    goto exit;
		}
	    }
	}
	if (opcode == DMU_CREATE_TABLE)
	{
	    /*
	    ** if this is a table creation, we may need to create default
	    ** tuples.  we will loop through all the attributes, looking
	    ** for attributes for which PSF has cooked up iidefault
	    ** tuples.  for those attributes, we will lookup the default
	    ** ids corresponding to the iidefault tuples.  if there is no
	    ** default id for that tuple, we'll allocate an id, and write
	    ** the tuple to iidefault.
	    ** The parser prevents temporary tables from having user
	    ** defaults, so skip the useless work in that case.
	    **
	    ** simplicity is our first name.
	    */

	    if (!TempTable)
	    {
		status = createDefaultTuples( &qef_rcb, dmu_ptr );
		if ( status != E_DB_OK ) 	
		{ 
		     err = qef_rcb.error.err_code;
		     goto exit; 
		}
	    }
	    /* Other create-table callers don't have a qeu cb, so set an
	    ** rcb flag meaning "temporary table" (even if that isn't
	    ** what the name looks like...)
	    */
	    qef_rcb.qef_stmt_type = 0;
	    if (TempTable)
		qef_rcb.qef_stmt_type = QEF_DGTT_AS_SELECT;
	    status = qeu_create_table(&qef_rcb, dmu_ptr,
			qeu_cb->qeu_logpart_list, tabid_count, table_id);
	    if (DB_FAILURE_MACRO(status))
	    {
		err = qef_rcb.error.err_code;
		if (qeu_cb->qeu_eflag == QEF_EXTERNAL)
		    qeu_cb->error.err_code = qef_rcb.error.err_code;
		goto exit;
	    }

	    /* 
	    ** see if we have a REGISTER TABLE query, add it to iiqrytext 
	    ** if so.  We had to wait until now because we rely on DMF to
	    ** provide us with query text id
	    */
	    status = qeu_reg_table_text(qeu_cb, &dmu_cb, qef_cb);
	    if (status != E_DB_OK)
	    {
		err = qeu_cb->error.err_code;
	    }
	}
	else if (opcode != DMU_DESTROY_TABLE)	
	{
	    i4 changing_part;
	    i4 error;
	    i4 errcode = E_QE0000_OK;

	    /* (schka24) original code did the create-default-tuples
	    ** stuff for alter table add column, but you can't specify
	    ** value defaults for that case!  so no need.
	    ** 
	    ** (coomi01) Bug 120413	    
	    ** But you are allowed to specify default values for
	    ** alter table alter column
	    */
	    if (createDfltTuple && (DMU_ALTER_TABLE == opcode))
	    {
		status = createDefaultTuples( &qef_rcb, dmu_ptr );
		if ( status != E_DB_OK ) 	
		{ 
		    err = qef_rcb.error.err_code;
		    goto exit; 
		}
	    }

	    /*
	    ** if this is a modify on a base table, be sure to recreate
	    ** the persistent indices if they are dropped.  These are 
	    ** the steps we go through:
	    **
	    **	o before modifying the base table, construct DMU_CBs
	    **	  suitable for recreating the indices.
	    **
	    **	o modify the base table.  dmf will casually destroy all
	    **	  indices on it.
	    **
	    **	o drop all views defined on the now destroyed persistent
	    **	  indices.  we must do this because with the view definitions
	    **	  are stored trees that refer to the now extinct index IDs.
	    **
	    **	o recreate the persistent indices.
	    **
	    **	o update IIDBDEPENDS tuples for which the persistent indices
	    **	  were the dependent object.  in these tuples, overwrite the
	    **	  old index IDs with the new ones we just created.
	    **
	    ** simplicity is our middle name.
	    */

	    /* Need to block modify of table/index with constraint
	    ** depenencies.
	    */
	    if( (opcode == DMU_MODIFY_TABLE ) &&
		(!ult_check_macro(&qef_cb->qef_trace,
				  QEF_TRACE_NO_DEPENDENCY_CHK,
				  &dummy1, &dummy2)) &&
		(!(dmu_cb.dmu_flags_mask & DMU_NODEPENDENCY_CHECK)) &&
                !Qef_s_cb->qef_no_dependency_chk
	      )
	    {
                DB_TAB_ID indexid;
                bool             found_one = FALSE;
		i4               error;
	        QEUQ_CB          qeuq_cb;
	        DMT_TBL_ENTRY    dmt_tbl_entry;
	        DMT_SHW_CB       dmt_show;
 
	        /* Format the DMT_SHOW request block */
	        dmt_show.type = DMT_SH_CB;
	        dmt_show.length = sizeof(DMT_SHW_CB);
	        dmt_show.dmt_char_array.data_in_size = 0;
	        dmt_show.dmt_char_array.data_out_size  = 0;
	        STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, dmt_show.dmt_tab_id);
	        dmt_show.dmt_flags_mask = DMT_M_TABLE;
	        dmt_show.dmt_db_id	= qeu_cb->qeu_db_id;
	        dmt_show.dmt_session_id = qeu_cb->qeu_d_id;
	        dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	        dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	        dmt_show.dmt_char_array.data_address = (PTR)NULL;
	        dmt_show.dmt_char_array.data_in_size = 0;
	        dmt_show.dmt_char_array.data_out_size  = 0;
	        status = dmf_call(DMT_SHOW, &dmt_show);

	        if (status != E_DB_OK &&
		    dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
	        {
		    err = dmt_show.error.err_code;
		    goto exit;
	        }
    
	        status = E_DB_OK;
    
	        qeuq_cb.qeuq_next = qeuq_cb.qeuq_prev = 0;
	        qeuq_cb.qeuq_length = sizeof (qeuq_cb);
	        qeuq_cb.qeuq_type = QEUQCB_CB;
	        qeuq_cb.qeuq_owner = (PTR)DB_QEF_ID;
	        qeuq_cb.qeuq_ascii_id = QEUQCB_ASCII_ID;
	        qeuq_cb.qeuq_db_id = qeu_cb->qeu_db_id;
	        qeuq_cb.qeuq_d_id = qeu_cb->qeu_d_id;
	        qeuq_cb.qeuq_eflag = qeu_cb->qeu_eflag;
	        qeuq_cb.qeuq_rtbl = &dmu_ptr->dmu_tbl_id;
	        qeuq_cb.qeuq_status_mask = dmt_tbl_entry.tbl_status_mask;
	        qeuq_cb.qeuq_qid.db_qry_high_time =
		    qeuq_cb.qeuq_qid.db_qry_low_time = 0;
    
                if (!memoryStreamOpened)
                {
                    /* get a memory stream in which to build DMU_CBs */
                    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
                    if ((status = qec_mopen(&ulm)) != E_DB_OK)
                    {
                        err = ulm.ulm_error.err_code;
                        goto exit;
                    }
                    memoryStreamOpened = TRUE;
                }
        
                STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, indexid);
    
                /* Check for a dependent constraint */
                status = qeu_d_check_conix(qef_cb, &qeuq_cb,
                      &ulm, &indexid, 
                      (dmt_tbl_entry.tbl_temporary) ? TRUE:FALSE,
                      &error, &found_one);
       
                if ((status != E_DB_OK) || (found_one))
                {
                    if(status != E_DB_OK)
                    {
                        err = error;
                    }
                    else
                    {
                        /* Dependent constraint found */
                        status = E_DB_ERROR;
                        qef_error(I_QE2038_CONSTRAINT_TAB_IDX_ERR, 0L, 
                               status, &error, error_block, 3,
                               (sizeof("MODIFY") - 1), "MODIFY",
                               (sizeof("modify") - 1), "modify",
                               qec_trimwhite(DB_TAB_MAXNAME,
                               dmu_ptr->dmu_table_name.db_tab_name),
                               dmu_ptr->dmu_table_name.db_tab_name);
                        err = E_QE0025_USER_ERROR;
                    }
                    goto exit;
                }
            }

	   /* If renaming a table or a column then validate 
	   ** the dependent objects on this table 
	   */
            if (dmu_atbl_rencol || dmu_atbl_rentab)
	    {
                i4               error;
	        QEUQ_CB          qeuq_cb;
                DMT_TBL_ENTRY    dmt_tbl_entry;
                DMT_SHW_CB       dmt_show;

                /* Format the DMT_SHOW request block */
                dmt_show.type = DMT_SH_CB;
                dmt_show.length = sizeof(DMT_SHW_CB);
                dmt_show.dmt_char_array.data_in_size = 0;
                dmt_show.dmt_char_array.data_out_size  = 0;
                STRUCT_ASSIGN_MACRO(dmu_ptr->dmu_tbl_id, dmt_show.dmt_tab_id);
                dmt_show.dmt_flags_mask = DMT_M_TABLE;
                dmt_show.dmt_db_id      = qeu_cb->qeu_db_id;
                dmt_show.dmt_session_id = qeu_cb->qeu_d_id;
                dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
                dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
                dmt_show.dmt_char_array.data_address = (PTR)NULL;
                dmt_show.dmt_char_array.data_in_size = 0;
                dmt_show.dmt_char_array.data_out_size  = 0;
                status = dmf_call(DMT_SHOW, &dmt_show);

                if (status != E_DB_OK &&
                    dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
                {
                    err = dmt_show.error.err_code;
                    goto exit;
                }

		status = E_DB_OK;

                qeuq_cb.qeuq_next = qeuq_cb.qeuq_prev = 0;
                qeuq_cb.qeuq_length = sizeof (qeuq_cb);
                qeuq_cb.qeuq_type = QEUQCB_CB;
                qeuq_cb.qeuq_owner = (PTR)DB_QEF_ID;
                qeuq_cb.qeuq_ascii_id = QEUQCB_ASCII_ID;
                qeuq_cb.qeuq_db_id = qeu_cb->qeu_db_id;
                qeuq_cb.qeuq_d_id = qeu_cb->qeu_d_id;
                qeuq_cb.qeuq_eflag = qeu_cb->qeu_eflag;
                qeuq_cb.qeuq_rtbl = &dmu_ptr->dmu_tbl_id;
                qeuq_cb.qeuq_status_mask = dmt_tbl_entry.tbl_status_mask;
                qeuq_cb.qeuq_qid.db_qry_high_time =
                    qeuq_cb.qeuq_qid.db_qry_low_time = 0;

		if (!memoryStreamOpened)
                {
                    /* get a memory stream in which to build DMU_CBs */
                    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
                    if ((status = qec_mopen(&ulm)) != E_DB_OK)
                    {
                        err = ulm.ulm_error.err_code;
                        goto exit;
                    }
                    memoryStreamOpened = TRUE;
                }

		if (dmu_atbl_rencol)
                {
                    dmu_attr = (DMF_ATTR_ENTRY **)
                                  dmu_ptr->dmu_attr_array.ptr_address;
                    MEcopy(dmu_attr[1]->attr_name.db_att_name, DB_ATT_MAXNAME, column_name);
                    column_name[DB_ATT_MAXNAME] = 0;

                    qeuq_cb.qeuq_ano = dmu_attr[0]->attr_size;
                    qeuq_cb.qeuq_aid = dmu_attr[0]->attr_type;
                    qeuq_cb.qeuq_flag_mask |= QEU_RENAME_COLUMN;

		    ret_stat  = qeu_renameValidate( qef_cb, &qeuq_cb, &ulm, error_block, 
				&dmt_tbl_entry, TRUE, dmu_attr); 
                }
		else
		{
		    ret_stat  = qeu_renameValidate( qef_cb, &qeuq_cb, 
				&ulm, error_block, &dmt_tbl_entry, FALSE, NULL); 
		}

		if (ret_stat)
		{
		   err = error_block->err_code;
		   if (err == E_QE016C_RENAME_TAB_HAS_OBJS)
		   {
		       qef_error( E_QE016C_RENAME_TAB_HAS_OBJS,
                                  0L, status, &error, error_block, 2,
                                  qec_trimwhite(DB_TAB_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_name.db_tab_name ),
                                  (char *)&dmt_tbl_entry.tbl_name.db_tab_name,
                                  qec_trimwhite(DB_OWN_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_owner.db_own_name ),
                                  (char *)&dmt_tbl_entry.tbl_owner.db_own_name );
		   }
		   else if (err == E_QE0181_RENAME_COL_HAS_OBJS)
	 	   {
		       qef_error ( E_QE0181_RENAME_COL_HAS_OBJS,
                                  0L, status, &error, error_block, 3,
				  qec_trimwhite(DB_ATT_MAXNAME, (char *) &column_name), 
				  	   (char *)&column_name,
                                  qec_trimwhite(DB_TAB_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_name.db_tab_name ),
                                  (char *)&dmt_tbl_entry.tbl_name.db_tab_name,
                                  qec_trimwhite(DB_OWN_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_owner.db_own_name ),
                                  (char *)&dmt_tbl_entry.tbl_owner.db_own_name );
		   }
		   else
                   {
		       qef_error( err,
                                  0L, status, &error, error_block, 2,
                                  qec_trimwhite(DB_TAB_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_name.db_tab_name ),
                                  (char *)&dmt_tbl_entry.tbl_name.db_tab_name,
                                  qec_trimwhite(DB_OWN_MAXNAME,
                                           (char *) &dmt_tbl_entry.tbl_owner.db_own_name ),
                                  (char *)&dmt_tbl_entry.tbl_owner.db_own_name );
		   }
                   err = E_QE0025_USER_ERROR;
		   goto exit;
		}
	    }

	    /* 
	    ** The 'rules' for rebuilding persistent indexes as part of a modify operation are:
	    ** 
	    ** 1. It is a DMU_MODIFY_TABLE (which excludes 'to relocate' which is DMU_RELOCATE_TABLE) 
	    ** AND 
	    ** 2. Modify is on a table, not on an index 
	    ** AND EITHER
	    ** 3(a) Modify is a structure change (eg 'to btree', 'to heap' etc) OR
	    ** OR
	    ** 3(b) Modify is 'to truncate'.
	    ** 
	    ** Note that only persistent indexes will be rebuilt.
	    */
	    if ( opcode == DMU_MODIFY_TABLE && dmu_ptr->dmu_tbl_id.db_tab_index <= 0 )
	    {
	        chr = * ((DMU_CHAR_ENTRY *) dmu_cb.dmu_char_array.data_address);
		if (chr.char_id == DMU_STRUCTURE
		  || chr.char_id == DMU_TRUNCATE )
		{
		    rebuild_pindex = TRUE;
		}

	    }

	    /* There are several reasons why we might want a memory stream for
	    ** temporary information.  It's simplest to just open a stream if 
	    ** there is any remote possibility of needing it:
	    ** - if we might need to track persistent indexes to be reconstructed
	    ** - if we might need a partitioning ppchar_array
	    */
	    if (! memoryStreamOpened
	      && (rebuild_pindex
		  || dmu_ptr->dmu_part_def != NULL
		  || ( (opcode == DMU_MODIFY_TABLE || opcode == DMU_RELOCATE_TABLE)
			&& dmu_ptr->dmu_nphys_parts > 0)
		 ) )
	    {
		/* get a memory stream for work areas */
		STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
		if ((status = qec_mopen(&ulm)) != E_DB_OK)
		{
		    err = ulm.ulm_error.err_code;
		    goto exit;
		}
		memoryStreamOpened = TRUE;
	    }

	    if (rebuild_pindex)
	    {
		changing_part = NO_CHANGE;

		/* If base table is being partitioned, the default for
		** its persistent indexes is to become global.  If the
		** base table is becoming non-partitioned, we have to
		** make all indexes regular indexes.  If the base table
		** isn't being repartitioned, or is already partitioned,
		** we don't change the global-ness of indexes.
		** Caution: a partdef must be present for this to be any
		** kind of repartitioning modify.  If there's no partdef,
		** this is a NO_CHANGE modify.
		*/
		if (dmu_ptr->dmu_nphys_parts == 0 && dmu_ptr->dmu_part_def != NULL)
		    changing_part = BECOMING_PARTITIONED;
		else if (dmu_ptr->dmu_nphys_parts > 0
		      && (dmu_ptr->dmu_part_def != NULL && dmu_ptr->dmu_part_def->ndims == 0) )
		    changing_part = BECOMING_NONPARTITIONED;

		/*
		** build an array of DMU_CBs for recreating the persistent
		** indices on this table
		*/

		status = collectPersistentIndices( &ulm, &qef_rcb,
		    dmu_ptr->dmu_tbl_id.db_tab_base,	/* base table ID */
		    changing_part,
		    &persistentIndexDMU_CBs, &oldPersistentIndexIDs,
		    &numberOfPersistentIndices );
		if ( status != E_DB_OK )
		{
		    err = dmu_ptr->error.err_code;
		    goto exit;
		}

		/* link persistent index DMU_CBs to MODIFY DMU_CB */
		if (ult_check_macro(&qef_cb->qef_trace, 
			    QEF_TRACE_NO_PAR_INDEX_70, &val_1, &val_2))
		    dmu_ptr->dmu_flags_mask |= DMU_NO_PAR_INDEX;
		if (numberOfPersistentIndices)
		    dmu_ptr->dmu_flags_mask |= DMU_PIND_CHAINED;
		dmu_ptr->q_next = (PTR) persistentIndexDMU_CBs;
	    }


	    /* For MODIFY or RELOCATE on a partitioned master, we might have to
	    ** figure out what gets done and where it goes.  We might also have
	    ** to pre-create extra partitions in a repartitioning modify.
	    ** Modify prep might substitute a trimmed-down partition def
	    ** that DMF will like better, so save our complete one for later.
	    */
	    full_part_def = dmu_ptr->dmu_part_def;
	    full_partdef_size = dmu_ptr->dmu_partdef_size;
	    if ((dmu_ptr->dmu_nphys_parts > 0 || dmu_ptr->dmu_part_def != NULL)
	      && (opcode == DMU_MODIFY_TABLE || opcode == DMU_RELOCATE_TABLE) )
	    {
		status = qeu_modify_prep(qeu_cb, dmu_ptr, &ulm, &pp_array, &err);
		if (DB_FAILURE_MACRO(status))
		    goto exit;
	    }

	    /* now do the actual DMF operation */

	    status = dmf_call(opcode, dmu_ptr);

	    errdmu_ptr = dmu_ptr;
	    dmu_ptr->dmu_part_def = full_part_def;  /* Put original back */
	    dmu_ptr->dmu_partdef_size = full_partdef_size;
	    if ( status != E_DB_OK && (opcode == DMU_PINDEX_TABLE ||
			(rebuild_pindex && dmu_ptr->dmu_flags_mask & DMU_PIND_CHAINED)) )
	    {
		/* Check each control block for the one with the error */
		for (tdmu_ptr = dmu_ptr; tdmu_ptr; 
		    tdmu_ptr = ( DMU_CB * )tdmu_ptr->q_next)
		{
		    if (tdmu_ptr->error.err_code)
		    {
			errdmu_ptr = tdmu_ptr;
			break;
		    }
		}
	    }

	    errcode = errdmu_ptr->error.err_code;

	    if (status != E_DB_OK &&
		errcode != E_DM0114_FILE_NOT_FOUND)
	    {
		i4	    err_num;
		i4	    keysize;
		i4	    maxkeysize;

		if (qeu_cb->qeu_eflag == QEF_EXTERNAL)
		{
		    switch (errcode)
		    {
			DMF_ATTR_ENTRY	**dmu_attr;
			DB_LOC_NAME	*loc;
			i4		loc_idx;

			case E_DM0078_TABLE_EXISTS:
			    if ((opcode == DMU_INDEX_TABLE) ||
				     (opcode == DMU_PINDEX_TABLE))
				qef_error(5102L, 0L, status, &error, 
				error_block, 2, 
				(sizeof ("CREATE INDEX") - 1), "CREATE INDEX",
				sizeof(dmu_ptr->dmu_index_name), 
				(PTR) &dmu_ptr->dmu_index_name);
			    else
				break;
			    errcode = E_QE0025_USER_ERROR;
			    break;

			case E_DM001D_BAD_LOCATION_NAME:
			    if (opcode != DMU_INDEX_TABLE &&
				opcode != DMU_PINDEX_TABLE &&
				opcode != DMU_RELOCATE_TABLE)
				break;
			    loc_idx = errdmu_ptr->error.err_data;
			    loc = ((DB_LOC_NAME *)
			        (errdmu_ptr->dmu_location.data_address)) + 
				loc_idx;
			    qef_error(5113L, 0L, status, &error, error_block, 
				      1, sizeof(DB_LOC_NAME), (PTR)(loc));
			    errcode = E_QE0025_USER_ERROR;
			    break;

			case E_DM0187_BAD_OLD_LOCATION_NAME:
			    if (opcode != DMU_RELOCATE_TABLE)
				break;
				
			    loc_idx = dmu_ptr->error.err_data;
			    loc = ((DB_LOC_NAME *)
			        (dmu_ptr->dmu_olocation.data_address))+ loc_idx;
                            /* b92641. Report BAD_OLD_LOCATION not */
                            /* BAD_LOCATION as this is misleading. */
                            qef_error(5119L, 0L, status, &error, error_block,
                                      2, sizeof(DB_LOC_NAME), (PTR)(loc),
                                      sizeof(DB_TAB_NAME),
                                      (char *)&dmu_ptr->dmu_table_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;

			case E_DM005D_TABLE_ACCESS_CONFLICT:
			    qef_error(5005L, 0L, status, &error, 
			    error_block, 1,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    dmu_ptr->dmu_table_name.db_tab_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM0045_DUPLICATE_KEY:
			    if ((opcode == DMU_INDEX_TABLE) || 
				(opcode == DMU_PINDEX_TABLE))
			    {
				/*
				** If we are creating a UNIQUE index on behalf
				** of a UNIQUE CONSTRAINT and we get a
				** duplicate key error, then we want to be as
				** specific as possible.  Tell the user that
				** the CONSTRAINT could not be created because
				** the table contained duplicates on the
				** user specified key.
				*/
				if ( qef_cb->qef_callbackStatus &
				     QEF_MAKE_UNIQUE_CONS_INDEX )
				{
				    DMT_TBL_ENTRY	table_entry;
				    DMT_SHW_CB		dmt_show;

				    /* get base table's name */

				    dmt_show.type = DMT_SH_CB;
				    dmt_show.length = sizeof(DMT_SHW_CB);
				    dmt_show.dmt_session_id = qeu_cb->qeu_d_id;
				    dmt_show.dmt_db_id = qeu_cb->qeu_db_id;
				    dmt_show.dmt_char_array.data_in_size = 0;
				    dmt_show.dmt_char_array.data_out_size  = 0;
				    dmt_show.dmt_tab_id.db_tab_base =
					dmu_ptr->dmu_tbl_id.db_tab_base;
				    dmt_show.dmt_tab_id.db_tab_index = 0;
				    dmt_show.dmt_flags_mask = DMT_M_TABLE;
				    dmt_show.dmt_table.data_address =
					(PTR) &table_entry;
				    dmt_show.dmt_table.data_in_size =
					sizeof(DMT_TBL_ENTRY);
				    dmt_show.dmt_char_array.data_address =
					(PTR)NULL;
				    dmt_show.dmt_char_array.data_in_size = 0;
				    dmt_show.dmt_char_array.data_out_size  = 0;

				    ( VOID ) dmf_call(DMT_SHOW, &dmt_show);

				    qef_error( 
					E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT,
					0L, status, &error, error_block, 1,
					qec_trimwhite(DB_TAB_MAXNAME,
					    table_entry.tbl_name.db_tab_name ), 
			    		table_entry.tbl_name.db_tab_name );
					errcode = E_QE0025_USER_ERROR;
					break;
				}
				else { err_num = 5522; }
			    }
			    else /* MODIFY */
				err_num = 5521;
			    qef_error(err_num, 0L, status, &error, 
			    error_block, 0);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM005F_CANT_INDEX_CORE_SYSCAT:
			    qef_error(5305L, 0L, status, &error, 
			    error_block, 1,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    dmu_ptr->dmu_table_name.db_tab_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM005A_CANT_MOD_CORE_STRUCT:
			    qef_error(5533L, 0L, status, &error, 
			    error_block, 1,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    dmu_ptr->dmu_table_name.db_tab_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			    
			case E_DM007D_BTREE_BAD_KEY_LENGTH:
			    if ((opcode == DMU_INDEX_TABLE) ||
			    	(opcode == DMU_PINDEX_TABLE))
				err_num = 5538;
			    else /* MODIFY */
				err_num = 5524;

			    /*
			    ** Maximum btree key length depends on 
			    ** page size, page type, number of keys,
			    ** and server configuration
			    ** DMF returns the current maximum in dmu_tup_cnt
			    */
			    maxkeysize = errdmu_ptr->dmu_tup_cnt;

			    keysize = errdmu_ptr->error.err_data;
			    qef_error(err_num, 0L, status, &error, 
			    error_block, 3,
			    qec_trimwhite(DB_TAB_MAXNAME,
				errdmu_ptr->dmu_table_name.db_tab_name),
			    errdmu_ptr->dmu_table_name.db_tab_name,
			    sizeof (keysize), (PTR) &keysize, 
			    sizeof(maxkeysize), (PTR) &maxkeysize);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM010F_ISAM_BAD_KEY_LENGTH:
			    if ((opcode == DMU_INDEX_TABLE) ||
			    	(opcode == DMU_PINDEX_TABLE))
				err_num = 5537;
			    else /* MODIFY */
				err_num = 5508;
			    maxkeysize = (DB_MAXTUP +2 -2 *2)/2;
			    keysize = errdmu_ptr->error.err_data;
			    qef_error(err_num, 0L, status, &error, 
			    error_block, 3,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    dmu_ptr->dmu_table_name.db_tab_name,
			    sizeof (keysize), (PTR) &keysize, 
			    sizeof(maxkeysize), (PTR) &maxkeysize);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			
			case E_DM0110_COMP_BAD_KEY_LENGTH:
			    if ((opcode == DMU_INDEX_TABLE) ||
			    	(opcode == DMU_PINDEX_TABLE))
				err_num = 5539;
			    else /* MODIFY */
				err_num = 5536;
			    qef_error(err_num, 0L, status, &error, 
			    error_block, 1,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    dmu_ptr->dmu_table_name.db_tab_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM0111_MOD_IDX_UNIQUE:
			    qef_error(5541L, 0L, status, &error, 
			    error_block, 1,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    &dmu_ptr->dmu_table_name);
                            errcode = E_QE0025_USER_ERROR;
                            break;
                        case E_DM0181_PROD_MODE_ERR:
                            qef_error(errcode, 0L, status, &error, 
					error_block, 0);
			    errcode = E_QE0025_USER_ERROR;
			    break;
 			case E_DM004B_LOCK_QUOTA_EXCEEDED:
 				qeu_cb->error.err_code = errcode; 
 				break;
			case E_DM0157_NO_BMCACHE_BUFFERS:
			    qef_error(errcode, 0L, status, &error, error_block,
					0);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM00A6_ATBL_COL_INDEX_KEY:
			case E_DM00A7_ATBL_COL_INDEX_SEC:
		       	    dmu_attr = (DMF_ATTR_ENTRY **)
		       		       dmu_ptr->dmu_attr_array.ptr_address;
			    qef_error(errcode, 0L, status, &error,
			    error_block, 2,
			    qec_trimwhite(DB_ATT_MAXNAME,
					  dmu_attr[0]->attr_name.db_att_name),
			    &dmu_attr[0]->attr_name.db_att_name,
			    qec_trimwhite(DB_TAB_MAXNAME,
					  dmu_ptr->dmu_table_name.db_tab_name),
			    &dmu_ptr->dmu_table_name.db_tab_name);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM0103_TUPLE_TOO_WIDE:
 				qeu_cb->error.err_code = errcode; 
 				break;
			case E_DM0183_CANT_MOD_TEMP_TABLE:
 				qeu_cb->error.err_code = errcode; 
 				break;
			case E_DM0200_MOD_TEMP_TABLE_KEY:
				qeu_cb->error.err_code = errcode;
				break;
			case E_DM006A_TRAN_ACCESS_CONFLICT:
 				qeu_cb->error.err_code = errcode; 
 				break;
			case E_DM0065_USER_INTR:
			    qeu_cb->error.err_code = errcode;
			    break;
			case E_DM0189_RAW_LOCATION_INUSE:
			    loc_idx = errdmu_ptr->error.err_data;
			    loc = ((DB_LOC_NAME *)
			        (errdmu_ptr->dmu_location.data_address)) + 
				loc_idx;
			    qef_error(E_DM0189_RAW_LOCATION_INUSE, 0L, 
					status, &error, error_block, 
				      1, sizeof(DB_LOC_NAME), (PTR)(loc));
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM0190_RAW_LOCATION_OCCUPIED:
			    loc_idx = errdmu_ptr->error.err_data;
			    loc = ((DB_LOC_NAME *)
			        (errdmu_ptr->dmu_location.data_address)) + 
				loc_idx;
			    qef_error(E_DM0190_RAW_LOCATION_OCCUPIED, 0L, 
					status, &error, error_block, 
				      1, sizeof(DB_LOC_NAME), (PTR)(loc));
			    errcode = E_QE0025_USER_ERROR;
			    break;
			case E_DM0191_RAW_LOCATION_MISSING:
			    qef_error(E_DM0191_RAW_LOCATION_MISSING, 0L, 
					status, &error, error_block, 0);
			    errcode = E_QE0025_USER_ERROR;
			    break;
			default:
			    qeu_cb->error.err_code = errcode; 
			    break;
		    }
		}
		err = errcode;
	    }
	    else
	    {
		/* If we were doing a repartitioning modify, there might be
		** partitions no longer in use that need to be dropped.
		** Also, it would probably be a good idea to update the
		** partitioning catalogs.
		*/
		if (opcode == DMU_MODIFY_TABLE && dmu_ptr->dmu_part_def != NULL)
		{
		    status = qeu_modify_finish(qef_cb, dmu_ptr, &pp_array);
		    if (DB_FAILURE_MACRO(status))
		    {
			err = dmu_ptr->error.err_code;
			goto exit;
		    }
		}

		/* 
		** see if we have a REGISTER INDEX query, add it to iiqrytext 
		** if so.  We had to wait until now because we rely on DMF to
		** provide us with query text id
		*/
		status = qeu_reg_table_text(qeu_cb, &dmu_cb, qef_cb);
		if (status != E_DB_OK)
		{
		    err = qeu_cb->error.err_code;
		    goto  exit;
		}

		if ( rebuild_pindex )
		{
		    /*
		    ** If we are in here, then any persistent indexes have been rebuilt. 
		    ** What we need to do now is to update the catalogs with the data 
		    ** about these newly-built persistent indices.
		    */

		    if ( numberOfPersistentIndices > 0 )
		    {
		        /* first, get the IDs for the new persistent indexes */

		        status = getNewIndexIDs(  &qef_rcb, &ulm, persistentIndexDMU_CBs, 
			    numberOfPersistentIndices, &newPersistentIndexIDs );
		        if ( status != E_DB_OK )
			{
			    err = qef_rcb.error.err_code;
			    goto exit;
			}

		        /*
		        ** for each recreated index, update the dependent object
		        ** id in IIDBDEPENDS tuples that refer to it
		        */

		        status = qeu_updateDependentIDs( &qef_rcb, &ulm,
			    oldPersistentIndexIDs, newPersistentIndexIDs,
			    numberOfPersistentIndices, ( i4 ) DB_INDEX );
		        if ( status != E_DB_OK )
			{
			    err = qef_rcb.error.err_code;
			    goto exit;
			}

		        /* drop all views on the persistent indices */

		        status = dropDependentViews( &qef_rcb,
			    oldPersistentIndexIDs, numberOfPersistentIndices );
		        if ( status != E_DB_OK )
			{
			    err = qef_rcb.error.err_code;
			    goto exit;
			}

		    }	/* end if there are persistent indices */

		}	/* end if this is a base table modify */

		status = E_DB_OK;
		err = E_QE0000_OK;
	    }
	}
    }

exit:

    /* Grab rowcount before returning memory, the dmucb's might be
    ** in the temporary memory stream
    */
    *row_count = -1;
    if (opcode == DMU_INDEX_TABLE || opcode == DMU_PINDEX_TABLE ||
	    opcode == DMU_MODIFY_TABLE)
    {
	/* More no-rowcount cases:
	** special system-generated index stuff
	** modify that's not a structuring modify, or to truncated
	*/
	if (! suppress_rowcount(opcode, dmu_ptr)
	  && (opcode != DMU_MODIFY_TABLE
	      || ((DMU_CHAR_ENTRY *) dmu_ptr->dmu_char_array.data_address)->char_id == DMU_STRUCTURE
	      || ((DMU_CHAR_ENTRY *) dmu_ptr->dmu_char_array.data_address)->char_id == DMU_TRUNCATE) )
	    *row_count = dmu_ptr->dmu_tup_cnt;
    }

    if ( memoryStreamOpened == TRUE )
    {
	ulm_closestream(&ulm);
    }

    if (table_opened && (stat = dmf_call(DMT_CLOSE, &dmt_cb)) != E_DB_OK)
    {
	status = stat;
	err = dmt_cb.error.err_code;
    }
    table_opened = FALSE;

    /* delete the QSF object */
    if (qeu_cb->qeu_d_cb == 0 && qsf_obj)
    {
	if (stat = qsf_call(QSO_DESTROY, &qsf_rcb))
	{
	    status = stat;
	    err = qsf_rcb.qsf_error.err_code;
	}
    }
    /* 
    ** process error if called externally (external to QEF).
    ** otherwise, other parts of QEF will process errors and
    ** transactions.
    **
    ** note that if ret_stat is set, the error has been processed 
    ** by qeuqry.c
    */
    if (caller == QEF_EXTERNAL)
    {
	oqef_rcb = qef_cb->qef_rcb;
	qef_cb->qef_rcb = &qef_rcb;
	qef_rcb.qef_cb  = qef_cb;
	qef_rcb.qef_eflag = qeu_cb->qeu_eflag;

	if (status || ret_stat)
	{
	    /* handle error situation */
	    if (status)
	    {
		i4	err1;

		qef_error(err, 0L, status, &err1, error_block, 0);
		err = error_block->err_code;
	    }
	    
	    /* abort the transaction if forced-abort errors */

	    if (qef_cb->qef_abort)
	    {
		qef_rcb.qef_spoint = (DB_SP_NAME *)NULL;
		stat = qet_abort(qef_cb);
		if (stat)
		{
		    status = stat;
		    err = qef_rcb.error.err_code;
		}
		/* clean up any cursor that might be opened */
		stat = qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);
		if (stat)
		{
		    status = stat;
		    err = qef_rcb.error.err_code;
		}
	    }

	    /* all other errors, abort the transaction if we started it */

	    else if (tran_started)
	    {
		if ((qef_cb->qef_stat == QEF_ITRAN) ||
		    (qef_cb->qef_stat == QEF_SSTRAN))
		{
		    qef_rcb.qef_spoint = (DB_SP_NAME *)NULL;
		    stat = qet_abort(qef_cb);
		}
		else if (qef_cb->qef_stat != QEF_NOTRAN)
		{
		    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS) /* distrib?*/
			qef_rcb.qef_spoint = (DB_SP_NAME *)NULL;
		    else
		    {
			qef_rcb.qef_spoint = &Qef_s_cb->qef_sp_savepoint;
			stat = qet_abort(qef_cb);
		    }
		}
		if (stat)
		{
		    status = stat;
		    err = qef_rcb.error.err_code;
		}
		/* clean up any cursor that might be opened */
		stat = qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);
		if (stat)
		{
		    status = stat;
		    err = qef_rcb.error.err_code;
		}
	    }
	}
	else
	{
	    /* handle transaction state for normal completion */
	    if (tran_started)
	    {
		if ((qef_cb->qef_stat == QEF_MSTRAN) &&
		    (qef_cb->qef_open_count == 0))
		{
		    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
			stat = E_DB_OK;	    /* no internal savepoint in distr */
		    else
		    {
			qef_rcb.qef_spoint = &Qef_s_cb->qef_sp_savepoint;
			stat = qet_savepoint(qef_cb);
		    }
		}
		else
		{
		    stat = qet_commit(qef_cb);
		    /* clean up any cursor that might be opened */
		    status = qee_cleanup(&qef_rcb, (QEE_DSH **)NULL);
		}
		if (stat)
		{
		    status = stat;
		    err = qef_rcb.error.err_code;
		}
	    }
	}
	qef_cb->qef_rcb = oqef_rcb;
    }

    error_block->err_code = err;
    return ((ret_stat>status)? ret_stat : status);
}

/*{
** Name: QEU_QUERY      - perform a set query operation
**
** Description:
**      Execute a standard query plan stored in QSF memory.
** This call is the same as the QEQ_QUERY call except that the
** session control block is not passed in. 
**
** Inputs:
**	qeu_cb
**	    .qeu_eflag		    designate error handling semantis
**				    for user errors.
**		QEF_INTERNAL	    return error code.
**		QEF_EXTERNAL	    send message to user.
**	    .qeu_d_id		dmf session id
**	    .qeu_db_id		database id
**          .qeu_qp             name of query plan to execute
**				used if handle is zero.
**	    .qeu_qso		handle of query plan
**          .qeu_pcount         number of input rows
**          .qeu_param		parameters to query
**          .qeu_count          number of output rows
**          .qeu_output         base of output row buffer
**
** Outputs:
**      qeu_cb
**          .qeu_count              number of output rows
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE001A_DATA_TYPE_OVERFLOW
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0009_NO_PERMISSION
**                                  E_QE0012_DUPLICATE_KEY
**                                  E_QE0010_DUPLICATE_ROW
**                                  E_QE0011_AMBIGUOUS_REPLACE
**                                  E_QE0004_NO_TRANSACTION
**                                  E_QE000D_NO_MEMORY_LEFT
**                                  E_QE000E_ACTIVE_COUNT_EXCEEDED
**                                  E_QE000F_OUT_OF_OTHER_RESOURCES
**                                  E_QE0014_NO_QUERY_PLAN
**                                  E_QE0015_NO_MORE_ROWS
**				    E_QE0024_TRANSACTION_ABORTED
**				    E_QE0034_LOCK_QUOTA_EXCEEDED
**				    E_QE0035_LOCK_RESOURCE_BUSY
**				    E_QE0036_LOCK_TIMER_EXPIRED
**				    E_QE002A_DEADLOCK
**      Returns:
**          E_DB_OK                 
**          E_DB_WARN            
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**      none
**
** History:
**      27-may-86 (daved)
**          written
**	19-apr-89 (paul)
**	    Changed qeq_query calling sequence for handling cursors.
**	    Must update qeq_query call in this module.
**      26-jul-89 (neil)
**          One more update to qeq_query call (qef_intstate).
**      08-jun-93 (rblumer)
**          add special code to handle new flag from PSF: QEU_WANT_USER_XACT.
**          This flag tells us to start a user transaction if we are in an
**          internal transaction.
**      13-sep-93 (rblumer)
**          remove special code for obsolete QEU_WANT_USER_XACT flag.  We have
**          moved the ALTER TABLE constraint-verification code from PSF to QEF,
**          so there is no longer any need for this hack to force a query to be
**          part of a user transaction.
*/
DB_STATUS
qeu_query(
QEF_CB		   *qef_cb,
QEU_CB             *qeu_cb)
{
    QEF_RCB	qef_rcb;
    DB_STATUS	status;

    MEfill(sizeof(qef_rcb), NULLCHAR, (PTR) &qef_rcb);

    qef_rcb.qef_next	= qef_rcb.qef_prev = 0;
    qef_rcb.qef_length	= sizeof (QEF_RCB);
    qef_rcb.qef_type	= QEFRCB_CB;
    qef_rcb.qef_owner	= (PTR)DB_QEF_ID;
    qef_rcb.qef_ascii_id= QEFRCB_ASCII_ID;
    qef_rcb.qef_eflag	= qeu_cb->qeu_eflag;
    qef_rcb.qef_sess_id	= qeu_cb->qeu_d_id;
    qef_rcb.qef_cb	= qef_cb;
    qef_rcb.qef_pcount	= qeu_cb->qeu_pcount;
    qef_rcb.qef_param	= qeu_cb->qeu_param;
    qef_rcb.qef_rowcount= qeu_cb->qeu_count;
    qef_rcb.qef_output	= qeu_cb->qeu_output;
    qef_rcb.qef_qso_handle = qeu_cb->qeu_qso;
    qef_rcb.qef_intstate= 0;
    MEcopy((PTR)&qeu_cb->qeu_qp, sizeof (DB_CURSOR_ID), 
	    (PTR)&qef_rcb.qef_qp);
    qef_rcb.qef_modifier= 0;
    qef_rcb.qef_qacc	= QEF_UPDATE;
    qef_rcb.qef_db_id	= qeu_cb->qeu_db_id;	/* database id */
    qef_rcb.qef_sess_id	= qeu_cb->qeu_d_id;	/* session id */
    qef_cb->qef_rcb	= &qef_rcb;
    
    /* perform the query */
    status = qeq_query(&qef_rcb, QEQ_QUERY);
    
    qeu_cb->qeu_count = qef_rcb.qef_rowcount;
    STRUCT_ASSIGN_MACRO(qef_rcb.error, qeu_cb->error);
    return (status);
}

/*{
**  Name: QEU_REG_TABLE_TEXT -- put text of a REGISTER TABLE statement onto
**	iiqrytext.
**  
**  Description:
**	Check whether the current query is a REGISTER TABLE.  If so, set up
**	a qeuq_cb, put the query text into tuple structures, and write the
**	tuples to iiqrytext.
**  
**  Inputs:
**	qeu_cb.
**	    qeu_d_op:	    the token for the query verb, CREATE_TABLE
**			    for a REGISTER TABLE.
**	dmu_cb.
**	    dmu_gw_id:	    identifier for which type of gateway, if any.
**	    dmu_qry_id	    text id computed by dm2u_create()
**	qef_cb:		    passed on to qeu_tuple_to_iiqrytext().
**               
**  Outputs:
**	none.
**
**  Returns:
**	status:		    E_DB_OK, unless lower levels return
**			    error status to us.
**
**  Exceptions:
**	none.
**  
**  Side Effects:
**	Qeuq_cb is allocated on stack and set up.  Tuples get set up and
**	appended to iiqrytext.
**  
**  History:
**	13-apr-90 (fourt)
**	    written, following outline of CREATE VIEW.
**	24-apr-90 (linda)
**	    Add logic to handle REGISTER INDEX statement as well as REGISTER
**	    TABLE.
**	24-apr-90 (fourt)
**	    Put qryid into dmu_cb, so it'll get into iirelation and we can
**	    join with iiqrytext on it for iiregistrations view.
**      20-apr-92 (schang)
**          fix GW bug 42825
**          properly enclose allocated memory with open and close, otherwise
**          memory may be overwritten before being referenced
**	27-oct-93 (andre)
**	    instead of us telling DMF the query text id, DMF computes it based
**	    on id of the new object (which has been created by the time this
**	    function has been called.)
**	10-Jan-2001 (jenjo02)
**	    Don't return without closing memory stream!
*/

static  DB_STATUS
qeu_reg_table_text(
QEU_CB  *qeu_cb,
DMU_CB  *dmu_cb,
QEF_CB  *qef_cb)
{
    RDD_QDATA	qef_iiqrydata;
    QEUQ_CB	qeuq_cb;
    DB_STATUS	status;
    ULM_RCB     ulm;
    GLOBALREF	    QEF_S_CB	  *Qef_s_cb;

    /*
    **  Check if we have a REGISTER TABLE/INDEX command for a gateway.  If so,
    **  the operation in the qeu_cb will be CREATE TABLE (sic) for REG TAB
    **  or INDEX TABLE for REG IND, and the dmu_cb will claim we have a
    **  Gateway (and will identify its type).
    */
    if ((qeu_cb->qeu_d_op != DMU_CREATE_TABLE)
	&&
	(qeu_cb->qeu_d_op != DMU_INDEX_TABLE))
	return (E_DB_OK);
    if (dmu_cb->dmu_gw_id == DMGW_NONE)
	return (E_DB_OK);

    /*
    **  It's probably some kind of error if it doesn't claim to be an RMS
    **  Gateway (i.e. dmu_gw_id != DMGW_RMS), but what?  Ignore for now --
    */


    /* Initialize a qeuq_cb */
    qeuq_cb.qeuq_next	= 0;
    qeuq_cb.qeuq_prev	= 0;
    qeuq_cb.qeuq_length	= sizeof (qeuq_cb);
    qeuq_cb.qeuq_type	= QEUQCB_CB;
    qeuq_cb.qeuq_owner	= (PTR)DB_QEF_ID;
    qeuq_cb.qeuq_ascii_id = QEUQCB_ASCII_ID;
    qeuq_cb.qeuq_db_id	= qeu_cb->qeu_db_id;
    qeuq_cb.qeuq_d_id	= qeu_cb->qeu_d_id;
    qeuq_cb.qeuq_qid.db_qry_high_time	= 0;
    qeuq_cb.qeuq_qid.db_qry_low_time	= 0;
    qeuq_cb.qeuq_eflag	= qeu_cb->qeu_eflag;
    qeuq_cb.qeuq_rtbl = &dmu_cb->dmu_tbl_id;
	/* announce no tuples of any type now existant */
    qeuq_cb.qeuq_ct = 0;
    qeuq_cb.qeuq_ci = 0;
    qeuq_cb.qeuq_cp = 0;
    qeuq_cb.qeuq_cq = 0;  /* iiqrytext tups -- reset below */
    qeuq_cb.qeuq_cr = 0;
    qeuq_cb.qeuq_cagid = 0;
    qeuq_cb.qeuq_cdbpriv = 0;
	/* zero protection, integrity, attribute numbers */
    qeuq_cb.qeuq_pno = 0;
    qeuq_cb.qeuq_ino = 0;
    qeuq_cb.qeuq_ano = 0;

    /*
    **  Convert the text string into iiqrytext tuples.  Recall that we're
    **  using qeu_cb->qeu_qso to point to a C string containing the text.
    */
    /* schang 04/20/92 : open a memory stream for our tuples */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    if ((status = qec_mopen(&ulm)) != E_DB_OK)
    {
	qeu_cb->error.err_code = E_QE001E_NO_MEM; 
	return(status);
    }

    status = qeu_qrytext_to_tuple(qeu_cb, DB_REG_IMPORT, &qef_iiqrydata,
	&dmu_cb->dmu_qry_id, &ulm);
    if ( status == E_DB_OK )
    {
	qeuq_cb.qeuq_qry_tup = qef_iiqrydata.qrym_data;
	qeuq_cb.qeuq_cq = qef_iiqrydata.qrym_cnt;  /* count of tuples */

	/* put the tuples out onto iiqrytext */
	status = qeu_tuple_to_iiqrytext(qeu_cb, &qeuq_cb, qef_cb);
    }

    /* schang : 04/20/92  shut down the mem stream */
    ulm_closestream(&ulm);

    return (status);
}


/*{
**  Name: QEU_QRYTEXT_TO_TUPLE -- create tuples for iiqrytext from the
**	text of a REGISTER TABLE query.
**  
**  Description:
**	Create all the elements of an iiqrytext tuple from the text of
**	a REGISTER TABLE command, plus a unique identifier.
**  
**  Inputs:
**	qeu_cb.
**	    qeu_qso:	    illicitly used to point to text of REG TABLE query.
**	mode:		    indicator that this is a REG TABLE query.
**	qryid:		    unique id (actually a timestamp) to tie iirelation
**				& iiqrytext tuples together.
**               
**  Outputs:
**	data.
**	    qrym_data:	    points to the tuples we created.
**	    qrym_cnt:	    count  of  "    "    "     "   .
**
**  Returns:
**	status:		    E_DB_OK unless lower level returns bad status to us.
**
**  Exceptions:
**	none.
**
**  Side Effects:
**	memory stream is opened, tuples are allocated from it.
**  
**  History:
**	13_apr_90 (fourt)
**	    written, after the model of CREATE VIEW.
**      20-apr-92 (schang)
**          fix GW bug 42825
**          move open and close stream out and pass ulm in
**	08-dec-92 (barbara)
**	    Address of query text is stored in QEU_CB.qeu_qtext
**	    instead of overloading qeu_qso.
*/

static DB_STATUS
qeu_qrytext_to_tuple(
QEU_CB	    *qeu_cb,
i4	    mode,
RDD_QDATA   *data,
DB_QRY_ID   *qryid,
ULM_RCB     *ulm)
{
    /* using qeu_qso illicitly to point to squirreled-away query text */
    PTR		    qrytext  = qeu_cb->qeu_qtext;
    DB_STATUS	    memstatus;
    DB_IIQRYTEXT    *tuple;
    QEF_DATA	    **qdatapp;		/* data blocks containing output rows */
    i4		    length;
    i4		    seq_no;		/* sequence number */
    i4		    txtsize;		/* amount of room a tuple will hold */

    length = STlength(qrytext);
    seq_no = 0;				/* sequence number */
    txtsize = sizeof (tuple->dbq_pad) + sizeof(tuple->dbq_text.db_t_text);

    /* set the pointer to the data block list */
    qdatapp = &data->qrym_data;

    /* move the text to tuple buffers as long as we have more data to move */
    for(;length > 0;)
    {
	QEF_DATA    *qdatap;		/* ptr to QEF data block */
    	i4  err;

        ulm->ulm_psize = sizeof(*qdatap);
        if ((memstatus = qec_malloc(ulm)) != E_DB_OK)
        {
            qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
                &qeu_cb->error, 0);
            return (memstatus);
        }
	qdatap = (QEF_DATA *)ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(*tuple);
        if ((memstatus = qec_malloc(ulm)) != E_DB_OK)
        {
            qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
                &qeu_cb->error, 0);
            return (memstatus);
        }
	tuple = (DB_IIQRYTEXT *)ulm->ulm_pptr;

	/* Link the allocated data block with the input data block. */
	qdatap->dt_data = (PTR) tuple;
	qdatap->dt_next	= NULL;
	qdatap->dt_size = sizeof (DB_IIQRYTEXT);
	*qdatapp = qdatap;
	qdatapp = &qdatap->dt_next;

	tuple->dbq_mode = mode;
	tuple->dbq_seq	= seq_no++;
	/*
	**  Rather than unilaterally setting the "status" (strange word) to
	**  indicate 'language is SQL', we should pick up the
	**  rdfcb-rdf_rb.rdr_status field here, which should be set in psy
	**  to DBQ_SQL iff sess_cb->pss_lang == DB_SQL.  But this is much
	**  too painful for us now, since we don't pass thru psy for this
	**  query and we don't have a good way of getting at the session
	**  cb here.  So, just doing it -- [note this field also has a bit
	**  for the presence of a WITH CHECK OPTION, which REGISTER TABLE
	**  never has].
	*/
	tuple->dbq_status = DBQ_SQL;

	/* insert the unique query id into this tuple */
	STRUCT_ASSIGN_MACRO(*qryid, tuple->dbq_txtid);
	{
	    i4	    len;		/* amount of data to move to current 
					** tuple.
					*/
	    /* determine how much of text goes into this tuple */
	    len = length > txtsize ? txtsize : length;

	    /* move the data */
	    tuple->dbq_text.db_t_count = len;
	    MEcopy((PTR)qrytext, len, (PTR)&tuple->dbq_text.db_t_text[0]);

	    /* adjust remaining length and current text pointer */
	    length  -= len;
	    qrytext = (PTR)&((char *)qrytext)[len];
	}
    }	/* End of for. */

    data->qrym_cnt	= seq_no;
    return(E_DB_OK);
}


/*{
**  Name: QEU_TUPLE_TO_IIQRYTEXT -- write tuples into table iiqrytext.
**
**  Description:
**	append some tuples set up in a qeuq_cb to iiqrytext.
**
**  Inputs:
**	qeu_cb:		    passed on to lower level after further setting
**			    up here.
**	qeuq_cb.
**	    qeuq_qry_tup:   bodies of tuples to paste onto iiqrytext.
**	    qeuq_cq:	    count  of   "     "   "     "      "    .
**	qef_cb:		    passed on to lower level.
**
**  Outputs:
**	none.
**
**  Returns:
**	status:		    E_DB_OK, unless error at lower level.
**
**  Exceptions:
**	none.
**
**  Side Effects:
**	tuples are written into the database.
**
**  History:
**	13-apr-90 (fourt)
**	    written, following model of CREATE PROCEDURE.
*/

static  DB_STATUS
qeu_tuple_to_iiqrytext(
QEU_CB	    *qeu_cb,
QEUQ_CB	    *qeuq_cb,
QEF_CB	    *qef_cb)
{
    DB_STATUS	status;
    DB_STATUS	appstatus  = E_DB_OK;
    DB_STATUS	error;

    /* Open the iiqrytext system catalog */
    qeu_cb->qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID;
    qeu_cb->qeu_tab_id.db_tab_index = 0L;
    qeu_cb->qeu_access_mode = DMT_A_WRITE;
    qeu_cb->qeu_lk_mode = DMT_SIX;   /* shared tbl lock with intent to update */

    status = qeu_open(qef_cb, qeu_cb);
    if (DB_FAILURE_MACRO(status))
    {
        error = qeu_cb->error.err_code;
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
        goto  thru;
    }
	

    /* Append the query text tuples to the iiqrytext table */
    qeu_cb->qeu_count = qeuq_cb->qeuq_cq;
    qeu_cb->qeu_tup_length = sizeof(DB_IIQRYTEXT);
    qeu_cb->qeu_input = qeuq_cb->qeuq_qry_tup;
    appstatus = qeu_append(qef_cb, qeu_cb);
    if (DB_FAILURE_MACRO(appstatus))
    {
        error = qeu_cb->error.err_code;
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    }

    /* to prevent the "(1 row)" msg from appearing, quash the tuple count now */
    qeu_cb->qeu_count = -1;

    status = qeu_close(qef_cb, qeu_cb);
    if (DB_FAILURE_MACRO(status))
    {
        error = qeu_cb->error.err_code;
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    }

  thru:
    /* return the worst status we received */
    if (appstatus > status)

	status = appstatus;
    return (status);
}


/*{
**  QEU_REMOVE_FROM_IIQRYTEXT -- for REMOVE TABLE/INDEX, remove associated
**				    entries in iiqrytext.
**
**  Description:
**	The text of a REGISTER TABLE or REGISTER INDEX command is put into
**	system table iiqrytext.  When the associated item is REMOVEd, we
**	have to remove the text from iqrytext too.  We are only called for
**	Gateway DESTROYs.  We assume we're already within a transaction.
**
**  Inputs:
**	gw		-- identifier of which gateway this db is on.
**	key		-- qryid of the iiqrytext tuples we want to remove.
**	qeuq_cb		-- ptr to a QEUQ_CB, from which we set up our
**			    local QEU_CB.
**	qef_cb		-- ptr to a QEF_CB, which we pass on to
**			    qeu_open/delete/close.
**	error		-- ptr to a cell in our caller where we can stash
**			    errors.
**
**  Outputs:
**	none.
**
**  Returns:
**	E_DB_OK, unless someone below us finds trouble.
**
**  Exceptions:
**	None.
**
**  Side Effects:
**	Tuple(s) removed from iqrytext.
**
**  History:
**	2-may-90 (fourt)
**	    written, after the model of qeu_dview.
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
**	1-Jun-2004 (schka24)
**	    "invalid query plan" error is not very helpful, try "error
**	    destroying a table".  Not much better, I know.
*/

static  DB_STATUS
qeu_remove_from_iiqrytext(
i4	    gw,
DB_QRY_ID   *key,
QEUQ_CB	    *qeuq_cb,
QEF_CB	    *qef_cb,
i4	    *error)
{
    QEU_CB	    qqeu;
    DMR_ATTR_ENTRY  qkey_array[2];
    DMR_ATTR_ENTRY  *qkey_ptr_array[2];
    DB_STATUS	    status, close_status;


    /* Why won't VMS C let us initialize these in the declaration? */
    qkey_ptr_array[0] = &qkey_array[0];
    qkey_ptr_array[1] = &qkey_array[1];

    /* be sure this is a valid gateway id; server coding error if not */
    if (gw < DMGW_NONE  ||  gw > GW_GW_COUNT)
    {
	*error = E_QE0002_INTERNAL_ERROR;
	return (E_DB_ERROR);
    }

    /* set up QEU_CB for accessing iiqrytext */
    qqeu.qeu_type = QEUCB_CB;
    qqeu.qeu_length = sizeof(QEUCB_CB);
    qqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
    qqeu.qeu_lk_mode = DMT_IX;
    qqeu.qeu_flag = DMT_U_DIRECT;
    qqeu.qeu_access_mode = DMT_A_WRITE;
    qqeu.qeu_tab_id.db_tab_base = DM_B_QRYTEXT_TAB_ID;
    qqeu.qeu_mask = 0;

    for (;;)
    {
	qqeu.qeu_tab_id.db_tab_index = 0L;

	/* open iiqrytext */	
	status = qeu_open(qef_cb, &qqeu);
	if (status != E_DB_OK)
	{
	    *error = qqeu.error.err_code;
	    break;
	}

	/* 
	** Now set keys into the iiqrytext table for the 
	** query id for the REGISTER associated with this REMOVE.
	*/	    
	qqeu.qeu_getnext = QEU_REPO;
	qqeu.qeu_klen = 2;       
	qqeu.qeu_key = qkey_ptr_array;
	qkey_ptr_array[0]->attr_number = 1;
	qkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	qkey_ptr_array[0]->attr_value = (char*) &key->db_qry_high_time;
  	qkey_ptr_array[1]->attr_number = 2;
	qkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	qkey_ptr_array[1]->attr_value = (char*) &key->db_qry_low_time;

	qqeu.qeu_qual = 0;
	qqeu.qeu_qarg = 0;
	qqeu.qeu_count = 0;
        qqeu.qeu_tup_length = sizeof(DB_IIQRYTEXT);

	status = qeu_delete(qef_cb, &qqeu);
	if (status != E_DB_OK)
	{
	    *error = qqeu.error.err_code;
	    break;
	}

	/*
	**  Not sure we want to test for this error condition here.  This
	**  was copied from qeu_dview (like the rest of this stuff), where
	**  it was headed by an indecipherable note. We should test that this
	**  error looks good when we've removed the iiqrytext tuples by hand,
	**  before doing a REMOVE.  So check whether any tuples were
	**  found; error if not.
	*/
    	if (qqeu.qeu_count == 0)
	{
	    TRdisplay("%@ Missing iiqrytext for REMOVE, keys (%d,%d)\n",
		key->db_qry_high_time, key->db_qry_low_time);
	    *error = E_QE008A_BAD_TABLE_DESTROY;
	    status = E_DB_ERROR;
	}
	break;
    }

    close_status = qeu_close(qef_cb, &qqeu);    
    if (close_status != E_DB_OK)
    {
	*error = qqeu.error.err_code;
	status = close_status;
    }
    return (status);
}

/*
** Name: qeu_create_table -- Create possibly partitioned table utility
**
** Description:
**	Table creation can occur in two different contexts: a standalone
**	CREATE TABLE statement, or a CREATE TABLE AS SELECT.  (Or the
**	analogous DECLARE GLOBAL TEMPORARY TABLE statements.)
**	There is significant common work for the two flavors of create
**	table, especially when the table is to be partitioned.
**	This routine exists to do the CREATE TABLE work that is common
**	to both situations.
**
**	First, DMF is called with the DMU_CB as passed by the caller.
**	If this is a partitioned table, this will create the partition
**	master (and take an X control lock on it, as well).
**	If there are partitions, we then create each partition, slowly
**	and laboriously, one partition at a time.  (A future optimization
**	might be to set DMF the task of creating all the iirelation
**	records in one worker thread, and creating table files in N other
**	worker threads.  However the speed of a partitioned CREATE TABLE
**	is not typically a limiting factor!)
**
**	As tables are created, their table ID's can optionally be passed
**	back to the caller.  The caller can pass in an array of DB_TAB_ID's.
**	This is most likely to be used by create-as-select to fill in
**	partition validation table ID's.
**
**	If partitioning, different partitions may get different physical
**	characteristics.  (At present, the only supported characteristic
**	is the table location.)  The user's instructions are encoded in
**	a list of QEU_LOGPART_CHAR blocks.  Each block describes the
**	physical structure for one or more logical partitions at some
**	partitioning dimension.  That structure applies to all physical
**	partitions making up those logical partitions -- unless, of course,
**	it is overridden by some inner-dimension instruction.
**
**	After all the table creation stuff is done, if everything went
**	OK, an implicit schema creation check is done.  (For real tables,
**	not for gtt's.)  I.e. if this is the first table created for
**	some user karl, a schema for karl is created.  If the table
**	was partitioned, we'll ask RDF to write out the new partition
**	definition to the partitioning catalogs.
**
**	If something goes wrong, error translation to QEF-flavored
**	(or USF-flavored) messages is done, and the translated QEF
**	style error is stored in the caller's qef_rcb.error block.
**
**	Important:  the caller is expected to supply a writable
**	DMU_CB.  We're not going to change the contents of anything
**	that the DMU_CB points to;  but we will be changing the
**	DMU_CB itself.  If the DMU_CB belongs to a sharable query plan,
**	the caller must make a writable copy.
**
** Inputs:
**	qef_rcb		QEF request block for error return
**	    .qef_stmt_type  Set QEF_DGTT_AS_SELECT if dgtt (as-select or not)
**	dmucb		Caller's copy of DMU control block (gets changed!)
**	lpc_list	List of QEU_LOGPART_CHAR structure blocks
**	tabid_count	Number of entries in table_ids array
**	table_ids	An output
**
** Outputs:
**	Returns DB_STATUS, if error then QEF style error in qef_rcb.error
**	table_ids	Filled in with newly created table ID(s)
**
** History:
**	11-Feb-2004 (schka24)
**	    The railroad tracks are starting to meet.
**	10-Mar-2004 (schka24)
**	    Prefix partition table names with ii so that users can't
**	    generate similar names, not even with delimited ID's.
**	    (Allows us to get away without name-locks in create and drop.)
**	    Change how partition tabids are passed to fix modify screwup.
**	23-Nov-2005 (kschendel, fie on schka24!)
**	    Slice allocation= by number of partitions, to match modify.
**	    (plus it makes sense.)
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better cancel check.
*/

DB_STATUS
qeu_create_table(QEF_RCB *qefrcb, DMU_CB *dmucb,
	QEU_LOGPART_CHAR *lpc_list, i4 tabid_count, DB_TAB_ID *table_ids)
{

    char masterNameAsciz[DB_TAB_MAXNAME+1];
    DB_STATUS status;			/* Called routine status */
    DB_TAB_ID masterId;			/* Master table ID number */
    DM_DATA masterLoc;			/* Master location array */
    i4 errcode;				/* Called facility error code */
    i4 toss_error;

    /* Start by creating the (master) table */

    MEcopy(dmucb->dmu_table_name.db_tab_name, DB_TAB_MAXNAME, masterNameAsciz);
    masterNameAsciz[DB_TAB_MAXNAME] = '\0';
    dmucb->dmu_flags_mask &= ~DMU_PARTITION;   /* Just in case... */
    status = dmf_call(DMU_CREATE_TABLE, dmucb);
    if (DB_FAILURE_MACRO(status) && dmucb->error.err_code == E_DM0101_SET_JOURNAL_ON)
    {
	/* The DM0101 error isn't an error, it produces an informational
	** message.  "journaling will be turned on at the next ckp..."
	*/
	qef_error(5117, 0, status, &toss_error, &qefrcb->error,
	    1, qec_trimwhite(DB_TAB_MAXNAME, dmucb->dmu_table_name.db_tab_name),
	    dmucb->dmu_table_name.db_tab_name);
	qefrcb->error.err_code = E_QE0000_OK;
	status = E_DB_OK;
    }
    if (DB_FAILURE_MACRO(status))
    {
	errcode = dmucb->error.err_code;
	goto error_xlate;		/* Other errors, get out of the way */
    }

    /* If caller wanted the master ID, return it */
    if (tabid_count > 0 && table_ids != NULL)
    {
	STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, table_ids[0]);
    }

    /* If we're doing partitions, create all the partitions.  The
    ** only part that needs any cleverness is dealing with the logpart
    ** characteristics list.  This list is not in any particular order,
    ** and it's too late to mess with it here (it's readonly).
    ** Only one block from each dimension can be relevant for a partition,
    ** though;  so it's quite easy to keep track of which info blocks
    ** can affect the partition we're about to create.  It's just a
    ** little bit of bookkeeping.  Fortunately, given that the only
    ** characteristic supported at present is the location, we can simply
    ** look for the innermost applicable block.  In the future it will
    ** become necessary to merge potentially non-overlapping instructions.
    */
    if (dmucb->dmu_part_def != NULL)
    {
	char tabname[DB_TAB_MAXNAME+30];/* iixxxxx ppnnnnn-master name */
	DB_PART_DEF *part_def;
	DMU_CHAR_ENTRY *chr;		/* DMU characteristics array entry */
	i4 dim;
	i4 i;
	i4 ndims;
	QEU_LOGPART_CHAR *cur_lpc[DBDS_MAX_LEVELS];
	QEU_LOGPART_CHAR *logpart_ptr;
	QEU_LOGPART_CHAR *starting_lpc[DBDS_MAX_LEVELS];
	RDF_CB rdfcb;			/* RDF request block */
	u_i2 partseq[DBDS_MAX_LEVELS];	/* Current partition sequence */
	u_i2 partno;			/* Current partition number */

	/* Save some master related stuff */
	STRUCT_ASSIGN_MACRO(dmucb->dmu_location, masterLoc);
	STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, masterId);

	part_def = dmucb->dmu_part_def;
	ndims = part_def->ndims;
	for (dim=0; dim<ndims; ++dim)
	{
	    partseq[dim] = 1;
	    starting_lpc[dim] = NULL;
	    cur_lpc[dim] = NULL;
	}

	/* If the user said ALLOCATION=, divide the allocation by the
	** number of partitions (same as what modify does in DMF).  The
	** idea is that the allocation applies to the entire table.
	** (I would do this in DMF except DMF doesn't know the total number
	** of partitions conveniently.)
	*/

	i = dmucb->dmu_char_array.data_in_size / sizeof(DMU_CHAR_ENTRY);
	chr = (DMU_CHAR_ENTRY *) dmucb->dmu_char_array.data_address;
	if (chr != NULL)
	{
	    while (--i >= 0)
	    {
		if (chr->char_id == DMU_ALLOCATION)
		{
		    chr->char_value = chr->char_value / part_def->nphys_parts;
		    if (chr->char_value < 4)
			chr->char_value = 0;	/* Maintain reasonable min */
		    break;
		}
		++chr;
	    }
	}

	/* Get things started by finding the logpart block with the
	** smallest partition sequence, for each dimension.
	*/
	if (lpc_list != NULL)
	{
	    for (dim = 0; dim < ndims; ++dim)
	    {
		logpart_ptr = lpc_list;
		do
		{
		    if (dim == logpart_ptr->dim)
			if (starting_lpc[dim] == NULL
			  || logpart_ptr->partseq < starting_lpc[dim]->partseq)
			    starting_lpc[dim] = logpart_ptr;
		    logpart_ptr = logpart_ptr->next;
		} while (logpart_ptr != NULL);
		cur_lpc[dim] = starting_lpc[dim];
	    }
	}
	/* Create partitions... */
	for (partno = 0; partno < part_def->nphys_parts; ++partno)
	{
	    /* Assume location will be master's, check for override.
	    ** Innermost char block wins.
	    */
	    STRUCT_ASSIGN_MACRO(masterLoc, dmucb->dmu_location);
	    for (dim = ndims-1; dim >= 0; --dim)
	    {
		if (cur_lpc[dim] != NULL
		  && partseq[dim] >= cur_lpc[dim]->partseq
		  && partseq[dim] < cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
		{
		    /* Info block cur_lpc applies. */
		    STRUCT_ASSIGN_MACRO(cur_lpc[dim]->loc_array,
				dmucb->dmu_location);
		    break;
		}
	    }
	    /* Create the partition */
	    dmucb->dmu_flags_mask |= DMU_PARTITION;
	    dmucb->dmu_tbl_id.db_tab_base = masterId.db_tab_base;
	    /* (below not a typo.  create already allocated N id numbers
	    ** off of db_tab_base and it computes the partition ID off from
	    ** what we pass in db_tab_index.)
	    */
	    dmucb->dmu_tbl_id.db_tab_index = masterId.db_tab_base;
	    dmucb->dmu_partno = partno;
	    /* Tables have to have names, but you don't use the name
	    ** if it's a partition;  generate an illegal name.
	    ** The table reltid is included for uniqueness, the table name
	    ** and partition # is just for tech support humans.
	    */
	    STprintf(tabname,"ii%x pp%d-%s", partno + masterId.db_tab_base,
				partno, masterNameAsciz);
	    /* tabname is already space-filled thanks to mastername */
	    MEcopy(tabname, DB_TAB_MAXNAME, dmucb->dmu_table_name.db_tab_name);
	    status = dmf_call(DMU_CREATE_TABLE, dmucb);
	    if (DB_FAILURE_MACRO(status))
	    {
		/* Ignore those DM0101's */
		errcode = dmucb->error.err_code;
		if (errcode == E_DM0101_SET_JOURNAL_ON)
		    status = E_DB_OK;
		else
		    goto error_xlate;	/* Oops... */
	    }
	    /* Return partition ID to caller if he wants it.  Remember
	    ** that table-id index 0 was the master.
	    */
	    if (tabid_count > partno+1)
	    {
		STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, table_ids[partno+1]);
	    }
	    /* Ok, bump up the innermost partition sequence.  If the current
	    ** characteristics block is "used up", look for the next one,
	    ** or reset to the start.  If we reach the end of a dimension
	    ** start it over and increment the next outer dimension.
	    */
	    dim = ndims - 1;
	    do
	    {
		++ partseq[dim];
		if (partseq[dim] > part_def->dimension[dim].nparts)
		{
		    partseq[dim] = 1;
		    cur_lpc[dim] = starting_lpc[dim];
		    --dim;
		    /* End of a dimension is as good a place as any to
		    ** ensure that we don't have query lockout
		    */
		    CScancelCheck(qefrcb->qef_cb->qef_ses_id);
		}
		else
		{
		    /* Keep going in this dimension. */
		    if (cur_lpc[dim] != NULL
		      && partseq[dim] >= cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
		    {
			/* Try to find the next logpart block that will
			** apply in this dimension.  If there aren't any
			** more, make cur-lpc null.
			*/
			cur_lpc[dim] = NULL;
			logpart_ptr = lpc_list;  /* Can't be NULL if here */
			do
			{
			    if (dim == logpart_ptr->dim
			      && logpart_ptr->partseq >= partseq[dim]
			      && (cur_lpc[dim] == NULL || logpart_ptr->partseq < cur_lpc[dim]->partseq))
				cur_lpc[dim] = logpart_ptr;
			    logpart_ptr = logpart_ptr->next;
			} while (logpart_ptr != NULL);
		    }
		    break;
		}
	    } while (dim >= 0);
	} /* for each partition */

	/* It looks promising so far.  Go ahead and write out the partition
	** catalog stuff.
	*/

	qeu_rdfcb_init((PTR)&rdfcb, qefrcb->qef_cb);
	STRUCT_ASSIGN_MACRO(masterId, rdfcb.rdf_rb.rdr_tabid);
	rdfcb.rdf_rb.rdr_newpart = part_def;
	status = rdf_call(RDF_PART_UPDATE, &rdfcb);
	if (DB_FAILURE_MACRO(status))
	{
	    errcode = rdfcb.rdf_error.err_code;
	    goto error_xlate;
	}

    }

    /* Almost done.  If there's implicit schema stuff to do, do it. */
    if ((qefrcb->qef_stmt_type & QEF_DGTT_AS_SELECT) == 0)
    {
	status = qea_schema(qefrcb, NULL, qefrcb->qef_cb,
		    (DB_SCHEMA_NAME *) &dmucb->dmu_owner, IMPSCH_FOR_TABLE);
	errcode = qefrcb->error.err_code;
    }
    if (DB_SUCCESS_MACRO(status))
	return (E_DB_OK);

error_xlate:

    /* Something went wrong,  facility error in errcode.  Translate
    ** the message to something QEF-ish if needed.
    */

    if (qefrcb->qef_eflag != QEF_EXTERNAL)
    {
	qefrcb->error.err_code = errcode;
	return (E_DB_ERROR);
    }

    errcode = createTblErrXlate(dmucb, masterNameAsciz, errcode, status);

    qefrcb->error.err_code = errcode;
    return (E_DB_ERROR);

} /* qeu_create_table */

/*
** Name: qeu_modify_prep -- Prepare for partitioned MODIFY
**
** Description:
**	The MODIFY operation (including RELOCATE) in the presence of
**	partitions needs a bit more preparation than we can plausibly
**	do at the parse stage. This routine does the needed
**	preparation.  The preparation falls into three general classes:
**	- partition def fixup
**	- resolving location information
**	- pre-creating additional partition tables
**
**	(In the future, if and when more storage characteristics
**	become separable by physical partition, there will be even
**	more work to do.)
**
**	There are two kinds of MODIFY to deal with, repartitioning
**	modify and regular modify.  Regardless of which one we're
**	doing, the goal is to build a list of modify targets and their
**	(new) locations.  New location information should be omitted
**	if the target already resides in the proper place.  The main
**	difference between handling a repartitioning modify and a
**	regular one, is that a repartitioning modify may have many
**	different new-location specifications depending on logical
**	partition.  (This info comes from a qeu_logpart_list of
**	QEU_LOGPART_CHAR blocks attached by the parser to the request
**	QEU_CB.)  A regular modify has at most one new-location spec,
**	and it's in the dmucb dmu_location.
**
**	This routine doesn't need to do anything unless the modify is
**	a storage-structure modify, or a reorganize, or a relocate. 
**	(All repartitioning modifies are storage- structure modifies
**	too.)  And, if the modify is a storage-structure modify, and
**	there's no (new) target-location given, we don't need to do
**	anything except on a repartitioning modify.
**
**	For repartitioning modifies, as well as resolving location
**	stuff, we need to check whether the new scheme has more
**	partitions than the old one did.  If so, we need to create
**	partition tables to hold the new partitions (in the proper
**	locations!)
**
**	Also for repartitioning modifies, we'll create a copy of the
**	partition definition in our QEF temp memory.  This copy will be
**	in contiguous memory, and will not contain any names, texts, etc.
**	Why?  because DMF needs to log the partition definition, and
**	DMF is too grand to be seen chasing bits and pieces of a partdef
**	around.  So it wants the partdef all in one piece.  It doesn't
**	need the names and text, so we'll eliminate that.  (The partdef
**	passed in DOES have all the name stuff, for catalog updating.)
**
** Inputs:
**	qeucb		QEU request control block built by parse (READONLY)
**	dmucb		qeu copy (writable) of DMU request block built by parse
**	ulm		Requestor for an opened temp qef memory stream
**	pp_array	An output
**	err_code	An output
**
** Outputs:
**	Returns the usual E_DB_xxx error code
**	pp_array	The (old) master physical-partition array descriptor
**			is returned here, we might need it for repartitioning
**	err_code	Set to DMU or ULM error code if error occurs
**	Creates and/or updates the dmu_ppchar_array as needed
**
** Side Effects:
**	May change the dmu_part_def to point to a trimmed copy, caller
**	should preserve the original (complete) one.
**
** History:
**	26-Feb-2004 (schka24)
**	    Munch on modify requests before spitting them out to DMF.
**	9-Mar-2004 (schka24)
**	    Flailing about with location formats.  Always build a ppchar
**	    array if we bother with all the dmt-show's, because it saves
**	    modify doing a similar pass.
**	1-Apr-2004 (schka24)
**	    A fitting day to start implementing partition-def logging,
**	    so repartitioning modifies can be journaled.
**	5-Apr-2004 (schka24)
**	    More of the above: partitions need to be created with the same
**	    journaling status as the master.  We hadn't noticed that before
**	    because you could never get that far before...
**	15-Feb-2005 (thaju02)
**	    For online modify, don't create new partitions here (locks
**	    table). Do it in do_online_modify().
*/

/* Definitions strictly local for this routine */

#define NUM_CRE_DMU_CHAR 10	/* See setupDmuChar, has to be enough! */


DB_STATUS
qeu_modify_prep(QEU_CB *qeucb, DMU_CB *dmucb, ULM_RCB *ulm,
	DM_DATA *return_pp_array, i4 *err_code)
{
    char masterNameAsciz[DB_TAB_MAXNAME+1];
    char tabname[DB_TAB_MAXNAME + 30];	/* Generated partition table name */
    DB_LOC_NAME *locname_ptr;		/* Pointer to a location name */
    DB_LOC_NAME **loc_pp;		/* Pointer to loc pointer array */
    DB_PART_DEF *part_def;		/* (new) partition definition */
    DB_STATUS status;			/* Called routine status */
    DM_DATA master_loc;			/* Where master is currently */
    DM_DATA *new_loc;			/* Where partition should end up */
    DMT_PHYS_PART *dmt_pp_array;	/* Base of old partition info array */
    DMT_SHW_CB *dmtshow;		/* Table info request cb */
    DMT_TBL_ENTRY *tbl;			/* Table info from show */
    DMU_CB cre_dmucb;			/* DMU cb for creating tables */
    DMU_CHAR_ENTRY cre_char_array[NUM_CRE_DMU_CHAR];  /* DMU characteristics
					** for newly created partitions */
    DMU_PHYPART_CHAR *ppc_base;		/* Base of physical partition directives */
    DMU_PHYPART_CHAR *phypart_ptr;	/* Ptr to one phys-partition directive */
    i4 dim;				/* A dimension index */
    i4 natts;				/* Count of attributes */
    i4 newdims;				/* Number of (new) dimensions */
    i4 new_nparts;			/* Number of new physical partitions */
    i4 nparts;				/* A partition counter */
    i4 old_nparts;			/* Number of old physical partitions */
    i4 opcode;				/* What we're doing */
    i4 partno;				/* Physical partition number */
    QEU_LOGPART_CHAR *cur_lpc[DBDS_MAX_LEVELS];
    QEU_LOGPART_CHAR *logpart_ptr;	/* Ptr to one log-partition directive */
    QEU_LOGPART_CHAR *lpc_list;		/* New-logical-partition directives */
    QEU_LOGPART_CHAR *starting_lpc[DBDS_MAX_LEVELS];
    u_i2 partseq[DBDS_MAX_LEVELS];	/* Current partition sequence */

    /* See if we need to be here */
    opcode = qeucb->qeu_d_op;
    if (opcode == DMU_MODIFY_TABLE)
	opcode = ((DMU_CHAR_ENTRY *) dmucb->dmu_char_array.data_address)->char_id;

    /* We only care about relocating and restructuring */
    if (opcode != DMU_RELOCATE_TABLE
      && opcode != DMU_STRUCTURE
      && opcode != DMU_REORG)
	return (E_DB_OK);

    /* If restructuring, and not relocating and not repartitioning, leave */
    if (opcode == DMU_STRUCTURE
      && dmucb->dmu_location.data_in_size == 0
      && dmucb->dmu_part_def == NULL)
	return (E_DB_OK);

    /* if the parser managed to resolve the request down to a specific
    ** physical partition, it all gets handled pretty much like a regular
    ** table.  (Can't be a repartitioning modify if so.)
    */
    if (dmucb->dmu_tbl_id.db_tab_index < 0)
	return (E_DB_OK);

    /* Looks like there's work to do.  We'll need to do a bunch of
    ** dmt-show's on the master and the partitions, so allocate space
    ** for the request block and the answers.  For the location answer,
    ** just make it the max size possible.
    */

    part_def = dmucb->dmu_part_def;

    ulm->ulm_psize = sizeof(DMT_SHW_CB);
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }
    dmtshow = (DMT_SHW_CB *) ulm->ulm_pptr;
    MEfill(sizeof(DMT_SHW_CB), 0, dmtshow);
    dmtshow->type = DMT_SH_CB;
    dmtshow->length = sizeof(DMT_SHW_CB);

    ulm->ulm_psize = sizeof(DMT_TBL_ENTRY);
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }
    dmtshow->dmt_table.data_address = (PTR) ulm->ulm_pptr;
    dmtshow->dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
    tbl = (DMT_TBL_ENTRY *) ulm->ulm_pptr;

    if (dmucb->dmu_nphys_parts > 0)
    {
	ulm->ulm_psize = dmucb->dmu_nphys_parts * sizeof(DMT_PHYS_PART);
	dmtshow->dmt_phypart_array.data_in_size = ulm->ulm_psize;
	status = qec_malloc(ulm);
	if (DB_FAILURE_MACRO(status))
	{
	    *err_code = ulm->ulm_error.err_code;
	    return (status);
	}
	dmtshow->dmt_phypart_array.data_address = (PTR) ulm->ulm_pptr;
	dmt_pp_array = (DMT_PHYS_PART *) ulm->ulm_pptr;
    }
    else
    {
	/* Must be a nonpartition->partition repartitioning modify, else
	** we wouldn't get here.
	*/
	dmt_pp_array = NULL;
    }

    ulm->ulm_psize = DM_LOC_MAX * (sizeof(DB_LOC_NAME *) + sizeof(DB_LOC_NAME));
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }
    dmtshow->dmt_loc_array.ptr_address = (PTR) ulm->ulm_pptr;
    dmtshow->dmt_loc_array.ptr_size = sizeof(DB_LOC_NAME);
    dmtshow->dmt_loc_array.ptr_in_count = DM_LOC_MAX;
    loc_pp = (DB_LOC_NAME **) (dmtshow->dmt_loc_array.ptr_address) + (DM_LOC_MAX-1);
    locname_ptr = (DB_LOC_NAME *) (loc_pp + 1);
    /* Yeah, this rigs up the pointers backwards, but who cares */
    while ((PTR) loc_pp >= dmtshow->dmt_loc_array.ptr_address)
	*loc_pp-- = locname_ptr++;

    dmtshow->dmt_db_id = dmucb->dmu_db_id;
    dmtshow->dmt_session_id = qeucb->qeu_d_id;
    dmtshow->dmt_char_array.data_in_size = 0;
    dmtshow->dmt_char_array.data_address = NULL;

    /* Ok, now we're ready to get some work done.  Ask for the master info.
    ** We need to know the physical partition table ID's.
    ** We also need to know where the master is defined to be currently.
    */

    dmtshow->dmt_flags_mask = DMT_M_TABLE | DMT_M_LOC | DMT_M_PHYPART;
    STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, dmtshow->dmt_tab_id);
    status = dmf_call(DMT_SHOW, dmtshow);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = dmtshow->error.err_code;
	return (status);
    }
    STRUCT_ASSIGN_MACRO(dmtshow->dmt_phypart_array, *return_pp_array);

    /* If this is an un-partitioning modify, that's all we needed -- just
    ** the partition tabid data for modify-finish.
    */
    if (part_def != NULL && part_def->ndims == 0)
	return (E_DB_OK);

    natts = tbl->tbl_attr_count;	/* For later */
    MEcopy(tbl->tbl_name.db_tab_name, DB_TAB_MAXNAME, masterNameAsciz);
    masterNameAsciz[DB_TAB_MAXNAME] = '\0';

    /* If we're repartitioning (but not unpartitioning), make a clean
    ** copy of the definition, with no extra crap in it, for DMF.
    ** Substitute this copy for the original.  (Presumably someone
    ** higher up still has a pointer to the original!  It will be needed
    ** later to write out the partitioning catalogs.)
    */
    if (part_def != NULL)
    {
	status = copyPartDef(qeucb, dmucb, ulm, err_code);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	/* dmucb's dmu_part_def gets updated to point to the copy.
	** It's ok to keep using the original part def in "part_def";
	** this routine doesn't care which is used.
	*/
    }

    /* Reformat the current master location into a straight DB_LOC_NAME
    ** array pointed to by a DM_DATA.  This is how sameLoc expects to
    ** see the "should be" location.
    */
    status = locPtrToArray(&dmtshow->dmt_loc_array, &master_loc, ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }

    old_nparts = tbl->tbl_nparts;
    new_nparts = old_nparts;		/* Fixed later if repartitioning */
    newdims = tbl->tbl_ndims;
    ppc_base = (DMU_PHYPART_CHAR *) dmucb->dmu_ppchar_array.data_address;
    if (dmucb->dmu_ppchar_array.data_in_size == 0)
    {
	ppc_base = NULL;		/* Just in case I was careless */
	dmucb->dmu_ppchar_array.data_address = NULL;
    }
    /* Clean out "show" for the pparray, won't show it again */
    dmtshow->dmt_phypart_array.data_address = NULL;
    dmtshow->dmt_phypart_array.data_in_size = 0;

    /* Before we create a ppchar array, see if the modify was operating on
    ** the entire table as opposed to some logical partitions.  PSF only
    ** creates a ppchar array when it syntactically has to, so the absence
    ** of a ppchar array and a master table-id mean that the operation is
    ** a whole-table operation.
    ** (Once we create a ppchar array here, DMF has no way to tell if the
    ** operation was on the master or not.)
    */
    if (dmucb->dmu_ppchar_array.data_address == NULL
      && dmucb->dmu_tbl_id.db_tab_index >= 0)
	dmucb->dmu_flags_mask |= DMU_MASTER_OP;

    /* Before we get going with the partition examiner loop, if this is a
    ** repartitioning modify, we might have logical partition info blocks
    ** with directions on how to structure individual logical partitions.
    ** Set up the machinery for keeping track of which logical partition we're
    ** on, and which logpart info block applies.
    ** Fortunately, at present the only info we apply is the location, so
    ** the innermost applicable logpart info block applies.  In the future
    ** it may become necessary to merge potentially non-overlapping info.
    */
    lpc_list = qeucb->qeu_logpart_list;
    if (part_def != NULL)
    {
	newdims = part_def->ndims;	/* Won't be zero */
	new_nparts = part_def->nphys_parts;
	for (dim = 0; dim < newdims; ++dim)
	{
	    partseq[dim] = 1;
	    starting_lpc[dim] = NULL;
	    cur_lpc[dim] = NULL;
	}
	/* Get things started by finding the logpart block with the
	** smallest partition sequence, for each dimension.
	*/
	if (lpc_list != NULL)
	{
	    for (dim = 0; dim < newdims; ++dim)
	    {
		logpart_ptr = lpc_list;
		do
		{
		    if (dim == logpart_ptr->dim)
			if (starting_lpc[dim] == NULL
			  || logpart_ptr->partseq < starting_lpc[dim]->partseq)
			    starting_lpc[dim] = logpart_ptr;
		    logpart_ptr = logpart_ptr->next;
		} while (logpart_ptr != NULL);
		cur_lpc[dim] = starting_lpc[dim];
	    }
	}
    }

    /* Since we're going to do all these shows, make a ppchar array
    ** so we can tell DMF the old-location counts (at the very least.)
    */
    if (ppc_base == NULL)
    {
	status = newPPcharArray(dmucb, ulm, new_nparts, return_pp_array);
	if (DB_FAILURE_MACRO(status))
	{
	    *err_code = ulm->ulm_error.err_code;
	    return (status);
	}
	ppc_base = (DMU_PHYPART_CHAR *) dmucb->dmu_ppchar_array.data_address;
    }

    /* Examine each existing physical partition.  Decide which location
    ** it's to end up at:
    **  - as directed by the logical partition info blocks (if repartitioning,
    **    and if a logical partition info block applies);
    **  - as directed by the dmucb's dmu_location (i.e. the MODIFY with-
    **    location if there is one);
    **  - the current master location as a fallback if repartitioning;
    **  - if nothing else applies, leave it where it is.
    ** If the physical partition is already in the right place, leave
    ** it's ppchar entry location null;  and if the operation is just a
    ** relocate/reorg, the pp can be ignored entirely.
    */
    nparts = old_nparts;
    if (new_nparts < old_nparts) nparts = new_nparts;

    for (partno = 0; partno < nparts; ++partno)
    {
	if (ppc_base == NULL ||
	  (ppc_base != NULL && (ppc_base[partno].flags & DMU_PPCF_IGNORE_ME) == 0))
	{
	    /* Partition is not being ignored */
	    /* Do a SHOW on the partition to see where it's residing now. */
	    dmtshow->dmt_flags_mask = DMT_M_TABLE | DMT_M_LOC;
	    STRUCT_ASSIGN_MACRO(dmt_pp_array[partno].pp_tabid, dmtshow->dmt_tab_id);
	    status = dmf_call(DMT_SHOW, dmtshow);
	    if (DB_FAILURE_MACRO(status))
	    {
		*err_code = dmtshow->error.err_code;
		return (status);
	    }

	    /* Tell DMF about the source locations */
	    ppc_base[partno].oloc_count = dmtshow->dmt_loc_array.ptr_out_count;

	    /* Decide where this partition is to end up */
	    new_loc = &dmucb->dmu_location;
	    if (part_def != NULL)
	    {
		/* Repartitioning, if no WITH clause use master loc */
		if (new_loc->data_in_size == 0)
		    new_loc = &master_loc;
		/* Logical partition directive overrides all */
		for (dim = newdims - 1; dim >= 0; --dim)
		{
		    if (cur_lpc[dim] != NULL
		      && partseq[dim] >= cur_lpc[dim]->partseq
		      && partseq[dim] < cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
		    {
			/* Info block cur_lpc applies. */
			new_loc = &cur_lpc[dim]->loc_array;
			break;
		    }
		}
	    }
	    if (sameLoc(new_loc, &dmtshow->dmt_loc_array))
	    {
		/* Partition is already at the right place.
		** If this is relocate or reorganize, this partition can
		** be skipped.
		*/
		if (opcode == DMU_RELOCATE_TABLE || opcode == DMU_REORG)
		{
		    ppc_base[partno].flags |= DMU_PPCF_IGNORE_ME;
		}
		else
		{
		    /* Partition already where it belongs, make sure that
		    ** the location description for this partition is empty.
		    */
		    ppc_base[partno].loc_array.data_address = NULL;
		    ppc_base[partno].loc_array.data_in_size = 0;
		}
	    }
	    else
	    {
		/* Partition isn't at the right place.  We need to indicate
		** where it should go, in the ppchar array.
		*/
		STRUCT_ASSIGN_MACRO((*new_loc), ppc_base[partno].loc_array);
	    }
	} /* if not ignored */

	/* Move to next physical partition.  If we are repartitioning,
	** we need to track which logical partition info block applies
	** after incrementing the logical partition number:  if the current
	** logical partition info block is "used up", look for the next one.
	** If we reach the end of a dimension, start it over and increment
	** the next outer dimension.  (The same logic as create uses.)
	*/
	if (part_def != NULL)
	{
	    dim = newdims - 1;
	    do
	    {
		++partseq[dim];
		if (partseq[dim] > part_def->dimension[dim].nparts)
		{
		    partseq[dim] = 1;
		    cur_lpc[dim] = starting_lpc[dim];
		    --dim;
		}
		else
		{
		    /* Keep going in this dimension. */
		    if (cur_lpc[dim] != NULL
		      && partseq[dim] >= cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
		    {
			/* Try to find the next logpart block that will
			** apply in this dimension.  If there aren't any
			** more, make cur-lpc null.
			*/
			cur_lpc[dim] = NULL;
			logpart_ptr = lpc_list;  /* Can't be NULL if here */
			do
			{
			    if (dim == logpart_ptr->dim
			      && logpart_ptr->partseq >= partseq[dim]
			      && (cur_lpc[dim] == NULL || logpart_ptr->partseq < cur_lpc[dim]->partseq))
				cur_lpc[dim] = logpart_ptr;
			    logpart_ptr = logpart_ptr->next;
			} while (logpart_ptr != NULL);
		    }
		    break;
		}
	    } while (dim >= 0);
	} /* if repartitioning */
    } /* physical partition loop */

    /* If this is a repartitioning modify, and if we are adding partitions,
    ** continue looping to create the additional partition tables.
    ** We'll need all of the attribute stuff, so we'll "show" it from the
    ** master.  (Unfortunately it can't easily be combined with the first
    ** master "show" because we didn't know the attribute count at that
    ** point.)
    ** If it's not a repartitioning modify, we're done, get out.
    */
    if (part_def == NULL || new_nparts <= old_nparts)
	return (E_DB_OK);

    /* We can't use the same DMU cb that we're using for modify, copy
    ** to a work DMU-cb.
    */
    STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, dmtshow->dmt_tab_id);
    STRUCT_ASSIGN_MACRO(*dmucb, cre_dmucb);
    status = setupAttrInfo(&cre_dmucb, dmtshow, natts, ulm, err_code);
    if (DB_FAILURE_MACRO(status))
	return (status);
    cre_dmucb.dmu_char_array.data_address = (PTR) &cre_char_array[0];
    cre_dmucb.dmu_char_array.data_in_size = 0;
    setupDmuChar(&cre_dmucb, dmtshow);
    if (cre_dmucb.dmu_char_array.data_in_size / sizeof(DMU_CHAR_ENTRY) > NUM_CRE_DMU_CHAR)
    {
	/* Whoops, internal error, local array too small.  Stack is
	** probably hosed out as well...
	*/
	TRdisplay("%@Modify prep: cre_char_array too small, want %d\n",
			cre_dmucb.dmu_char_array.data_in_size / sizeof(DMU_CHAR_ENTRY));
	*err_code = E_QE0002_INTERNAL_ERROR;
	return (E_DB_FATAL);
    }
    cre_dmucb.dmu_ppchar_array.data_address = NULL;
    cre_dmucb.dmu_ppchar_array.data_in_size = 0;

    /* Continue the "partno" loop from the above for loop */
    while (partno < new_nparts)
    {
	/* All the same logical partition info stuff as the above loop.
	** It would be nice to not duplicate it but no obvious way occurs
	** to me...
	*/
	/* Decide where this partition is to end up */
	/* Default is the modify with-location... */
	new_loc = &dmucb->dmu_location;
	/* If there isn't any such thing, default is where the master is.
	** (has to be something there, which is a good thing since creates
	** require at least one location.)
	*/
	if (new_loc->data_in_size == 0)
	    new_loc = &master_loc;
	/* If a logical partition directive applies it overrides all */
	for (dim = newdims - 1; dim >= 0; --dim)
	{
	    if (cur_lpc[dim] != NULL
	      && partseq[dim] >= cur_lpc[dim]->partseq
	      && partseq[dim] < cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
	    {
		/* Info block cur_lpc applies. */
		new_loc = &cur_lpc[dim]->loc_array;
		break;
	    }
	}
	/* if online, don't create new parts here */
	if (!(dmucb->dmu_flags_mask & DMU_ONLINE_START))
	{
	    STRUCT_ASSIGN_MACRO((*new_loc), cre_dmucb.dmu_location);

	    /* Create of a partition needs the master table ID in dmu_tbl_id
	    ** db_tab_base.  For the partition db_tab_index we need:
	    ** <new tabid> - partition # - 1.  Create will add back
	    ** partition # + 1.  (The reason for this funkiness is that
	    ** for normal tables, create grabs N id's all at once and sticks
	    ** the base ID in both db_tab_base and _index, and the calculations
	    ** assign the partition ID's sequentially.  That trick doesn't
	    ** work here, of course.)
	    ** We need to allocate a table ID and fudge up the create table ID
	    ** so that it Does The Right Thing (TM).
	    ** FIXME would be good to pass a number-of-ids to DMT_GET_TABID,
	    ** perhaps in the dmu_partno field or somewhere, and only do one
	    ** tabid call a la create.
	    */
	    cre_dmucb.dmu_flags_mask = 0;
	    status = dmf_call(DMU_GET_TABID, &cre_dmucb);
	    if (DB_FAILURE_MACRO(status))
	    {
		*err_code = cre_dmucb.error.err_code;
		return (status);
	    }
	    STprintf(tabname,"ii%x pp%d-%s", cre_dmucb.dmu_tbl_id.db_tab_base,
				    partno, masterNameAsciz);
	    /* Note: result will be space-filled far enough because the
	    ** master name is space filled already.
	    */
	    MEcopy(tabname, sizeof(DB_TAB_NAME), &cre_dmucb.dmu_table_name);
	    /* Move the table ID we got to the index position, create will add
	    ** the I-am-a-partition flag.
	    */
	    cre_dmucb.dmu_tbl_id.db_tab_index = cre_dmucb.dmu_tbl_id.db_tab_base;
	    cre_dmucb.dmu_tbl_id.db_tab_base = dmucb->dmu_tbl_id.db_tab_base;
	    cre_dmucb.dmu_tbl_id.db_tab_index -= (partno + 1);
	    cre_dmucb.dmu_partno = partno;
	    cre_dmucb.dmu_flags_mask = DMU_PARTITION;
	    /* Now actually create the partition table */
	    status = dmf_call(DMU_CREATE_TABLE, &cre_dmucb);
	    if (DB_FAILURE_MACRO(status) && cre_dmucb.error.err_code != E_DM0101_SET_JOURNAL_ON)
	    {
		*err_code = createTblErrXlate(&cre_dmucb, masterNameAsciz, status,
			    cre_dmucb.error.err_code);
		return (status);
	    }
	    /* Remember the created table ID in the ppchar array for MODIFY */
	    STRUCT_ASSIGN_MACRO(cre_dmucb.dmu_tbl_id, ppc_base[partno].part_tabid);
	    ppc_base[partno].flags |= DMU_PPCF_NEW_TABID;

	    /* Note that we created the new partition in the right place,
	    ** but DMU has no way to know where that proper place is!  (The
	    ** new partition can't be opened or fixed until the modify is
	    ** all done.)  Therefore QEU must always pass the location
	    ** stuff for the new partitions in the ppchar array.
	    */
	}  
	STRUCT_ASSIGN_MACRO((*new_loc), ppc_base[partno].loc_array);
	ppc_base[partno].oloc_count = new_loc->data_in_size / sizeof(DB_LOC_NAME);

	/* Continue on to the next partition sequence, doing all the usual
	** logical partition info block bookkeeping.
	*/
	dim = newdims - 1;
	do
	{
	    ++partseq[dim];
	    if (partseq[dim] > part_def->dimension[dim].nparts)
	    {
		partseq[dim] = 1;
		cur_lpc[dim] = starting_lpc[dim];
		--dim;
	    }
	    else
	    {
		/* Keep going in this dimension. */
		if (cur_lpc[dim] != NULL
		  && partseq[dim] >= cur_lpc[dim]->partseq + cur_lpc[dim]->nparts)
		{
		    /* Try to find the next logpart block that will
		    ** apply in this dimension.  If there aren't any
		    ** more, make cur-lpc null.
		    */
		    cur_lpc[dim] = NULL;
		    logpart_ptr = lpc_list;  /* Can't be NULL if here */
		    do
		    {
			if (dim == logpart_ptr->dim
			  && logpart_ptr->partseq >= partseq[dim]
			  && (cur_lpc[dim] == NULL || logpart_ptr->partseq < cur_lpc[dim]->partseq))
			    cur_lpc[dim] = logpart_ptr;
			logpart_ptr = logpart_ptr->next;
		    } while (logpart_ptr != NULL);
		}
		break;
	    }
	} while (dim >= 0);

	++ partno;
    } /* while creating new physical partitions */

    /* Sheesh.  We seem to be ready to do the modify. */
    return (E_DB_OK);
} /* qeu_modify_prep */


/* Helper routines for qeu_modify_prep. */
/* FIXME FIXME put comments into standard form, minimalist for now */

/*
** copyPartDef copies a partition definition, asking for a minimalist
** copy (no names, no text strings).  The partition copied utility,
** which just happens to live in RDF, always creates a contiguously
** allocated definition.  This is a Good Thing(TM), as DMF is going
** to insist on a contiguous definition.
**
** It would be possible to check the one-piece flag here and not bother
** copying.  But, DMF is going to log the partition definition, and
** the smaller it is, the better.  So we'll always copy.  (The original
** partdef necessarily had all the name and text poop in it, because
** it's going to be needed later for the partitioning catalogs.)
**
** Call: DB_STATUS status = copyPartDef(qeucb, dmucb, ulm, &err_code);
**	QEU_CB *qeucb;  -- QEU control block
**	DMU_CB *dmucb;  -- DMU request control block pointing to partdef
**			  Gets updated with new copy of partdef
**	ULM_RCB *ulm;  -- A requestor for a temporary memory stream
**	i4 err_code;  -- Ref: RDF or ULM error code stored here
**	Returns E_DB_xxx ok or error status
**
** History:
**	1-Apr-2004 (schka24)
**	    Invent so that DMF can log the partition definition.
*/

static DB_STATUS
copyPartDef(QEU_CB *qeucb, DMU_CB *dmucb, ULM_RCB *ulm, i4 *err_code)
{

    DB_STATUS status;			/* The usual call status */
    RDF_CB rdfcb;			/* Rdf call control block */
    RDR_INFO rdf_info;			/* Dummy RDF info block */

    MEfill(sizeof(rdfcb), 0, &rdfcb);	/* Don't confuse RDF with junk */
    MEfill(sizeof(rdf_info), 0, &rdf_info);
    rdfcb.rdf_info_blk = &rdf_info;
    rdfcb.rdf_rb.rdr_fcb = NULL;
    rdfcb.rdf_rb.rdr_db_id = qeucb->qeu_db_id;
    rdfcb.rdf_rb.rdr_session_id = qeucb->qeu_d_id;
    rdfcb.rdf_rb.rdr_partcopy_mem = partdefCopyMem;
    rdfcb.rdf_rb.rdr_partcopy_cb = ulm;
    rdfcb.rdf_rb.rdr_instr = 0;		/* No names, no text */
    rdf_info.rdr_types = RDR_BLD_PHYS;	/* Copier wants to see this */
    rdf_info.rdr_parts = dmucb->dmu_part_def;  /* What to copy */
    status = rdf_call(RDF_PART_COPY, &rdfcb);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = rdfcb.rdf_error.err_code;
	return (status);
    }
    /* Replace DMU cb pointer to partdef with pointer to new copy */
    dmucb->dmu_part_def = rdfcb.rdf_rb.rdr_newpart;
    dmucb->dmu_partdef_size = rdfcb.rdf_rb.rdr_newpartsize;

    return (E_DB_OK);
} /* copyPartDef */



/*
** partdefCopyMem is a memory allocator passed to the partition definition
** copier utility.  Since we have a ULM request block available, this is
** very simple.  (The ULM request block should be pointing to the temp
** QEF memory stream opened by qeu-dbu.)
**
** Call:  DB_STATUS status = partdefCopyMem(ulm, psize, &pptr);
**	ULM_RCB *ulm;  -- ULM stream request block
**	i4 psize;  -- Size of memory area needed
**	void *pptr;  -- Ref: where to store pointer to new area
**	Returns E_DB_xxx ok or error status
**
** History:
**	1-Apr-2004 (schka24)
**	    Write for DMF logging of repartitioning
*/

static DB_STATUS
partdefCopyMem(ULM_RCB *ulm, i4 psize, void *pptr)
{

    DB_STATUS status;

    ulm->ulm_psize = psize;
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
	return (status);
    *(void **)pptr = ulm->ulm_pptr;
    return (E_DB_OK);
} /* partdefCopyMem */



/*
** locPtrToArray reformats the current master location as returned by
** a DMT SHOW, into a straight array of DB_LOC_NAME's pointed to by a
** DM_DATA, for the sameLoc tests.  (And also for supplying to DMU if
** we determine that the should-be-at location for some partition is
** the master location and it ain't there currently.)  Note that
** we look at the pointer OUT count, and set the data IN size.
**
** The reformatted array is stored in the memory stream described by
** the ulm request block passed in by the caller.
**
** Call: DB_STATUS status = locPtrToArray(dmPtrLoc, dmDataLoc, ulm);
**	DM_PTR *dmPtrLoc -- loc array described by a dm-ptr (DB_LOC_NAME)
**	DM_DATA *dmDataLoc -- returned dm-data version of same (DB_LOC_NAME)
**	ULM_RCB *ulm;  -- Memory stream to allocate new array from
**	Returns E_DB_xxx status
**
** History:
**	8-Mar-2004 (schka24)
**	    Oops.  Master loc is the underlying default for modify, so
**	    we need a dm-data describing what the master location says.
*/

static DB_STATUS
locPtrToArray(DM_PTR *dmPtrLoc, DM_DATA *dmDataLoc, ULM_RCB *ulm)
{
    DB_LOC_NAME **locPptr;		/* Location entry pointer pointer */
    DB_LOC_NAME *locPtr;		/* Location name address */
    DB_STATUS status;
    i4 nlocs;				/* Number of locations */

    nlocs = dmPtrLoc->ptr_out_count;
    ulm->ulm_psize = nlocs * sizeof(DB_LOC_NAME);
    dmDataLoc->data_in_size = ulm->ulm_psize;
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
	return (status);
    dmDataLoc->data_address = (PTR) ulm->ulm_pptr;
    locPtr = (DB_LOC_NAME *) dmDataLoc->data_address;
    locPptr = (DB_LOC_NAME **) dmPtrLoc->ptr_address;
    while (--nlocs >= 0)
    {
	MEcopy(*locPptr, sizeof(DB_LOC_NAME), locPtr);
	++ locPptr;
	++ locPtr;
    }
    return (E_DB_OK);
} /* locPtrToArray */



/*
** sameLoc tests two location arrays for identity.  One of the arrays
** is pointed to by a DM_DATA; the other by DM_PTR.  (The latter coming
** from a dmt-show.)  Note that we look at the data IN size, and the
** pointer OUT count.
**
** Call: bool isSame = sameLoc(dmDataLoc, dmPtrLoc);
**	DM_DATA *dmDataLoc;  -- loc array described by a dm-data (DB_LOC_NAME)
**	DM_PTR *dmPtrLoc;  -- loc array described by a dm-ptr (DB_LOC_NAME)
**	Return TRUE if they're the same, else FALSE
**
** History:
**	1-Mar-2004 (schka24)
**	    Written.
**	9-Mar-2004 (schka24)
**	    Flailing about with location formats.
*/

static bool
sameLoc(DM_DATA *dmDataLoc, DM_PTR *dmPtrLoc)
{

    DB_LOC_NAME *locPtr;		/* Pointer to data style array */
    DB_LOC_NAME **locPptr;		/* Pointer to ptr style array */
    i4 nlocs;				/* Number of entries */

    nlocs = dmPtrLoc->ptr_out_count;
    if (nlocs != dmDataLoc->data_in_size / sizeof(DB_LOC_NAME))
	return (FALSE);

    locPtr = (DB_LOC_NAME *) dmDataLoc->data_address;
    locPptr = (DB_LOC_NAME **) dmPtrLoc->ptr_address;
    while (--nlocs >= 0)
    {
	if (MEcmp(locPtr, *locPptr, sizeof(DB_LOC_NAME)) != 0)
	    return (FALSE);
	++ locPtr;
	++ locPptr;
    }
    return (TRUE);
} /* sameLoc */

/*
** newPPcharArray creates a new ppchar array for the DMU request and
** initializes it to empty.  Table ID's for existing partitions are
** then copied in from the master physical-partition array.
**
** Call: DB_STATUS status = newPPcharArray(dmucb, ulm, nparts, pp_desc);
**	DMU_CB *dmucb;  -- The DMU request block
**	ULM_RCB *ulm;  -- The ULM request block to get memory from
**	i4 nparts;  -- The number of (new) physical partitions
**	DM_DATA pp_desc; -- Descriptor for master physical-partition array
**	Returns DB_xxx status, dmu_ppchar_array filled in
**
** History:
**	1-Mar-2004 (schka24)
**	    Written.
*/

static DB_STATUS
newPPcharArray(DMU_CB *dmucb, ULM_RCB *ulm, i4 nparts, DM_DATA *pp_desc)

{
    DB_STATUS status;
    DMT_PHYS_PART *pp_array;
    DMU_PHYPART_CHAR *ppc_base;
    DMU_PHYPART_CHAR *ppc_ptr;
    i4 i;
    i4 old_nparts;

    ulm->ulm_psize = nparts * sizeof(DMU_PHYPART_CHAR);
    dmucb->dmu_ppchar_array.data_in_size = ulm->ulm_psize;
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
	return (status);
    ppc_base = (DMU_PHYPART_CHAR *) ulm->ulm_pptr;
    dmucb->dmu_ppchar_array.data_address = (PTR) ppc_base;
    /* Init the array */
    MEfill(dmucb->dmu_ppchar_array.data_in_size, 0, ppc_base);
    /* Copy in all of the currently existing table ID's */
    old_nparts = pp_desc->data_out_size / sizeof(DMT_PHYS_PART);
    if (old_nparts > 0)
    {
	pp_array = (DMT_PHYS_PART *) pp_desc->data_address;
	ppc_ptr = ppc_base;
	/* The ppchar array has NEW-nparts entries, don't go off the end */
	if (old_nparts > nparts) old_nparts = nparts;
	for (i = 0; i < old_nparts; ++i, ++ppc_ptr)
	{
	    STRUCT_ASSIGN_MACRO(pp_array[i].pp_tabid, ppc_ptr->part_tabid);
	}
    }

    return (E_DB_OK);
} /* newPPcharArray */


/*
** setupAttrInfo does a dmt-show on the master to fetch the table attribute
** info, and reformats it from dmt-show style to dmu-create style.
**
** I'll leave it to the reader to devise suitable excoriations for a
** design that requires 80 lines of code (not counting blank lines and
** comments) to say "a := a".
**
** Call: DB_STATUS status = setupAttrInfo(dmucb, dmtshow, natts, ulm, err_code);
**	DMU_CB *dmucb;  -- DMU request block to attach attr info to
**	DMT_SHW_CB *dmtshow;  -- DMT_SHOW request block set up for master
**	i4 natts; -- Number of attributes in table
**	ULM_RCB *ulm;  -- ULM request block for getting memory
**	i4 *err_code;  -- Where to put ULM or DMT error details
**
** History:
**	1-Mar-2004 (schka24)
**	    Written.
**	16-dec-04 (inkdo01)
**	    Init a collID.
**	05-Oct-2009 (troal01)
**		Add geospatial support.
*/

static DB_STATUS
setupAttrInfo(DMU_CB *dmucb, DMT_SHW_CB *dmtshow,
	i4 natts, ULM_RCB *ulm, i4 *err_code)

{

    DB_STATUS status;
    DMT_ATT_ENTRY **attp_base;		/* Base of show pointer array */
    DMT_ATT_ENTRY **attpp;		/* A show pointer pointer */
    DMT_ATT_ENTRY *att_base;		/* Base of show attr entries */
    DMT_ATT_ENTRY *attp;		/* A show pointer */
    DMF_ATTR_ENTRY **dmu_attp_base;	/* Base of dmu pointer array */
    DMF_ATTR_ENTRY **dmu_attpp;		/* A dmu pointer pointer */
    DMF_ATTR_ENTRY *dmu_att_base;	/* Base of dmu attr entries */
    DMF_ATTR_ENTRY *dmu_attp;		/* A dmu entry pointer */
    i4 i;

    /* "show" attr numbers are 1-origined: */
    ++ natts;

    /* Allocate space for DMT show pointer array followed by attrs */
    ulm->ulm_psize = natts * (sizeof(DMT_ATT_ENTRY **) + sizeof(DMT_ATT_ENTRY));
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }
    attp_base = (DMT_ATT_ENTRY **) ulm->ulm_pptr;
    dmtshow->dmt_attr_array.ptr_address = (PTR) attp_base;
    att_base = (DMT_ATT_ENTRY *) ((PTR) attp_base + natts * sizeof(DMT_ATT_ENTRY **));

    /* Initialize the pointers */
    attpp = attp_base;
    attp = att_base;
    i = natts;
    do
	*attpp++ = attp++;
    while (--i > 0);
    dmtshow->dmt_attr_array.ptr_in_count = natts;
    dmtshow->dmt_attr_array.ptr_size = sizeof(DMT_ATT_ENTRY);

    /* Now do it all over again for the DMU attribute array.
    ** For some idiotic reason, the DMU attribute descriptor is DIFFERENT!
    ** Not only is the structure different, the attrs start at 0, not 1.
    */
    -- natts;
    ulm->ulm_psize = natts * (sizeof(DMF_ATTR_ENTRY **) + sizeof(DMF_ATTR_ENTRY));
    status = qec_malloc(ulm);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = ulm->ulm_error.err_code;
	return (status);
    }
    dmu_attp_base = (DMF_ATTR_ENTRY **) ulm->ulm_pptr;
    dmucb->dmu_attr_array.ptr_address = (PTR) dmu_attp_base;
    dmu_att_base = (DMF_ATTR_ENTRY *) ((PTR) dmu_attp_base + natts * sizeof(DMF_ATTR_ENTRY **));

    /* Initialize the pointers */
    dmu_attpp = dmu_attp_base;
    dmu_attp = dmu_att_base;
    i = natts;
    do
	*dmu_attpp++ = dmu_attp++;
    while (--i > 0);
    dmucb->dmu_attr_array.ptr_in_count = natts;
    dmucb->dmu_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

    /* Do a master show to get the attributes */
    dmtshow->dmt_flags_mask = DMT_M_ATTR;
    status = dmf_call(DMT_SHOW, dmtshow);
    if (DB_FAILURE_MACRO(status))
    {
	*err_code = dmtshow->error.err_code;
	return (status);
    }
    /* Translate DMT attributes to DMU attributes.  Big sigh. */
    attpp = attp_base;
    ++ attpp;				/* Show starts at 1, not 0 */
    dmu_attpp = dmu_attp_base;
    i = natts;
    do
    {
	attp = *attpp;
	dmu_attp = *dmu_attpp;
	MEcopy(&attp->att_name, sizeof(DB_ATT_NAME), &dmu_attp->attr_name);
	dmu_attp->attr_type = attp->att_type;
	dmu_attp->attr_size = attp->att_width;
	dmu_attp->attr_precision = attp->att_prec;
	dmu_attp->attr_flags_mask = attp->att_flags;
	dmu_attp->attr_collID = attp->att_collID;
	dmu_attp->attr_geomtype = attp->att_geomtype;
	dmu_attp->attr_srid = attp->att_srid;
	STRUCT_ASSIGN_MACRO(attp->att_defaultID, dmu_attp->attr_defaultID);
	dmu_attp->attr_defaultTuple = NULL;
	++ attpp;
	++ dmu_attpp;
    } while (--i > 0);

    /* We should be ready to create partitions now */
    return (E_DB_OK);
} /* setupAttrInfo */

/*
** setupDmuChar -- Set up DMU characteristics for new partitions.
** When modify creates new partitions, they have to share many of the
** same characteristics as the master.  In particular, they MUST share
** the same journaling status and page size.  There are a handful of
** other things we'll detect from the master as well.  Each of these
** things are passed to DMF as DMU characteristics entries.
**
** It's arguable whether or not DMF should be doing this.  Given that
** many things will fall over if the partitions look different from the
** master, a strong case could be made that DMU should simply be copying
** a bunch of stuff out of the master.  But, it's a little too inconvenient
** at this point -- we have the master info, and DMU create doesn't.
**
** The caller should point the DMU CB char-array pointer at an array
** that has enough room for all the entries we'll create, and start
** the char-array data_in_size at zero.
**
** Call: setupDmuChar(dmucb, dmtshow);
**	DMU_CB *dmucb;  -- DMU request block with char-array ready
**	DMT_SHW_CB *dmtshow;  -- "Show" info for master
**
** History:
**	30-Dec-2005 (kschendel)
**	    Statement-level-unique was checked twice, remove one.
*/

static void
setupDmuChar(DMU_CB *dmucb, DMT_SHW_CB *dmtshow)
{

    DMT_TBL_ENTRY *tbl;			/* Master table info */
    DMU_CHAR_ENTRY *chr;		/* Next characteristics array entry */

    chr = (DMU_CHAR_ENTRY *) dmucb->dmu_char_array.data_address;

    tbl = (DMT_TBL_ENTRY *) dmtshow->dmt_table.data_address;

    /* Journaling? */
    if (tbl->tbl_status_mask & (DMT_JON | DMT_JNL))
    {
	chr->char_id = DMU_JOURNALED;
 	chr->char_value = DMU_C_ON;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* Page size */
    chr->char_id = DMU_PAGE_SIZE;
    chr->char_value = tbl->tbl_pgsize;
    ++chr;
    dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);

    /* Unique? */
    if (tbl->tbl_status_mask & DMT_UNIQUEKEYS)
    {
	chr->char_id = DMU_UNIQUE;
 	chr->char_value = DMU_C_ON;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* Duplicates allowed? */
    if (tbl->tbl_status_mask & DMT_DUPLICATES)
    {
	chr->char_id = DMU_DUPLICATES;
 	chr->char_value = DMU_C_ON;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* Statement level unique? */
    if (tbl->tbl_2_status_mask & DMT_STATEMENT_LEVEL_UNIQUE)
    {
	chr->char_id = DMU_STATEMENT_LEVEL_UNIQUE;
 	chr->char_value = DMU_C_ON;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* Cache priority */
    if (tbl->tbl_cachepri != 0)
    {
	chr->char_id = DMU_TABLE_PRIORITY;
 	chr->char_value = tbl->tbl_cachepri;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    /* Extend */
    /* Not clear how this is defaulted, I'll just check for zero; this may
    ** cause extend to be specified for all new partitions, but that's OK
    */
    if (tbl->tbl_extend != 0)
    {
	chr->char_id = DMU_EXTEND;
	chr->char_value = tbl->tbl_extend;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

    if (tbl->tbl_status_mask & DMT_SYS_MAINTAINED)
    {
	chr->char_id = DMU_SYS_MAINTAINED;
	chr->char_value = DMU_C_ON;
	++ chr;
	dmucb->dmu_char_array.data_in_size += sizeof(DMU_CHAR_ENTRY);
    }

} /* setupDmuChar */

/*
** createTblErrXlate -- Translate and issue a CREATE TABLE error message.
** The DMU create table operation returns a bunch of different DMF
** error messages, which this routine translates into QEF or user-error
** style error messages.  One error that is NOT handled is the
** E_DM0101_SET_JOURNAL_ON "error", which is really an informational message;
** the caller should trap that one and not get here.
**
** Call: i4 translated_error = createTblErrXlate(dmucb, tbl_name, dmf_err, status);
**	DMU_CB *dmucb;  -- The create table DMU request block
**	char *tbl_name;  -- The (master) table name
**	i4 dmf_err;  -- The error code as returned by DMF
**	DB_STATUS status;  -- The error status as returned by DMF
**	Returns an appropriate QEF error code
*/

static i4
createTblErrXlate(DMU_CB *dmucb, char *tbl_name,
	i4 dmf_error, DB_STATUS status)

{
    char masterNameAsciz[DB_TAB_MAXNAME+1];
    DB_ERROR toss_errblk;
    DB_LOC_NAME *loc;
    i4 errcode = dmucb->error.err_code;
    i4 loc_idx;
    i4 toss_error;

    STcopy(tbl_name, masterNameAsciz);
    STtrmwhite(&masterNameAsciz[0]);
    switch (dmf_error)
    {

	case E_DM0078_TABLE_EXISTS:
	    qef_error(5102L, 0L, status, &toss_error,
		&toss_errblk, 2, 
		(sizeof ("CREATE") - 1), "CREATE",
		0, masterNameAsciz);
	    errcode = E_QE0025_USER_ERROR;
	    break;

	case E_DM001D_BAD_LOCATION_NAME:
	    loc_idx = dmucb->error.err_data;
	    loc = ((DB_LOC_NAME *) (dmucb->dmu_location.data_address)) + loc_idx;
	    qef_error(5113L, 0L, status, &toss_error, &toss_errblk, 
		      1, sizeof(DB_LOC_NAME), (PTR)(loc));
	    errcode = E_QE0025_USER_ERROR;
	    break;

	case E_DM0181_PROD_MODE_ERR:
	    qef_error(errcode, 0L, status, &toss_error, &toss_errblk, 0);
	    errcode = E_QE0025_USER_ERROR;
	    break;
	case E_DM0157_NO_BMCACHE_BUFFERS:
	    qef_error(errcode, 0L, status, &toss_error, &toss_errblk, 0);
	    errcode = E_QE0025_USER_ERROR;
	    break;
	case E_DM0189_RAW_LOCATION_INUSE:
	    loc_idx = dmucb->error.err_data;
	    loc = ((DB_LOC_NAME *)
		(dmucb->dmu_location.data_address)) + loc_idx;
	    qef_error(E_DM0189_RAW_LOCATION_INUSE, 0L, 
			status, &toss_error, &toss_errblk, 
		      1, sizeof(DB_LOC_NAME), (PTR)(loc));
	    errcode = E_QE0025_USER_ERROR;
	    break;
	case E_DM0190_RAW_LOCATION_OCCUPIED:
	    loc_idx = dmucb->error.err_data;
	    loc = ((DB_LOC_NAME *)
		(dmucb->dmu_location.data_address)) + loc_idx;
	    qef_error(E_DM0190_RAW_LOCATION_OCCUPIED, 0L, 
			status, &toss_error, &toss_errblk, 
		      1, sizeof(DB_LOC_NAME), (PTR)(loc));
	    errcode = E_QE0025_USER_ERROR;
	    break;
	case E_DM0191_RAW_LOCATION_MISSING:
	    qef_error(E_DM0191_RAW_LOCATION_MISSING, 0L, 
			status, &toss_error, &toss_errblk, 0);
	    errcode = E_QE0025_USER_ERROR;
	    break;
    }

    return (errcode);
} /* createTblErrXlate

/*
** Name: qeu_modify_finish -- Finish up after repartitioning modify
**
** Description:
**	After a repartitioning modify, there is additional work to do.
**	If the new partitioning scheme had fewer physical partitions than
**	the original scheme, we need to drop the now-unused extra physical
**	partitions.  Also, we need to update the partitioning catalogs.
**
**	Notice that when dropping partitions, we specify the no-file
**	flag.  MODIFY has already renamed the original partition file
**	to a drop-temp as part of normal modify processing, so the
**	disk file isn't there any more.  DROP still has to do all the
**	other catalog stuff, though.
**
** Inputs:
**	qef_cb 		QEF session control block
**	dmucb		Modify DMU request block
**	pp_array	DM_DATA pointer describing original physical partitions
**
** Outputs:
**	Returns E_DB_xxx status
**
**
** History:
**	1-Mar-2004 (schka24)
**	    Get partitioning modify working.
*/

static DB_STATUS
qeu_modify_finish(QEF_CB *qef_cb, DMU_CB *dmucb, DM_DATA *pp_array)

{
    DB_STATUS status;
    DMT_PHYS_PART *pp_base;		/* Base of actual pp array */
    DMU_CB drop_dmucb;			/* DMU block for dropping tables */
    i4 new_nparts;			/* New number of partitions */
    i4 old_nparts;			/* original number of partitions */
    i4 partno;				/* Physical partition number */
    RDF_CB rdfcb;			/* RDF request block */

    old_nparts = pp_array->data_out_size / sizeof(DMT_PHYS_PART);
    pp_base = (DMT_PHYS_PART *) pp_array->data_address;
    new_nparts = dmucb->dmu_part_def->nphys_parts;
    if (dmucb->dmu_part_def->ndims == 0)
	new_nparts = 0;
    if (old_nparts > new_nparts)
    {
	/* We need to drop some old partitions */

	/* Don't smash caller's DMU cb in case someone looks at it */
	/* (this is probably paranoia */
	STRUCT_ASSIGN_MACRO(*dmucb, drop_dmucb);
	drop_dmucb.dmu_flags_mask = DMU_PARTITION | DMU_DROP_NOFILE;
	drop_dmucb.dmu_char_array.data_address = NULL;
	drop_dmucb.dmu_char_array.data_in_size = 0;
	for (partno = new_nparts; partno < old_nparts; ++partno)
	{
	    STRUCT_ASSIGN_MACRO(pp_base[partno].pp_tabid, drop_dmucb.dmu_tbl_id);
	    status = dmf_call(DMU_DESTROY_TABLE, &drop_dmucb);
	    if (DB_FAILURE_MACRO(status))
	    {
		STRUCT_ASSIGN_MACRO(drop_dmucb.error, dmucb->error);
		return (status);
	    }
	}
    }

    /* Ok, now try writing out the new partition definition */
    qeu_rdfcb_init((PTR) &rdfcb, qef_cb);
    STRUCT_ASSIGN_MACRO(dmucb->dmu_tbl_id, rdfcb.rdf_rb.rdr_tabid);
    rdfcb.rdf_rb.rdr_newpart = dmucb->dmu_part_def;
    status = rdf_call(RDF_PART_UPDATE, &rdfcb);
    if (DB_FAILURE_MACRO(status))
    {
	/* Put errcode where caller can find it */
	dmucb->error.err_code = rdfcb.rdf_error.err_code;
	return (status);
    }

    return (E_DB_OK);
} /* qeu_modify_finish */

/*
** Name: qeu_dqrymod_objects -	destroy qrymod objects corresponding to a given
**				query id
**
** Description:
**	obtain, if necessary, tbl_status_mask for the table and proceed to
**	destroy relevant qrymod objects
**
** Input:
**	qef_cb		    QEF control block
**	qeuq_cb		    QEUQ control block initialized by the caller
**	    qeuq_rtbl	    id of the table whose qrymod objects must be
**			    destroyed
**	    qeuq_db_id
**	    qeuq_d_id
**	tbl_status_mask	    ptr to the relstat mask for the table; may be set to
**			    NULL if the caller prefers to have it obtained here
**	tbl_2_status_mask   ptr to the relstat2 mask for the table; may be set 
**			    to NULL if the caller prefers to have it obtained 
**			    here.
**
** Output
**	qeuq_cb
**	    error.err_code  filled in if an error occurred
**
** Returns:
**	E_DB_{OK | ERROR}
**
**
** History:
**	11-dec-90 (andre)
**	    written
**	16-oct-91 (bryanp)
**	    Part of 6.4 -> 6.5 merge changes. Must initialize new dmt_char_array
**	    field for DMT_SHOW calls.
**	22-jul-92 (andre)
**	    qeu_dprot() expects to see QEU_DROP_ALL set in
**	    qeuq_cb->qeuq_permit_mask if all permits and/or security_alarms are
**	    to be dropped
**	23-jul-92 (andre)
**	    let qeu_dprot() know that it does not need to check whether
**	    destruction of specified permit(s) would render any objects
**	    abandoned by setting QEU_SKIP_ABANDONED_OBJ_CHECK in
**	    qeuq_permit_mask
**	21-mar-93 (andre)
**	    qeu_dintg() expects the caller to notify it whether it should
**	    destroy "old style" INGRES integrities, constraints or both.  Since
**	    here we call qeu_dintg() in conjunction with destroying a table,
**	    tell qeu_dintg() to drop all "old style" INGRES integrities and
**	    constraints
**	14-apr-93 (andre)
**	    tell qeu_drule() that it is NOT being called as a part of DROP RULE
**	    processing
**	16-oct-93 (andre)
**	    when calling qeu_drop_synonym(), set QEU_DROP_IDX_SYN only if table
**	    being dropped has indices defined on it
**	7-dec-93 (robf)
**          drop any security alarms on the table.
**	16-feb-94 (robf)
**          (Performance) only call qeu_dsecalarm() if DMF  indicates
**	    the table may have alarms.
**	22-jul-96 (ramra01)
**	    Alter table drop col cascade/restrict. 
**	27-dec-1996 (nanpr01)
**	    bug 79853 : restrict option proceeds even if comment dependecy
**	    is present.
**      30-Nov-2009 (coomi01) b122958
**          Make check on comments present column specific.
*/
DB_STATUS
qeu_dqrymod_objects(
QEF_CB		*qef_cb,
QEUQ_CB		*qeuq_cb,
i4		*tbl_status_mask,
i4 	*tbl_2_status_mask)
{
    DB_IICOMMENT outBuffer;
    DB_IICOMMENT *outPtr;
    i4		relstat;
    i4		err_num;
    i4		relstat2;
    bool		drop_col;
    bool		cascade;
    DB_STATUS		status = E_DB_OK;

    for (;;)		/* something to break out of */
    {
	/*
	** if caller has not supplied us with the table's status_mask, call
	** DMF to get it now 
	*/
	if (tbl_status_mask == (i4 *) NULL ||
	    tbl_2_status_mask == (i4 *)NULL)
	{
	    DMT_SHW_CB		dmt_show;
	    DMT_TBL_ENTRY	dmt_tbl_entry;

	    /* Format the DMT_SHOW request block */
	    dmt_show.type = DMT_SH_CB;
	    dmt_show.length = sizeof(DMT_SHW_CB);
	    dmt_show.dmt_char_array.data_in_size = 0;
	    dmt_show.dmt_char_array.data_out_size  = 0;
	    STRUCT_ASSIGN_MACRO(*qeuq_cb->qeuq_rtbl, dmt_show.dmt_tab_id);
	    dmt_show.dmt_flags_mask = DMT_M_TABLE;
	    dmt_show.dmt_db_id	= qeuq_cb->qeuq_db_id;
	    dmt_show.dmt_session_id = qeuq_cb->qeuq_d_id;
	    dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	    dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);

	    status = dmf_call(DMT_SHOW, &dmt_show);

	    if (status && dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
	    {
		err_num = dmt_show.error.err_code;
		break;
	    }

	    status = E_DB_OK;

	    /*
	    ** relstat will be set regardless of whether we got it from the
	    ** caller or from DMF
	    */
	    relstat = dmt_tbl_entry.tbl_status_mask;
	    relstat2 = dmt_tbl_entry.tbl_2_status_mask;
	}
	else
	{
	    relstat = *tbl_status_mask;
	    relstat2 = *tbl_2_status_mask;
	}

	/* Destroy qrymod objects as necessary */

	qeuq_cb->qeuq_status_mask = relstat;

	drop_col = (qeuq_cb->qeuq_flag_mask & QEU_DROP_COLUMN) ? TRUE : FALSE;
	cascade  = (qeuq_cb->qeuq_flag_mask & QEU_DROP_CASCADE) ? TRUE : FALSE;

	if (relstat & DMT_PROTECTION)
	{
	    qeuq_cb->qeuq_pno		= 0;
	    qeuq_cb->qeuq_permit_mask   = QEU_PERM_OR_SECALM | QEU_DROP_ALL |
		QEU_SKIP_ABANDONED_OBJ_CHECK;

	    qeuq_cb->qeuq_flag_mask	= 0;

	    if (drop_col)
	    {
	       qeuq_cb->qeuq_flag_mask |= QEU_DROP_COLUMN;

	       if (cascade)
		  qeuq_cb->qeuq_flag_mask |= QEU_DROP_CASCADE;
	    }

	    if (status = qeu_dprot(qef_cb, qeuq_cb))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	if (relstat & DMT_INTEGRITIES)
	{
	    qeuq_cb->qeuq_ino = 0;	/* all integrities */

	    if (drop_col)
	    {
	       qeuq_cb->qeuq_flag_mask |= QEU_DROP_COLUMN;

	       if (cascade)
		  qeuq_cb->qeuq_flag_mask |= QEU_DROP_CASCADE;
	    }

	    if (status = qeu_dintg(qef_cb, qeuq_cb, (bool) TRUE, (bool) TRUE))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	/*
	** If there are rules remove them all.  The setting of qeuq_cr to 0 
	** implies there are no rule tuples (ie, not from DROP RULE) and 
	** deletes rules by table id (qeuq_rtbl) rather than by name.
	*/
	if (relstat & DMT_RULE)
	{
	    qeuq_cb->qeuq_cr = 0;	/* No rule tuple - by table id */

	    if (drop_col)
	    {
	       qeuq_cb->qeuq_flag_mask |= QEU_DROP_COLUMN;

	       if (cascade)
		  qeuq_cb->qeuq_flag_mask |= QEU_DROP_CASCADE;
	    }

	    if (status = qeu_drule(qef_cb, qeuq_cb, (bool) FALSE))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}
	/*
	** Drop any security alarms on the table
	*/
	if ( (relstat2 & DMT_ALARM)  && ( drop_col == FALSE ) )
	{
	    qeuq_cb->qeuq_permit_mask = 0;
	    if (status = qeu_dsecalarm(qef_cb, qeuq_cb, (bool) FALSE))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	if (relstat & DMT_ZOPTSTATS)
	{
	    if (drop_col)
	       qeuq_cb->qeuq_flag_mask |= QEU_DROP_COLUMN;

	    if (status = qeu_dstat(qef_cb, qeuq_cb))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	/*
	** will try to drop synonyms if relstat indicates that there may be
	** synonyms defined for this object or if the object is a base
	** table with indices defined on it
	*/
	if ( (relstat & (DMT_SYNONYM | DMT_IDXD))  && (drop_col == FALSE) )
	{
	    qeuq_cb->qeuq_uld_tup = NULL;

	    /*
	    ** NOTE:
	    ** 1) If we are dropping an index, qeu_drop_synonym() needs to
	    **    compare both base and index id to make sure that only the
	    **    synonyms defined for the index get dropped.
	    ** 2) If the object being dropped is a base table with indices
	    **    defined on it, there may be synonyms defined for the
	    **    indices, so we need to let qeu_drop_synonym() know that
	    **    synonyms defined for the table as well as for indices must
	    **    be dropped.  This is accomplished by setting
	    **    QEU_DROP_IDX_SYN bit in qeuq_cb->qeuq_flag_mask.
	    */

	    if (relstat & DMT_IDXD)
	    {
		qeuq_cb->qeuq_flag_mask |= QEU_DROP_IDX_SYN;
	    }
	    
	    if (status = qeu_drop_synonym(qef_cb, qeuq_cb))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	/* 
	** If the caller is trying to drop a column in restricted mode
	** use the DMT_COMMENT bit as a hint to search for THIS column
	** in iidbms_comments. 
	*/
	if (drop_col && (!cascade)&& (relstat & DMT_COMMENT))
	{
	    qeuq_cb->qeuq_uld_tup = NULL;

	    /* 
	    ** Find the comment 
	    ** ~ We own the buffer space, and we point at it
	    ** ~ We then pass the address of the pointer
	    */
	    outPtr = &outBuffer;
	    qeu_fcomment(qef_cb, qeuq_cb, &outPtr);

	    /*
	    ** If the outPtr remains intact, the comment was found
	    ** and is contained in the buffer pointed at.
	    ** ~ If the pointer was reset to NULL then it means the
	    **   comment was not found, and our buffer remains empty.
	    */
	    if ( NULL != outPtr )
	    {
		/* Comment presnt, So deny the request */
		err_num = E_QE016B_COLUMN_HAS_OBJECTS;
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** if there are comments on the table, or on columns of the table,
	** drop them all.
	*/
	if (relstat & DMT_COMMENT)
	{
	    if (drop_col)
	    {
	       qeuq_cb->qeuq_flag_mask |= QEU_DROP_COLUMN;
	    }

	    qeuq_cb->qeuq_uld_tup = NULL;

	    if (!cascade)
	    {
		break;
	    }

	    if (status = qeu_dcomment(qef_cb, qeuq_cb))
	    {
		err_num = qeuq_cb->error.err_code;
		break;
	    }
	}

	break;
    }

    if (status != E_DB_OK)
    {
	(VOID) qef_error(err_num, 0L, status, &err_num, &qeuq_cb->error, 0);
    }

    return(status);
}

/*{
** Name: collectPersistentIndices - identify a table's persistent indices
**
** Description:
**	This routine finds all the persistent indices on a table and passes
**	back an array of DMU_CBs.  Each DMU_CB corresponds to a persistent
**	index and is suitable for instructing DMF to recreate the index.
**	For each persistent index, this routine also hands back its table
**	ID.
**
**	This routine is called before a table is modified.  Modification
**	will destroy all indices on the table.  The DMU_CBs cooked up here
**	are used to recreate the persistent indices.  The index IDs are
**	used to hunt down IIDBDEPENDS tuples involving the indices and
**	to update them;  also to drop any views defined on the indices.
**
**
** Inputs:
**	ulm		ULM_RCB of memory stream to allocate objects out of
**	qef_rcb		QEF_RCB of the caller
**	baseTableID	db_tab_base of the base table
**
** Outputs:
**	indexDMU_CBs	linked list of DMU_CBs for recreating the persistent
**				indices
**	oldIndexIDs	array of DB_TAB_IDs of persistent indices
**	returnedNumberOfPersistentIndice	# of persistent indices
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	16-Mar-2004 (schka24)
**	    Pass just base ID, caller might be working on a partition.
**	10-Mar-2005 (schka24)
**	    Pass thru becoming-partitioned flag.
**
*/

static DB_STATUS
collectPersistentIndices(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    i4			baseTableID,
    i4			changing_part,
    DMU_CB		**indexDMU_CBs,	/* linked list of DMU_CBs for
					** recreating the persistent indices */
    DB_TAB_ID		**oldIndexIDs,
    i4			*returnedNumberOfPersistentIndices
)
{
    DB_STATUS		status = E_DB_OK;
    DB_TAB_ID		baseId;
    DB_TAB_ID		*indexIDs;
    DMT_IDX_ENTRY	*currentIndex;
    DMT_IDX_ENTRY	**indexEntries;
    DMU_CB		*newDMU_CB;
    DMU_CB		*currentDMU_CB;
    i4			numberOfIndices;
    i4			i;
    i4			numberOfPersistentIndices;
    DMT_SHW_CB		*baseTableInfo;
    DMT_SHW_CB		indexDMT_TBL_ENTRY;
    DMT_TBL_ENTRY	tableEntry;

    *returnedNumberOfPersistentIndices = numberOfPersistentIndices = 0;
    *indexDMU_CBs = currentDMU_CB = ( DMU_CB * ) NULL;
    *oldIndexIDs = ( DB_TAB_ID * ) NULL;
    baseId.db_tab_base = baseTableID;
    baseId.db_tab_index = 0;

    /*
    ** do a dmt_show on baseTableID, getting table, attribute, and index
    ** information
    */

    status = lookupTableInfo( ulm, qef_rcb, &baseId, &baseTableInfo );
    if ( status != E_DB_OK)	return( status );

    /* allocate an array of index IDs for all possible indexes in this table */

    numberOfIndices = baseTableInfo->dmt_index_array.ptr_out_count;
    indexEntries =
	( DMT_IDX_ENTRY ** ) baseTableInfo->dmt_index_array.ptr_address;

    /* if there are no indices, there is nothing to do */

    if ( numberOfIndices == 0 )	return( status );

    GET_MEMORY( numberOfIndices * sizeof( DB_TAB_ID ), &indexIDs );
    *oldIndexIDs = indexIDs;

    /*
    ** fill in a DMU_CB for each persistent index.  This index will be
    ** used by our caller to recreate the index after the base table is
    ** modified.
    */

    for ( i = 0; i < numberOfIndices; i++ )
    {
	currentIndex = indexEntries[ i ];

	status = lookupTableEntry( qef_rcb, &currentIndex->idx_id, &indexDMT_TBL_ENTRY,
			&tableEntry );
	if ( status != E_DB_OK )	return( status );

	if ( tableEntry.tbl_2_status_mask & DMT_PERSISTS_OVER_MODIFIES )
	{
	    /* save this index id for later reference by our caller */

	    MEcopy( ( PTR ) &currentIndex->idx_id, sizeof( DB_TAB_ID ),
		    ( PTR ) &indexIDs[ numberOfPersistentIndices++ ] );

	    /* allocate space for another DMU_CB */

	    GET_MEMORY( sizeof( DMU_CB ), &newDMU_CB );

	    /*
	    ** build into this DMU_CB everything we need to know to recreate
	    ** the index after modifying its base relation
	    */

	    status = buildPersistentIndexDMU_CB( ulm, qef_rcb, newDMU_CB,
			baseTableInfo, changing_part,
			i, &indexDMT_TBL_ENTRY );
	    if ( status != E_DB_OK )	return( status );

	    newDMU_CB->q_next = ( PTR ) NULL;
	    if ( currentDMU_CB == ( DMU_CB * ) NULL )
	    {
	    	*indexDMU_CBs = newDMU_CB;
	    }
	    else
	    {
		currentDMU_CB->q_next = ( PTR ) newDMU_CB;
	    }
	    currentDMU_CB = newDMU_CB;

	}	/* end if this index supports a constraint */

    }	/* end loop on indices */

    *returnedNumberOfPersistentIndices = numberOfPersistentIndices;
    return( status );
}




/*{
** Name: lookupTableInfo - do some DMT_SHOWs on a table
**
** Description:
**
**	This routine gets the table, attribute, and index descriptor for
**	a table.
**
**
** Inputs:
**	ulm		memory stream
**	qef_rcb		request control block.  loaded with goodies
**	baseTableID	DB_TAB_ID of the base table whose info we want
**
** Outputs:
**	tableCB		returned DMT_SHW_CB replete with dangling table,
**			attribute, and index descriptors
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static DB_STATUS
lookupTableInfo(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*baseTableID,
    DMT_SHW_CB		**tableCB
)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4		error;
    DB_STATUS		status = E_DB_OK;
    DMT_SHW_CB		*dmt_shw_cb;
    DMT_TBL_ENTRY	*tableEntry;
    i4		numberOfAttributes;
    i4		numberOfIndices;

    /* allocate a DMT_SHW_CB and a table entry descriptor */

    GET_MEMORY( sizeof( DMT_SHW_CB ), tableCB );
    dmt_shw_cb = *tableCB;

    GET_MEMORY( sizeof( DMT_TBL_ENTRY ), &tableEntry );

    /* look up the table entry for this table */

    status = lookupTableEntry( qef_rcb, baseTableID, dmt_shw_cb, tableEntry );
    if ( status != E_DB_OK )	return( status );

    /*
    ** that gave us enough information to figure out how big the
    ** attribute and index arrays should be.
    */

    dmt_shw_cb->dmt_flags_mask = 0;

    numberOfAttributes = tableEntry->tbl_attr_count;
    numberOfIndices = tableEntry->tbl_index_count;

    /* now the attribute array */

    if ( numberOfAttributes > 0 )
    {
        status = buildDM_PTR( ulm, qef_rcb, numberOfAttributes + 1,
			sizeof( DMT_ATT_ENTRY ),
			&dmt_shw_cb->dmt_attr_array );
	if ( status != E_DB_OK )	return( status );

        dmt_shw_cb->dmt_flags_mask |= DMT_M_ATTR;
    }

    /* now the index array */

    if ( numberOfIndices > 0 )
    {
        status = buildDM_PTR( ulm, qef_rcb, numberOfIndices,
			sizeof( DMT_IDX_ENTRY ),
			&dmt_shw_cb->dmt_index_array );
	if ( status != E_DB_OK )	return( status );

        dmt_shw_cb->dmt_flags_mask |= DMT_M_INDEX;
    }

    /*
    ** oh great DMF, please do not reject our control block, unworthy
    ** though it is.
    */

    if ( dmt_shw_cb->dmt_flags_mask != 0 )
    {
        status = dmf_call(DMT_SHOW, dmt_shw_cb);
	if ( status != E_DB_OK )
	{
            QEF_ERROR( dmt_shw_cb->error.err_code );
            return (status);
        }
    }

    return( status );
}


/*{
** Name: lookupTableEntry - get the DMT_TBL_ENTRY for a table
**
** Description:
**
**	This routine performs the first DMT_SHOW on a table, requesting the
**	high level table descriptor.
**
**
** Inputs:
**	qef_rcb		request control block
**	tableID		table to do the DMT_SHOW on
**	tableCB		DMT_SHW_CB to fill in
**	tableEntry	table entry to fill in
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

#define	ZERO_DM_PTR( dm_ptr )	\
	dm_ptr.ptr_in_count = dm_ptr.ptr_out_count = 0;	\
	dm_ptr.ptr_address = ( PTR ) NULL;

#define	ZERO_DM_DATA( dm_data )	\
	dm_data.data_in_size = dm_data.data_out_size = 0;	\
	dm_data.data_address = ( char * ) NULL;

static DB_STATUS
lookupTableEntry(
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*tableID,
    DMT_SHW_CB		*dmt_shw_cb,
    DMT_TBL_ENTRY	*tableEntry
)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4		error;
    DB_STATUS		status = E_DB_OK;

    /* boiler plate */

    dmt_shw_cb->length = sizeof(DMT_SHW_CB);
    dmt_shw_cb->type = DMT_SH_CB;
    dmt_shw_cb->dmt_db_id = qef_rcb->qef_db_id;
    dmt_shw_cb->dmt_session_id = qef_cb->qef_ses_id;
    MEcopy( ( PTR ) tableID, sizeof( DB_TAB_ID ),
	    ( PTR ) &dmt_shw_cb->dmt_tab_id );

    /* setup to ask DMF for table information */

    dmt_shw_cb->dmt_flags_mask = DMT_M_TABLE;
    dmt_shw_cb->dmt_table.data_address = ( PTR ) tableEntry;
    dmt_shw_cb->dmt_table.data_in_size = sizeof( DMT_TBL_ENTRY );

    ZERO_DM_PTR( dmt_shw_cb->dmt_attr_array );
    ZERO_DM_PTR( dmt_shw_cb->dmt_index_array );
    ZERO_DM_PTR( dmt_shw_cb->dmt_loc_array );
    ZERO_DM_DATA( dmt_shw_cb->dmt_char_array );

    /* great DMF, we beseech thee */

    status = dmf_call(DMT_SHOW, dmt_shw_cb);
    if ( status != E_DB_OK )
    {
        QEF_ERROR( dmt_shw_cb->error.err_code );
        return (status);
    }

    return( status );
}



/*{
** Name: buildPersistentIndexDMU_CB - populate a DMU_CB for recreating an index
**
** Description:
**	This routine populates a DMU_CB, describing how to
**	recreate a persistent index.
**
** Inputs:
**	ulm			memory stream
**	qef_rcb			request control block
**	dmu_cb			DMU_CB to fill in
**	baseTableDMT_SHW_CB	table information on base table
**	changing_part		Indicates whether partition-ness of base
**				table is changing
**	indexNumber		number of this index
**	indexDMT_SHW_CB		table information on this index
**
** Outputs:
**	dmu_cb			DMU_CB to fill in
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	21-nov-1996 (shero03)
**	    Add support for RTree indexes
**	18-Dec-1996 (nanpr01)
**	    Bug 79836 - Persistent index is getting reverted back to
**	    default_page_size.
**	18-Aug-1997 (shero03)
**	    Bug 84473: Rtree persistent index messes up whole database.
**      17-Apr-2001 (horda03) Bug 104402
**          Store the DMU_NOT_UNIQUE characteristic.
**	16-Mar-2004 (schka24)
**	    Preserve the globalness of indexes.
**	10-Mar-2005 (schka24)
**	    Also, if base table is becoming partitioned, its indexes need
**	    to become global (the default); and vice versa.
*/

static DB_STATUS
buildPersistentIndexDMU_CB(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    DMU_CB		*dmu_cb,
    DMT_SHW_CB		*baseTableDMT_SHW_CB,
    i4			changing_part,
    i4			indexNumber,
    DMT_SHW_CB		*indexDMT_SHW_CB
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    DMT_IDX_ENTRY	**indexArray =
	( DMT_IDX_ENTRY ** ) baseTableDMT_SHW_CB->dmt_index_array.ptr_address;
    DMT_ATT_ENTRY	**attributePointers =
	( DMT_ATT_ENTRY ** ) baseTableDMT_SHW_CB->dmt_attr_array.ptr_address;
    DMT_ATT_ENTRY	*attributeEntry;
    i4		attributeNumber;
    DB_LOC_NAME		**sourceLocationPointers;
    DB_LOC_NAME		*targetLocations;
    DMT_IDX_ENTRY	*indexDMT_IDX_ENTRY = indexArray[ indexNumber ];
    i4		numberOfLocations;
    DMT_TBL_ENTRY	*indexDMT_TBL_ENTRY =
        ( DMT_TBL_ENTRY * ) indexDMT_SHW_CB->dmt_table.data_address;
    i4			i;
    i4			sizeOfLocationArray;
    i4			sizeofRange;
    i4		numberOfIndexAttributes;
    DMU_KEY_ENTRY	**keyPointers;
    DMU_KEY_ENTRY	*keyEntry;
    i4		structureType;
    i4			toggle;
    i4			pass;

    /* Fill in the DMU control block header */
    dmu_cb->type = DMU_UTILITY_CB;
    dmu_cb->length = sizeof(DMU_CB);
    dmu_cb->dmu_flags_mask = 0;
    dmu_cb->dmu_db_id = baseTableDMT_SHW_CB->dmt_db_id;
    STRUCT_ASSIGN_MACRO( baseTableDMT_SHW_CB->dmt_owner, dmu_cb->dmu_owner);
    dmu_cb->dmu_gw_id = DMGW_NONE;

    dmu_cb->dmu_tran_id = qef_cb->qef_dmt_id;
    dmu_cb->dmu_tbl_id.db_tab_base =
	baseTableDMT_SHW_CB->dmt_tab_id.db_tab_base;
    dmu_cb->dmu_tbl_id.db_tab_index = 0;
    MEcopy( ( PTR ) &indexDMT_IDX_ENTRY->idx_name,
	    sizeof( DB_TAB_NAME ), ( PTR ) &dmu_cb->dmu_index_name );

    /* for error handling */
    MEcopy( ( PTR ) &baseTableDMT_SHW_CB->dmt_name,
	    sizeof( DB_TAB_NAME ), ( PTR ) &dmu_cb->dmu_table_name );

    /* Clear RTree fields */
    dmu_cb->dmu_range_cnt = 0;
    dmu_cb->dmu_range = NULL;

    /* find the location array for the old index */

    numberOfLocations = indexDMT_TBL_ENTRY->tbl_loc_count;
    if ( numberOfLocations > 1 )
    {
        status = buildDM_PTR( ulm, qef_rcb, numberOfLocations,
			sizeof( DB_LOC_NAME ),
			&indexDMT_SHW_CB->dmt_loc_array );
	if ( status != E_DB_OK )	return( status );

        indexDMT_SHW_CB->dmt_flags_mask = DMT_M_LOC;

        status = dmf_call(DMT_SHOW, indexDMT_SHW_CB );
	if ( status != E_DB_OK )
	{
            QEF_ERROR( indexDMT_SHW_CB->error.err_code );
            return (status);
        }
    }

    /* copy the location array */

    sizeOfLocationArray = sizeof( DB_LOC_NAME ) * numberOfLocations;
    GET_MEMORY( sizeOfLocationArray, &targetLocations );

    dmu_cb->dmu_location.data_address = ( PTR ) targetLocations;
    dmu_cb->dmu_location.data_in_size = sizeOfLocationArray;

    if ( numberOfLocations > 1 )
    {
	/*
	** now we have to convert the dmt_shw_cb's DM_PTR to the dmu_cb's
	** DM_DATA
	*/

	sourceLocationPointers =
		( DB_LOC_NAME ** ) indexDMT_SHW_CB->dmt_loc_array.ptr_address;

	for ( i = 0; i < numberOfLocations; i++ )
	{
	    MEcopy( *sourceLocationPointers++,
		    sizeof( DB_LOC_NAME ),
		    ( PTR ) &targetLocations[ i ] );
	}
    }
    else	/* only one location */
    {
	MEcopy( ( PTR ) &indexDMT_TBL_ENTRY->tbl_location,
		sizeof( DB_LOC_NAME ),
		( PTR ) &targetLocations[ 0 ] );
    }

    /* source of gateway table for gateway registration.  not applicable */

    dmu_cb->dmu_olocation.data_address = ( char * ) NULL;

    /* copy the range array, if any */

    sizeofRange = indexDMT_TBL_ENTRY->tbl_rangesize;
    if ( sizeofRange > 0)
    {
      GET_MEMORY( sizeofRange, &dmu_cb->dmu_range );
      dmu_cb->dmu_range_cnt = sizeofRange / sizeof(f8);
      MEcopy( (PTR)indexDMT_TBL_ENTRY->tbl_range, sizeofRange,
      		(PTR)dmu_cb->dmu_range);
    }

    /* now fill in the keys */

    numberOfIndexAttributes = indexDMT_IDX_ENTRY->idx_array_count;
    dmu_cb->dmu_key_array.ptr_in_count = numberOfIndexAttributes;
    dmu_cb->dmu_key_array.ptr_out_count = 0;
    dmu_cb->dmu_key_array.ptr_size = sizeof( DMU_KEY_ENTRY );

    GET_MEMORY( numberOfIndexAttributes * sizeof( DMU_KEY_ENTRY * ),
		&keyPointers );
    dmu_cb->dmu_key_array.ptr_address = ( PTR ) keyPointers;
		  
    for ( i = 0; i < numberOfIndexAttributes; i++ )
    {
        GET_MEMORY( sizeof( DMU_KEY_ENTRY ), &keyEntry );
	keyPointers[ i ] = keyEntry;

	attributeNumber = indexDMT_IDX_ENTRY->idx_attr_id[ i ];
	attributeEntry = attributePointers[ attributeNumber ];

	MEcopy( ( PTR ) &attributeEntry->att_name,
		sizeof( DB_ATT_NAME ),
		( PTR ) &keyEntry->key_attr_name );

	keyEntry->key_order = DMU_ASCENDING;

    }	/* end of loop through keys */

    /*
    ** now fill in the attributes.  these are a subset of the keys. seems
    ** a little backwards:  in the dmu_cb for an index, the key array
    ** describes all the columns in the index and the attribute array
    ** describes which columns are keys.
    */

    dmu_cb->dmu_attr_array.ptr_address  = dmu_cb->dmu_key_array.ptr_address;
    dmu_cb->dmu_attr_array.ptr_size	= sizeof(DMU_KEY_ENTRY);
    if (indexDMT_TBL_ENTRY->tbl_storage_type == DMT_RTREE_TYPE)
      dmu_cb->dmu_attr_array.ptr_in_count = 0;
    else
      dmu_cb->dmu_attr_array.ptr_in_count = indexDMT_IDX_ENTRY->idx_key_count;

    /*
    ** build the characteristics array for this index.  the following
    ** characteristics entries are never built:
    **
    **	CHARACTERISTIC			EXCUSE
    **
    **	DMU_MERGE	  	MODIFY option:  BTREE garbage collection
    **	DMU_TRUNCATE      	MODIFY option:  empties table, makes it a heap
    **	DMU_DUPLICATES    	only valid for CREATE TABLE
    **	DMU_REORG         	for specifying locations
    **	DMU_SYS_MAINTAINED	only valid for CREATE TABLE
    **	DMU_TEMP_TABLE     	flags a temporary table
    **	DMU_VIEW_CREATE    	flags a view
    **	DMU_JOURNALED      	only valid for CREATE TABLE
    **	DMU_GATEWAY	  	only for gateway
    **	DMU_GW_UPDT	  	only for gateway
    **	DMU_GW_RCVR	  	only for gateway
    **	DMU_VERIFY	  	VERIFYDB option
    **		DMU_V_VERIFY	VERIFYDB option
    **		DMU_V_REPAIR	VERIFYDB option
    **		DMU_V_PATCH	VERIFYDB option
    **		DMU_V_FPATCH	VERIFYDB option
    **		DMU_V_DEBUG	VERIFYDB option
    **		DMU_V_VERBOSE	VERIFYDB option
    **	DMU_VOPTION	  	VERIFYDB option
    **		DMU_T_BITMAP	VERIFYDB option
    **		DMU_T_LINK	VERIFYDB option
    **		DMU_T_RECORD	VERIFYDB option
    **		DMU_T_ATTRIBUTE	VERIFYDB option
    **	DMU_RECOVERY	  	TEMPORARY TABLES:  what to do on failures
    **	DMU_EXT_CREATE	  	blob support.  not an index issue
    */

    /* First pass just counts.  Second pass actually does "stuff". */

    for ( pass = 0; pass < NUMBER_OF_PASSES; pass++ )
    {
	status = initCharacteristics( qef_rcb, ulm, pass,
				      &dmu_cb->dmu_char_array );
	if ( status != E_DB_OK )	return( status );

	/* index structure */

	switch ( indexDMT_TBL_ENTRY->tbl_storage_type )
	{
	    case DMT_HEAP_TYPE:
		structureType = DB_HEAP_STORE;
		break;

	    case DMT_ISAM_TYPE:
		structureType = DB_ISAM_STORE;
		break;

	    case DMT_HASH_TYPE:
		structureType = DB_HASH_STORE;
		break;

	    case DMT_BTREE_TYPE:
		structureType = DB_BTRE_STORE;
		break;

	    case DMT_RTREE_TYPE:
		structureType = DB_RTRE_STORE;
		break;

	    default:
		status = E_DB_ERROR;
		QEF_ERROR( E_QE0062_BAD_INDEX );
		return( status );
	}
	ADD_CHARACTERISTIC( DMU_STRUCTURE, structureType );

	ADD_CHARACTERISTIC( DMU_DATAFILL, indexDMT_TBL_ENTRY->tbl_d_fill_factor);
	ADD_CHARACTERISTIC( DMU_MINPAGES, indexDMT_TBL_ENTRY->tbl_min_page );
	ADD_CHARACTERISTIC( DMU_MAXPAGES, indexDMT_TBL_ENTRY->tbl_max_page );
	ADD_CHARACTERISTIC( DMU_PAGE_SIZE, indexDMT_TBL_ENTRY->tbl_pgsize ); 

	toggle = ( indexDMT_TBL_ENTRY->tbl_status_mask & DMT_UNIQUEKEYS ) ?
		DMU_C_ON : DMU_C_OFF;
	ADD_CHARACTERISTIC( DMU_UNIQUE, toggle );
		
	ADD_CHARACTERISTIC( DMU_IFILL, indexDMT_TBL_ENTRY->tbl_i_fill_factor );

	toggle = ( indexDMT_TBL_ENTRY->tbl_status_mask & DMT_COMPRESSED ) ?
		   DMU_C_ON : DMU_C_OFF;
	ADD_CHARACTERISTIC( DMU_COMPRESSED, toggle );

	if ( indexDMT_TBL_ENTRY->tbl_storage_type == DMT_BTREE_TYPE ||
	     indexDMT_TBL_ENTRY->tbl_storage_type == DMT_RTREE_TYPE)
	{ ADD_CHARACTERISTIC( DMU_LEAFFILL, indexDMT_TBL_ENTRY->tbl_l_fill_factor );}

	ADD_CHARACTERISTIC( DMU_ALLOCATION, indexDMT_TBL_ENTRY->tbl_allocation );
	ADD_CHARACTERISTIC( DMU_EXTEND, indexDMT_TBL_ENTRY->tbl_extend );

	toggle = ( indexDMT_TBL_ENTRY->tbl_status_mask & DMT_INDEX_COMP ) ?
		   DMU_C_ON : DMU_C_OFF;
	ADD_CHARACTERISTIC( DMU_INDEX_COMP, toggle );

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_STATEMENT_LEVEL_UNIQUE )
	{ ADD_CHARACTERISTIC( DMU_STATEMENT_LEVEL_UNIQUE, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_PERSISTS_OVER_MODIFIES )
	{ ADD_CHARACTERISTIC( DMU_PERSISTS_OVER_MODIFIES, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_SYSTEM_GENERATED )
	{ ADD_CHARACTERISTIC( DMU_SYSTEM_GENERATED, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_SUPPORTS_CONSTRAINT )
	{ ADD_CHARACTERISTIC( DMU_SUPPORTS_CONSTRAINT, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_NOT_UNIQUE )
	{ ADD_CHARACTERISTIC( DMU_NOT_UNIQUE, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_NOT_DROPPABLE )
	{ ADD_CHARACTERISTIC( DMU_NOT_DROPPABLE, DMU_C_ON ); }

	if ( indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_ROW_SEC_AUDIT )
	{ ADD_CHARACTERISTIC( DMU_ROW_SEC_AUDIT, DMU_C_ON ); }

	/* No global index if base table is being unpartitioned.
	** Keep global index if it was one already;  make it a global index
	** if base table is becoming partitioned.
	*/
	if ( (indexDMT_TBL_ENTRY->tbl_2_status_mask & DMT_GLOBAL_INDEX
		|| changing_part == BECOMING_PARTITIONED)
	  && changing_part != BECOMING_NONPARTITIONED)
	{ ADD_CHARACTERISTIC( DMU_GLOBAL_INDEX, DMU_C_ON ); }

    }	/* end two pass characteristics compiler */

    return( status );
}



/*{
** Name: initCharacteristics - initialize characteristics descriptors
**
** Description:
**	This is the initialization routine for a two pass compiler that
**	builds characteristics descriptors suitable for stringing into
**	a DMU_CB.  During the first pass, all we do is count the number
**	of descriptors we'll need.  During the second pass, we allocate
**	that number of descriptors and actually fill them in.
**
** Inputs:
**	qef_rcb		request control block
**	ulm		memory stream to allocate descriptors out of
**	pass		which pass of the compiler we're executing
**	dm_data		DM_DATA describing the characteristics array
**
** Outputs:
**	dm_data		DM_DATA describing the characteristics array
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static DB_STATUS
initCharacteristics(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    i4			pass,
    DM_DATA		*dm_data
)
{
    DB_STATUS		status = E_DB_OK;

    switch ( pass )
    {
	case 0:
	    dm_data->data_address = ( char * ) NULL;
	    dm_data->data_in_size = 0;	
	    dm_data->data_out_size = 0;
	    break;

	case ( NUMBER_OF_PASSES - 1 ):

	    GET_MEMORY( dm_data->data_in_size, &dm_data->data_address );
	    break;

	default:
	    break;
    }			/* end switch */

    dm_data->data_in_size = 0;
    return( status );
}


/*{
** Name: addCharacteristic - add another characteristic to a growing list
**
** Description:
**	This routine adds another table characteristic to a growing list
**	of table properties.  This is a two pass compiler.  During the first
**	pass, all we do is count the number of characteristics.  During the
**	second pass, we actually fill in the properties.
**
**
** Inputs:
**	pass		pass 0 or 1
**	dm_data		root of the characteristics list
**	characteristic	suitable for a DMU_CHAR_ENTRY
**	value		usually DMU_C_ON or DMU_C_OFF, though depending on
**			the characteristic, this could be something more
**			interesting
**
** Outputs:
**	dm_data		the next characteristic in the list is filled in
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static VOID
addCharacteristic(
    i4			pass,
    DM_DATA		*dm_data,
    i4			characteristic,
    i4			value
)
{
    DB_STATUS		status = E_DB_OK;
    DMU_CHAR_ENTRY	*dmu_char_entryArray;
    DMU_CHAR_ENTRY	*dmu_char_entry;
    i4		index;

    switch ( pass )
    {
	case 0:
	    break;

	case ( NUMBER_OF_PASSES - 1 ):

	    index = dm_data->data_in_size / ( sizeof( DMU_CHAR_ENTRY ) );
	    dmu_char_entryArray = ( DMU_CHAR_ENTRY * ) dm_data->data_address;
	    dmu_char_entry = &dmu_char_entryArray[ index ];

	    dmu_char_entry->char_id = characteristic;
	    dmu_char_entry->char_value = value;
	    break;

	default:
	    break;
    }			/* end switch */

    dm_data->data_in_size += sizeof( DMU_CHAR_ENTRY );
}


/*{
** Name: buildDM_PTR - build a DM_PTR structure
**
** Description:
**	This routine fills in a DM_PTR structure, allocating the space needed
**	for the array of pointers to objects and the actual objects themselves.
**
** Inputs:
**	ulm		memory stream out of which to allocate objects
**	qef_rcb		request control block
**	numberOfObjects	what it is
**	objectSize	what it is
**
** Outputs:
**	dm_ptr		filled in by this routine
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static	DB_STATUS
buildDM_PTR(
    ULM_RCB		*ulm,
    QEF_RCB		*qef_rcb,
    i4		numberOfObjects,
    i4			objectSize,
    DM_PTR		*dm_ptr
)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    DB_STATUS		status = E_DB_OK;
    PTR			*objectPointers;
    i4			i;

    GET_MEMORY( numberOfObjects * sizeof( PTR ), &objectPointers );

    for ( i = 0; i < numberOfObjects; i++ )
    {
        GET_MEMORY( objectSize, &objectPointers[ i ] );
    }
    dm_ptr->ptr_address = ( PTR ) objectPointers;
    dm_ptr->ptr_in_count = numberOfObjects;
    dm_ptr->ptr_size = objectSize;

    return( status );
}



/*{
** Name: getMemory - get a chunk of memory
**
** Description:
**	This is the boiler plate for calling qec_malloc.  The size (in bytes)
**	of memory needed is passed in.  This routine allocates the memory
**	and fills in a pointer to it.
**
** Inputs:
**	qef_rcb		request control block
**	ulm		memory stream
**	pieceSize	number of bytes of memory to allocate
**
** Outputs:
**	pieceAddress	where to write the pointer to the allocated memory
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static DB_STATUS
getMemory(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    i4			pieceSize,
    PTR			*pieceAddress
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4		error;

    ulm->ulm_psize = pieceSize;
    status = qec_malloc(ulm);
    if (status != E_DB_OK)
    {
        QEF_ERROR( ulm->ulm_error.err_code );
    }
    *pieceAddress = ulm->ulm_pptr;

    return (status);
}


/*{
** Name: qeu_updateDependentIDs - overwrite stale IDs with fresh ones
**
** Description:
**	This tedious routine finds all the IIDBDEPENDS tuples for which
**	the old ID is the dependent object.  Then this routine updates
**	the stale old ID with a fabulous fresh one.  This is used to
**	update the dependencies of persistent indices, which DMF was
**	kind enough to hammer for us and which we painstakingly reconstructed.
**
** Inputs:
**	qef_rcb			request control block
**	ulm			memory stream
**	oldDependentIDs		a list of stale IDs
**	newDependentIDs		a list of fresh IDs
**	numberOfIDs		must I explain this?
**	dependentObjectType	currently a DB_INDEX, but this routine was
**				designed to be extensible to other object types
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	30-mar-93 (rickh)
**	    Zero out some more qeu_cb fields in qeu_updateDependentIDs.
**
*/

#define	QUDI_NUMBER_OF_KEYS	4

static DB_STATUS
qeu_updateDependentIDs(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    DB_TAB_ID		*oldDependentIDs,
    DB_TAB_ID		*newDependentIDs,
    i4			numberOfIDs,
    i4			dependentObjectType
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4			i;
    DB_IIDBDEPENDS	baseTuple;
    DB_IIXDBDEPENDS	indexTuple;
    DB_TAB_ID		*oldID;
    DB_TAB_ID		*newID;
    QEU_CB	        base_cb, idx_cb;
    bool	        base_opened = FALSE;
    bool	        idx_opened = FALSE;
    QEF_DATA	        index_qef_data;
    QEF_DATA	        base_qef_data;
    DMR_ATTR_ENTRY	key_array[ QUDI_NUMBER_OF_KEYS ];
    DMR_ATTR_ENTRY      *key_ptr_array[ QUDI_NUMBER_OF_KEYS ];
    i4		        secondaryDependentID = 0;

    if ( numberOfIDs <= 0 )	return( status );

    BEGIN_CODE_BLOCK	/* code block to break out of */

        /* open IIXDBDEPENDS and IIDBDEPENDS */

	idx_cb.qeu_type = base_cb.qeu_type = QEUCB_CB;
	idx_cb.qeu_length = base_cb.qeu_length = sizeof( QEU_CB );

        idx_cb.qeu_tab_id.db_tab_base  = DM_B_XDEPENDS_TAB_ID;
        idx_cb.qeu_tab_id.db_tab_index  = DM_I_XDEPENDS_TAB_ID;
        idx_cb.qeu_db_id = qef_rcb->qef_db_id;
        idx_cb.qeu_eflag = QEF_INTERNAL;
        idx_cb.qeu_flag = DMT_U_DIRECT;

        idx_cb.qeu_lk_mode = DMT_IX;
        idx_cb.qeu_access_mode = DMT_A_WRITE;
        idx_cb.qeu_mask = 0;
    	
        if ((status = qeu_open(qef_cb, &idx_cb)) != E_DB_OK)
        {   QEF_ERROR( idx_cb.error.err_code ); EXIT_CODE_BLOCK; }

	idx_opened = TRUE;

        /* Open the iidbdepends table */
        base_cb.qeu_tab_id.db_tab_base  = DM_B_DEPENDS_TAB_ID;
        base_cb.qeu_tab_id.db_tab_index  = DM_I_DEPENDS_TAB_ID;
        base_cb.qeu_db_id = qef_rcb->qef_db_id;
        base_cb.qeu_eflag = QEF_INTERNAL;
    
        base_cb.qeu_flag = DMT_U_DIRECT;
        base_cb.qeu_lk_mode = DMT_IX;
        base_cb.qeu_access_mode = DMT_A_WRITE;
        base_cb.qeu_mask = 0;
	
        if ((status = qeu_open(qef_cb, &base_cb)) != E_DB_OK)
        {   QEF_ERROR( base_cb.error.err_code ); EXIT_CODE_BLOCK; }

	base_opened = TRUE;

	/* set up for reading IIXDBDEPENDS by key */

        idx_cb.qeu_output = &index_qef_data;
        index_qef_data.dt_next = 0;
        index_qef_data.dt_size = sizeof(DB_IIXDBDEPENDS);
        index_qef_data.dt_data = (PTR) &indexTuple; 
        idx_cb.qeu_count = 1;
        idx_cb.qeu_tup_length = sizeof(DB_IIXDBDEPENDS);
	idx_cb.qeu_qual = 0;
        idx_cb.qeu_qarg = 0;

        idx_cb.qeu_klen =  QUDI_NUMBER_OF_KEYS;       
        idx_cb.qeu_key = key_ptr_array;
        for (i=0; i<  QUDI_NUMBER_OF_KEYS; i++)
	{   key_ptr_array[i] = &key_array[i]; }

        /* set up base_cb for reading/deleting iidbdepends tuple by tid */
     
        base_cb.qeu_input = base_cb.qeu_output = &base_qef_data;
        base_qef_data.dt_next = 0;
        base_qef_data.dt_size = sizeof(DB_IIDBDEPENDS);
        base_qef_data.dt_data = (PTR) &baseTuple; 
        base_cb.qeu_count = 1;
        base_cb.qeu_tup_length = sizeof(DB_IIDBDEPENDS);
        base_cb.qeu_klen = 0;
        base_cb.qeu_flag = QEU_BY_TID;
        base_cb.qeu_getnext = QEU_REPO;
	base_cb.qeu_qual = 0;
	base_cb.qeu_qarg = 0;
	base_cb.qeu_f_qual = 0;
	base_cb.qeu_f_qarg = 0;

        /*
        ** loop through the old dependent objects.  for each one, update
        ** the IIDBDEPENDS tuples for which it is a dependent object.
        */

        for ( i = 0; i < numberOfIDs; i++ )
        {
	    oldID = &oldDependentIDs[ i ];
	    newID = &newDependentIDs[ i ];

	    /*
	    ** scan IIXDBEPENDS for IIDBDEPENDS tuples with this
	    ** dependent id
	    */

            key_ptr_array[0]->attr_number = DM_1_XDEPENDS_KEY;
            key_ptr_array[0]->attr_operator = DMR_OP_EQ;
            key_ptr_array[0]->attr_value = (char *) &oldID->db_tab_base;

            key_ptr_array[1]->attr_number = DM_2_XDEPENDS_KEY;
            key_ptr_array[1]->attr_operator = DMR_OP_EQ;
            key_ptr_array[1]->attr_value = (char *) &oldID->db_tab_index;

            key_ptr_array[2]->attr_number = DM_3_XDEPENDS_KEY;
            key_ptr_array[2]->attr_operator = DMR_OP_EQ;
            key_ptr_array[2]->attr_value = (char *) &dependentObjectType;

            key_ptr_array[3]->attr_number = DM_4_XDEPENDS_KEY;
            key_ptr_array[3]->attr_operator = DMR_OP_EQ;
            key_ptr_array[3]->attr_value = (char *) &secondaryDependentID;

            idx_cb.qeu_getnext = QEU_REPO;
    
	    for ( ; ; )
	    {

	        /* get a tuple from the index table, iixdbdepends */

	        if ((status = qeu_get(qef_cb, &idx_cb)) != E_DB_OK)
	        {
	            if (idx_cb.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {   status = E_DB_OK; }
		    else
	            {   QEF_ERROR( idx_cb.error.err_code ); }

	            break;
	        }

	        idx_cb.qeu_getnext = QEU_NOREPO;    /* no repo after 1st get */
    
	        /*
		** found one.  read the whole tuple from IIDBDEPENDS by TID.
		*/

	        base_cb.qeu_tid = indexTuple.dbvx_tidp;
	        base_cb.qeu_key = (DMR_ATTR_ENTRY **) NULL;
	        base_cb.qeu_flag = QEU_BY_TID;
	        base_cb.qeu_getnext = QEU_NOREPO;
	        base_cb.qeu_klen = 0;
		base_cb.qeu_count = 1;

		status = qeu_get(qef_cb, &base_cb );
		if (status != E_DB_OK)
		{   QEF_ERROR( base_cb.error.err_code ); break; }

	        /* update the tuple (by TID) with the new ID */

	        MEcopy( ( PTR ) newID, sizeof( DB_TAB_ID ),
		        ( PTR ) &baseTuple.dbv_dep );

		status = qeu_replace(qef_cb, &base_cb );
		if (status != E_DB_OK)
		{   QEF_ERROR( base_cb.error.err_code ); break; }

	    }	/* end of loop through IIXDBDEPENDS on this dependent id */
	    if ( status != E_DB_OK )	break;

        }		/* end of loop through old dependent objects */

    END_CODE_BLOCK

    /* close IIDBDEPENDS and IIXDBDEPENDS */

    if ( base_opened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &base_cb );    }
    if ( idx_opened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &idx_cb );    }

    return( status );
}



/*{
** Name: dropDependentViews - drop the views dependent on an object
**
** Description:
**	This routine is used to drop the views depending on a persistent
**	index.  We do this because the view tree (stored in the catalogs)
**	refers to the old ID of the index.  We do not have world enough
**	or time to dissect the view tree and tweeze in a new index id.  Sorry.
**
**
** Inputs:
**	qef_rcb		request control block
**	indexIDs	stale index IDs, views on which must be dropped
**	numberOfIDs	what it is
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

#define	DDV_NUMBER_OF_KEYS	2

static	DB_STATUS
dropDependentViews(
    QEF_RCB		*qef_rcb,
    DB_TAB_ID		*indexIDs,
    i4			numberOfIDs
)
{
    DB_STATUS		status = E_DB_OK;
    i4		error;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4			i;
    DB_IIDBDEPENDS	tuple;
    DB_TAB_ID		*indexID;
    QEF_DATA	        qef_data;
    DMR_ATTR_ENTRY	key_array[ DDV_NUMBER_OF_KEYS ];
    DMR_ATTR_ENTRY      *key_ptr_array[ DDV_NUMBER_OF_KEYS ];
    QEU_CB	        dependsCB;
    bool		depends_opened = FALSE;
    QEUQ_CB		qeuq_cb;
    DB_ERROR		err_blk;
    DB_QRY_ID		viewQueryID;

    if ( numberOfIDs <= 0 )	return( status );

    /* qeuq_cb boiler plate */

    MEfill ( sizeof( QEUQ_CB ), ( char ) 0, (PTR) &qeuq_cb );

    qeuq_cb.qeuq_eflag  = QEF_INTERNAL;
    qeuq_cb.qeuq_type   = QEUQCB_CB;
    qeuq_cb.qeuq_length = sizeof(QEUQ_CB);
    qeuq_cb.qeuq_d_id   = qef_cb->qef_ses_id;
    qeuq_cb.qeuq_db_id  = qef_rcb->qef_db_id;

    BEGIN_CODE_BLOCK	/* code block to break out of */

        /* open IIDBDEPENDS */

	dependsCB.qeu_type	= QEUCB_CB;
	dependsCB.qeu_length	= sizeof(QEU_CB);
        dependsCB.qeu_tab_id.db_tab_base  = DM_B_DEPENDS_TAB_ID;
        dependsCB.qeu_tab_id.db_tab_index  = DM_I_DEPENDS_TAB_ID;
        dependsCB.qeu_db_id = qef_rcb->qef_db_id;
        dependsCB.qeu_eflag = QEF_INTERNAL;
    
        dependsCB.qeu_flag = DMT_U_DIRECT;
        dependsCB.qeu_lk_mode = DMT_IX;
        dependsCB.qeu_access_mode = DMT_A_WRITE;
        dependsCB.qeu_mask = 0;
	
        if ((status = qeu_open(qef_cb, &dependsCB)) != E_DB_OK)
        {   QEF_ERROR( dependsCB.error.err_code ); EXIT_CODE_BLOCK; }

	depends_opened = TRUE;

	/* set up for reading IIDBDEPENDS by key */

        dependsCB.qeu_output = &qef_data;
        qef_data.dt_next = 0;
        qef_data.dt_size = sizeof(DB_IIDBDEPENDS);
        qef_data.dt_data = (PTR) &tuple; 
        dependsCB.qeu_count = 1;
        dependsCB.qeu_tup_length = sizeof(DB_IIDBDEPENDS);
	dependsCB.qeu_qual = 0;
        dependsCB.qeu_qarg = 0;

        dependsCB.qeu_klen = DDV_NUMBER_OF_KEYS;
        dependsCB.qeu_key = key_ptr_array;
        for (i=0; i < DDV_NUMBER_OF_KEYS; i++)
	{   key_ptr_array[i] = &key_array[i]; }

	/*
	** loop through index IDs.  for each one drop all views on it.
	*/

        for ( i = 0; i < numberOfIDs; i++ )
        {
	    indexID = &indexIDs[ i ];

	    /*
	    ** scan IIDBDEPENDS for tuples with this
	    ** independent id
	    */

            key_ptr_array[0]->attr_number = DM_1_XDEPENDS_KEY;
            key_ptr_array[0]->attr_operator = DMR_OP_EQ;
            key_ptr_array[0]->attr_value = (char *) &indexID->db_tab_base;

            key_ptr_array[1]->attr_number = DM_2_XDEPENDS_KEY;
            key_ptr_array[1]->attr_operator = DMR_OP_EQ;
            key_ptr_array[1]->attr_value = (char *) &indexID->db_tab_index;

            dependsCB.qeu_getnext = QEU_REPO;
    
	    for ( ; ; )
	    {

	        /* get a tuple from IIDBDEPENDS */

	        if ((status = qeu_get(qef_cb, &dependsCB)) != E_DB_OK)
	        {
	            if (dependsCB.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {   status = E_DB_OK; }
		    else
	            {   QEF_ERROR( dependsCB.error.err_code ); }

	            break;
	        }

	        dependsCB.qeu_getnext = QEU_NOREPO;    /* no repo after 1st get */
    
		/* found one.  if it's a view, delete it */

		if ( tuple.dbv_dtype == DB_VIEW )
		{
		    /* get the view id */

		    qeuq_cb.qeuq_rtbl   = &tuple.dbv_dep;

		    /* lookup the view's query id */

		    status = qeu_qid( &qeuq_cb, &tuple.dbv_dep, 
			&viewQueryID, &err_blk);
		    if (status != E_DB_OK)
		    {   QEF_ERROR( err_blk.err_code ); break; }

		    STRUCT_ASSIGN_MACRO( viewQueryID, qeuq_cb.qeuq_qid );

		    /* now drop the view */

		    qeuq_cb.qeuq_flag_mask = 0;

		    status = qeu_dview( qef_cb, &qeuq_cb );
		    if (status != E_DB_OK)	{	break;	}

		}	/* endif this dependent object is a view */

	    }	/* end of loop through IIDBDEPENDS on this independent id */
	    if ( status != E_DB_OK )	break;

        }		/* end of loop through index IDs */

    END_CODE_BLOCK

    /* close IIDBDEPENDS */

    if ( depends_opened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &dependsCB );    }

    return( status );
}



/*{
** Name: getNewIndexIDs - pull out the index IDs from a list of DMU_CBs.
**
** Description:
**	This routine is passed a list of DMU_CBs which have been used
**	to create one or more indexes. This routine reads through the 
**	list of DMU_CBs pulling out the corresponding index IDs into 
**	an array of index IDs for return to the caller.
**
** Inputs:
**	qef_rcb		request control block
**	ulm		memory stream to allocate a block of new IDs out of
**	indexDMU_CBs	array of DMU_CBs to fire at DMF
**	numberOfIndices	number of DMU_CBs
**
** Outputs:
**	newIndexIDs	we allocate an array of index IDs, one for each
**			DMU_CBs, and fill in the array with the IDs that
**			DMF return to us
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	16-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	12-apr-93 (rickh)
**	    DMU_CB list is a linked list, not an array.
**	14-feb-94 (rickh)
**	    When recreating persistent indices, store the new index id, NOT
**	    the base table id (for later poking into the IIDBDPENDS tuple).
**	    This fixes bug 58850:  if a base table with a constraint was
**	    modified, then subsequent dropping of the constraint would also
**	    clobber the base table.
**	30-mar-1998 (nanpr01)
**	     Build the indexes in parallel if possible.
**	19-nov-2007 (smeke01) b119490
**	     Index building has not been done in this function since SIR 99001 
**	     in April 2004. Since then it has been done by rebuild_persistent()  
**	     in dm2u_modify(). All this function needs to do is grab the new 
**	     index-ids from the DMU_CBs used earlier by rebuild_persistent() to 
**	     create the indexes. Changed the name from createIndices to 
**	     getNewIndexIDs to reflect what it actually does.
*/

static DB_STATUS
getNewIndexIDs(
    QEF_RCB		*qef_rcb,
    ULM_RCB		*ulm,
    DMU_CB		*indexDMU_CBs,
    i4			numberOfIndices,
    DB_TAB_ID		**newIndexIDs
)
{
    DB_STATUS		status = E_DB_OK;
    i4			i;
    DMU_CB		*dmu_cb;
    DB_TAB_ID		*indexIDs;

    /* allocate an array of index IDs */
    GET_MEMORY( numberOfIndices * sizeof(DB_TAB_ID), &indexIDs );
    *newIndexIDs = indexIDs;

    dmu_cb = indexDMU_CBs;
 
    for ( i = 0, dmu_cb = indexDMU_CBs; i < numberOfIndices; i++, dmu_cb = (DMU_CB *) dmu_cb->q_next )
    {
          MEcopy( (PTR) &dmu_cb->dmu_idx_id, sizeof(DB_TAB_ID), (PTR) &indexIDs[i] );
    }

    return( status );
}


/*{
** Name: createDefaultTuples - create default tuples at table creation time
**
** Description:
**	This routine loops through the attributes in a CREATE TABLE dmu_cb.
**	When this routine returns to its caller, every attribute in the
**	DMU_CB will have a valid default ID.  Here are the cases:
**
**	o	the parser didn't fill in a default tuple for an attribute
**
**		this means that the attribute takes one of the canonical
**		defaults -- in this case, the parser has already written
**		that canonical default into the attribute's descriptor.
**		we don't have to do anything.
**
**	o	the parser did fill in a default tuple
**
**		this means the attribute takes a non-canonical default.
**		there are two sub-cases here:
**
**		*	there is already a tuple in IIDEFAULT with the same
**			string value as the user specified default
**
**			in this case, we simply retrieve the default ID
**			from that IIDEFAULT tuple and store the ID in the
**			attribute's descriptor.
**
**		*	we can't find a tuple in IIDEFAULT with the user
**			specified default string.
**
**			in this case, we allocate a new default id, write
**			it into the default tuple that the parser supplied,
**			and store the whole thing in IIDEFAULT.  the ID
**			we just allocated is then written into the attribute
**			descriptor.
**
** Inputs:
**	qef_rcb		request control block
**	dmu_cb		CREATE TABLE DMF control block.  this contains
**			the attribute descriptors.
**
** Outputs:
**	dmu_cb		we fill in a default ID for each attribute
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	19-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	11-june-2008 (dougi)
**	    Add logic to write iisequence tuple for identity columns.
**	31-march-2009 (dougi) bug 121877
**	    Add code to overcome duplicate sequence names.
**      15-Feb-2010 (hanal04) Bug 123292
**          Prevet E_QE0018 errors in qefErrorFcn() by setting
**          the caller's error code after E_US1912.
**	07-Apr-2010 (thaju02) Bug 123518
**	    Build default string only if default tuple exists.
*/

static DB_STATUS
createDefaultTuples(
    QEF_RCB		*qef_rcb,
    DMU_CB		*dmu_cb
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4			i;
    bool		IIDEFAULTopened = FALSE;
    bool		IIDEFAULTIDXopened = FALSE;
    DM_PTR		*dm_ptr = &dmu_cb->dmu_attr_array;
    DMF_ATTR_ENTRY	**attributePointers =
			( DMF_ATTR_ENTRY ** ) dm_ptr->ptr_address;
    DMF_ATTR_ENTRY	*attribute;
    i4		attributeCount = dm_ptr->ptr_in_count;
    QEU_CB		IIDEFAULTqeu_cb;
    QEU_CB		IIDEFAULTIDXqeu_cb;

    /*
    ** loop through the table attributes, looking for ones for which PSF
    ** has cooked up a default tuple.
    */

    for ( i = 0; i < attributeCount; i++ )
    {
	attribute = attributePointers[ i ];

	/* First see if this is an identity column. */
	if (attribute->attr_seqTuple != (DB_IISEQUENCE *) NULL)
	{
	    DB_IISEQUENCE	*seqp = attribute->attr_seqTuple, tempseq;
	    DMU_CB		dmucb;
	    QEU_CB		seqqeu;
	    QEF_DATA		qef_data;
	    DMR_ATTR_ENTRY	seqkey_array[2];
	    DMR_ATTR_ENTRY	*seqkey_ptr_array[2];
	    i4			error, seq_modifier;
	    char 		own[DB_OWN_MAXNAME+1], name[DB_MAXNAME+1];
	    DB_NAME		work_name;
	    bool		userseq = FALSE;
	    

	    /* First up - get the sequence identifier. */
	    dmucb.type = DMU_UTILITY_CB;
	    dmucb.length = sizeof(DMU_CB);
	    dmucb.dmu_flags_mask = 0;
	    dmucb.dmu_tran_id = qef_cb->qef_dmt_id;
	    dmucb.dmu_db_id = qef_rcb->qef_db_id;
	    status = dmf_call(DMU_GET_TABID, &dmucb);
	    if (status == E_DB_OK)
		seqp->dbs_uniqueid = dmucb.dmu_tbl_id;
	    else break;

	    /* Now check if we need to generate a sequence name. If 
	    ** first char is 0x0, must not be pre-named. */
	    if (seqp->dbs_name.db_name[0] == 0)
	    {
		/* Fabricate initial name from sequence ID. */

		STprintf((char *)&work_name, "$iiidentity_sequence_%07d", 
				seqp->dbs_uniqueid.db_tab_base);
		STmove((char *)&work_name, ' ', sizeof(DB_NAME), 
					(char *)&seqp->dbs_name);
	    }
	    else userseq = TRUE;

	    /* Copy owner name (same as for table). */
	    MEcopy((PTR)&dmu_cb->dmu_owner, sizeof(DB_OWN_NAME), 
					(PTR)&seqp->dbs_owner);

	    /* Open iisequences catalog */
	    status = qeu_tableOpen(qef_rcb, &seqqeu,
				(u_i4) DM_B_SEQ_TAB_ID,
				(u_i4) DM_I_SEQ_TAB_ID);
	    if (status != E_DB_OK)
		break;

	    /* Verify that this sequence is not already there. */
	    seqqeu.qeu_count = 1;
            seqqeu.qeu_tup_length = sizeof(DB_IISEQUENCE);
	    seqqeu.qeu_output = &qef_data;
	    qef_data.dt_next = NULL;
            qef_data.dt_size = sizeof(DB_IISEQUENCE);
            qef_data.dt_data = (PTR)&tempseq;
	    seqqeu.qeu_getnext = QEU_REPO;
	    seqqeu.qeu_klen = 2;			
	    seqqeu.qeu_key = seqkey_ptr_array;
	    seqkey_ptr_array[0] = &seqkey_array[0];
	    seqkey_ptr_array[1] = &seqkey_array[1];
	    seqkey_ptr_array[0]->attr_number = DM_1_SEQ_KEY;
	    seqkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    seqkey_ptr_array[0]->attr_value = (char *)&seqp->dbs_name;
	    seqkey_ptr_array[1]->attr_number = DM_2_SEQ_KEY;
	    seqkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    seqkey_ptr_array[1]->attr_value = (char *)&seqp->dbs_owner;
	    seqqeu.qeu_qual = NULL;
	    seqqeu.qeu_qarg = NULL;
	    status = qeu_get(qef_cb, &seqqeu);

	    if (status == E_DB_OK)		/* Found duplicate sequence */
	    {
		/* If not user spec. name, try up to 100 appendages to name
		** to make a unique one. */
		if (!userseq)
		 for (seq_modifier = 0; seq_modifier < 100; seq_modifier++)
		 {
		    STprintf((char *)&work_name, "$iiidentity_sequence_%07d_%02d", 
				seqp->dbs_uniqueid.db_tab_base, seq_modifier);
		    STmove((char *)&work_name, ' ', sizeof(DB_NAME), 
					(char *)&seqp->dbs_name);
		    status = qeu_get(qef_cb, &seqqeu);
		    if (status != E_DB_OK)
			break;
		 }
		
		if (status == E_DB_OK)
		{
		    (VOID)qef_error(E_US1912_6418_SEQ_EXISTS, 0L, E_DB_ERROR,
			    &error, &seqqeu.error, 1, 
			    qec_trimwhite(sizeof(seqp->dbs_name),
					  (PTR)&seqp->dbs_name),
					  (PTR)&seqp->dbs_name);
                    SETDBERR(&qef_rcb->error, 0, E_QE0025_USER_ERROR);
		    status = E_DB_ERROR;
		    break;
		}
	    }
		
	    /* Insert new sequence tuple. */
	    seqqeu.qeu_count = 1;
	    seqqeu.qeu_tup_length = sizeof(DB_IISEQUENCE);
	    seqqeu.qeu_input = &qef_data;
	    qef_data.dt_data = (PTR)seqp;
	    status = qeu_append(qef_cb, &seqqeu);
	    if (status != E_DB_OK)
		break;

	    status = qeu_close(qef_cb, &seqqeu);    
	    if (status != E_DB_OK)
		break;

	    /* Now that the sequence is done, let's build the default. */

	    /* Generate space-trimmed copies of seqowner and seqname,
	    ** and then build the default string.  Might as well do it
	    ** directly into the default tuple value area. */

	    if ( attribute->attr_defaultTuple != ( DB_IIDEFAULT * ) NULL )
	    {
		STlcopy((char *) &seqp->dbs_owner, &own[0], sizeof(own)-1);
		STlcopy((char *) &seqp->dbs_name, &name[0], sizeof(name)-1);
		STtrmwhite(&own[0]);
		STtrmwhite(&name[0]); 
		STlpolycat(5, sizeof(attribute->attr_defaultTuple->dbd_def_value)-1,
		    "next value for \"", own, "\".\"", name, "\"",
		    &attribute->attr_defaultTuple->dbd_def_value[0]);
		attribute->attr_defaultTuple->dbd_def_length = 
			STlength(attribute->attr_defaultTuple->dbd_def_value);

	    }
	}

	/* if PSF has cooked up a default tuple */

	if ( attribute->attr_defaultTuple != ( DB_IIDEFAULT * ) NULL )
	{
	    /* if we haven't opened IIDEFAULT yet, do so */

	    if ( IIDEFAULTopened == FALSE )
	    {
		status = qeu_tableOpen( qef_rcb, &IIDEFAULTqeu_cb,
				    ( u_i4 ) DM_B_IIDEFAULT_TAB_ID,
				    ( u_i4 ) DM_I_IIDEFAULT_TAB_ID );
 		if ( status != E_DB_OK )	break;

		IIDEFAULTopened = TRUE;
	    }

	    /* if we haven't opened IIDEFAULTIDX yet, do so */

	    if ( IIDEFAULTIDXopened == FALSE )
	    {
		status = qeu_tableOpen( qef_rcb, &IIDEFAULTIDXqeu_cb,
				    ( u_i4 ) DM_B_IIDEFAULTIDX_TAB_ID,
				    ( u_i4 ) DM_I_IIDEFAULTIDX_TAB_ID );
 		if ( status != E_DB_OK )	break;

		IIDEFAULTIDXopened = TRUE;
	    }

	    status = qeu_findOrMakeDefaultID( qef_rcb,
					  &IIDEFAULTqeu_cb,
					  &IIDEFAULTIDXqeu_cb,
					  attribute->attr_defaultTuple,
					  &attribute->attr_defaultID );
	    if ( status != E_DB_OK )	break;

	}	/* endif PSF filled in a default tuple for this attribute */

    }	/* endloop through the attributes */

    if ( IIDEFAULTopened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &IIDEFAULTqeu_cb );    }
    if ( IIDEFAULTIDXopened == TRUE )
    {	( VOID ) qeu_close( qef_cb, &IIDEFAULTIDXqeu_cb );    }

    return( status );
}


/*{
** Name: qeu_tableOpen - open a table
**
** Description:
**	This routine opens a table.
**
**
** Inputs:
**	qef_rcb		request control block
**	qeu_cb		QEU control block describing the table
**	baseID		table base ID
**	indexID		table index ID
**
** Outputs:
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	19-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

DB_STATUS
qeu_tableOpen(
    QEF_RCB		*qef_rcb,
    QEU_CB		*qeu_cb,
    u_i4		baseID,
    u_i4		indexID
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4		error;

    qeu_cb->qeu_type = QEUCB_CB;
    qeu_cb->qeu_length = sizeof( QEU_CB );

    qeu_cb->qeu_tab_id.db_tab_base = baseID;
    qeu_cb->qeu_tab_id.db_tab_index = indexID;
    qeu_cb->qeu_db_id = qef_rcb->qef_db_id;
    qeu_cb->qeu_eflag = QEF_INTERNAL;
    qeu_cb->qeu_flag = DMT_U_DIRECT;

    qeu_cb->qeu_lk_mode = DMT_IX;
    qeu_cb->qeu_access_mode = DMT_A_WRITE;
    qeu_cb->qeu_mask = 0;
    	
    if ((status = qeu_open(qef_cb, qeu_cb)) != E_DB_OK)
    {   QEF_ERROR( qeu_cb->error.err_code ); return( status ); }

    return( status );
}


/*{
** Name: qeu_findOrMakeDefaultID - lookup or create a default id
**
** Description:
**	This routine is called by createDefaultTuples and does the hard
**	work for its caller.
**
**	This routine returns the default ID corresonding to a default
**	string cooked up by the parser.
**
**	This routine is passed an IIDEFAULT tuple cooked up by the parser.
**	If the default string in that default already lives in an IIDEFAULT
**	tuple on disk, this routine will simply return the ID of the on disk
**	tuple.
**
**	If, on the other hand, the string in the parser's tuple doesn't yet
**	exist on disk, this routine will allocate an ID for the string
**	and write the parser's tuple (together with that ID) to disk.  Then
**	the allocated ID will be returned.
**
**
** Inputs:
**	qef_rcb	       		request control block
**	IIDEFAULTqeu_cb		control block for reading/writing IIDEFAULT
**	IIDEFAULTIDXqeu_cb	control block for reading IIDEFAULTIDX
**	parserTuple		the tuple that the parser cooked up
**
** Outputs:
**	returnedDefaultID	filled in with a valid default ID on return
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** History:
**	19-feb-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	07-jul-93 (anitap)
**	    Added arguments for qea_reserveID().
**	19-aug-93 (anitap)
**	    Initialized qef_cb->qef_dbid.
*/

#define	CDT_NUMBER_OF_KEYS	1

DB_STATUS
qeu_findOrMakeDefaultID(
    QEF_RCB		*qef_rcb,
    QEU_CB		*IIDEFAULTqeu_cb,
    QEU_CB		*IIDEFAULTIDXqeu_cb,
    DB_IIDEFAULT	*parserTuple,
    DB_DEF_ID		*returnedDefaultID
)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb; /* session control block */
    i4		error;
    QEF_DATA		IIDEFAULTqef_data;
    QEF_DATA		IIDEFAULTIDXqef_data;
    DB_IIDEFAULT	IIDEFAULTtuple;
    DB_IIDEFAULTIDX	IIDEFAULTIDXtuple;
    DMR_ATTR_ENTRY	key_array[ CDT_NUMBER_OF_KEYS ];
    DMR_ATTR_ENTRY      *key_ptr_array[ CDT_NUMBER_OF_KEYS ];
    DB_DEF_ID		*defaultIDptr;
    i4			i;

    /* set up for reading IIDEFAULTIDX by key */

    IIDEFAULTIDXqeu_cb->qeu_output = &IIDEFAULTIDXqef_data;
    IIDEFAULTIDXqef_data.dt_next = 0;
    IIDEFAULTIDXqef_data.dt_size = sizeof(DB_IIDEFAULTIDX);
    IIDEFAULTIDXqef_data.dt_data = (PTR) &IIDEFAULTIDXtuple; 
    IIDEFAULTIDXqeu_cb->qeu_count = 1;
    IIDEFAULTIDXqeu_cb->qeu_tup_length = sizeof(DB_IIDEFAULTIDX);
    IIDEFAULTIDXqeu_cb->qeu_qual = 0;
    IIDEFAULTIDXqeu_cb->qeu_qarg = 0;

    IIDEFAULTIDXqeu_cb->qeu_klen =  CDT_NUMBER_OF_KEYS;       
    IIDEFAULTIDXqeu_cb->qeu_key = key_ptr_array;
    for (i=0; i<  CDT_NUMBER_OF_KEYS; i++)
    {   key_ptr_array[i] = &key_array[i]; }

    key_ptr_array[0]->attr_number = DM_1_IIDEFAULTIDX_KEY;
    key_ptr_array[0]->attr_operator = DMR_OP_EQ;
    key_ptr_array[0]->attr_value = (char *) &parserTuple->dbd_def_length;

    IIDEFAULTIDXqeu_cb->qeu_getnext = QEU_REPO;
    
    /* set up reading/appending iidefault tuple by tid */
     
    IIDEFAULTqeu_cb->qeu_input = IIDEFAULTqeu_cb->qeu_output =
	&IIDEFAULTqef_data;
    IIDEFAULTqef_data.dt_next = 0;
    IIDEFAULTqef_data.dt_size = sizeof(DB_IIDEFAULT);
    IIDEFAULTqeu_cb->qeu_count = 1;
    IIDEFAULTqeu_cb->qeu_tup_length = sizeof(DB_IIDEFAULT);
    IIDEFAULTqeu_cb->qeu_klen = 0;
    IIDEFAULTqeu_cb->qeu_flag = QEU_BY_TID;
    IIDEFAULTqeu_cb->qeu_getnext = QEU_REPO;
    IIDEFAULTqeu_cb->qeu_qual = 0;
    IIDEFAULTqeu_cb->qeu_qarg = 0;

    /* get a tuple from the index table, IIDEFAULTIDX */

    if ( ( status = qeu_get(qef_cb, IIDEFAULTIDXqeu_cb ) ) != E_DB_OK )
    {
	if (IIDEFAULTIDXqeu_cb->error.err_code != E_QE0015_NO_MORE_ROWS)
	{   QEF_ERROR( IIDEFAULTIDXqeu_cb->error.err_code ); return( status );}
	else
	{
	    status = E_DB_OK;

	    /* there is no default tuple for this string.  create one */

	    /* first, allocate a default id */

	    defaultIDptr = &parserTuple->dbd_def_id;
	    qef_cb->qef_dbid = qef_rcb->qef_db_id;
            status = qea_reserveID( ( DB_TAB_ID * ) defaultIDptr, qef_cb,
				&qef_rcb->error);
            if ( status != E_DB_OK )	return( status );

	    /* create a default tuple with this id and string */

	    IIDEFAULTqef_data.dt_data = (PTR) parserTuple;
	    status = qeu_append( qef_cb, IIDEFAULTqeu_cb );
	    if (DB_FAILURE_MACRO( status ) )
	    {   QEF_ERROR( IIDEFAULTqeu_cb->error.err_code); return( status ); }
	}
    }
    else	/* this string does have a default tuple associated with it */
    {
	/* read the base tuple from IIDEFAULT */

        IIDEFAULTqeu_cb->qeu_tid = IIDEFAULTIDXtuple.dbdi_tidp;
        IIDEFAULTqeu_cb->qeu_key = (DMR_ATTR_ENTRY **) NULL;
        IIDEFAULTqeu_cb->qeu_flag = QEU_BY_TID;
        IIDEFAULTqeu_cb->qeu_getnext = QEU_NOREPO;
        IIDEFAULTqeu_cb->qeu_klen = 0;
        IIDEFAULTqeu_cb->qeu_count = 1;

	IIDEFAULTqef_data.dt_data = (PTR) &IIDEFAULTtuple; 

        status = qeu_get(qef_cb, IIDEFAULTqeu_cb );
        if (status != E_DB_OK)
        {   QEF_ERROR( IIDEFAULTqeu_cb->error.err_code ); return( status ); }

	defaultIDptr = &IIDEFAULTtuple.dbd_def_id;

    }	/* endif this default tuple has a string associated with it */

    /* return the default id for this string */

    MEcopy( ( PTR ) defaultIDptr, sizeof( DB_DEF_ID ),
	    ( PTR ) returnedDefaultID );

    return( status );
}

/*{
** Name: suppress_rowcount - determine whether to print the rowcount diagnostic
**
** Description:
**	This routine determines whether this DBU action should print
**	out a rowcount.  For system generated INDEX creations, we don't
**	print a rowcount.  This routine cycles through the characteristics
**	array of the DBU action, looking for the SYSTEM_GENERATED flag.
**
**
**
** Inputs:
**	opcode			DBU operation
**	dmu			DMU_CB of the DBU action
**
** Outputs:
**	Returns:
**
**	TRUE			if rowcount should be suppressed
**	FALSE			otherwise
**
**
** History:
**	22-mar-93 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static	bool
suppress_rowcount(
    i4	opcode,
    DMU_CB	*dmu
)
{
    bool		result = FALSE;
    DMU_CHAR_ENTRY	*chr;
    i4		chr_count;
    i4		i;

    /* report rowcount for operations other than CREATE INDEX */

    if ( opcode != DMU_INDEX_TABLE )	{ return( result ); }

    chr = (DMU_CHAR_ENTRY*) dmu->dmu_char_array.data_address;
    chr_count = dmu->dmu_char_array.data_in_size / sizeof(DMU_CHAR_ENTRY);
    if ( ( chr_count <= 0 ) || ( chr == ( DMU_CHAR_ENTRY * ) NULL ) )
    { return( result ); }

    for ( i = 0; i < chr_count; i++ )
    {
	if ( chr[ i ].char_id == DMU_SYSTEM_GENERATED )
	{
	    if ( chr[ i ].char_value == DMU_C_ON )
	    { result = TRUE; }
	    else
	    { result = FALSE; }
	}
    }

    return( result );
}

