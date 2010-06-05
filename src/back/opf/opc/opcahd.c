/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <cs.h>
#include   <cm.h>
#include    <lk.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmhcb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <dmucb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefmain.h>
#include    <qeuqcb.h>
#include    <qefqeu.h>
#include    <psfindep.h>
#include    <me.h>
#include    <st.h>
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
#include    <opckey.h>
#include    <opcd.h>
#define        OPC_AHD		TRUE
#include    <opxlint.h>
#include    <opclint.h>

/*
** Dick Williamson guess at optimal tuples per buffer for opc_pagesize()
*/
#define		OPC_PAGESIZE_DW_ROWS	20

/**
**
**  Name: OPCAHD.C - Routines to fill in QEF_AHD structs
**
**  Description:
**      Opcahd.c contains routines that will fill in QEF_AHD structs. When it 
**      comes to filling in the QEN_ADF and QEN_NODE structs that belong to a
**      QEF_AHD then opc_adf_build() and opc_qnode_build() will be called.
**
**          opc_ahd_build() - Entry point for building QEF_AHD structs
[@func_list@]...
**
**
**  History:
**      31-jul-86 (eric)
**          created
**	02-sep-88 (puree)
**	    rename qen_adf to qen_ade_cx
**      07-sep-88 (thurston)
**          Fixed a memory smashing bug in opc_dvahd_build().
**	08-sep-88 (puree)
**	    Fixed allocating memory of zero byts in opc_dvahd_build().
**      13-dec-88 (seputis)
**          lint changes
**	30-jan-89 (paul)
**	    Enhanced to compile rule statement lists. Updated opc_qepahd to
**	    check for rules statements and save the state information required
**	    to compile them.  Added the opc_addrules routine.
**	    Added compilation routines opc_emsgahd_build and opc_cpahd_build
**	24-mar-89 (paul)
**	    Corrected logic for compiling RAISE ERROR statements.
**	30-mar-89 (paul)
**	    Add support for compiling query cost estimates into action headers.
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	18-apr-89 (paul)
**	    Add support for delete cursor actions (previously a special case)
**	09-may-89 (neil)
**	    Assign pst_rulename for callproc to support rule tracing.
**	26-jun-89 (jrb)
**	    Fixed bug where too little memory was being allocated for an array
**	    of pointers to attribute descriptors in opc_dmuahd and opc_dmtahd.
**	05-sep-89 (neil)
**	    Alerters: Added routine opc_eventahd_build.
**	22-sep-89 (paul)
**	    Fixed bug 8054: Allow TIDs to be used as parameters to procedures
**	    called from rules. This requires recognizing old.tid and new.tid.
**      1-nov-89 (eric)
**          Added support for the new field ahd_ruprow for bug 8388. 
**          Opc_qepahd(), opc_rupahd(), and opc_rdelahd() changed for this.
**          The idea is that ahd_ruprow is in the define cursor query plan
**          (for an updateable or deleteable cursor). Ahd_ruprow tells where
**          the row buffer is to use to update the row. Normally, qen_output
**          in ahd_current tells this, but this is not appropriate for the
**          define cursor. Also, ahd_tidrow and ahd_tidoffset are set to
**          indicate whether a positioned or bytid cursor update or delete
**          should happen.
**	26-mar-90 (stec)
**	    Put an ifdef flag around opc_chktarget() forward reference,
**	18-apr-90 (stec)
**	    Fix bug 20968.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	27-jun-90 (stec)
**	    Fixed bug 31113 in opc_qepahd().
**	29-jun-90 (stec)
**	    Fixed bug 31092 in opc_qepahd().
**	13-jul-90 (stec)
**	    Modified opc_qepahd(). Changed pagestouched (local var) from i4
**	    to i4. Modified code to prevent floating point and integer 
**	    overflow when pagestouched is calculated.
**      16-oct-90 (jennifer)
**          Add PST_HIDDEN to opc_chktarget routine.
**          Also, changed check for pst_print (able) so PST_HIDDEN checked first in 
**          opc_dmuahd.
**	06-nov-90 (stec)
**	    Modified opc_rupahd(), opc_rdelahd() to return correct error
**	    code in case of QSO_INFO call failure.
**	04-dec-90 (stec)
**	    Modified code in opc_qepahd() to fix bug 34217.
**      05-dec-90 (ralph)
**          Copy rule owner name into ahd_ruleowner (b34531)
**	10-dec-90 (neil)
**	    Alerters: Added routine opc_eventahd_build() and
**	    commented opc_addrules().
**	10-jan-91 (stec)
**	    Changed code in opc_qepahd().
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization, modified IF stmt
**	    checking for datatype equality in opc_rtnahd_build(),
**	    opc_msgahd_build().
**	04-feb-91 (stec)
**	    Added missing opr_prec initialization in opc_eventahd_build().
**	27-feb-91 (ralph)
**	    Copy message destination flags from PST_MSGSTMT.pst_msgdest
**	    to QEF_MESSAGE.ahd_mdest.
**	14-mar-91 (stec)
**	    Modified opc_qepahd().
**	25-mar-91 (bryanp)
**	    Added support for Btree index compression. opc_ahd_build now treats
**	    the pst_compress field in the PST_RESTAB as a bit word, with bits
**	    for whether data and/or index compression is in use, and passes
**	    this information down to opc_dmuahd so that it can build the
**	    appropriate DMU control blocks for DMF.
**	16-apr-91 (davebf)
**	    Added parm to opc_valid() call in order to enable vl_size_sensitive
**	01-jul-91 (nancy)
**	    Fixed missing end comment in opc_qepahd().
**	12-feb-92 (nancy)
**	    Bug 39225.  Modified opc_rtnahd_build() to create cx to initialize
**	    local dbp vars in the same way other statements do.  
**	30-March-1992 (rmuth)
**	    File extend project. Added DMU_ALLOCATION and DMU_EXTEND to
**	    opc_dmuahd().
**	31-mar-1992 (bryanp)
**	    Temporary table enhancements.
**	04-aug-92 (anitap)
**	    Commented out xDEV_TEST after #endif as HP compiler was 
**	    complaining.
**	16-jun-92 (anitap)
**	    Cast naked NULLs being passed into procedures in opc_rupahd(), 
**	    opc_ahd_build(), opc_qepahd(), opc_eventahd_build(), 
**	    opc_ifahd_build(), opc_rtnahd_build(), opc_msgahd_build(), 
**	    opc_cpahd_build().
**	26-oct-92 (jhahn)
**	    Added support for new actions needed for statement level rules.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	17-nov-92 (rickh)
**	    Added opc_createTableAHD, opc_createIntegrityAHD.
**	30-nov-92 (anitap)
**	    Added new routines opc_schemaahd_build(), opc_copy_schema(), 
**	    opc_exeimmahd_build() and opc_copy_info().
**	14-dec-92 (jhahn)
**	    Did some cleanup for  handling of nested procedure calls.
**	20-dec-92 (anitap)
**	    Fixed bugs in opc_schemaahd_build(), opc_copy_info(), 
**	    opc_copy_schema(), opc_exeimmahd_build().
**	23-dec-92 (andre)
**	    added opc_crt_view_ahd()
**      04-Jan-1993 (fred)
**          Added handling to opc_dmuahd() for correct configuration
**          of large objects during "create as select" type queries.
**	20-jan-93 (anitap)
**	    Fixed compiler warnings in opc_exeimmahd_build(), 
**	    copyIntegrityDetails(), allocateAndCopyColumnList(), 
**	    opc_rprocahd_build(). Fixed bug. Removed routine opc_copy_schema().
**	30-jan-93 (rickh)
**	    Added more persistent objects to the DSH of createIntegrity
**	    statements.  Also added checkRuleText to check constraints.
**	02-feb-93 (anitap)
**	    Fixed size in MEcopy() and removed unnecessary declaration of 
**	    DB_TEXT_STRING in opc_copy_info().
**	12-feb-93 (jhahn)
**	    Added support for statement level rules.
**	22-feb-93 (rickh)
**	    Make sure we copy the default tuple if provided.
**      22-feb-1993 (rblumer)
**          Change opc_dmuahd to get the default value for attribute entries
**          from the new pst_defid field of the resdom in the query tree,
**          rather than using the nullability of the datatype and the query
**          language. 
**	01-mar-93 (rickh)
**	    Copy the schema name out of PSF's CREATE_INTEGRITY structure into
**	    QEF's.
**	04-mar-93 (anitap)
**	    Changed VOID to void for CREATE SCHEMA routines and changed the 
**	    names to correspond to coding standards.
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	2-apr-93 (rickh)
**	    Add integrity number to DSH of CREATE INTEGRITY statements.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	17-may-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	28-may-93 (jhahn)
**	    Fixed problem with multiple QEA_INVOKE_RULE actions being generated.
**	    Added code to bump qp_ahd_cnt in some actions where it had been
**	    missed.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (anitap)
**	    Changed opc_schema to be a pointer in opc_shemaahd_build().
**	    Account for the case when PST_INFO is NULL in opc_copy_info().
**      09-jul-93 (jhahn)
**          Added back in change from 27-jun-90 that had gotten lost duing
**	    Outer Join rototilling.
**	9-aug-93 (rickh)
**	    CREATE_INTEGRITY actions now need to compile in a flag indicating
**	    whether a REFERENTIAL CONSTRAINT should be verified at the time
**	    it's added.  Also added a new field to the CREATE_INTEGRITY
**	    action:  initialState of the DDL state machines.
**	17-aug-93 (anitap)
**	    Typecast the values in the conditional tests involving dissimilar
**	    types like float and MAXI4. The lack of these casts caused by
**	    floating point exceptions on a certain machine in opc_qepahd().
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID. Added casts to rdf_call.
**	26-aug-93 (rickh)
**	    When compiling a CREATE TABLE AS SELECT statement, flag any NOT
**	    NULL actions to remember the rowcount of the SELECT action.
**      13-aug-93 (rblumer)
**          The DMT_F_NDEFAULT flag was being overwritten in opc_dmuahd;
**          change other bits to use |= so as not to overwrite existing bits.
**	    This fixes 'internal error' in bug b54185.
**      5-oct-93 (eric)
**          Added support for table in resource lists.
**	17-jan-94 (rickh)
**	    Fixed some casts.  Also added a field to the CREATE INTEGRITY
**	    header:  modifyRefingProc was changed to insertRefingProc and
**	    updateRefingProc.
**	26-nov-93 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to qeu_owner whose type has
**	    changed.
**	18-apr-94 (rblumer)
**	    changed pst_integrityTuple2 to pst_knownNotNullableTuple, and
**	    changed qci_integrityTuple2 to qci_knownNotNullableTuple.
**	07-oct-94 (mikem)
**	    Changed root node initialization so that duplicate key/row 
**	    errors will be ignored if in SQL and the PST_4IGNORE_DUPERR bit 
**	    is asserted in pst_mask1.  This bit is asserted if the ps201
**	    tracepoint has been executed in this session.  This traceflag
**	    will be used by the CA-MASTERPIECE product so that duplicate
**	    key errors will not shut down all open cursors (whenever the
**	    "keep cursors open on errors" project is integrated this change
**	    will no longer be necessary).
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	05-jan-97 (pchang)
**	    In opc_dmuahd(), when filling in fake RDF entries for result 
**	    attributes, a failure to skip non-printing attributes caused OPF 
**	    memory overrun. (B80560)
**	26-june-1997(angusm)
**	    bugs 79623, 80750, 77040: flag up cross-table update for
**	    special handling in DMF.
**	07-jan-1997 (canor01)
**	    If a page size has been specified, add a DMU_PAGE_SIZE entry.
**      13-jun-97 (dilma04)
**          When building CALL_DEFERRED_PROC ahd, set QEN_CONSTRAINT flag
**          in QEN_ADF if procedure supports integrity constraint.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      08-jul-99 (gygro01)
**	    Fix for bug 96955/pb ingsrv 859. Restricted AHD_NOROWS support.
**	    If repeated query with parameter get AHD_NOROWS flagged during
**	    optimization, all the following execution of the query even with
**	    correct parameters, will get no rows back (incorrect result).
**	    This is because repeated queries get only compiled once.
**	    This problem doesn't occurs for procedures or cursors.
**      12-may-2000 (stial01)
**          retrieve into with page_size, put in modify dmu_cb (SIR 98092)
**      12-oct-2000 (stial01)
**          opc_pagesize() Fixed number of rows per page calculation
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	12-Dec-2005 (kschendel)
**	    Make QEN_ADF handling a bit more generic, move all remaining
**	    inline structs to pointers.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**      12-aug-2009 (stial01)
**          Moved KEYSET initialization before Resultsetcol is init (B122462)
**	19-Apr-2007 (kschendel) b122118
**	    Delete unused adbase parameters and ahd-key.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*
**  Forward and/or External function references.
*/

static VOID
opc_build_call_deferred_proc_ahd(
    OPS_STATE	*global,
    QEF_AHD	*proc_insert_ahd);

static QEF_AHD *	
opc_qepahd(
	OPS_STATE   *global,
	QEF_AHD	    **proj_ahd );

static void
opc_keyset_build(
	OPS_STATE   *global,
	QEF_AHD	    *ahd,
	OPC_NODE    *cnode);

static OPO_CO *
opc_keyset_co(
	OPO_CO	*cop,
	OPS_SUBQUERY *subquery,
	i4	resvno,
	i4	*rowsz);

static VOID
opc_saggopt(
        OPS_SUBQUERY *sq,
	QEF_AHD      *ahd,
	PST_QNODE    *root);

static QEF_AHD *	
opc_dmuahd(
	OPS_STATE   *global,
	i4	    dmu_func,
	PST_QTREE   *qtree,
	OPV_IGVARS  gresvno,
	i4	    structure,
	i4	    compressed,
	i4	    index_compressed );

static QEF_AHD *	
opc_dmtahd(
	OPS_STATE   *global,
	PST_QNODE   *root,
	i4	    flags_mask,
	OPV_IGVARS  gresvno );

static QEF_AHD *	
opc_rupahd(
	OPS_STATE   *global );

static QEF_AHD *	
opc_rdelahd(
	OPS_STATE   *global );

static i4
opc_cposition(
	OPS_STATE	*global,
	OPO_CO		*co,
	OPV_IVARS	lresno);

static i4
opc_btmleft_dmrcbno(
	OPS_STATE   *global,
	QEN_NODE    * qen);

static VOID
opc_addrules(
	OPS_STATE   *global,
	QEF_AHD	    *ahd,
	RDR_INFO    *rel );

static void
opc_gtrowno(
	OPS_STATE	*global,
	QEF_QEP		*qhd,
	QEN_NODE	*qn,
	QEF_VALID	*valid );

static VOID
opc_gtt_proc_parm(
    OPS_STATE	*global,
    QEF_AHD	*ahd,
    PST_TAB_NODE *tabnode);

static void 
opc_copy_info(
	OPS_STATE	       *global,
        PST_INFO               *pst_info,
        PST_INFO               **target_info);

static VOID
copyIntegrityDetails(
		OPS_STATE			*global,
		PST_CREATE_INTEGRITY		*psfDetails,
		QEF_CREATE_INTEGRITY_STATEMENT	*qefDetails );

static VOID
allocateAndCopyStruct(
		OPS_STATE	*global,
		PTR		source,
		PTR		*target,
		i4		size );

static VOID
allocateAndCopyColumnList(
		OPS_STATE	*global,
		PST_COL_ID	*source,
		PST_COL_ID	**target );

static VOID
opc_alloc_and_copy_QEUQ_CB(
	OPS_STATE       *global,
	QEUQ_CB		*source,
	QEUQ_CB         **target,
	i4		action);

static VOID
allocate_and_copy_QEUCB(
		OPS_STATE	*global,
		QEU_CB		*source,
		QEU_CB		**target );

static VOID
allocate_and_copy_DMUCB(
		OPS_STATE	*global,
		DMU_CB		*source,
		DMU_CB		**target );

static VOID
allocate_and_copy_a_dm_data(
		OPS_STATE	*global,
		DM_DATA		*source,
		DM_DATA		*target );
static VOID
allocate_and_copy_a_dm_ptr(
		OPS_STATE	*global,
		i4		flags,
		DM_PTR		*source,
		DM_PTR		*target );
static VOID
opc_updcolmap(
		OPS_STATE	*global,
		QEF_QEP		*upd_ahd,
		PST_QNODE	*setlist,
		DB_PART_DEF	*partp);

/* Full partition copy including all value text, partition names */
static VOID opc_full_partdef_copy(
	OPS_STATE	*global,
	DB_PART_DEF	*pdefp,
	i4		pdefsize,
	DB_PART_DEF	**outppp);

static DB_STATUS opc_qsfmalloc(
	OPS_STATE *global,
	i4 psize,
	void *pptr);

static void opc_loadopt(
	OPS_STATE *global,
	QEF_AHD *ahd,
	QEF_VALID *valid);

#ifdef xDEV_TEST
static VOID
opc_chktarget(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root );
#endif /* xDEV_TEST */

/*
**  Defines of other constants.
*/


/*{
** Name: OPC_AHD_BUILD	- Entry point for building the QEF_AHD structs for a
**			    OPS_SUBQUERY struct.
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
**      31-jul-86 (eric)
**          written
**	28-sep-87 (rogerk)
**	    Change sequencing for RETRIEVE INTO.  Instead of creating
**	    HEAP table, doing PUTS and then modifying to the desired
**	    structure - create and modify table, then use LOAD to
**	    fill up the table (Except hash case - load HEAP and then
**	    modify).
**      20-Dec-87 (eric)
**          Use QEA_APP action for RETIEVE INTO if journaled, use QEA_LOAD
**	    otherwise. We cannot use QEA_LOAD if the relation is being journaled
**	    because QEA_LOAD does no logging.
**	18-apr-89 (paul)
**	    Add support for delete cursor actions (previously a special case)
**	25-mar-1991 (bryanp)
**	    Added support for Btree index compression. opc_ahd_build now treats
**	    the pst_compress field in the PST_RESTAB as a bit word, with bits
**	    for whether data and/or index compression is in use, and passes
**	    this information down to opc_dmuahd so that it can build the
**	    appropriate DMU control blocks for DMF.
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	16-jun-92 (anitap)
**	    Cast naked NULL being passed into ule_format().	
**	07-jan-1997 (canor01)
**	    If a page size has been specified, add a DMU_PAGE_SIZE entry.
**	11-Feb-2004 (schka24)
**	    amazing, the default RETRIEVE INTO has always done an extra
**	    modify if journalling.  Modify before, or after, but not both.
**      28-apr-2004 (stial01)
**          Optimizer changes for 256K rows, rows spanning pages
**	17-Nov-2009 (kschendel) SIR 122890
**	    pst-resjour expanded, reflect here.
*/
VOID
opc_ahd_build(
		OPS_STATE   *global,
		QEF_AHD	    **pahd,
		QEF_AHD	    **proj_ahd)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd = NULL;
    QEF_AHD		*ahd_list = NULL;
    QEF_VALID		**retinto_alt = NULL;
    i4		compressed;
    i4		index_compressed;
    i4		storage_type;
    OPV_GRV		*grv;
    i4			journaled;
    DMU_CB              *mod_dmucb;
    bool	modify_pre_load;	/* MODIFY before load, not after */

    global->ops_cstate.opc_relation = NULL;

    /* if (this is a replace cursor) */
    if (subqry->ops_sqtype == OPS_MAIN && 
	    global->ops_qheader->pst_mode == PSQ_REPCURS
	)
    {
	/* make the action to replace the cursor; */
	ahd_list = opc_rupahd(global);
    }
    else if (subqry->ops_sqtype == OPS_MAIN &&
	     global->ops_qheader->pst_mode == PSQ_DELCURS)
    {
	ahd_list = opc_rdelahd(global);
    }
    else
    {
	/* if (this is a ret into) */
	if (subqry->ops_sqtype == OPS_MAIN && 
		(global->ops_qheader->pst_mode == PSQ_RETINTO ||
		 global->ops_qheader->pst_mode == PSQ_DGTT_AS_SELECT)
	    )
	{
	    /* make an action to create the relation */
	    ahd_list = 
		    opc_dmuahd(global, DMU_CREATE_TABLE, global->ops_qheader, 
			subqry->ops_result, (i4)DB_HEAP_STORE, DMU_C_OFF,
			DMU_C_OFF);

	    index_compressed = DMU_C_OFF;

	    /*
	    ** Modify the table to the desired storage structure if that
	    ** structure is not HEAP (the table is created as a heap) or
	    ** HASH/CHASH (we modify to hash after load).
	    */
	    journaled = (global->ops_qheader->pst_restab.pst_resjour == PST_RESJOUR_ON);
	    if (global->ops_qheader->pst_restab.pst_struct == 0)
	    {
		storage_type = global->ops_cb->ops_alter.ops_storage;
		if (global->ops_cb->ops_alter.ops_compressed == TRUE)
		{
		    compressed = DMU_C_ON;
		}
		else
		{
		    compressed = DMU_C_OFF;
		}
	    }
	    else
	    {
		storage_type = global->ops_qheader->pst_restab.pst_struct;
		if (global->ops_qheader->pst_restab.pst_compress &
				    (PST_NO_COMPRESSION | PST_COMP_DEFERRED))
		{
		    i4 err_code;
		    /*
		    ** This indicates an error in the parser -- it passed
		    ** some junk to us which it should have handled itself.
		    ** Handle this error by logging a complaint, then just
		    ** using no compression.
		    */
		    ule_format(E_OP0AA0_BAD_COMP_VALUE, (CL_ERR_DESC *)0,
			ULE_LOG , (DB_SQLSTATE *)NULL, (char * )0, 0L, 
			(i4 *)0, &err_code, 1, 
			sizeof(global->ops_qheader->pst_restab.pst_compress),
			&global->ops_qheader->pst_restab.pst_compress);

		    global->ops_qheader->pst_restab.pst_compress = 0; 
		}
		if (global->ops_qheader->pst_restab.pst_compress & 
				    PST_DATA_COMP)
		{
		    compressed = DMU_C_ON;
		}
		else
		{
		    compressed = DMU_C_OFF;
		}
		if (global->ops_qheader->pst_restab.pst_compress & 
				    PST_INDEX_COMP)
		{
		    index_compressed = DMU_C_ON;
		}
		else
		{
		    index_compressed = DMU_C_OFF;
		}
	    }

	    /* If the table is journalled, qepahd is going to compile a
	    ** row-by-row insert, else it's going to do a LOAD.
	    ** Row-by-row is best done into a heap, which is what the
	    ** table is now.  If the result will be heap compressed,
	    ** go ahead and make it compressed now.
	    ** If we're going to do a bulk load, and it's a tree structure,
	    ** modify now.  For hash, don't modify, as hash bulk load needs
	    ** to know the bucket count.
	    ** Summary:
	    ** Modify now if: non-journalled isam/btree,
	    **		or result is compressed heap.
	    ** Modify post-load under all other conditions.
	    */
	    modify_pre_load = (storage_type == DB_HEAP_STORE
					&& (compressed == DMU_C_ON
					    || index_compressed == DMU_C_ON))
			    ||
			      (! journaled && (storage_type == DB_ISAM_STORE
					      || storage_type == DB_BTRE_STORE));
	    if (modify_pre_load)
	    {
		/* Remember the DMU action header fixup for the CREATE
		** action, before we smash it with a MODIFY DMU action.
		** We need to point the CREATE DMU to the validation
		** stuff to fix up once table ID's are assigned.
		** opc-dmuahd sets the fixup pointer, so save it for
		** use below after we compile the query body.
		*/
		grv = 
		    global->ops_rangetab.opv_base->opv_grv[subqry->ops_result];
		retinto_alt = grv->opv_ptrvalid;

		if (storage_type == DB_HASH_STORE || journaled )
		{
		    ahd = opc_dmuahd(global, DMU_MODIFY_TABLE, 
				    global->ops_qheader, subqry->ops_result, 
				    (i4)DB_HEAP_STORE, compressed,
				    index_compressed);
		}
		else
		{
		    ahd = opc_dmuahd(global, DMU_MODIFY_TABLE, 
				    global->ops_qheader, subqry->ops_result, 
				    (i4)storage_type, compressed,
				    index_compressed);
		}
		mod_dmucb = (DMU_CB *)ahd->qhd_obj.qhd_dmu.ahd_cb;
		/* Tell modify it's allowed to pick a page size if needed */
		mod_dmucb->dmu_flags_mask |= DMU_RETINTO;
		ahd->ahd_prev = ahd_list->ahd_prev;
		ahd->ahd_next = ahd_list;
		ahd_list->ahd_prev->ahd_next = ahd;
		ahd_list->ahd_prev = ahd;
	    }
	}
	else if (subqry->ops_sqtype == OPS_FAGG)
	{
	    ahd_list = opc_dmtahd(global, subqry->ops_root, 
					    (i4) 0, subqry->ops_result);
	}
	else if (subqry->ops_sqtype == OPS_PROJECTION)
	{
	    ahd_list = opc_dmtahd(global, subqry->ops_root, 
					    DMT_LOAD, subqry->ops_result);
	}

	/* make an action to get the data and put it where it belongs; */
	ahd = opc_qepahd(global, proj_ahd);

	if (ahd_list == NULL)
	{
	    ahd_list = ahd;
	}
	else
	{
	    ahd->ahd_prev = ahd_list->ahd_prev;
	    ahd->ahd_next = ahd_list;
	    ahd_list->ahd_prev->ahd_next = ahd;
	    ahd_list->ahd_prev = ahd;
	}

	/* Normally opc-valid fixes up any DMU action, telling DMU where
	** to stick newly assigned table ID's;  but if we aimed the
	** wrong DMU (a modify), aim the right one now.
	*/
	if (retinto_alt != NULL)
	{
	    *retinto_alt = grv->opv_topvalid;
	}

	if (subqry->ops_sqtype == OPS_MAIN && 
		(global->ops_qheader->pst_mode == PSQ_RETINTO ||
		 global->ops_qheader->pst_mode == PSQ_DGTT_AS_SELECT) &&
		! modify_pre_load
	    )
	{
	    /* Didn't modify before, apply storage structure now.
	    ** Don't do it here either if result is uncompressed heap.
	    */
	    if (storage_type != DB_HEAP_STORE || compressed == DMU_C_ON)
	    {
		ahd = opc_dmuahd(global, DMU_MODIFY_TABLE, 
			    global->ops_qheader, subqry->ops_result,
			    (i4) storage_type, compressed, DMU_C_OFF);
		ahd->ahd_prev = ahd_list->ahd_prev;
		ahd->ahd_next = ahd_list;
		ahd_list->ahd_prev->ahd_next = ahd;
		ahd_list->ahd_prev = ahd;
	    }
	}
    }

    /* return the doubly linked list of actions just made; */
    *pahd = ahd_list;
    return;
}

/*{
** Name: OPC_QEPAHD	- Build a single QEP action header.
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
**      31-jul-86 (eric)
**          written
**	28-sep-87 (rogerk)
**	    Use QEA_LOAD action for RETRIEVE INTO
**      20-Dec-87 (eric)
**          Use QEA_APP action for RETIEVE INTO if journaled, use QEA_LOAD
**	    otherwise.
**	30-mar-89 (paul)
**	    Initialize query cost estimates to non-existent in QEP actions.
**	18-apr-90 (stec)
**	    Fix bug 20968.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	27-jun-90 (stec)
**	    Fixed bug 31113, which was to remove the fix for 20968 and put
**	    the check for valid ptr being NULL inside the for loop condition.
**	    Also changed initialization for ahd_ruprow which will depend on
**	    whether the update is by tid or positioned.
**	29-jun-90 (stec)
**	    Fixed bug 31092. The fix is to set proj_ahd to NULL when we are
**	    done processing the aggregate function case.
**	13-jul-90 (stec)
**	    Changed pagestouched (local var) from i4  to i4. Modified code
**	    to prevent floating point and integer overflow when pagestouched
**	    is calculated.
**	04-dec-90 (stec)
**	    Modified code to fix bug 34217. OPF code had to be modified as well
**	    to pass info necessary to determine eqcno for the TID attribute.
**	10-jan-91 (stec)
**	    Changed code dealing with integer overflow issue.
**	14-mar-91 (stec)
**	    Added casting to computations of pgs_t to make result correct.
**	16-apr-91 (davebf)
**	    Added parm to opc_valid() call in order to enable vl_size_sensitive
**	01-jul-91 (nancy)
**	    Fixed problem found in porting.  End comment missing in line above
**	    initialization of pst_t (pages touched) resultin in uninitialized
**	    value.
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**	16-jun-92 (anitap)
**	    Cast naked NULL passed into opc_addrules().
**	20-jul-92 (rickh)
**	    Wrong number of arguments to opc_target.
**	12-feb-93 (jhahn)
**	    Support for statement level rules.
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	17-may-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	09-jul-93 (jhahn)
**          Added back in change from 27-jun-90 that had gotten lost duing
**	    Outer Join rototilling.
**	17-aug-93 (anitap)
**	    Typecast the values in the conditional tests involving dissimilar
**	    types like float and MAXI4. The lack of these casts caused by
**	    floating point exceptions on a certain machine.
**	26-aug-93 (rickh)
**	    When compiling a CREATE TABLE AS SELECT statement, flag any NOT
**	    NULL actions to remember the rowcount of the SELECT action.
**	16-sep-93  (robf)
**          Pass on UPD_ROW_SECLABEL flag from query tree header into action 
**	    header. 
**      15-OCT-93 (eric)
**          corrected a number of valid list related issues
**		- how the update in-place dmr_cb is located.
**		- Open updateable cursor for reading if update clause not
**		  not specified.
**	07-oct-94 (mikem)
**	    Changed root node initialization so that duplicate key/row 
**	    errors will be ignored if in SQL and the PST_4IGNORE_DUPERR bit 
**	    is asserted in pst_mask1.  This bit is asserted if the ps201
**	    tracepoint has been executed in this session.  This traceflag
**	    will be used by the CA-MASTERPIECE product so that duplicate
**	    key errors will not shut down all open cursors (whenever the
**	    "keep cursors open on errors" project is integrated this change
**	    will no longer be necessary).
**	6-dec-93 (robf)
**	    Pass on NEED_QRY_SYSCAT flag from query tree to into action 
**	    header.
**      19-jun-95 (inkdo01)
**          changed 2-valued i4  ahd_any to 32-valued ahd_qepflag
**	26-june-1997(angusm)
**	    bugs 79623, 80750, 77040: flag up cross-table update for
**	    special handling in DMF.
**	06-mar-1996 (nanpr01)
**	    Changes to support multiple page size.
**	19-dec-1996 (nanpr01)
**	    For temporary table create with select may have tbl_pgsize set
**	    to zero since a suitable page_size has to be determined. The
**	    problem was identified in Solaris running on x386 platform where
**	    division by zero causes segv.
**	29-oct-97 (inkdo01)
**	    Added support of AHD_NOROWS flag (indicating query cannot return
**	    any rows).
**	26-oct-98 (inkdo01)
**	    Various changes to support QEA_RETROW (for row producing procs).
**	28-jan-99 (inkdo01)
**	    If row producing procedure, set QEQP_ROWPROC flag to identify them
**	    for scs_sequencer (and possibly others)
**      08-jul-99 (gygro01)
**	    Fix for bug 96955/pb ingsrv 859. Restricted AHD_NOROWS support.
**	    If repeated query with parameter get AHD_NOROWS flagged during
**	    optimization, all the following execution of the query even with
**	    correct parameters, will get no rows back (incorrect result).
**	    This is because repeated queries get only compiled once.
**	    This problem doesn't occurs for procedures or cursors.
**	3-mar-00 (inkdo01)
**	    Support for "select first n ...".
**	23-jan-01 (inkdo01)
**	    Changes for hash aggregation.
**	30-apr-01 (inkdo01)
**	    Added "first n" logic for QEA_APP (insert select/cr tab as select)
**	3-dec-01 (my daughter's birthday - inkdo01)
**	    Added "first n" logic for QEA_LOAD (decl global temp table as select
**	    and create table as select with nojournaling).
**	20-jan-04 (inkdo01)
**	    Added ahd_part_def - table partitioning descriptor.
**	6-feb-04 (inkdo01)
**	    Set flag indicating updates to partitioning columns.
**	19-mar-04 (inkdo01)
**	    Set QEA_NODEACT flag in act hdrs that own QEN_NODEs to help
**	    with || query cleanup.
**	14-Mar-2005 (schka24)
**	    Set 4byte-tidp flag using length info, so that we can get it
**	    right in the presence of sorts, etc..
**	7-Jul-2005 (schka24)
**	    Don't clip aggregate-result estimate at 10000; that's, uh,
**	    a little too small for 100 Gb TPC-H queries.
**	5-july-05 (inkdo01)
**	    Copy aggregate estimate (for hash agg) from bestco.
**	15-Jul-2005 (schka24)
**	    Don't set partitioning-column-update for deletes, not needed.
**	1-Sep-2005 (schka24)
**	    Bounds-check aggregate estimate since QP carries it as integer.
**	30-Jun-2006 (kschendel)
**	    Agg output estimates are now in the corresponding GRV.
**	    The subquery bestco costs the input to the agg.
**	5-mar-2007 (toumi01)
**	    For scrollable cursor temp tables default to 16K pages but if
**	    those are unavailable pick whatever else makes the most sense.
**	14-Mar-2007 (kschendel) SIR 122513
**	    Cut back on aggregation estimate if partition compatible, since
**	    each input aggregates separately.
**	16-mar-2007 (dougi)
**	    Changes for keyset scrollable cursors.
**	17-july-2007 (dougi)
**	    Add QEQP_SCROLL_CURS flag for scrollable cursors.
**	25-Sep-2007 (kschendel) SIR 122512
**	    Turn on contains-hashop flag for haggf.
**	12-dec-2007 (dougi)
**	    Add code to support parameterized top "n"/offset "n".
**	28-jan-2008 (dougi)
**	    Change ahd_rssplix to qp_rssplix.
**	4-Jun-2009 (kschendel) b122118
**	    Set MAIN flag for qe11.
**	8-Jun-2009 (kibro01) b122171
**	    Break up scrolling cursor temp table into multiple columns if
**	    larger than adu_maxstring.
**      12-aug-2009 (stial01)
**          Moved KEYSET initialization before setting up Resultsetcol(s)
**	11-May-2010 (kschendel)
**	    Don't set NODEACT flag unless there's really a QP underneath.
**	    The flag is meant as a shortcut for "this action has a sub-plan";
**	    it defeats the purpose if one has to test ahd_qep for NULL too.
*/
static QEF_AHD *
opc_qepahd(
	OPS_STATE   *global,
	QEF_AHD	    **proj_ahd )
{
    QEF_QP_CB	    *qp = global->ops_cstate.opc_qp;
    QEF_AHD	    *ahd;
    OPS_SUBQUERY    *sq = global->ops_cstate.opc_subqry;
    i4		    proj_row;
    OPC_NODE	    cnode;
    i4		    ev_size;
    OPE_IEQCLS	    eqcno;
    OPV_IVARS	    lresno;
    OPZ_IATTS	    jattno;    
    OPZ_ATTS	    *zatt;
    QEF_VALID	    *valid;
    QEF_VALID	    *orig_valid;
    PST_QNODE	    *const_root;
    PST_QNODE	    *resdom;
    OPV_GRV	    *grv;
    ADE_OPERAND	    tempop;
    i4		    topsort;
    i4		    resdomno;
    OPC_TGPART	    *tsoffsets;
    i4		pagestouched;
    OPC_PST_STATEMENT	*opc_pst;
    i4			cpos_ret;
    i4			unspec_updtmode;
    i4			res_dmrcbno;
    bool	    pcolsupd = FALSE;

    /* First, lets allocate the action header */
    ahd = sq->ops_compile.opc_ahd = 
			    (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD)-
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP));
    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    sq->ops_compile.opc_bgpostfix = sq->ops_compile.opc_endpostfix = NULL;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) - sizeof(ahd->qhd_obj) + sizeof(QEF_QEP);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;

    /* Set the status flag info in the QP node that is dependant on this
    ** action. 
    ** EJLFIX: This info should be set in a status field in the action header
    **		not in the QP node. Fix this when QEF is ready.
    */
    ahd->ahd_flags = 0;
    if (global->ops_qheader->pst_updtmode == PST_DIRECT ||
	    global->ops_qheader->pst_updtmode == PST_UNSPECIFIED
	)
    {
	ahd->ahd_flags = (ahd->ahd_flags | QEA_DIR);
    }
    else if (global->ops_qheader->pst_updtmode == PST_DEFER)
    {
	ahd->ahd_flags = (ahd->ahd_flags | QEA_DEF);
    }
    if (opc_pst->opc_stmtahd == ahd)
    {
	ahd->ahd_flags = (ahd->ahd_flags | QEA_NEWSTMT);
    }
    if ((sq->ops_mask & OPS_ZEROTUP_CHECK) != 0)
    {
	ahd->ahd_flags = (ahd->ahd_flags | QEA_ZEROTUP_CHK);
    }
    if (global->ops_qheader->pst_mask1 & PST_NEED_QRY_SYSCAT)
    {
	ahd->ahd_flags = (ahd->ahd_flags |  QEA_NEED_QRY_SYSCAT);
    }
    if (global->ops_qheader->pst_mask1 & PST_XTABLE_UPDATE)
    {
	ahd->ahd_flags = (ahd->ahd_flags | QEA_XTABLE_UPDATE);
    }

    /* Now lets get to the interesting stuff */
    switch (sq->ops_sqtype)
    {
     case OPS_FAGG:
	ahd->ahd_atype = QEA_AGGF;
	break;

     case OPS_SAGG:
     case OPS_RSAGG:
	ahd->ahd_atype = QEA_SAGG;
	break;

     case OPS_HFAGG:
	ahd->ahd_atype = QEA_HAGGF;
	break;

     case OPS_RFAGG:
	ahd->ahd_atype = QEA_RAGGF;
	break;

     case OPS_PROJECTION:
	ahd->ahd_atype = QEA_PROJ;
	*proj_ahd = ahd;
	break;

     default:
	switch (sq->ops_mode)
	{
	 case PSQ_RETROW:
	    global->ops_cstate.opc_retrowoff = 0;	/* init result row */
	    global->ops_cstate.opc_retrow_rsd = global->ops_qheader->pst_qtree;
							/* parse tree, for 
							** sqlda build */
	    ahd->ahd_atype = QEA_RETROW;
	    qp->qp_status |= QEQP_ROWPROC;
	    break;
	 case PSQ_RETRIEVE:
	 case PSQ_DEFCURS:
	    ahd->ahd_atype = QEA_GET;
	    break;

	 case PSQ_FOR:
	    ahd->ahd_atype = QEA_FOR;
	    break;

	 case PSQ_APPEND:
	    ahd->ahd_atype = QEA_APP;
	    break;

	 case PSQ_RETINTO:
	 case PSQ_DGTT_AS_SELECT:
	    /*
	    ** Flag subsequent NOT NULL integrity actions to remember the
	    ** rowcount accumulated by the SELECT action.
	    */
	    global->ops_cstate.opc_flags |= OPC_SAVE_ROWCOUNT;
	    ahd->ahd_atype = QEA_APP;	/* May get changed to LOAD later */
	    break;

	 case PSQ_DELETE:
	    if (opc_cposition(global, sq->ops_bestco, sq->ops_localres) == TRUE)
	    {
		/* Tell QEF to delete using the current position */
		ahd->ahd_atype = QEA_PDEL;
	    }
	    else
	    {
		/* Tell QEF to delete by tid */
		ahd->ahd_atype = QEA_DEL;
	    }
	    break;

	 case PSQ_REPLACE:
	    if (opc_cposition(global, sq->ops_bestco, sq->ops_localres) == TRUE)
	    {
		/* Tell QEF to update using the current position */
		ahd->ahd_atype = QEA_PUPD;
	    }
	    else
	    {
		/* Tell QEF to update by tid */
		ahd->ahd_atype = QEA_UPD;
	    }
	    break;
	 default:
	    opx_error(E_OP0686_CQMODE);
	    break;
	}
	break;
    }

    /* initialize the cnode */
    cnode.opc_cco = sq->ops_bestco;
    cnode.opc_level = OPC_COTOP;
    cnode.opc_invent_sort = FALSE;
    cnode.opc_qennode = (QEN_NODE *) NULL;
    cnode.opc_below_rows = 0;
    if (cnode.opc_cco == NULL)
    {
	cnode.opc_ceq = NULL;
    }
    else
    {
	ev_size = sq->ops_eclass.ope_ev * sizeof (OPC_EQ);
	cnode.opc_ceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
	MEfill(ev_size, (u_char)0, (PTR)cnode.opc_ceq);
    }

    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &sq->ops_compile.opc_mkey);

    /* fill in the qhd_qep stuff; */
    ahd->qhd_obj.qhd_qep.ahd_upd_colmap = (DB_COLUMN_BITMAP *) NULL;
    ahd->qhd_obj.qhd_qep.ahd_qep_cnt = 0;
    ahd->qhd_obj.qhd_qep.ahd_tidrow = -1;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = -1;
    ahd->qhd_obj.qhd_qep.ahd_reprow = -1;
    ahd->qhd_obj.qhd_qep.ahd_repsize = -1;
    ahd->qhd_obj.qhd_qep.ahd_firstncb = -1;
    ahd->qhd_obj.qhd_qep.ahd_qepflag = 0;   
    ahd->qhd_obj.qhd_qep.ahd_estimates = 0;
    ahd->qhd_obj.qhd_qep.ahd_dio_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cost_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cpu_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_row_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_page_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_part_def = (DB_PART_DEF *) NULL;
    ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
	global->ops_cstate.opc_before_row = -1;
    ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable = (QEN_TEMP_TABLE *)NULL;
 
    /* Set "I have hash operations" flag if a hash aggregation */
    if (ahd->ahd_atype == QEA_HAGGF)
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_CONTAINS_HASHOP;

    /* Set "I am in MAIN" if appropriate, mostly for QE11 at the moment */
    if (sq->ops_sqtype == OPS_MAIN)
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_MAIN;

    /* Fix for bug 96955. Don't apply AHD_NOROWS for repeated
       queries with parameters */
    if (sq->ops_bfs.opb_qfalse == TRUE &&
        !((global->ops_qheader->pst_mask1 & PST_RPTQRY) &&
          global->ops_qheader->pst_numparm > 0))
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_NOROWS;
					/* no rows from this one */

    if (global->ops_qheader == (PST_QTREE *)NULL ||
	    global->ops_qheader->pst_qtree == (PST_QNODE *)NULL ||
	((global->ops_qheader->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang
				    == DB_SQL) && 
	 (global->ops_qheader->pst_qtree->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_4IGNORE_DUPERR) == 0))
    {
	ahd->qhd_obj.qhd_qep.ahd_duphandle = QEF_ERR_DUP;	
    }
    else
    {
	ahd->qhd_obj.qhd_qep.ahd_duphandle = QEF_SKIP_DUP;	
    }

    if (global->ops_qheader != NULL && (ahd->ahd_atype == QEA_GET &&
	sq->ops_sqtype == OPS_MAIN || ahd->ahd_atype == QEA_APP ||
	ahd->ahd_atype == QEA_LOAD))
    {
	/* Set up first "n"/offset "n" values/parms. */
	if (global->ops_qheader->pst_offsetn == (PST_QNODE *) NULL)
	    ahd->qhd_obj.qhd_qep.ahd_offsetn = 0;
	else
	{
	    PST_QNODE 	*cptr = global->ops_qheader->pst_offsetn;
	    i4		ival;

	    if (cptr->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER)
	    {
		/* Constant - load it and save it. */
		if (cptr->pst_sym.pst_dataval.db_length == 2)
		    ival = *(i2 *)cptr->pst_sym.pst_dataval.db_data;
		else ival = *(i4 *)cptr->pst_sym.pst_dataval.db_data;

		ahd->qhd_obj.qhd_qep.ahd_offsetn = ival;
	    }
	    else
	    {
		ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PARM_OFFSETN;
		if (global->ops_procedure->pst_isdbp)
		{
		    opc_lvar_row(global, 
			cptr->pst_sym.pst_value.pst_s_cnst.pst_parm_no,
			(OPC_ADF *) NULL, &tempop);
		    ahd->qhd_obj.qhd_qep.ahd_offsetn = tempop.opr_base;
		}
		else ahd->qhd_obj.qhd_qep.ahd_offsetn = 
			cptr->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
	    }
	}

	if (global->ops_qheader->pst_firstn == (PST_QNODE *) NULL)
	    ahd->qhd_obj.qhd_qep.ahd_firstn = 0;
	else
	{
	    PST_QNODE 	*cptr = global->ops_qheader->pst_firstn;
	    i4		ival;

	    if (cptr->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER)
	    {
		/* Constant - load it and save it. */
		if (cptr->pst_sym.pst_dataval.db_length == 2)
		    ival = *(i2 *)cptr->pst_sym.pst_dataval.db_data;
		else ival = *(i4 *)cptr->pst_sym.pst_dataval.db_data;

		ahd->qhd_obj.qhd_qep.ahd_firstn = ival;
	    }
	    else
	    {
		ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PARM_FIRSTN;
		if (global->ops_procedure->pst_isdbp)
		{
		    opc_lvar_row(global, 
			cptr->pst_sym.pst_value.pst_s_cnst.pst_parm_no,
			(OPC_ADF *) NULL, &tempop);
		    ahd->qhd_obj.qhd_qep.ahd_firstn = tempop.opr_base;
		}
		else ahd->qhd_obj.qhd_qep.ahd_firstn = 
			cptr->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
	    }
	}

	if (ahd->qhd_obj.qhd_qep.ahd_offsetn != 0 ||
		ahd->qhd_obj.qhd_qep.ahd_firstn != 0)
	    opc_ptcb(global, &ahd->qhd_obj.qhd_qep.ahd_firstncb, 0);
					/* NOTE: size of block is ignored */
    }
    else 
    {
	ahd->qhd_obj.qhd_qep.ahd_offsetn = 0;
	ahd->qhd_obj.qhd_qep.ahd_firstn = 0;
    }

    if (cnode.opc_cco != NULL)
    {
	opc_qentree_build(global, &cnode);

	if (cnode.opc_qennode != NULL && 
		    cnode.opc_qennode->qen_type == QE_TSORT
	    )
	{
	    topsort = TRUE;
	}
	else
	{
	    topsort = FALSE;
	}

	if (ahd->ahd_atype == QEA_UPD	    || 
		ahd->ahd_atype == QEA_PUPD  ||
		ahd->ahd_atype == QEA_DEL   ||
		ahd->ahd_atype == QEA_PDEL
	    )
	{
	    i4 tidlength;

	    if (topsort == TRUE)
	    {
		ahd->qhd_obj.qhd_qep.ahd_tidrow = cnode.opc_qennode->qen_row;
		    /* EJLFIX: Check that the opc_toffset for the DB_IMTID
		    ** is valid.
		    */
		tsoffsets = sq->ops_compile.opc_sortinfo.opc_tsoffsets;
		ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
					    tsoffsets[DB_IMTID].opc_toffset;
		tidlength = tsoffsets[DB_IMTID].opc_tlength;
	    }
	    else
	    {
		if (sq->ops_localres != OPV_NOVAR)
		{
		    lresno = sq->ops_localres;
		    eqcno = sq->ops_vars.opv_base->opv_rt[lresno]->
			opv_primary.opv_tid;
		}
		else
		{
		    /* Fix for bug 34217 */
		    OPZ_IATTS	jatt;
		    OPZ_ATTS	*zatt;
		    i4		found = 0;

		    for (jatt = 0; jatt < sq->ops_attrs.opz_av; jatt++)
		    {
			zatt = sq->ops_attrs.opz_base->opz_attnums[jatt];
			if (zatt->opz_gvar == global->ops_tvar &&
			    zatt->opz_attnm.db_att_id ==
				    global->ops_tattr.db_att_id)
			{
			    found++;
			    break;
			}
		    }

		    if (found)
		    {
			eqcno = zatt->opz_equcls;
		    }
		    else
		    {
			opx_error(E_OP0889_EQ_NOT_HERE);
		    }
		}

		if (cnode.opc_ceq[eqcno].opc_eqavailable == FALSE)
		{
		    opx_error(E_OP0889_EQ_NOT_HERE);
		}
		if (cnode.opc_ceq[eqcno].opc_attavailable == FALSE)
		{
		    opx_error(E_OP0888_ATT_NOT_HERE);
		}

		ahd->qhd_obj.qhd_qep.ahd_tidrow = 
					    cnode.opc_ceq[eqcno].opc_dshrow;
		ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
					cnode.opc_ceq[eqcno].opc_dshoffset;
		tidlength = cnode.opc_ceq[eqcno].opc_eqcdv.db_length;
	    }
	    /* If this query fragment returned 4-byte tids (perhaps from
	    ** a retrieval from a secondary index), make sure that the QEF
	    ** action routine knows that they are 4-byte and not 8-byte.
	    */
	    if (tidlength == DB_TID_LENGTH)
		ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_4BYTE_TIDP;
	}
	else if (sq->ops_sqtype == OPS_MAIN && sq->ops_mode == PSQ_DEFCURS &&
		    (global->ops_qheader->pst_updtmode != PST_READONLY ||
			global->ops_qheader->pst_delete == TRUE
		    )
		)
	{
	    for (resdom = sq->ops_root->pst_left;
		    resdom->pst_sym.pst_type == PST_RESDOM;
		    resdom = resdom->pst_left
		)
	    {
		/* If we have found the non-printing tid resdom for this
		** updatable cursor, then record where the tid is so that
		** updates and deletes to the cursor will work correctly.
		*/
		if (!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT) &&
			resdom->pst_right->pst_sym.pst_type == PST_VAR
		    )
		{
		    jattno = resdom->pst_right->pst_sym.
					pst_value.pst_s_var.pst_atno.db_att_id;

		    if (sq->ops_attrs.opz_base->opz_attnums[jattno]->
						opz_attnm.db_att_id == DB_IMTID
			)
		    {
			OPE_IEQCLS	eqcn;
			i4		tidlength;

			eqcn = sq->ops_attrs.opz_base->
			    opz_attnums[jattno]->opz_equcls;

			if (topsort == TRUE)
			{
			    ahd->qhd_obj.qhd_qep.ahd_tidrow = 
						    cnode.opc_qennode->qen_row;
				/* EJLFIX: Check that the opc_toffset for 
				** the DB_IMTID is valid.
				*/
			    resdomno = 
				resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
			    tsoffsets =
				    sq->ops_compile.opc_sortinfo.opc_tsoffsets;
			    ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
						tsoffsets[resdomno].opc_toffset;
			    tidlength = tsoffsets[resdomno].opc_tlength;
			}
			else
			{
			    /* Now find the attribute that we actually have
			    ** for the eq class of the attribute that was
			    ** asked for.
			    */
			    zatt = sq->ops_attrs.opz_base->opz_attnums[jattno];
			    eqcno = zatt->opz_equcls;
			    if (cnode.opc_ceq[eqcno].opc_eqavailable == FALSE)
			    {
				opx_error(E_OP0889_EQ_NOT_HERE);
			    }
			    if (cnode.opc_ceq[eqcno].opc_attavailable == FALSE)
			    {
				opx_error(E_OP0888_ATT_NOT_HERE);
			    }

			    /* Tell QEF where to get the tid */
			    ahd->qhd_obj.qhd_qep.ahd_tidrow = 
				cnode.opc_ceq[eqcn].opc_dshrow;
			    ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
				cnode.opc_ceq[eqcn].opc_dshoffset;
			    tidlength = cnode.opc_ceq[eqcn].opc_eqcdv.db_length;
			}
			/* Mark action header if small tids were returned */
			if (tidlength == DB_TID_LENGTH)
			    ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_4BYTE_TIDP;
			break;
		    }
		}
	    }
	}
    }
    ahd->qhd_obj.qhd_qep.ahd_qep = cnode.opc_qennode;
    if (cnode.opc_qennode != NULL)
	ahd->ahd_flags |= QEA_NODEACT;	/* flag act hdr as owning NODE */
    ahd->qhd_obj.qhd_qep.ahd_postfix = sq->ops_compile.opc_bgpostfix;

    opc_adend(global, &sq->ops_compile.opc_mkey);

    /* Now lets add the result relation to the valid list */
    /* Pick up global range var for subquery while we're at it.  If both
    ** result and gentry are set they should be the same.
    */
    valid = (QEF_VALID *) NULL;
    grv = NULL;
    if (sq->ops_gentry != OPV_NOGVAR)
	grv = global->ops_rangetab.opv_base->opv_grv[sq->ops_gentry];
    if (sq->ops_result != OPV_NOGVAR && sq->ops_mode != PSQ_RETRIEVE)
    {
	/* Tell QEF about the result relation for this action */
	grv = global->ops_rangetab.opv_base->opv_grv[sq->ops_result];
	valid = orig_valid = NULL;

	if (ahd->ahd_atype == QEA_PDEL ||
		ahd->ahd_atype == QEA_PUPD ||
		sq->ops_mode == PSQ_DEFCURS
	    )
	{
	    /* If this is an updateable or deletable cursor, or an in
	    ** place (current position) replace or delete statement, then we
	    ** just want to update or delete from the same valid list
	    ** entry that is doing the reading
	    */
	    cpos_ret = opc_cposition(global, sq->ops_bestco, sq->ops_localres);
	    if (global->ops_qheader->pst_updtmode == PST_UNSPECIFIED)
		unspec_updtmode = TRUE;
	    else
		unspec_updtmode = FALSE;

	    if (cpos_ret == TRUE && unspec_updtmode == FALSE)
	    {
		res_dmrcbno = opc_btmleft_dmrcbno(global, cnode.opc_qennode);
		for (valid = ahd->ahd_valid;
			valid != NULL && valid->vl_dmr_cb != res_dmrcbno;
			valid = valid->vl_next
		    )
		{
		    /* Null body: we just want to find the valid list entry
		    **	for the result relation.
		    */
		}
	    }

	    /* remember the valid list entry for the orig node of the result
	    ** relation. We might discover in the next if statement that we
	    ** can't do an in place update/delete, so orig_valid will allow
	    ** us to remember where the tuple was read in from.
	    */
	    orig_valid = valid;
	}

	if (ahd->ahd_atype != QEA_PDEL &&
		ahd->ahd_atype != QEA_PUPD &&
		(sq->ops_mode != PSQ_DEFCURS || 
		    cpos_ret == FALSE ||
		    unspec_updtmode == TRUE
		)
	    )
	{
	    /* This update, delete, whatever will not be done in place.
	    ** Instead, it will be done by tid.
	    ** Estimate the number of pages that will be touched by the
	    ** result relation.
	    */
	    if (sq->ops_bestco == NULL || grv == NULL || 
		grv->opv_relation->rdr_rel == NULL ||
		    (sq->ops_mode == PSQ_DEFCURS &&
			global->ops_qheader->pst_updtmode == PST_READONLY
		    )
		)
	    {
		/* if there isn't any CO tree or if this is a deletable but
		** not updatable cursor then assume that we will touch one
		** page.
		*/
		pagestouched = 1;
	    }
	    else
	    {
		f4	pgs_t;	/* number of pages touched */
		i4 pg_size;

		if (grv->opv_relation->rdr_rel->tbl_pgsize)
		    pg_size = grv->opv_relation->rdr_rel->tbl_pgsize;
		else
		    pg_size = opc_pagesize(global->ops_cb->ops_server, 
					grv->opv_relation->rdr_rel->tbl_width);

		/* pgs_t will be in the range (0.0, 1.0) */
		pgs_t = (f4)grv->opv_relation->rdr_rel->tbl_width / pg_size;
						/* previously DB_PAGESIZE; */
		/* Overflow should never occur. */
		if (grv->opv_gcost.opo_tups > 0)
		    pgs_t *= grv->opv_gcost.opo_tups;

		/* Code below will prevent integer overflow. */
		pagestouched = (pgs_t < (f4)MAXI4) ? (i4)pgs_t : MAXI4;
	    }

	    valid = (QEF_VALID *) opc_valid(global, grv, DMT_X, 
					    pagestouched, (bool)FALSE);
	}
	else if (sq->ops_mode == PSQ_DEFCURS)
	{
	    /* This cursor will be updated by position,
	    ** not by tid. This means that the tid info will not be required.
	    */
	    ahd->qhd_obj.qhd_qep.ahd_tidrow = -1;
	    ahd->qhd_obj.qhd_qep.ahd_tidoffset = -1;
	}
    }

    /* Fill in ahd_ruprow, the row number of the row to be updated in a
    ** replace cursor statement
    */
    ahd->qhd_obj.qhd_qep.ahd_ruprow = -1;
    ahd->qhd_obj.qhd_qep.ahd_ruporig = -1;
    if (sq->ops_result != OPV_NOGVAR && sq->ops_mode == PSQ_DEFCURS)
    {
	if (orig_valid == valid && valid != (QEF_VALID *) NULL)
	{
	    /* This is the POSITIONED UPDATE case, since target
	    ** and source valid struct ptrs are identical.
	    ** Fill in originating row and node number.
	    */
	    opc_gtrowno(global, &ahd->qhd_obj.qhd_qep,
			cnode.opc_qennode, orig_valid);
	}
	else
	{
	    /* This is the UPDATE BY TID case, since target and
	    ** source valid struct ptrs are different. Grv is set
	    ** up above to point to the entry for the result relation.
	    ** Don't need to fill in ruporig in this case since
	    ** execution time can get what it needs from the tid.
	    */
	    opc_ptrow(global, &ahd->qhd_obj.qhd_qep.ahd_ruprow,
		grv->opv_relation->rdr_rel->tbl_width);
	}
    }

    /* Now that we know about the result valid list entry, we can fill in the
    ** ahd_odmr_cb crap.
    */
    if (valid == (QEF_VALID *)NULL)
    {
	ahd->qhd_obj.qhd_qep.ahd_odmr_cb = -1;
    }
    else
    {
	ahd->qhd_obj.qhd_qep.ahd_odmr_cb = valid->vl_dmr_cb;
	global->ops_cstate.opc_qp->qp_upd_cb = valid->vl_dmr_cb;
	ahd->qhd_obj.qhd_qep.ahd_dmtix = valid->vl_dmf_cb;
    }

    /* compile the constant qualification if there is one. */
    const_root = sq->ops_bfs.opb_bfconstants;
    if (const_root == NULL ||
	    const_root->pst_sym.pst_type == PST_QLEND
	)
    {
	ahd->qhd_obj.qhd_qep.ahd_constant = (QEN_ADF *)NULL;
    }
    else
    {
	opc_ccqtree(global, &cnode, const_root, 
			    &ahd->qhd_obj.qhd_qep.ahd_constant, -1);
    }

    /* Now that we have built the qen tree, we can compile the target list */
    if (ahd->ahd_atype != QEA_DEL && ahd->ahd_atype != QEA_PDEL)
    {
#ifdef xDEV_TEST
	opc_chktarget(global, &cnode, sq->ops_root);
#endif

	switch (sq->ops_sqtype)
	{
	 case OPS_FAGG:
	    opc_agtarget(global, &cnode, sq->ops_root, 
					    &ahd->qhd_obj.qhd_qep);
	    if (*proj_ahd == NULL)
	    {
		ahd->qhd_obj.qhd_qep.ahd_by_comp = (QEN_ADF *)NULL;
		ahd->qhd_obj.qhd_qep.ahd_proj = -1;
	    }
	    else
	    {
		proj_row = (*proj_ahd)->qhd_obj.qhd_qep.ahd_current->qen_output;
		opc_agproject(global, &cnode, 
				&ahd->qhd_obj.qhd_qep.ahd_by_comp,
				ahd->qhd_obj.qhd_qep.ahd_current->qen_output,
				proj_row);
		ahd->qhd_obj.qhd_qep.ahd_proj = 
				(*proj_ahd)->qhd_obj.qhd_qep.ahd_odmr_cb;
		/* Fix for bug 31092 */
		*proj_ahd = NULL;
	    }
	    break;

	 case OPS_SAGG:
	 case OPS_RSAGG:
	 case OPS_RFAGG:
	 case OPS_HFAGG:
	    ahd->qhd_obj.qhd_qep.ahd_by_comp = (QEN_ADF *)NULL;
	    opc_agtarget(global, &cnode, sq->ops_root, 
					    &ahd->qhd_obj.qhd_qep);
	    if (sq->ops_sqtype == OPS_SAGG || sq->ops_sqtype == OPS_RSAGG) 
			opc_saggopt(sq, ahd, sq->ops_root);
				/* check for simple agg optimization */
	    if (sq->ops_sqtype == OPS_HFAGG)
	    {
		ahd->qhd_obj.qhd_qep.u1.s2.ahd_agcbix =
			global->ops_cstate.opc_qp->qp_hashagg_cnt++;
				/* hash aggregate control block index */
		ahd->qhd_obj.qhd_qep.u1.s2.ahd_aggest = MAXI4/2;
		if (grv != NULL && grv->opv_gcost.opo_tups < MAXI4/2)
		    ahd->qhd_obj.qhd_qep.u1.s2.ahd_aggest = 
				grv->opv_gcost.opo_tups;
		if (ahd->qhd_obj.qhd_qep.u1.s2.ahd_aggest >= 10000
		  && sq->ops_mask & OPS_PCAGG)
		{
		    /* Reduce estimate by the number of partition groups.
		    ** At present, qef uses a very coarse measure to
		    ** figure aggregation allocations, else I'd bump
		    ** the estimate up by 10% or so after reducing.
		    ** Don't bother reducing puny aggregations.
		    */
		    ahd->qhd_obj.qhd_qep.u1.s2.ahd_aggest /= sq->ops_gcount;
		}

		opc_ptcb(global, &ahd->qhd_obj.qhd_qep.u1.s2.ahd_agdmhcb,
			sizeof(DMH_CB));
	    }
	    ahd->qhd_obj.qhd_qep.ahd_proj = -1;
	    break;

	 default:
	    opc_target(global, &cnode, sq->ops_root->pst_left, 
				    &ahd->qhd_obj.qhd_qep.ahd_current );
	    ahd->qhd_obj.qhd_qep.ahd_by_comp = (QEN_ADF *)NULL;
	    ahd->qhd_obj.qhd_qep.ahd_proj = -1;
	    /* Build setlist bitmap for update action headers */
	    if (ahd->ahd_atype == QEA_UPD || ahd->ahd_atype == QEA_PUPD)
		opc_updcolmap(global, &ahd->qhd_obj.qhd_qep,
			sq->ops_root->pst_left, grv->opv_relation->rdr_parts);
	    if (ahd->ahd_atype == QEA_RETROW &&
		global->ops_cstate.opc_qp->qp_res_row_sz == 0)
	     global->ops_cstate.opc_qp->qp_res_row_sz = 
		global->ops_cstate.opc_retrowoff;
	    break;
	}
    }
    else
    {
	ahd->qhd_obj.qhd_qep.ahd_current = (QEN_ADF *)NULL;
	ahd->qhd_obj.qhd_qep.ahd_by_comp = (QEN_ADF *)NULL;
	ahd->qhd_obj.qhd_qep.ahd_proj = -1;
    }

    if ((ahd->ahd_atype == QEA_APP || ahd->ahd_atype == QEA_UPD ||
	ahd->ahd_atype == QEA_PUPD || ahd->ahd_atype == QEA_DEL ||
	ahd->ahd_atype == QEA_PDEL || ahd->ahd_atype == QEA_LOAD) &&
	grv->opv_relation->rdr_parts != (DB_PART_DEF *) NULL)
    {
	/* Insert/update/delete on partitioned table - copy DB_PART_DEF */
	opc_partdef_copy(global, grv->opv_relation->rdr_parts, opu_qsfmem,
	    &ahd->qhd_obj.qhd_qep.ahd_part_def);
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PART_TAB;
    }

    /* Check for scrollable cursor GET action. */
    if (sq->ops_mode == PSQ_DEFCURS && 
		(global->ops_qheader->pst_mask1 & PST_SCROLL))
    {
	DMF_ATTR_ENTRY	*att;
	i4	dummy_rsize;
	i4	natts;
	i4	loop;

	/* Allocate & init the structure to house the scrollable stuff. */
	ahd->qhd_obj.qhd_qep.ahd_scroll = 
		(QEF_SCROLL *) opu_qsfmem(global, sizeof (QEF_SCROLL));
	opc_ptcb(global, &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsdmtix, 0);
	opc_ptcb(global, &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rswdmrix, 0);
	opc_ptcb(global, &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrdmrix, 0);
	opc_ptcb(global, &global->ops_cstate.opc_qp->qp_rssplix, 0);
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize = qp->qp_res_row_sz;

	/* Init KEYSET cursor fields */
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskscnt = 0;
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskattr = (DMF_ATTR_ENTRY *) NULL;
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff1 = (i4 *) NULL;
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff2 = (i4 *) NULL;
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskcurr = (QEN_ADF *) NULL;

	/*
	** Do extra stuff for KEYSET processing.  
	** This may reset ahd_rsrsize, so do it BEFORE referencing ahd_rsrsize
	*/
	if (global->ops_qheader->pst_mask1 & PST_KEYSET)
	    opc_keyset_build(global, ahd, &cnode);

	natts = (ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize + 
		    (global->ops_adfcb->adf_maxstring - 1)) /
			global->ops_adfcb->adf_maxstring;

	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_SCROLL;
	qp->qp_status |= QEQP_SCROLL_CURS;

	att = (DMF_ATTR_ENTRY *)opu_qsfmem(global, sizeof (DMF_ATTR_ENTRY) * natts);
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsattr =
		(DMF_ATTR_ENTRY **)opu_qsfmem(global, sizeof (PTR) * natts);

	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsnattr = natts;
	for (loop=0;loop<natts;loop++)
	{
	    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsattr[loop] = &att[loop];
	    if (loop==(natts-1))
		att[loop].attr_size =
			ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize %
				global->ops_adfcb->adf_maxstring;
	    else
		att[loop].attr_size = global->ops_adfcb->adf_maxstring;
	    att[loop].attr_type = DB_CHA_TYPE;
	    att[loop].attr_type = DB_BYTE_TYPE;

	    att[loop].attr_precision = 0;
	    att[loop].attr_collID = -1;
	    att[loop].attr_flags_mask = 0;
	    SET_CANON_DEF_ID(att[loop].attr_defaultID, DB_DEF_NOT_DEFAULT);
	    att[loop].attr_defaultTuple = NULL;
	    STprintf(att[loop].attr_name.db_att_name,"Resultsetcol%d",loop);
	}

	/* use hard coded value to prejudice opc_pagesize to 16K */
	dummy_rsize = 16384/(OPC_PAGESIZE_DW_ROWS+1);
	if (dummy_rsize < ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize)
	    dummy_rsize = ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize;
	ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rspsize =
		opc_pagesize(global->ops_cb->ops_server, dummy_rsize);
    }
    else ahd->qhd_obj.qhd_qep.ahd_scroll = (QEF_SCROLL *) NULL;

    if (sq->ops_mode == PSQ_REPLACE)
    {
	if (grv->opv_gmask & OPV_STATEMENT_LEVEL_UNIQUE)
	{
	    ahd->ahd_flags |= QEF_STATEMENT_UNIQUE;
	    opc_tempTable(global, &ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable,
			  grv->opv_relation->rdr_rel->tbl_width,
			  &global->ops_cstate.opc_tempTableRow);
	    ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable->ttb_attr_list = NULL;
	    ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable->ttb_attr_count = 0;
	    ahd->qhd_obj.qhd_qep.u1.s1.ahd_failed_a_temptable->ttb_pagesize =
	    		grv->opv_relation->rdr_rel->tbl_pgsize;
	}
	ahd->qhd_obj.qhd_qep.ahd_repsize =
	    grv->opv_relation->rdr_rel->tbl_width;
    }
    /* Add any rules information required */
    opc_addrules(global, ahd, (RDR_INFO *)NULL);

    /* Change APPEND to LOAD where possible */
    if (ahd->ahd_atype == QEA_APP)
	opc_loadopt(global, ahd, valid);

    /* Finally, mark in the qp header that there is another action,
    ** and fill in the "stat cnt" (what ever the hell that is).
    */
    qp->qp_ahd_cnt += 1;
    if (qp->qp_stat_cnt == 0)
    {
	qp->qp_stat_cnt = 1;
    }
    qp->qp_stat_cnt += ahd->qhd_obj.qhd_qep.ahd_qep_cnt;

    return(ahd);
}

/*{
** Name: OPC_KEYSET_BUILD - add KEYSET info to action header
**
** Description:
**      This is a keyset scrollable cursor. Additional stuff needs 
**	to be compiled into the GET action header for the cursor and
**	this is where it is done.
**
** Inputs:
**  global	- ptr to global state structure
**  ahd		- ptr to action header to augment
**
** Outputs:
**
**	Returns:
**	    Additional fields are filled into the action header atop the
**	    scrollable cursor.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-mar-2007 (dougi)
**          Created for scrollable cursors.
**	31-may-2007 (dougi)
**	    Added separate cb array slot for addr of update/delete DMR_CB.
**	17-july-2007 (dougi)
**	    Add space for checksum to keyset buffer.
**	2-oct-2007 (dougi)
**	    Add ahd_rsrrow and new tidrow/offset assignment to properly address
**	    tids in KEYSET posn'd upd/del.
**	7-Sep-2009 (kibro01) b122556
**	    Allocate proper amount of space for offset pointers in keyset.
*/
static VOID
opc_keyset_build(
	OPS_STATE	*global,
	QEF_AHD		*ahd,
	OPC_NODE	*cnode)

{
    PST_QNODE	*rsdmp, **rsdmpp;
    DMF_ATTR_ENTRY *attp;
    OPO_CO	*cop;
    OPS_SUBQUERY *subquery = global->ops_subquery;
    OPC_EQ	*ceq, *oldceq;
    OPE_IEQCLS	eqcno;
    i4		kcnt, rcnt, memsize, evsize, rowsz, rrowsz, rowno;
    i4		*koff1, *koff2;
    i4		off1, off2;
    i4		i;


    /* Count keyset entries for memory allocation. */
    for (kcnt = 0, rcnt=0, rsdmp = global->ops_qheader->pst_qtree->pst_left;
	    rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
	rcnt++, rsdmp = rsdmp->pst_left)
     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_KEYSET)
	kcnt++;

    memsize = kcnt * sizeof(DMF_ATTR_ENTRY) + 2 * kcnt * sizeof(i4);
    attp = (DMF_ATTR_ENTRY *)opu_qsfmem(global, memsize);
    koff1 = (i4 *)&attp[kcnt];
    koff2 = &koff1[kcnt];

    ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_KEYSET;
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskscnt = kcnt;
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskattr = attp;
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff1 = koff1;
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskoff2 = koff2;

    /* Allocate RESDOM ptr array and fill with all RESDOM addrs. */
    rsdmpp = (PST_QNODE **) opu_Gsmemory_get(global, rcnt * sizeof(PTR));

    /* Loop over RESDOMs again, saving addrs in array in reverse
    ** sequence - matching the result set. */
    for (rsdmp = global->ops_qheader->pst_qtree->pst_left, i = rcnt-1;
	    rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
	rsdmp = rsdmp->pst_left, i--)
     rsdmpp[i] = rsdmp;

    /* Locate CO-node for base table. */
    cop = opc_keyset_co(global->ops_subquery->ops_bestco, subquery,
	global->ops_qheader->pst_restab.pst_resvno, &rowsz);

    /* Allocate/init CEQ array. */
    evsize = subquery->ops_eclass.ope_ev * sizeof (OPC_EQ);
    ceq = (OPC_EQ *) opu_Gsmemory_get(global, evsize);
    MEfill(evsize, (u_char)0, (PTR)ceq);

    /* Allocate row buffer & CBs and assign relevant CEQ entries. */
    opc_ptrow(global, &rowno, rowsz);
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsbtrow = rowno;
    opc_ptcb(global, &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsbtdmrix, 0);
    opc_ptcb(global, &global->ops_cstate.opc_qp->qp_upd_cb, 0);
					/* pos'd upd/del DMR_CB ptr */
    opc_ratt(global, ceq, cop, rowno, rowsz);

    rrowsz = DB_TID8_LENGTH + sizeof(i4); /* leave space for TID & checksum */

    /* Loop over RESDOMs again, locating KEYSET entries and filling in
    ** the arrays. */
    for (i = 0, off1 = 0; i < rcnt; i++)
    {
	rsdmp = rsdmpp[i];
	if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_KEYSET)
	{
	    /* Fill in various array entries. */
	    attp->attr_type = rsdmp->pst_sym.pst_dataval.db_datatype;
	    attp->attr_size = rsdmp->pst_sym.pst_dataval.db_length;
	    attp->attr_precision = rsdmp->pst_sym.pst_dataval.db_prec;
	    attp->attr_collID = rsdmp->pst_sym.pst_dataval.db_collID;
	    attp->attr_flags_mask = 0;
	    attp->attr_defaultTuple = NULL;

	    if (rsdmp->pst_right && rsdmp->pst_right->pst_sym.pst_type == PST_VAR)
	    {
		eqcno = subquery->ops_attrs.opz_base->opz_attnums[
		    rsdmp->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.
			db_att_id]->opz_equcls;
		if (!ceq[eqcno].opc_attavailable)
		    opx_error(E_OP0889_EQ_NOT_HERE);

		*koff2 = ceq[eqcno].opc_dshoffset;
	    }
	    else opx_error(E_OP088B_BAD_NODE);	/* trouble */

	    rrowsz += attp->attr_size;
	    *koff1 = off1;
	    attp++; koff1++; koff2++;
	}

	if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT)
	    off1 += rsdmp->pst_sym.pst_dataval.db_length;
    }

    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrsize = rrowsz;	
						/* keyset buff size */
    opc_ptrow(global, &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrrow, rrowsz);
						/* add a buffer, for use by 
						** posn'd upd/del */

    /* KEYSET cursor needs to set up different tidrow/offset. But save 
    ** originals for use in storing temp table result rows. */
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_tidoffset =
				ahd->qhd_obj.qhd_qep.ahd_tidoffset;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = 0;
    ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_tidrow =
				ahd->qhd_obj.qhd_qep.ahd_tidrow;
    ahd->qhd_obj.qhd_qep.ahd_tidrow = 
		ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rsrrow;

    /* Finally, CX to rebuild result row from row buffer. */
    oldceq = cnode->opc_ceq;
    cnode->opc_ceq = ceq;

    opc_target(global, cnode, subquery->ops_root->pst_left, 
			    &ahd->qhd_obj.qhd_qep.ahd_scroll->ahd_rskcurr);


}

/*{
** Name: OPC_KEYSET_CO	- search for CO node of underlying base table
**
** Description:
**      This is a keyset scrollable cursor. Locate the CO node of
**	the underlying base table (there can only be one).
**
** Inputs:
**	cop	- ptr to CO subtree to be searched
**	resvno	- global var no of underlying table
**	subquery - ptr to subquery structure
**
** Outputs:
**
**	Returns:
**	    CO ptr of ORIG node of underlying base table
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-mar-2007 (dougi)
**          Created for scrollable cursors.
*/
static OPO_CO *
opc_keyset_co(
	OPO_CO	*cop,
	OPS_SUBQUERY *subquery,
	i4	resvno,
	i4	*rowsz)

{
    OPO_CO	*cop1;
    OPV_IGVARS	gvno;
    OPV_VARS	*varp;

    /* Descend left and right by recursion, looking for ORIG's. */
    if (cop == NULL)
	return(NULL);			/* shouldn't happen */

    if (cop->opo_outer)
     if ((cop1 = opc_keyset_co(cop->opo_outer, subquery, resvno, rowsz)))
	return(cop1);

    if (cop->opo_inner)
	return(opc_keyset_co(cop->opo_inner, subquery, resvno, rowsz));
					/* no need to come back */

    if (cop->opo_sjpr == DB_ORIG)
    {
	/* Should only be ORIG, but code for exceptions. */
	varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];
	if (varp->opv_mask & OPV_CINDEX)
	    varp = subquery->ops_vars.opv_base->opv_rt[
				varp->opv_index.opv_poffset];
					/* replacement index */
	gvno = varp->opv_gvar;
	*rowsz = varp->opv_grv->opv_relation->rdr_rel->tbl_width;

	if (gvno == resvno)
	    return(cop);		/* got match - return OPO_CO * */
    }

    return(NULL);
}

/*{
** Name: OPC_SAGGOPT  - Determine optimization potential of count, max and   
**                        min functions
**
** Description:
**      This function determines whether the simple aggregates being 
**      computed by the input ahd fall into the categories of 
**      optimizeable count, max and/or min functions. If so, flags
**      are set in the appropriate blocks.
**
** Inputs:
**  sq - 
**	Subquery state info.  
**  ahd -
**	The SAGG action header to scrutinize.
**  root -
**      The resdom list of aggregate functions being performed.
**
** Outputs:
**
**	Returns:
**	    Flags set in one or both of the SAGG action header (for max/
**          min optimization) or the ORIG node (count optimization).
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-jul-95 (inkdo01)
**          Created
**      20-aug-95 (ramra01)
**          Made some minor changes to permit MAX, MIN with the where clause
**          as well as the fact that they may have a NULLABLE column defined.
**      07-feb-96 (musro02)
**          Changed the bool variables max, min, count, qual to ..._flag 
**	    This prevents collisions with identically named functions.  
**	    (The collision caused problems on su4_u42 with acc.)
[@history_template@]...
*/
static VOID
opc_saggopt(
	OPS_SUBQUERY * sq,  
	QEF_AHD     * ahd,
	PST_QNODE   * root)
{
    PST_QNODE       * nodep, *rsdp, *varp;  /* work ptrs */
    QEN_NODE        * qenp = ahd->qhd_obj.qhd_qep.ahd_qep;
                                    /* root node of qtree under SAGG */
    QEF_VALID       * valid;       
    RDR_INFO	    * rdrdesc;	    /* to check out ORIG table/ix    */
    bool              max_flag, min_flag, count_flag, qual_flag;

    /* start by perusing the qtree under the SAGG. It better be an orig 
    ** node
    */
    if (qenp == NULL || qenp->qen_type != QE_ORIG) return;
				/* we can only optimize very simple trees
				*/
    qual_flag = (qenp->node_qen.qen_orig.orig_qual && 
		qenp->node_qen.qen_orig.orig_qual->qen_ade_cx != NULL);
				/* later we care about whether there's
				** a where clause */
    max_flag = min_flag = count_flag = FALSE;	/* init rest of the flags */

    /* Now we examine the resdom list, looking for optimization
    ** potential. All we can take are count(*), count(<not null column>),
    ** max and/or min(<not null column>), or some combination of them
    ** all. Any other resdom entries precludes all optimization. If 
    ** we have a max/min function, there must be no where clause AND
    ** the orig node must define access to a Btree or ISAM index in 
    ** which the max/min column is left-most. Got all that? */

    /* We now permit max/min with where clause with NULL/NOTNULL    */
   
    for (rsdp = root->pst_left; rsdp != NULL && rsdp->pst_sym.pst_type == 
		PST_RESDOM; rsdp = rsdp->pst_left)
				/* loop over resdoms */
     if ((nodep = rsdp->pst_right)->pst_sym.pst_type == PST_AOP)
      switch (nodep->pst_sym.pst_value.pst_s_op.pst_opno) 
	{
	case 39 /*ADI_CNTAL_OP*/: 	/* count(*) */
	    if (count_flag) return;  /* only one of each agg */
	     else count_flag = TRUE;	/* flag count on */
	    break;

	case 38 /*ADI_CNT_OP*/: /* count(column) */
	    if (count_flag) return;  /* only one of each agg */
	     else goto merge;
	case 59 /*ADI_MAX_OP*/:	/* max(column) */
	    if (max_flag) return;
	     else goto merge;
	case 60 /*ADI_MIN_OP*/:	/* min(column) */
	    if (min_flag) return;
	  merge:
	    if ((varp = nodep->pst_left)->pst_sym.pst_type !=
		PST_VAR) return;  /* MUST be atomic column */

	    /* Check for not null columns - nullable isn't
	    ** allowed */
	    if (sq->ops_attrs.opz_base->opz_attnums[varp->
		pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->
		opz_dataval.db_datatype < 0 &&
	        nodep->pst_sym.pst_value.pst_s_op.pst_opno == 38) return;
				/* that was the test! */
	    if (nodep->pst_sym.pst_value.pst_s_op.pst_opno == 38)
		{		/* if count(column) ... */
		count_flag = TRUE;	/* set flag */
		break;		/* and back to loop */
		}
	    
	    /* Just max/min left now. First disallow max/min
	    **  in combination with where clause, then check for
	    **  index access. */
/*  We are allowing qual for max and min -- ramra01 */
/*	    if (qual) return;   */ /* no where with max/min opt */
	    valid = ahd->ahd_valid;
	    if ( !valid || valid->vl_dmr_cb != qenp->node_qen.qen_orig.orig_get)
		return;		/* should only be 1 QEF_VALID entry
				**  and it should be our orig guy */
	    rdrdesc = (RDR_INFO *)valid->vl_debug;
	    if (rdrdesc->rdr_rel->tbl_storage_type != DMT_ISAM_TYPE 
		&& rdrdesc->rdr_rel->tbl_storage_type != DMT_BTREE_TYPE)
		    return;	/* max/min MUST be on index structure */
	    if (rdrdesc->rdr_keys == NULL || MEcmp((PTR)
		varp->pst_sym.pst_value.pst_s_var.pst_atname.db_att_name,
		(PTR)rdrdesc->rdr_attr[rdrdesc->rdr_keys->key_array[0]]
		->att_name.db_att_name, DB_ATT_MAXNAME) != 0)
		   return;	/* this checks if max/min column is 
				** 1st column of index (comparing 
				** name of column in PST_VAR with 
				** name from attr array as indexed 
				** by 1st entry of key array) */

	    /* If we get this far, we've passed the tests! */
	    if (nodep->pst_sym.pst_value.pst_s_op.pst_opno == 59
		/*ADI_MAX_OP*/) max_flag = TRUE;
	     else min_flag = TRUE;	/* set correct flag */
	    break;

	default:		/* anything else */
	    return;		/* isn't allowed */
	}
      else return;  		/* rhs wasn't AOP (if that's 
				** possible) */

    /* If we got this far, something useful must have happened. Examine
    ** local flags and set ahd/qen flags accordingly. */

    if (count_flag) ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_CSTAROPT;
    if (max_flag) qenp->node_qen.qen_orig.orig_flag |= ORIG_MAXOPT;
    if (min_flag) qenp->node_qen.qen_orig.orig_flag |= ORIG_MINOPT;
    return;
}	/* end of opc_saggopt */

/*{
** Name: OPC_CPOSITION	- Can a replace or delete use DMF current of positioning
**
** Description:
**      The routine returns TRUE or FALSE, which indicates whether QEF can
**      use DMFs where current of positioning when doing replaces or deletes. 
**      If we return TRUE then where current of positioning can be used (which 
**      is faster for DMF). If we return FALSE, then QEF must position by tid 
**      when doing the replace or delete. 
**        
**      Whether we can position where current of depends on where the orig 
**      node for the result relation is. If it the outer most orig node, ie. 
**      it is not below the inner of any join nodes, and it is not below a 
**      SORT or a QP node then where current of can be used. This is because,
**      in the above cases, the row that the orig node is currently positioned 
**      on is the same as the row that is being replaced or deleted.  
[@comment_line@]...
**
** Inputs:
**	global -
**	    The query-wide state variable.
**	co -
**	    This is the root (or sub-root) of the CO tree in question.
**	lresno -
**	    This is the local range var number for the result relation.
**
** Outputs:
**	none
**  
**	Returns:
**	    TRUE or FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-oct-87 (eric)
**          written
**      22-sept-88 (eric)
**          Return FALSE if CO is NULL.
**      15-oct-88 (eric)
**          Added checks for opo_osort and opo_isort. If there are created
**          sort nodes then we can't use the positioned versions of 
**          replace and delete.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	5-may-99 (inkdo01)
**	    Hash joins interfere with positioning, too.
**	19-sep-2007 (toumi01)
**	    Keyset cursors (scrollable, updateable) can never update
**	    or delete based on dmf position.
[@history_template@]...
*/
static i4
opc_cposition(
	OPS_STATE	*global,
	OPO_CO		*co,
	OPV_IVARS	lresno)
{
    i4	    ret;

    ret = FALSE;

    if (global->ops_qheader->pst_mask1 & PST_SCROLL)
    {
	return(FALSE);
    }

    if (co == NULL)
    {
	return(FALSE);
    }

    switch (co->opo_sjpr)
    {
     case DB_ORIG:
	if (co->opo_union.opo_orig == lresno)
	{
	    ret = TRUE;
	}
	break;

     case DB_PR:
	if (co->opo_inner != NULL && co->opo_isort == FALSE)
	{
	    ret = opc_cposition(global, co->opo_inner, lresno);
	}
	else if (co->opo_osort == FALSE)
	{
	    ret = opc_cposition(global, co->opo_outer, lresno);
	}
	break;

     case DB_SJ:

	if (co->opo_osort == FALSE && 
		co->opo_variant.opo_local->opo_jtype != OPO_SJHASH)
	{
	    ret = opc_cposition(global, co->opo_outer, lresno);
	}
	break;
    }

    return (ret);
}

/*{
** Name: OPC_BTMLEFT_DMRCBNO	- Get the dmrcb num from the bottom left orig 
**					node
**
** Description:
**      This searches a given qen tree for the bottom left most orig node 
**      and returns the number of the dmr_cb that it uses. 
**
** Inputs:
**  global - 
**	Query wide state info.
**  qen -
**	The QEN tree to search.
**
** Outputs:
**
**	Returns:
**	    The number of the dmr_cb for the bottom left most orig node.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-oct-93 (eric)
**          Created
**	5-may-99 (inkdo01)
**	    Add the hash join.
[@history_template@]...
*/
static i4
opc_btmleft_dmrcbno(
	OPS_STATE   *global,
	QEN_NODE    * qen)
{
    /* iterate down the outer pointers of the nodes until we find an orig 
    ** node
    */
    while (qen != NULL && qen->qen_type != QE_ORIG)
    {
	switch (qen->qen_type)
	{
	 case QE_TJOIN:
	    qen = qen->node_qen.qen_tjoin.tjoin_out;
	    break;

	 case QE_KJOIN:
	    qen = qen->node_qen.qen_kjoin.kjoin_out;
	    break;

	 case QE_HJOIN:
	    qen = qen->node_qen.qen_hjoin.hjn_out;
	    break;

	 case QE_FSMJOIN:
	 case QE_ISJOIN:
	 case QE_CPJOIN:
	    qen = qen->node_qen.qen_sjoin.sjn_out;
	    break;

	 case QE_SEJOIN:
	    qen = qen->node_qen.qen_sejoin.sejn_out;
	    break;

	 case QE_SORT:
	    qen = qen->node_qen.qen_sort.sort_out;
	    break;

	 case QE_TSORT:
	    qen = qen->node_qen.qen_tsort.tsort_out;
	    break;

	 default:
	    qen = NULL;
	    break;
	}
    }

    if (qen != NULL && qen->qen_type == QE_ORIG)
	return(qen->node_qen.qen_orig.orig_get);
    else
	return(-1);
}

/*{
** Name: OPC_GTROWNO	- This routine gets the row number for a valid list 
**									entry
**
** Description:
**      This routine searchs the given QEN tree to find the DSH row number 
**      that will hold the tuple for a given valid list entry. 
**
**	It fills in the ahd_ruprow and ahd_ruporig fields of the passed
**	down QEP header.  ruprow will be used at qp setup time to properly
**	set the "where is the in-place row to be updated" stuff for
**	potential nested in-place updates/deletes.  ruporig is used
**	at execution time for the situation where the update needs to
**	know the current partition number and it doesn't have a tid
**	around.  (Nodes doing the fetch for an in-place update will
**	arrange to fill in the current partition number in that node's
**	QEN_STATUS state.)
[@comment_line@]...
**
** Inputs:
**	global -
**	    the query wide state variable for OPF
**	qhd -
**	    ptr to QEF_QEP info to be updated
**	qn -
**	    The top of the QEN tree (or sub-tree) to be searched
**	valid -
**	    The valid list entry that we want the row number for.
**
** Outputs:
**	non
**
**	Returns:
*	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-oct-89 (eric)
**          written
**	3-feb-04 (inkdo01)
**	    Set flags for positioned update/delete and removed out of
**	    date code while in the neighbourhood.
**	18-Jul-2005 (schka24)
**	    RUP needs to get at originator's partition number, rearrange
**	    so that we fill in originator node number as well as row.
**	10-Apr-2007 (kschendel) b122118
**	    Remove PUPDDEL flag setting, was replaced by ruporig machinery.
[@history_template@]...
*/
static void
opc_gtrowno(
	OPS_STATE	*global,
	QEF_QEP		*qhd,
	QEN_NODE	*qn,
	QEF_VALID	*valid )
{

    switch (qn->qen_type)
    {
     case QE_TJOIN:
	if (valid->vl_dmr_cb == qn->node_qen.qen_tjoin.tjoin_get)
	{
	    qhd->ahd_ruprow = qn->qen_row;
	    qhd->ahd_ruporig = qn->qen_num;
	}
	else
	{
	    opc_gtrowno(global, qhd, qn->node_qen.qen_tjoin.tjoin_out, valid);
	}
	break;

     case QE_KJOIN:
	if (valid->vl_dmr_cb == qn->node_qen.qen_kjoin.kjoin_get)
	{
	    qhd->ahd_ruprow = qn->qen_row;
	    qhd->ahd_ruporig = qn->qen_num;
	}
	else
	{
	    opc_gtrowno(global, qhd, qn->node_qen.qen_kjoin.kjoin_out, valid);
	}
	break;

     case QE_ORIG:
	if (valid->vl_dmr_cb == qn->node_qen.qen_orig.orig_get)
	{
	    qhd->ahd_ruprow = qn->qen_row;
	    qhd->ahd_ruporig = qn->qen_num;
	}
	break;

     case QE_SORT:
	opc_gtrowno(global, qhd, qn->node_qen.qen_sort.sort_out, valid);
	break;

     case QE_TSORT:
	opc_gtrowno(global, qhd, qn->node_qen.qen_tsort.tsort_out, valid);
	break;

     case QE_SEJOIN:
	opc_gtrowno(global, qhd, qn->node_qen.qen_sejoin.sejn_out, valid);
	break;

     /* Other node types can't be used for in-place updating. */
    }

    return;
}
/*{
** Name: OPC_CHKTARGET	- Perform some consistency checks on target lists
**
** Description:
**      This routine performs some consistency checks on targets lists. We 
**      are primarily interested in whether the pst_ttargtype and pst_ntargno 
**      fields are filled in correctly. 
**
**      This routine has been "#ifdef xDEV_TEST"ed and all calls to it
**      should also be so ifdefed. 
[@comment_line@]...
**
** Inputs:
**  global -
**	the query wide state variable
**  cnode -
**	OPC's info in the best CO tree
**  root -
**	The root of the query tree.
**
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    only on error
**
** Side Effects:
**	    Will generate errors if the target list is bad.
**
** History:
**      16-aug-88 (eric)
**          created
**      16-oct-90 (jennifer)
**          Add PST_HIDDEN to opc_chktarget routine.
[@history_template@]...
*/
#ifdef xDEV_TEST
static VOID
opc_chktarget(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root )
{
    OPS_SUBQUERY    *sq = global->ops_cstate.opc_subqry;
    QEF_AHD	    *ahd = sq->ops_compile.opc_ahd;
    PST_QNODE	    *resdom;

    for (resdom = root->pst_left; 
	    resdom->pst_sym.pst_type == PST_RESDOM;
	    resdom = resdom->pst_left
	)
    {
	switch (ahd->ahd_atype)
	{
	 case QEA_APP:
	 case QEA_PROJ:
	 case QEA_AGGF:
	 case QEA_UPD:
	 case QEA_DEL:
	 case QEA_RUP:
	 case QEA_LOAD:
	 case QEA_PUPD:
	 case QEA_PDEL:
	    switch (root->pst_left->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
	    {
	     case PST_ATTNO:
	     case PST_HIDDEN:
		break;

	     default:
		opx_error(E_OP0897_BAD_CONST_RESDOM);
	    }
	    break;

	 case QEA_SAGG:
	 case QEA_RAGGF:
	 case QEA_GET:
	    switch (root->pst_left->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype)
	    {
	     case PST_ATTNO:
	     case PST_LOCALVARNO:
	     case PST_RQPARAMNO:
	     case PST_USER:
	     case PST_HIDDEN:
		break;

	     default:
		opx_error(E_OP0897_BAD_CONST_RESDOM);
	    }
	    break;
	}
    }

}
#endif

/*{
** Name: OPC_DMUAHD	- Build a single DMU action header.
**
** Description:
**	DMU action headers are built for CREATE TABLE AS SELECT actions.
**	This statement requires a number of DMU operations to occur
**	within the context of the CREATE TABLE statement.  Each DMU
**	operation will get its own action header.  The possibilities
**	are CREATE and MODIFY.
**
**	DMU action headers can also be used when temporary tables are
**	needed.  In that case the action code is DMT_CREATE_TEMP, and
**	the action header is built by opc-dmtahd -- not this routine.
**	(Note: "temporary table" here means OPF requested temporary
**	table.  Session temps - DGTT's - are regular tables with the
**	DMU_TEMPORARY characterstic, as far as we're concerned.)
**
**	Standalone CREATE and MODIFY (and CREATE INDEX, and DROP, etc)
**	do not create DMU action headers.  Standalone CREATE TABLE builds
**	a CREATE action header (see elsewhere in opc), while the other
**	statements do not flow through OPC at all.  They are sent from
**	the parser directly to QEF to a QEU function.
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
**      31-jul-86 (eric)
**          written
**      1-sep-88 (seputis)
**          fix unix lint problems
**	26-jun-89 (jrb)
**	    Fixed bug where too little memory being allocated.
**      16-oct-90 (jennifer)
**          Check for PST_HIDDEN before pst_print == FALSE when
**          counting attributes.
**	25-mar-1991 (bryanp)
**	    Added new input argument 'index_compressed' to indicate whether or
**	    not index compression is in use -- values are DMU_C_ON|DMU_C_OFF.
**	    This value is only used if dmu_func is DMU_MODIFY_TABLE.
**	30-March-1992 (rmuth)
**	    File extend Project. Two new fields that can be passed in the
**	    pst_restab structure these are allocation and extend. They
**	    are built into the dmu_cb characteristics array.
**	31-mar-1992 (bryanp)
**	    Temporary table enhancements. set tbl_temporary in the rdr_rel,
**	    and conditionally build a DMU_TEMP_TABLE characteristic.
**      04-Jan-1993 (fred)
**          Fixup large object attribute specifications when creating
**          tables which contain large objects.  As my contribution to
**          code cleanup, I also coalesced to two loops which formerly
**          traversed the same target list, one building the dmucb's
**          attribute list, the other building a fake rdf description.
**          Since I added code which makes calls to ADF to determine
**          things about peripheral datatypes, duplicating these
**          seemed bad, so I merged the two loops.  Why they were
**          separate originally remains an unanswered question.
**      22-feb-1993 (rblumer)
**          Change code building up attribute entries to get the default value
**          from the new pst_defid field of the resdom in the query tree,
**          rather than using the nullability of the datatype and the query
**          language. 
**	18-jun-93 (robf)
**	    Secure 2.0:
**	    - Add support for row security label/audit key attributes
**      13-aug-93 (rblumer)
**          The DMT_F_NDEFAULT flag was being overwritten in opc_dmuahd;
**          change other bits to use |= so as not to overwrite existing bits.
**	    This fixes 'internal error' in bug b54185.
**      5-oct-93 (eric)
**          Added support for tables on resource lists.
**	30-apr-96 (inkdo01)
**	    Fix for attribute array induced SEGV (array overrun).
**	05-jan-97 (pchang)
**	    When filling in fake RDF entries for result attributes, a failure
**	    to skip non-printing attributes caused OPF memory overrun. (B80560)
**	9-Feb-2004 (schka24)
**	    Add partitioning support, some doc.
**	10-mar-04 (inkdo01)
**	    Hack to fill in partitioning column row_offset values for "create
**	    table ... as select ..." statements.
**	1-Jul-2004 (schka24)
**	    Adjust for seclabel removal from char array.
**	27-Jun-2005 (schka24)
**	    Pass allocate and extend to table-create operation.  There may not
**	    be any modify taking place, and anyway the parameters should be in
**	    effect during the initial table load.
**	23-Nov-2005 (kschendel)
**	    Suppress allocation= on create if we're going to do a modify,
**	    because the modify needs to allocate as well (for best disk file
**	    layout) and allocating on the create is a waste.
**	28-apr-06 (dougi)
**	    Assign att_collID for "create table as ...".
**	05-May-2006 (jenjo02)
**	    Handle pst_clustered, TRUE if CLUSTERED Btree.
**	20-Dec-2006 (kschendel)
**	    Update partition def types from latest resdom info, outer join
**	    analysis could make a result column nullable.
**	16-Jan-2007 (kschendel)
**	    Fix stupid in last change:  automatic partitioning doesn't have
**	    any column info, caused segv.
*/
static QEF_AHD *
opc_dmuahd(
	OPS_STATE   *global,
	i4	    dmu_func,
	PST_QTREE   *qtree,
	OPV_IGVARS  gresvno,
	i4	    structure,
	i4	    compressed,
	i4	    index_compressed)
{
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd;
    DMU_CB		*dmucb;
    DMU_CB		*psf_dmucb;	/* parser constructed DMU_CB */
    PST_QNODE		*resdom;
    QEU_CB		*qeucb;		/* Parser constructed QEU_CB */
    DMF_ATTR_ENTRY	**attrs;
    DMF_ATTR_ENTRY	*attr;
    DMT_ATT_ENTRY	*att;
    DMU_KEY_ENTRY	**keys;
    DMU_KEY_ENTRY	*key;
    DMU_CHAR_ENTRY	*chars;
    RDR_INFO		*rel;
    i4		tupsize;
    i4			attno;
    i4			hidden_atts = 0;
    OPV_GRV		*grv;
    i4			char_no;
    i4			keyno;
    PST_RESKEY		*reskey;
    OPC_PST_STATEMENT	*opc_pst;
    QEF_RESOURCE	*resource;
    i4			char_array_idx;

    if (structure == DB_SORT_STORE)
    {
	structure = DB_HEAP_STORE;
    }

    /* First, lets allocate and init a DMU_CB. To start with, we will do the
    ** stuff common to create and modify.
    */
    dmucb = (DMU_CB *) opu_qsfmem(global, sizeof (DMU_CB));
    dmucb->type = DMU_UTILITY_CB;
    dmucb->length = sizeof (DMU_CB);
    dmucb->dmu_tran_id = 0;
    dmucb->dmu_flags_mask = 0;
    dmucb->dmu_db_id = global->ops_cb->ops_dbid;
    STRUCT_ASSIGN_MACRO(qtree->pst_restab.pst_resname, dmucb->dmu_table_name);
    STRUCT_ASSIGN_MACRO(qtree->pst_restab.pst_resown, dmucb->dmu_owner);
    STRUCT_ASSIGN_MACRO(qtree->pst_restab.pst_resloc, dmucb->dmu_location);
    if (dmucb->dmu_location.data_in_size != 0)
    {
	dmucb->dmu_location.data_address = 
	    (char *) opu_qsfmem(global, (i4)dmucb->dmu_location.data_in_size);
	MEcopy((PTR)qtree->pst_restab.pst_resloc.data_address, 
			dmucb->dmu_location.data_in_size,
			    (PTR)dmucb->dmu_location.data_address);
    }

    /* Get the partition definition, if any, for this table.
    ** Notice that PSF has constructed a QEU_CB and DMU_CB and attached
    ** them to the query tree header.  Perhaps someday the parser will
    ** fill in more "stuff" and reduce the amount of work needed above.
    ** For now, the only thing that should be relied on in the parser
    ** QEU_CB and DMU_CB is the partitioning info.
    ** QUEL doesn't create/store the qeucb, so be careful.
    */
    psf_dmucb = NULL;
    qeucb = qtree->pst_qeucb;
    if (qeucb != NULL)
	psf_dmucb = (DMU_CB *) qeucb->qeu_d_cb;

    if (dmu_func == DMU_CREATE_TABLE)
    {
	i4	attr_array_sz = 0;

	/* now lets allocate and init an RDR_INFO struct for this 
	** table being created.
	*/
	rel = (RDR_INFO *) opu_memory(global, sizeof (RDR_INFO));
	MEfill(sizeof (RDR_INFO), (u_char)0, (PTR)rel);
	rel->rdr_rel = 
		(DMT_TBL_ENTRY *) opu_memory(global, sizeof (DMT_TBL_ENTRY));
	MEfill(sizeof (DMT_TBL_ENTRY), (u_char)0, (PTR)rel->rdr_rel);
	STRUCT_ASSIGN_MACRO(qtree->pst_restab.pst_resname, 
						    rel->rdr_rel->tbl_name);
	STRUCT_ASSIGN_MACRO(qtree->pst_restab.pst_resown, 
						    rel->rdr_rel->tbl_owner);
	STRUCT_ASSIGN_MACRO(*((DB_LOC_NAME *) qtree->
	    pst_restab.pst_resloc.data_address), rel->rdr_rel->tbl_location);
	rel->rdr_rel->tbl_attr_count = 0;
	for (resdom = qtree->pst_qtree->pst_left;
		resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
		resdom = resdom->pst_left
	    )
	{
	    if (attr_array_sz < resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
		    attr_array_sz = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    if ((resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype !=	PST_HIDDEN )
                && !(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		continue;
	    }
	    rel->rdr_rel->tbl_attr_count += 1;
	    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
		PST_HIDDEN)
	    {
		hidden_atts++;
	    }
	}
	rel->rdr_rel->tbl_storage_type = DMT_HEAP_TYPE;
	rel->rdr_rel->tbl_temporary = qtree->pst_restab.pst_temporary;
	global->ops_rangetab.opv_base->
				opv_grv[sq->ops_result]->opv_relation = rel;

	/* Now lets finish filling in the dmucb with the create specific
	** information.
	*/
	dmucb->dmu_qry_id.db_qry_high_time = 0;
	dmucb->dmu_qry_id.db_qry_low_time = 0;
	dmucb->dmu_key_array.ptr_address = NULL;
	dmucb->dmu_key_array.ptr_size = 0;
	dmucb->dmu_key_array.ptr_in_count = 0;

	/* *** WARNING *** hardcoded attr count here.
	** Dups, journaling, sec-audit, sys-maintained.
	** Plus temporary if gtt, plus page-size if given.
	** Also include extend= and allocate= if given.
	*/
	if (qtree->pst_restab.pst_temporary)
	    dmucb->dmu_char_array.data_in_size = 6 * sizeof (DMU_CHAR_ENTRY);
	else
	    dmucb->dmu_char_array.data_in_size = 4 * sizeof (DMU_CHAR_ENTRY);

	if (qtree->pst_restab.pst_page_size)
	    dmucb->dmu_char_array.data_in_size += sizeof (DMU_CHAR_ENTRY);
	if (qtree->pst_restab.pst_alloc > 0
	  && (structure == DB_HEAP_STORE && compressed == DMU_C_OFF))
	    dmucb->dmu_char_array.data_in_size += sizeof (DMU_CHAR_ENTRY);
	if (qtree->pst_restab.pst_extend > 0)
	    dmucb->dmu_char_array.data_in_size += sizeof (DMU_CHAR_ENTRY);

	dmucb->dmu_char_array.data_address = 
	   (char *) opu_qsfmem(global, (i4)dmucb->dmu_char_array.data_in_size);
	chars = (DMU_CHAR_ENTRY *) dmucb->dmu_char_array.data_address;
	char_array_idx = 0;
	chars[char_array_idx].char_id = DMU_DUPLICATES;
	if (qtree->pst_restab.pst_resdup == TRUE)
	{
	    chars[char_array_idx].char_value = DMU_C_ON;
	}
	else
	{
	    chars[char_array_idx].char_value = DMU_C_OFF;
	}
	char_array_idx++;
	/* Beware here:  both "on" and "later" compile to DMF journaling ON */
	chars[char_array_idx].char_id = DMU_JOURNALED;
	if (qtree->pst_restab.pst_resjour == PST_RESJOUR_OFF)
	{
	    chars[char_array_idx].char_value = DMU_C_OFF;
	}
	else
	{
	    chars[char_array_idx].char_value = DMU_C_ON;
	}
	char_array_idx++;
	chars[char_array_idx].char_id = DMU_ROW_SEC_AUDIT;
	if (qtree->pst_restab.pst_flags&PST_ROW_SEC_AUDIT)
	    chars[char_array_idx].char_value = DMU_C_ON;
	else
	    chars[char_array_idx].char_value = DMU_C_OFF;

	char_array_idx++;
	chars[char_array_idx].char_id = DMU_SYS_MAINTAINED;
	if (qtree->pst_restab.pst_flags&PST_SYS_MAINTAINED)
	    chars[char_array_idx].char_value = DMU_C_ON;
	else
	    chars[char_array_idx].char_value = DMU_C_OFF;

	if (qtree->pst_restab.pst_temporary)
	{
	    char_array_idx++;
	    chars[char_array_idx].char_id    = DMU_TEMP_TABLE;
	    chars[char_array_idx].char_value = DMU_C_ON;
	    char_array_idx++;
	    chars[char_array_idx].char_id    = DMU_RECOVERY;
	    chars[char_array_idx].char_value = 
				    (qtree->pst_restab.pst_recovery ? DMU_C_ON :
				    DMU_C_OFF);
	   
	}

	if (qtree->pst_restab.pst_page_size)
	{
	    char_array_idx++;
	    chars[char_array_idx].char_id    = DMU_PAGE_SIZE;
	    chars[char_array_idx].char_value = qtree->pst_restab.pst_page_size;
	}
	/* Only do create-time allocation if uncompressed heap because in
	** that case there will not be any modify later on.
	*/
	if (qtree->pst_restab.pst_alloc > 0
	  && (structure == DB_HEAP_STORE && compressed == DMU_C_OFF))
	{
	    char_array_idx++;
	    chars[char_array_idx].char_id    = DMU_ALLOCATION;
	    chars[char_array_idx].char_value = qtree->pst_restab.pst_alloc;
	}
	if (qtree->pst_restab.pst_extend)
	{
	    char_array_idx++;
	    chars[char_array_idx].char_id    = DMU_EXTEND;
	    chars[char_array_idx].char_value = qtree->pst_restab.pst_extend;
	}

	/* fill in the list of attributes to create */
	dmucb->dmu_attr_array.ptr_in_count =
	    rel->rdr_rel->tbl_attr_count - hidden_atts;
	dmucb->dmu_attr_array.ptr_size = sizeof (DMF_ATTR_ENTRY);
	dmucb->dmu_attr_array.ptr_address = 
		    (PTR) opu_qsfmem(global, (i4)(sizeof (DMF_ATTR_ENTRY *) * 
					dmucb->dmu_attr_array.ptr_in_count));
	attrs = (DMF_ATTR_ENTRY **) dmucb->dmu_attr_array.ptr_address;

	/* Now let's fill in the array of attributes for our descriptor. */
	if (rel->rdr_rel->tbl_attr_count > attr_array_sz)
		attr_array_sz = rel->rdr_rel->tbl_attr_count;
				/* assure attr array is big enough */
	rel->rdr_attr = (DMT_ATT_ENTRY **) opu_memory(global,
						      (i4) 
                                          (sizeof (DMT_ATT_ENTRY *) *
					   (attr_array_sz + 1)));
	rel->rdr_attr[0] = NULL;

	for (	resdom = qtree->pst_qtree->pst_left;
	        resdom != NULL && resdom->pst_sym.pst_type != PST_TREE; 
		resdom = resdom->pst_left
	    )
	{
	    /* Skip non-printing attributes */

	    if (!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
		continue;

	    /* First, fill in the fake RDF entry for this attribute */

	    rel->rdr_attr[resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno] =
		att = 
		(DMT_ATT_ENTRY *) opu_memory(global, sizeof (DMT_ATT_ENTRY));

	    MEcopy((PTR)resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
			sizeof(att->att_name.db_att_name), 
			(PTR)att->att_name.db_att_name);
	    att->att_number = 1;
	    att->att_type   = resdom->pst_sym.pst_dataval.db_datatype;
	    att->att_width  = resdom->pst_sym.pst_dataval.db_length;
	    att->att_prec   = resdom->pst_sym.pst_dataval.db_prec;
	    att->att_collID = resdom->pst_sym.pst_dataval.db_collID;
	    att->att_key_seq_number = 0;
	    att->att_flags   = 0;
	    att->att_default = NULL;

	    /* copy default id from resdom
	     */
	    COPY_DEFAULT_ID(resdom->pst_sym.pst_value.pst_s_rsdm.pst_defid,
			    att->att_defaultID);

	    if (EQUAL_CANON_DEF_ID(att->att_defaultID,
				   DB_DEF_NOT_DEFAULT))
	    {
		att->att_flags |= DMU_F_NDEFAULT;
	    }

	    if (att->att_type > 0)
	    {
		att->att_flags |= DMU_F_KNOWN_NOT_NULLABLE;
	    }
	    /*
	    ** Copy over any special flags for the resdom, mainly
	    ** for hidden or system maintained
	    */
	    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags!=0)
	    {
		/*
		** The dmuflags should match exactly with the DMF attribute
		** flags required. 
		** The DMU_F_LEGAL_ATTR_BITS mask is a hack to remove
		** excess bits, really code should be cleaned up to
		** ensure only legal bits get set
		*/
		att->att_flags|= 
			(resdom->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags &
				DMU_F_LEGAL_ATTR_BITS);
	    }
	    

	    if ((att->att_width == ADE_LEN_UNKNOWN) ||
		(att->att_width == ADE_LEN_LONG))
	    {
		DB_STATUS         status;
		i4           bits;
		DB_DATA_VALUE     pdv;
		DB_DATA_VALUE     rdv;

		status = adi_dtinfo(global->ops_adfcb,
				    att->att_type,
				    &bits);

		if (bits & AD_PERIPHERAL)
		{
		    pdv.db_datatype = att->att_type;
		    pdv.db_length = 0;
		    pdv.db_prec = 0;
		    STRUCT_ASSIGN_MACRO(pdv, rdv);

		    status = adc_lenchk(global->ops_adfcb,
					TRUE,
					&pdv,
					&rdv);
		    att->att_width = rdv.db_length;
		    att->att_prec = rdv.db_prec;
		    att->att_flags |= DMU_F_PERIPHERAL;
		}
	    }

	    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype ==
		PST_HIDDEN)
	    {
		att->att_flags |= DMU_F_HIDDEN;

		/*	
		**	Skip hidden attributes for table creation 
		**      because DMU_CREATE_TABLE will always supply
		**      them.
		*/
		continue;
	    }

	    if (!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
	    {
		continue;
	    }

	    attr =
		attrs[resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno
		    - hidden_atts - 1] = 
		(DMF_ATTR_ENTRY *) opu_qsfmem(global, sizeof (DMF_ATTR_ENTRY));

	    MEcopy((PTR)resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
			sizeof(attr->attr_name.db_att_name), 
			(PTR)attr->attr_name.db_att_name);
	    attr->attr_type      = att->att_type;
	    attr->attr_size      = att->att_width;
	    attr->attr_precision = att->att_prec;
	    attr->attr_collID    = att->att_collID;
	    attr->attr_flags_mask =att->att_flags;
	    attr->attr_defaultTuple = (DB_IIDEFAULT *) NULL;

	    /* copy default id from resdom
	     */
	    COPY_DEFAULT_ID(resdom->pst_sym.pst_value.pst_s_rsdm.pst_defid,
			    attr->attr_defaultID);

	    if (EQUAL_CANON_DEF_ID(attr->attr_defaultID,
				   DB_DEF_NOT_DEFAULT))
	    {
		attr->attr_flags_mask |= DMU_F_NDEFAULT;
	    }

	    if (attr->attr_type > 0)
	    {
		attr->attr_flags_mask |= DMU_F_KNOWN_NOT_NULLABLE;
	    }
	}  /* end for resdom->pst_sym.pst_type != PST_TREE */

	tupsize = 0;
	for (attno = 1; attno <= rel->rdr_rel->tbl_attr_count; attno += 1)
	{
	    rel->rdr_attr[attno]->att_offset = tupsize;
	    tupsize += rel->rdr_attr[attno]->att_width;
	}
	rel->rdr_rel->tbl_width = tupsize;

	/* Attach partitioning info so that the CREATE step knows to create a
	** partitioned table.  The DMUCB gets a full copy since that's what
	** QEF will use to write all the partitioning catalogs from.  The
	** faked up RDF CB will get a trimmed copy just in case someone else
	** in opf looks there for partitioning info.
	** Watch out for QUEL, doesn't attach qeu/dmu to header..
	*/
	if (psf_dmucb != NULL && psf_dmucb->dmu_part_def != NULL)
	{
	    DMT_PHYS_PART *pp_array;
	    DB_PART_DIM	*wdimp;
	    DB_PART_LIST *part_entryp;
	    i4 nparts;
	    i4	i, j;

	    /* If this is "create table ... as select ...", we must 
	    ** first fill "row_offset" values into the faked up
	    ** partitioning definitions.
	    ** Update type and length too, might be nullability discrepancy
	    ** caused by outer join analysis which isn't complete until
	    ** the optimizer is done with the query.
	    ** FIXME I'm pretty sure that the only way we can make a
	    ** partition-break column non-nullable from nullable is if
	    ** the partition breaks had an explicit NULL that shouldn't
	    ** have been there.  We could throw the error here, except I'm
	    ** not QUITE sure enough that that's the only case...
	    */
	    if (sq->ops_mode == PSQ_RETINTO)
	    {
		for (i = 0; i < psf_dmucb->dmu_part_def->ndims; i++)
		{
		    wdimp = &psf_dmucb->dmu_part_def->dimension[i];
		    if (wdimp->distrule == DBDS_RULE_AUTOMATIC)
			continue;		/* Skip auto dims */
		    for (part_entryp = &wdimp->part_list[wdimp->ncols-1];
			 part_entryp >= &wdimp->part_list[0];
			 --part_entryp)
		    {
			att = rel->rdr_attr[part_entryp->att_number];
			part_entryp->row_offset = att->att_offset;
			part_entryp->type = att->att_type;
			part_entryp->length = att->att_width;
		    }
		}
	    }

	    opc_full_partdef_copy(global, psf_dmucb->dmu_part_def,
			psf_dmucb->dmu_partdef_size,
			&dmucb->dmu_part_def);
	    dmucb->dmu_partdef_size = psf_dmucb->dmu_partdef_size;
	    opc_partdef_copy(global, psf_dmucb->dmu_part_def,
			opu_memory, &rel->rdr_parts);
	    /* That's just a start!  In order to make the fake RDF info
	    ** look right, we have to create a fake PP array as if it
	    ** came back from dmt-show.
	    */
	    nparts = rel->rdr_parts->nphys_parts;
	    rel->rdr_rel->tbl_nparts = nparts;
	    rel->rdr_rel->tbl_ndims = rel->rdr_parts->ndims;
	    rel->rdr_rel->tbl_status_mask |= DMT_IS_PARTITIONED;
	    pp_array = (DMT_PHYS_PART *) opu_memory(global,
				nparts * sizeof(DMT_PHYS_PART));
	    rel->rdr_pp_array = pp_array;
	    do
	    {
		/* We don't know the tabid's yet, they aren't created */
		pp_array->pp_tabid.db_tab_base = pp_array->pp_tabid.db_tab_index = 0;
		pp_array->pp_reltups = 0;
		pp_array->pp_relpages = 1;
		++ pp_array;
	    } while (-- nparts > 0);
	}
    }
    else if (dmu_func == DMU_MODIFY_TABLE)
    {
	if (structure == DB_HEAP_STORE)
	{
	    dmucb->dmu_key_array.ptr_address = NULL;
	    dmucb->dmu_key_array.ptr_size = 0;
	    dmucb->dmu_key_array.ptr_in_count = 0;
	}
	else
	{
	    if (qtree->pst_restab.pst_reskey == NULL)
	    {
		dmucb->dmu_key_array.ptr_in_count = 1;
	    }
	    else
	    {
		dmucb->dmu_key_array.ptr_in_count = 0;
		for (reskey = qtree->pst_restab.pst_reskey; 
			reskey != NULL; 
			reskey = reskey->pst_nxtkey
		    )
		{
		    dmucb->dmu_key_array.ptr_in_count += 1;
		}
	    }

	    dmucb->dmu_key_array.ptr_address = 
			opu_qsfmem(global, (i4)(sizeof (DMU_KEY_ENTRY *) * 
					    dmucb->dmu_key_array.ptr_in_count));
	    keys = (DMU_KEY_ENTRY **) dmucb->dmu_key_array.ptr_address;
	    dmucb->dmu_key_array.ptr_size = sizeof (DMU_KEY_ENTRY);

	    if (qtree->pst_restab.pst_reskey == NULL)
	    {
		for (resdom = qtree->pst_qtree->pst_left;
			(resdom != NULL && 
			    resdom->pst_sym.pst_type == PST_RESDOM &&
			    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno != 1
			);
			resdom = resdom->pst_left
		    )
		{
		    /* NULL body */
		}
		key = (DMU_KEY_ENTRY *)opu_qsfmem(global,sizeof(DMU_KEY_ENTRY));

		MEcopy((PTR)resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
			sizeof(key->key_attr_name.db_att_name), 
			(PTR)key->key_attr_name.db_att_name);
		key->key_order = DMU_ASCENDING;
		keys[0] = key;
	    }
	    else
	    {
		for (reskey = qtree->pst_restab.pst_reskey, keyno = 0;
			reskey != NULL; 
			reskey = reskey->pst_nxtkey, keyno += 1
		    )
		{
		    key = (DMU_KEY_ENTRY *) 
				opu_qsfmem(global, sizeof (DMU_KEY_ENTRY));

		    MEcopy((PTR)reskey->pst_attname.db_att_name, 
				sizeof(key->key_attr_name.db_att_name), 
				(PTR)key->key_attr_name.db_att_name);
		    key->key_order = DMU_ASCENDING;
		    keys[keyno] = key;
		}
	    }
	}

	dmucb->dmu_attr_array.ptr_address = NULL;
	dmucb->dmu_attr_array.ptr_size = 0;
	dmucb->dmu_attr_array.ptr_in_count = 0;

	/*
	** Characteristics 'structure', 'compressed', and 'index compressed'
	** are always provided. Others may or may not be.
	*/
	char_no = 3;
	if (qtree->pst_restab.pst_fillfac != 0)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_leaff != 0)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_nonleaff != 0)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_minpgs != 0)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_maxpgs != 0)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_alloc != 0 )
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_extend !=0 )
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_temporary)
	{
	    char_no += 2;
	}
	if (qtree->pst_restab.pst_page_size)
	{
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_clustered)
	{
	    char_no += 1;
	}

	dmucb->dmu_char_array.data_in_size = char_no * sizeof (DMU_CHAR_ENTRY);
	dmucb->dmu_char_array.data_address = (char *) opu_qsfmem(global, 
	    (i4)(dmucb->dmu_char_array.data_in_size));
	chars = (DMU_CHAR_ENTRY *) dmucb->dmu_char_array.data_address;
	chars[0].char_id = DMU_STRUCTURE;
	chars[0].char_value = structure;
	chars[1].char_id = DMU_COMPRESSED;
	chars[1].char_value = compressed;
	chars[2].char_id = DMU_INDEX_COMP;
	chars[2].char_value = index_compressed;

	char_no = 3;

	if (qtree->pst_restab.pst_fillfac != 0)
	{
	    chars[char_no].char_id = DMU_DATAFILL;
	    chars[char_no].char_value = qtree->pst_restab.pst_fillfac;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_leaff != 0)
	{
	    chars[char_no].char_id = DMU_LEAFFILL;
	    chars[char_no].char_value = qtree->pst_restab.pst_leaff;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_nonleaff != 0)
	{
	    chars[char_no].char_id = DMU_IFILL;
	    chars[char_no].char_value = qtree->pst_restab.pst_nonleaff;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_minpgs > 0)
	{
	    chars[char_no].char_id = DMU_MINPAGES;
	    chars[char_no].char_value = qtree->pst_restab.pst_minpgs;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_maxpgs > 0)
	{
	    chars[char_no].char_id = DMU_MAXPAGES;
	    chars[char_no].char_value = qtree->pst_restab.pst_maxpgs;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_alloc > 0)
	{
	    chars[char_no].char_id = DMU_ALLOCATION;
	    chars[char_no].char_value = qtree->pst_restab.pst_alloc;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_extend > 0)
	{
	    chars[char_no].char_id = DMU_EXTEND;
	    chars[char_no].char_value = qtree->pst_restab.pst_extend;
	    char_no += 1;
	}
	if (qtree->pst_restab.pst_temporary)
	{
	    chars[char_no].char_id    = DMU_TEMP_TABLE;
	    chars[char_no].char_value = DMU_C_ON;
	    chars[char_no+1].char_id    = DMU_RECOVERY;
	    chars[char_no+1].char_value = (qtree->pst_restab.pst_recovery ?
				DMU_C_ON : DMU_C_OFF);
	    char_no += 2;
	}
	if (qtree->pst_restab.pst_page_size)
	{
	    chars[char_no].char_id    = DMU_PAGE_SIZE;
	    chars[char_no].char_value = qtree->pst_restab.pst_page_size;
	    char_no += 1;
	}
	if ( qtree->pst_restab.pst_clustered )
	{
	    chars[char_no].char_id    = DMU_CLUSTERED;
	    chars[char_no].char_value = DMU_C_ON;
	    char_no += 1;
	}
	/* The MODIFY step of create-as-select does not get any partitioning
	** info.  It will not be a partition-changing MODIFY, it will simply
	** restructure the partitions individually (if the table is partitioned
	** at all.)
	*/
	dmucb->dmu_part_def = NULL;
    }
    else
    {
	opx_error(E_OP0881_DMUFUNC);
    }

    /* Now, lets allocate the action header */
    ahd = sq->ops_compile.opc_ahd = 
			    (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DMU));
    sq->ops_compile.opc_bgpostfix = sq->ops_compile.opc_endpostfix = NULL;

    /* if this is the first action header that we have created then lets
    ** remember it.
    */
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DMU);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_DMU;
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    ahd->ahd_valid = NULL;

    ahd->qhd_obj.qhd_dmu.ahd_func = dmu_func;
    ahd->qhd_obj.qhd_dmu.ahd_cb = (PTR) dmucb;

    /* maintain the linked list of valid structs that hang off resource
    ** structs.
    */
    grv = global->ops_rangetab.opv_base->opv_grv[gresvno];
    resource = opc_get_resource(global, grv,
		(i4 *) NULL, (PTR *) NULL, (QEF_MEM_CONSTTAB **) NULL);
    if (resource == (QEF_RESOURCE *) NULL)
    {
	ahd->qhd_obj.qhd_dmu.ahd_alt = NULL;
	grv->opv_ptrvalid = &ahd->qhd_obj.qhd_dmu.ahd_alt;	
    }
    else
    {
	ahd->qhd_obj.qhd_dmu.ahd_alt = resource->qr_resource.qr_tbl.qr_valid;
	grv->opv_ptrvalid = (QEF_VALID **) NULL;
    }

    /* If the table was partitioned, and there were partition specific
    ** physical storage options, the parse left them in a linked list hung
    ** from the parser qeu-cb.  We're not passing that data block to QEF,
    ** so we need to connect the partitioning storage instructions to the
    ** action header instead.
    ** This is only done for the CREATE step.  Once created, the modify
    ** step does not need to fool with the partitioning.
    ** Watch out for QUEL, doesn't set any of this up.
    */
    ahd->qhd_obj.qhd_dmu.ahd_logpart_list = NULL;
    if (dmu_func == DMU_CREATE_TABLE && qeucb != NULL)
    {
	QEU_LOGPART_CHAR *ahd_lpl_ptr;	/* Copied entry */
	QEU_LOGPART_CHAR *psf_lpl_ptr;	/* Original psf-generated entry */

	psf_lpl_ptr = qeucb->qeu_logpart_list;
	/* This loop happens to reverse the order of the list but it
	** doesn't make any difference to anything.
	*/
	while (psf_lpl_ptr != NULL)
	{
	    ahd_lpl_ptr = (QEU_LOGPART_CHAR *) opu_qsfmem(global,
			sizeof(QEU_LOGPART_CHAR));
	    MEcopy(psf_lpl_ptr, sizeof(QEU_LOGPART_CHAR), ahd_lpl_ptr);
	    allocate_and_copy_a_dm_data(global,
			&psf_lpl_ptr->loc_array, &ahd_lpl_ptr->loc_array);
	    allocate_and_copy_a_dm_data(global,
			&psf_lpl_ptr->char_array, &ahd_lpl_ptr->char_array);
	    allocate_and_copy_a_dm_ptr(global, 0,
			&psf_lpl_ptr->key_array, &ahd_lpl_ptr->key_array);
	    ahd_lpl_ptr->next = ahd->qhd_obj.qhd_dmu.ahd_logpart_list;
	    ahd->qhd_obj.qhd_dmu.ahd_logpart_list = ahd_lpl_ptr;
	    psf_lpl_ptr = psf_lpl_ptr->next;
	}
    }

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    return(ahd);
}

/*{
** Name: OPC_DMTAHD	- Build a single DMU action header for a DMT action.
**
** Description:
**	This routine builds a DMU action header for a DMT_CREATE_TEMP
**	action.  This kind of action is for creating temporary tables
**	used as part of the query plan.  (NOT for creating user tables,
**	whether "real" or session temporary table.)
**
**	In addition to creating the DMU machinery to hand to QEF
**	eventually, we'll create RDF-lookalike table info for the
**	table.  That way the table "looks like" a real relation to
**	the rest of OPC.
**	
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
**          written
**      12-jan-87 (eric)
**          changed $ingres to $default.
**	26-jun-89 (jrb)
**	    Fixed bug where too little memory being allocated.
**	10-feb-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA. Force
**	    DMT_DBMS_REQUEST to be on for these internally-generated temporary
**	    tables.
**	21-jun-93 (robf)
**	    Make sure we copy attribute flags from att into attr.
**      5-oct-93 (eric)
**          Add support for tables on resource lists.
**      19-dec-94 (sarjo01) from 15-sep-94 (johna)
**          initialize dmt_mustlock (added by markm while fixing bug (58681)
**	7-Apr-2004 (schka24)
**	    Set default stuff in attr entries rather than leaving them
**	    to the whims of uninitialized memory.
**	28-apr-06 (dougi)
**	    Assign att_collID for "create table as ...".
[@history_template@]...
*/
static QEF_AHD *
opc_dmtahd(
	OPS_STATE   *global,
	PST_QNODE   *root,
	i4	    flags_mask,
	OPV_IGVARS  gresvno )
{
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd;
    DMT_CB		*dmtcb;
    PST_QNODE		*resdom;
    i4			resno;
    DMF_ATTR_ENTRY	**attrs;
    DMF_ATTR_ENTRY	*attr;
    DMT_ATT_ENTRY	*att;
    DMU_KEY_ENTRY	**keys;
    DMU_KEY_ENTRY	*key;
    RDR_INFO		*rel;
    i4		tupsize;
    i4			attno;
    OPV_GRV		*grv;
    OPC_PST_STATEMENT	*opc_pst;
    OPC_TGINFO		tginfo;
    OPC_TGPART		*tsoffsets;
    QEF_RESOURCE	*resource;
    PTR			nullptr = NULL;

    /* First, lets allocate and initialize an DMT_CB to create a sort temp */
    dmtcb = (DMT_CB *) opu_qsfmem(global, sizeof (DMT_CB));
    MEfill(sizeof(DMT_CB), 0, (PTR) dmtcb);
    dmtcb->type = DMT_TABLE_CB;
    dmtcb->length = sizeof (DMT_CB);
    dmtcb->dmt_tran_id = 0;
    STprintf(dmtcb->dmt_table.db_tab_name, "T%d", (i4) gresvno);
    STmove(dmtcb->dmt_table.db_tab_name, ' ', 
		sizeof (dmtcb->dmt_table.db_tab_name), 
				dmtcb->dmt_table.db_tab_name);
    MEfill(sizeof (dmtcb->dmt_owner.db_own_name), (u_char)' ',
				(PTR)dmtcb->dmt_owner.db_own_name);
    dmtcb->dmt_flags_mask = (flags_mask | DMT_DBMS_REQUEST);
    /* EJLFIX: should the location of this be $default? */
    dmtcb->dmt_location.data_address =
		    (PTR) opu_qsfmem(global, sizeof (DB_LOC_NAME));
    dmtcb->dmt_location.data_in_size = sizeof(DB_LOC_NAME);
    STmove("$default", ' ', sizeof (DB_LOC_NAME), 
					    dmtcb->dmt_location.data_address);
    dmtcb->dmt_mustlock = FALSE;
    /* Now lets fill in 'tginfo' since that is an easier way to deal with
    ** the target list.
    */
    opc_tloffsets(global, root->pst_left, &tginfo, FALSE);

    /* now lets allocate and init an RDR_INFO struct for this temporary
    ** table.
    ** No partitioning on temporary tables.
    */
    rel = (RDR_INFO *) opu_memory(global, sizeof (RDR_INFO));
    MEfill(sizeof (RDR_INFO), (u_char)0, (PTR)rel);
    rel->rdr_rel = (DMT_TBL_ENTRY *) opu_memory(global, sizeof (DMT_TBL_ENTRY));
    MEfill(sizeof (DMT_TBL_ENTRY), (u_char)0, (PTR)rel->rdr_rel);
    STRUCT_ASSIGN_MACRO(dmtcb->dmt_table, rel->rdr_rel->tbl_name);
    STRUCT_ASSIGN_MACRO(dmtcb->dmt_owner, rel->rdr_rel->tbl_owner);
    MEcopy((PTR)dmtcb->dmt_location.data_address, sizeof(DB_LOC_NAME),
	   (PTR)&rel->rdr_rel->tbl_location);

    rel->rdr_rel->tbl_attr_count = tginfo.opc_nresdom;
    rel->rdr_rel->tbl_storage_type = DMT_HEAP_TYPE;

    /* Let's fill in the array of attributes to create. */
    resdom = root->pst_left;
    dmtcb->dmt_attr_array.ptr_in_count = rel->rdr_rel->tbl_attr_count;
    dmtcb->dmt_attr_array.ptr_size = sizeof (DMF_ATTR_ENTRY);
    dmtcb->dmt_attr_array.ptr_address = 
		(PTR) opu_qsfmem(global, (i4)(sizeof (DMF_ATTR_ENTRY *) * 
				    dmtcb->dmt_attr_array.ptr_in_count));
    attrs = (DMF_ATTR_ENTRY **) dmtcb->dmt_attr_array.ptr_address;

    for (resno = 0; resno <= tginfo.opc_hiresdom; resno += 1)
    {
	tsoffsets = &tginfo.opc_tsoffsets[resno];
	resdom = tsoffsets->opc_resdom;

	if (resdom == NULL)
	{
	    continue;
	}

	attr = attrs[tsoffsets->opc_sortno - 1] = 
		(DMF_ATTR_ENTRY *) opu_qsfmem(global, sizeof (DMF_ATTR_ENTRY));

	/* EJLFIX: Ed should fill in the rsname */
	STprintf(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, "a%d", 
								(i4) resno);
	STmove(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, ' ', 
		sizeof (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname);

	MEcopy((PTR)resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
	    sizeof(attr->attr_name.db_att_name), 
	    (PTR)attr->attr_name.db_att_name);
	attr->attr_type = tsoffsets->opc_tdatatype;
	attr->attr_size = tsoffsets->opc_tlength;
	attr->attr_precision = tsoffsets->opc_tprec;
	attr->attr_flags_mask = 0;
	SET_CANON_DEF_ID(attr->attr_defaultID, DB_DEF_NOT_DEFAULT);
	attr->attr_defaultTuple = NULL;
    }

    /* Now let's fill in the array of attributes for our descriptor. */
    rel->rdr_attr = (DMT_ATT_ENTRY **) opu_memory(global, (i4)
	    (sizeof (DMT_ATT_ENTRY *) * (rel->rdr_rel->tbl_attr_count + 1)));
    rel->rdr_attr[0] = NULL;
    tupsize = 0;
    for (attno = 1; attno <= rel->rdr_rel->tbl_attr_count; attno += 1)
    {
	att = rel->rdr_attr[attno] = 
		(DMT_ATT_ENTRY *) opu_memory(global, sizeof (DMT_ATT_ENTRY));
	attr = attrs[attno - 1];

	STRUCT_ASSIGN_MACRO(attr->attr_name, att->att_name);
	att->att_number = 1;
	att->att_offset = tupsize;
	att->att_type = attr->attr_type;
	att->att_width = attr->attr_size;
	att->att_prec = attr->attr_precision;
	att->att_collID = attr->attr_collID;
	att->att_flags = attr->attr_flags_mask;
	att->att_key_seq_number = 0;
	SET_CANON_DEF_ID(att->att_defaultID, DB_DEF_NOT_DEFAULT);
	att->att_default = NULL;

	tupsize += att->att_width;
    }
    rel->rdr_rel->tbl_width = tupsize;

    /* Lets fill in the array of attributes to sort on */
    if (flags_mask & DMT_LOAD)
    {
	dmtcb->dmt_key_array.ptr_in_count = rel->rdr_rel->tbl_attr_count;
	dmtcb->dmt_key_array.ptr_size = sizeof (DMT_KEY_ENTRY);
	dmtcb->dmt_key_array.ptr_address = 
		    (PTR) opu_qsfmem(global, (i4)(sizeof (DMT_KEY_ENTRY *) * 
					    dmtcb->dmt_key_array.ptr_in_count));
	keys = (DMU_KEY_ENTRY **) dmtcb->dmt_key_array.ptr_address;

	for (resno = 0; resno <= tginfo.opc_hiresdom; resno += 1)
	{
	    tsoffsets = &tginfo.opc_tsoffsets[resno];
	    resdom = tsoffsets->opc_resdom;

	    if (resdom == NULL)
	    {
		continue;
	    }

	    key = keys[tsoffsets->opc_sortno - 1] = 
		(DMU_KEY_ENTRY *) opu_qsfmem(global, sizeof (DMT_KEY_ENTRY));

	    MEcopy((PTR)resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
			sizeof(key->key_attr_name.db_att_name), 
			(PTR)key->key_attr_name.db_att_name);
	    key->key_order = DMT_ASCENDING;
	}
    }
    else
    {
	dmtcb->dmt_key_array.ptr_in_count = 0;
	dmtcb->dmt_key_array.ptr_size = 0;
	dmtcb->dmt_key_array.ptr_address = NULL;
    }

    /* Finally, lets initialize the char array */
    dmtcb->dmt_char_array.data_address = NULL;
    dmtcb->dmt_char_array.data_in_size = 0;
    dmtcb->dmt_char_array.data_out_size = 0;

    /* Now, lets allocate the action header */
    ahd = sq->ops_compile.opc_ahd = 
			    (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DMU));
    sq->ops_compile.opc_bgpostfix = sq->ops_compile.opc_endpostfix = NULL;

    /* if this is the first action header that we have created then lets
    ** remember it.
    */
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DMU);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_DMU;
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    ahd->ahd_valid = NULL;

    ahd->qhd_obj.qhd_dmu.ahd_func = DMT_CREATE_TEMP;
    ahd->qhd_obj.qhd_dmu.ahd_cb = (PTR) dmtcb;


    /* maintain the linked list of valid structs that hang off resource
    ** structs.
    */
    grv = global->ops_rangetab.opv_base->opv_grv[gresvno];
    resource = opc_get_resource(global, grv,
		(i4 *) nullptr, (PTR *) nullptr, 
		(QEF_MEM_CONSTTAB **) nullptr);
    if (resource == (QEF_RESOURCE *) NULL)
    {
	ahd->qhd_obj.qhd_dmu.ahd_alt = NULL;
	grv->opv_ptrvalid = &ahd->qhd_obj.qhd_dmu.ahd_alt;	
    }
    else
    {
	ahd->qhd_obj.qhd_dmu.ahd_alt = resource->qr_resource.qr_tbl.qr_valid;
	grv->opv_ptrvalid = (QEF_VALID **) NULL;
    }

    /* lets put the descriptor in the global range table where the rest of
    ** OPC can use it.
    */
    global->ops_rangetab.opv_base->opv_grv[gresvno]->opv_relation = rel;

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    return(ahd);
}

/*{
** Name: OPC_RUPAHD	- Build a single RUP action header.
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
**      31-jul-86 (eric)
**          written
**	18-apr-89 (paul)
**	    Modified to build a standard QEF_QEP style action header. This
**	    allows us to treat cursor update as a normal query action for
**	    more regular execution in QEF and for rules support.
**      02-apr-90 (teg)
**          initialized rdr_2types_mask;
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	06-nov-90 (stec)
**	    Changed error code to be returned in 
**	    case of QSO_INFO call failure.
**	23-jan-91 (seputis)
**	    eliminated redundant initialization of RDF control block
**	16-jun-92 (anitap)
**	    Cast naked NULL passed into opc_cxest().
**	20-jul-92 (rickh)
**	    Add pointer cast in rdf_call call.
**      19-jun-95 (inkdo01)
**          changed 2-valued i4  ahd_any to 32-valued ahd_qepflag
**	20-jan-04 (inkdo01)
**	    Added ahd_part_def - table partitioning descriptor.
**	6-feb-04 (inkdo01)
**	    Set flag indicating updates to partitioning columns.
**	27-Jul-2004 (schka24)
**	    Make sure we get partdef stuff from RDF.
**	28-Feb-2005 (schka24)
**	    Duplicate 4byte-tidp flag from cursor qp.
**	15-Jul-2005 (schka24)
**	    Don't set partitioning-column-update flag here, we do it
**	    in updcolmap.  Copy over ahd_ruporig from parent QP.
**	26-Oct-2008 (kiria01) SIR121012
**	    Removed uninitialised passing of opdummy as no result expected
**	    in this context.
[@history_template@]...
*/
static QEF_AHD *
opc_rupahd(
	OPS_STATE   *global )
{
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd;
    QSF_RCB		qsfcb;
    DB_STATUS		ret;
    QEF_QP_CB		*parent_qep;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    QEF_VALID		*vl;
    DB_STATUS		status;
    RDR_INFO		*rel;
    PST_QNODE		*root = global->ops_qheader->pst_qtree;
    OPC_PST_STATEMENT	*opc_pst;
    bool		pcolsupd = FALSE;

    /* First, lets allocate the action header */
    ahd = global->ops_cstate.opc_subqry->ops_compile.opc_ahd = 
			    (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    sq->ops_compile.opc_bgpostfix = sq->ops_compile.opc_endpostfix = NULL;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_RUP;
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    ahd->ahd_valid = NULL;

    /* fill in the qhd_qep stuff; */
    ahd->qhd_obj.qhd_qep.ahd_qep_cnt = 0;
    ahd->qhd_obj.qhd_qep.ahd_tidrow = -1;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = -1;
    ahd->qhd_obj.qhd_qep.ahd_reprow = -1;
    ahd->qhd_obj.qhd_qep.ahd_repsize = -1;
    ahd->qhd_obj.qhd_qep.ahd_firstncb = -1;
    ahd->qhd_obj.qhd_qep.ahd_qepflag = 0; 
    ahd->qhd_obj.qhd_qep.ahd_estimates = 0;
    ahd->qhd_obj.qhd_qep.ahd_dio_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cost_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cpu_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_row_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_page_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_part_def = (DB_PART_DEF *) NULL;

    /* Now lets get to the interesting stuff */

    /* first, lets get the 'handle' of the parent QP */
    STRUCT_ASSIGN_MACRO(global->ops_qsfcb, qsfcb);
    STRUCT_ASSIGN_MACRO(global->ops_caller_cb->opf_parent_qep,
	qsfcb.qsf_obj_id);
    if ((ret = qsf_call(QSO_INFO, &qsfcb)) != E_DB_OK)
    {
	opx_verror(ret, E_OP089D_QSF_INFO, 
	    global->ops_qsfcb.qsf_error.err_code);
    }
    parent_qep = (QEF_QP_CB*)qsfcb.qsf_root;

    ahd->qhd_obj.qhd_qep.ahd_qhandle = 
			    global->ops_caller_cb->opf_parent_qep.qso_handle;

    for (vl = parent_qep->qp_ahd->ahd_valid;
	    vl != NULL;
	    vl = vl->vl_next
	)
    {
	if (vl->vl_rw == DMT_A_WRITE)
	{
	    break;
	}
    }
    /*if (vl == NULL)
    **{
    **	error;	EJLFIX: add an error here;
    **}
    */
    /* Duplicate some stuff that QEF will need from the cursor query:
    ** tid row and offset, 4-byte tidp flag, originating node number.
    ** If it's partitioned, QEF will need the DMTCB index and direct vs
    ** deferred flag.
    */
    ahd->qhd_obj.qhd_qep.ahd_tidrow = 
			    parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_tidrow;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
			    parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_tidoffset;
    ahd->qhd_obj.qhd_qep.ahd_qepflag |=
	    (parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP);
    ahd->qhd_obj.qhd_qep.ahd_ruporig = 
			    parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_ruporig;
    ahd->ahd_flags |= (parent_qep->qp_ahd->ahd_flags & QEA_DIR);
    ahd->qhd_obj.qhd_qep.ahd_odmr_cb = parent_qep->qp_upd_cb;
    ahd->qhd_obj.qhd_qep.ahd_dmtix = vl->vl_dmf_cb;
    STRUCT_ASSIGN_MACRO(vl->vl_tab_id, 
	global->ops_rangetab.opv_rdfcb.rdf_rb.rdr_tabid);
    global->ops_rangetab.opv_rdfcb.rdf_rb.rdr_types_mask = 
				(RDR_RELATION | RDR_ATTRIBUTES | RDR_BLD_PHYS);
    global->ops_rangetab.opv_rdfcb.rdf_info_blk = NULL;
    status = rdf_call(RDF_GETDESC, &global->ops_rangetab.opv_rdfcb);
    if (DB_FAILURE_MACRO(status) == TRUE)
    {
	opx_error(E_OP0004_RDF_GETDESC);
    }
    rel = global->ops_cstate.opc_relation = 
				global->ops_rangetab.opv_rdfcb.rdf_info_blk;

    /* initialize the ahd_current QEN_ADF struct; */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    max_base = 3;
    opc_cxest(global, (OPC_NODE *)NULL, root->pst_left, 
					    &ninstr, &nops, &nconst, &szconst);
    opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_qep.ahd_current, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

	/* add a base to hold the parent qeps 
	** tuple and the result.
	*/
    opc_adbase(global, &cadf, QEN_OUT, ADE_ZBASE);

    opc_crupcurs(global, root->pst_left, &cadf, (ADE_OPERAND*)NULL, 
						rel, (OPC_NODE *)NULL);
    opc_adend(global, &cadf);
    /* Build setlist bitmap */
    opc_updcolmap(global, &ahd->qhd_obj.qhd_qep, root->pst_left, 
				rel->rdr_parts);

    /* if (there is a qualification) */
    if (root->pst_right == NULL || 
	    root->pst_right->pst_sym.pst_type == PST_QLEND
	)
    {
	ahd->qhd_obj.qhd_qep.ahd_constant = (QEN_ADF *)NULL;
    }
    else
    {
	/* initialize the ahd_current QEN_ADF struct; */
	ninstr = 0;
	nops = 0;
	nconst = 0;
	szconst = 0;
	max_base = 3;
	opc_cxest(global, (OPC_NODE *)NULL, root->pst_right, 
					    &ninstr, &nops, &nconst, &szconst);
	opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_qep.ahd_constant, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

	    /* add a base to hold the parent qeps 
	    ** tuple and the result.
	    */
	opc_adbase(global, &cadf, QEN_OUT, ADE_ZBASE);

	opc_crupcurs(global, root->pst_right, &cadf, (ADE_OPERAND*)NULL, 
						rel, (OPC_NODE *)NULL);
	opc_adend(global, &cadf);
    }

    if (rel->rdr_parts != (DB_PART_DEF *) NULL)
    {
	/* Update on partitioned table - copy DB_PART_DEF. */
	opc_partdef_copy(global, rel->rdr_parts, opu_qsfmem,
	    &ahd->qhd_obj.qhd_qep.ahd_part_def);
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PART_TAB;
	/* Partition-changing update might need this: */
	ahd->qhd_obj.qhd_qep.ahd_repsize = rel->rdr_rel->tbl_width;
    }

    /* Add any rules information required */
    opc_addrules(global, ahd, rel);

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    return(ahd);
}

/*{
** Name: OPC_RDELAHD	- Build a single RDEL action header for delete cursor.
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
**	18-apr-89 (paul)
**	    Stolen from opc_rupahd.
**      02-apr-90 (teg)
**          initialized rdr_2types_mask;
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	06-nov-90 (stec)
**	    Changed error code to be returned in 
**	    case of QSO_INFO call failure.
**	23-jan-91 (seputis)
**	    eliminated redundant initialization of RDF control block
**	20-jul-92 (rickh)
**	    Add pointer cast in rdf_call call.
**      19-jun-95 (inkdo01)
**          changed 2-valued i4  ahd_any to 32-valued ahd_qepflag
**	27-Jul-2004 (schka24)
**	    Make sure we get partdef stuff from RDF.
**	28-Feb-2005 (schka24)
**	    Duplicate 4byte-tidp flag from cursor qp.
[@history_template@]...
*/
static QEF_AHD *
opc_rdelahd(
	OPS_STATE   *global )
{
    OPS_SUBQUERY	*sq = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd;
    QSF_RCB		qsfcb;
    DB_STATUS		ret;
    QEF_QP_CB		*parent_qep;
    QEF_VALID		*vl;
    DB_STATUS		status;
    RDR_INFO		*rel;
    PST_QNODE		*root = global->ops_qheader->pst_qtree;
    OPC_PST_STATEMENT	*opc_pst;

    /* First, lets allocate the action header */
    ahd = global->ops_cstate.opc_subqry->ops_compile.opc_ahd = 
			    (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    sq->ops_compile.opc_bgpostfix = sq->ops_compile.opc_endpostfix = NULL;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_RDEL;
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    ahd->ahd_valid = NULL;

    /* fill in the qhd_qep stuff; */
    ahd->qhd_obj.qhd_qep.ahd_qep_cnt = 0;
    ahd->qhd_obj.qhd_qep.ahd_tidrow = -1;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = -1;
    ahd->qhd_obj.qhd_qep.ahd_reprow = -1;
    ahd->qhd_obj.qhd_qep.ahd_repsize = -1;
    ahd->qhd_obj.qhd_qep.ahd_firstncb = -1;
    ahd->qhd_obj.qhd_qep.ahd_qepflag = 0; 
    ahd->qhd_obj.qhd_qep.ahd_estimates = 0;
    ahd->qhd_obj.qhd_qep.ahd_dio_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cost_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cpu_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_row_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_page_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_part_def = (DB_PART_DEF *) NULL;

    /* Now lets get to the interesting stuff */

    /* first, lets get the 'handle' of the parent QP */
    STRUCT_ASSIGN_MACRO(global->ops_qsfcb, qsfcb);
    STRUCT_ASSIGN_MACRO(global->ops_caller_cb->opf_parent_qep, 
	qsfcb.qsf_obj_id);
    if ((ret = qsf_call(QSO_INFO, &qsfcb)) != E_DB_OK)
    {
	opx_verror(ret, E_OP089D_QSF_INFO, 
	    global->ops_qsfcb.qsf_error.err_code);
    }
    parent_qep = (QEF_QP_CB*)qsfcb.qsf_root;

    ahd->qhd_obj.qhd_qep.ahd_qhandle = 
			    global->ops_caller_cb->opf_parent_qep.qso_handle;
    /* EJLFIX: check that the parent has a single QEA_GET action */
    /* EJLFIX: check that the parent has a single base relation on it's
    ** valid list.
    */
    for (vl = parent_qep->qp_ahd->ahd_valid;
	    vl != NULL;
	    vl = vl->vl_next
	)
    {
	if (vl->vl_rw == DMT_A_WRITE)
	{
	    break;
	}
    }
    /*if (vl == NULL)
    **{
    **	error;	EJLFIX: add an error here;
    **}
    */
    /* Duplicate some stuff that QEF will need from the cursor query:
    ** tid row and offset, 4-byte tidp flag.
    ** If it's partitioned, QEF will need the DMTCB index and direct vs
    ** deferred flag.
    */
    ahd->qhd_obj.qhd_qep.ahd_tidrow = 
			    parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_tidrow;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = 
			    parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_tidoffset;
    ahd->qhd_obj.qhd_qep.ahd_qepflag |=
	    (parent_qep->qp_ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_4BYTE_TIDP);
    ahd->ahd_flags |= (parent_qep->qp_ahd->ahd_flags & QEA_DIR);
    ahd->qhd_obj.qhd_qep.ahd_odmr_cb = parent_qep->qp_upd_cb;
    ahd->qhd_obj.qhd_qep.ahd_dmtix = vl->vl_dmf_cb;
    STRUCT_ASSIGN_MACRO(vl->vl_tab_id, 
	global->ops_rangetab.opv_rdfcb.rdf_rb.rdr_tabid);
    global->ops_rangetab.opv_rdfcb.rdf_rb.rdr_types_mask = 
				(RDR_RELATION | RDR_ATTRIBUTES | RDR_BLD_PHYS);
    global->ops_rangetab.opv_rdfcb.rdf_info_blk = NULL;
	/* save the data base id for this session only */
    status = rdf_call(RDF_GETDESC, &global->ops_rangetab.opv_rdfcb);
    if (DB_FAILURE_MACRO(status) == TRUE)
    {
	opx_error(E_OP0004_RDF_GETDESC);
    }
    rel = global->ops_cstate.opc_relation = 
				global->ops_rangetab.opv_rdfcb.rdf_info_blk;

    if (rel->rdr_parts != (DB_PART_DEF *) NULL)
    {
	/* Delete on partitioned table - copy DB_PART_DEF. */
	opc_partdef_copy(global, rel->rdr_parts, opu_qsfmem,
	    &ahd->qhd_obj.qhd_qep.ahd_part_def);
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_PART_TAB;
    }


    /* Add any rules information required */
    opc_addrules(global, ahd, rel);
    
    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    return(ahd);
}

/*{
** Name: OPC_DVAHD_BUILD	- Build an "action" for a DECVAR statement.
**
** Description:
**      Build an "action" for a DECVAR statement. OK, so we don't really 
**      build an action for a DECVAR statement, but that's what we do for 
**      all of the other statements so it just seem to fit here. 
**      
**      What we do do here is to decide where to put the data for the 
**      variables that are being declared here. 
[@comment_line@]...
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
**      23-may-88 (eric)
**          created
**      25-aug-88 (eric)
**          added support for the built-ins
**      07-sep-88 (thurston)
**          Fixed a memory smashing bug.  In the first block of code below,
**	    it was calculating the amount of memory needed to store the
**	    parameters for a DECVAR (in params_size) correctly, but then was
**	    allocating only `sizeof (QEF_AHD)' bytes in the call to opu_qsfmem()
**	    and promptly MEfill'ing in with zeros.  When there were enough
**	    parameters, this would crunch memory.
**	08-sep-88 (puree)
**	    prevent allocating 0 byte memory.  This happened when
**	    the procedure does not have a parameter.
**      9-nov-89 (eric)
**          Added support for multiple decvar statements. This involved
**          maintaining a linked list of them to allow searching all of
**          them when resolving dbp local var references. See opc_lvar_row()
**          in OPCQUAL.C
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	9-june-06 (dougi)
**	    Added dbp_mode to record parameter mode (IN, OUT, INOUT).
[@history_template@]...
*/
VOID
opc_dvahd_build(
		OPS_STATE	*global,
		PST_DECVAR	*decvar)
{
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    i4			qef_varno;
    i4			varno;
    i4			rowno;
    i4			byref_value_rowno;
    OPC_PST_STATEMENT	*opc_pst;
    QEF_DBP_PARAM	*parms;
    i4			params_size;

    if (decvar->pst_nvars <= 0)
    {
	return;
    }

    if (decvar == global->ops_procedure->pst_parms)
    {
	opc_pst = NULL;
	qp->qp_ndbp_params = decvar->pst_nvars;
	if (qp->qp_ndbp_params != 0)
	{
	    /* allocate QSF memory and dsh row only when there are parameters */
	    params_size = qp->qp_ndbp_params * sizeof(QEF_DBP_PARAM);
	    parms = qp->qp_dbp_params = 
			    (QEF_DBP_PARAM *) opu_qsfmem(global, params_size);
	    MEfill(params_size, (u_char) 0, (PTR)parms);
	    /* allocate row for pointers to source for byrefs */
	    opc_ptrow(global, &byref_value_rowno,
		      sizeof(DB_DATA_VALUE) * qp->qp_ndbp_params);

	}
	else
	{
	    parms = NULL;
	}
    }
    else
    {
	parms = NULL;
	opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;

	/* make the linked list of DECVAR statements */
	opc_pst->opc_ndecvar = global->ops_cstate.opc_topdecvar;
	global->ops_cstate.opc_topdecvar = global->ops_statement;
    }

    qef_varno = 0;
    for (varno = 0; varno < decvar->pst_nvars; varno += 1)
    {
	opc_ptrow(global, &rowno, decvar->pst_vardef[varno].db_length);

	if (varno + decvar->pst_first_varno == PST_RCNT)
	{
	    qp->qp_rrowcnt_row = rowno;
	    qp->qp_orowcnt_offset = 0;

	    /* decrement the number of parameters for QEF, since QEF is told
	    ** about this via the fields above.
	    */
	    qp->qp_ndbp_params -= 1;
	}
	else if (varno + decvar->pst_first_varno == PST_ERNO)
	{
	    qp->qp_rerrorno_row = rowno;
	    qp->qp_oerrorno_offset = 0;

	    /* decrement the number of parameters for QEF, since QEF is told
	    ** about this via the fields above.
	    */
	    qp->qp_ndbp_params -= 1;
	}
	else if (parms != NULL)
	{
	    MEcopy((PTR)decvar->pst_varname[varno].db_parm_name, 
			    sizeof (parms[qef_varno].dbp_name), 
					(PTR)parms[qef_varno].dbp_name);
	    parms[qef_varno].dbp_rowno = rowno;
	    parms[qef_varno].dbp_offset = 0;
	    parms[qef_varno].dbp_byref_value_rowno = byref_value_rowno;

	    parms[qef_varno].dbp_byref_value_offset = qef_varno *
		sizeof(DB_DATA_VALUE);
	    STRUCT_ASSIGN_MACRO(decvar->pst_vardef[varno], 
					    parms[qef_varno].dbp_dbv);
	    if (parms[qef_varno].dbp_dbv.db_data != NULL)
	    {
		parms[qef_varno].dbp_dbv.db_data = 
			opu_qsfmem(global, parms[qef_varno].dbp_dbv.db_length);
		MEcopy((PTR)decvar->pst_vardef[varno].db_data,
			    parms[qef_varno].dbp_dbv.db_length, 
				(PTR)parms[qef_varno].dbp_dbv.db_data);
	    }
	    if (decvar->pst_parmmode[varno] == PST_PMOUT)
		parms[qef_varno].dbp_mode = DBP_PMOUT;
	    else if (decvar->pst_parmmode[varno] == PST_PMINOUT)
		parms[qef_varno].dbp_mode = DBP_PMINOUT;
	    else parms[qef_varno].dbp_mode = DBP_PMIN;

	    /* We just told QEF about another dbp parameter, so lets increment
	    ** our variable.
	    */
	    qef_varno += 1;
	}

	if (varno == 0)
	{
	    if (opc_pst == NULL)
	    {
		global->ops_cstate.opc_pvrow_dbp = rowno;
	    }
	    else
	    {
		opc_pst->opc_lvrow = rowno;
	    }
	}
    }
}

/*{
** Name: OPC_IFAHD_BUILD	- Build an action header for an IF statement.
**
** Description:
**      This routine coordinates the building of an action header for an IF 
**      statement. 
[@comment_line@]...
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
**      20-may-88 (eric)
**          created
**	16-jun-92 (anitap)
**	    Cast naked NULL being passed into opc_cxest().
**	30-oct-95 (inkdo01)
**	    Remove trailing ADE_AND as part of ade_execute_cx speedup.
**      18-Jan-1999 (hanal04) Bug 92148 INGSRV494
**          If the IF's condition tree has not been normalised set up
**          labels and pass this information on to opc_cupdcurs() and
**          resolve the labels afterwards. The new logic mirrors opc_qual()
**          and its call to opc_qual1().
**	23-feb-00 (hayke02)
**	    Removed opc_noncnf - we now test for PST_NONCNF in
**	    global->ops_statement->pst_stmt_mask directly in opc_crupcurs().
**	    This change fixes bug 100484.
**	26-Oct-2008 (kiria01) SIR121012
**	    Removed uninitialised passing of opdummy as no result expected
**	    in this context.
[@history_template@]...
*/
VOID
opc_ifahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_IF ));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_IF);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_IF;
    ahd->ahd_valid = NULL;

    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Now lets go on to the IF specific stuff. We fill the ahd_true and
    ** ahd_false pointers in with NULLs because we don't have those actions
    ** yet. We will fill those in later, when we finish the query plan.
    */
    ahd->qhd_obj.qhd_if.ahd_true = NULL;
    ahd->qhd_obj.qhd_if.ahd_false = NULL;

    /* initialize the ahd_condition QEN_ADF struct; */
    ninstr = 1;
    nops = 0;
    nconst = 0;
    szconst = 0;
    max_base = 3;
    opc_cxest(global, (OPC_NODE *)NULL, 
		global->ops_statement->pst_specific.pst_if.pst_condition,
					    &ninstr, &nops, &nconst, &szconst);
    opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_if.ahd_condition,
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

    /* b92148 - If non-cnf make sure opc_crupcurs() knows about it.        */
    /* We must set the true and false labels before calling opc_crupcurs() */
    /* and resolve the labels afterwards.                                  */
    /* B92148 code removed here due to integration with 2.5 change for bug */
    /* 101915. */

    /* Note that the address of the table descriptor in the following call  */
    /* is only valid when processing rules. It is only used by QTREE nodes   */
    /* unique to rule expressions (PST_RULE nodes). */
    opc_crupcurs(global, 
	    global->ops_statement->pst_specific.pst_if.pst_condition,
	    &cadf, (ADE_OPERAND*)NULL, global->ops_cstate.opc_rdfdesc, (OPC_NODE *) NULL);
    opc_adend(global, &cadf);

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_FORAHD_BUILD	- Build an action header for a FOR statement.
**
** Description:
**      This routine coordinates the building of an action header for a FOR 
**      statement. 
[@comment_line@]...
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
**      23-oct-98 (inkdo01)
**	    Cloned from opc_ifahd_build.
**	18-dec-2007 (dougi)
**	    Init various first/offset "n" fields.
*/
VOID
opc_forahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst;
    i4			szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_QEP);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_FOR;
    ahd->ahd_valid = NULL;

    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* fill in the qhd_qep stuff; */
    /* This is just background initialization, because the "for" action
    ** header simply acts as a bridge to the corresponding select qtree.
    ** That is, the "for" and the following "get" BOTH address the 
    ** select qtree, once the QP gets resolved. */
    ahd->qhd_obj.qhd_qep.ahd_qep_cnt = 0;
    ahd->qhd_obj.qhd_qep.ahd_tidrow = -1;
    ahd->qhd_obj.qhd_qep.ahd_tidoffset = -1;
    ahd->qhd_obj.qhd_qep.ahd_reprow = -1;
    ahd->qhd_obj.qhd_qep.ahd_repsize = -1;
    ahd->qhd_obj.qhd_qep.ahd_firstncb = -1;
    ahd->qhd_obj.qhd_qep.ahd_qepflag = 0; 
    ahd->qhd_obj.qhd_qep.ahd_estimates = 0;
    ahd->qhd_obj.qhd_qep.ahd_dio_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cost_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_cpu_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_row_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_page_estimate = 0;
    ahd->qhd_obj.qhd_qep.ahd_offsetn = 0; 
    ahd->qhd_obj.qhd_qep.ahd_firstn = 0; 
    ahd->qhd_obj.qhd_qep.ahd_part_def = (DB_PART_DEF *) NULL;
    ahd->qhd_obj.qhd_qep.ahd_qep = (QEN_NODE *)NULL;

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_RTNAHD_BUILD	- Build an action header for a RETURN statement.
**
** Description:
**      This routine coordinates the building of an action header for an RETURN
**      statement. 
[@comment_line@]...
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
**      20-may-88 (eric)
**          created
**	17-jul-89 (jrb)
**	    Set opr_prec field correctly and check for equality iff type is
**	    decimal datatype.
**	12-feb-92  (nancy)
**	    Bug 39225, add call to opc_bgmkey() to create cx to initialize
**	    local variables. (change also requires qeq_validate() fix)
**	16-jun-92 (anitap)
**	    Cast naked NULLs being passed into opc_cxest().
[@history_template@]...
*/
VOID
opc_rtnahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPC_PST_STATEMENT	*opc_pst;
    ADE_OPERAND		ops[3];
    QEF_AHD		*ahd;
    PST_QNODE		*qretnum;
    DB_STATUS		err;
    ADI_FI_ID		opno;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_RETURN));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_RETURN);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_RETURN;

    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    ahd->ahd_valid = NULL;

    /* Now lets go on to the RETURN specific stuff. */
    qretnum = global->ops_statement->pst_specific.pst_rtn.pst_rtn_value;
    if (qretnum == NULL)
    {
	ahd->qhd_obj.qhd_return.ahd_rtn = NULL;
    }
    else
    {
	ninstr = 1;
	nops = 2;
	nconst = 0;
	szconst = 0;
	max_base = 3;
	opc_cxest(global, (OPC_NODE *)NULL, qretnum, &ninstr, &nops, &nconst, &szconst);
	opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_return.ahd_rtn,
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
	opc_adbase(global, &cadf, QEN_OUT, 0);

	ops[1].opr_dt = qretnum->pst_sym.pst_dataval.db_datatype;
	opc_crupcurs(global, qretnum, &cadf, &ops[1], (RDR_INFO *) NULL,
							(OPC_NODE *) NULL);

	ops[0].opr_dt = DB_INT_TYPE;
	ops[0].opr_prec = 0;
	ops[0].opr_len = 4;
	ops[0].opr_prec = 0;
	ops[0].opr_base = cadf.opc_out_base;
	ops[0].opr_offset = 0;

	/*
	** If (type and length of the resdom != type and length of the
	** attribute and prec of same for decimal datatype).
	*/
	if (ops[0].opr_dt  != ops[1].opr_dt	||
	    ops[0].opr_len != ops[1].opr_len	||
	    /*
	    ** At this point opr_dt && opr_len are equal, so for
	    ** decimal datatype we want to check precision.
	    */
	   (abs(ops[0].opr_dt) == DB_DEC_TYPE
                &&  ops[0].opr_prec != ops[1].opr_prec
                )
	   )
	{
	    /* Find the func inst id to do the conversion; */
	    if ((err = adi_ficoerce(global->ops_adfcb, 
			ops[1].opr_dt, ops[0].opr_dt, &opno)) != E_DB_OK
		)
	    {
		opx_verror(err, E_OP0783_ADI_FICOERCE,
			    global->ops_adfcb->adf_errcb.ad_errcode);
	    }
	}
	else
	{
	    opno = ADE_MECOPY;
	}

	opc_adinstr(global, &cadf, opno, ADE_SMAIN, 2, ops, 1);

	opc_adend(global, &cadf);
    }

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_MSGAHD_BUILD   - Build an action header for a MESSAGE statement.
**
** Description:
**      This routine coordinates the building of an action header for an MESSAGE
**      statement. 
[@comment_line@]...
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
**      20-may-88 (eric)
**          created
**	14-jun-89 (paul)
**	    The ahd_mkey expression for the first action in a QP contains
**	    the initialization code for procedure local variables. This
**	    structure was not being created for MESSAGE, RAISE ERROR,
**	    COMMIT, ROLLBACK, CALLPROC or IPROC statements.
**	17-jul-89 (jrb)
**	    Set opr_prec field correctly and check for equality iff type is
**	    decimal datatype.
**	27-feb-91 (ralph)
**	    Copy message destination flags from PST_MSGSTMT.pst_msgdest
**	    to QEF_MESSAGE.ahd_mdest.
**	16-jun-92 (anitap)
**	    Cast naked NULL being passed into opc_cxest().
[@history_template@]...
*/
VOID
opc_msgahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPC_PST_STATEMENT	*opc_pst;
    ADE_OPERAND		ops[3];
    QEF_AHD		*ahd;
    PST_QNODE		*qmsgnum;
    PST_QNODE		*qmsgtext;
    DB_STATUS		err;
    ADI_FI_ID		opno;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_MESSAGE));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_MESSAGE);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_MESSAGE;
    ahd->ahd_valid = NULL;

    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Now lets go on to the MESSAGE specific stuff. */
    qmsgnum = global->ops_statement->pst_specific.pst_msg.pst_msgnumber;
    qmsgtext = global->ops_statement->pst_specific.pst_msg.pst_msgtext;
    ahd->qhd_obj.qhd_message.ahd_mdest =
	global->ops_statement->pst_specific.pst_msg.pst_msgdest;
    if (qmsgnum == NULL)
    {
	ahd->qhd_obj.qhd_message.ahd_mnumber = NULL;
    }
    else
    {
	ninstr = 1;
	nops = 2;
	nconst = 0;
	szconst = 0;
	max_base = 3;
	opc_cxest(global, (OPC_NODE *)NULL, qmsgnum, &ninstr, &nops, 
			&nconst, &szconst);
	opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_message.ahd_mnumber,
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
	opc_adbase(global, &cadf, QEN_OUT, 0);

	ops[1].opr_dt = qmsgnum->pst_sym.pst_dataval.db_datatype;
	opc_crupcurs(global, qmsgnum, &cadf, &ops[1], (RDR_INFO *) NULL,
							(OPC_NODE *) NULL);

	ops[0].opr_dt = DB_INT_TYPE;
	ops[0].opr_prec = 0;
	ops[0].opr_len = 4;
	ops[0].opr_prec = 0;
	ops[0].opr_base = cadf.opc_out_base;
	ops[0].opr_offset = 0;

	/*
	** If (type and length of the resdom != type and length of the
	** attribute and prec of same for decimal datatype).
	*/
	if (ops[0].opr_dt  != ops[1].opr_dt	||
	    ops[0].opr_len != ops[1].opr_len	||
	    /*
	    ** At this point opr_dt && opr_len are equal, so for
	    ** decimal datatype we want to check precision.
	    */
	   (abs(ops[0].opr_dt) == DB_DEC_TYPE
	   &&  ops[0].opr_prec != ops[1].opr_prec
           )
	  )
	{
	    /* Find the func inst id to do the conversion; */
	    if ((err = adi_ficoerce(global->ops_adfcb, 
			ops[1].opr_dt, ops[0].opr_dt, &opno)) != E_DB_OK
		)
	    {
		opx_verror(err, E_OP0783_ADI_FICOERCE,
			    global->ops_adfcb->adf_errcb.ad_errcode);
	    }
	}
	else
	{
	    opno = ADE_MECOPY;
	}

	opc_adinstr(global, &cadf, opno, ADE_SMAIN, 2, ops, 1);

	opc_adend(global, &cadf);
    }

    if (qmsgtext == NULL)
    {
	ahd->qhd_obj.qhd_message.ahd_mtext = NULL;
    }
    else
    {
	ninstr = 1;
	nops = 2;
	nconst = 0;
	szconst = 0;
	max_base = 3;
	opc_cxest(global, (OPC_NODE *)NULL, qmsgtext, &ninstr, &nops, 
			&nconst, &szconst);
	opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_message.ahd_mtext,
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
	opc_adbase(global, &cadf, QEN_OUT, 0);

	ops[1].opr_dt = DB_VCH_TYPE;
	opc_crupcurs(global, qmsgtext, &cadf, &ops[1], (RDR_INFO *) NULL,
							(OPC_NODE *) NULL);

	ops[0].opr_dt   = ops[1].opr_dt;
	ops[0].opr_len  = ops[1].opr_len;
	ops[0].opr_prec = ops[1].opr_prec;
	ops[0].opr_base = cadf.opc_out_base;
	ops[0].opr_offset = 0;

	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);

	opc_adend(global, &cadf);
    }

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_CMTAHD_BUILD	- Build an action header for a COMMIT statement.
**
** Description:
**      This routine coordinates the building of an action header for an COMMIT
**      statement. 
[@comment_line@]...
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
**      20-may-88 (eric)
**          created
**	14-jun-89 (paul)
**	    The ahd_mkey expression for the first action in a QP contains
**	    the initialization code for procedure local variables. This
**	    structure was not being created for MESSAGE, RAISE ERROR,
**	    COMMIT, ROLLBACK, CALLPROC or IPROC statements.
**	17-jul-89 (jrb)
**	    Set opr_prec field correctly and check for equality iff type is
**	    decimal datatype.
**	1-oct-98 (inkdo01)
**	    Indicate COMMIT in db procedure (for internal temptab recovery).
[@history_template@]...
*/
VOID
opc_cmtahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;
    OPC_ADF		cadf;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_COMMIT;
    ahd->ahd_valid = NULL;
    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* There is no COMMIT specific stuff. */

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
    global->ops_cstate.opc_qp->qp_status |= QEQP_DBP_COMRBK;
				/* and show COMMIT in db procedure */
}

/*{
** Name: OPC_ROLLAHD_BUILD	- Build an action header for a ROLLBACK stmt.
**
** Description:
**      This routine coordinates the building of an action header for an 
**	rollback statement. 
[@comment_line@]...
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
**      20-may-88 (eric)
**          created
**	14-jun-89 (paul)
**	    The ahd_mkey expression for the first action in a QP contains
**	    the initialization code for procedure local variables. This
**	    structure was not being created for MESSAGE, RAISE ERROR,
**	    COMMIT, ROLLBACK, CALLPROC or IPROC statements.
**	1-oct-98 (inkdo01)
**	    Indicate ROLLBACK in db procedure (for internal temptab recovery).
[@history_template@]...
*/
VOID
opc_rollahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;
    OPC_ADF		cadf;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_ROLLBACK;
    ahd->ahd_valid = NULL;
    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* There is no ROLLBACK specific stuff. */

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
    global->ops_cstate.opc_qp->qp_status |= QEQP_DBP_COMRBK;
				/* and show ROLLBACK in db procedure */
}

/*{
** Name: OPC_EMSGAHD_BUILD	- Build an AHD for a RAISE ERROR statement.
**
** Description:
**      This routine coordinates the building of an action header for a
**      RAISE ERROR statement. 
**
** Inputs:
**	global	    - the global control block for compilation
**
**	new_ahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	Returns:
**	    Pointer to newly created AHD in the address specified by new_ahd.
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	30-jan-89 (paul)
**	    written to support rules processing
**	24-mar-89 (paul)
**	    Set the action type in the ahd_atype field correctly.
[@history_template@]...
*/
VOID
opc_emsgahd_build(
		OPS_STATE	*global,
		QEF_AHD		**new_ahd)
{

    /* This is identical to message processing with the exception that	    */
    /* the final action type is QEA_EMESSAGE instead of QEA_MESSAGE.	    */
    /* Build a message action and then change its type. */
    opc_msgahd_build(global, new_ahd);
    (*new_ahd)->ahd_atype = QEA_EMESSAGE;
}

/*{
** Name: OPC_CPAHD_BUILD	- Build an AHD for a CALLPROC statement.
**
** Description:
**	This routine cooridnates the building of an action header for a
**	CALL PROCEDURE statement.
**
** Inputs:
**	global	    - the global control block for compilation
**
**	new_ahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	Returns:
**	    Pointer to newly created AHD in the address specified by new_ahd.
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	30-jan-89 (paul)
**	    Written to support rules processing
**	09-may-89 (neil)
**	    Assign pst_rulename to trace rule firing.
**	14-jun-89 (paul)
**	    The ahd_mkey expression for the first action in a QP contains
**	    the initialization code for procedure local variables. This
**	    structure was not being created for MESSAGE, RAISE ERROR,
**	    COMMIT, ROLLBACK, CALLPROC or IPROC statements.
**	05-dec-90 (ralph)
**	    Copy rule owner name into ahd_ruleowner (b34531)
**	16-jun-92 (anitap)
**	    Cast naked NULLs being passed into opc_cxest().
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	14-dec-92 (jhahn)
**	    Did some cleanup for  handling of nested procedure calls.
**	24-jul-96 (inkdo01)
**	    Added global temptab as proc parm support.
**	14-june-06 (dougi)
**	    Added support for BEFORE triggers.
**	26-Oct-2008 (kiria01) SIR121012
**	    Removed uninitialised passing of opdummy to opc_crupcurs as
**	    no result expected in this context.
[@history_template@]...
*/
VOID
opc_cpahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst, max_base;
    i4		szconst;
    PST_QNODE		*root;
    ADE_OPERAND		opdummy;
    QEF_AHD		*ahd;
    OPC_PST_STATEMENT	*opc_pst;
    OPC_ADF		cadf;
    PST_RSDM_NODE	*return_status;

    ahd = *pahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CALLPROC));

    /* If this is the first AHD or the top AHD, set the proper status.	    */
    /* 'first ahd' is the first AHD of this QP.				    */
    /* 'top ahd' is the top(first) AHD for this QEP. */
    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CALLPROC);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_CALLPROC;
    ahd->ahd_valid = NULL;
    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    STRUCT_ASSIGN_MACRO(global->ops_statement->pst_specific.pst_callproc.pst_rulename,
	ahd->qhd_obj.qhd_callproc.ahd_rulename);
    STRUCT_ASSIGN_MACRO(global->ops_statement->pst_specific.pst_callproc.pst_ruleowner,
	ahd->qhd_obj.qhd_callproc.ahd_ruleowner);
    /* Now define the CALLPROC action information in the AHD.		    */
    /* Copy the procedure name and the number of parameters to the AHD.	    */
    /* Both these values were computed by PSF. Note that a CALLPROC name    */
    /* is a QSO_NAME(alias) so that procedure references will never be	    */
    /* ambiguous. */
    MEcopy((PTR)&global->ops_statement->pst_specific.pst_callproc.pst_procname,
	sizeof(QSO_NAME), (PTR)&ahd->qhd_obj.qhd_callproc.ahd_dbpalias);
    ahd->qhd_obj.qhd_callproc.ahd_proc_temptable = NULL; /* not set input */
    ahd->qhd_obj.qhd_callproc.ahd_pcount =
	global->ops_statement->pst_specific.pst_callproc.pst_argcnt;
    ahd->qhd_obj.qhd_callproc.ahd_procp_return = (QEN_ADF *) NULL;

    /* If there is a list of parameters, compile the expresion to compute    */
    /* the actual parameters for each CALLPROC invocation. */
    root = global->ops_statement->pst_specific.pst_callproc.pst_arglist;
    if (ahd->qhd_obj.qhd_callproc.ahd_pcount == 1 && root &&
	root->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype == PST_TTAB_DBPARM)
    {
	/* This procedure was called with one parm - a global temporary
	** table. Call opc_gtt_proc_parm to validate table definition with
	** formal parameter declaration, then save tabid in action header. */
	opc_gtt_proc_parm(global, ahd, 
		&root->pst_right->pst_sym.pst_value.pst_s_tab);
    }
    else if (ahd->qhd_obj.qhd_callproc.ahd_pcount > 0 && root)
    {
	/* The parameter list for rule actions consists of constants and    */
	/* values from the before and after images of the row updated by    */
	/* the triggering query. We make the assumption that these rows	    */
	/* have been identified and their index stored in OPC_STATE. We	    */
	/* also assume a descriptor for the target table is available from  */
	/* OPC_STATE.							    */
	/*								    */
	/* This processing is similar to that for IF and REPLACE CUROSR	    */
	/* actions. */

	/* Create a new parameter array for this parameter list. */
	ahd->qhd_obj.qhd_callproc.ahd_params =
	    (QEF_CP_PARAM *)opu_qsfmem(global, sizeof(QEF_CP_PARAM) *
		       ahd->qhd_obj.qhd_callproc.ahd_pcount);

	/* The parser has created a QTREE that contains a target list that  */
	/* computes the actual parameter values. Get the pointer to the	    */
	/* first RESDOM node in the tree (we assume the is no ROOT node at  */
	/* the top of this tree but that there is a TREE node after the	    */
	/* last RESDOM. */
	
	/* Create a new QEN_ADF struct to contain the definition of the	    */
	/* expression that computes the parameter list. */
	ninstr = 0;
	nops = 0;
	nconst = 0;
	szconst = 0;
	/*
	** The magic number is 4 for the following bases:
	**	2  -  one for old row and one for new row
	**	2  -  one for old tid and one for new tid
	*/
	max_base = 4;
	opc_cxest(global, (OPC_NODE *)NULL, root, &ninstr, &nops, &nconst, 
			&szconst);
	opc_adalloct_begin(global, &cadf, &ahd->qhd_obj.qhd_callproc.ahd_procparams,
	    ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

	/* Now compile the TREE expression that computes the parameter list */
	opc_crupcurs(global, root, &cadf, (ADE_OPERAND*)NULL,
	    global->ops_cstate.opc_rdfdesc, (OPC_NODE *) NULL);
	opc_adend(global, &cadf);

	/* Now make another one to copy INOUT and OUT parameters back (for
	** BEFORE triggers). */
	ninstr = 0;
	nops = 0;
	nconst = 0;
	szconst = 0;
	/*
	** The magic number is 4 for the following bases:
	**	2  -  one for old row and one for new row
	**	2  -  one for old tid and one for new tid
	*/
	max_base = 4;
	opc_cxest(global, (OPC_NODE *)NULL, root, &ninstr, &nops, &nconst, 
			&szconst);
	opc_adalloct_begin(global, &cadf, &ahd->qhd_obj.qhd_callproc.ahd_procp_return,
	    ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

	/* This is done by calling opc_cparmret() to effectively reverse
	** the procparams code. */
	if (!opc_cparmret(global, root, &cadf, &opdummy,
	    global->ops_cstate.opc_rdfdesc))
	    ahd->qhd_obj.qhd_callproc.ahd_procp_return = (QEN_ADF *) NULL;
	else opc_adend(global, &cadf);
    }

    return_status =
	global->ops_statement->pst_specific.pst_callproc.pst_return_status;
    if (return_status != NULL)
    {
	DB_DATA_VALUE *var_info;

	opc_lvar_info(global,
		      return_status->pst_ntargno,
		      &ahd->qhd_obj.qhd_callproc.ahd_return_status.ret_rowno,
		      &ahd->qhd_obj.qhd_callproc.ahd_return_status.ret_offset,
		      &var_info);
		STRUCT_ASSIGN_MACRO(*var_info,
				    ahd->qhd_obj.qhd_callproc.
				    ahd_return_status.ret_dbv);
    }
    else
    {
	ahd->qhd_obj.qhd_callproc.ahd_return_status.ret_dbv.db_datatype =
	    DB_NODT; /*no return value */
    }
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_GTT_PROC_PARM
**
** Description:
**
** Inputs:
**	global	    - the global control block for compilation
**
**	ahd	    - address of the callproc action header
**
**	tabname	    - address of global temporary table name
**
** Outputs:
**	Returns:
**
**	ahd->qhd_obj.qhd_callproc.ahd_gttid - temporary table ID.
**
**	Exceptions:
**	E_OP08AE_NOTSETI_PROC	- if called proc has no "set of" parameter
**
**	E_OP08AF_GTTPARM_MISMATCH - if temp table column definitions don't 
**	    exactly match procedure formal parameters.
**
**	various other RDF failures
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	24-jul-96 (inkdo01)
**	    initial creation
**	01-Oct-2009 (smeke01) b122210
**	    If tuples_to_fill is non-zero, do not just give a general 
**	    error: check whether the procedure is accessible first and 
**	    raise an error accordingly. Also amend the whole function so 
**	    that clean up is done where appropriate even if there is an 
**	    error.
**	    
[@history_template@]...
*/
static VOID
opc_gtt_proc_parm(
    OPS_STATE	*global,
    QEF_AHD	*ahd,
    PST_TAB_NODE *tabnode)
{
    DMT_ATT_ENTRY	**collist;
    DMT_ATT_ENTRY	*coldesc;
    i4			numcols;
    RDF_CB		*rdf_cb = &global->ops_rangetab.opv_rdfcb;
    RDR_RB		*rdf_rb = &rdf_cb->rdf_rb;
    PST_CPROCSTMT	*cps = &global->ops_statement->
				  pst_specific.pst_callproc;
    i4			tuples_to_fill;
    DB_STATUS		status;
    DB_PROCEDURE	ptuple;
    QEF_DATA		*qp;
    i4			tupcount;
    RDR_INFO		*rdrinfo;
    OPT_NAME		proc_name;		/* for error messages */
    OPT_NAME		gtt_name;
    OPX_ERROR		opx_error_number = 0;
    PTR			opx_error_arg1 = NULL;
    PTR			opx_error_arg2 = NULL;

    /* global temporary table parm - we must confirm (using RDF
    ** descriptions) that the temp table column definitions and 
    ** set of parameter definitions match EXACTLY. */

    STncpy( (char *)&proc_name, (char *)&cps->pst_procname.qso_n_id.db_cur_name,
	DB_DBP_MAXNAME);
    ((char *)(&proc_name))[ DB_DBP_MAXNAME ] = '\0';
    STncpy((char *)&gtt_name, (char *)&tabnode->pst_tabname, DB_TAB_MAXNAME);
    ((char *)(&gtt_name))[ DB_TAB_MAXNAME ] = '\0';

    /* First, get the temp table information. */
    /* No need for keys/physical stuff here, can't be partitioned */
    rdf_rb->rdr_types_mask = RDR_RELATION | RDR_ATTRIBUTES | RDR_BY_NAME;
    STRUCT_ASSIGN_MACRO(tabnode->pst_owner, rdf_rb->rdr_owner);
    STRUCT_ASSIGN_MACRO(tabnode->pst_tabname, rdf_rb->rdr_name.rdr_tabname);

    /*
    ** need to set rdf_info_blk to NULL for otherwise RDF assumes that we
    ** already have the info_block
    */
    rdf_cb->rdf_info_blk = (RDR_INFO *) NULL;
    status = rdf_call(RDF_GETDESC, rdf_cb);
    if (DB_FAILURE_MACRO(status))
	    opx_error(E_OP0004_RDF_GETDESC);
    rdrinfo = rdf_cb->rdf_info_blk;
    numcols = rdrinfo->rdr_no_attr;		/* save column info for check */
    collist = rdrinfo->rdr_attr;
    STRUCT_ASSIGN_MACRO(rdrinfo->rdr_rel->tbl_id,
            ahd->qhd_obj.qhd_callproc.ahd_gttid);	/* copy for QEF */

    /*
    ** get the dbproc description before looking up descriptions of its
    ** parameters
    */
    MEcopy ((PTR)&cps->pst_procname.qso_n_id.db_cur_name,
	sizeof(DB_TAB_NAME), (PTR)&rdf_rb->rdr_name.rdr_prcname);
    STRUCT_ASSIGN_MACRO(cps->pst_procname.qso_n_own, rdf_rb->rdr_owner);
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    rdf_cb->rdf_info_blk = (RDR_INFO *) NULL;
    
    /*
    ** rdr_rec_access_id must be set null prior to calling RDR_OPEN later.
    ** We set it to NULL here because this makes the clean up easier.
    */
    rdf_rb->rdr_rec_access_id  = NULL;

    status = rdf_call(RDF_GETINFO, rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	{
	    /* wonder where PSF got that IIPROCEDURE tuple... */
	    opx_error_number = E_OP000C_DBPROC_NOT_FOUND;
	}
	else
	{
	    /* this is mighty unfortunate */
	    opx_error_number = E_OP0004_RDF_GETDESC;
	}
    }
    
    if (!opx_error_number)
    {
	MEcopy((PTR)rdf_cb->rdf_info_blk->rdr_dbp, sizeof(DB_PROCEDURE), 
	    (PTR)&ptuple);
	if (!(ptuple.db_mask[0] & DBP_SETINPUT))
	{
	    /* Can't pass temp table to a dbproc which doesn't have "set of" parm. */
	    opx_error_number = E_OP08AE_NOTSETI_PROC;
	    opx_error_arg1 = (PTR)&proc_name;
	}
    }

    if (!opx_error_number)
    {
	STRUCT_ASSIGN_MACRO(ptuple.db_procid, 
		ahd->qhd_obj.qhd_callproc.ahd_procedureID);
				/* fill this in, too, while we're here */

	STRUCT_ASSIGN_MACRO(ptuple.db_procid, rdf_rb->rdr_tabid);
	rdf_rb->rdr_types_mask     = 0;
	rdf_rb->rdr_2types_mask    = RDR2_PROCEDURE_PARAMETERS;
	rdf_rb->rdr_instr          = RDF_NO_INSTR;

	rdf_rb->rdr_update_op      = RDR_OPEN;
	rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
	rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
	rdf_cb->rdf_error.err_code = 0;

	tuples_to_fill = (i4) ptuple.db_parameterCount;
	if (tuples_to_fill != numcols)
	{
	    opx_error_number = E_OP08AF_GTTPARM_MISMATCH; 
	    opx_error_arg1 = (PTR)&gtt_name;
	    opx_error_arg2 = (PTR)&proc_name;		/* mismatch */
	}
    }

    while (!opx_error_number && rdf_cb->rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	rdf_rb->rdr_update_op = RDR_GETNEXT;

	/* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	** E_DB_WARN that would be missed.
	*/
	if (status != E_DB_OK)
	{
	    switch(rdf_cb->rdf_error.err_code)
	    {
		case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;
		    continue;

		default:
		    opx_error_number = E_OP0013_READ_TUPLES;
		    break;

	    }	    /* switch */

	    if (opx_error_number)
		break;

	}	/* if status != E_DB_OK */

	/* For each dbproc parameter tuple */
	for 
	(
	    qp = rdf_cb->rdf_info_blk->rdr_pptuples->qrym_data,
	    tupcount = 0;
	    !opx_error_number && 
	    (tupcount < rdf_cb->rdf_info_blk->rdr_pptuples->qrym_cnt);
	    qp = qp->dt_next,
	    tupcount++
	)
	{
	    DB_PROCEDURE_PARAMETER *param_tup =
		(DB_PROCEDURE_PARAMETER *) qp->dt_data;
	    i4		colindx;
	    if (tuples_to_fill-- == 0)
	    {
		opx_error_number = E_OP08A2_EXCEPTION; 
		break;
	    }
	    for (colindx = 1; colindx <= numcols; colindx++)
	     if ((coldesc = collist[colindx])->att_number ==
			param_tup->dbpp_number)
	    {		/* check matching column/parm */
		if (coldesc->att_type != param_tup->dbpp_datatype ||
		    coldesc->att_width != param_tup->dbpp_length ||
		    coldesc->att_prec != param_tup->dbpp_precision ||
		    coldesc->att_offset != param_tup->dbpp_offset ||
		    MEcmp((PTR)&coldesc->att_name, (PTR)&param_tup->dbpp_name,
			sizeof(DB_ATT_NAME)))
		    {
			opx_error_number = E_OP08AF_GTTPARM_MISMATCH; 
			opx_error_arg1 = (PTR)&gtt_name; 
			opx_error_arg2 = (PTR)&proc_name;		/* mismatch */
			break;
		    }

	    }
	}
    }

    /* 
    ** The while-loop above in the successful case will iterate once per db 
    ** proc parameter. The number of iterations is counted off by 
    ** tuples_to_fill. If however it does not reach 0 then something has gone 
    ** wrong, which could include the database procedure being deleted whilst 
    ** we were trying to get the information back, so we take another look 
    ** with a call to RDF_GETINFO.
    */

    if (!opx_error_number && tuples_to_fill != 0)
    {
	MEcopy ((PTR)&cps->pst_procname.qso_n_id.db_cur_name,
	sizeof(DB_TAB_NAME), (PTR)&rdf_rb->rdr_name.rdr_prcname);
	STRUCT_ASSIGN_MACRO(cps->pst_procname.qso_n_own, rdf_rb->rdr_owner);
	rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
	rdf_rb->rdr_2types_mask = 0;
	rdf_rb->rdr_instr       = RDF_NO_INSTR;

	rdf_cb->rdf_info_blk = (RDR_INFO *) NULL;
    
	status = rdf_call(RDF_GETINFO, rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    if (rdf_cb->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
		opx_error_number = E_OP000C_DBPROC_NOT_FOUND;
	    else
		opx_error_number = E_OP0004_RDF_GETDESC;
	}
	else
	    opx_error_number = E_OP08A2_EXCEPTION;
    }

    /* 
    ** From here on is clean up. We'll attempt to do the clean up even if 
    ** there has been a prior OPX_ERROR unless that error means that the 
    ** clean up isn't logically required. Note that if there has been a 
    ** prior OPX_ERROR, we'll stick with that rather than overwrite it with 
    ** a subsequent one.
    */
    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	if (!opx_error_number && DB_FAILURE_MACRO(status))
	    opx_error_number = E_OP0013_READ_TUPLES;
    }

    /* We check the opx_error_number as we don't need to unfix if the RDF_GETINFO call failed */
    if (opx_error_number != E_OP000C_DBPROC_NOT_FOUND && opx_error_number != E_OP0004_RDF_GETDESC)
    {
    	/* now unfix the dbproc description */
	rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
	rdf_rb->rdr_2types_mask = 0;
	rdf_rb->rdr_instr       = RDF_NO_INSTR;

	status = rdf_call(RDF_UNFIX, rdf_cb);
	if (!opx_error_number && DB_FAILURE_MACRO(status))
	    opx_error_number = E_OP008D_RDF_UNFIX;
    }

    /* and unfix the temptab description */
    rdf_cb->rdf_info_blk = rdrinfo;
    rdf_rb->rdr_types_mask = RDR_RELATION;

    status = rdf_call(RDF_UNFIX, rdf_cb);
    if (!opx_error_number && DB_FAILURE_MACRO(status))
	opx_error_number = E_OP008D_RDF_UNFIX;

    if (opx_error_number)
    {
	if (opx_error_arg1 == NULL)
	    opx_error(opx_error_number);
	else if (opx_error_arg2 == NULL)
	    opx_1perror(opx_error_number, opx_error_arg1);
	else
	    opx_2perror(opx_error_number, opx_error_arg1, opx_error_arg2);
    }
}

/*{
** Name: OPC_BUILD_CP_ATTRIBUTE_LIST	- D.
**
** Description:
**
** Inputs:
**	global	    - the global control block for compilation
**
**	procedureID - ID of procedure
**
**	p_attr_list - pointer to where attribute list should be returned.
**
**	p_offset_list - pointer to where offset list should be returned.
**
** Outputs:
**	Returns:
**
**	*p_attr_list - attribute list for temptable create for procedure.
**
**	*p_offset_list - list of row offsets for parameters.
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	08-oct-92 (jhahn)
**	    initial creation
**	10-feb-93 (teresa)
**	    Changed to use new RDF_READTUPLES interface.  Also changed incorrect
**	    E_OP0004 error message to E_OP0013_READ_TUPLES.
**	04-mar-93 (andre)
**	    before asking RDF for descriptions of dbproc parameters, we need to
**	    obtain a description of a dbproc itself
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	    
[@history_template@]...
*/
VOID
opc_build_cp_attribute_list(
    OPS_STATE	*global,
    DB_TAB_ID	*procedureID,
    DMF_ATTR_ENTRY	**p_attr_list[],
    i4		*p_offset_list[])
{
    DMF_ATTR_ENTRY	**attr_list;
    i4		*offset_list;
    DMF_ATTR_ENTRY	*attr;
    RDF_CB		*rdf_cb = &global->ops_rangetab.opv_rdfcb;
    RDR_RB		*rdf_rb = &rdf_cb->rdf_rb;
    DB_PROCEDURE	*ptuple = global->ops_statement->
				  pst_specific.pst_callproc.pst_ptuple;
    i4			tuples_to_fill;
    DB_STATUS		status;
    QEF_DATA		*qp;
    i4			tupcount;

    /*
    ** get the dbproc description before looking up descriptions of its
    ** parameters
    */
    STRUCT_ASSIGN_MACRO(ptuple->db_dbpname, rdf_rb->rdr_name.rdr_prcname);
    STRUCT_ASSIGN_MACRO(ptuple->db_owner, rdf_rb->rdr_owner);
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    /*
    ** need to set rdf_info_blk to NULL for otherwise RDF assumes that we
    ** already have the info_block
    */
    rdf_cb->rdf_info_blk = (RDR_INFO *) NULL;
    
    status = rdf_call(RDF_GETINFO, rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	{
	    /* wonder where PSF got that IIPROCEDURE tuple... */
	    opx_error(E_OP000C_DBPROC_NOT_FOUND);
	}
	else
	{
	    /* this is mighty unfortunate */
	    opx_error(E_OP0004_RDF_GETDESC);
	}
    }
    
    STRUCT_ASSIGN_MACRO(*procedureID, rdf_rb->rdr_tabid);
    rdf_rb->rdr_types_mask     = 0;
    rdf_rb->rdr_2types_mask    = RDR2_PROCEDURE_PARAMETERS;
    rdf_rb->rdr_instr          = RDF_NO_INSTR;

    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;	/* get all tuples */
    rdf_rb->rdr_qtuple_count   = 20;	/* get 20 at a time */
    rdf_cb->rdf_error.err_code = 0;

    /*
    ** must set rdr_rec_access_id since otherwise RDF will barf when we
    ** try to RDR_OPEN
    */
    rdf_rb->rdr_rec_access_id  = NULL;

    tuples_to_fill = (i4) ptuple->db_parameterCount;
    /* if (tuples_to_fill == 0)
	opx_error(E_OP0880_NOT_READY); /* To be general purpose should allow this */
    attr_list = *p_attr_list = (DMF_ATTR_ENTRY **)
	opu_qsfmem(global, sizeof(DMF_ATTR_ENTRY *) * tuples_to_fill);
    offset_list = *p_offset_list = (i4 *)
	 opu_Gsmemory_get(global, sizeof(i4) * tuples_to_fill);

    while (rdf_cb->rdf_error.err_code == 0)
    {
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	rdf_rb->rdr_update_op = RDR_GETNEXT;

	/* Must not use DB_FAILURE_MACRO because E_RD0011 returns
	** E_DB_WARN that would be missed.
	*/
	if (status != E_DB_OK)
	{
	    switch(rdf_cb->rdf_error.err_code)
	    {
		case E_RD0011_NO_MORE_ROWS:
		    status = E_DB_OK;
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    status = E_DB_OK;
		    continue;

		default:
		    opx_error(E_OP0013_READ_TUPLES);
	    }	    /* switch */
	}	/* if status != E_DB_OK */

	/* For each dbproc parameter tuple */
	for 
	(
	    qp = rdf_cb->rdf_info_blk->rdr_pptuples->qrym_data,
	    tupcount = 0;
	    tupcount < rdf_cb->rdf_info_blk->rdr_pptuples->qrym_cnt;
	    qp = qp->dt_next,
	    tupcount++
	)
	{
	    DB_PROCEDURE_PARAMETER *param_tup =
		(DB_PROCEDURE_PARAMETER *) qp->dt_data;
	    if (tuples_to_fill-- == 0)
		opx_error(E_OP08A2_EXCEPTION);
	    attr = attr_list[param_tup->dbpp_number-1] = (DMF_ATTR_ENTRY *)
		opu_qsfmem(global, sizeof(DMF_ATTR_ENTRY));
	    offset_list[param_tup->dbpp_number-1] = param_tup->dbpp_offset;
	    STRUCT_ASSIGN_MACRO(param_tup->dbpp_name, attr->attr_name);
	    attr->attr_type = param_tup->dbpp_datatype;
	    attr->attr_size = param_tup->dbpp_length;
	    attr->attr_precision = param_tup->dbpp_precision;
	    attr->attr_flags_mask = 0;
	    if (param_tup->dbpp_flags & DBPP_NOT_NULLABLE)
		attr->attr_flags_mask |= DMU_F_KNOWN_NOT_NULLABLE;
	    if (param_tup->dbpp_flags & DBPP_NDEFAULT)
		attr->attr_flags_mask |= DMU_F_NDEFAULT;
	    STRUCT_ASSIGN_MACRO(param_tup->dbpp_defaultID,
				attr->attr_defaultID);
	    attr->attr_defaultTuple = NULL;
	}
    }

    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	rdf_rb->rdr_update_op = RDR_CLOSE;
	status = rdf_call(RDF_READTUPLES, rdf_cb);
	if (DB_FAILURE_MACRO(status))
	    opx_error(E_OP0013_READ_TUPLES);
    }

    /* now unfix the dbproc description */
    rdf_rb->rdr_types_mask  = RDR_PROCEDURE | RDR_BY_NAME;
    rdf_rb->rdr_2types_mask = 0;
    rdf_rb->rdr_instr       = RDF_NO_INSTR;

    status = rdf_call(RDF_UNFIX, rdf_cb);
    if (DB_FAILURE_MACRO(status))
	opx_error(E_OP008D_RDF_UNFIX);

    if (tuples_to_fill != 0)
	opx_error(E_OP08A2_EXCEPTION);
}

/*{
** Name: OPC_PROC_INSERT_AHD_BUILD	- Build an AHD for a deferred
**					  CALLPROC statement.
**
** Description:
**	This routine cooridnates the building of an action header for a
**	PROC_INSERTT action of a CALL PROCEDURE statement.
**
** Inputs:
**	global	    - the global control block for compilation
**
**	new_ahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	Returns:
**	    Pointer to newly created AHD in the address specified by new_ahd.
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	08-oct-92 (jhahn)
**	    Written to support statement level rule processing
**	12-feb-93 (jhahn)
**	    More support for statement level rules.
**	02-apr-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	20-mar-96 (nanpr01)
**	    Call opc_pagesize to determine the temporary table page_size
**	    based on tuple length. This is need for variable page size
**	    project.
**	8-july-96 (inkdo01)
**	    Added additional call_proc/proc_insert semantics for statement 
**	    level rules support.
[@history_template@]...
*/
VOID
opc_build_proc_insert_ahd(
    OPS_STATE	*global,
    QEF_AHD		**pahd)
{
    i4			ninstr, nops, nconst, max_base;
    i4		szconst;
    PST_QNODE		*root;
    QEF_AHD		*ahd;
    OPC_PST_STATEMENT	*opc_pst;
    OPC_ADF		cadf;
    DMF_ATTR_ENTRY	**attr_list;
    i4		*offset_list;
    DB_PROCEDURE	*ptuple = global->ops_statement->
				  pst_specific.pst_callproc.pst_ptuple;
    
    if (ptuple->db_parameterCount == 0)
    {		/* if no parms, don't need a PROC_INSETR */
	*pahd = (QEF_AHD *) NULL;
	return;
    }

    if (!(ptuple->db_mask[0] & DBP_SETINPUT))
    {
	/* Statement level rule must invoke a "set of" procedure. */
	OPT_NAME	proc_name;
	STncpy((char *)&proc_name, (char *)&ptuple->db_dbpname, DB_DBP_MAXNAME);
	((char *)(&proc_name))[ DB_DBP_MAXNAME ] = '\0';
	opx_1perror(E_OP08AE_NOTSETI_PROC, (PTR)&proc_name);
    }

    ahd = *pahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_PROC_INSERT));

    /* If this is the first AHD or the top AHD, set the proper status.	    */
    /* 'first ahd' is the first AHD of this QP.				    */
    /* 'top ahd' is the top(first) AHD for this QEP. */
    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_PROC_INSERT);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_PROC_INSERT;
    ahd->ahd_valid = NULL;
    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Now define the CALLPROC action information in the AHD.		    */
    /* Copy the procedure name and the number of parameters to the AHD.	    */
    /* Both these values were computed by PSF. Note that a CALLPROC name    */
    /* is a QSO_NAME(alias) so that procedure references will never be	    */
    /* ambiguous. */
    opc_build_cp_attribute_list(global,
				&ptuple->db_procid,
				&attr_list, &offset_list);
    
    opc_tempTable(global, &ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable,
		  ptuple->db_recordWidth, (OPC_OBJECT_ID **) NULL);
    ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_attr_list = attr_list;
    ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_attr_count =
	ptuple->db_parameterCount;
    ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_pagesize = 
	opc_pagesize(global->ops_cb->ops_server, 
			(OPS_WIDTH) ptuple->db_recordWidth);

    /* compile the expresion to compute    */
    /* the actual parameters for each PROC_INSERT invocation. */

    /* The parser has created a QTREE that contains a target list that  */
    /* computes the actual parameter values. Get the pointer to the	    */
    /* first RESDOM node in the tree (we assume the is no ROOT node at  */
    /* the top of this tree but that there is a TREE node after the	    */
    /* last RESDOM. */

    root = global->ops_statement->pst_specific.pst_callproc.pst_arglist;
    
    /* Create a new QEN_ADF struct to contain the definition of the	    */
    /* expression that computes the parameter list. */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    /*
     ** The magic number is 5 for the following bases:
     **	2  -  one for old row and one for new row
     **	2  -  one for old tid and one for new tid
     **	1  -  contains the parameters list we are actually building
     */
    max_base = 5;
    opc_cxest(global, (OPC_NODE *)NULL, root, &ninstr, &nops, &nconst, 
	      &szconst);
    opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_proc_insert.ahd_procparams,
		ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
    
    /* Add a base for the resulting parameter list */
    opc_adbase(global, &cadf, QEN_ROW,
	    ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable->ttb_tuple);

    /* Now compile the TREE expression that computes the parameter list for insert*/
    opc_proc_insert_target_list(global, root, &cadf,
				global->ops_cstate.opc_rdfdesc,
				global->ops_statement->pst_specific.
				pst_callproc.pst_argcnt,
				ptuple->db_parameterCount,
				attr_list, 
				cadf.opc_row_base[ahd->qhd_obj.qhd_proc_insert.
						  ahd_proc_temptable->ttb_tuple],
				offset_list);
    opc_adend(global, &cadf);

    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

}

/*{
** Name: OPC_ATTACH_AHD_TO_TRIGGER	- Attaches an ahd to the triggering ahd.
**
** Description:
**	Used while compiling rules to attach an action header to the ahd
**	associated with the firing rule. This will create a QEA_INVOKE_RULE
**	action if one does not already exist for the triggering actio
** Inputs:
**	global	    - the global control block for compilation
**
**	new_ahd	    - pointer to the ahd to attach.
**		      the new AHD.
** Outputs:
**	Returns:
**
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    Attaches ahd to to triggering statement.
**
** History:
**	08-oct-92 (jhahn)
**	    Created
**	17-may-93 (jhahn)
**	    Added creation of QEA_INVOKE_RULE action.
**	28-may-93 (jhahn)
**	    Fixed problem with multiple QEA_INVOKE_RULE actions being generated.
**	    Added code to bump qp_ahd_cnt.
**	    
[@history_template@]...
*/
VOID
opc_attach_ahd_to_trigger(
    OPS_STATE	*global,
    QEF_AHD	*new_ahd)
{
    OPC_PST_STATEMENT	*opc_pst = (OPC_PST_STATEMENT *)
	global->ops_trigger->pst_opf;
    QEF_AHD	*ahd, *head, *tail;


    if (global->ops_cstate.opc_invoke_rule_ahd == NULL)
    {
	/* Build an INVOKE_RULE ahd and point its action list to new_ahd */
	global->ops_cstate.opc_invoke_rule_ahd = ahd =
	    (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_INVOKE_RULE));
	MEfill((sizeof(QEF_AHD)-sizeof(ahd->qhd_obj)+
		sizeof(QEF_INVOKE_RULE)), (u_char)0, (PTR)ahd);
	
	ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_INVOKE_RULE);
	ahd->ahd_type = QEAHD_CB;
	ahd->ahd_ascii_id = QEAHD_ASCII_ID;
	ahd->ahd_atype = QEA_INVOKE_RULE;
	ahd->ahd_valid = NULL;
	ahd->qhd_obj.qhd_invoke_rule.ahd_rule_action_list = new_ahd;
	new_ahd->ahd_next = NULL;
	new_ahd->ahd_prev = new_ahd;
	head = opc_pst->opc_stmtahd;
	global->ops_cstate.opc_qp->qp_ahd_cnt++;
    }
    else
    {
	head = global->ops_cstate.opc_invoke_rule_ahd->
	    qhd_obj.qhd_invoke_rule.ahd_rule_action_list;
	ahd = new_ahd;
    }
    /* Now we will either put the newly built INVOKE_RULE action at the tail
    ** of actions for the triggering statement, or we will put new_ahd at
    ** the tail of actions of the action list of the previously built
    ** INVOKE_RULE action.
    */
    tail = head->ahd_prev;
    ahd->ahd_next = tail->ahd_next;
    ahd->ahd_prev = tail;
    tail->ahd_next = ahd;
    head->ahd_prev = ahd;
}

/*{
** Name: OPC_BUILD_CALL_DEFERRED_PROC_AHD	- Build this ahd.
**
** Description:
**	Builds CALL_DEFERRED_PROC ahd and attaches it to the triggering
**	statement.
** Inputs:
**
**	global		- the global control block for compilation
**
**	proc_insert_ahd	- pointer associated PROC_INSERT ahd.
**
** Outputs:
**	Returns: none
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	Attaches ahd to triggering statement.
**	    [@description_or_none@]
**
** History:
**	08-oct-92 (jhahn)
**	    Created
**	12-feb-93 (jhahn)
**	    More support for statement level rules.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	28-may-93 (jhahn)
**	    Added code to bump qp_ahd_cnt.
**	8-july-96 (inkdo01)
**	    Added support for statement level rules (and possibly empty 
**	    proc_insert parm list).
**      13-jun-97 (dilma04)
**          Set QEN_CONSTRAINT flag in QEN_ADF, if this procedure supports
**          an integrity constraint.
**	15-Jan-2010 (jonj)
**	    A procedure may be a constraint even if it has no parameters.
**	    Set QEF_CP_CONSTRAINT in ahd_flags, deprecate QEN_CONSTRAINT.
[@history_template@]...
*/
static VOID
opc_build_call_deferred_proc_ahd(
    OPS_STATE	*global,
    QEF_AHD	*proc_insert_ahd)
{
    DB_PROCEDURE	*ptuple = global->ops_statement->
				  pst_specific.pst_callproc.pst_ptuple;
    QEF_AHD	*ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CALLPROC));

    MEfill((sizeof(QEF_AHD)-sizeof(ahd->qhd_obj)+
		sizeof(QEF_CALLPROC)), (u_char)0, (PTR)ahd);

    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CALLPROC);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_CALLPROC;
    ahd->ahd_valid = NULL;

    STRUCT_ASSIGN_MACRO(global->ops_statement->pst_specific.
			pst_callproc.pst_rulename,
			ahd->qhd_obj.qhd_callproc.ahd_rulename);
    STRUCT_ASSIGN_MACRO(global->ops_statement->pst_specific.
			pst_callproc.pst_ruleowner,
			ahd->qhd_obj.qhd_callproc.ahd_ruleowner);

    MEcopy((PTR)&global->ops_statement->pst_specific.pst_callproc.pst_procname,
	   sizeof(QSO_NAME),
	   (PTR)&ahd->qhd_obj.qhd_callproc.ahd_dbpalias);
    STRUCT_ASSIGN_MACRO(ptuple->db_procid,
			ahd->qhd_obj.qhd_callproc.ahd_procedureID);
    ahd->qhd_obj.qhd_callproc.ahd_procp_return = (QEN_ADF *) NULL;

    if (proc_insert_ahd != NULL)
      ahd->qhd_obj.qhd_callproc.ahd_proc_temptable =
	proc_insert_ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable;
    else ahd->qhd_obj.qhd_callproc.ahd_proc_temptable = NULL;

    if (ptuple->db_mask[0] & DBP_CONS)
	ahd->qhd_obj.qhd_callproc.ahd_flags |= QEF_CP_CONSTRAINT;
    else
	ahd->qhd_obj.qhd_callproc.ahd_flags &= ~QEF_CP_CONSTRAINT;

    opc_attach_ahd_to_trigger(global, ahd);
    global->ops_cstate.opc_qp->qp_ahd_cnt++;
}

/*{
** Name: OPC_BUILD_CLOSETEMP_AHD	- Builds a CLOSETEMP ahd.
**
** Description:
**	Builds CLOSETEMP ahd and attaches it to the triggering
**	statement.
** Inputs:
**
**	global		- the global control block for compilation
**
**	proc_insert_ahd	- pointer associated PROC_INSERT ahd.
**
** Outputs:
**	Returns: none
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	Attaches ahd to triggering statement.
**
** History:
**	08-oct-92 (jhahn)
**	    Written.
**	28-may-93 (jhahn)
**	    Added code to bump qp_ahd_cnt.
**	    
[@history_template@]...
*/
VOID
opc_build_closetemp_ahd(
    OPS_STATE	*global,
    QEF_AHD	*proc_insert_ahd)
{
    QEF_AHD	*ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CLOSETEMP));

    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CLOSETEMP);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_CLOSETEMP;
    ahd->ahd_valid = NULL;

    ahd->qhd_obj.qhd_closetemp.ahd_temptable_to_close =
	proc_insert_ahd->qhd_obj.qhd_proc_insert.ahd_proc_temptable;

    opc_attach_ahd_to_trigger(global, ahd);
    global->ops_cstate.opc_qp->qp_ahd_cnt++;
}

/*{
** Name: OPC_DEFERRED_CPAHD_BUILD	- Build an AHD for a deferred CALLPROC statement.
**
** Description:
**	This routine cooridnates the building of an action header for a
**	CALL PROCEDURE statement.
**
** Inputs:
**	global	    - the global control block for compilation
**
**	pahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	Returns:
**	    Pointer to newly created AHD in the address specified by new_ahd.
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	Actions will be attached to the triggering statement.
**	    [@description_or_none@]
**
** History:
**	08-oct-92 (jhahn)
**	    Written to support statement level rule processing
[@history_template@]...
*/
VOID
opc_deferred_cpahd_build(
    OPS_STATE	*global,
    QEF_AHD	**pahd)
{
    opc_build_proc_insert_ahd(global, pahd);
    opc_build_call_deferred_proc_ahd(global, *pahd);
    if (*pahd != NULL)
      opc_build_closetemp_ahd(global, *pahd);
}

/*{
** Name: OPC_IPROCAHD_BUILD	- Build an AHD for a QEA_IPROC statement.
**
** Description:
**	This routine coordinates the building of an action header for a
**	DBMS internal DBP execution statement.
**
** Inputs:
**	global	    - the global control block for compilation
**
**	new_ahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	new_ahd:
**	    Pointer to newly created AHD in the address specified by new_ahd.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-apr-89 (eric)
**          created.
**	14-jun-89 (paul)
**	    The ahd_mkey expression for the first action in a QP contains
**	    the initialization code for procedure local variables. This
**	    structure was not being created for MESSAGE, RAISE ERROR,
**	    COMMIT, ROLLBACK, CALLPROC or IPROC statements.
[@history_template@]...
*/
VOID
opc_iprocahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;
    OPC_ADF		cadf;

    /* First, lets allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_IPROC));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_IPROC);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_IPROC;
    ahd->ahd_valid = NULL;
    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Now lets go on to the IPROC specific stuff. */
    STRUCT_ASSIGN_MACRO(
		global->ops_statement->pst_specific.pst_iproc.pst_procname,
		ahd->qhd_obj.qhd_iproc.ahd_procnam
			);

    STRUCT_ASSIGN_MACRO(
		global->ops_statement->pst_specific.pst_iproc.pst_ownname,
		ahd->qhd_obj.qhd_iproc.ahd_ownname
			);

    /* Finally, mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
}

/*{
** Name: OPC_RPROCAHD_BUILD	- Build an AHD for a QEA_REGPROC statement.
**
** Description:
**	This routine coordinates the building of an action header for a
**	registered DB procedure execution statement.
**
** Inputs:
**	global	    - the global control block for compilation
**
**	pahd	    - pointer to a pointer in which to place the address of
**		      the new AHD.
**
** Outputs:
**	pahd:
**	    Pointer to newly created AHD in the address specified by pahd.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-dec-92 (teresa)
**          created (used opc_iprocahd_build as a model).
**	20-jan-93 (anitap)
**	    Fixed compiler warning.
[@history_template@]...
*/
VOID
opc_rprocahd_build(
		OPS_STATE	*global,
		QEF_AHD		**pahd)
{
    OPC_PST_STATEMENT	*opc_pst;
    QEF_AHD		*ahd;
    OPC_ADF		cadf;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    i4		row_size;

    /* allocate the action header */
    ahd = *pahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEQ_D10_REGPROC));

    if (global->ops_cstate.opc_topahd == NULL)
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *) global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEQ_D10_REGPROC);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_D10_REGPROC;
    ahd->ahd_valid = NULL;
    global->ops_cstate.opc_flags = OPC_REGPROC;  /* indicate this is a 
						 ** single statement, which is
						 ** an execute of a registered
						 ** procedure */
    
    /* there are really no rows for a registered procedure, but lower level
    ** opc code gets confused with 0 rows, so tell it there's one row so it
    ** can allocate a small piece of memory and be happy.
    */
    global->ops_cstate.opc_qp->qp_row_cnt = 1;
    qp->qp_row_cnt = 1;
    row_size = sizeof (*qp->qp_row_len) * qp->qp_row_cnt;
    qp->qp_row_len = (i4 *) opu_qsfmem(global, row_size);
    qp->qp_row_len[0] = sizeof (row_size);

    /* call opc_bgmkey() since this fills in local vars with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Begin populating the REGPROC specific data.
    **
    */
    ahd->qhd_obj.qed_regproc.qeq_r1_lang = DB_SQL;  /* only allow sql in star */
    ahd->qhd_obj.qed_regproc.qeq_r2_quantum = 0;    /* no timeout for now.
						    ** may want to change in the
						    ** future */
    ahd->qhd_obj.qed_regproc.qeq_r4_csr_handle = 0; /* Not clear if this is
						    ** needed, so start wihtout
						    ** it -- teresa
						    */
    /* Get the REGPROC descriptor from Query Tree and put into QP for QEF.
    ** Start by allocating memory to hold the DD_REGPROC_DESC and then
    ** populate it from the one PSF supplied 
    */
    ahd->qhd_obj.qed_regproc.qeq_r3_regproc = 
	(DD_REGPROC_DESC *) opu_qsfmem(global, sizeof (DD_REGPROC_DESC));
    STRUCT_ASSIGN_MACRO(
		*global->ops_statement->pst_specific.pst_regproc.pst_rproc_desc,
		*ahd->qhd_obj.qed_regproc.qeq_r3_regproc);
    ahd->qhd_obj.qed_regproc.qeq_r3_regproc->dd_p5_ldbdesc = 
	(DD_LDB_DESC *) opu_qsfmem(global, sizeof (DD_LDB_DESC));
    STRUCT_ASSIGN_MACRO(*global->ops_statement->pst_specific.pst_regproc.
			    pst_rproc_desc->dd_p5_ldbdesc,
		       *ahd->qhd_obj.qed_regproc.qeq_r3_regproc->dd_p5_ldbdesc);

    /* mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    /* set up qp->qp_ddq_cb */
    MEfill (sizeof(qp->qp_ddq_cb), (u_i4) 0, (PTR) &qp->qp_ddq_cb);
}

/*{
** Name: opc_addrules 	- Set up preliminary rule data structures
**
** Description:
**	Determine if rules apply to this statemen and, if they do, set up
**	the preliminary data structures to allow the rule action to be attached
**	later.
**
** Inputs:
**	global			    Global OPC descriptor
**	    .ops_statement	    The triggering statement (if pst_after_stmt
**				    is not NULL).
**	        .ops_after_stmt	    Rule statement list (may be NULL and in this
**				    case rule actions are not attached).
**	ahd			    Action header - this must logically be the
**				    "top action", and this is saved as the
**				    action to apply rules rules too.
**	rel			    RDF descriptor of table that rule may be
**				    applied to. 
**
** Outputs:
**	global			    Global OPC descriptor
**	    .ops_inAfterRules	    If there were after rules applied then this
**				    is set otherwise this is cleared and other 
**				    rule fields are ignored.
**	    .ops_inBeforeRules	    If there were before rules applied then this
**				    is set otherwise this is cleared and other 
**				    rule fields are ignored.
**	    .ops_trigger	    Original PST statement that fired the rule
**	    .ops_firstAfterRule	    First PST statement in after rule list
**	    .ops_firstBeforeRule    First PST statement in before rule list
**	    .ops_cstate
**	        .opc_rule_ahd	    First rule action header.  Set to ahd.  This
**				    is saved to allow the rule to later be
**				    attached to the correct action (ie, there
**				    may be DBU's included but we want the rule
**				    to be attached to the real DML action).
**	ahd			    Action header - if rules were applied then
**				    the action is primed with old/new rows, tid
**				    rows and other rule data structures.
**	Returns:
**	    VOID
**
** History:
**      29-jan-89 (paul)
**          Wrote for rules processing.
**	12-feb-93 (jhahn)
**	    Support for statement level rules.
**	24-mar-93 (jhahn)
**	    Various fixes for support of statement level rules. (FIPS)
**	17-may-93 (jhahn)
**	    Moved code for allocating before row for replace actions from
**	    here to opc_crupcurs.
**	3-feb-04 (inkdo01)
**	    Change TID operands to TID8 for table partitioning.
**	15-june-05 (dougi)
**	    Add support for "before" triggers.
*/

static VOID
opc_addrules(
	OPS_STATE   *global,
	QEF_AHD	    *ahd,
	RDR_INFO    *rel )
{
    /* Determine if any rules apply to this statement.			    */
    if (ahd == global->ops_cstate.opc_topahd &&
	(global->ops_statement->pst_after_stmt != NULL ||
	 global->ops_statement->pst_before_stmt != NULL))
    {
	OPS_SUBQUERY	*subqry;

	subqry = global->ops_cstate.opc_subqry;
	
	/* We have a rule statement list, set the environment for	    */
	/* processing rules. The major effort is to arrange space for the   */
	/* before and after images of the row that was just updated.	    */
	/* For append and replace					    */
	/*	qen_output contains the newly updated row.		    */
	/*	   _reprow contains the old row if it is != -1		    */
	/* For delete							    */
	/*	no row is available */
	switch (ahd->ahd_atype)
	{
	    case QEA_UPD:
	    case QEA_PUPD:
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row =
		    global->ops_cstate.opc_after_row =
		    (ahd->qhd_obj.qhd_qep.ahd_current) ?
		    	ahd->qhd_obj.qhd_qep.ahd_current->qen_output : -1;

		/* Also point to the descriptor for the result relation for use
		** in rule compilation. We do not deallocate the descriptor
		** array for the current QEP until AFTER rules are compiled.
		** This descriptor is needed to compile references to before
		** and after values in the rule list.
		*/
		global->ops_cstate.opc_rdfdesc =
		    global->ops_rangetab.opv_base->opv_grv[subqry->ops_result]->
		    opv_relation;

		/*
		** Add space to remember the before and after TID values in case
		** the user decides to pass the TID as a parameter to the rule
		** action. There are special DSH_ROWs in which to place the before
		** and after TID.
		**
		*/
		opc_ptrow(global, &global->ops_cstate.opc_bef_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid =
		    global->ops_cstate.opc_bef_tid;
		opc_ptrow(global, &global->ops_cstate.opc_aft_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid =
		    global->ops_cstate.opc_aft_tid;
		break;

	    case QEA_APP:
		/* In this case we only have after values. Point both the   */
		/* before and after rows at the after values as specified   */
		/* by the semantics for rules. */
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row = global->ops_cstate.opc_before_row =
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row = global->ops_cstate.opc_after_row =
		    (ahd->qhd_obj.qhd_qep.ahd_current) ?
		    	ahd->qhd_obj.qhd_qep.ahd_current->qen_output : -1;

		/* Also point to the descriptor for the result relation for use
		** in rule compilation. We do not deallocate the descriptor
		** array for the current QEP until AFTER rules are compiled.
		** This descriptor is needed to compile references to before
		** and after values in the rule list.
		*/
		global->ops_cstate.opc_rdfdesc =
		    global->ops_rangetab.opv_base->opv_grv[subqry->ops_result]->
		    opv_relation;

		/*
		** Add space to remember the before and after TID values in case
		** the user decides to pass the TID as a parameter to the rule
		** action. There are special DSH_ROWs in which to place the before
		** and after TID.
		**
		** Before and after are the same for insert
		*/
		opc_ptrow(global, &global->ops_cstate.opc_bef_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid =
		    ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid =
		    global->ops_cstate.opc_aft_tid =
		    global->ops_cstate.opc_bef_tid;
	    
		break;
		
	    case QEA_DEL:
	    case QEA_PDEL:
		/* Do not have space for any rows. Reserve space for a copy */
		/* of the old row values and point both the before and	    */
		/* after rows at the before values. */
		opc_ptrow(global, &global->ops_cstate.opc_before_row,
		    global->ops_rangetab.opv_base->opv_grv[subqry->ops_result]->
		    opv_relation->rdr_rel->tbl_width);
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row = global->ops_cstate.opc_after_row =
		    ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row = global->ops_cstate.opc_before_row;
		ahd->qhd_obj.qhd_qep.ahd_repsize =
		    global->ops_rangetab.opv_base->opv_grv[subqry->
		    ops_result]->opv_relation->rdr_rel->tbl_width;

		/* Also point to the descriptor for the result relation for use
		** in rule compilation. We do not deallocate the descriptor
		** array for the current QEP until AFTER rules are compiled.
		** This descriptor is needed to compile references to before
		** and after values in the rule list.
		*/
		global->ops_cstate.opc_rdfdesc =
		    global->ops_rangetab.opv_base->opv_grv[subqry->ops_result]->
		    opv_relation;

		/*
		** Add space to remember the before and after TID values in case
		** the user decides to pass the TID as a parameter to the rule
		** action. There are special DSH_ROWs in which to place the before
		** and after TID.
		**
		** Before and after are the same for delete
		*/
		opc_ptrow(global, &global->ops_cstate.opc_bef_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid =
		    ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid =
		    global->ops_cstate.opc_aft_tid =
		    global->ops_cstate.opc_bef_tid;
	    
		break;

	    case QEA_RUP:
		/* Cursor update processing
		** has already identified a row in which the updated row values
		** can be found. In summary:
		**	qen_output contains the newly updated row
		**	allocate a before image.
		** There is a bit of a trick here, since an output row has not
		** really been allocated; the cursor update code simply
		** references back to the parent QP. We will reserve space for
		** both an old and a new row.
		*/
		opc_ptrow(global, &global->ops_cstate.opc_after_row,
		    rel->rdr_rel->tbl_width);
		opc_ptrow(global, &global->ops_cstate.opc_before_row,
		    rel->rdr_rel->tbl_width);
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
		    global->ops_cstate.opc_before_row;
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row = global->ops_cstate.opc_after_row /* =
		    (ahd->qhd_obj.qhd_qep.ahd_current) ?
		    	ahd->qhd_obj.qhd_qep.ahd_current->qen_uoutput : -1*/;
		ahd->qhd_obj.qhd_qep.ahd_repsize = rel->rdr_rel->tbl_width;

		/* Also point to the descriptor for the result relation for use
		** in rule compilation. We do not deallocate the descriptor
		** array for the current QEP until AFTER rules are compiled.
		** This descriptor is needed to compile references to before
		** and after values in the rule list.
		**
		** NOTE: This is a different assignment than for non-cursor
		** operations.
		*/
		global->ops_cstate.opc_rdfdesc = rel;

		/*
		** Add space to remember the before and after TID values in case
		** the user decides to pass the TID as a parameter to the rule
		** action. There are special DSH_ROWs in which to place the before
		** and after TID.
		**
		*/
		opc_ptrow(global, &global->ops_cstate.opc_bef_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid =
		    global->ops_cstate.opc_bef_tid;
		opc_ptrow(global, &global->ops_cstate.opc_aft_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid =
		    global->ops_cstate.opc_aft_tid;
	    
		break;

	    case QEA_RDEL:
		opc_ptrow(global, &global->ops_cstate.opc_before_row,
		    rel->rdr_rel->tbl_width);
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row =
		    global->ops_cstate.opc_before_row;
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_row = global->ops_cstate.opc_after_row =
		    ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_row;
		ahd->qhd_obj.qhd_qep.ahd_repsize = rel->rdr_rel->tbl_width;

		/* Also point to the descriptor for the result relation for use
		** in rule compilation. We do not deallocate the descriptor
		** array for the current QEP until AFTER rules are compiled.
		** This descriptor is needed to compile references to before
		** and after values in the rule list.
		**
		** NOTE: This is a different assignment than for non-cursor
		** operations.
		*/
		global->ops_cstate.opc_rdfdesc = rel;

		/*
		** Add space to remember the before and after TID values in case
		** the user decides to pass the TID as a parameter to the rule
		** action. There are special DSH_ROWs in which to place the before
		** and after TID.
		**
		** Before and after are the same for delete
		*/
		opc_ptrow(global, &global->ops_cstate.opc_bef_tid, sizeof(DB_TID8));
		ahd->qhd_obj.qhd_qep.u1.s1.ahd_b_tid =
		    ahd->qhd_obj.qhd_qep.u1.s1.ahd_a_tid =
		    global->ops_cstate.opc_aft_tid =
		    global->ops_cstate.opc_bef_tid;
	    
		break;
	}

	/* Do some bookkeeping so that OPC has enough information to	    */
	/* compile the rule list and link it back to the triggering	    */
	/* statement when it is called to process each statement in the	    */
	/* rule list. Set opc_rule_ahd to the current action so we know	    */
	/* where to attach the compiled rule list. Also point ops_trigger   */
	/* to the firing statement (the one we are currently compiling) and */
	/* ops_firstrule to the first statement in the rule list as an	    */
	/* anchor for resolving IF conditions when completing the rule */
	/* compilation. */
	global->ops_cstate.opc_rule_ahd = ahd;
	global->ops_trigger = global->ops_statement;
	global->ops_firstAfterRule = global->ops_statement->pst_after_stmt;
	global->ops_firstBeforeRule = global->ops_statement->pst_before_stmt;
	global->ops_cstate.opc_invoke_rule_ahd = NULL;
	/* Tell OPF to compile the rule list we detected */
	if (global->ops_firstAfterRule)
	    global->ops_inAfterRules = TRUE;
	else global->ops_inBeforeRules = TRUE;
    }
    else
    {
	global->ops_inAfterRules = FALSE;
	global->ops_inBeforeRules = FALSE;
    }
} /* opc_addrules */

/*{
** Name: opc_eventahd_build	- Build an action header for an EVENT statement.
**
** Description:
**	This routine coordinates the building of an action header for an
**	EVENT statement.  The type of the event statement may be RAISE,
**	REGISTER or REMOVE.  The mapping is according to the following:
**
**		PSF:				QEF:
**
**	Type: 	PST_EVREG_TYPE		    	QEA_EVREGISTER
**		PST_EVDEREG_TYPE		QEA_EVDEREG
**		PST_EVRAISE_TYPE		QEA_EVRAISE
**
**	Struct:	PST_EVENTSTMT			QEF_EVENT
**
**	  Name:	DB_ALERT_NAME			DB_ALERT_NAME
**	  Flag:	PST_EVNOSHARE			QEF_EVNOSHARE
**	  Text:	NULL or PST_QNODE		NULL or QEN_ADF
**
**	This routine does not verify that that the flags and text value are only
**	associated with RAISE.  It is up to PSF to guarantee that the values are
**	in sync with the statement types.
**
**	Note that the CX associated with the event text uses the data type
**	DB_VCH_TYPE and the ADF operation type ADE_MECOPY for increased
**	performance.  This implies that QEF routine that extracts
**	the event text must allow for a VARCHAR with maximum text length
**	even though it may truncate the text before sending to the
**	application.
**
** Inputs:
**	global	    		Global control block for compilation.
**	  .ops_statement	Current statement being compiled.
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	 None - statement memory allocation is done upon return to caller.
**
** History:
**	05-sep-89 (neil)
**	    Written for Terminator II/alerters.
**      04-feb-91 (stec)
**          Added missing opr_prec initialization.
**	16-jun-92 (anitap)
**	    Cast naked NULL passed into opc_cxest().
**	6-sec-93 (robf)
**          Copy over event security label.
*/
VOID
opc_eventahd_build(
		OPS_STATE	*global,
		QEF_AHD		**new_ahd)
{
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    OPC_PST_STATEMENT	*opc_pst;
    ADE_OPERAND		ops[3];
    QEF_AHD		*ahd;
    PST_EVENTSTMT	*pst_ev;

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_EVENT));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *)global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_EVENT);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    switch (global->ops_statement->pst_type)
    {
      case PST_EVREG_TYPE:
	ahd->ahd_atype = QEA_EVREGISTER;
	break;
      case PST_EVDEREG_TYPE:
	ahd->ahd_atype = QEA_EVDEREG;
	break;
      case PST_EVRAISE_TYPE:
	ahd->ahd_atype = QEA_EVRAISE;
	break;
    }

    /* Opc_bgmkey fills in local variables (if in DBP) with initial values */
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    opc_bgmkey(global, ahd, &cadf);
    opc_adend(global, &cadf);

    /* Assign the EVENT-specific data */
    pst_ev = &global->ops_statement->pst_specific.pst_eventstmt;

    /* Copy full unique event name */
    STRUCT_ASSIGN_MACRO(pst_ev->pst_event, ahd->qhd_obj.qhd_event.ahd_event);
;

    /* Copy flags */
    if (pst_ev->pst_evflags & PST_EVNOSHARE)
	ahd->qhd_obj.qhd_event.ahd_evflags = QEF_EVNOSHARE;
    else
	ahd->qhd_obj.qhd_event.ahd_evflags = 0;

    /* Copy optional user event value */
    if (pst_ev->pst_evvalue == NULL)
    {
	ahd->qhd_obj.qhd_event.ahd_evvalue = NULL;
    }
    else
    {
	ninstr = 1;
	nops = 2;
	nconst = 0;
	szconst = 0;
	max_base = 3;
	opc_cxest(global, (OPC_NODE *)NULL, pst_ev->pst_evvalue, &ninstr, 
			&nops, &nconst, &szconst);
	opc_adalloc_begin(global, &cadf, &ahd->qhd_obj.qhd_event.ahd_evvalue,
		    ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);
	opc_adbase(global, &cadf, QEN_OUT, 0);

	ops[1].opr_dt = DB_VCH_TYPE;
	opc_crupcurs(global, pst_ev->pst_evvalue, &cadf, &ops[1],
	  	     (RDR_INFO *)NULL, (OPC_NODE *) NULL);
	ops[0].opr_dt = ops[1].opr_dt;
	ops[0].opr_prec = ops[1].opr_prec;
	ops[0].opr_len = ops[1].opr_len;
	ops[0].opr_base = cadf.opc_out_base;
	ops[0].opr_offset = 0;
	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	opc_adend(global, &cadf);
    } /* If there's user text */

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;
} /* opc_eventahd_build */

/*{
** Name: opc_shemaahd_build 	- Build an action header for a CREATE SCHEMA 
**					statement.
**
** Description:
**	This routine coordinates the building of an action header for a
**	CREATE SCHEMA statement. A pointer to the PST_CREATE_SCHEMA structure
**	is copied to the QEA_CREATE_SCHEMA structure. The mapping of the 
**	statement to action is as follows:
**
**		PSF:				QEF:
**	Type:	PST_CREATE_SCHEMA_TYPE		QEA_CREATE_SCHEMA
**
**	Struct:	PST_CREATE_SCHEMA		QEF_CREATE_SCHEMA	
**
**	Name:	DB_SCHEMA_NAME			DB_SCHEMA_NAME
**	Owner:	DB_OWN_NAME			DB_OWN_NAME	
**
** Inputs:
**      global   - the global control block for compilation
**
**      sahd     - pointer to a pointer in which to place the address of
**                    the new AHD.
**
** Outputs:
**      sahd:
**          pointer to newly created AHD in the address specified by sahd.
**
** Side Effects:
**          none
**
** History:
**	23-nov-92 (anitap)
**	    Created.
**	20-dec-92 (anitap)
**	    Fixed bug.
**	04-mar-93 (anitap)
**	    Changed VOID to void.
**	07-jul-93 (anitap)
**	    Changed opc_schema to be a pointer.
[@history_template@]...
*/
void
opc_shemaahd_build(
		  OPS_STATE	*global,
		  QEF_AHD	**sahd)
{
    QEF_AHD		*ahd;
    PST_CREATE_SCHEMA	*opc_schema;

    opc_schema = &global->ops_statement->pst_specific.pst_createSchema;

    /* First, lets allocate the action header */
    ahd = *sahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_SCHEMA));

    /* If this is the first AHD or the top AHD, set the proper status.  */
    /* 'first ahd' is the first AHD of this QP.                         */
    /* 'top ahd' is the top(first) AHD for this QEP. */
    if (global->ops_cstate.opc_topahd == NULL)
        global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
        global->ops_cstate.opc_firstahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_SCHEMA);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_CREATE_SCHEMA;
    ahd->ahd_valid = NULL;

    /* Now let's go on to the CREATE SCHEMA stuff. We need to copy over the
    ** the PST_CREATE_SCHEMA structure to QEF_CREATE_SCHEMA structure 
    */

    STRUCT_ASSIGN_MACRO(opc_schema->pst_schname, 
			ahd->qhd_obj.qhd_schema.qef_schname);
    STRUCT_ASSIGN_MACRO(opc_schema->pst_owner, 
			ahd->qhd_obj.qhd_schema.qef_schowner);	

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt++;

} /* opc_shemaahd_build */ 

/*}
** Name: opc_eximmahd_build    - Build an action header for an EXECUTE 
**                                      IMMEDIATE statement.
**
** Description:
**      This routine coordinates the building of an action header for an
**      EXECUTE IMMEDIATE statement. A pointer to the qrytext and PST_INFO 
**      structure are copied to the QEA_EXEC_IMM structure. The mapping of the 
**      statement to action is as follows:
**
**              PSF:                       QEF:
**      Type:   PST_EXEC_IMM_TYPE          QEA_EXEC_IMM
**
**      Struct: PST_EXEC_IMM               QEF_EXEC_IMM
**
**      pointer to qtext		   pointer to qtext	 
**      PST_INFO struct			   PST_INFO struct
**
** Inputs:
**      global     - the global control block for compilation
**
**      new_ahd    - pointer to a pointer in which to place the address of
**                   the new AHD.
**
** Outputs:
**      new_ahd:
**          pointer to newly created AHD in the address specified by new_ahd.
**
** Side Efects:
**          none
**
** History:
**      23-nov-92 (anitap)
**          Created.
**	20-jan-93 (anitap)
**	    Fixed compiler warning. Fixed bug.
**	02-feb-93 (anitap)
**	    Fixed size in MEcopy().
**	04-mar-93 (anitap)
**	    Changed VOID to void.
[@history_template@]...
*/
void
opc_eximmahd_build(
                  OPS_STATE     *global,
                  QEF_AHD       **new_ahd)
{
    QEF_AHD		*ahd;
    DB_TEXT_STRING	*qtext;
    PST_INFO		*opc_info;
    QEF_EXEC_IMM	*opc_exeimm;

    opc_info = &global->ops_statement->pst_specific.pst_execImm.pst_info;
    qtext = global->ops_statement->pst_specific.pst_execImm.pst_qrytext;

    /* First, lets allocate the action header */
    ahd = *new_ahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_EXEC_IMM));

    /* If this is the first AHD or the top AHD, set the proper status.  */
    /* 'first ahd' is the first AHD for this QP.                         */
    /* 'top ahd' is the top(first)AHD for this QEP. */
    if (global->ops_cstate.opc_topahd == NULL)
       global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)
        global->ops_cstate.opc_firstahd = ahd;

    /* Now lets fill in the boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof (QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_EXEC_IMM);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_atype = QEA_EXEC_IMM;
    ahd->ahd_valid = NULL;

    /* Now let's go on to the EXECUTE IMMEDIATE stuff. We need to copy over the
    ** PST_INFO and qtext.
    */
    /* FIXME: When PST_QNODE is included in PST_EXEC_IMM, a CX will
    ** need to be compiled. 
    */	 	

    opc_exeimm = &ahd->qhd_obj.qhd_execImm;
    opc_exeimm->ahd_text = (DB_TEXT_STRING *) opu_qsfmem(global, 
			sizeof(DB_TEXT_STRING) + qtext->db_t_count);
   
    /* copy the contents of source DB_TEXT_STRING into target DB_TEXT_STRING */
    opc_exeimm->ahd_text->db_t_count = qtext->db_t_count;

    MEcopy((PTR)qtext->db_t_text, 
		 opc_exeimm->ahd_text->db_t_count,
		 (PTR)opc_exeimm->ahd_text->db_t_text);

    opc_copy_info(global, opc_info, &opc_exeimm->ahd_info); 

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt++;

} /* opc_eximmahd_build */

/*{
** Name: opc_copy_info - allocate and copy PST_INFO and qtext
**
** Description:
**	This routine copies the PST_INFO structure of PST_EXEC_IMM structure 
**      from the parser to the PST_INFO structure of QEF_EXEC_IMM structure in
**      the compiler. This involves allocating PST_INFO struct and filling it 
**	in.
**
** Inputs:
**	global			Global control block for compilation
**      pst_info		pointer to source PST_INFO struct 
**	target_info		pointer to target PST_INFO struct
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          None
**
** History:
**      23-nov-92 (anitap)
**          Written for CREATE SCHEMA project(release 6.5)
**	02-feb-93 (anitap)
**	    Removed DB_TEXT_STRING from declaration of the function.
**	04-mar-93 (anitap)
**	    Changed VOID to void.
**	07-jul-93 (anitap)
**	    Account for the case when PST_INFO is NULL.
*/
static void 
opc_copy_info(
	OPS_STATE		*global,
	PST_INFO		*pst_info,
	PST_INFO		**target_info)
{
    PST_INFO		*opc_info;

    opc_info = (PST_INFO *) opu_qsfmem(global, sizeof(PST_INFO));	
    *target_info = opc_info;
    MEcopy((PTR)pst_info, sizeof (PST_INFO), (PTR)opc_info);

    if (pst_info == NULL)
       opc_info = NULL;
    else
    {
       /* copy the contents of source PST_INFO to target PST_INFO */
       opc_info->pst_baseline = pst_info->pst_baseline;	    
       opc_info->pst_basemode = pst_info->pst_basemode;
       opc_info->pst_execflags = pst_info->pst_execflags;    
    }
}

/*{
** Name: opc_createTableAHD - Build action header for an CREATE TABLE statement.
**
** Description:
**	This routine coordinates the building of an action header for an
**	CREATE TABLE statement.
**
**		PSF:				QEF:
**
**	Type: 	PST_CREATE_TABLE_TYPE		QEA_CREATE_TABLE
**
**	Struct:	PST_CREATE_TABLE		QEF_DDL_STATEMENT
**
** Inputs:
**	global	    		Global control block for compilation.
**	createTableQEUCB	QEU_CB describing the create table details.
**	IDrow			DSH row in which to stuff table id.
**
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	 None - statement memory allocation is done upon return to caller.
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

VOID
opc_createTableAHD(
		OPS_STATE	*global,
		QEU_CB		*createTableQEUCB,
		i4		IDrow,
		QEF_AHD		**new_ahd)
{
    QEF_AHD		*ahd;

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DDL_STATEMENT));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DDL_STATEMENT);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_CREATE_TABLE;

    /* other objects we might we interested in */

    ahd->ahd_ID = IDrow;	/* DSH row to stick our table id in */
    ahd->ahd_relatedObjects = ( QEF_RELATED_OBJECT * ) NULL;

    /* copy the qeu_cb out of the parser's QSF object into the compiler's */

    allocate_and_copy_QEUCB( global, createTableQEUCB,
	( QEU_CB ** ) &ahd->qhd_obj.qhd_DDLstatement.ahd_qeuCB );

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

} /* opc_createTableAHD */

/*{
** Name: opc_renameTableAHD - Build action header for an RENAME TABLE statement.
**
** Description:
**	This routine coordinates the building of an action header for an
**	RENAME TABLE statement.
**
** Inputs:
**	global	    		Global control block for compilation.
**	IDrow			DSH row in which to stuff table id.
**
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	 None - statement memory allocation is done upon return to caller.
**
** History:
**	18-mar-2010 (gupsh01) SIR 123444
**	    Written for ALTER TABLE ..RENAME support.
*/
VOID
opc_renameAHD(
		OPS_STATE	*global,
		PST_STATEMENT	*pst_statement,
		i4		IDrow,
		QEF_AHD		**new_ahd)
{
    QEF_AHD		*ahd;
    PST_RENAME		*psfDetails = &pst_statement->pst_specific.pst_rename;
    QEU_CB		*renameQEUCB;
    QEF_RENAME_STATEMENT *qefDetails;

    renameQEUCB = ( QEU_CB * ) global->ops_statement->pst_specific.pst_rename.pst_rnm_QEUCB;

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_RENAME_STATEMENT));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_RENAME_STATEMENT);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_RENAME_STATEMENT;   /* Lets set specific flags for rename */

    /* other objects we might we interested in */
    ahd->ahd_ID = IDrow;	/* DSH row to stick our table id in */
    ahd->ahd_relatedObjects = ( QEF_RELATED_OBJECT * ) NULL;

    qefDetails = &ahd->qhd_obj.qhd_rename;

    /* Copy the various fields now */
    STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_qeuqCB, qefDetails->qrnm_ahd_qeuqCB);
    STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_tab_id, qefDetails->qrnm_tab_id);
    STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_owner, qefDetails->qrnm_owner);
    STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_old_tabname, qefDetails->qrnm_old_tabname);

    /* copy the qeu_cb out of the parser's QSF object into the compiler's */
    allocate_and_copy_QEUCB( global, renameQEUCB,
	( QEU_CB ** ) &ahd->qhd_obj.qhd_rename.qrnm_ahd_qeuCB );

    qefDetails->qrnm_type = psfDetails->pst_rnm_type;
    qefDetails->qrnm_state = QEF_RENAME_INIT;

    if (qefDetails->qrnm_type == PST_ATBL_RENAME_COLUMN)
    {
      STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_col_id, qefDetails->qrnm_col_id);
      STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_old_colname, qefDetails->qrnm_old_colname);
      STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_new_colname, qefDetails->qrnm_new_colname);
    }
    else
      STRUCT_ASSIGN_MACRO( psfDetails->pst_rnm_new_tabname, qefDetails->qrnm_new_tabname);

    opc_ptrow(global, &qefDetails->qrnm_ulm_rcb, sizeof( ULM_RCB ) );
    opc_ptrow(global, &qefDetails->qrnm_state, sizeof(i4) );
    opc_ptrow(global, &qefDetails->qrnm_pstInfo, sizeof( PST_INFO ) );

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

}
 /* opc_renameAHD*/

/*{
** Name: opc_createIntegrityAHD - Build action for CREATE INTEGRITY statement.
**
** Description:
**	This routine coordinates the building of an action header for a
**	CREATE INTEGRITY statement.
**
**		PSF:				QEF:
**
**	Type: 	PST_CREATE_INTEGRITY_TYPE	QEA_CREATE_INTEGRITY
**
**	Struct:	PST_CREATE_INTEGRITY		QEF_CREATE_INTEGRITY_STATEMENT
**
** Inputs:
**	global	    		Global control block for compilation.
**	pst_statement		What PSF saw.
**	IDrow			DSH row in which to stuff constraint id.
**
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** Side Effects:
**	 None - statement memory allocation is done upon return to caller.
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	30-jan-93 (rickh)
**	    Added to the DSH more objects that must persist over execute
**	    immediates.
**	2-apr-93 (rickh)
**	    Add integrity number to DSH of CREATE INTEGRITY statements.
**	9-aug-93 (rickh)
**	    Added a new field to the CREATE_INTEGRITY
**	    action:  initialState of the DDL state machines.
**	17-jan-94 (rickh)
**	    Fixed some casts.  Also added a field to the CREATE INTEGRITY
**	    header:  modifyRefingProc was changed to insertRefingProc and
**	    updateRefingProc.
*/

VOID
opc_createIntegrityAHD(
		OPS_STATE	*global,
		PST_STATEMENT	*pst_statement,
		i4		IDrow,
		QEF_AHD		**new_ahd )
{
    QEF_AHD			*ahd;
    PST_CREATE_INTEGRITY	*psfDetails = &pst_statement->pst_specific.
				pst_createIntegrity;
    i4				flags = psfDetails->pst_createIntegrityFlags;
    QEF_CREATE_INTEGRITY_STATEMENT	*qefDetails;
				

    *new_ahd = ( QEF_AHD * ) NULL;

    /* Currently, we only compile Constraints */

    if ( ( flags & PST_CONSTRAINT ) == 0 )
    {
	opx_error(E_OP08A3_UNKNOWN_STATEMENT);
	return;
    }

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
	    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_INTEGRITY_STATEMENT));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
	    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_INTEGRITY_STATEMENT);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_CREATE_INTEGRITY;

    /* other objects we might be interested in */

    ahd->ahd_ID = IDrow;	/* DSH row to stick our constraint id in */
    ahd->ahd_relatedObjects = ( QEF_RELATED_OBJECT * ) NULL;

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    /* now copy the PSF integrity structure to the QEF one.  QEF will
    ** do the real work.
    */

    qefDetails = &ahd->qhd_obj.qhd_createIntegrity;
    copyIntegrityDetails( global, psfDetails, qefDetails );

    /* compile space for structures which must persist over subactions
    ** that are spawned as "execute immediates" by QEF */

    opc_ptrow(global, &qefDetails->qci_internalConstraintName,
		sizeof( DB_CONSTRAINT_NAME ) );
    opc_ptrow(global, &qefDetails->qci_constraintID,
		sizeof( DB_CONSTRAINT_ID ) );
    opc_ptrow(global, &qefDetails->qci_integrityNumber,
		sizeof( i4  ) );
    opc_ptrow(global, &qefDetails->qci_ulm_rcb, sizeof( ULM_RCB) );
    opc_ptrow(global, &qefDetails->qci_constrainedTableID,
		sizeof( DB_TAB_ID ) );
    opc_ptrow(global, &qefDetails->qci_referredTableID,
		sizeof( DB_TAB_ID ) );
    opc_ptrow(global, &qefDetails->qci_cnstrnedTabAttributeArray,
		sizeof( DMT_ATT_ENTRY ** ) );
    opc_ptrow(global, &qefDetails->qci_referredTableAttributeArray,
		sizeof( DMT_ATT_ENTRY ** ) );
    opc_ptrow(global, &qefDetails->qci_nameOfIndex, sizeof( DB_TAB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfInsertRefingProc,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfUpdateRefingProc,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfDeleteRefedProc,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfUpdateRefedProc,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfInsertRefingRule,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfUpdateRefingRule,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfDeleteRefedRule,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_nameOfUpdateRefedRule,
		sizeof( DB_NAME ) );
    opc_ptrow(global, &qefDetails->qci_pstInfo, sizeof( PST_INFO ) );
    opc_ptrow(global, &qefDetails->qci_state, sizeof( i4  ) );
    opc_ptrow(global, &qefDetails->qci_initialState, sizeof( i4  ) );

} /* opc_createIntegrityAHD */

/*{
** Name: copyIntegrityDetails - copy psf summary of integrity to qef summary
**
** Description:
**	Mostly, OPC is just a pass through for creating integrities.  QEF
**	does the real work.  This routine copies the parser's summary of
**	the integrity into a structure that QEF can understand.
**
**
**
** Inputs:
**	global	    		Global control block for compilation.
**	psfDetails		What PSF saw.
**	qefDetails		What QEF will see.
**
**
** Outputs:
**	qefDetails		Filled in.
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
**
** History:
**	02-dec-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	20-jan-93 (anitap)
**	    Fixed compiler warning.
**	30-jan-93 (rickh)
**	    Added check rule text.
**	01-mar-93 (rickh)
**	    Copy the schema name out of PSF's CREATE_INTEGRITY structure into
**	    QEF's.
**	26-mar-93 (andre)
**	    PST_CREATE_INTEGRITY structure describing a REFERENCES constraint
**	    will contain a ptr (pst_indep) to a PSQ_INDEP_OBJECTS structure
**	    describing a UNIQUE or PRIMARY KEY constraint and, if the owner of
**	    the <referenced table> is different from that of the <referencing 
**	    table>, description of a REFERENCES privilege on column(s) of the
**	    <referenced table> on which the new constraint will depend.  If
**	    present, this structure will be copied into OPC memory and
**	    QEF_CREATE_INTEGRITY_STATEMENT.qci_indep will be made to point at
**	    it.
**	9-aug-93 (rickh)
**	    CREATE_INTEGRITY actions now need to compile in a flag indicating
**	    whether a REFERENTIAL CONSTRAINT should be verified at the time
**	    it's added.
**	26-aug-93 (rickh)
**	    When compiling a CREATE TABLE AS SELECT statement, flag any NOT
**	    NULL actions to remember the rowcount of the SELECT action.
**	13-sep-93 (rblumer)
**	    Changed name PST_ALTER_TABLE bit to more appropriate
**	    PST_VERIFY_CONS_HOLDS, per rickh's suggestion.
**	18-apr-94 (rblumer)
**	    changed pst_integrityTuple2 to pst_knownNotNullableTuple, and
**	    changed qci_integrityTuple2 to qci_knownNotNullableTuple.
**	1-apr-98 (inkdo01)
**	    Numerous changes to add information for index with clause.
**	26-may-98 (inkdo01)
**	    Added flags for delete/update cascade/set null referential actions.
**	20-oct-98 (inkdo01)
**	    Added flags for delete/update restrict referential actions.
**	3-may-2007 (dougi)
**	    Added flag to force base table structure to btree on constrained
**	    columns, instead of building secondary index.
*/

#define	COPY_STRUCT( a, b )	\
	( STRUCT_ASSIGN_MACRO( psfDetails->a, qefDetails->b ) )

static VOID
copyIntegrityDetails(
		OPS_STATE			*global,
		PST_CREATE_INTEGRITY		*psfDetails,
		QEF_CREATE_INTEGRITY_STATEMENT	*qefDetails )
{
    i4		queryTextLength;
    OPC_STATE	*compilerState = &global->ops_cstate;

    if ( psfDetails->pst_createIntegrityFlags & PST_CONSTRAINT )
    {	qefDetails->qci_flags |= QCI_CONSTRAINT; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_NAME_SPEC )
    {	qefDetails->qci_flags |= QCI_CONS_NAME_SPEC; }
    if ( psfDetails->pst_createIntegrityFlags & PST_SUPPORT_SUPPLIED )
    {
	qefDetails->qci_flags |= QCI_SUPPORT_SUPPLIED;
	COPY_STRUCT( pst_suppliedSupport, qci_suppliedSupport );
    }
    if ( compilerState->opc_flags & OPC_SAVE_ROWCOUNT )
    {	qefDetails->qci_flags |= QCI_SAVE_ROWCOUNT; }
    if ( psfDetails->pst_createIntegrityFlags & PST_VERIFY_CONS_HOLDS )
    {	qefDetails->qci_flags |= QCI_VERIFY_CONSTRAINT_HOLDS; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_WITH_OPT )
    {	qefDetails->qci_flags |= QCI_INDEX_WITH_OPTS; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_NEWIX )
    {	qefDetails->qci_flags |= QCI_NEW_NAMED_INDEX; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_OLDIX )
    {	qefDetails->qci_flags |= QCI_OLD_NAMED_INDEX; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_BASEIX )
    {	qefDetails->qci_flags |= QCI_BASE_INDEX; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_NOIX )
    {	qefDetails->qci_flags |= QCI_NO_INDEX; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_SHAREDIX )
    {	qefDetails->qci_flags |= QCI_SHARED_INDEX; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_DEL_CASCADE )
    {	qefDetails->qci_flags |= QCI_DEL_CASCADE; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_UPD_CASCADE )
    {	qefDetails->qci_flags |= QCI_UPD_CASCADE; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_DEL_SETNULL )
    {	qefDetails->qci_flags |= QCI_DEL_SETNULL; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_UPD_SETNULL )
    {	qefDetails->qci_flags |= QCI_UPD_SETNULL; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_DEL_RESTRICT )
    {	qefDetails->qci_flags |= QCI_DEL_RESTRICT; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_UPD_RESTRICT )
    {	qefDetails->qci_flags |= QCI_UPD_RESTRICT; }
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_TABLE_STRUCT )
    {	qefDetails->qci_flags |= QCI_TABLE_STRUCT; }

    if ( psfDetails->pst_integrityTuple != ( DB_INTEGRITY * ) NULL )
	allocateAndCopyStruct(	global,
				( PTR ) psfDetails->pst_integrityTuple,
				( PTR* ) &qefDetails->qci_integrityTuple,
				sizeof( DB_INTEGRITY ) );
    if ( psfDetails->pst_knownNotNullableTuple != ( DB_INTEGRITY * ) NULL )
	allocateAndCopyStruct(	global,
				( PTR ) psfDetails->pst_knownNotNullableTuple,
				( PTR* ) &qefDetails->qci_knownNotNullableTuple,
				sizeof( DB_INTEGRITY ) );

    COPY_STRUCT( pst_cons_tabname, qci_cons_tabname );
    COPY_STRUCT( pst_cons_ownname, qci_cons_ownname );
    allocateAndCopyColumnList(	global, psfDetails->pst_cons_collist,
		    	( PST_COL_ID ** ) &qefDetails->qci_cons_collist );
    COPY_STRUCT( pst_ref_tabname, qci_ref_tabname );
    COPY_STRUCT( pst_ref_ownname, qci_ref_ownname );
    allocateAndCopyColumnList(	global, psfDetails->pst_ref_collist,
		    	( PST_COL_ID ** ) &qefDetails->qci_ref_collist );
    COPY_STRUCT( pst_ref_depend_consid, qci_ref_depend_consid );
    if ( psfDetails->pst_createIntegrityFlags & PST_CONS_NEWIX )
	allocateAndCopyColumnList(	global, psfDetails->pst_key_collist,
		    	( PST_COL_ID ** ) &qefDetails->qci_key_collist );
    else qefDetails->qci_key_collist = NULL;

    queryTextLength = psfDetails->pst_integrityQueryText->db_t_count
			+ sizeof( DB_TEXT_STRING );
    allocateAndCopyStruct( global, ( PTR) psfDetails->pst_integrityQueryText,
	( PTR * ) &qefDetails->qci_integrityQueryText, queryTextLength );

    /*
    ** for CHECK CONSTRAINTS, we must copy the text used to cook up
    ** the WHERE clause in the supporting rule.
    */

    if ( psfDetails->pst_checkRuleText != ( DB_TEXT_STRING * ) NULL )
    {
        queryTextLength = psfDetails->pst_checkRuleText->db_t_count
			+ sizeof( DB_TEXT_STRING );
        allocateAndCopyStruct( global,
	    ( PTR) psfDetails->pst_checkRuleText,
	    ( PTR * ) &qefDetails->qci_checkRuleText, queryTextLength );
    }
    else
    {
	qefDetails->qci_checkRuleText = ( DB_TEXT_STRING * ) NULL;
    }

    /*
    ** some day, we may want to copy the query tree from the parsed
    ** statement to the action header.  since we don't store query trees
    ** for CONSTRAINTS, we don't have to worry about this now!
    */

    qefDetails->qci_queryTree = (PTR) NULL;

    COPY_STRUCT( pst_integritySchemaName, qci_integritySchemaName );

    /*
    ** if PST_CREATE_INTEGRITY statement represented a REFERENCES constraint, we
    ** need to copy description of a constraint and, if the owner of the
    ** <referenced table> is different from that of the <referencing table>, of
    ** privilege on which the new REFERENCES constraint will depend
    */
    if (   psfDetails->pst_integrityTuple != ( DB_INTEGRITY * ) NULL
	&& psfDetails->pst_integrityTuple->dbi_consflags & CONS_REF)
    {
	PSQ_INDEP_OBJECTS	*psf_indep, *qef_indep;

	/* there will always be exactly one independent object descriptor */
	allocateAndCopyStruct(global, (PTR) psfDetails->pst_indep,
	    (PTR *) &qefDetails->qci_indep, sizeof(PSQ_INDEP_OBJECTS));

	psf_indep = (PSQ_INDEP_OBJECTS *) psfDetails->pst_indep;
	qef_indep = (PSQ_INDEP_OBJECTS *) qefDetails->qci_indep;

	allocateAndCopyStruct(global, (PTR) psf_indep->psq_objs,
	    (PTR *) &qef_indep->psq_objs, sizeof(PSQ_OBJ));

	/*
	** if the independent privilege descriptor was suppled, copy it; for the
	** time being there can be exactly one of those
	*/
	if (psf_indep->psq_colprivs)
	{
	    allocateAndCopyStruct(global, (PTR) psf_indep->psq_colprivs,
		(PTR *) &qef_indep->psq_colprivs, sizeof(PSQ_COLPRIV));
	}
    }

    /* 
    ** Copy the various bits and pieces of index with options (if any).
    */

    if (qefDetails->qci_flags & QCI_INDEX_WITH_OPTS)
    {
	COPY_STRUCT( pst_indexopts.pst_resname, qci_idxname );
	allocate_and_copy_a_dm_data( global, 
		&psfDetails->pst_indexopts.pst_resloc,
		&qefDetails->qci_idxloc );
	qefDetails->qci_idx_fillfac = psfDetails->pst_indexopts.pst_fillfac;
	qefDetails->qci_idx_leaff   = psfDetails->pst_indexopts.pst_leaff;
	qefDetails->qci_idx_nonleaff= psfDetails->pst_indexopts.pst_nonleaff;
	qefDetails->qci_idx_page_size= psfDetails->pst_indexopts.pst_page_size;
	qefDetails->qci_idx_minpgs  = psfDetails->pst_indexopts.pst_minpgs;
	qefDetails->qci_idx_maxpgs  = psfDetails->pst_indexopts.pst_maxpgs;
	qefDetails->qci_idx_alloc   = psfDetails->pst_indexopts.pst_alloc;
	qefDetails->qci_idx_extend  = psfDetails->pst_indexopts.pst_extend;
	qefDetails->qci_idx_struct  = psfDetails->pst_indexopts.pst_struct;
    }

    return;
} /* copyIntegrityDetails */

/*{
** Name: allocateAndCopyStruct - allocate and copy a structure
**
** Description:
**	This routine allocates space for a structure and fills it in.
**	This will only work for structures which don't have any pointers
**	in them (e.g., tuples and character strings).
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			structure to copy from
**	target			structure to copy to
**	size			size of the structures
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	02-dec-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/

static VOID
allocateAndCopyStruct(
		OPS_STATE	*global,
		PTR		source,
		PTR		*target,
		i4		size )
{
    if ( source == ( PTR ) NULL )
    {
	*target = ( PTR ) NULL;
    }
    else
    {
	*target = opu_qsfmem( global, size );
	MEcopy( source, ( u_i2 ) size, *target );
    }
}

/*{
** Name: allocateAndCopyColumnList - allocate and copy a list of columns
**
** Description:
**	This routine copies a column list into a chunk of memory which
**	it allocates.
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			column list to copy from
**	target			column list to allocate and copy to
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	02-dec-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	20-jan-93 (anitap)
**	    Fixed compiler warning.
**
*/

static VOID
allocateAndCopyColumnList(
		OPS_STATE	*global,
		PST_COL_ID	*source,
		PST_COL_ID	**target )
{
    PST_COL_ID	*sourceColumn;
    PST_COL_ID	*targetColumn;
    PST_COL_ID	**targetPTR;


    if ( source == ( PST_COL_ID * ) NULL )
    {
	*target = ( PST_COL_ID * ) NULL;
    }
    else
    {
	for ( sourceColumn = source, targetPTR = target;
	      sourceColumn != ( PST_COL_ID * ) NULL;
	      sourceColumn = sourceColumn->next
	    )
	{
	    *targetPTR =
		targetColumn = (PST_COL_ID *) opu_qsfmem( global, sizeof( PST_COL_ID ) );

	    MEcopy( ( PTR ) &sourceColumn->col_id, sizeof( DB_ATT_ID ),
		    ( PTR ) &targetColumn->col_id );
	    targetColumn->next = ( PST_COL_ID * ) NULL;

	    targetPTR = &targetColumn->next;
	}	/* end for */

    }	/* endif */
}

/*{
** Name: allocate_and_copy_QEUCB - allocate and copy a QEU_CB
**
** Description:
**	This routine copies a QEU_CB from one QSF object to another.  This
**	involves allocating a QEU_CB of the same size in the target object
**	and filling it in.   There should be a subordinate DMU_CB, and
**	possibly attached partition definition info;  these are also copied.
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			QEU_CB to copy from
**	target			QEU_CB to copy to
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	10-Feb-2004 (schka24)
**	    Copy the physical partition characteristics list.
**	20-Feb-2004 (schka24)
**	    Some clown got the name wrong, s/physical/logical/
**
*/

static VOID
allocate_and_copy_QEUCB(
		OPS_STATE	*global,
		QEU_CB		*source,
		QEU_CB		**target )
{
    QEU_CB	*qeu_cb;
    QEU_LOGPART_CHAR *qeu_lpl_ptr;	/* Copied entry */
    QEU_LOGPART_CHAR *psf_lpl_ptr;	/* Original psf-generated entry */

    qeu_cb = ( QEU_CB * ) opu_qsfmem( global, sizeof( QEU_CB ) );
    *target = qeu_cb;
    MEcopy( ( PTR ) source, ( u_i2 ) sizeof( QEU_CB ), ( PTR ) qeu_cb );

    qeu_cb->qeu_owner	    = (PTR)DB_OPF_ID;

    /* now for the pointers */

    /* we assume that the following pointer type fields aren't interesting */

    qeu_cb->qef_next = ( QEU_CB * ) NULL;
    qeu_cb->qeu_prev = ( QEU_CB * ) NULL;
    qeu_cb->qeu_acc_id = ( PTR ) NULL;
    qeu_cb->qeu_input = ( QEF_DATA * ) NULL;
    qeu_cb->qeu_param = ( QEF_PARAM * ) NULL;
    qeu_cb->qeu_output = ( QEF_DATA * ) NULL;
    qeu_cb->qeu_key = ( DMR_ATTR_ENTRY ** ) NULL;
    qeu_cb->qeu_qual = NULL;
    qeu_cb->qeu_qarg = ( QEU_QUAL_PARAMS * ) NULL;
    qeu_cb->qeu_mem = ( ULM_RCB * ) NULL;
    qeu_cb->qeu_f_qual = ( PTR ) NULL;
    qeu_cb->qeu_f_qarg = ( PTR ) NULL;
    qeu_cb->qeu_qso = ( PTR ) NULL;	/* FIXME */

    /*
    ** we also assume that the database id and session id point to stable
    ** ground in SCF's space, not in the quicksand of QSF.  that is, these
    ** two fields:  qeu_db_id and qeu_d_id.
    */

    /* copy the DMUCB out of the parser's QEU_CB into ours */

    allocate_and_copy_DMUCB( global, ( DMU_CB * ) source->qeu_d_cb,
			     ( DMU_CB ** ) &qeu_cb->qeu_d_cb );

    /* If there is a list of physical structuring info blocks, copy them
    ** to the target plan so that QEF can follow the instructions.
    */
    qeu_cb->qeu_logpart_list = NULL;
    psf_lpl_ptr = source->qeu_logpart_list;
    /* This loop happens to reverse the order of the list but it
    ** doesn't make any difference to anything.
    */
    while (psf_lpl_ptr != NULL)
    {
	qeu_lpl_ptr = (QEU_LOGPART_CHAR *) opu_qsfmem(global,
			sizeof(QEU_LOGPART_CHAR));
	MEcopy(psf_lpl_ptr, sizeof(QEU_LOGPART_CHAR), qeu_lpl_ptr);
	allocate_and_copy_a_dm_data(global,
			&psf_lpl_ptr->loc_array, &qeu_lpl_ptr->loc_array);
	allocate_and_copy_a_dm_data(global,
			&psf_lpl_ptr->char_array, &qeu_lpl_ptr->char_array);
	allocate_and_copy_a_dm_ptr(global, 0,
			&psf_lpl_ptr->key_array, &qeu_lpl_ptr->key_array);
	qeu_lpl_ptr->next = qeu_cb->qeu_logpart_list;
	qeu_cb->qeu_logpart_list = qeu_lpl_ptr;
	psf_lpl_ptr = psf_lpl_ptr->next;
    }


}


/*{
** Name: opc_supress_qryTxt
**
** Description:
**       Look for a piece of text in the query buffer(s) and
**       replace it, char by char with blanks. This is but a
**       simple utility routine to remove "with check option"
**       from the query text whenever its presence has been
**       flagged as an error.
**
**       Putting in a routine keep the main code tidier.
**       It is not intended for general use, and is static.  
**
** Inputs:
**       tuplesPtr : A linked list of tuple buffers, wherein
**                   the target text lies.
**       targetTxt : A pointer to the text that will be blanked.
**
** History:
**       13-Jun-08 (coomi01) Bug 110708
**           Created.
**}
*/
static void
opc_supress_qryTxt(QEF_DATA *tuplesPtr, PTR targetTxt)
{
    DB_IIQRYTEXT *queryTxtPtr;
    char aBlank = ' ';
    i4   partialMatch;
    PTR    nxtBuffPtr;
    PTR       buffPtr;
    i4        buffLen;
    i4         txtLen;
    i4        currPos;
    i4     maybeFound;

    txtLen = strlen(targetTxt);

    while ( tuplesPtr )
    {
	queryTxtPtr = (DB_IIQRYTEXT *)(tuplesPtr->dt_data);

	/* Two convenience vars */
	buffPtr = queryTxtPtr->dbq_text.db_t_text;
	buffLen = queryTxtPtr->dbq_text.db_t_count;

	/* Start at begining of buffer */
	currPos = 0;

	/* And loop whilst it has further content */
	while ( currPos < buffLen )
	{
	    if ( 0 == CMcmpnocase(targetTxt, &buffPtr[currPos]) )
	    {
		/* 
		** We may not compare beyound end of buffer
		*/
		maybeFound = min((buffLen-currPos), txtLen);
		
		partialMatch = STncasecmp(&buffPtr[currPos],
					  targetTxt, 
					  maybeFound);
				   
		/* Have we a starting point */
		if (0 == partialMatch)
		{
		    /*
		    ** Have we a potential split ? 
		    */
		    if ((maybeFound != txtLen) && (tuplesPtr->dt_next))
		    {
			/*
			** Yes, then look at (start of) next
			*/
			queryTxtPtr = (DB_IIQRYTEXT *)tuplesPtr->dt_next->dt_data;
			
			/*
			** Is there enough ?
			*/
			if ( (txtLen - maybeFound) <= queryTxtPtr->dbq_text.db_t_count)
			{
			    /* Set another convenience ptr */
			    nxtBuffPtr = queryTxtPtr->dbq_text.db_t_text;

			    /* Look at the rest */
			    partialMatch = STncasecmp(nxtBuffPtr,
						      &targetTxt[maybeFound],
						      txtLen - maybeFound);

			    /* 
			    ** Have we found the rest ? 
			    */
			    if (0 == partialMatch)
			    {
				/* 
				** Split Text Found 
				*/
				MEfill(maybeFound,
				       aBlank, 
				       &buffPtr[currPos]);

				MEfill(txtLen - maybeFound,
				       aBlank,
				       nxtBuffPtr);
				return;
			    }
			}
		    }
		    else
		    {
			/*
			** Single Text Found
			*/
			MEfill(txtLen,
			       aBlank,
			       &buffPtr[currPos]);
			return;
		    }
		}
	    }

	    /* 
	    ** Next position 
	    */
	    currPos++;
	}

	/* Next tuple to search */
	tuplesPtr = tuplesPtr->dt_next;
    }
}


/*{
** Name: opc_crt_view_ahd - Build action for CREATE/DEFINE VIEW statement.
**
** Description:
**	This routine coordinates the building of an action header for a
**	CREATE/DEFINE VIEW statement.
**
**		PSF:				QEF:
**
**	Type: 	PST_CREATE_VIEW_TYPE	    QEA_CREATE_VIEW
**
**	Struct:	PST_CREATE_VIEW		    QEF_CREATE_VIEW
**
** Inputs:
**	global	    		Global control block for compilation.
**	pst_statement		What PSF saw.
**	IDrow			DSH row in which to stuff view id.
**
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	
** Returns:
**	E_DB_{OK,ERROR}
**	
** Exceptions:
**	None
**
** Side Effects:
**	 None - statement memory allocation is done upon return to caller.
**
** History:
**	23-dec-92 (andre)
**	    Written for FIPS CHECK OPTION project (release 6.5).
**	28-feb-93 (andre)
**	    added more objects that must persist across EXECUTE IMMEDIATE 
**	    processing to DSH
**      13-Jun-08 (coomi01) Bug 110708
**          When not allowed, ensure "with check option" does not get
**          through to iiqrytxt.
**      26-Oct-2009 (coomi01) b122714
**          Use PST_TEXT_STORAGE to allow for long (>64k) view text.
*/
VOID
opc_crt_view_ahd(
		OPS_STATE	*global,
		PST_STATEMENT	*pst_statement,
		i4		IDrow,
		QEF_AHD		**new_ahd )
{
    QEF_AHD		*ahd;
    PST_CREATE_VIEW	*psf_crt_view = &pst_statement->pst_specific.
					    pst_create_view;
    i4			flags = psf_crt_view->pst_flags;
    QEF_CREATE_VIEW	*qef_crt_view;
    QEUQ_CB		*qeuqcb;
				
    *new_ahd = ( QEF_AHD * ) NULL;

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_VIEW));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_CREATE_VIEW);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_CREATE_VIEW;

    /* other objects we might be interested in */

    ahd->ahd_ID = IDrow;	/* DSH row in which to store view id */
    ahd->ahd_relatedObjects = ( QEF_RELATED_OBJECT * ) NULL;

    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    /*
    ** now copy the PSF CREATE_VIEW structure to the QEF one.
    ** QEF will do the real work.
    */

    qef_crt_view = &ahd->qhd_obj.qhd_create_view;

    /* name of the view being created */
    STRUCT_ASSIGN_MACRO(psf_crt_view->pst_view_name,
			qef_crt_view->ahd_view_name);

    /* translate information represented by bit masks */
    
    if (flags & PST_CHECK_OPT_DYNAMIC_INSRT)
    {
	qef_crt_view->ahd_crt_view_flags |= AHD_CHECK_OPT_DYNAMIC_INSRT;
    }
    
    if (flags & PST_CHECK_OPT_DYNAMIC_UPDT)
    {
	qef_crt_view->ahd_crt_view_flags |= AHD_CHECK_OPT_DYNAMIC_UPDT;

	/*
	** remember whether CHECK OPTION will have to be enforced dynamically
	** after every UPDATE or after UPDATE of certain columns
	*/
	if (flags & PST_VERIFY_UPDATE_OF_ALL_COLS)
	{
	    qef_crt_view->ahd_crt_view_flags |= AHD_VERIFY_UPDATE_OF_ALL_COLS;
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(psf_crt_view->pst_updt_cols,
				qef_crt_view->ahd_updt_cols);
	}
    }

    if (flags & PST_STAR_VIEW)
    {
	qef_crt_view->ahd_crt_view_flags |= AHD_STAR_VIEW;
    }

    opc_alloc_and_copy_QEUQ_CB(global, (QEUQ_CB *) psf_crt_view->pst_qeuqcb,
	(QEUQ_CB **) &qef_crt_view->ahd_crt_view_qeuqcb, QEA_CREATE_VIEW);

    qeuqcb = (QEUQ_CB *) qef_crt_view->ahd_crt_view_qeuqcb;

    /*
    ** now call RDF to pack query text and query tree into IIQRYTEXT and IITREE
    ** tuples
    */
    {
	RDF_CB			*rdf_cb = &global->ops_rangetab.opv_rdfcb;
	RDR_RB			*rdf_rb = &rdf_cb->rdf_rb;
	RDF_QT_PACK_RCB		pack_rcb;
	DB_STATUS		status = E_DB_OK;
	QEF_DATA		*qtext_tuples = (QEF_DATA *) NULL;
	QEF_DATA                *qtree_tuples = (QEF_DATA *) NULL;
	QEF_DATA		**data_pp;

	rdf_rb->rdr_l_querytext = psf_crt_view->pst_view_text_storage->pst_t_count;
	rdf_rb->rdr_querytext   = (PTR) psf_crt_view->pst_view_text_storage->pst_t_text;
	
	rdf_rb->rdr_status	= psf_crt_view->pst_status;
	
	rdf_rb->rdr_instr      |= RDF_USE_CALLER_ULM_RCB;
	
				    /* ptr to ULM_RCB for session */
	rdf_rb->rdf_fac_ulmrcb	= (PTR) &global->ops_mstate.ops_ulmrcb;
	
	/* have RDF allocate memory using global memory stream */
        global->ops_mstate.ops_ulmrcb.ulm_streamid_p =
	    &global->ops_mstate.ops_streamid;
	
	rdf_rb->rdr_qt_pack_rcb	    = &pack_rcb;
	pack_rcb.rdf_qt_tuples	    = &qtext_tuples;
	pack_rcb.rdf_qt_tuple_count = &qeuqcb->qeuq_cq;
	pack_rcb.rdf_qt_mode	    = DB_VIEW;

	status = rdf_call(RDF_PACK_QTEXT, rdf_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    /*
	    ** turn off RDF_USE_CALLER_ULM_RCB since other callers of RDF may
	    ** not appreciate it
	    */
	    rdf_rb->rdr_instr &= ~RDF_USE_CALLER_ULM_RCB;

	    opx_error(E_OP0012_TEXT_PACK);
	}

	/* most of the fields in rdf_rb have been set for the previous call */

	rdf_rb->rdr_qry_root_node   = (PTR) psf_crt_view->pst_view_tree;
	
	pack_rcb.rdf_qt_tuples	    = &qtree_tuples;
	pack_rcb.rdf_qt_tuple_count = &qeuqcb->qeuq_ct;
	
	rdf_rb->rdr_tabid.db_tab_base = rdf_rb->rdr_tabid.db_tab_index = 0;

	status = rdf_call(RDF_PACK_QTREE, rdf_cb);

	/*
	** turn off RDF_USE_CALLER_ULM_RCB since other callers of RDF may
	** not appreciate it
	*/
	rdf_rb->rdr_instr &= ~RDF_USE_CALLER_ULM_RCB;

	if (DB_FAILURE_MACRO(status))
	    opx_error(E_OP0011_TREE_PACK);

	/*
        ** copy query text and query tree tuples from the OPF's memory into the
	** query plan memory
	*/

	/* for no apparent reason, start off by copying query text */
	{
	    DB_IIQRYTEXT	*orig, *copy;
	    
	    /* 
	    ** Have we flaged an error in the query text ?
	    ** - Even if so, we now go onto store the text ???
	    **
	    ** Rather than allow the inconsistency to stand, which will
	    ** upset copydb if ever used, we shall remove the offending
	    ** text.
	    **
	    */
	    if (flags & PST_SUPPRESS_CHECK_OPT)
	    {
		/* 
		** Call utility routine above, rather than do it inline.
		*/
		opc_supress_qryTxt(qtext_tuples, "with check option");
	    }

	    for (
		 data_pp = &qeuqcb->qeuq_qry_tup;

		 qtext_tuples;
		 
		 qtext_tuples = qtext_tuples->dt_next,
		 data_pp = &(*data_pp)->dt_next
		)
	    {
		*data_pp = (QEF_DATA *) opu_qsfmem(global, sizeof(QEF_DATA));

		orig = (DB_IIQRYTEXT *) qtext_tuples->dt_data;
		copy =
		    (DB_IIQRYTEXT *) opu_qsfmem(global, sizeof(DB_IIQRYTEXT));

		/*
		** hoping to gain a tiny bit of performance, I am using
		** STRUCT_ASSIGN_MACRO instead of MEcopy()
		*/	    
		STRUCT_ASSIGN_MACRO((*orig), (*copy));

		(*data_pp)->dt_data = (PTR) copy;
		(*data_pp)->dt_size = qtext_tuples->dt_size;
	    }
	}

	/* now copy query tree */
	{
	    DB_IITREE	    *orig, *copy;
	    
	    for (
		 data_pp = &qeuqcb->qeuq_tre_tup;

		 qtree_tuples;
		 
		 qtree_tuples = qtree_tuples->dt_next,
		 data_pp = &(*data_pp)->dt_next
		)
	    {
		*data_pp = (QEF_DATA *) opu_qsfmem(global, sizeof(QEF_DATA));

		orig = (DB_IITREE *) qtree_tuples->dt_data;
		copy = (DB_IITREE *) opu_qsfmem(global, sizeof(DB_IITREE));

		/*
		** hoping to gain a tiny bit of performance, I am using
		** STRUCT_ASSIGN_MACRO instead of MEcopy()
		*/	    
		STRUCT_ASSIGN_MACRO((*orig), (*copy));

		(*data_pp)->dt_data = (PTR) copy;
		(*data_pp)->dt_size = qtree_tuples->dt_size;
	    }
	}
    }

    /* 
    ** now allocate space which will house data that must persist across 
    ** EXECUTE IMMEDIATE subactions
    */
    opc_ptrow(global, &qef_crt_view->ahd_ulm_rcb, sizeof(ULM_RCB));
    opc_ptrow(global, &qef_crt_view->ahd_state, sizeof(i4));
    opc_ptrow(global, &qef_crt_view->ahd_view_id, sizeof(DB_TAB_ID));
    opc_ptrow(global, &qef_crt_view->ahd_check_option_dbp_name, 
	sizeof(DB_DBP_NAME));
    opc_ptrow(global, &qef_crt_view->ahd_view_owner, sizeof(DB_OWN_NAME));
    opc_ptrow(global, &qef_crt_view->ahd_pst_info, sizeof(PST_INFO));
    opc_ptrow(global, &qef_crt_view->ahd_tbl_entry, sizeof(DMT_TBL_ENTRY));

    return;
} /* opc_crt_view_ahd */


/*{
** Name: opc_d_integ - Build action for ALTER TABLE DROP CONSTRAINT (and
**		       eventually DROP INTEGRITY) statement
**
** Description:
**	This routine coordinates the building of an action header for a
**	ALTER TABLE DROP CONSTRAINT statement.  Eventually, it will also handle
**	DROP INTEGRITY statement which for now will follow the traditional
**	PSF->RDF->QEU path.
**
**		PSF:				QEF:
**
**	Type: 	PST_DROP_INTEGRITY_TYPE	    QEA_DROP_INTEGRITY
**
**	Struct:	PST_DROP_INTEGRITY	    QEF_DROP_INTEGRITY
**
** Inputs:
**	global	    		Global control block for compilation.
**	pst_statement		What PSF saw.
**
** Outputs:
**	new_ahd	    		Address in which to place the new AHD.
**	
** Returns:
**	E_DB_{OK,ERROR}
**	
** Exceptions:
**	None
**
** Side Effects:
**	memory will be allocated
**
** History:
**	13-mar-93 (andre)
**	    Written for ALTER TABLE DROP CONSTRAINT
**	13-apr-93 (andre)
**	    psf_drop_integ was referenced before being initialized (which
**	    explains why psf_flags was not set correctly)
*/
VOID
opc_d_integ(
	    OPS_STATE       *global,
	    PST_STATEMENT   *pst_statement,
	    QEF_AHD         **new_ahd)
{
    QEF_AHD			    *ahd;
    PST_DROP_INTEGRITY		    *psf_drop_integ;
    i4				    psf_flags;
    QEF_DROP_INTEGRITY		    *qef_drop_integ;

    psf_drop_integ = &pst_statement->pst_specific.pst_dropIntegrity;
    psf_flags = psf_drop_integ->pst_flags;

    /* Allocate the action header */
    ahd = *new_ahd = (QEF_AHD *)opu_qsfmem(global, sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DROP_INTEGRITY));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_DROP_INTEGRITY);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_DROP_INTEGRITY;
    
    /* Mark in the qp header that there is another action */
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    /*
    ** now translate PST_DROP_INTEGRITY structure into a QEF_DROP_INTEGRITY
    ** structure; these two structures are similar but by no means identical:
    ** the main difference is that the PSF structure was designed to describe
    ** either DROP INTEGRITY or ALTER TABLE DROP CONSTRAINT, whereas QEF
    ** structure is capable of describing both at once (as would be the case
    ** when a table is being dropped)
    */

    qef_drop_integ = &ahd->qhd_obj.qhd_drop_integrity;

    /*
    ** coming from PSF we expect to get a request to drop EITHER "old style"
    ** integrity(ies) OR a constraint
    */
    if (psf_flags & PST_DROP_ANSI_CONSTRAINT)
    {
	struct PST_DROP_CONS_DESCR_	    *drop_cons;

	qef_drop_integ->ahd_flags |= AHD_DROP_ANSI_CONSTRAINT;

	if (psf_flags & PST_DROP_CONS_CASCADE)
	    qef_drop_integ->ahd_flags |= AHD_DROP_CONS_CASCADE;

	drop_cons = &psf_drop_integ->pst_descr.pst_drop_cons_descr;

	STRUCT_ASSIGN_MACRO(drop_cons->pst_cons_tab_id,
			    qef_drop_integ->ahd_integ_tab_id);
	STRUCT_ASSIGN_MACRO(drop_cons->pst_cons_tab_name,
			    qef_drop_integ->ahd_integ_tab_name);
	STRUCT_ASSIGN_MACRO(drop_cons->pst_cons_schema,
			    qef_drop_integ->ahd_integ_schema);
	STRUCT_ASSIGN_MACRO(drop_cons->pst_cons_name,
			    qef_drop_integ->ahd_cons_name);
	STRUCT_ASSIGN_MACRO(drop_cons->pst_cons_schema_id,
			    qef_drop_integ->ahd_cons_schema_id);
    }
    else
    {
	struct PST_DROP_INTEG_DESCR_    *drop_integ;

	qef_drop_integ->ahd_flags |= AHD_DROP_INGRES_INTEGRITY;
	
	if (psf_flags & PST_DROP_ALL_INTEGRITIES)
	    qef_drop_integ->ahd_flags |= AHD_DROP_ALL_INTEGRITIES;

	drop_integ = &psf_drop_integ->pst_descr.pst_drop_integ_descr;

	qef_drop_integ->ahd_integ_number = drop_integ->pst_integ_number;
	STRUCT_ASSIGN_MACRO(drop_integ->pst_integ_tab_id,
			    qef_drop_integ->ahd_integ_tab_id);
    }

    return;
}	/* opc_d_integ */

/*
** Name: opc_alloc_and_copy_QEUQ_CB - allocate memory for a QEUQ_CB and make a
**				      copy of the original passed by the caller
**
** Description:
**	This function will be responsible for copying a QEUQ_CB passed by the
**	caller into QSF memory; a copy of a DMU_CB whose address is contained
**	in QEUQ_CB.qeuq_dmf_cb will also be made here.
**
**	At the time this function is being written, it will only be called for
**	QEA_CREATE_VIEW action.  When/if it needs to be called for other
**	actions, defferent fields may need to be copied depending on "action".
**
** Input:
**	global                  Global control block for compilation.
**	source			address of the QEUQ_CB to copy
**	action			action for which this function is invoked
**
** Output:
**	*target			address of a copy of the original QEUQ_CB
**
** Returns:
**	none
**
** Side effects:
**	Memory will be allocated
**
** History:
**	30-dec-92 (andre)
**	    written
**	28-may-93 (andre)
**	    renamed alloc_and_copy_QEUQ_CB() to opc_alloc_and_copy_QEUQ_CB() to
**	    conform to coding standard
*/
static VOID
opc_alloc_and_copy_QEUQ_CB(
	OPS_STATE       *global,
	QEUQ_CB         *source,
	QEUQ_CB         **target,
	i4		action)
{
    QEUQ_CB	*qeuq_cb;
    i4		i;
    DB_TAB_ID	*src_id, *dest_id;
    i4		*src_type, *dest_type;

    *target = qeuq_cb = (QEUQ_CB *) opu_qsfmem( global, sizeof(QEUQ_CB));
    MEcopy((PTR) source, sizeof(QEUQ_CB), (PTR) qeuq_cb );

    /*
    ** we also assume that the database id (qeuq_db_id) and session id
    ** (qeuq_d_id) point to stable ground in SCF's space, not in the quicksand
    ** of QSF.
    */

    /*
    ** at the time of writing, this function expects to only be called for 
    ** QEA_CREATE_VIEW;
    ** if/when it is called when processing other actions, the following code
    ** may need to be altered (e.g. to copy fields that are not used for
    ** CREATE/DEFINE VIEW)
    */

    switch (action)
    {
	case QEA_CREATE_VIEW:
	{
	    /*
	    ** copy the list of ids of tables/views (if any) appearing in the
	    ** view's definition
	    */
	    if ((i = qeuq_cb->qeuq_tsz) > 0)
	    {
		qeuq_cb->qeuq_tbl_id =
		    (DB_TAB_ID *) opu_qsfmem(global, i * (sizeof(DB_TAB_ID) +
							sizeof(i4)));
		qeuq_cb->qeuq_tbl_type = (i4 *)&qeuq_cb->
					qeuq_tbl_id[qeuq_cb->qeuq_tsz];

		for (
		     src_id = source->qeuq_tbl_id,
		     dest_id = qeuq_cb->qeuq_tbl_id,
		     src_type = source->qeuq_tbl_type,
		     dest_type = qeuq_cb->qeuq_tbl_type;
		     
		     i > 0;
		     
		     src_id++, dest_id++, src_type++, dest_type++, i--
		    )
		{
		    dest_id->db_tab_base  = src_id->db_tab_base;
		    dest_id->db_tab_index = src_id->db_tab_index;
		    *dest_type = *src_type;
		}

		/*
		** if creating a STAR view, copy a list of types of underlying
		** objects, if any
		*/
		if (qeuq_cb->qeuq_obj_typ_p != (DD_OBJ_TYPE *) NULL)
		{
		    DD_OBJ_TYPE	    *src_type, *dest_type;
		    
		    i = qeuq_cb->qeuq_tsz;

		    qeuq_cb->qeuq_obj_typ_p =
			(DD_OBJ_TYPE *) opu_qsfmem(global,
			    i * sizeof(DD_OBJ_TYPE));

		    for (
			 src_type = source->qeuq_obj_typ_p,
			 dest_type = qeuq_cb->qeuq_obj_typ_p;
			 
			 i > 0;
			 
			 src_type++, dest_type++, i--
			)
		    {
			*dest_type = *src_type;
		    }
		}
	    }

	    /*
	    ** copy the independent object/privilege description
	    */
	    {
		PSQ_INDEP_OBJECTS       *indep_objs;
		PSQ_INDEP_OBJECTS       *src_indep_objs =
		    (PSQ_INDEP_OBJECTS *) source->qeuq_indep;

		indep_objs = (PSQ_INDEP_OBJECTS *) opu_qsfmem(global,
		    sizeof(PSQ_INDEP_OBJECTS));

		STRUCT_ASSIGN_MACRO((*src_indep_objs), (*indep_objs));

		indep_objs->psq_grantee = (DB_OWN_NAME *) opu_qsfmem(global,
		    sizeof(DB_OWN_NAME));

		STRUCT_ASSIGN_MACRO((*src_indep_objs->psq_grantee),
				    (*indep_objs->psq_grantee));

		qeuq_cb->qeuq_indep = (PTR) indep_objs;

		/* copy description of independent objects, if any */
		if (src_indep_objs->psq_objs != (PSQ_OBJ *) NULL)
		{
		    PSQ_OBJ		*orig_obj, **copy_obj;
		    i4			size;

		    for (
			 orig_obj = src_indep_objs->psq_objs,
			 copy_obj = &indep_objs->psq_objs;

			 orig_obj != (PSQ_OBJ *) NULL;

			 orig_obj = orig_obj->psq_next,
			 copy_obj = &(*copy_obj)->psq_next
			)
		    {
			/*
			** PSQ_OBJ involves a variable-size array of table ids
			** and a space for a dbproc name which is used only when
			** the structure describes a dbproc - which will never
			** be the case for views
			*/

			size =   sizeof(PSQ_OBJ)
			       + sizeof(DB_TAB_ID) * orig_obj->psq_num_ids
			       - sizeof(DB_TAB_ID) - sizeof(DB_DBP_NAME);
			
			*copy_obj = (PSQ_OBJ *) opu_qsfmem(global, size);

			(*copy_obj)->psq_objtype = orig_obj->psq_objtype;
			(*copy_obj)->psq_num_ids = orig_obj->psq_num_ids;
			
			for (
			     src_id = orig_obj->psq_objid,
			     dest_id = (*copy_obj)->psq_objid,
			     i = orig_obj->psq_num_ids;

			     i > 0;

			     src_id++, dest_id++, i--
			    )
			{
			    dest_id->db_tab_base  = src_id->db_tab_base;
			    dest_id->db_tab_index = src_id->db_tab_index;
			}
		    }
		}

		/*
		** copy descriptions of independent table-wide privileges,
		** if any
		*/
		if (src_indep_objs->psq_objprivs != (PSQ_OBJPRIV *) NULL)
		{
		    PSQ_OBJPRIV		*orig_objpriv, **copy_objpriv;

		    for (
			 orig_objpriv = src_indep_objs->psq_objprivs,
			 copy_objpriv = &indep_objs->psq_objprivs;

			 orig_objpriv != (PSQ_OBJPRIV *) NULL;

			 orig_objpriv = orig_objpriv->psq_next,
			 copy_objpriv = &(*copy_objpriv)->psq_next
			)
		    {
			*copy_objpriv = (PSQ_OBJPRIV *) opu_qsfmem(global,
			    sizeof(PSQ_OBJPRIV));

			STRUCT_ASSIGN_MACRO((*orig_objpriv), (**copy_objpriv));
		    }
		}

		/*
		** copy descriptions of independent column-specific privileges,
		** if any
		*/
		if (src_indep_objs->psq_colprivs != (PSQ_COLPRIV *) NULL)
		{
		    PSQ_COLPRIV		*orig_colpriv, **copy_colpriv;

		    for (
			 orig_colpriv = src_indep_objs->psq_colprivs,
			 copy_colpriv = &indep_objs->psq_colprivs;

			 orig_colpriv != (PSQ_COLPRIV *) NULL;

			 orig_colpriv = orig_colpriv->psq_next,
			 copy_colpriv = &(*copy_colpriv)->psq_next
			)
		    {
			*copy_colpriv = (PSQ_COLPRIV *) opu_qsfmem(global,
			    sizeof(PSQ_COLPRIV));

			STRUCT_ASSIGN_MACRO((*orig_colpriv), (**copy_colpriv));
		    }
		}
	    }

	    /* copy the DMUCB out of the parser's QEUQ_CB into ours */
	    
	    allocate_and_copy_DMUCB(global, (DMU_CB *) source->qeuq_dmf_cb,
		(DMU_CB **) &qeuq_cb->qeuq_dmf_cb);
		
	    break;
	}
	
	default:
	{
	    break;
	}
    }
    
    return;
}

/*{
** Name: allocate_and_copy_DMUCB - allocate and copy a DMUCB
**
** Description:
**	This routine copies a DMUCB from one QSF object to another.  This
**	involves allocating DMUCB of the same size in the target object
**	and filling it it.
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			DMUCB to copy from
**	target			DMUCB to copy to
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	22-feb-93 (rickh)
**	    Make sure we copy the default tuple if provided.
**	10-Feb-2004 (schka24)
**	    Copy partition definition if source has one.
**
*/


#define	ALLOCATE_AND_COPY_A_DM_DATA( field )			\
	allocate_and_copy_a_dm_data( global,			\
				     &source->field,		\
				     &dmu_cb->field );

#define	NO_FLAGS		0
#define	ATTRIBUTE_ENTRY		1

#define	ALLOCATE_AND_COPY_A_DM_PTR( field )			\
	allocate_and_copy_a_dm_ptr( global,			\
				    NO_FLAGS,			\
				    &source->field,		\
				    &dmu_cb->field );

#define	ALLOCATE_AND_COPY_DMU_ATTR( field )			\
	allocate_and_copy_a_dm_ptr( global,			\
				    ATTRIBUTE_ENTRY,		\
				    &source->field,		\
				    &dmu_cb->field );


static VOID
allocate_and_copy_DMUCB(
		OPS_STATE	*global,
		DMU_CB		*source,
		DMU_CB		**target )
{
    DMU_CB	*dmu_cb;

    dmu_cb = ( DMU_CB * ) opu_qsfmem( global, sizeof( DMU_CB ) );
    *target = dmu_cb;
    MEcopy( ( PTR ) source, ( u_i2 ) sizeof( DMU_CB ), ( PTR ) dmu_cb );

    /* now we have to find every field in the DMUCB that is a pointer, and 
    ** copy what it points to to our memory stream.  sheesh!
    */

    ALLOCATE_AND_COPY_A_DM_DATA( dmu_location );
    ALLOCATE_AND_COPY_A_DM_DATA( dmu_olocation );
    ALLOCATE_AND_COPY_A_DM_PTR( dmu_key_array );
    ALLOCATE_AND_COPY_DMU_ATTR( dmu_attr_array );
    ALLOCATE_AND_COPY_A_DM_DATA( dmu_char_array );
    ALLOCATE_AND_COPY_A_DM_DATA( dmu_conf_array );
    ALLOCATE_AND_COPY_A_DM_PTR( dmu_gwattr_array );
    ALLOCATE_AND_COPY_A_DM_DATA( dmu_gwchar_array );
    ALLOCATE_AND_COPY_A_DM_DATA( dmu_ppchar_array );
			/* for now this will always be empty because
			** create table as select never generates 
			** partial table modifies (or so saith Karl) */
    if (source->dmu_part_def != NULL)
    {
	/* This might be (ok, IS) a create table, so we need the complete
	** partition definition including names, text, etc.
	*/
	opc_full_partdef_copy(global, source->dmu_part_def,
		source->dmu_partdef_size, &dmu_cb->dmu_part_def);
	dmu_cb->dmu_partdef_size = source->dmu_partdef_size;
    }
}

/*{
** Name: allocate_and_copy_a_dm_data - copy a dm_data between QSF objects
**
** Description:
**	This routine copies a DM_DATA from one QSF object to another.  This
**	involves allocating DM_DATA of the same size in the target object
**	and filling it it.
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			DM_DATA to copy from
**	target			DM_DATA to copy to
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**
*/


static VOID
allocate_and_copy_a_dm_data(
		OPS_STATE	*global,
		DM_DATA		*source,
		DM_DATA		*target )
{
    target->data_in_size = source->data_in_size;

    if ( source->data_in_size == 0 )
    {
	target->data_address = ( char * ) NULL;
    }
    else
    {
	target->data_address =
		opu_qsfmem( global, source->data_in_size );
	MEcopy( ( PTR ) source->data_address,
	        ( u_i2 ) source->data_in_size,
	        ( PTR ) target->data_address );
    }	/* endif */
}

/*{
** Name: allocate_and_copy_a_dm_ptr - copy a dm_ptr between QSF objects
**
** Description:
**	This routine copies a DM_PTR from one QSF object to another.  This
**	involves allocating DM_PTR of the same size in the target object
**	and filling it it.
**
** Inputs:
**	global	    		Global control block for compilation.
**	source			DM_PTR to copy from
**	target			DM_PTR to copy to
**
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    None
**
** History:
**	17-nov-92 (rickh)
**	    Written for FIPS CONSTRAINTS project (release 6.5).
**	22-feb-93 (rickh)
**	    Copy the default tuple if provided if this DM_PTR is a attribute
**	    array.
**      16-Feb-2010 (hanal04) Bug 123292
**          Copy the seqTuple if provided if this DM_PTR is a attribute
**          array.
**
*/


static VOID
allocate_and_copy_a_dm_ptr(
		OPS_STATE	*global,
		i4		flags,
		DM_PTR		*source,
		DM_PTR		*target )
{
    i4		objectCount = source->ptr_in_count;
    i4		objectSize = source->ptr_size;
    i4		i;
    char		**sourceObjectAddress;
    char		**targetObjectAddress;
    DMF_ATTR_ENTRY	*sourceAttr;
    DMF_ATTR_ENTRY	*targetAttr;

    target->ptr_in_count = source->ptr_in_count;
    target->ptr_out_count = source->ptr_out_count;
    target->ptr_size = source->ptr_size;

    if ( objectCount == 0 )
    {
	target->ptr_address = ( char * ) NULL;
    }
    else
    {
	target->ptr_address =
		opu_qsfmem( global, sizeof( PTR ) * objectCount );

	for ( i = 0,
		sourceObjectAddress = ( char ** ) source->ptr_address,
		targetObjectAddress = ( char ** ) target->ptr_address;
	      i < objectCount; i++ )
	{
	    targetObjectAddress[ i ] =
		( char * ) opu_qsfmem( global, objectSize );
	    MEcopy( ( PTR ) sourceObjectAddress[ i ],
		    ( u_i2 ) objectSize,
		    ( PTR ) targetObjectAddress[ i ] );

	    /*
	    ** if this is an attribute entry, make sure you copy the
	    ** default tuple.
	    */

	    if ( flags & ATTRIBUTE_ENTRY )
	    {
		sourceAttr = ( DMF_ATTR_ENTRY * ) sourceObjectAddress[ i ];
		targetAttr = ( DMF_ATTR_ENTRY * ) targetObjectAddress[ i ];

		if ( sourceAttr->attr_defaultTuple != ( DB_IIDEFAULT * ) NULL )
		{
		    targetAttr->attr_defaultTuple = ( DB_IIDEFAULT * )
			opu_qsfmem( global, sizeof( DB_IIDEFAULT ) );

		    MEcopy( ( PTR ) sourceAttr->attr_defaultTuple,
			    ( u_i2 ) sizeof( DB_IIDEFAULT ),
			    ( PTR ) targetAttr->attr_defaultTuple );

		}	/* end if  there's a default tuple */

                if ( sourceAttr->attr_seqTuple != ( DB_IISEQUENCE * ) NULL )
                {
                    targetAttr->attr_seqTuple = ( DB_IISEQUENCE * )
                        opu_qsfmem( global, sizeof( DB_IISEQUENCE ) );

                    MEcopy( ( PTR ) sourceAttr->attr_seqTuple,
                            ( u_i2 ) sizeof( DB_IISEQUENCE ),
                            ( PTR ) targetAttr->attr_seqTuple );

                }       /* end if  there's a default tuple */

	    }	/* end if this is an attribute entry */

	}	/* end for */

    }	/* endif */
}

/*{
** Name: opc_pagesize - Determine the page size of table based on
**                      row width. This routine is similar to opn_pagesize.
**
** Description:
**      This routine takes tuple width as input, looks at the available
**      page_sizes and picks the appropriate one. If none available,
**      returns an error.
**
** Inputs:
**      width                           takes tuple width.
**
** Outputs:
**      pagesize                        Page Size is returned.
**
**      Returns:
**         None.
**      Exceptions:
**         None.
**
** Side Effects:
**          none
**
** History:
**      20-mar-1996 (nanpr01)
**          Created for multiple page size support and increased tuple size.
[@history_template@]...
*/
i4
opc_pagesize(
        OPG_CB          *opg_cb,
        OPS_WIDTH       width)
{
        DB_STATUS       status;
        i4         page_sz, pagesize;
        i4             i;
        OPO_TUPLES	noofrow, prevnorow;
	i4	    page_type;
 
        /*
        ** Now find best fit for the row
        ** We will try to fit atleast 20 row in the buffer
        */
        noofrow = prevnorow = 0;
        pagesize = 0;
        for (i = 0, page_sz = DB_MINPAGESIZE; i < DB_NO_OF_POOL;
                i++, page_sz *= 2)
        {
          if (opg_cb->opg_tupsize[i] == 0)
            continue;
 
	    if (page_sz == DB_COMPAT_PGSIZE)
		page_type = DB_PG_V1;
	    else
		page_type = DB_PG_V2;

	  noofrow = ((OPO_TUPLES)(page_sz - DB_VPT_PAGE_OVERHEAD_MACRO(page_type))/
		(OPO_TUPLES)((width + DB_VPT_SIZEOF_LINEID(page_type) + 
		DB_VPT_SIZEOF_TUPLE_HDR(page_type))));

	  /* noofrow should only be fraction if < 1 */
	  if (noofrow > 1)
	      noofrow = (OPO_TUPLES)(i4)noofrow;

          /*
          ** 20 row is arbitrary as discussed and proposed by Dick Williamson
          */
          if (noofrow >= OPC_PAGESIZE_DW_ROWS)
          {
            pagesize = page_sz;
            break;
          }
          else
          {
            if (noofrow > prevnorow)
            {
                prevnorow = noofrow;
                pagesize = page_sz;
            }
          }
        }
        if (noofrow == 0)
        {
          /*
          ** FIXME : cannot fit in buffer return error.
          ** Though this error should be caught before.
          */
          opx_error(E_OP0009_TUPLE_SIZE);
        }
        return(pagesize);
}

/*{
** Name: OPC_UPDCOLMAP -	construct bitmap of set-list of update
**
** Description:
**      Allocate bitmap for update set-list columns, then traverse parse
**	tree, building the map.
**
** Inputs:
**  global - 
**	Query wide state info.
**  upd_ahd -
**	The update action header structure to contain the bitmap.
**  setlist -
**	The parse tree root of the set-list to be posted into the map.
**
** Outputs:
**
**	Returns:
**	    The modified action header, complete with pointer to bitmap.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-dec-96 (inkdo01)
**          Created (for performance improvements to update)
**	6-feb-04 (inkdo01)
**	    Set flag indicating updates to partitioning columns.
[@history_template@]...
*/
static VOID
opc_updcolmap(
OPS_STATE   *global,
QEF_QEP     *upd_ahd,
PST_QNODE   *setlist,
DB_PART_DEF *partp)
{
    PST_QNODE	*rsdp;
    i4		i, j;
    DB_PART_DIM	*dimp;
    DB_PART_LIST *plistp;
    bool	gotone = FALSE;
 
    /* Allocate the map (opu_qsfmem init's to 0 automatically) */
    upd_ahd->ahd_upd_colmap = (DB_COLUMN_BITMAP *)
 			opu_qsfmem(global, sizeof(DB_COLUMN_BITMAP));
 
    /* Then just loop down left side of parse tree, picking up RESDOM's
    ** and setting corresponding bits. */
    for (rsdp = setlist; rsdp != (PST_QNODE *)NULL && 
 	rsdp->pst_sym.pst_type == PST_RESDOM; rsdp = rsdp->pst_left)
     if (rsdp->pst_sym.pst_value.pst_s_rsdm.pst_rsno != 0)
 				/* don't set bit 0 */
    {
	BTset(rsdp->pst_sym.pst_value.pst_s_rsdm.pst_rsno, 
 	    (char *)upd_ahd->ahd_upd_colmap);	/* set bit */

	/* If this is a partitioned table, check if columns in 
	** set list are partitioned upon. */
	if (partp && !gotone)
	 for (i = 0; i < partp->ndims && !gotone; i++)
	{
	    dimp = &partp->dimension[i];
	    if (dimp->distrule == DBDS_RULE_AUTOMATIC)
		continue;	/* no processing for automatic */
	    for (j = 0; j < dimp->ncols; j++)
	    {
		plistp = &dimp->part_list[j];
		if (rsdp->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
			plistp->att_number)
		{
		    upd_ahd->ahd_qepflag |= AHD_PCOLS_UPDATE;
				/* show part. column in set list */
		    gotone = TRUE;
		    break;
		}
	    }
	}
    }
}

/*{
** Name: OPC_PARTDEF_COPY	copy a table partitioning scheme
**
** Description:
**      Copy the runtime pieces of the DB_PART_DEF data structure for a
**	partitioned table. Used to compute the partition number for
**	inserted/updated rows, or for partition qualification.
**
**	Any value text or partition names in the original is not copied.
**
** Inputs:
**	global		query wide state info.
**	pdefp		ptr to DB_PART_DEF structure to be copied
**	getmem		Either opu_memory or opu_qsfmem
**	outppp		ptr to partition definition ptr into which
**			addr of copied info is placed
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-04 (inkdo01)
**	    Written for table partitioning.
**	2-Aug-2007 (kschendel) SIR 122513
**	    Make global instead of static.
*/
VOID
opc_partdef_copy(
	OPS_STATE	*global,
	DB_PART_DEF	*pdefp,
	PTR		(*getmem)(OPS_STATE *, i4),
	DB_PART_DEF	**outppp)

{
    DB_PART_DEF		*wpdefp;
    DB_PART_DIM		*wpdimp;
    DB_PART_BREAKS	*wpbrkp;
    PTR			valuep;
    i4			i, j;


    /* Simply copy chunks at a time, resolving addresses as we go. */
    wpdefp = (DB_PART_DEF *)(*getmem)(global, sizeof(DB_PART_DEF) +
			(pdefp->ndims-1) * sizeof(DB_PART_DIM));
    wpdefp->nphys_parts = pdefp->nphys_parts;
    wpdefp->ndims = pdefp->ndims;
    wpdefp->part_flags = pdefp->part_flags & ~(DB_PARTF_ONEPIECE);

    for (i = 0; i < pdefp->ndims; i++)
    {
	/* Copy the dimension array entries and the part_list and
	** part_breaks arrays within each. */
	STRUCT_ASSIGN_MACRO(pdefp->dimension[i], wpdefp->dimension[i]);
	wpdimp = &wpdefp->dimension[i];
	wpdimp->text_total_size = 0;
	wpdimp->partnames = NULL;
	if (wpdimp->ncols > 0)
	{
	    wpdimp->part_list = (DB_PART_LIST *)(*getmem)(global, 
		wpdimp->ncols * sizeof(DB_PART_LIST));

	    for (j = 0; j < wpdimp->ncols; j++)
		STRUCT_ASSIGN_MACRO(pdefp->dimension[i].part_list[j],
		    wpdimp->part_list[j]);

	    if (wpdimp->nbreaks > 0)
	    {
		wpdimp->part_breaks = (DB_PART_BREAKS *)(*getmem)(global,
			wpdimp->nbreaks * sizeof(DB_PART_BREAKS));
		valuep = (*getmem)(global, wpdimp->nbreaks * wpdimp->value_size);

		for (j = 0; j < wpdimp->nbreaks; j++)
		{
		    STRUCT_ASSIGN_MACRO(pdefp->dimension[i].part_breaks[j],
			wpdimp->part_breaks[j]);
		    wpbrkp = &wpdimp->part_breaks[j];
		    wpbrkp->val_text = NULL;
		    /* Copy the break constant value. */
		    MEcopy(wpbrkp->val_base, wpdimp->value_size, valuep);
		    wpbrkp->val_base = valuep;
		    valuep = &valuep[wpdimp->value_size];
		}
	    } /* any breaks */
	} /* any columns */
    }

    *outppp = wpdefp;
}

/*{
** Name: OPC_PARTDIM_COPY	copy one dimension of a table partitioning
**				scheme
**
** Description:
**      Copy one dimension of a table partitioning scheme. This 
**	consists of the DB_PART_DIM structure and those nested inside.
**	These are used by QEF to determine partitions qualified by
**	restriction predicates on partitioning columns.
**
** Inputs:
**	global		query wide state info.
**	pdefp		ptr to DB_PART_DEF structure to be copied
**	dimno		number of partitioning dimension to be copied
**	outppp		ptr to partition dimension ptr into which
**			addr of copied info is placed
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-04 (inkdo01)
**	    Written for table partitioning.
*/
VOID
opc_partdim_copy(
	OPS_STATE	*global,
	DB_PART_DEF	*pdefp,
	u_i2		dimno,
	DB_PART_DIM	**outppp)

{
    DB_PART_DIM		*wpdimp;
    DB_PART_BREAKS	*wpbrkp;
    PTR			valuep;
    i4			i;


    /* Simply copy chunks at a time, resolving addresses as we go. */
    wpdimp = (DB_PART_DIM *)opu_qsfmem(global, sizeof(DB_PART_DIM));
    STRUCT_ASSIGN_MACRO(pdefp->dimension[dimno], *wpdimp);

    wpdimp->text_total_size = 0;
    wpdimp->partnames = NULL;
    wpdimp->part_list = (DB_PART_LIST *)opu_qsfmem(global, 
	wpdimp->ncols * sizeof(DB_PART_LIST));
    *outppp = wpdimp;

    for (i = 0; i < wpdimp->ncols; i++)
	STRUCT_ASSIGN_MACRO(pdefp->dimension[dimno].part_list[i],
	    wpdimp->part_list[i]);

    if (wpdimp->nbreaks == 0)
	return;			/* no breaks for HASH, AUTOMATIC */

    wpdimp->part_breaks = (DB_PART_BREAKS *)opu_qsfmem(global,
	wpdimp->nbreaks * sizeof(DB_PART_BREAKS));
    valuep = opu_qsfmem(global, wpdimp->nbreaks * wpdimp->value_size);

    for (i = 0; i < wpdimp->nbreaks; i++)
    {
	STRUCT_ASSIGN_MACRO(pdefp->dimension[dimno].part_breaks[i],
	    wpdimp->part_breaks[i]);
	wpbrkp = &wpdimp->part_breaks[i];
	wpbrkp->val_text = NULL;
	/* Copy the break constant value. */
	MEcopy(wpbrkp->val_base, wpdimp->value_size, valuep);
	wpbrkp->val_base = valuep;
	valuep = &valuep[wpdimp->value_size];
    }

}

/*
** Name: opc_full_partdef_copy -- Copy a complete partition definition
**
** Description:
**	Here's yet another partition definition copier.  This one is
**	used when generating a QP for a DDL operation, specifically
**	either of the CREATE TABLE forms.  (Not declare global temporary
**	table, they can't be partitioned;  and not modify or create index,
**	as those two don't come through OPC, they go straight from the
**	parser to QEF.)
**
**	The reason for having yet another definition copier is that
**	most of the DML statements don't care about the value texts or
**	partition names that are part of a complete partition definition.
**	CREATE does care, however, because eventually the partition
**	definition is going to be written out to the catalogs, and it
**	might be nice if all the pieces were there!
**
**	This routine could be written to assume that the incoming
**	partition definition is flagged DB_PARTF_ONEPIECE, meaning that
**	it can be copied in one piece with a bit of pointer relocation.
**	All the necessary info is there.  At the moment, though, I have
**	bigger fish to fry, so I'm just going to call the usual RDF
**	partition copier utility and let it figure everything out.
**
** Inputs:
**	global		OPF request state holder
**	pdefp		Pointer to partition definition to copy
**	pdefsize	Size of partition definition (if known)
**	outppp		An output
**
** Outputs:
**	No return value
**	outppp		Pointer to the copied partition definition
**
** History:
**	9-Feb-2004 (schka24)
**	    Yet another partition copier.
**	6-Jul-2006 (kschendel)
**	    Fill in unique dbid for completeness.
*/

static void
opc_full_partdef_copy(
	OPS_STATE *global,
	DB_PART_DEF *pdefp,
	i4 pdefsize,
	DB_PART_DEF **outppp)
{
    DB_STATUS status;
    RDF_CB rdfcb;			/* RDF request block */
    RDR_INFO rdf_info;			/* RDF info area, mostly dummy */

    /* Format a request block for RDF. */
    MEfill(sizeof(rdfcb), 0, &rdfcb);	/* Don't confuse RDF with junk */
    MEfill(sizeof(rdf_info), 0, &rdf_info);
    rdfcb.rdf_info_blk = &rdf_info;
    rdfcb.rdf_rb.rdr_db_id = global->ops_cb->ops_dbid;
    rdfcb.rdf_rb.rdr_unique_dbid = global->ops_cb->ops_udbid;
    rdfcb.rdf_rb.rdr_fcb = global->ops_cb->ops_server->opg_rdfhandle;
    rdfcb.rdf_rb.rdr_session_id = global->ops_cb->ops_sid;
    rdfcb.rdf_rb.rdr_partcopy_mem = opc_qsfmalloc;
    rdfcb.rdf_rb.rdr_partcopy_cb = global;
    rdfcb.rdf_rb.rdr_instr = RDF_PCOPY_NAMES | RDF_PCOPY_TEXTS;
    /* The original partition def is in the rdf info block,
    ** and the new copy comes back in rdfrb.
    */
    rdf_info.rdr_types = RDR_BLD_PHYS;	/* Copy function wants to see this */
    rdf_info.rdr_parts = pdefp;
    status = rdf_call(RDF_PART_COPY, &rdfcb);
    if (DB_FAILURE_MACRO(status))
    {
	opx_verror(status, E_OP009C_RDF_COPY, rdfcb.rdf_error.err_code);
    }
    *outppp = rdfcb.rdf_rb.rdr_newpart;
} /* opc_full_partdef_copy */


/* The RDF copier expects a slightly different call interface than what
** we get from opu_qsfmem, so here is a little wrapper:
*/

static DB_STATUS
opc_qsfmalloc(
	OPS_STATE *global,
	i4 psize,
	void *pptr)
{
    *(void **)pptr = opu_qsfmem(global, psize);
    return (E_DB_OK);
}

/* Name: opc_loadopt -- Check for APPEND -> LOAD optimization
**
** Description:
**
**	*Note* for now (Nov 2009), LOAD is still only used for CTAS.
**	It's not yet enabled for insert/select.
**
**	When inserting multiple rows into a table, it's very desirable
**	to use the bulk LOAD operation rather than the row by row APPEND
**	operation.  This routine is called after the insert statement
**	is compiled (including rules if any), to check for and perform
**	the transformation of APPEND into LOAD.
**
**	LOAD may be used if (and only if):
**	1. The statement was APPEND, RETINTO, or DGTT_AS_SELECT.  (The
**	   SQL equivalents of the first two are INSERT and CREATE TABLE
**	   AS SELECT.)
**
**	2. The target table is not journaled.  In the case of RETINTO,
**	   it's OK for the target to be "with journaling" as long as
**	   the database itself is not being journaled.
**
**	3. The target table has no secondary indexes.
**
**	4. If it's the APPEND (INSERT) statement, the target must be a
**	   heap.  Technically, DMF allows LOAD into empty structured
**	   tables as well, but it's done as a "rebuild" load which has
**	   table-locking semantics that don't match the traditional
**	   INSERT locking semantics.  For heaps, it doesn't matter.
**
**	5. If there are any rules (which implies that it's an INSERT),
**	   the rule procedure parameters must not involve the TID.
**	   (Tids don't exist in the normal sense when loading, and in
**	   any case DMF doesn't pass them back.)
**
**	6. Again, if there are any rules, the called DB procedures may
**	   not refer to the target table.  This includes DBP's called
**	   indirectly by nested rules.  Unfortunately, we don't have
**	   the machinery to figure this out efficiently;  a transitive
**	   closure on iidbdepends should do it, but that's hardly efficient.
**	   For now, we'll restrict allowable rules to system generated
**	   rules, as we know they will be safe.
**
**	7. The target table is not a gateway, or a partitioned temporary
**	   table.  DMF does not have the necessary smarts for either,
**	   and these non-logged targets don't suffer as much from row-PUT.
**
**	If the statement can be transformed into a LOAD, all we need to
**	do is change the action header operation, and maybe light a flag
**	or two.  The rest of the compiled plan remains untouched.
**
**	As a side note, this optimization is really more than just an
**	optimization.  Because SQL's CTAS has no convenient way to
**	enforce precise target table data types, nullability, and default-
**	ability, it's often desirable to CREATE a table exactly as desired,
**	and then INSERT into it.  Clearly we want to use LOAD if at
**	all possible, and avoid row logging.
**
** Inputs:
**	global		Optimizer state info for query
**	ahd		Pointer to compiled APPEND action header
**	valid		Pointer to result's VALID struct entry
**
** Outputs:
**	None
**
** History:
**	23-Oct-2006 (kschendel) SIR 122890
**	    Written.
**	16-Oct-2007 (kschendel)
**	    Eschew loads for partitioned GTT's, gateway streams.
*/

static void
opc_loadopt(OPS_STATE *global, QEF_AHD *ahd, QEF_VALID *valid)
{

    DMT_TBL_ENTRY *dmt;			/* dmt show info for result table */
    OPS_SUBQUERY *sq = global->ops_cstate.opc_subqry;
    i4 qmode = sq->ops_mode;

    if (qmode != PSQ_APPEND && qmode != PSQ_RETINTO
      && qmode != PSQ_DGTT_AS_SELECT)
	return;

    if (valid == NULL)
	return;				/* Should not happen, defensive */

    if (qmode != PSQ_APPEND)
    {
	/* RETINTO and DGTT-AS-SELECT can compile a LOAD unless the
	** target is journaled.  A new table necessarily satisfies the
	** other load conditions.
	*/
	if (global->ops_qheader->pst_restab.pst_resjour == PST_RESJOUR_ON)
	    return;
	/* We'll do this one with LOAD.  Flag it as a CTAS so that QEF
	** knows to tell DMF that the table is known to be empty.
	*/
	ahd->ahd_atype = QEA_LOAD;
	ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_LOAD_CTAS;
	valid->vl_flags |= QEF_NEED_XLOCK;  /* Open with X locking for LOAD */
	return;
    }

    /* *** INSERT/SELECT via load not yet supported */
    return;

    /* Here for INSERT.  First, don't bother if no QP underneath;
    ** LOAD would work for insert/values, but there's no point.
    */
    if (ahd->qhd_obj.qhd_qep.ahd_qep == NULL)
	return;

    /* The transform can be turned off at need by trace point op129 */
    if (opt_strace(global->ops_cb, OPT_T129_NO_LOAD))
	return;

    /* Require that the target be a non-journaled HEAP with no secondary
    ** indexes, and not a gateway table.
    ** This even applies to IngresStream gateway tables.  DMF has no hook
    ** from LOAD into GWF, and it's not really needed anyway;  the
    ** row-PUT stuff dispatches off to GWF pretty much right away,
    ** and there would be little benefit to using LOAD for stream-tables.
    */
    dmt = global->ops_rangetab.opv_base->opv_grv[sq->ops_result]->opv_relation->rdr_rel;
    if (dmt->tbl_storage_type != DMT_HEAP_TYPE
      || dmt->tbl_status_mask & (DMT_IDXD | DMT_CATALOG | DMT_JNL | DMT_GATEWAY) )
	return;

    /* Require that the target not be a partitioned temp table.
    ** It's unclear that DMF always does the right thing in this case,
    ** because it's always assumed that "in-memory" loads apply to the
    ** entire table.  Plus, since GTT's are not logged anyway, the
    ** row-PUT's are not going to be all that much more expensive
    ** than LOAD would be.
    ** If/when DMF gets its act together re partitioned GTT's, this
    ** test can be removed.
    */
    if (dmt->tbl_status_mask & DMT_IS_PARTITIONED && dmt->tbl_temporary)
	return;

    /* If there is any BEFORE rule, it can't be a system generated one,
    ** so no LOAD.  If there is an AFTER rule, ask whether it's system
    ** generated.  If so, allow the LOAD.  Note that the restriction about
    ** no tids will be enforced as the rule itself compiles (see opcqual).
    */
    if (global->ops_statement->pst_before_stmt != NULL)
	return;

    if (global->ops_statement->pst_after_stmt != NULL
      && opn_nonsystem_rule(sq))
	return;

    ahd->ahd_atype = QEA_LOAD;
    valid->vl_flags |= QEF_NEED_XLOCK;	/* Open with X locking for LOAD */

} /* opc_loadopt */
