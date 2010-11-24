/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <bt.h>
#include    <ci.h>
#include    <er.h>
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <cui.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>
#include    <usererror.h>
#include    <uld.h>

/*
** Name: PSLMDFY.C:	this file contains functions used by both SQL and/or
**			QUEL grammar in parsing of MODIFY statement
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_MD<number>{[S|Q]}_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
**	if the function will be called only by the SQL grammar, <number> must be
**	followed by 'S';
**	
**	if the function will be called only by the QUEL grammar, <number> must
**	be followed by 'Q';
**
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of MODIFY statement.  Unless noted otherwise, functions are used
**	by both QUEL and SQL grammars.
**
**	psl_md1_modify()	    - semantic action for MODIFY production
**	psl_md2_modstmnt()	    - semantic action for MODSTMNT production
**	psl_md3_modstorage()	    - semantic action for MODSTORAGE production
**	psl_md4_modstorname()	    - semantic action for MODSTORNAME production
**	psl_md5_modkeys()	    - semantic action for MODKEYS production
**	psl_md6_modbasekey()	    - semantic action for MODBASEKEY production
**	psl_md7_modkeyname()	    - semantic action for MODKEYNAME production
**	psl_md8_modtable()	    - semantic action for MODTABLE production
**
** History:
**	11-feb-91 (andre)
**	    created
**	18-nov-1991 (andre)
**	    merged 5-feb-1991 (bryanp) change:
**		Added support for Btree Index Compression.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements.
**	25-sep-1992 (pholman)
**	    For C2, replace obsolete qeu_audit calls with their SXF
**	    equivalent.  Add <sxf.h> and <tm.h> inclusion.
**	16-nov-1992 (pholman)
**	    C2: upgrade sxf_call to be called via psy_secaudit wrapper
**	30-mar-1993 (rmuth)
**	    Add support for "to add_extend".
**	06-apr-1993 (rblumer)
**	    added [no]persistence with-clause option to psl_md9_modopt_word;
**	    changed CMcmpcase to CMcmpnocase, since case doesn't matter;
**	    added with_clauses parameter to psl_md3_modstorage.
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	15-may-1993 (rmuth)
**	    Add support for [NO]READONLY tables.
**      12-Jul-1993 (fred)
**          Disallow direct manipulation of extension tables.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rmuth)
**	    Various
**	     - Modify to truncated|merge|add_extend|readonly were not setting
**	       DMU_TEMP_TABLE characteristic for user temp tables.
**	     - Don't allow a WITH subclause on [NO]READONLY.
**	     - Include <usererror.h>
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
**	03-sep-93 (rblumer)
**	    in psl_md1_modify, set up info to validate unique_scope and
**	    persistence for indexes that support unique constraints
**	16-Sep-1993 (fred)
**	    Set up defaults so that on table relocations, the 
**	    extension tables are relocated as well.  This is the 
**	    default.
**	21-oct-93 (stephenb)
**	    Fixed call to psy_secaudit() to audit table modify message not
**	    table access message.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	06-jan-95 (forky01)
**	    Added support for partial backup and recovery syntax on MODIFY
**	    which includes:
**	    MODIFY tablename TO alter_status
**
**	    alter_status:   PHYS_CONSISTENT
**	                  | PHYS_INCONSISTENT
**	                  | LOG_CONSISTENT
**	                  | LOG_INCONSISTENT
**	                  | TABLE_RECOVERY_ALLOWED
**	                  | TABLE_RECOVERY_DISALLOWED
**	    No with_clause will be allowed.
**	03-jan-96 (kch)
**	    SIR 72811 - remove restriction on direct manipulation of
**	    extension tables.
**	08-feb-95 (cohmi01)
**	    For RAW/IO, allocate storage for extent names in dmu_cb.
**	31-Mar-1997 (jenjo02)
**	    Table priority project:
**	    PRIORITY = <n> clause may occur in either 
**		MODIFY ... TO PRIORITY = <n> (in place of structure) or in 
**		MODIFY ... TO ... WITH PRIORITY = <n>
**	    Distinguish here between the two for dm2umod. In the first case,
**	    the table isn't physically modified - just the priority is changed.
**	    In the second case, the priority is changed as well as its physical
**	    structure.
**      21-mar-1999 (nanpr01)
**          Support raw locations.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	01-Mar-2001 (jenjo02)
**	    Remove references to obsolete DB_RAWEXT_NAME.
**      04-oct-2002 (stial01)
**          Move btree key length validation to end of modify processing,
**          always report table name and composite key length. (b108876)
**      02-jan-2004 (stial01)
**          Changes to expand number of WITH CLAUSE options
**	23-Feb-2004 (schka24)
**	    Partitioned table support in MODIFY.
**      14-apr-2004 (stial01)
**          Maximum btree key size varies by pagetype, pagesize (SIR 108315)
**	5-Aug-2004 (schka24)
**	    Add a check that I forgot before: modify to unique on a partitioned
**	    table is only allowed when the partitioning scheme and the
**	    proposed storage structure are compatible.
**      20-oct-2006 (stial01)
**          Disable CLUSTERED index build changes for ingres2006r2
**	26-sep-2007 (kibro01) b119080
**	    Allow modify to nopersistence
**      21-Nov-2008 (hanal04) Bug 121248
**          If we are invalidating a table referenced in a range variable
**          we should pass the existing pss_rdrinfo reference if there is one.
**          Furthermore, we need RDF to clear down our reference to the
**          rdf_info_blk if it is freed.
**	28-May-2009 (kschendel) b122118
**	    Catch bogus index modifies (eg truncated) sooner.
**	    Table drive single-keyword with-clause actions.
**	15-feb-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**	09-Apr-2010 (frima01) SIR 122739
**	    In psl_md3_modstorage change DMU char to the with-option
**	    form of unique_scope to facilitate distinguishing catalog
**	    only modifies from table structure changes.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**       6-Oct-2010 (hanal04) Bug 124553
**          Modify operations that do not actually modify the table should
**          not be blocked because of the existence of dependencies that
**          would be broken by an actual modify. Turn off dependency checking
**          for no-op modify operations such as table_debug.
**	08-Oct-2010 (miket) SIR 122403 BUG 124543 SD 146855
**	    MODIFY <table> ENCRYPT is also a MODIFY that does not modify the
**	    table structure, so set PSL_MDF_NO_OP.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	11-Oct-2010 (kschendel) SIR 124544
**	    Changes for consolidated WITH-clause parsing.
**	    Put MODIFY action in dmu_action.
*/

/* Module local definitions and storage. */

/* Forward procedure declarations */

static DB_STATUS psl_md_part_unique(
	PSS_SESBLK *sess_cb,
	DMU_CB *dmucb,
	PSS_YYVARS *yyvarsp,
	DB_ERROR *err_blk);


/* Since there are so many MODIFY action variants, let's table
** drive this thing.
** Some of the actions need a wee bit of extra checking or work beyond
** the norm;  define the flags needed for the special cases.
*/

#define PSL_MDF_HEAPSORT	0x0001		/* Heapsort option */
#define PSL_MDF_COMPRESSED	0x0002		/* Compressed e.g. cbtree */
#define PSL_MDF_RECONSTRUCT	0x0004		/* Reconstruct is handled out-of-line */
#define PSL_MDF_MERGE		0x0008		/* MERGE, needs btree */
#define PSL_MDF_ERROR		0x0010		/* Obsolete keyword, error */

/* Some actions are restricted against partitioned tables.
** In some cases the action must be against the entire table;
** in others the action must be against one physical partition.
** Just to make the table easier to read, use a "partial OK" flag instead of
** "master only", since there are more of the latter.
*/
#define PSL_MDF_LPART_OK	0x0020		/* Partial table is OK */
#define PSL_MDF_PPART_ONLY	0x0040		/* One partition only */

/* For clustered btree */
#define PSL_MDF_CLUSTERED	0x0080		/* Clustered Btree */

#define PSL_MDF_NOINDEX         0x0100          /* Don't allow on 2ary index */

/* Some operations are unnecessarily blocked because an actual modify would
** break constraints defined on the table. If the operation is guaranteed
** NOT to break any dependencies it can be defined as a no-op in which
** dependency checking can be safely skipped. Use with CAUTION.
*/
#define PSL_MDF_NO_OP		0x0200		/* Table is NOT modified */

/* Here's the action name table itself.
** Every action has an action code that is put into the DMU CB.
** Most if not all actions also set a specific DMU char indicator
** and DMU_CHARACTERISTICS value:
**    if the indicator is DMU_STRUCTURE, the value is a structure type;
**    if the indicator is DMU_ACTION_ONOFF or DMU_PERSISTS_OVER_MODIFIES,
**    the value is the flag to store into dmu_flags if ON, and zero if OFF;
**    if the indicator is DMU_VACTION, the value goes into dmu_vaction;
**    if the indicator is -1, don't set any indicator or value.
** The special-flags are defined above.
** The table is checked sequentially, so the more common options are at
** the top.  (Not that it's a big deal.)  A null entry ends the list.
**
** You'll notice that rtree isn't on the list.  At present there is
** no DMF modify support for modifying RTREE's.  You have to drop and
** recreate to get the effect.
*/

static const struct _MODIFY_STORNAMES
{
    char	*name_string;	/* The modify action name */
    enum dmu_action_enum action; /* The dmu-action to store in the DMU CB */
    i4		indicator;	/* dmu-chars indicator or -1 */
    i4		value;		/* DMU characteristics value */
    i2		special_flags;	/* PSL_MDF_xxx special flags */
} modify_stornames[] = {
	{"btree",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_BTRE_STORE, 0
	},
	{"heap",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HEAP_STORE, PSL_MDF_NOINDEX
	},
	{"hash",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HASH_STORE, 0
	},
	{"isam",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_ISAM_STORE, 0
	},
/* --> comment out structure "clustered"
	{"clustered",
	 DMU_STRUCTURE, DB_BTRE_STORE, PSL_MDF_CLUSTERED | PSL_MDF_NOINDEX
	},
*/
	{"heapsort",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HEAP_STORE, PSL_MDF_HEAPSORT | PSL_MDF_NOINDEX
	},
	{"cbtree",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_BTRE_STORE, PSL_MDF_COMPRESSED
	},
	{"cheap",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HEAP_STORE, PSL_MDF_COMPRESSED | PSL_MDF_NOINDEX
	},
	{"chash",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HASH_STORE, PSL_MDF_COMPRESSED
	},
	{"cisam",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_ISAM_STORE, PSL_MDF_COMPRESSED
	},
	{"cheapsort",
	 DMU_ACT_STORAGE,
	 DMU_STRUCTURE, DB_HEAP_STORE,
			PSL_MDF_HEAPSORT | PSL_MDF_COMPRESSED | PSL_MDF_NOINDEX
	},
	{"reconstruct",
	 DMU_ACT_STORAGE,
	 -1, 0, PSL_MDF_RECONSTRUCT | PSL_MDF_LPART_OK
	},
	{"merge",
	 DMU_ACT_MERGE,
	 -1, 0, PSL_MDF_MERGE | PSL_MDF_LPART_OK
	},
	/* FIXME **** note: modify foo partition bar to truncated
	** needs to be allowed, but we need some kind of semantics that
	** says "preserve storage structure".  We can't allow a partition
	** to be heapified when the rest of the table is (say) btree.
	** add back lpart-ok when this is fixed somehow.
	*/
	{"truncated",
	 DMU_ACT_TRUNC,
	 -1, 0, PSL_MDF_NOINDEX
	},
	{"reorganize",
	 DMU_ACT_REORG,
	 -1, 0, PSL_MDF_LPART_OK
	},
	{"reorganise",		/* Across-the-water spelling */
	 DMU_ACT_REORG,
	 -1, 0, PSL_MDF_LPART_OK
	},
	{"add_extend",
	 DMU_ACT_ADDEXTEND,
	 -1, 0, PSL_MDF_LPART_OK
	},
	{"readonly",
	 DMU_ACT_READONLY,
	 DMU_ACTION_ONOFF, DMU_FLAG_ACTON, 0
	},
	{"noreadonly",
	 DMU_ACT_READONLY,
	 DMU_ACTION_ONOFF, 0, 0
	},
	{"phys_consistent",
	 DMU_ACT_PHYS_CONSISTENT,
	 DMU_ACTION_ONOFF, DMU_FLAG_ACTON, 0
	},
	{"phys_inconsistent",
	 DMU_ACT_PHYS_CONSISTENT,
	 DMU_ACTION_ONOFF, 0, 0
	},
	{"log_consistent",
	 DMU_ACT_LOG_CONSISTENT,
	 DMU_ACTION_ONOFF, DMU_FLAG_ACTON, 0
	},
	{"log_inconsistent",
	 DMU_ACT_LOG_CONSISTENT,
	 DMU_ACTION_ONOFF, 0, 0
	},
	{"table_recovery_allowed",
	 DMU_ACT_TABLE_RECOVERY,
	 DMU_ACTION_ONOFF, DMU_FLAG_ACTON, 0
	},
	{"table_recovery_disallowed",
	 DMU_ACT_TABLE_RECOVERY,
	 DMU_ACTION_ONOFF, 0, 0
	},
	{"persistence",
	 DMU_ACT_PERSISTENCE,
	 DMU_PERSISTS_OVER_MODIFIES, DMU_FLAG_PERSISTENCE, 0
	},
	{"nopersistence",
	 DMU_ACT_PERSISTENCE,
	 DMU_PERSISTS_OVER_MODIFIES, 0, 0
	},
	{"encrypt",
	 DMU_ACT_ENCRYPT,
	 -1, 0, PSL_MDF_NOINDEX | PSL_MDF_NO_OP
	},
	{"table_verify",
	 DMU_ACT_VERIFY,
	 DMU_VACTION, DMU_V_VERIFY, PSL_MDF_PPART_ONLY | PSL_MDF_NO_OP
	},
	{"table_repair",
	 DMU_ACT_VERIFY,
	 DMU_VACTION, DMU_V_REPAIR, PSL_MDF_PPART_ONLY | PSL_MDF_NO_OP
	},
	{"table_patch",
	 DMU_ACT_VERIFY,
	 DMU_VACTION, DMU_V_PATCH, PSL_MDF_PPART_ONLY | PSL_MDF_NO_OP
	},
	{"table_debug",
	 DMU_ACT_VERIFY,
	 DMU_VACTION, DMU_V_DEBUG, PSL_MDF_PPART_ONLY | PSL_MDF_NO_OP
	},
	{"checklink",
	 0, 0, 0, PSL_MDF_ERROR
	},
	{"patchlink",
	 0, 0, 0, PSL_MDF_ERROR
	},

	{NULL, 0, 0, 0}
};


/*{
** Name: psl_md1_modify - semantic action for MODIFY production
**
** Description:
**  This is a semantic action for MODIFY production used by both SQL and
**  QUEL grammars.
**
**	Wrap up MODIFY parsing, make any final checks, and finalize
**	the DMU_CB that was created.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	yyvarsp		    Parser state variables
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	N/A
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	18-nov-1991 (andre)
**	    merged 5-feb-1991 (bryanp) change:
**		Added support for Btree Index Compression.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements: if modifying a temporary table,
**	    set DMU_TEMP_TABLE characteristic in the DMU_CB and allow a table
**	    owned by the current session to be modified.
**	03-aug-92 (barbara)
**	    Invalidate the underlying table's infoblk from the RDF cache.
**	06-oct-92 (bonobo)
**	    Removed stray ^T which found its way into the integration.
**	07-dec-92 (andre)
**	    we will return() at the end of the processing code corresponding to
**	    MODIFY TO <structure> only if psl_validate_options() returned bad
**	    status.  This will ensure that we reach code that invalidates
**	    the RDF cache entry if everything else looks good
**	26-jul-1993 (rmuth)
**	    Various.
**	      - Modify to truncated|merge|add_extend|readonly were not setting
**	        DMU_TEMP_TABLE characteristic for user temp tables.
**	      - [NO]READONLY options are not allowed any with clauses so 
**	        check here.
**	10-aug-93 (andre)
**	    fixed cause of compiler warning
**	03-sep-93 (rblumer)
**	    set up info to validate unique_scope and persistence
**	    for indexes that support unique constraints
**	21-oct-93 (stephenb)
**	    Fixed call to psy_secaudit() to audit table modify message not
**	    table access message.
**	06-jan-1994 (forky01)
**	    Add support for partial recovery syntax. Use storname to
**          pass status change of PHYS_[IN]CONSISTENT | LOG_[IN]CONSISTENT |
**	    TABLE_RECOVERY_[DIS]ALLOWED.  Add new entry in DMU_CHAR_ENTRY
**	    for DMU_INCONSISTENT, DMU_LOG_INCONSISTENT, 
**	    DMU_TABLE_RECOVERY_DISALLOWED. No with clause allowed.
**	31-Mar-1997 (jenjo02)
**	    Table priority project:
**	    PRIORITY = <n> clause may occur in either 
**		MODIFY ... TO PRIORITY = <n> (in place of structure) or in 
**		MODIFY ... TO ... WITH PRIORITY = <n>
**	    Distinguish here between the two for dm2umod. In the first case,
**	    the table isn't physically modified - just the priority is changed.
**	    In the second case, the priority is changed as well as its physical
**	    structure.
**	24-Feb-2004 (schka24)
**	    Teach psl-nm-eq-no how to do the above, as part of preventing
**	    nonsense modifies like modify foo to fillfactor=100.
**	    Finish off modify to reconstruct data.
**	5-Aug-2004 (schka24)
**	    Check for uniqueness / partitioning compatibility.
**	10-Mar-2005 (schka24)
**	    Don't allow indexes to be partitioned via MODIFY.
**	23-Nov-2005 (schka24)
**	    Validate allocation= against number of partitions.
**	07-May-2006 (jenjo02)
**	    Indexes can't be modified to CLUSTERED storage structure.
**	20-Nov-2008 (kibro01) b121249
**	    Modify of temporary tables to partitioned is not yet implemented.
**      17-Dec-2009 (hanal04) Bug 123070
**          Allocation values should not be divided by nparts when checking
**          for validity.
**	13-Oct-2010 (kschendel) SIR 124544
**	    Generate/finalize DMU_CHARACTERISTICS, not dmu char array.
*/
DB_STATUS
psl_md1_modify(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk)
{
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    RDF_CB		 rdf_cb;
    i4		err_code;
    DB_STATUS		status = E_DB_OK;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    if (qeu_cb->qeu_d_op == DMU_RELOCATE_TABLE)
    {
	/*
	** In RELOCATE variant of MODIFY we have to check whether both
	** lists exist and whether they are of equal length.
	*/
	if (!dmu_cb->dmu_location.data_in_size	    ||
	    !dmu_cb->dmu_olocation.data_in_size	    ||
	    (dmu_cb->dmu_location.data_in_size !=
	     dmu_cb->dmu_olocation.data_in_size)
	   )
	{
	    _VOID_ psf_error(5542L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return(E_DB_ERROR);
	}
    }
    else if (dmu_cb->dmu_action == DMU_ACT_STORAGE)   /* qeu_d_op == DMU_MODIFY_TABLE */
    {
	/*
	** check for legal characteristics if storage structure was defined
	*/

	/* If user is modifying to a UNIQUE storage structure, check for
	** compatibility with any partitioning.
	*/
	if (BTtest(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators))
	    if (psl_md_part_unique(sess_cb, dmu_cb, yyvarsp, err_blk) != E_DB_OK)
		return (E_DB_ERROR);

	if (sess_cb->pss_resrng->pss_tabdesc->tbl_temporary)
	    BTset(DMU_TEMP_TABLE,dmu_cb->dmu_chars.dmu_indicators);

    } /* if (structure was defined) */
    else if (dmu_cb->dmu_action == DMU_ACT_REORG)
    {
	/* MODIFY TO REORGANIZE was specified */
	if (!dmu_cb->dmu_location.data_in_size)
	{
	    /* Locations clause required for REORGANIZE variant. */
	    _VOID_ psf_error(5548L, 0L, PSF_USERERR, &err_code,
		err_blk, 0);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	/* MODIFY TO MERGE/TRUNCATED/ADD_EXTEND/READONLY/PHYS_CONSISTENT/
	**     LOG_CONSISTENT/TABLE_RECOVERY_ALLOWED  was specified */

	/*
	** If a temporary table then set a char array accordingly
	*/
	if (sess_cb->pss_resrng->pss_tabdesc->tbl_temporary)
	    BTset(DMU_TEMP_TABLE,dmu_cb->dmu_chars.dmu_indicators);
    }

    /*
    ** Invalidate base table information from RDF cache.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_resrng->pss_tabid,
			rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_info_blk = sess_cb->pss_resrng->pss_rdrinfo;
    rdf_cb.caller_ref = &sess_cb->pss_resrng->pss_rdrinfo;
    
    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error, err_blk);

    return(status);
}

/*{
** Name: psl_md2_modstmnt - semantic action for MODSTMNT production
**
** Description:
**  This is a semantic action for MODSTMNT production used by both SQL
**  and QUEL grammars (modstmnt:    MODIFY).
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	psq_cb		    PSF request CB
**	yyvarsp		    Parser state variables
**
** Outputs:
**	sess_cb
**	    pss_distrib     DB_3_DDB_SESS if distributed thread
**	    pss_ostream	    new memory stream opened
**	    pss_object	    will point to the newly allocated and initialized
**			    QEU_CB
**	    pss_rsdmno	    set to 0 to indicate that no keys have been
**			    specified yet
**	psq_cb
**	    psq_mode	    set to PSQ_MODIFY
**	    psq_error	    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	Opens a memory stream and allocates space for QEU_CB, DMU_CB, key
**	entries,characteristics array, and locations entries
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	19-nov-91 (andre)
**	    merged 7-mar-1991 (bryanp) change:
**		Added with_clauses argument and added support for Btree Index
**		Compression.
**	21-apr-92 (barbara)
**	    Updated for Sybil (use different constant to tell if distrib).
**	08-feb-95 (cohmi01)
**	    For RAW/IO, allocate storage for extent names in dmu_cb.
**	23-feb-2004 (schka24)
**	    Init logical partition lookup things.
**	28-Oct-2005 (schka24)
**	    Set up general query name for later errmsgs.
*/
DB_STATUS
psl_md2_modstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	PSS_YYVARS	*yyvarsp)
{
    i4	    err_code;
    i4		    i;
    DB_STATUS	    status;
    QEU_CB	    *qeu_cb;
    DMU_CB	    *dmu_cb;

    psq_cb->psq_mode = PSQ_MODIFY;
    psl_command_string(PSQ_MODIFY, sess_cb,
		&yyvarsp->qry_name[0], &yyvarsp->qry_len);

    /* "modify" is not allowed in distributed yet */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	(VOID) psf_error(5530L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);    /* non-zero return means error */
    }

    /* Start off with no columns in the key */
    sess_cb->pss_rsdmno = 0;

    /* Allocate QEU_CB for MODIFY and initialize its header */
    status = psl_qeucb(sess_cb, DMU_MODIFY_TABLE, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    qeu_cb = (QEU_CB *) sess_cb->pss_object;

    /* Allocate a DMU control block */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_CB),
	(PTR *) &dmu_cb, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    qeu_cb->qeu_d_cb = (PTR) dmu_cb;

    /* Zero everything in the new dmu cb */
    MEfill(sizeof(DMU_CB), 0, dmu_cb);

    /* Fill in the DMU control block header */
    dmu_cb->type	    = DMU_UTILITY_CB;
    dmu_cb->length	    = sizeof(DMU_CB);
    dmu_cb->dmu_flags_mask  = 0;
    dmu_cb->dmu_db_id	    = (char*) sess_cb->pss_dbid;
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dmu_cb->dmu_owner);

    /*
    ** Allocate the key entries.  Allocate enough space for DB_MAX_COLS
    ** (the maximum number of columns in a table), although it's probably
    ** fewer.  This is because we don't know how many columns we have at
    ** this point, and it would be a big pain to allocate less and then
    ** run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	DB_MAX_COLS * sizeof(DMU_KEY_ENTRY *),
	(PTR *) &dmu_cb->dmu_key_array.ptr_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    dmu_cb->dmu_key_array.ptr_size	= sizeof(DMU_KEY_ENTRY);
    dmu_cb->dmu_key_array.ptr_in_count	= 0;	    /* Start with 0 keys */

    /*
    ** Allocate the location entries.  Assume DM_LOC_MAX, although it's
    ** probably fewer.  This is because we don't know how many locations
    ** we have at this point, and it would be a big pain to allocate
    ** less and then run into problems.
    */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, DM_LOC_MAX * sizeof(DB_LOC_NAME),
	(PTR *) &dmu_cb->dmu_location.data_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    dmu_cb->dmu_location.data_in_size = 0;    /* Start with 0 locations */

    /* partition, other stuff zeroed by mefill */

    /* No logical partitions yet (modify pmast PARTITION logpart. ... ) */
    yyvarsp->md_part_lastdim = -1;
    for (i = 0; i < DBDS_MAX_LEVELS; ++i)
	yyvarsp->md_part_logpart[i] = -1;
    yyvarsp->md_reconstruct = FALSE;
    yyvarsp->is_heapsort = FALSE;

    return(E_DB_OK);
}

/*{
** Name: psl_md3_modunique - semantic action for MODSTORAGE production
**
** Description:
**  This is a semantic action for MODSTORAGE production used by both SQL
**  and QUEL grammars (modstorage:	<storage_type> UNIQUE).
**
**	The user has specified a UNIQUE storage structure, check it out
**	and remember the uniqueness.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	yyvarsp		    Parser state variables
**
** Outputs:
**	err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	N/A
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	06-apr-93 (rblumer)
**	    added with_clauses parameter; set bit in it for UNIQUE option.
**	23-Feb-2004 (schka24)
**	    check for modify to reconstruct.
**	4-may-2007 (dougi)
**	    Make room for possible "unique_scope = statement".
**	13-Oct-2010 (kschendel) SIR 124544
**	    Rename for clarity; changes for dmu characteristics.
*/
DB_STATUS
psl_md3_modunique(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk)
{
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /*
    ** Unique cannot be specified with MODIFY TO RELOCATE
    ** or MODIFY TO RECONSTRUCT.  (The latter looks like a regular modify to
    ** storage structure once it gets here, so there's a separate flag.)
    */

    if (qeu_cb->qeu_d_op != DMU_MODIFY_TABLE
	||
	dmu_cb->dmu_action != DMU_ACT_STORAGE
	||
	yyvarsp->md_reconstruct
	||
	(dmu_cb->dmu_chars.dmu_struct != DB_ISAM_STORE &&
	 dmu_cb->dmu_chars.dmu_struct != DB_HASH_STORE &&
	 dmu_cb->dmu_chars.dmu_struct != DB_BTRE_STORE
	)
       )
    {
	i4		err_code;

	/* "unique" not allowed */
	(VOID) psf_error(5512L, 0L, PSF_USERERR, &err_code,
	    err_blk, 2,
	    psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
	    &sess_cb->pss_resrng->pss_tabdesc->tbl_name,
	    sizeof("unique") - 1, "unique");
	return (E_DB_ERROR);
    }

    /* Indicate uniqueness by setting WITH-option flag */
    BTset(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators);

    return(E_DB_OK);
}

/*{
** Name: psl_md4_modstorname - semantic action for MODSTORNAME production
**
** Description:
**  This is a semantic action for MODSTORNAME production used by both SQL
**  and QUEL grammars.
**
**	See the modify_stornames array for the list of allowed MODIFY
**	actions.  The most commonly used ones are storage structure names,
**	hence the routine name;  but there are lots and lots and lots of
**	special things.  MODIFY is a good place to dump physical table
**	alterations of one sort or another.
**
**	NOTE: There are two classes of MODIFY action names that does not
**	go through here, and that's the two that look like WITH-options:
**		modify tab to PRIORITY=n
**		modify tab to UNIQUE_SCOPE=[row|statement]
**	These are handled by the grammar rules for WITH-options, and so
**	the DMU action is set by psl-withopt-post.
**
**	For RECONSTRUCT, the storage structure and uniqueness DMU
**	indicators are set, but nothing else.  The structure key
**	columns and all the attributes are added at the end of modify
**	parsing, to allow overrides from the modify statement.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_ostream	    stream to use for memory allocation
**	psq_cb		    query parse state cb
**	storname	    value specified by the user
**
** Outputs:
**	yyvarsp
**	    .is_heapsort    set to 1 if [C]HEAPSORT was specified
**	err_blk		    will be filled in if an error occurs
**	DMU-chars indicators, DMU cb action code updated
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	if storname=="relocate", memory will be allocated for OLOCATION entries
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	19-nov-91 (andre)
**	    merge 5-mar-1991 (bryanp) change:
**		Added 'with_clauses' argument and added duplicate with clause
**		checking as part of Btree index compression project.
**	30-mar-1993 (rmuth)
**	    Add support for "to add_extend".
**	15-may-1993 (rmuth)
**	    Add support for [NO]READONLY tables.
**	26-July-1993 (rmuth)
**	    Alter [NO]READONLY to use just a DMU_READONLY entry in
**	    DMU_CHAR_ENTRY.
**      16-Sep-1993 (fred)
**          Set up the defaults in the relocation case to relocate the 
**          base table and any extensions.  This default is changed 
**          later via a `with' clause to move either only the 
**          extensions or no extensions.
**	06-jan-1994 (forky01)
**	    Add support for partial recovery syntax. Use storname to
**          pass status change of PHYS_[IN]CONSISTENT | LOG_[IN]CONSISTENT |
**	    TABLE_RECOVERY_[DIS]ALLOWED.  Add new entry in DMU_CHAR_ENTRY
**	    for DMU_INCONSISTENT, DMU_LOG_INCONSISTENT, 
**	    DMU_TABLE_RECOVERY_DISALLOWED.
**	9-apr-98 (inkdo01)
**	    Add support for "persistence", "unique_scope" for constraint
**	    index with options.
**	29-jan-99 (inkdo01)
**	    Set STRUCTURE flag for modify's.
**	24-Feb-2004 (schka24)
**	    Table drive;  add modify to reconstruct.
**	12-Mar-2004 (schka24)
**	    Fix bad flag test in the above.
**	29-Apr-2004 (schka24)
**	    Disable (temporarily??) modify-to-relocate for partitioned tables.
**	05-May-2006 (jenjo02)
**	    Add "clustered" storage structure for Clustered primary Btree.
**	4-may-2007 (dougi)
**	    Tolerate lack of "unique" for unique_scope when we know it is
**	    "modify ... to <structure> ... unique_scope ...".
**	19-Mar-2008 (kschendel) b122118
**	    Catch modify si to truncated here, don't wait for DMF to catch
**	    it with a funky key-sequence error msg.
**	07-Apr-2009 (thaju02)
**	    Check if modify action is for a single partition. (B121893)
**	13-Oct-2010 (kschendel) SIR 124544
**	    Revise the table, new dmu-action and dmu indicators.
*/
DB_STATUS
psl_md4_modstorname(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb,
	char		*storname)
{
    DB_ERROR		*err_blk;
    i4			err_code;
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMT_TBL_ENTRY	*showptr;
    const struct _MODIFY_STORNAMES *stornames_ptr;
    PSS_YYVARS		*yyvarsp;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    err_blk = &psq_cb->psq_error;
    yyvarsp = sess_cb->pss_yyvars;
    /* Table info frequently needed: */
    showptr = sess_cb->pss_resrng->pss_tabdesc;

    /* handle RELOCATE separately */
    if (!STcompare(storname, "relocate"))
    {
	DB_STATUS	    status;

	/* Don't allow for partitioned tables, as there's no DMF support
	** yet.  (and nobody can get real enthused, either.  I guess
	** we'll get to it.)
	*/

	if (showptr->tbl_status_mask & DMT_IS_PARTITIONED)
	{
	    (void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0L, PSF_USERERR,
			&err_code, err_blk, 3,
			0, "MODIFY",
			0, "RELOCATE",
			0, "a partitioned table");
	    return (E_DB_ERROR);
	}

	/* Tell QEF we are doing a relocate */
	qeu_cb->qeu_d_op = DMU_RELOCATE_TABLE;

	/*
	** Allocate the OLOCATION entries. Assume DM_LOC_MAX, although it's
	** probably fewer.  This is because we don't know how many locations
	** we have at this point, and it would be a big pain to allocate
	** less and then run into problems.
	*/
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	    DM_LOC_MAX * sizeof(DB_LOC_NAME),
	    (PTR *) &dmu_cb->dmu_olocation.data_address, err_blk);
	if (status != E_DB_OK)
	    return (status);

	/* This is bogus, but set it anyway in case someone looks */
	dmu_cb->dmu_action = DMU_ACT_RELOC;

	dmu_cb->dmu_olocation.data_in_size = 0; /* Start with 0 locations */
	dmu_cb->dmu_flags_mask = DMU_EXTTOO_MASK;  /* The default */

    }
    else
    {

	/* Search the action names table for our action */
	stornames_ptr = &modify_stornames[0];
	do
	{
	    if (STcompare(storname, stornames_ptr->name_string) == 0)
		break;
	    ++ stornames_ptr;
	} while (stornames_ptr->name_string != NULL);

	if (stornames_ptr->name_string == NULL)
	{
	    (VOID) psf_error(5510L, 0L, PSF_USERERR, &err_code,
		err_blk, 1, (i4) STtrmwhite(storname), storname);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}

	/* Make special case checks */
	if (stornames_ptr->special_flags & PSL_MDF_ERROR)
	{
	    /* obsolete action, issue error */
	    (VOID) psf_error(5505L, 0L, PSF_USERERR, &err_code,
		err_blk, 1,
		psf_trmwhite(sizeof (DB_TAB_NAME), 
		    (char *) &showptr->tbl_name),
		&showptr->tbl_name);
	    return (E_DB_ERROR);
	}
	if (stornames_ptr->special_flags & PSL_MDF_NOINDEX
	  && showptr->tbl_status_mask & DMT_IDX)
	{
	    /* Invalid modify secondary-index to xxx, complain */
	    (void) psf_error(5540, 0, PSF_USERERR, &err_code,
		err_blk, 2,
		psf_trmwhite(sizeof (DB_TAB_NAME), 
		    (char *) &showptr->tbl_name),
		&showptr->tbl_name,
		0, storname);
	    return (E_DB_ERROR);
	}
	if (stornames_ptr->special_flags & PSL_MDF_RECONSTRUCT)
	{
	    /* Modify to reconstruct is just modify to storage-structure.
	    ** Set the proper storage structure.  We'll have to come back
	    ** and preserve storage structure options later, so remember
	    ** that this is reconstruct.
	    */
	    BTset(DMU_STRUCTURE, dmu_cb->dmu_chars.dmu_indicators);
	    dmu_cb->dmu_chars.dmu_struct = showptr->tbl_storage_type;
	    yyvarsp->md_reconstruct = TRUE;
	    /* If the storage structure was UNIQUE, say so now.  Some
	    ** of the with-clause validations need to know about UNIQUE.
	    */
	    if (showptr->tbl_status_mask & DMT_UNIQUEKEYS)
		BTset(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators);
	}
	if (stornames_ptr->special_flags & PSL_MDF_MERGE)
	{
	    /* Modify to merge requires BTREE table */
	    if (showptr->tbl_storage_type != DB_BTRE_STORE)
	    {
		(VOID) psf_error(5525L, 0L, PSF_USERERR, &err_code,
		    err_blk, 1,
		    psf_trmwhite(sizeof(DB_TAB_NAME),
			(char *) &showptr->tbl_name),
		    &showptr->tbl_name);
		return (E_DB_ERROR);
	    }
	}
	if (showptr->tbl_status_mask & DMT_IS_PARTITIONED)
	{
	    /* Some actions require a specific physical partition.  For
	    ** these, verify that we resolved the DMU request to a partition.
	    */
	    if (stornames_ptr->special_flags & PSL_MDF_PPART_ONLY)
	    {
		if (dmu_cb->dmu_tbl_id.db_tab_index >= 0)
		{
		    (void) psf_error(E_US1946_6464_PPART_ONLY, 0, PSF_USERERR,
			&err_code, err_blk, 2,
			0, "MODIFY", 0, storname);
		    return (E_DB_ERROR);
		}
	    }
	    else if ((stornames_ptr->special_flags & PSL_MDF_LPART_OK) == 0
			&& (dmu_cb->dmu_ppchar_array.data_in_size > 0
			    || dmu_cb->dmu_tbl_id.db_tab_index < 0))
	    {
		/* Other actions are only allowed on the whole table */
		(void) psf_error(E_US1945_6463_LPART_NOTALLOWED, 0, PSF_USERERR,
			&err_code, err_blk, 2,
			0, "MODIFY", 0, storname);
		return (E_DB_ERROR);
	    }
	} /* if partitioned */

	if (sess_cb->pss_resrng->pss_tabdesc->tbl_2_status_mask & DMT_PARTITION)
        {
	    /* partition */
	    if (!(stornames_ptr->special_flags & PSL_MDF_PPART_ONLY))
	    {
                   (void) psf_error(2117L, 0, PSF_USERERR,
                        &err_code, err_blk, 1,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *)&sess_cb->pss_resrng->pss_tabdesc->tbl_name),
			&sess_cb->pss_resrng->pss_tabdesc->tbl_name);
                    return (E_DB_ERROR);
	    }
	}
        
        if (stornames_ptr->special_flags & PSL_MDF_NO_OP)
        {
            /* Table will NOT be modified so skip dependency checking 
            ** this serves as an optimisation and also prevents
            ** operations from failing when there is no need to
            ** block the operation.  Do not set the "nodependency_check"
	    ** indicator, user can explicitly say "with nodependency_check"
	    ** if they so choose.
            */
            dmu_cb->dmu_flags_mask |= DMU_NODEPENDENCY_CHECK;
        }

	/* Action passes tests, set the DMU action, and maybe a DMU
	** characteristics indicator and value.
	*/
	dmu_cb->dmu_action = stornames_ptr->action;
	yyvarsp->md_action = stornames_ptr->action;
	if (stornames_ptr->indicator != -1)
	{
	    BTset(stornames_ptr->indicator, dmu_cb->dmu_chars.dmu_indicators);
	    switch (stornames_ptr->indicator)
	    {
	    case DMU_STRUCTURE:
		dmu_cb->dmu_chars.dmu_struct = stornames_ptr->value;
		break;

	    case DMU_ACTION_ONOFF:
	    case DMU_PERSISTS_OVER_MODIFIES:
		dmu_cb->dmu_chars.dmu_flags |= stornames_ptr->value;
		break;

	    case DMU_VACTION:
		dmu_cb->dmu_chars.dmu_vaction = stornames_ptr->value;
		break;
	    }
	}
	if (stornames_ptr->special_flags & PSL_MDF_COMPRESSED)
	{
	    dmu_cb->dmu_chars.dmu_dcompress = DMU_COMP_DFLT;
	    dmu_cb->dmu_chars.dmu_kcompress = DMU_COMP_DFLT;
	    BTset(PSS_WC_COMPRESSION, dmu_cb->dmu_chars.dmu_indicators);
	}

	/*
	** If modifying to CLUSTERED or table is already clustered and
	** storage structure isn't changing, set/retain DMU_CLUSTERED.
	** NB: Clustered requires "unique"; it will be forced in dmu_modify.
	*/
	if ( stornames_ptr->special_flags & PSL_MDF_CLUSTERED ||
	    (showptr->tbl_status_mask & DMT_CLUSTERED &&
	     stornames_ptr->action != DMU_ACT_STORAGE) )
	{
	    BTset(DMU_UNIQUE, dmu_cb->dmu_chars.dmu_indicators);
	    BTset(DMU_CLUSTERED, dmu_cb->dmu_chars.dmu_indicators);
	    dmu_cb->dmu_chars.dmu_flags |= DMU_FLAG_CLUSTERED;
	}

	/* One last special, heapsort has a special flag */
	if (stornames_ptr->special_flags & PSL_MDF_HEAPSORT)
	    yyvarsp->is_heapsort = TRUE;

    } /* not relocate */

    /* Since modify establishes the WITH-option context early on,
    ** we need to re-establish it now that the modify action is known.
    */
    psl_withopt_init(sess_cb, psq_cb);

    return(E_DB_OK);
}

/*{
** Name: psl_md5_modkeys - semantic action for MODKEYS production
**
** Description:
**  This is a semantic action for MODKEYS production used by both SQL and
**  QUEL grammars.
**
**	This action is called when no "ON column-list" has been seen.
**	No column list is required if the modify is not a storage
**	structuring one, or is RECONSTRUCT.  For structuring
**	modifies, the column list is not required for HEAP.  If a
**	key column list is needed, we'll grab the first column and
**	use it as a key.  (Not a particularly useful choice in general,
**	but that's how it works.)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	yyvarsp		    Parser state variables
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	Allocates memory for a key entry
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	18-mar-91 (andre)
**	    call psl_check_key() to check datatype for keyability
**	    (this was added as a part of integrating 6.3/04 changes into 6.5)
**	25-Feb-2004 (schka24)
**	    Skip key stuff if RECONSTRUCT.
*/
DB_STATUS
psl_md5_modkeys(
	PSS_SESBLK	*sess_cb,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk)
{
    DMU_CB		*dmu_cb;
    QEU_CB		*qeu_cb;

    /* LOAD DEFAULT MODIFY KEY */

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /*
    ** no need to specify a key if structure was not specified or if heap
    ** was specified; if qeu_d_op is not DMU_MODIFY_TABLE, RELOCATE must 
    ** have been specified, so no structure could possibly have been
    ** specified
    */
    if (qeu_cb->qeu_d_op == DMU_MODIFY_TABLE
	&&
	dmu_cb->dmu_action == DMU_ACT_STORAGE
	&&
	! yyvarsp->md_reconstruct
	&&
	(dmu_cb->dmu_chars.dmu_struct != DB_HEAP_STORE || yyvarsp->is_heapsort)
       )
    {
	DB_STATUS	status;
	DMU_KEY_ENTRY	**key;

	key = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_KEY_ENTRY),
	    (PTR *) key, err_blk);
	if (status != E_DB_OK)
	    return (status);

	/* Look up column name */
	cui_move(sess_cb->pss_resrng->pss_attdesc[1]->att_nmlen,
	    sess_cb->pss_resrng->pss_attdesc[1]->att_nmstr, ' ', 
	    sizeof(DB_ATT_NAME), (char *) &(*key)->key_attr_name);

	status = psl_check_key(sess_cb, err_blk,
		(DB_DT_ID) sess_cb->pss_resrng->pss_attdesc[1]->att_type);
	if (DB_FAILURE_MACRO(status))
	{
	    i4		    err_code;

	    (VOID) psf_error(2179L, 0L, PSF_USERERR, &err_code, err_blk, 2,
		sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
		psf_trmwhite(DB_ATT_MAXNAME, (char *) &(*key)->key_attr_name),
		&(*key)->key_attr_name);
	    return (E_DB_ERROR);
	}

	/* Add a key to the array */
	(*key)->key_order = DMU_ASCENDING;
	dmu_cb->dmu_key_array.ptr_in_count += 1;
    }

    return(E_DB_OK);
}

/*{
** Name: psl_md6_modbasekey - semantic action for MODBASEKEY production
**
** Description:
**  This is a semantic action for MODBASEKEY production used by both SQL and
**  QUEL grammars.
**
**	This production is the sort direction, ASC or DESC.  For
**	now this can only be specified for HEAPSORT.  The grammar
**	assures that a key column name was already seen (md7) and
**	we're assured that this is a structure modify.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_lang	    query language
**	ascending	    non-zero if sort order was specified as ASC in SQL
**			    or ASCENDING in QUEL;
**			    zero if sort order was specified as DESC in SQL or
**			    DESCENDING in QUEL
**	heapsort	    non-zero if [C]HEAPSORT structure was specified;
**			    zero otherwise
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	will store direction of sort in the appropriate DMU_KEY_ENTRY
**
** History:
**	11-feb-91 (andre)
**	    plagiarized from SQL grammar
*/
DB_STATUS
psl_md6_modbasekey(
	PSS_SESBLK	*sess_cb,
	bool		ascending,
	DB_ERROR	*err_blk)
{
    QEU_CB		*qeu_cb;
    DMU_CB		*dmu_cb;
    DMU_KEY_ENTRY	**keys;
    DMU_KEY_ENTRY	*key;

    /* Sortorder allowed only with heapsort & cheapsort */
    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;
    keys = (DMU_KEY_ENTRY**) dmu_cb->dmu_key_array.ptr_address;
    key = keys[sess_cb->pss_rsdmno - 1];

    if (dmu_cb->dmu_chars.dmu_struct != DB_HEAP_STORE
      || ! sess_cb->pss_yyvars->is_heapsort)
    {
	i4         err_code;
	char		*sortword;
	i4		sortw_len;

	if (sess_cb->pss_lang == DB_SQL)
	{
	    if (ascending)
	    {
		sortword = "asc";
		sortw_len = sizeof("asc") - 1;
	    }
	    else
	    {
		sortword = "desc";
		sortw_len = sizeof("desc") - 1;
	    }
	}
	else
	{
	    if (ascending)
	    {
		sortword = "ascending";
		sortw_len = sizeof("ascending") - 1;
	    }
	    else
	    {
		sortword = "descending";
		sortw_len = sizeof("descending") - 1;
	    }
	}

	(VOID) psf_error(5520L, 0L, PSF_USERERR, &err_code, err_blk, 3,
	    psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
	    &sess_cb->pss_resrng->pss_tabdesc->tbl_name,
	    sortw_len, sortword,
	    psf_trmwhite(sizeof(DB_ATT_NAME), (char *) &key->key_attr_name),
	    &key->key_attr_name);
	return (E_DB_ERROR);
    }

    key->key_order = (ascending) ? DMU_ASCENDING : DMU_DESCENDING;

    return(E_DB_OK);
}

/*{
** Name: psl_md7_modkeyname - semantic action for MODKEYNAME production
**
** Description:
**  This is a semantic action for MODKEYNAME production used by both SQL and
**  QUEL grammars.
**
**	An ON column name has been parsed.  Make sure that the MODIFY
**	variant allows the ON key-column clause;  the only variants that
**	do allow it are the modify to <storage structure> variants, and
**	not HEAP or RECONSTRUCT.
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	keyname		    name of the key
**	yyvarsp		    Parser state variables
**
** Outputs:
**     err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	Allocates memory for a new key entry
**
** History:
**	12-feb-91 (andre)
**	    plagiarized from SQL grammar
**	18-mar-91 (andre)
**	    call psl_check_key() to check datatype for keyability
**	    (this was added as a part of integrating 6.3/04 changes into 6.5)
**	08-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid"
**	25-Feb-2004 (schka24)
**	    No column names if RECONSTRUCT.
*/
DB_STATUS
psl_md7_modkeyname(
	PSS_SESBLK	*sess_cb,
	char		*keyname,
	PSS_YYVARS	*yyvarsp,
	DB_ERROR	*err_blk)
{
    char		    *err_string;
    i4		    err_code;
    DB_STATUS		    status;
    QEU_CB		    *qeu_cb;
    DMU_CB		    *dmu_cb;
    DMT_ATT_ENTRY	    *attribute;
    DB_ATT_NAME		    attname;
    register DMU_KEY_ENTRY  **key;
    register i4	    i;
    i4			    colno;
    i4			    key_nmlen;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    if (qeu_cb->qeu_d_op != DMU_MODIFY_TABLE ||
	dmu_cb->dmu_action != DMU_ACT_STORAGE)
    {
	/* Not allowed unless storage structure present. */
	(VOID) psf_error(5515L, 0L, PSF_USERERR, &err_code,
	    err_blk, 0);
	return (E_DB_ERROR);
    }

    /* no keys on heap or RECONSTRUCT */
    err_string = NULL;
    if (yyvarsp->md_reconstruct)
	err_string = ERx("with the RECONSTRUCT operation");
    else if (dmu_cb->dmu_chars.dmu_struct == DB_HEAP_STORE && !yyvarsp->is_heapsort)
	err_string = ERx("for the HEAP structure");
    if (err_string != NULL)
    {
	(VOID) psf_error(5502L, 0L, PSF_USERERR, &err_code,
		err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
		&sess_cb->pss_resrng->pss_tabdesc->tbl_name,
		0, err_string);
	return (E_DB_ERROR);
    }

    /* Count columns in key.  Error if too many */
    colno = sess_cb->pss_rsdmno;
    if (++sess_cb->pss_rsdmno > DB_MAX_COLS)
    {
	(VOID) psf_error(5503L, 0L, PSF_USERERR, &err_code,
	    err_blk, 0);
	return (E_DB_ERROR);
    }

    /* Look up column name */
    key_nmlen = STlength(keyname);
    cui_move(key_nmlen, keyname, ' ', sizeof(DB_ATT_NAME), (char *) &attname);
    attribute = pst_coldesc(sess_cb->pss_resrng, keyname, key_nmlen);
    if (attribute == (DMT_ATT_ENTRY *) NULL)
    {
	(VOID) psf_error(5511L, 0L, PSF_USERERR, &err_code,
	    err_blk, 2,
	    psf_trmwhite(sizeof(DB_TAB_NAME),
		(char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
	    &sess_cb->pss_resrng->pss_tabdesc->tbl_name,
	    STtrmwhite(keyname), keyname);
	return (E_DB_ERROR);
    }

    /*
    ** Make sure column isn't listed twice and its name is not `tid'.
    */

    /* Error if the name is equal to "tid" */
    if (STlength(keyname) == sizeof("tid") - 1 &&
	!MEcmp(keyname,
		((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"),
		sizeof("tid") - 1))
    {
	(VOID) psf_error(2105L, 0L, PSF_USERERR,
	    &err_code, err_blk, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    sizeof("tid") - 1,
	    ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? "TID" : "tid"));
	return (E_DB_ERROR);
    }

    status = psl_check_key(sess_cb, err_blk, (DB_DT_ID) attribute->att_type);

    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(2179L, 0L, PSF_USERERR, &err_code, err_blk, 2,
	    sizeof(sess_cb->pss_lineno), &sess_cb->pss_lineno,
	    psf_trmwhite(DB_ATT_MAXNAME, (char *) &attname), &attname);
	return (status);
    }

    for (i = 0, key = (DMU_KEY_ENTRY **) dmu_cb->dmu_key_array.ptr_address;
	 i < colno;
	 i++, key++)
    {
	if (!MEcmp((char *) &attname, (char *) &(*key)->key_attr_name,
	    sizeof(DB_ATT_NAME)))
	{
	    (VOID) psf_error(5507L, 0L, PSF_USERERR, &err_code,
		err_blk, 2,
		psf_trmwhite(sizeof(DB_TAB_NAME),
		    (char *) &sess_cb->pss_resrng->pss_tabdesc->tbl_name),
		&sess_cb->pss_resrng->pss_tabdesc->tbl_name,
		STtrmwhite(keyname), keyname);
	    return (E_DB_ERROR);
	}
    }

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMU_KEY_ENTRY),
	(PTR *) key, err_blk);
    if (status != E_DB_OK)
	return (status);
    /* Add a key to the array */
    STRUCT_ASSIGN_MACRO(attname, (*key)->key_attr_name);
    (*key)->key_order = DMU_ASCENDING;
    dmu_cb->dmu_key_array.ptr_in_count++;

    return(E_DB_OK);
}

/*{
** Name: psl_md8_modtable - semantic action for MODTABLE production
**
** Description:
**	This is a semantic action for MODTABLE production
**
**	modtable:	    obj_spec	    (SQL)
**	modtable:	    NAME	    (QUEL)
**
** Inputs:
**	sess_cb		    ptr to a PSF session CB
**	    pss_object	    points to preallocated QEU_CB
**	    pss_ses_flag    describes session attributes
**		PSS_CATUPD  user has catalog update privilege
**	    pss_user	    name of the effective user
**	tblspec		    address of a PSS_OBJ_NAME structure describing
**			    table specified by the user
**	    pss_objspec_flags
**		PSS_OBJSPEC_EXPL_SCHEMA
**			    set if object name was qualified with schema
**			    name (i.e. schema.object construct was used)
**	    pss_owner	    name of the schema (same as object owner unless we 
**			    are dealing with DGTT) if schema.object construct
**			    was used
**	    pss_obj_name    normalized table name
**
** Outputs:
**	sess_cb
**	    pss_auxrng	    range table
**		pss_rsrng   will contain description of the table
**	    pss_resrng	    will point at the range table entry containing the
**			    description of the table to be modified
**			    (sess_cb->pss_auxrng.pss_rsrng)
**	err_blk		    will be filled in if an error occurs
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	N/A
**
** History:
**	21-mar-91 (andre)
**	    plagiarized from SQL grammar
**	28-sep-92 (robf)
**	   Detect trying to MODIFY the structure of a gateway table - 
**	   MODIFY only applies to local INGRES tables (can end up with
**	   table in illegal state due to bypassing REGISTER checks)
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**      12-Jul-1993 (fred)
**          Disallow direct manipulation of extension tables.
**	17-aug-93 (andre)
**	    PSS_OBJ_NAME.pss_qualified has been replaced with 
**	    PSS_OBJSPEC_EXPL_SCHEMA defined over PSS_OBJ_NAME.pss_objspec_flags
**	03-jan-96 (kch)
**	    SIR 72811 - remove restriction on direct manipulation of
**	    extension tables.
**	25-mar-98 (nanpr01)
**	    It is a bad idea to implement the SIR 72811 since customer
**	    base can mess up extension tables.
**	26-Feb-2004 (schka24)
**	    Fill in number of partitions for qef.
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl_rngent for WITH support.
*/
DB_STATUS
psl_md8_modtable(
	PSS_SESBLK	*sess_cb,
	PSS_OBJ_NAME	*tblspec,
	DB_ERROR	*err_blk)
{
    DB_STATUS		    status;
    i4			    rngvar_info;
    PSS_RNGTAB		    *rngvar;
    QEU_CB		    *qeu_cb;
    DMU_CB		    *dmu_cb;
    i4		    err_code;
    i4		    mask;
    i4		    mask2;
    i4		    err_num = 0L;
    bool		    must_audit = FALSE;


    if (tblspec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
    {
	status = psl_orngent(&sess_cb->pss_auxrng, -1, "",
	    &tblspec->pss_owner, &tblspec->pss_obj_name, sess_cb, FALSE,
	    &rngvar, PSQ_MODIFY, err_blk, &rngvar_info);
    }
    else
    {
	status = psl_rngent(&sess_cb->pss_auxrng, -1, "",
	    &tblspec->pss_obj_name, sess_cb, FALSE, &rngvar, PSQ_MODIFY,
	    err_blk, &rngvar_info, NULL);
    }

    if (DB_FAILURE_MACRO(status))
	return (status);

    mask = rngvar->pss_tabdesc->tbl_status_mask;
    mask2 = rngvar->pss_tabdesc->tbl_2_status_mask;

    if (mask & DMT_CATALOG || mask2 & DMT_TEXTENSION)
    {
	/* If not an extended catalog and no catalog update
	** permission - error.
	*/
	if (~mask & DMT_EXTENDED_CAT && !(sess_cb->pss_ses_flag & PSS_CATUPD))
	{
	    must_audit = TRUE;
	    err_num = 5504L;
	}
    }
    /*
    ** Make sure that either the user or the session
    ** owns the table being modified
    */
    else if (MEcmp((char *) &rngvar->pss_ownname, (char *) &sess_cb->pss_user,
		    sizeof(DB_OWN_NAME)) != 0 &&
	     MEcmp((char *) &rngvar->pss_ownname,
		    (char *) &sess_cb->pss_sess_owner,
		    sizeof(DB_OWN_NAME)) != 0)
    {
	must_audit = TRUE;
	err_num = 5501L;
    }

    /* Don't allow user to modify a view */
    if (err_num == 0L && mask & DMT_VIEW)
    {
	err_num = 5519L;
    }
    /*
    **	Don't allow user to modify a Gateway table, unless user
    **  has CATUPD priv (where all bets are off anyway)
    */
    if (err_num == 0L && (mask & DMT_GATEWAY) &&
	 !(sess_cb->pss_ses_flag & PSS_CATUPD))
    {
	err_num = 5557L;
    }
    if (err_num != 0L)
    {
	status = E_DB_ERROR;

	if ( must_audit && Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    /* Must audit MODIFY failure. */
	    DB_STATUS   local_status;
	    DB_ERROR	e_error;

	    local_status = psy_secaudit(FALSE, sess_cb,
	    		(char *)&rngvar->pss_tabname,
			&rngvar->pss_tabdesc->tbl_owner,
	    		sizeof(DB_TAB_NAME), SXF_E_TABLE,
	      		I_SX270F_TABLE_MODIFY, SXF_A_FAIL | SXF_A_MODIFY,
	      		&e_error);

	    
	    if (DB_FAILURE_MACRO(local_status) && local_status > status)
	    {
		status = local_status;
	    }
	}

	/*
	** let user know if name supplied by the user was resolved to a
	** synonym (this can only happen in SQL)
	*/
	if (rngvar_info & PSS_BY_SYNONYM)
	{
	    psl_syn_info_msg(sess_cb, rngvar, tblspec, rngvar_info,
		sizeof("MODIFY") - 1, "MODIFY", err_blk);
	}

	_VOID_ psf_error(err_num, 0L, PSF_USERERR, &err_code, err_blk, 1,
	    psf_trmwhite(sizeof(DB_TAB_NAME), (char *) &rngvar->pss_tabname),
	    &rngvar->pss_tabname);

	return (status);
    }

    sess_cb->pss_resrng = rngvar;

    qeu_cb = (QEU_CB *) sess_cb->pss_object;
    dmu_cb = (DMU_CB *) qeu_cb->qeu_d_cb;

    /* Put it in the control block */
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, dmu_cb->dmu_tbl_id);
    /* for error handling in QEF */
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabname, dmu_cb->dmu_table_name);
    /* For QEF since it won't have the dmt-show info around at a
    ** convenient time:
    */
    dmu_cb->dmu_nphys_parts = rngvar->pss_tabdesc->tbl_nparts;

    return(E_DB_OK);
}

/*
** Name: psl_md_logpartname -- Handle logical partition name
**
** Description:
**	For partitioned tables, individual logical partitions can be
**	modified via the syntax:
**
**	modify tablename PARTITION logpart.logpart. ...
**
**	A "logpart" has been parsed and it's our job to figure out
**	which partition has been named.  The logical partition sequence
**	will be stored in yyvarsp->md_part_logpart[dim].
**
**	The logical partition names have to occur in increasing dimension
**	order, although it's allowable to skip dimensions.  A skipped
**	dimension has an md_part_logpart entry of -1, which is how they
**	are initialized.
**
**	Of course, if the master table isn't partitioned, this whole
**	thing is futile.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	yyvarsp		Pointer to parser state variables
**	logname		The logical partition name
**
** Outputs:
**	Returns E_DB_xxx status
**
** History:
**	23-Feb-2003 (schka24)
**	    Written.
*/

DB_STATUS
psl_md_logpartname(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb,
	PSS_YYVARS *yyvarsp, char *logname)
{

    bool found;
    DB_NAME pname;			/* Space-filled logical name */
    DB_NAME *pname_ptr;			/* Ptr to dimension's names */
    DB_PART_DEF *part_def_ptr;		/* Pointer to partition definition */
    i4 dim;				/* Dimension index */
    i4 lpart;				/* Logical partition index */
    i4 toss_err;

    /* Make sure that the table is partitioned */
    part_def_ptr = sess_cb->pss_resrng->pss_rdrinfo->rdr_parts;
    if ((sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IS_PARTITIONED) == 0
      || part_def_ptr == NULL)
    {
	(void) psf_error(E_US1941_6459_NOT_PART, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 2,
		0, "MODIFY",
		psf_trmwhite(sizeof(DB_TAB_NAME), sess_cb->pss_resrng->pss_tabname.db_tab_name),
		sess_cb->pss_resrng->pss_tabname.db_tab_name);
	return (E_DB_ERROR);
    }

    /* Look for the name, dimension by dimension.  (Each dimension has
    ** its own names array.)
    */
    cui_move(STlength(logname), logname, ' ', 
		sizeof(DB_NAME), &pname.db_name[0]);
    found = FALSE;
    for (dim = 0; dim < part_def_ptr->ndims; ++dim)
    {
	pname_ptr = part_def_ptr->dimension[dim].partnames;
	for (lpart = 0; lpart < part_def_ptr->dimension[dim].nparts; ++lpart)
	{
	    if (MEcmp(&pname.db_name[0], &pname_ptr[lpart].db_name[0], sizeof(DB_NAME)) == 0)
	    {
		found = TRUE;
		break;
	    }
	}
	if (found) break;
    }
    if (! found)
    {
	(void) psf_error(E_US1942_6460_EXPECTED_BUT, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 3,
		0, "MODIFY",
		0, "A logical partition name",
		0, logname);
	return (E_DB_ERROR);
    }

    /* Make sure that the dimension is higher than what we had before */
    if (dim <= yyvarsp->md_part_lastdim)
    {
	(void) psf_error(E_US1943_6461_NOT_SUBPART, 0, PSF_USERERR,
		&toss_err, &psq_cb->psq_error, 3,
		0, "MODIFY",
		0, logname,
		psf_trmwhite(sizeof(DB_NAME), &pname_ptr[lpart].db_name[0]),
		&pname_ptr[lpart]);
	return (E_DB_ERROR);
    }

    /* Remember the logical partition for later.  Use partition sequence
    ** (1 origin).
    */
    yyvarsp->md_part_lastdim = dim;
    yyvarsp->md_part_logpart[dim] = lpart + 1;

    return (E_DB_OK);
} /* psl_md_logpartname */

/*
** Name: psl_md_modpart - Determine partitions to modify
**
** Description:
**	The user has asked for a modify of specific logical partition(s),
**	via modify tablname PARTITION logpart.logpart...
**	DMF isn't too interested in logical partitions, so someone has
**	to figure out which physical partitions to modify.  We might
**	as well do that here.
**
**	The partitions to operate on are specified by means of a
**	DMU_PHYPART_CHAR array, one entry per physical partition.
**	Partitions that are not involved are flagged with an "ignore me"
**	flag.
**
** Inputs:
**	sess_cb		PSS_SESBLK parser session control block
**	psq_cb		Query parse control block
**	yyvarsp		Parser state variables
**	    .md_part_logpart[]  Logical partition sequence numbers;
**				-1 indicates "all in this dimension"
**
** Outputs:
**	Returns E_DB_xxx status
**	Constructs dmu_ppchar_array attached to dmucb.
**
** History:
**	23-feb-2004 (schka24)
**	    Written.
**	11-Mar-2004 (schka24)
**	    partno removed from ppchar array.
*/

DB_STATUS
psl_md_modpart(PSS_SESBLK *sess_cb, PSQ_CB *psq_cb, PSS_YYVARS *yyvarsp)
{

    DB_PART_DEF *part_def_ptr;		/* Table's partition definition */
    DB_STATUS status;			/* The usual call status */
    DMT_PHYS_PART *dmt_pp_base;		/* Master physical partition array */
    DMU_CB *dmucb;			/* The DMU cb we're building */
    DMU_PHYPART_CHAR *ppc_ptr;		/* Physical partition entry */
    i4 dim;				/* A dimension index */
    i4 n_not_ignored;			/* Number of not-ignored pp's */
    i4 pp_not_ignored;			/* Physical partition # not ignored */
    i4 ppart;				/* Physical partition index */
    i4 psize;				/* Size (bytes) of array */
    QEU_CB *qeucb;			/* QEU control block for modify */
    u_i2 cur_lpart[DBDS_MAX_LEVELS];	/* Logical partition sequence #s */

    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    part_def_ptr = sess_cb->pss_resrng->pss_rdrinfo->rdr_parts;
    dmt_pp_base = sess_cb->pss_resrng->pss_rdrinfo->rdr_pp_array;

    /* Allocate an array with one entry per partition */
    psize = part_def_ptr->nphys_parts * sizeof(DMU_PHYPART_CHAR);
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, psize,
		&dmucb->dmu_ppchar_array.data_address,
		&psq_cb->psq_error);
    if (DB_FAILURE_MACRO(status))
	return (status);
    MEfill(psize, 0, (PTR) dmucb->dmu_ppchar_array.data_address);
    dmucb->dmu_ppchar_array.data_in_size = part_def_ptr->nphys_parts * sizeof(DMU_PHYPART_CHAR);

    /* As we run through the physical partitions, count off the corresponding
    ** logical partition sequence numbers, and compare against what the
    ** md_part_logpart array tells us to do.
    */
    for (dim = 0; dim < part_def_ptr->ndims; ++dim)
	cur_lpart[dim] = 1;

    ppc_ptr = (DMU_PHYPART_CHAR *) dmucb->dmu_ppchar_array.data_address;
    n_not_ignored = 0;
    pp_not_ignored = -1;
    for (ppart = 0; ppart < part_def_ptr->nphys_parts; ++ppart, ++ppc_ptr)
    {
	STRUCT_ASSIGN_MACRO(dmt_pp_base[ppart].pp_tabid, ppc_ptr->part_tabid);
	/* if the logical partition subscripts corresponding to this physical
	** partition aren't in the user's list, set the "ignore" flag.
	*/
	for (dim = 0; dim < part_def_ptr->ndims; ++dim)
	{
	    if (yyvarsp->md_part_logpart[dim] != -1
	      && yyvarsp->md_part_logpart[dim] != cur_lpart[dim])
	    {
		ppc_ptr->flags |= DMU_PPCF_IGNORE_ME;
		break;			/* No need to keep going */
	    }
	}
	if ((ppc_ptr->flags & DMU_PPCF_IGNORE_ME) == 0)
	{
	    ++ n_not_ignored;
	    pp_not_ignored = ppart;
	}
	/* Advance the innermost logical partition sequence, if it runs
	** off the end reset to 1 and increment the next higher.  Do this
	** all the way up the dimensions as needed.
	*/
	for (dim = part_def_ptr->ndims - 1; dim >= 0; --dim)
	{
	    if (++ cur_lpart[dim] <= part_def_ptr->dimension[dim].nparts)
		break;
	    cur_lpart[dim] = 1;
	}
    }

    /* Ok, after all of that whirling around, see if the user actually asked
    ** for an operation on one single physical partition.  We can
    ** short-circuit things for that case and tell everyone to just work
    ** on the named partition.  That way nobody will be setting up all
    ** sorts of multi-partition machinery, etc and waste it on one partition.
    ** NOTE: If you need the master table ID, it's still in:
    ** sess_cb->pss_resrng->pss_tabid.
    */
    if (n_not_ignored == 1)
    {
	DMT_PHYS_PART *pp_base;

	dmucb->dmu_ppchar_array.data_address = NULL;
	dmucb->dmu_ppchar_array.data_in_size = 0;
	pp_base = sess_cb->pss_resrng->pss_rdrinfo->rdr_pp_array;
	STRUCT_ASSIGN_MACRO(pp_base[pp_not_ignored].pp_tabid,
		dmucb->dmu_tbl_id);
	dmucb->dmu_flags_mask |= DMU_PARTITION;
	dmucb->dmu_nphys_parts = 0;
    }

    return (E_DB_OK);
} /* psl_md_modpart */

/*
** Name: psl_md_reconstruct -- Finish up MODIFY TO RECONSTRUCT
**
** Description:
**	The MODIFY TO RECONSTRUCT variant is a shorthand way of
**	asking for "modify to the current storage structure with all
**	of the current storage structure options".  This is particularly
**	useful with partitioned tables: by asking for RECONSTRUCT,
**	we can clean up the storage structure of one or more partitions
**	without having to do all sorts of checking to to see whether
**	the overall table properties have changed.  (Indeed, in
**	this release Feb '04, all partitions must have identical storage
**	structures.)
**
**	This routine is called after the MODIFY statement is completely
**	parsed.  We'll update the DMU_CHARACTERISTICS with-option info with
**	various storage structure attributes to be preserved, unless
**	the modify already specified overrides.  The RECONSTRUCT keyword
**	has already set the DMU_STRUCTURE and the unique-key
**	request if needed;  we'll fill in everything else.  Specifically:
**
**	compression, fillfactor, leaffill, nonleaffill, minpages, maxpages,
**	unique_scope.
**	(Other attributes such as page size are "sticky" and don't need
**	to be set each time the structure is modified.)
**
**	We also have to come up with the key attribute array if the
**	storage structure is one that uses keys.
**
**	This seems like a good place to enforce partitioned table
**	restrictions as well.  Currently (Release 3.0), all partitions
**	must have *identical* storage structures, since there's only one
**	copy of iiattribute.  For now, unless the entire table is
**	being modified, we'll forbid the use of any override options.
**
**	This routine is called before doing the final validation of
**	with-options, in case some override conflicts with a default.
**	(e.g. defaulted maxpages of 20 and override minpages of 100 will
**	produce an error, as it should.)
**
** Inputs:
**	sess_cb		Parser session control block
**	dmucb		The DMU_CB being constructed for the modify
**	err_blk		An output
**
** Outputs:
**	Returns E_DB_xxx status
**	err_blk		Any error information placed here
**	Additional DMU_CHARACTERISTICS values may be set
**
** History:
**	25-Feb-2004 (schka24)
**	    Get modify to reconstruct working.
**	16-Mar-2004 (schka24)
**	    Don't include tidp for secondary indexes, DMF includes
**	    tidp implicitly;  if we include it here, DMF complains.
**	    Preserve persistence for indexes.
**	11-Oct-2010 (kschendel) SIR 124544
**	    Make global, now called from with-option handler.  WITH-option
**	    checking now looks in the DMU_CHARACTERISTICS, so put stuff there.
*/

DB_STATUS
psl_md_reconstruct(PSS_SESBLK *sess_cb, DMU_CB *dmucb, DB_ERROR *err_blk)
{
    char local_indicators[(DMU_ALLIND_LAST+BITSPERBYTE)/BITSPERBYTE];
    DB_STATUS status;			/* Called routine status */
    DMT_ATT_ENTRY **table_atts;		/* Table's attribute pointer array */
    DMT_TBL_ENTRY *tblinfo;		/* Table information area */
    DMU_KEY_ENTRY *key_array;		/* Base of key column array */
    DMU_KEY_ENTRY **keyptr_ptr;		/* Pointer to DMU key column pointer array */
    i4 *keycol_ptr;			/* Pointer to RDF's key column #'s */
    i4 keycount;			/* Key count excluding tidp */
    i4 psize;				/* Memory piece size needed */
    i4 toss_err;
    PSS_YYVARS *yyvarsp;
    RDD_KEY_ARRAY *keybase;		/* RDF key info header */

    yyvarsp = sess_cb->pss_yyvars;

    /* If doing a partial partition modify, check for any options.
    ** Disallow anything other than what RECONSTRUCT itself sets, namely
    ** the "structure" and "unique" with-options.  (Also allow LOCATION=
    ** since we do have the machinery to handle that one.)
    */
    if (dmucb->dmu_ppchar_array.data_in_size > 0
      || dmucb->dmu_tbl_id.db_tab_index < 0)
    {
	MEcopy(dmucb->dmu_chars.dmu_indicators, sizeof(local_indicators),
		local_indicators);
	BTclear(DMU_STRUCTURE, local_indicators);
	BTclear(DMU_UNIQUE, local_indicators);
	BTclear(PSS_WC_LOCATION, local_indicators);
	if (BTcount(local_indicators, PSS_WC_LAST) > 0)
	{
	    /* Slightly bogus error message but should be OK */
	    (void) psf_error(E_US1945_6463_LPART_NOTALLOWED, 0, PSF_USERERR,
		&toss_err, err_blk, 2,
		0, "RECONSTRUCT",
		0, ERx("a WITH clause (except LOCATION)"));
	    return (E_DB_ERROR);
	}
    }

    tblinfo = sess_cb->pss_resrng->pss_tabdesc;

    /* Disallow RTREE because there's no DMF support for it.
    ** (if we do get DMF support, we'll want to copy the tblinfo->tbl_range
    ** into dmu_range;  see buildPersistentIndexDMU_CB in qeus.c)
    */
    if (tblinfo->tbl_storage_type == DB_RTRE_STORE)
    {
	(void) psf_error(E_US1947_6465_X_NOTWITH_Y, 0, PSF_USERERR,
		&toss_err, err_blk, 3,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, "RECONSTRUCT",
		0, "an RTREE index");
	return (E_DB_ERROR);
    }

    /* Start by generating key info.  Since MODIFY does not
    ** allow the WITH KEY=() option, no need to interface with WITH-options,
    ** just generate the DMU style key entries.
    */
    keybase = sess_cb->pss_resrng->pss_rdrinfo->rdr_keys;
    if (keybase != NULL && keybase->key_count > 0)
    {
	keycount = keybase->key_count;
	keycol_ptr = keybase->key_array;
	table_atts = sess_cb->pss_resrng->pss_rdrinfo->rdr_attr;
	if (tblinfo->tbl_status_mask & DMT_IDX && keycount > 1)
	{
	    /* Index: might need to back off key count by one if the last
	    ** key is tidp.  (tidp is not always part of the key so don't
	    ** just do this blindly.)  DMF is used to including tidp as
	    ** an add-on, if we include it here DMF complains.
	    */
	    if (cui_compare( (table_atts[keycol_ptr[keycount-1]])->att_nmlen,
		(table_atts[keycol_ptr[keycount-1]])->att_nmstr,
		4, ((*sess_cb->pss_dbxlate & CUI_ID_REG_U) ? ERx("TIDP") : 
			ERx("tidp"))) == 0)
		-- keycount;
	}
	/* We need one DMU_KEY_ENTRY for each key.  The pointers are
	** already allocated to the max possible number of key columns.
	*/
	psize = sizeof(DMU_KEY_ENTRY) * keycount;
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, psize,
		&key_array, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
	keyptr_ptr = (DMU_KEY_ENTRY **) dmucb->dmu_key_array.ptr_address;
	do
	{
	    /* Fill in this entry with name and sort direction for column */
	    cui_move((table_atts[*keycol_ptr])->att_nmlen,
		(table_atts[*keycol_ptr])->att_nmstr, ' ',
		sizeof(DB_ATT_NAME), key_array->key_attr_name.db_att_name);
	    key_array->key_order = DMU_ASCENDING;
	    if ( (table_atts[*keycol_ptr])->att_flags & ATT_DESCENDING)
		key_array->key_order = DMU_DESCENDING;
	    *keyptr_ptr++ = key_array++;
	    ++ dmucb->dmu_key_array.ptr_in_count;
	    ++ keycol_ptr;
	} while (dmucb->dmu_key_array.ptr_in_count < keycount);
    }

    /* Add additional characteristics if they weren't overridden in the
    ** query.  This is relatively unexciting.
    */

    if (! BTtest(PSS_WC_COMPRESSION, dmucb->dmu_chars.dmu_indicators))
    {
	if (tblinfo->tbl_status_mask & DMT_INDEX_COMP)
	    dmucb->dmu_chars.dmu_kcompress = DMU_COMP_ON;
	if (tblinfo->tbl_status_mask & DMT_COMPRESSED)
	{
	    dmucb->dmu_chars.dmu_dcompress = DMU_COMP_ON;
	    if (tblinfo->tbl_comp_type == DMT_C_HICOMPRESS)
		dmucb->dmu_chars.dmu_dcompress = DMU_COMP_HI;
	}
	BTset(PSS_WC_COMPRESSION, dmucb->dmu_chars.dmu_indicators);
	BTset(DMU_DCOMPRESSION, dmucb->dmu_chars.dmu_indicators);
	BTset(DMU_KCOMPRESSION, dmucb->dmu_chars.dmu_indicators);
    }

    if (! BTtest(DMU_STATEMENT_LEVEL_UNIQUE, dmucb->dmu_chars.dmu_indicators))
    {
	if (tblinfo->tbl_2_status_mask & DMT_STATEMENT_LEVEL_UNIQUE)
	{
	    dmucb->dmu_chars.dmu_flags |= DMU_FLAG_UNIQUE_STMT;
	    BTset(DMU_STATEMENT_LEVEL_UNIQUE, dmucb->dmu_chars.dmu_indicators);
	}
    }

    /* Fillfactor for non-heap */
    if (tblinfo->tbl_storage_type != DB_HEAP_STORE)
    {
	if (! BTtest(DMU_DATAFILL, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_fillfac = tblinfo->tbl_d_fill_factor;
	    BTset(DMU_DATAFILL, dmucb->dmu_chars.dmu_indicators);
	}
    }

    /* Leaffill and/or nonleaffill for btree or rtree */
    if (tblinfo->tbl_storage_type == DB_BTRE_STORE)
    {
	if (! BTtest(DMU_IFILL, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_nonleaff = tblinfo->tbl_i_fill_factor;
	    BTset(DMU_IFILL, dmucb->dmu_chars.dmu_indicators);
	}

	if (! BTtest(DMU_LEAFFILL, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_leaff = tblinfo->tbl_l_fill_factor;
	    BTset(DMU_LEAFFILL, dmucb->dmu_chars.dmu_indicators);
	}
    }

    /* Minpages and/or maxpages for hash */
    if (tblinfo->tbl_storage_type == DB_HASH_STORE)
    {
	if (! BTtest(DMU_MINPAGES, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_minpgs = tblinfo->tbl_min_page;
	    BTset(DMU_MINPAGES, dmucb->dmu_chars.dmu_indicators);
	}

	if (! BTtest(DMU_MAXPAGES, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_maxpgs = tblinfo->tbl_max_page;
	    BTset(DMU_MAXPAGES, dmucb->dmu_chars.dmu_indicators);
	}
    }

    /* Persistence for indexes */
    if (tblinfo->tbl_status_mask & DMT_IDX
      && tblinfo->tbl_2_status_mask & DMT_PERSISTS_OVER_MODIFIES)
    {
	if (! BTtest(DMU_PERSISTS_OVER_MODIFIES, dmucb->dmu_chars.dmu_indicators))
	{
	    dmucb->dmu_chars.dmu_flags |= DMU_FLAG_PERSISTENCE;
	    BTset(DMU_PERSISTS_OVER_MODIFIES, dmucb->dmu_chars.dmu_indicators);
	}
    }

    return (E_DB_OK);
} /* psl_md_reconstruct */

/* Name: psl_md_part_unique - Check uniqueness / partitioning compatibility
**
** Description:
**	If the user is modifying something to a UNIQUE storage structure,
**	this routine is called to check for any conflicts with partitioning.
**
**	A partitioned UNIQUE structure requires "compatibility" between
**	the partitioning scheme and the storage structure, in the sense
**	that all rows with any given storage-structure key must be
**	located in the same partition.  If this condition holds, then
**	uniqueness can be checked by DMF in the same way it is now.  (If
**	the compatibility condition were not true, the only way to check
**	uniqueness would be to probe additional partitions, perhaps all
**	of them.  Not only would this be slow, there's no machinery to
**	run any such probing.)
**
**	Luckily for us, there's a compatibility checker around already;
**	it's one of the partition-def functions in RDF.  So all we need
**	to do is call it, using the (new) key list and (new or old)
**	partition definition.
**
**	Some special cases and notes:
**	- If this is a repartitioning modify, and there's no new partition
**	  definition, it must be a modify WITH NOPARTITION and there's
**	  no need to check anything.
**	- if this is RECONSTRUCT, and it's not a repartitioning modify,
**	  we can skip the check.  (Things were checked out when the storage
**	  structure was originally imposed.)
**	- There's no need to worry about partial-table (logical partition)
**	  modifies, since they necessarily fall into the previous case
**	  (a non-repartitioning reconstruct).  Someday, if we ever allow
**	  disparate storage structures on partitions, this will need to change.
**	- Repartitioning modifies need to check against the new partition
**	  definition, of course.  Non-repartitioning modifies need to
**	  check against the existing partition definition;  if the table
**	  wasn't partitioned, there's nothing to do.
**
**	As a side effect, the STRUCT_COMPAT flag(s) are set or cleared
**	in the partition definition.  This is not something that needs to
**	be done anyway:  after the modify, this (new) copy of the partition
**	definition will be flushed, and the flag will be recalculated the
**	next time someone reads the definition from the partitioning catalogs. 
**	The flag is not stored permanently in the catalogs.
**
** Inputs:
**	[call for UNIQUE restructuring modifies]
**	sess_cb			PSS_SESBLK parser session control block
**	dmucb			The constructed DMU_CB for the modify
**	yyvarsp			Parser state for this query
**	err_blk			An output
**
** Outputs:
**	Returns E_DB_xxx status
**	err_blk			Error info returned here if error
**
** History:
**	5-Aug-2004 (schka24)
**	    I forgot to write this restriction with the initial modify
**	    partitioning support, better late than never.
**	6-Jul-2006 (kschendel)
**	    Fill in unique dbid for RDF, just in case.
*/

static DB_STATUS
psl_md_part_unique(PSS_SESBLK *sess_cb, DMU_CB *dmucb,
	PSS_YYVARS *yyvarsp, DB_ERROR *err_blk)
{
    char *noise;
    DB_PART_DEF *part_def;		/* Partition definition pointer */
    DB_STATUS status;			/* Usual status thing */
    DMT_ATT_ENTRY *attribute;		/* Column info */
    DMU_KEY_ENTRY **keyptr_ptr;		/* Pointer to DMU key pointer array */
    i4 i;
    i4 key_attno[DB_MAXKEYS];		/* Attribute numbers of keys */
    i4 *keyarray_ptr;			/* Points to key att number array */
    i4 toss_err;
    RDD_KEY_ARRAY key_header;		/* Key info header for RDF */
    RDF_CB rdfcb;			/* RDF request block */
    STATUS stat;			/* CL style status thing */

    /* Decide whether to look at existing partitioning, or new
    ** (re)-partitioning.
    */
    if (BTtest(PSS_WC_PARTITION, dmucb->dmu_chars.dmu_indicators))
    {
	/* We have either WITH PARTITION= or WITH NOPARTITION */
	part_def = dmucb->dmu_part_def;
	if (part_def == NULL)
	    return (E_DB_OK);		/* It's NOPARTITION, nothing to do */
	noise = "new";
    }
    else
    {
	/* Perhaps existing table is partitioned.  Or not. */
	if ((sess_cb->pss_resrng->pss_tabdesc->tbl_status_mask & DMT_IS_PARTITIONED) == 0)
	    return (E_DB_OK);		/* Not partitioned, get out */
	/* Perhaps it's RECONSTRUCT, and we know the existing scheme is OK */
	if (yyvarsp->md_reconstruct)
	    return (E_DB_OK);
	part_def = sess_cb->pss_resrng->pss_rdrinfo->rdr_parts;
	noise = "existing";
    }

    /* Ask the partitioning demons whether the combination of the (new)
    ** modify keys and the (new or old) partition definition is OK.
    */

    /* First we have to prepare a key list in attribute-number form instead
    ** of attribute-name form.  This is a PITA, and I wonder if DMU_KEY_ENTRY
    ** shouldn't just contain att numbers instead of names.  A side
    ** investigation for the future.
    */
    key_header.key_count = dmucb->dmu_key_array.ptr_in_count;
    /* Avoid the MEreqmem if possible.  <, not <=, because RDF wants one
    ** extra entry in the array.
    */
    if (key_header.key_count < DB_MAXKEYS)
    {
	keyarray_ptr = key_header.key_array = &key_attno[0];
    }
    else
    {
	keyarray_ptr = key_header.key_array = (i4 *) MEreqmem(
			0, sizeof(i4) * (key_header.key_count + 1),
			FALSE, &stat);
	if (keyarray_ptr == NULL || stat != OK)
	{
	    (void) psf_error(E_PS0F02_MEMORY_FULL, 0, PSF_INTERR,
			&toss_err, err_blk, 0);
	    return (E_DB_ERROR);
	}
    }
    keyptr_ptr = (DMU_KEY_ENTRY **) dmucb->dmu_key_array.ptr_address;
    for (i = 0; i < key_header.key_count; ++i)
    {
	/* Lookup attribute name for this key, must exist */

	attribute = pst_coldesc(sess_cb->pss_resrng, 
		    (*keyptr_ptr)->key_attr_name.db_att_name, DB_ATT_MAXNAME);
	*keyarray_ptr++ = attribute->att_number;
	++ keyptr_ptr;
    }
    *keyarray_ptr = 0;			/* Terminate in case RDF likes it so */

    /* Fill in the RDF request block stuff */

    MEfill(sizeof(rdfcb), 0, &rdfcb);	/* Don't confuse RDF with junk */
    rdfcb.rdf_rb.rdr_db_id = sess_cb->pss_dbid;
    rdfcb.rdf_rb.rdr_unique_dbid = sess_cb->pss_udbid;
    rdfcb.rdf_rb.rdr_fcb = Psf_srvblk->psf_rdfid;
    rdfcb.rdf_rb.rdr_session_id = sess_cb->pss_sessid;
    rdfcb.rdf_rb.rdr_newpart = part_def;
    rdfcb.rdf_rb.rdr_storage_type = dmucb->dmu_chars.dmu_struct;
    rdfcb.rdf_rb.rdr_keys = &key_header;
    status = rdf_call(RDF_PART_COMPAT, &rdfcb);
    /* Release memory if we allocated,  *Don't hurt "status" */
    if (key_header.key_array != &key_attno[0])
	(void) MEfree((PTR) key_header.key_array);
    if (DB_FAILURE_MACRO(status))
    {
	/* Really shouldn't fail, hand error back to caller */
	return (status);
    }

    /* Check that the "key in one partition" flag is set.  If it's not, we
    ** have to disallow uniqueness.
    */
    if ((part_def->part_flags & DB_PARTF_KEY_ONE_PART) == 0)
    {
	(void) psf_error(E_US1948_6466_UNIQUE_PART_INCOMPAT, 0, PSF_USERERR,
		&toss_err, err_blk, 2,
		yyvarsp->qry_len, yyvarsp->qry_name,
		0, noise);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
} /* psl_md_part_unique */

/*
** Name: psl_md_action_string -- Generate MODIFY action string
**
** Description:
**	The work to unify WITH-parsing has also led to more generic
**	error messages.  For instance, we might have:
**	   %0c: %1c is not a valid WITH-option for this statement
**	While there's nothing wrong with that per se, in a MODIFY
**	context it can be unclear.  For instance:
**	   MODIFY: LOCATION is not a valid WITH-option...
**	may be baffling, because it certainly is a valid option in
**	a general sense.  But if we add more MODIFY context, it's
**	better:
**	   MODIFY (to RELOCATE): LOCATION is not a valid ...
**
**	In order to generate the addendum, it makes sense to work
**	from the same table that parses the MODIFY action in the
**	first place, which is why this routine exists here.
**
**	We're called if psl_command_string detects that it's a
**	MODIFY statement.  The string "MODIFY" is already in the
**	result buffer;  we'll append to it if we have a modify action.
**
** Inputs:
**	sess_cb			PSS_SESBLK parser session CB
**	str			Where to put result
**	length			Pointer to length-so-far
**
** Outputs:
**	str, length		Updated with new string, string length
**
** History:
**	15-Oct-2010 (kschendel) SIR 124544
**	    Consolidated WITH-parsing needs better messages.
*/

void
psl_md_action_string(PSS_SESBLK *sess_cb, char *str, i4 *length)
{
    char *name;
    enum dmu_action_enum action;
    DMU_CB *dmucb;
    PSS_YYVARS *yyvars = sess_cb->pss_yyvars;
    QEU_CB *qeucb;
    const struct _MODIFY_STORNAMES *stornames_ptr;

    action = yyvars->md_action;
    if (action == DMU_ACT_NONE)
	return;

    name = NULL;
    qeucb = (QEU_CB *) sess_cb->pss_object;
    dmucb = (DMU_CB *) qeucb->qeu_d_cb;
    if (qeucb->qeu_d_op == DMU_RELOCATE_TABLE)
	name = "relocate";
    else if (action == DMU_ACT_STORAGE)
    {
	i4 sstruct = dmucb->dmu_chars.dmu_struct;

	if (yyvars->md_reconstruct)
	    name = "reconstruct";
	else if (sstruct == DB_HEAP_STORE && yyvars->is_heapsort)
	    name = "heapsort";
	else
	    name = uld_struct_name(sstruct);
    }
    else if (action == DMU_ACT_PRIORITY)
	name = "priority";		/* Not in the stornames table */
    else if (action == DMU_ACT_USCOPE)
	name = "unique_scope";		/* Not in the stornames table */
    else
    {
	/* Look up action in the "stornames" table. */
	for (stornames_ptr = &modify_stornames[0];
	     stornames_ptr->name_string != NULL;
	     ++ stornames_ptr)
	{
	    if (action == stornames_ptr->action)
	    {
		/* Might be the right one;  match up indicator and value too */
		if (stornames_ptr->indicator == -1)
		{
		    name = stornames_ptr->name_string;
		    break;
		}
		switch (stornames_ptr->indicator)
		{
		/* Don't need STRUCTURE case, just the other 3 */
		  case DMU_ACTION_ONOFF:
		    if ((dmucb->dmu_chars.dmu_flags & DMU_FLAG_ACTON) == stornames_ptr->value)
			name = stornames_ptr->name_string;
		    break;
		  case DMU_ACT_PERSISTENCE:
		    if ((dmucb->dmu_chars.dmu_flags & DMU_FLAG_PERSISTENCE) == stornames_ptr->value)
			name = stornames_ptr->name_string;
		    break;
		  case DMU_ACT_VERIFY:
		    if (dmucb->dmu_chars.dmu_vaction == stornames_ptr->value)
			name = stornames_ptr->name_string;
		    break;
		}
		if (name != NULL)
		    break;
	    }
	}
    }

    if (name != NULL)
    {
	/* MODIFY + "(to " + name + ")" + trailing null */
	if (STlength(str) + 4 + STlength(name) + 1 + 1 > PSL_MAX_COMM_STRING)
	    return;		/* arrgh */
	STcat(str, "(to ");
	STcat(str, name);
	STcat(str, ")");
	*length = STlength(str);
    }
} /* psl_md_action_string */
