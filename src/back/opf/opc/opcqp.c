/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <ade.h>
#include    <adfops.h>
#include    <adp.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <gca.h>
#include    <ulc.h>
#include    <me.h>
#include    <bt.h>
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
#define        OPC_QP 	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCQP.C - Routines for filling in the non-QEF_AHD part of 
**			the QEF_QP_CB
**
**  Description:
**      This file contains the routines required to initialize and/or fill 
**	in all of a QEF_QP_CB except for the list of QEF_AHD's. For those,
**      see OPCAHD.C and opc_ahd_build().
**
**	External Routines (visible outside this file):
**          opc_iqp_init()     - Begin building a QEF_QP_CB
**          opc_cqp_continue() - Continue building a QEF_QP_CB
**          opc_fqp_finish()   - Finish building a QEF_QP_CB
**          opc_qsfmem()       - Get memory from QSF, called by many.
**          opc_ptrow()	       - Put a row on the list for QEF's dsh
**	    opc_allocateIDrow  - Allocate a row to hold an 8 byte object id
**          opc_ptcb()         - Same as opc_ptrow() only for control blocks.
**          opc_pthold()       - Same as opc_ptrow() only for hold files
**	    opc_ptsort()       - Same as opc_ptrow() only for small sort info.
**          opc_ptparm()       - Add a repeat query parm to the QP and others.
**	    opc_ptcparm()      - Add a CALLPROC parameter array to the DSH.
**          opc_tempTable      - Alloc/format QEN_TEMP_TABLE.
**
**	Internal Routines:
**          opc_lsparms()   - Make a list of the repeat query param info
**	    opc_singleton() - Figure out if a QP can only return one tuple.
**	    opc_resolveif() - Finish filling in IF actions at the end.
**	    opc_mtlist()    - Modify target list to remove duplicate resdoms.
**
**
**  History:    
**      29-jul-86 (eric)
**          created
**      01-oct-87 (fred)
**          Modified for GCF.
**	02-sep-88 (puree)
**	    Modify VLUP/VLT handling.  Rename qp_cb_num to qp_cb_cnt.
**	30-jan-89 (paul)
**	    Enhance to support rule processing. Includes dispatching changes
**	    for new statement types (CALLPROC and EMESSAGE), changes to the
**	    compilation complation logic to properly attach rules to AHD's
**	    and opc_ptcparm to create descriptors for CALLPROC parameter arrays.
**	22-mar-89 (neil)
**	    Initialized more rules stuff.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
**      24-jul-89 (jennifer)
**          copy audit information from statement header to QP.
**	08-sep-89 (neil)
**	    Alerters: Add recognition of new EVENT actions.
**      12-dec-89 (fred)
**	    Added support for large object retrieval to opc_singleton().
**	12-jan-90 (neil)
**	    Make sure to attach rule to correct AHD.
**	15-jan-90 (fredp)
**	    Adjust the default value of qp->qp_key_sz and the size of 
**	    the key row to max(sizeof(i4), sizeof(ALIGN_RESTRICT)), in
**	    opc_iqb_init()).
**	12-jun-90 (stec)
**	    Fixed bug 30576 in opc_cqp_continue().
**	26-nov-90 (stec)
**	    Modified opc_fqp_finish().
**	10-dec-90 (neil)
**	    Alerters: Add recognition of new EVENT actions to opc_cqp_continue.
**	28-dec-90 (stec)
**	    Modified opc_fqp_finish().
**	04-feb-91 (stec)
**	    Minor code cleanup related to code style.
**	11-aug-92 (rickh)
**	    Added opc_tempTable.
**	01-sep-1992 (rog)
**	    Added gca.h and ulc.h to pull in ULC prototypes.
**	28-oct-92 (jhahn)
**	    Added support for statement end rules.
**	12-nov-92 (rickh)
**	    Translate PST_CREATE_TABLE_TYPE, PST_CREATE_INTEGRITY_TYPE,
**	    and PST_DROP_INTEGRITY_TYPE statements into actions.
**	23-nov-92 (anitap)
**	    Added support for CREATE SCHEMA project in opc_cqp_continue().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      20-jul-1993 (pearl)
**          Bug 45912.  Initialize new QEP_QP_CB field qp_kor_cnt in
**          opc_iqp_init().
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**      5-oct-93 (eric)
**          Added support for tables in resource lists in qp's.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs -
**	    specifically to replace QEN_TEMP_TABLE structs by pointers.
**      16-oct-2006 (stial01)
**          opc_iqp_init() if opf_locator, return locator instead of blob.
**      06-nov-2006 (stial01)
**          Changes to return lob/locator as specified by opf_locator
**	18-Aug-2009 (drivi01)
**	    Fixed precedence warnings and other warnings in efforts
**	    to port to Visual Studio 2009.
[@history_template@]...
**/

static VOID
opc_lsparms(
	OPS_STATE	*global,
	PST_QNODE	*root );

static VOID
opc_singleton(
	OPS_STATE	*global,
	i4		*status );

static VOID
opc_resolveif(
	OPS_STATE	*global,
	i4		for_depth,
	PST_STATEMENT	*cur_stmt,
	PST_STATEMENT	*last_stmt );

static VOID
opc_mtlist(
	OPS_STATE   *global,
	PST_QNODE   *root );

static VOID
opc_resdom_modifier(
	OPS_STATE    *global,
	PST_QNODE    *resdom,
	i4	     *bits);

VOID
opc_deferred_cpahd_build(
    OPS_STATE	*global,
    QEF_AHD	**pahd);


/*{
** Name: OPC_IQP_INIT	- Begin building an entire, complete QEF_QP_CB struct.
**
** Description:
**      Opc_iqp_init(), opc_cqp_continue(), and opc_fqp_finish together build
**	a complete QEP_QP_CB structure based on the info
**      in the OPS_STATE struct that is passed in. Most of the work toward this
**      goal is done by opc_ahd_build() and below. 
**
**	This routine, however, does
**      take the responsibility of allocating and giving initial values to
**	the non-action header parts of the QP. In some cases, we can directly 
**	figure out what the information is,
**      for example QEP_QP_CB.qp_res_row_sz. In other cases, we don't have
**      info the completely fill in the field(s), but we must initialize them
**      so that they may be correctly incremented later, for example 
**      QEF_QP_CB.qp_cb_cnt.
**
** Inputs:
**	global
**	    A pointer to the OPS_STATE struct for this optimization.
**	global->ops_qsfcb
**	    The QSF rcb that is used to allocate the QEF_QP_CB struct.
**	global->ops_qheader
**	    This is used to find out the updatability of this cursor, repeat
**	    query info, the size of the results, and other related info.
**	    This should only be used if the statement type is "query tree" (QT)
**	global->ops_subquery
**	    The list of subqueries are scanned, and each one is given to 
**	    opc_ahd_build().
**
** Outputs:
**	global->ops_cstate.opc_qp
**	    A pointer to the initialized QEF_QP_CB.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (eric)
**          written
**      10-may-88 (eric)
**          Created from opc_qp_build for DB procedures.
**	02-sep-88 (puree)
**	    Remove qp_cb_cnt.  Rename the old qp_cb_num to qp_cb_cnt.
**	    Initialize the new field qp_pcx_cnt added for VLUP/VLT handling.
**      13-dec-88 (seputis)
**          fixed lint error
**	30-jan-89 (paul)
**	    Initialize CALLPROC parameter counts and sizes for rule
**	    processing enhancements.
**	22-mar-89 (neil)
**	    Initialized opc_cparms too.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
**      24-jul-89 (jennifer)
**          copy audit information from statement header to QP.
**      9-nov-89 (eric)
**          added intialization for opc_topdecvar.
**	15-jan-90 (fredp)
**	    Adjust the default value of qp->qp_key_sz and the size of 
**	    the key row to max(sizeof(i4), sizeof(ALIGN_RESTRICT)).
**	25-jan-91 (andre)
**	    A new pointer in PST_QTREE may point to additional information used
**	    by PSF in determining if a given query may use an existing QEP.
**	    Made changes to copy it into QEF_QP_CB.  Also, PST_QTREE will
**	    contain indication if this  is a QEP for an SQL query and if it is
**	    a QEP for a shareable query.  Corresponding flags must be set in
**	    QEF_QP_CB.qp_status
**	08-may-91 (rog)
**	    Set the array of table ids to NULL if there aren't any instead
**	    of trying to allocate zero bytes for the array.
**	    (fix for bug 37410)
**	20-jul-92 (rickh)
**	    Pointer cast in MEcopy call.
**	11-aug-92 (rickh)
**	    Initialize to 0 the count of temporary tables.
**	12-may-93 (davebf)
**	    Removed call to opc_tstsingle.  It is obsolete now.  QP is now set
**	    repeat regardless.
**      20-jul-1993 (pearl)
**          Bug 45912.  Initialize new QEP_QP_CB field qp_kor_cnt in
**          opc_iqp_init().
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**      5-oct-95 (eric)
**          Corrected a problem in deferred/direct cursor semantics by
**          adding support for a deferred bit in qps.
**	30-mar-00 (inkdo01)
**	    Add logic to reduce the use of QEQP_DEFERRED when possible.
**	25-apr-01 (inkdo01)
**	    Reset BLOB result lengths to -1 in procedure "return row" stmts.
**	5-nov-03 (inkdo01)
**	    Reserve first 7 entries of QP row array for use by global CX 
**	    pointer array logic (|| queries).
**	24-aug-04 (inkdo01)
**	    Add rest of global CX base array logic.
**	24-Mar-2005 (schka24)
**	    Kill qconst mask.
**	1-Dec-2005 (kschendel)
**	    Add replacement- selection count.
**	14-Feb-2006 (kschendel)
**	    Remove above, never used it.
**	13-apr-06 (dougi)
**	    Turn on global CX base array by default.
**	25-may-06 (dougi)
**	    Copy dbproc name to qp for diagnostics.
**	26-Sep-2007 (kschendel) SIR 122512
**	    Initialize FOR-depth, parallel query head counters.
**	30-oct-2007 (dougi)
**	    Set repeat dynamic flag as needed.
**	22-jul-08 (hayke02)
**	    Switch off global base array (QEQP_GLOBAL_BASEARRAY) if we have
**	    an in-memory constant table (OPS_MEMTAB_XFORM). This prevents
**	    NULL dsh_rows resulting in SEGVs. This change fixes bug 120585.
**	10-march-2009 (dougi) bug 121773
**	    Changes to identify query plans for cached dynamic queries with 
**	    LOB locators enabled.
**	1-Jul-2010 (kschendel) b124004
**	    Kill qp_upd_cb, add qp-fetch-ahd.
*/
VOID
opc_iqp_init(
		OPS_STATE          *global)
{
    PST_QNODE	    *qn;
    QEF_QP_CB	    *qp;
    i4		    dummy;

    /* allocate the qp header; */
    qp = global->ops_cstate.opc_qp = 
			(QEF_QP_CB *) opu_qsfmem(global, sizeof (QEF_QP_CB));

    /* Fill in the qp header. 
    ** Lets start with the boiler plate stuff
    */
    qp->qp_next = qp->qp_prev = qp;
    qp->qp_length = sizeof (QEF_QP_CB);
    qp->qp_type = QEQP_CB;
    qp->qp_ascii_id = QEQP_ASCII_ID;

    /* Now lets get to the QP header specific stuff */

    qp->qp_status = 0;
    qp->qp_resources = (QEF_RESOURCE *) NULL;
    qp->qp_cnt_resources = 0;
    qp->qp_rssplix = -1;

    /*
    ** set qp_shr_rptqry_info ptr to NULL in case the information was not
    ** provided (i.e. if this is not a shareable QUEL repeat query)
    */
    qp->qp_shr_rptqry_info = (DB_SHR_RPTQRY_INFO *) NULL;

    if (global->ops_procedure->pst_isdbp == TRUE)
    {
	qp->qp_status = (qp->qp_status | QEQP_RPT | QEQP_DEFERRED | QEQP_ISDBP);
	STRUCT_ASSIGN_MACRO(global->ops_procedure->pst_dbpid, qp->qp_id);
    }
    else if (global->ops_qheader != NULL)
    {
        if (global->ops_qheader->pst_mask1 & PST_RPTQRY)
	{
	    qp->qp_status = (qp->qp_status | QEQP_RPT);
	}

        if (global->ops_procedure->pst_flags & PST_REPEAT_DYNAMIC)
	{
	    qp->qp_status = (qp->qp_status | QEQP_REPDYN);
	}

	if (global->ops_qheader->pst_delete == TRUE)
	{
	    qp->qp_status = (qp->qp_status | QEQP_DELETE);
	}

	if (global->ops_qheader->pst_updtmode == PST_DIRECT ||
		 (global->ops_qheader->pst_mode == PSQ_DEFCURS &&
		    global->ops_qheader->pst_updtmode == PST_UNSPECIFIED
		 )
	    )
	{
	    qp->qp_status = (qp->qp_status | QEQP_UPDATE);
	}
	else if (global->ops_qheader->pst_updtmode == PST_DEFER)
	{
	    qp->qp_status = (qp->qp_status | QEQP_UPDATE | QEQP_DEFERRED);
	}
	else
	{
	    qp->qp_status = (qp->qp_status | QEQP_DEFERRED);
	}

	if (global->ops_qheader->pst_mask1 & PST_SHAREABLE_QRY)
	{
	    qp->qp_status |= QEQP_SHAREABLE;
	}

	if (global->ops_qheader->pst_mask1 & PST_SQL_QUERY)
	{
	    qp->qp_status |= QEQP_SQL_QRY;
	}

	/*
	** check if PSF supplied PST_QTREE.pst_info
	*/

	if (global->ops_qheader->pst_info != (PST_QRYHDR_INFO *) NULL)
	{
	    /*
	    ** if "shared repeat query" information was provided, copy it from
	    ** PST_QTREE to QEF_QP_CB
	    */
	    if (global->ops_qheader->pst_info->pst_1_usage & PST_SHR_RPTQRY)
	    {
		DB_SHR_RPTQRY_INFO	*psf_qry_info, *qp_qry_info;
		register DB_TAB_ID	*psf_tab_id, *qp_tab_id;
		register i4		i;

		psf_qry_info =
		    global->ops_qheader->pst_info->pst_1_info.pst_shr_rptqry;

		/* first allocate the structure itself */
		qp->qp_shr_rptqry_info = qp_qry_info = (DB_SHR_RPTQRY_INFO *)
		    opu_qsfmem(global, sizeof (DB_SHR_RPTQRY_INFO));

		/* copy simple fields */
		qp_qry_info->db_qry_len    = psf_qry_info->db_qry_len;
		qp_qry_info->db_num_tblids = psf_qry_info->db_num_tblids;

                /*
                ** At this point in non-distributed code, the query text
                ** is copied.  For distributed, shareability of repeat
                ** queries can be decided without comparing text
		** - FIXME in distributed the following 2 statements were
		** omitted, does this make a difference?
                */
		/* copy query text */
		if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		    qp_qry_info->db_qry = NULL;
		else
		{
		    qp_qry_info->db_qry =
			(u_char *) opu_qsfmem(global, qp_qry_info->db_qry_len);

		    MEcopy((PTR) psf_qry_info->db_qry,
			qp_qry_info->db_qry_len, (PTR) qp_qry_info->db_qry);
		}

                /*
                ** copy array of table ids; if there are no IDs to copy, just
                ** set the pointer to NULL
                */
                qp_qry_info->db_tblids = (qp_qry_info->db_num_tblids) ?
                    (DB_TAB_ID *) opu_qsfmem(global,
                               qp_qry_info->db_num_tblids * sizeof(DB_TAB_ID))
                    : (DB_TAB_ID *) NULL;

		for (i = qp_qry_info->db_num_tblids,
		     psf_tab_id = psf_qry_info->db_tblids,
		     qp_tab_id = qp_qry_info->db_tblids;

		     i > 0;

		     i--, psf_tab_id++, qp_tab_id++
		    )
		{
		    qp_tab_id->db_tab_base  = psf_tab_id->db_tab_base;
		    qp_tab_id->db_tab_index = psf_tab_id->db_tab_index;
		}
	    }
	}
    }

    /* If old logic set QEQP_DEFERRED, look a little closer to see if we
    ** can turn it off. Specifically, it may not be needed if the query
    ** plan is being build for a simple delete, an insert which doesn't
    ** select from itself, a cursor with the FOR UPDATE clause or a 
    ** procedure with no update statements or nested
    ** proc. calls. For delete's and insert's, there may also be no 
    ** statement level rules being triggered. */
    if (qp->qp_status & QEQP_DEFERRED)
    {
	PST_STATEMENT	*stmt;
	PST_QTREE	*tree;
	i4		i;
	bool		defer = FALSE;

 	for (stmt = global->ops_procedure->pst_stmts; 
		stmt != NULL && !defer; stmt = stmt->pst_link)
	{
	    /* Loop over statements within procedure. If nested call,
	    ** keep flag on. If DML statement, look more closely; else 
	    ** loop. */
	    if (stmt->pst_type == PST_CP_TYPE) defer = TRUE;
	    else if (stmt->pst_type != PST_QT_TYPE) continue;
	    else
	    {
		/* DML statement - check the cases. */
		tree = stmt->pst_specific.pst_tree;
		if (tree->pst_mode == PSQ_REPLACE) defer = TRUE;
		else if ((tree->pst_mode == PSQ_DELETE ||
		    tree->pst_mode == PSQ_APPEND) && 
		    stmt->pst_after_stmt != stmt->pst_statementEndRules)
				defer = TRUE;  /* statement level rule */
		else if (tree->pst_mode == PSQ_DEFCURS &&
		    BTcount((char *)&tree->pst_updmap, 
			(i4)BITS_IN(tree->pst_updmap)) > 0)
				defer = TRUE;	/* update cursor */
		else if (tree->pst_mode == PSQ_APPEND && 
		    tree->pst_rngvar_count > 1)
		 for (i = 1; i < tree->pst_rngvar_count && !defer; i++)
		  if (tree->pst_rangetab[i] != NULL &&
			tree->pst_restab.pst_restabid.db_tab_base ==
			tree->pst_rangetab[i]->pst_rngvar.db_tab_base)
				defer = TRUE;  /* INSERT ... SELECT self ref */
	    }
	}	/* end of statement within proc loop */

	if (!defer) qp->qp_status &= ~QEQP_DEFERRED;
    }

    MEcopy(global->ops_qsfcb.qsf_obj_id.qso_name, 
		sizeof(qp->qp_id), (PTR) &qp->qp_id);

    /* Give query plan unique id */
    qp->qp_id.db_cursor_id[0] = (i4) global->ops_cb->ops_psession_id;
    qp->qp_id.db_cursor_id[1] = (i4) global->ops_cb->ops_session_ctr++;

    if (global->ops_procedure->pst_isdbp == TRUE)
    {
	qp->qp_qmode = PSQ_EXEDBP;
    }
    else if (global->ops_qheader != NULL)
    {
	qp->qp_qmode = global->ops_qheader->pst_mode;
    }
    /* else qp_qmode is left alone and unused */

    /* Set the result row size */
    qp->qp_res_row_sz = 0;
    if (global->ops_procedure->pst_isdbp == FALSE &&
	global->ops_qheader != NULL)
    {
	for (qn = global->ops_qheader->pst_qtree->pst_left;
	     qn->pst_sym.pst_type == PST_RESDOM; 
	     qn = qn->pst_left
	    )
	{
	    DB_STATUS	status;
	    i4		bits;
	    /*
	    ** Determine if RESDOM is a large object. If so, set its
	    ** length to unknown, and don't count it in the overall length.
	    */

	    status = adi_dtinfo(global->ops_adfcb,
				qn->pst_sym.pst_dataval.db_datatype,
				&bits);

	    /* The query modifier indicates if LOB or LOCATOR should be returned */
	    if (bits & (AD_PERIPHERAL | AD_LOCATOR))
	    {
		opc_resdom_modifier(global, qn, &bits);
	    }
	    if (bits & AD_LOCATOR)
		qp->qp_status |= QEQP_LOCATORS;		/* set flag */

	    if ((bits & AD_PERIPHERAL) == 0)
	    {
		qp->qp_res_row_sz += qn->pst_sym.pst_dataval.db_length;
	    }
	    else
	    {
		qn->pst_sym.pst_dataval.db_length = ADE_LEN_UNKNOWN;
	    }
	}
    }
    else if (global->ops_procedure->pst_isdbp == TRUE)
    {
	PST_STATEMENT	*stmt;
	PST_QTREE	*tree;

	/* Also transform BLOB RESDOMs in row producing procedure
	** "return row" statements. Loop in search of "return row", then
	** loop over its RESDOMs in search of BLOBs. */
 	for (stmt = global->ops_procedure->pst_stmts; 
		stmt != NULL; stmt = stmt->pst_link)
	 if (stmt->pst_type == PST_QT_TYPE && 
	    (tree = stmt->pst_specific.pst_tree)->pst_mode == PSQ_RETROW)
	  for (qn = tree->pst_qtree->pst_left;
	     qn && qn->pst_sym.pst_type == PST_RESDOM; 
	     qn = qn->pst_left
	    )
	{
	    DB_STATUS	status;
	    i4		bits;
	    /*
	    ** Determine if RESDOM is a large object. If so, set its
	    ** length to unknown, and don't count it in the overall length.
	    */

	    status = adi_dtinfo(global->ops_adfcb,
				qn->pst_sym.pst_dataval.db_datatype,
				&bits);

	    /* The query modifier indicates if LOB or LOCATOR should be returned */
	    if (bits & (AD_PERIPHERAL | AD_LOCATOR))
	    {
		opc_resdom_modifier(global, qn, &bits);
	    }
	    if (bits & AD_LOCATOR)
		qp->qp_status |= QEQP_LOCATORS;		/* set flag */

	    if ((bits & AD_PERIPHERAL) == 0)
	    {
		qp->qp_res_row_sz += qn->pst_sym.pst_dataval.db_length;
	    }
	    else
	    {
		qn->pst_sym.pst_dataval.db_length = ADE_LEN_UNKNOWN;
	    }
	}
    }

    /* At the current time, we don't know what values to put into the following
    ** fields, so we will give them default values. As the list of action
    ** headers are built, these fields will be filled in or increased to thier
    ** correct values.
    */
    qp->qp_key_row = -1;
    qp->qp_key_sz = max( sizeof (i4), sizeof (ALIGN_RESTRICT) );
    qp->qp_row_cnt = 0;
    qp->qp_kor_cnt = 0;
    qp->qp_row_len = NULL;
    qp->qp_cp_cnt = 0;
    qp->qp_cp_size = NULL;
    qp->qp_stat_cnt = 0;
    qp->qp_cb_cnt = 0;
    qp->qp_hld_cnt = 0;
    qp->qp_tempTableCount = 0;
    qp->qp_sort_cnt = 0;
    qp->qp_ahd_cnt = 0;
    qp->qp_ahd = NULL;
    qp->qp_fetch_ahd = NULL;
    qp->qp_ndbp_params = 0;
    qp->qp_dbp_params = NULL;
    qp->qp_pcx_cnt = 0;
    qp->qp_rrowcnt_row = -1;
    qp->qp_orowcnt_offset = -1;
    qp->qp_rerrorno_row = -1;
    qp->qp_oerrorno_offset = -1;
    qp->qp_for_depth = 0;
    qp->qp_pqhead_cnt = 0;

    /* For now - default is to compile CXs for global base array. Removing
    ** the flag will compile for local CX bases. */
    qp->qp_status |= QEQP_GLOBAL_BASEARRAY;
    /* TP op195 or in-memory constant table reverts to local CX bases. */
    if ((global->ops_cb->ops_check &&
			opt_strace(global->ops_cb, OPT_F067_PQ))
	||
	(global->ops_gmask & OPS_MEMTAB_XFORM))
        qp->qp_status &= ~QEQP_GLOBAL_BASEARRAY;
    
    /* initialize the compilation state so that we can fill in the action 
    ** headers.
    */
    global->ops_cstate.opc_rows = NULL;
    /* Init the first 7 entries to length -1 for global CX pointer
    ** array changes */
    for (dummy = 0; dummy < ADE_IBASE; dummy++)
	opc_ptrow(global, &dummy, -1);

    /* add the temp row, allowing space to hold the key type for making keys.
    ** Row size is set to larger of sizeof(i4) and sizeof (ALIGN_RESTRICT).
    ** Its addr is saved in opc_temprow for use in opcadf/qual.c.
    */
    if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED))
	opc_ptrow(global, &dummy, max(sizeof(i4), sizeof(ALIGN_RESTRICT)));
    global->ops_cstate.opc_temprow = global->ops_cstate.opc_rows->opc_oprev;

    global->ops_cstate.opc_qnnum = 0;
    global->ops_cstate.opc_holds = NULL;
    global->ops_cstate.opc_firstahd = NULL;
    global->ops_cstate.opc_rparms = NULL;
    global->ops_cstate.opc_cparms = NULL;
    global->ops_cstate.opc_pvrow_dbp = -1;
    global->ops_cstate.opc_flags = 0;
    global->ops_cstate.opc_topdecvar = NULL;
    global->ops_cstate.opc_curnode = NULL;

    /*
    ** Initialize the titan-specific portions of the query plan header.
    */

    qp->qp_ddq_cb.qeq_d1_end_p = NULL;
    qp->qp_ddq_cb.qeq_d2_ldb_p = NULL;
    qp->qp_ddq_cb.qeq_d3_elt_cnt = 0;

    /* if there are procedure parameters, then let's set up our data structures
    ** to handle them.
    */
    if (global->ops_procedure->pst_parms != NULL)
    {
	opc_dvahd_build(global, global->ops_procedure->pst_parms);
    }
}


/*{
** Name: OPC_CQP_CONTINUE   - Continue to build an entire QEF_QP_CB struct.
**
** Description:
**      Opc_iqp_init(), opc_cqp_continue(), and opc_fqp_finish together build
**	a complete QEP_QP_CB structure based on the info
**      in the OPS_STATE struct that is passed in. Most of the work toward this
**      goal is done by opc_ahd_build() and below. 
**
**	This routine takes the responsibility of having the current pst 
**	statement compiled into one or more actions.
**	
** Inputs:
**	global
**	    A pointer to the OPS_STATE struct for this optimization.
**	global->ops_qsfcb
**	    The QSF rcb that is used to allocate the QEF_QP_CB struct.
**	global->ops_qheader
**	    This is used to find out the updatability of this cursor, repeat
**	    query info, the size of the results, and other related info.
**	    This should only be used if the statement type is "query tree" (QT)
**	global->ops_subquery
**	    The list of subqueries are scanned, and each one is given to 
**	    opc_ahd_build().
**	global->ops_cstate.opc_qp
**	    A pointer to the initialized QEF_QP_CB.
**
** Outputs:
**	global->ops_cstate.opc_qp
**	    A pointer to the QEF_QP_CB that now contains info about the current
**	    statement.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (eric)
**          written
**      10-may-88 (eric)
**          Taken from opc_qp_build() for DB procedures.
**	30-jan-89 (paul)
**	    Support new statements types (CALLPROC and EMESSAGE) for
**	    rule processing.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
**	08-sep-89 (neil)
**	    Alerters: Add recognition of new EVENT actions.
**	12-jun-90 (stec)
**	    Fixed bug 30576. For main subquery, in case of updates or appends
**	    call opc_mtlist() to possibly modify the target list.
**	10-dec-90 (neil)
**	    Alerters: Add recognition of new EVENT actions.
**	14-nov-91 (anitap)
**	    Added brackets to newaud = (QEF_AUD *)mem = (char *)opu_qsfmem
**	    (global, size) as su3 compiler was complaining.
**	    Also cast qp->qp_audit = newaud as su3 compiler was complaining.	
**	20-jul-92 (rickh)
**	    Pointer cast in MEcopy call.
**	28-oct-92 (jhahn)
**	    Added support for statement end rules.
**	12-nov-92 (rickh)
**	    Translate PST_CREATE_TABLE_TYPE, PST_CREATE_INTEGRITY_TYPE,
**	    and PST_DROP_INTEGRITY_TYPE statements into actions.
**	23-nov-92 (anitap)
**	    Added support for CREATE SCHEMA project. Added calls to new 
**	    routines opc_schemaahd_build() and opc_exeimmahd_build().
**	07-dec-92 (teresa)
**	    added support for registered procedures.
**	23-dec-92 (andre)
**	    Translate PST_CREATE_VIEW_TYPE statement into an action
**	10-feb-93 (andre)
**	    do not allocate the ID row when creating a view in STAR since we
**	    will not need to store the id of the view once it is created
**
**	    if creating a view in STAR, set OPC_STARVIEW in opc_flags
**	04-mar-93 (anitap)
**	    Changed function names opc_schemaahd_build() to opc_shemaahd_build()
**	    and opc_exeimmahd_build() to opc_eximmahd_build() to conform to
**	    coding standards.
**	13-mar-93 (andre)
**	    translate PST_DROP_INTEGRITY into an action
**	3-apr-93 (markg)
**	    Added support for new structure of auditing control blocks. 
**	16-sep-93 (robf)
**          Audit rework, auditing for a statement now stays with the
**	    associated action rather  than being coalesced into the QP header.
**	11-sep-98 (inkdo01)
**	    Added check for RQ parms in func attrs contained in multi-att eqclasses
**	    (since they may represent BFs no longer in the BF structures).
**	21-june-00 (inkdo01)
**	    Minor change to prevent SEGV (followon to bug 86478).
**	25-may-06 (dougi)
**	    Add statement numbers to dbp action headers.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	18-dec-2007 (dougi)
**	    Support offset/first "n" parameters.
**	18-mar-2010 (gupsh01) SIR 123444
**	    Added support for alter table ...rename table/column.
**	21-Jun-2010 (kschendel) b123775
**	    Delete unused statement no.
*/
VOID
opc_cqp_continue(
		OPS_STATE          *global)
{
    QEF_AHD	    *new_ahd;
    OPS_SUBQUERY    *sq;
    OPS_SUBQUERY    *next_sq;
    OPS_SUBQUERY    *top_sq;
    i4		    rparms_size;
    i4		    num_boolfact;
    OPB_BOOLFACT    *boolfact;
    OPV_IGVARS	    next_grv_no;
    OPV_GRV	    *grv;
    OPC_PST_STATEMENT	*opc_pst;
    QEF_QP_CB       *qp;
    PST_STATEMENT   *statementp;
    i4		    dummyRow;

    /* AN implicit assumption is made that there are no PST_QT_TYPE	    */
    /* statements in a rule action list. While compiling a rule action list */
    /* the state of the QP is that of the triggering query */

    /* if there is a query tree involved */
    if (global->ops_statement->pst_type == PST_QT_TYPE)
    {   
	/*
	** For the current query locate the main subquery, check its mode
	** and if it is an append or update, traverse the target list to
	** eliminate resdoms that have duplicate pst_rsno. We will do it
	** in such a way so that the last one specified remains in the tree.
	** this will fix bug 30576 where an update query with the set clause
	** naming same column twice was getting an AV in QEF.
	*/
	for (sq = global->ops_subquery; sq != NULL; sq = sq->ops_next)
	{
	    if (sq->ops_sqtype != OPS_MAIN)
		continue;

	    /* we are dealing with main query now. */
	    if (sq->ops_mode == PSQ_APPEND ||
		sq->ops_mode == PSQ_REPLACE )
	    {
		/* pass first resdom */
		opc_mtlist(global, sq->ops_root->pst_left);
	    }
	}
    }

    /* if (there are any user repeat query parameters */
    if (global->ops_statement->pst_type == PST_QT_TYPE &&
	    global->ops_parmtotal > 0 &&
	    global->ops_procedure->pst_isdbp == FALSE
	)
    {   
	if (global->ops_cstate.opc_rparms == NULL)
	{
	    rparms_size = sizeof (*global->ops_cstate.opc_rparms) * 
					    (global->ops_parmtotal + 1);
	    global->ops_cstate.opc_rparms = (OPC_BASE *) opu_memory(global, 
								rparms_size);
	    MEfill(rparms_size, (u_char)0, 
		(PTR)global->ops_cstate.opc_rparms);
	}

	/* for each subquery that is used in the current query */
	top_sq = global->ops_subquery;
	next_grv_no = 0;
	while (top_sq != NULL)
	{
	    /* for each subquery struct that is unioned together */
	    for (next_sq = top_sq; 
		 next_sq != NULL; 
		 next_sq = next_sq->ops_next
		)
	    {
		/* for each subquery struct on the current list */
		for (sq = next_sq;
		     sq != NULL; 
		     sq = sq->ops_union
		    )
		{
		    OPE_IEQCLS	eqc;
		    OPE_EQCLIST	*eqcp;
		    OPZ_IATTS	attno;
		    OPZ_ATTS	*attrp;

		    if (sq->ops_root == NULL)
			continue;

		    /* register the target list repeat query parameters */
		    opc_lsparms(global, sq->ops_root->pst_left);

		    /* register the repeat query parameters that are in
		    ** the constant qualification, if there is one.
		    */
		    if (sq->ops_bfs.opb_bfconstants != NULL)
		    {
			opc_lsparms(global, sq->ops_bfs.opb_bfconstants);
		    }

		    /* For each boolean factor (ie. each qual clause) */
		    for (num_boolfact = 0; 
			 num_boolfact < sq->ops_bfs.opb_bv;
			 num_boolfact += 1
			)
		    {
			boolfact = 
			   sq->ops_bfs.opb_base->opb_boolfact[num_boolfact];

			/* register this boolfact's repeat 
			** query parameters
			*/
			opc_lsparms(global, boolfact->opb_qnode);
		    }
		    /* Look for equijoin eqclasses containing func attrs
		    ** which might contain RQ parms. These wouldn't be found
		    ** elsewhere, since they're removed from the BF list. */
		    for (eqc = 0; eqc < sq->ops_eclass.ope_ev; eqc++)
		    {
			eqcp = sq->ops_eclass.ope_base->ope_eqclist[eqc];
			if (!eqcp || BTcount((char *)&eqcp->ope_attrmap,
				(i4)sq->ops_attrs.opz_av) <= 1) continue;
			for (attno = -1; (attno = (OPZ_IATTS)BTnext((i4)attno,
				(char *)&eqcp->ope_attrmap, 
				(i4)sq->ops_attrs.opz_av)) >= 0; )
			 if ((attrp = sq->ops_attrs.opz_base->opz_attnums[attno])
				->opz_attnm.db_att_id == OPZ_FANUM)
			    opc_lsparms(global, sq->ops_funcs.opz_fbase->opz_fatts[
				attrp->opz_func_att]->opz_function);
		    }

		    /* Check for parameterized first "n"/offset "n", too. */
		    if (global->ops_procedure->pst_isdbp == FALSE)
		    {
			if (global->ops_qheader->pst_offsetn != NULL)
			  opc_lsparms(global, global->ops_qheader->pst_offsetn);
			if (global->ops_qheader->pst_firstn != NULL)
			  opc_lsparms(global, global->ops_qheader->pst_firstn);
		    }
		}
	    }

	    /* lets find the next subquery struct that's on the global
	    ** range var list to continue our repeat query registration
	    ** with.
	    */
	    top_sq = NULL;
	    for (	;
		 next_grv_no < global->ops_rangetab.opv_gv; 
		 next_grv_no += 1
		)
	    {
		grv = global->ops_rangetab.opv_base->opv_grv[next_grv_no];
		if (grv != NULL && grv->opv_gsubselect != NULL)
		{
		    top_sq = grv->opv_gsubselect->opv_subquery;
		    next_grv_no++;
		    break;
		}
	    }
	}
    }

    opc_pst = (OPC_PST_STATEMENT *) 
		    opu_memory(global, sizeof (OPC_PST_STATEMENT));
    opc_pst->opc_lvrow = -1;
    opc_pst->opc_stmtahd = (QEF_AHD *) NULL;
    global->ops_statement->pst_opf = (PTR) opc_pst;
    qp = global->ops_cstate.opc_qp;	

    /* For each subquery, fill in one or more QEF action header(s), making a 
    ** linked list as we go.
    */
    new_ahd = NULL;
    switch (global->ops_statement->pst_type)
    {
     case PST_DV_TYPE:
	/* Compile a DECVAR statement */
	opc_dvahd_build(global, global->ops_statement->pst_specific.pst_dbpvar);
	break;

     case PST_QT_TYPE:
	/* Compile a qtree statement */
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	    /* if session is distributed, call distributed OPC */
	    opc_dentry(global);
	else
	    opc_lsahd_build(global, global->ops_subquery, &new_ahd, TRUE);
	break;

     case PST_IF_TYPE:
	/* Compile an if statement */
	opc_ifahd_build(global, &new_ahd);
	break;

     case PST_FOR_TYPE:
	/* Compile a for statement */
	opc_forahd_build(global, &new_ahd);
	break;

     case PST_MSG_TYPE:
	/* Compile a message statement */
	opc_msgahd_build(global, &new_ahd);
	break;

     case PST_RTN_TYPE:
	/* Compile a return statement */
	opc_rtnahd_build(global, &new_ahd);
	qp->qp_status |= QEQP_CALLPR_RCB;
				/* note need to save RCB in DSH */
	break;

     case PST_CMT_TYPE:
	/* Compile a commit statement */
	opc_cmtahd_build(global, &new_ahd);
	break;

     case PST_RBK_TYPE:
	/* Compile a rollback statement */
	opc_rollahd_build(global, &new_ahd);
	break;

     case PST_RSP_TYPE:
	/* Compile a rollback to savepoint statement */
	opx_error(E_OP0880_NOT_READY);
	break;

     case PST_CP_TYPE:
	/* Compile a call procedure statement */
	if (global->ops_inAfterStatementEndRules)
	    opc_deferred_cpahd_build(global, &new_ahd);
	else
	{
	    opc_cpahd_build(global, &new_ahd);
	    if (global->ops_inAfterRules)
		new_ahd->qhd_obj.qhd_callproc.ahd_flags |= QEF_CP_AFTER;
	    else if (global->ops_inBeforeRules)
		new_ahd->qhd_obj.qhd_callproc.ahd_flags |= QEF_CP_BEFORE;
	}
	qp->qp_status |= QEQP_CALLPR_RCB;
				/* note need to save RCB in DSH */
	break;

     case PST_EMSG_TYPE:
	/* Compile a raise error statement */
	opc_emsgahd_build(global, &new_ahd);
	break;

     case PST_IP_TYPE:
	/* Compile a DBMS internal DBP statement */
	opc_iprocahd_build(global, &new_ahd);
	break;

     case PST_REGPROC_TYPE:
	/* Compile a DBMS internal DBP statement */
	opc_rprocahd_build(global, &new_ahd);
	break;

     case PST_EVREG_TYPE:
     case PST_EVDEREG_TYPE:
     case PST_EVRAISE_TYPE:
	/* Compile an event statement */
	opc_eventahd_build(global, &new_ahd);
	break;

     case PST_CREATE_SCHEMA_TYPE:
	/* Compile a create schema statement */
	opc_shemaahd_build(global, &new_ahd);
	break;

     case PST_EXEC_IMM_TYPE:
	/* Compile an execute immediate statement */
	opc_eximmahd_build(global, &new_ahd);
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;  	

     case PST_CREATE_TABLE_TYPE:
	opc_allocateIDrow( global, &dummyRow );
	opc_createTableAHD( global, 
	    ( QEU_CB * ) global->ops_statement->pst_specific.pst_createTable.
		pst_createTableQEUCB,
	    dummyRow, &new_ahd );
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;

     case PST_CREATE_INTEGRITY_TYPE:
	opc_allocateIDrow( global, &dummyRow );
	opc_createIntegrityAHD( global, global->ops_statement, dummyRow,
		&new_ahd );
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;

     case PST_RENAME_TYPE:
	opc_allocateIDrow( global, &dummyRow );
	opc_renameAHD( global, global->ops_statement, dummyRow, &new_ahd );
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;
    case PST_CREATE_VIEW_TYPE:
	/* do not allocate the ID row for STAR views */
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{
	    global->ops_cstate.opc_flags = OPC_STARVIEW;
	}
	else
	{
	    opc_allocateIDrow( global, &dummyRow );
	}
	    
	opc_crt_view_ahd(global, global->ops_statement, dummyRow, &new_ahd);
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;

     case PST_DROP_INTEGRITY_TYPE:
	opc_d_integ(global, global->ops_statement, &new_ahd);
	qp->qp_status |= QEQP_EXEIMM_RCB;
				/* note need to save RCB in DSH */
	break;

     /* currently, these aren't supported.  barf if you see them */

     case PST_CREATE_INDEX_TYPE:
     case PST_CREATE_RULE_TYPE:
     case PST_CREATE_PROCEDURE_TYPE:
     case PST_MODIFY_TABLE_TYPE:
	opx_error(E_OP08A3_UNKNOWN_STATEMENT);
	break;

     default:
	break;
    }

    opc_pst->opc_stmtahd = new_ahd;
    if (qp->qp_status & QEQP_ISDBP && new_ahd)
	new_ahd->ahd_stmtno = global->ops_statement->pst_lineno;
    else if (new_ahd)
	new_ahd->ahd_stmtno = -1;

    /* Handle the audit information for each statement. */
    statementp = global->ops_statement;

    if (statementp->pst_audit != NULL)
    {
	/* Copy the audit information. */
	QEF_AUD      *qpaud;
	QEF_AUD      *aud, *newaud;
	QEF_ART	     *next_art;
	QEF_APR	     *next_apr;
	char         *mem;
	i4          size;	
	i4          i;

	qpaud = (QEF_AUD *)new_ahd->ahd_audit;
	aud = (QEF_AUD *)statementp->pst_audit;
	size = sizeof(QEF_AUD) + (sizeof(QEF_ART) * aud->qef_cart) 
			       + (sizeof(QEF_APR) * aud->qef_capr);	

	/*  Allocate AUD + ART + APR in QP. */

	newaud = (QEF_AUD *)(mem = (char *)opu_qsfmem(global, size));

	/*  Initialize AUD. */

	MEcopy((char *)aud,sizeof(QEF_AUD),(char *)newaud);
	newaud->qef_next = NULL;
	newaud->qef_art = (QEF_ART *)&newaud[1];
	newaud->qef_apr = (QEF_APR *)&newaud->qef_art[aud->qef_cart];

	/*  Copy the ART's. */

	for (next_art = aud->qef_art, i = 0;
	     i < aud->qef_cart;
	     i++, next_art = next_art->qef_next)
	{
	    MEcopy((PTR) next_art, sizeof(QEF_ART), (PTR) &newaud->qef_art[i]);
	}

	/*  Copy the APR's. */

	mem += sizeof(QEF_AUD) + (sizeof(QEF_ART)*aud->qef_cart);

	for (next_apr = aud->qef_apr, i = 0;
	    i < aud->qef_capr;
	    i++, next_apr = next_apr->qef_next)
	{
	    MEcopy((PTR) next_apr, sizeof(QEF_APR), (PTR) &newaud->qef_apr[i]);
	}

	/* 
	** Queue this information with those of other
	** statements.
	*/

	if (qpaud == NULL)
	    new_ahd->ahd_audit = (PTR)newaud;
	else
	{
	    for (;qpaud->qef_next; qpaud = qpaud->qef_next)
	    {
	    }
	    qpaud->qef_next = newaud;
	}
    } /* end handle audit information */
}

/*{
** Name: OPC_FQP_FINISH	- Finish building an entire, complete QEF_QP_CB struct.
**
** Description:
**      Opc_iqp_init(), opc_cqp_continue(), and opc_fqp_finish together build
**	a complete QEP_QP_CB structure based on the info
**      in the OPS_STATE struct that is passed in. Most of the work toward this
**      goal is done by opc_ahd_build() and below. 
**
**	This routine takes the responsibility of putting the final touches on
**	the QP. 
**
** Inputs:
**	global
**	    A pointer to the OPS_STATE struct for this optimization.
**	global->ops_qsfcb
**	    The QSF rcb that is used to allocate the QEF_QP_CB struct.
**	global->ops_qheader
**	    This is used to find out the updatability of this cursor, repeat
**	    query info, the size of the results, and other related info.
**	    This should only be used if the statement type is "query tree" (QT)
**	global->ops_subquery
**	    The list of subqueries are scanned, and each one is given to 
**	    opc_ahd_build().
**	global->ops_cstate.opc_qp
**	    A pointer to the QEF_QP_CB that has been built up by opc_iqp_init()
**	    and opc_cqp_continue().
**
** Outputs:
**	    A pointer to the complete QEF_QP_CB.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-jul-86 (eric)
**          written
**      10-may-88 (eric)
**          taken from opc_qp_build() for DB procedures.
**	30-jan-89 (paul)
**	    Enhance for rule processing. Finalize is now called once for each
**	    rule action list and once for the user statement.
**	27-jun-89 (neil)
**	    Term 2: Check validity of query mode before inspecting ops_qheader.
**	    The statement compiled may be a random list of statements w/o any
**	    associated query tree.
**	12-jan-90 (neil)
**	    Rules bug: Adjust the rule firing action to the correct AHD.
**	26-nov-90 (stec)
**	    Added initialization of print buffer for trace code.
**	28-dec-90 (stec)
**	    Changed code to reflect fact that print buffer is in OPC_STATE
**	    struct.
**	10-feb-93 (andre)
**	    if processing CREATE VIEW in STAR, initialize
**	    qp->qp_ddq_cb.qeq_d4_total_cnt and qp->qp_ddq_cb.qeq_d5_fixed_cnt to
**	    0
**	27-oct-98 (inkdo01)
**	    Build sqlda for row producing procs, too.
**	10-may-01 (inkdo01)
**	    Support op150 from Star CDB, too.
**	18-july-01 (inkdo01)
**	    Add support for tp op149 to display op150's QP tree only.
**	24-aug-04 (inkdo01)
**	    Update key buffer size.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	20-apr-2007 (dougi)
**	    Flag scrollable cursors in qp_sqlda.
**	30-oct-2007 (dougi)
**	    Add repeat query parameter descriptor array for cached dynamic.
*/
VOID
opc_fqp_finish(
		OPS_STATE          *global)
{
    QEF_QP_CB	    *qp = global->ops_cstate.opc_qp;
    OPC_OBJECT_ID   *obj;
    i4		    row_size;
    DB_STATUS	    status;

    /* First, lets finish the if statements by pointing them at thier
    ** then and else actions, and set the qp_ahd field at the first
    ** ahd. Once that is done, place the user table valid list entry on
    ** the first action.
    */
    /* If we are processing a rule list, finish the rule list and connect   */
    /* it to the triggering action. */
    if (global->ops_inAfterRules || global->ops_inBeforeRules)
    {

	/* Connect all the actions together into the action list used by    */
	/* QEF. */
	if (global->ops_inAfterRules)
	    opc_resolveif(global, 0, global->ops_firstAfterRule,
		(PST_STATEMENT *)NULL);
	else opc_resolveif(global, 0, global->ops_firstBeforeRule,
		(PST_STATEMENT *)NULL);

	/*
	** Set the rule action list of the triggering statement to point to the
	** first action in the rule action list.  Note that some statements
	** create a few AHD's and the first one is not necessarily the one
	** we want to attach to.  For example:
	**	insert into t1 select * from t2 where x in (select x from t3)
	** will create a DMU and PROJ action prior to the APP action.  This
	** is done by saving the triggering action during earlier setup 
	** (opc_rule_ahd) and applying the rules to that.
	*/
	if (global->ops_inAfterRules)
	    global->ops_cstate.opc_rule_ahd->qhd_obj.qhd_qep.ahd_after_act =
		((OPC_PST_STATEMENT *)global->ops_firstAfterRule->pst_opf)->
							opc_stmtahd;
	else global->ops_cstate.opc_rule_ahd->qhd_obj.qhd_qep.ahd_before_act =
		((OPC_PST_STATEMENT *)global->ops_firstBeforeRule->pst_opf)->
							opc_stmtahd;
    }
    else
    {
	if (   global->ops_cb->ops_smask & OPS_MDISTRIBUTED &&
	    ~global->ops_cstate.opc_flags & OPC_STARVIEW)
	{
	    if (global->ops_cstate.opc_flags & OPC_REGPROC)
	    {
		/* call opc_resolve to get the QP_AHD pointer for this regproc*/
		opc_resolveif(global, 0, global->ops_procedure->pst_stmts,
			     (PST_STATEMENT *) NULL);
		qp->qp_ddq_cb.qeq_d4_total_cnt = 0;
		qp->qp_ddq_cb.qeq_d5_fixed_cnt = 0;
	    }
	    else
	    {
		qp->qp_ddq_cb.qeq_d4_total_cnt = global->ops_parmtotal;
		qp->qp_ddq_cb.qeq_d5_fixed_cnt = 
					global->ops_gdist.opd_user_parameters;
	    }
	}
	else
	{
	    qp->qp_ahd = NULL;

	    if (global->ops_cstate.opc_flags & OPC_STARVIEW)
	    {
		qp->qp_ddq_cb.qeq_d4_total_cnt =
		    qp->qp_ddq_cb.qeq_d5_fixed_cnt = 0;
	    }
	    
	    opc_resolveif(global, 0, global->ops_procedure->pst_stmts,
				(PST_STATEMENT *) NULL);

	    /* figure out if this query plan can return at most one tuple. */
	    if (global->ops_procedure->pst_isdbp == FALSE)
	    {
		opc_singleton(global, &qp->qp_status);
	    }

	    /* Now that we have enough information, we can allocate and fill in
	    ** the row and hold buffer arrays.
	    */
	    if (qp->qp_row_cnt > 0)
	    {
		row_size = sizeof (*qp->qp_row_len) * qp->qp_row_cnt;
		qp->qp_row_len = (i4 *) opu_qsfmem(global, row_size);
		obj = global->ops_cstate.opc_rows;
		do
		{
		    qp->qp_row_len[obj->opc_onum] = obj->opc_olen;
		    obj = obj->opc_onext;
		} while (obj != global->ops_cstate.opc_rows);
	    }
	    /* Reset key buffer row size (if needed for global base array). */
	    if (qp->qp_status & QEQP_GLOBAL_BASEARRAY && qp->qp_key_row > 0)
		qp->qp_row_len[qp->qp_key_row] = qp->qp_key_sz;

	    /* Allocate and fill the array of lengths of the QEF_USR_PARAM	    */
	    /* structures used in all the CALLPROC actions within this QP.	    */
	    /* There is one entry for each CALLPROC statement in the QP. */
	    if (qp->qp_cp_cnt > 0)
	    {
		row_size = sizeof (*qp->qp_cp_size) * qp->qp_cp_cnt;
		qp->qp_cp_size = (i4 *) opu_qsfmem(global, row_size);
		obj = global->ops_cstate.opc_cparms;
		do
		{
		    /* Sizes for paramater arrays are in bytes */
		    qp->qp_cp_size[obj->opc_onum] = obj->opc_olen;
		    obj = obj->opc_onext;
		} while (obj != global->ops_cstate.opc_cparms);
	    }
	}

	/* If this is a repeat query, fill in parm information for
	** possible use in resolving repeat cursor parms on later
	** OPENs. */
	if ((qp->qp_status & (QEQP_REPDYN | QEQP_RPT)) && 
		global->ops_qheader != (PST_QTREE *) NULL &&
		global->ops_qheader->pst_numparm > 0)
	{
	    OPC_BASE	*parms = global->ops_cstate.opc_rparms;
	    DB_DATA_VALUE *dvp;
	    i4		i;

	    qp->qp_nparams = global->ops_qheader->pst_numparm;
	    qp->qp_params = (DB_DATA_VALUE **) opu_qsfmem(global,
			global->ops_qheader->pst_numparm * sizeof(PTR));
	    for (i = 1; i <= global->ops_qheader->pst_numparm; i++)
	    {
		dvp = (DB_DATA_VALUE *) opu_qsfmem(global,
						sizeof(DB_DATA_VALUE));
		qp->qp_params[i-1] = dvp;
		STRUCT_ASSIGN_MACRO(parms[i].opc_dv, *dvp);
	    }

	}
	else
	{
	    qp->qp_nparams = 0;
	    qp->qp_params = (DB_DATA_VALUE **) NULL;
	}

	/* fill in the sqlda stuff for SCF */
	/* if (this is a retrieve or a define cursor) */
	if (   global->ops_procedure->pst_isdbp == FALSE
	    && global->ops_qheader != NULL
	    && (   global->ops_qheader->pst_mode == PSQ_RETRIEVE
		|| global->ops_qheader->pst_mode == PSQ_DEFCURS)
	   )
	{
	    status = ulc_bld_descriptor(global->ops_qheader->pst_qtree,
				global->ops_caller_cb->opf_names,
				&global->ops_qsfcb,
				(GCA_TD_DATA **)&qp->qp_sqlda,
#ifdef	OPT_F065_LVCH_TO_VCH
				opt_strace(global->ops_cb,
					    OPT_F065_LVCH_TO_VCH)
#else
				0
#endif
				);
	    /* Is this a scrollable cursor?? If so, flag qp_sqlda. */
	    if (qp->qp_ahd && qp->qp_ahd->ahd_atype == QEA_GET &&
		(qp->qp_ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_SCROLL))
		((GCA_TD_DATA *)qp->qp_sqlda)->gca_result_modifier |= 
						GCA_SCROLL_MASK;

	    if (status != E_DB_OK)
	    {
		opx_verror(status, E_OP0886_QSF_PALLOC, 
					global->ops_qsfcb.qsf_error.err_code);
	    }
	}
	else if (global->ops_procedure->pst_isdbp == TRUE &&
		global->ops_cstate.opc_retrow_rsd != NULL)
	{
	    /* For row producing procedures, must also build output row
	    ** descriptor. */
	    status = ulc_bld_descriptor(global->ops_cstate.opc_retrow_rsd,
				global->ops_caller_cb->opf_names,
				&global->ops_qsfcb,
				(GCA_TD_DATA **)&qp->qp_sqlda,
#ifdef	OPT_F065_LVCH_TO_VCH
				opt_strace(global->ops_cb,
					    OPT_F065_LVCH_TO_VCH)
#else
				0
#endif
				);

	    if (status != E_DB_OK)
	    {
		opx_verror(status, E_OP0886_QSF_PALLOC, 
					global->ops_qsfcb.qsf_error.err_code);
	    }
	}
	else
	{
	    qp->qp_sqlda = NULL;
	}

	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{
	    if (qp->qp_ddq_cb.qeq_d5_fixed_cnt > 0 )
	    {
		/*
		**  Copy user-supplied parameters
		*/

		DB_DATA_VALUE   *src_dbv;
		OPS_RQUERY      *rqptr;
		i4             x;
		i4             nelem;

		nelem = qp->qp_ddq_cb.qeq_d5_fixed_cnt;
		qp->qp_ddq_cb.qeq_d6_fixed_data =
		    (DB_DATA_VALUE *) opu_qsfmem( global,
			( sizeof(DB_DATA_VALUE) * nelem ));
		for (rqptr = global->ops_rqlist;
		     rqptr->ops_rqnode->
			pst_sym.pst_value.pst_s_cnst.pst_parm_no > nelem;
		     rqptr = rqptr->ops_rqnext)
		     ;                      /* skip over the simple aggregate parameters
					    ** in this list */
		for ( x=nelem-1; x >= 0; x--, rqptr = rqptr->ops_rqnext )
		{
		    DB_DATA_VALUE   *target_dbv;
		    if (rqptr->ops_rqnode->
			    pst_sym.pst_value.pst_s_cnst.pst_parm_no != (x+1))
			opx_error(E_OP0A8D_VARIABLE);
						/* check assumption that the list
						** is ordered */
		    /* copy the DB_DATA_VALUE */
		    target_dbv = &qp->qp_ddq_cb.qeq_d6_fixed_data[x];
		    src_dbv = &rqptr->ops_rqnode->pst_sym.pst_dataval;
		    STRUCT_ASSIGN_MACRO( *src_dbv, *target_dbv);

		    /* copy the actual data value */
		    target_dbv->db_data = (PTR) opu_qsfmem( global,
			target_dbv->db_length );
		    MEcopy( src_dbv->db_data, target_dbv->db_length,
			target_dbv->db_data );
		}
	    }
	    else
	    {
		qp->qp_ddq_cb.qeq_d6_fixed_data = (DB_DATA_VALUE *) NULL;
	    }
	}
#ifdef OPT_F022_FULL_QP
	if (opt_strace(global->ops_cb, OPT_F022_FULL_QP) == TRUE ||
	    opt_strace(global->ops_cb, OPT_F021_QPTREE) == TRUE)
	{
	    char	temp[OPT_PBLEN + 1];
	    bool	init = 0;
	    bool	full;

	    if (global->ops_cstate.opc_prbuf == NULL)
	    {
		global->ops_cstate.opc_prbuf = temp;
		init++;
	    }

	    full = opt_strace(global->ops_cb, OPT_F022_FULL_QP);
	    /* print out the qp header */
	    if (full)
		opt_qp(global, qp);
	    else
		opt_qp_brief(global, qp);
	    
	    if (init)
	    {
		global->ops_cstate.opc_prbuf = NULL;
	    }
	}
#endif
    }
}

/*{
** Name: OPC_SINGLETON	- This sets a status field if this QP can return
**					at most tuple.
**
** Description:
**      This routine sets the QEQP_SINGLETON bit in a given status field if
**      the current QP can return at most one tuple.
**
**	If the current QP will return large objects, then ignore the singleton
**	state.  This will result in better performance in most cases since we
**	can block up the data more efficiently.
**
** Inputs:
**	global -
**	    This is the query wide status field. It is used as a general
**	    source of information, and specifically to find the query plan
**	    that we just built. It is expected that the query plan has been
**	    completely built.
**	status -
**	    This is the address of a status field of unknown contents.
**
** Outputs:
**	status -
**	    The QEQP_SINGLETON bit will be ored in with the original contents
**	    if the current QP will return at most one tuple
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
**      11-Jan-87 (eric)
**          written
**      12-Dec-89 (fred)
**	    Added support for large objects.  If the QEQP_LARGE_OBJECTS bit is
**	    on, then do not turn on the singleton bit.
**      19-jun-95 (inkdo01)
**          Changed 2-valued i4  orig_ukey to bit in orig_flag 
[@history_template@]...
*/
static VOID
opc_singleton(
	OPS_STATE	*global,
	i4		*status )
{
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    QEF_AHD		*ahd;
    QEN_NODE		*qen;


    /* find the first action that returns a tuple */
    for (ahd = qp->qp_ahd; ahd != NULL; ahd = ahd->ahd_next)
    {
	if (ahd->ahd_atype == QEA_GET)
	{
	    /* If there is one action that returns tuples */
	    if (ahd->ahd_next == NULL)
	    {
		qen = ahd->qhd_obj.qhd_qep.ahd_qep;

		if (qen == NULL)
		{
		    /* If there isn't any QEN tree then we can return at most
		    ** one row since we can return only constants.
		    */

		    if ((*status & QEQP_LARGE_OBJECTS) == 0)
			*status = (*status) | QEQP_SINGLETON;
		}
		else
		{
		    if (qen->qen_type == QE_TSORT)
		    {
			qen = qen->node_qen.qen_tsort.tsort_out;
		    }

		    /* if the only QEN node is an orig node (except for 
		    ** possible top sort node) that is guaranteed to return 
		    ** only one row, then we've got our singleton.
		    */
		    if (qen->qen_type == QE_ORIG)
		    {
			if ((qen->node_qen.qen_orig.orig_flag & ORIG_UKEY)
				&& ((*status & QEQP_LARGE_OBJECTS) == 0))
			{
			    *status = (*status) | QEQP_SINGLETON;
			}
		    }
		}
	    }

	    break;
	}
    }
}

/*{
** Name: OPU_QSFMEM	- Get memory from QSF
**
** Description:
**      This routine will allocate memory from QSF for the object listed in 
**      'global->ops_qsfcb'. The memory will be aligned and will be at least
**      as big as was requested.
**
** Inputs:
**	global
**	    A pointer to a OPS_STATE struct
**	global->ops_qsfcb
**	    the QSF rcb to get the memory from
**	size
**	    The size of the memory to be allocated
**
** Outputs:
**  none
**
**	Returns:
**	    The address of the allocated memory.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-jul-86 (eric)
**          created
[@history_template@]...
*/
PTR
opu_qsfmem(
		OPS_STATE   *global,
		i4	    size)
{
    DB_STATUS	    ret;

    /* EJLFIX: Put me into OPU */

    global->ops_qsfcb.qsf_sz_piece = size;
    if ((ret = qsf_call(QSO_PALLOC, &global->ops_qsfcb)) != E_DB_OK)
    {
	opx_verror(ret, E_OP0886_QSF_PALLOC, 
					global->ops_qsfcb.qsf_error.err_code);
    }
    MEfill(size, (u_char)0, (PTR)global->ops_qsfcb.qsf_piece);
    return(global->ops_qsfcb.qsf_piece);
}

/*{
** Name: OPC_PTROW	- Put a row on the list for QEF's dsh_row.
**
** Description:
**      OPC maintains a list of rows, control blocks, hold file descriptions, 
**      and possibly others (future expansion) that QEF should allocate when 
**      this plan in executed. QEF expects arrays of descriptions of the 
**      rows, control blocks and hold files, but since the size of the arrays 
**      aren't known in advance, OPC keeps a linked list of them. Opc_ptrow() 
**      adds one more row description to the list. 
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_row_cnt -
**	Number of rows added so far
**  global->ops_cstate.opc_rows -
**	the row descriptions for the rows added so far
**  rowno -
**	Pointer to an uninitialized i4
**  rowsz -
**	The size of the row that is being added.
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_row_cnt -
**	This is increased by one.
**  global->ops_cstate.opc_rows -
**	One row description is added.
**  rowno -
**	The row number is filled in and returned
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
**      3-sept-86 (eric)
**          written
[@history_template@]...
*/
VOID
opc_ptrow(
		OPS_STATE	*global,
		i4		*rowno,
		i4		rowsz)
{
    OPC_OBJECT_ID	*nrow;

    nrow = (OPC_OBJECT_ID *) opu_memory(global, sizeof (OPC_OBJECT_ID));
    nrow->opc_onum = global->ops_cstate.opc_qp->qp_row_cnt;
    nrow->opc_olen = rowsz;

    *rowno = nrow->opc_onum;

    if (global->ops_cstate.opc_rows == NULL)
    {
	nrow->opc_onext = nrow->opc_oprev = 
					global->ops_cstate.opc_rows = nrow;
    }
    else
    {
	nrow->opc_onext = global->ops_cstate.opc_rows;
	nrow->opc_oprev = global->ops_cstate.opc_rows->opc_oprev;
	global->ops_cstate.opc_rows->opc_oprev->opc_onext = nrow;
	global->ops_cstate.opc_rows->opc_oprev = nrow;
    }

    global->ops_cstate.opc_qp->qp_row_cnt++;
}

/*{
** Name: OPC_ALLOCATEIDROW - Allocate a row to hold an 8 byte object id
**
** Description:
**	This routine calls opc_ptrow to allocate an 8 byte row to hold
**	an id.  Typically, these ids are DB_TAB_IDs assigned at query
**	run time.  Allocating this row allows one action to assign an
**	id to an object and for another action to link that id to another
**	object id in an IIDBDEPENDS tuple.
**
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  rowno -
**	Pointer to an uninitialized i4
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_row_cnt -
**	This is increased by one.
**  global->ops_cstate.opc_rows -
**	One row description is added.
**  rowno -
**	The row number is filled in and returned
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
**	19-nov-92 (rickh)
**	    Created.
**
[@history_template@]...
*/
VOID
opc_allocateIDrow(
		OPS_STATE	*global,
		i4		*rowno )
{
    opc_ptrow(global, rowno, sizeof(DB_TAB_ID));
}

/*{
** Name: OPC_PTCPARM	- Put a QEF_USR_PARM on the list of dsh_usr_params
**
** Description:
**      OPC maintains a list of rows, control blocks, hold file descriptions, 
**      and possibly others (future expansion) that QEF should allocate when 
**      this plan in executed. QEF expects arrays of descriptions of the 
**      rows, control blocks and hold files, but since the size of the arrays 
**      aren't known in advance, OPC keeps a linked list of them. opc_ptcparm
**	adds one more parameter array to the list.
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_cp_cnt -
**	Number of parameter arrays added so far
**  global->ops_cstate.opc_cparms -
**	The descriptors for the parameter arrays added thus far.
**  rowno -
**	Pointer to an uninitialized i4
**  rowsz -
**	The size of the row that is being added.
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_cp_cnt -
**	This is increased by one.
**  global->ops_cstate.opc_cparms -
**	One parameter array description is added.
**  rowno -
**	The row number is filled in and returned
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
**	30-jan-89 (paul)
**          written to support rule processing
[@history_template@]...
*/
VOID
opc_ptcparm(
		OPS_STATE	*global,
		i4		*rowno,
		i4		rowsz)
{
    OPC_OBJECT_ID	*nrow;

    nrow = (OPC_OBJECT_ID *) opu_memory(global, sizeof (OPC_OBJECT_ID));
    nrow->opc_onum = global->ops_cstate.opc_qp->qp_cp_cnt;
    nrow->opc_olen = rowsz * sizeof(QEF_USR_PARAM);

    *rowno = nrow->opc_onum;

    if (global->ops_cstate.opc_cparms == NULL)
    {
	nrow->opc_onext = nrow->opc_oprev = 
					global->ops_cstate.opc_cparms = nrow;
    }
    else
    {
	nrow->opc_onext = global->ops_cstate.opc_cparms;
	nrow->opc_oprev = global->ops_cstate.opc_cparms->opc_oprev;
	global->ops_cstate.opc_cparms->opc_oprev->opc_onext = nrow;
	global->ops_cstate.opc_cparms->opc_oprev = nrow;
    }

    global->ops_cstate.opc_qp->qp_cp_cnt++;
}

/*{
** Name: OPC_PTCB	- Put a control block on the list for DSH.
**
** Description:
**      Same as opc_ptrow() only for control blocks 
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_cb_cnt -
**	The count of the cbs added so far
**  cbno -
**	Pointer to an uninitialized i4
**  cbsz -
**	The size of the cb that is being added.
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_cb_cnt -
**	This is increased by one.
**  cbno -
**	The cb number is filled in and returned
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-oct-86 (eric)
**          written
**	02-sep-88 (puree)
**	    remove qp_cb_cnt. Rename the old qp_cb_num to qp_cb_cnt.
[@history_template@]...
*/
VOID
opc_ptcb(
		OPS_STATE	*global,
		i4		*cbno,
		i4		cbsz)
{
    *cbno = global->ops_cstate.opc_qp->qp_cb_cnt;

    global->ops_cstate.opc_qp->qp_cb_cnt++;
}

/*{
** Name: OPC_PTHOLD	- Add a hold file buffer for the DSH.
**
** Description:
**      Same as for opc_ptrow() only for hold file descriptions. 
**      The current implementation of this file is a little stupid (ie. 
**      why bother with the file) but hold info stuff has changed enough 
**      to warrent keeping the routine. 
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_hld_cnt -
**	Number of hold files added so far
**  holdno -
**	Pointer to an uninitialized i4
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_hld_cnt -
**	This is increased by one.
**  holdno -
**	The hold file number is filled in and returned
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-oct-86 (eric)
**          written
**      4-oct-88 (eric)
**          make changes for small sorts and hold files
[@history_template@]...
*/
VOID
opc_pthold(
		OPS_STATE	*global,
		i4		*holdno)
{
    *holdno = global->ops_cstate.opc_qp->qp_hld_cnt;
    global->ops_cstate.opc_qp->qp_hld_cnt++;
}

/*{
** Name: OPC_PTHASH	- Add a hash structure for the DSH.
**
** Description:
**      Same as for opc_ptrow() only for hash structure descriptions.
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_hash_cnt -
**	Number of hash structures added so far
**  holdno -
**	Pointer to an uninitialized i4
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_hash_cnt -
**	This is increased by one.
**  hashno -
**	The hash structure number is filled in and returned
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-feb-99 (inkdo01)
**	    Cloned from opc_pthold for hash join support.
[@history_template@]...
*/
VOID
opc_pthash(
		OPS_STATE	*global,
		i4		*hashno)
{
    *hashno = global->ops_cstate.opc_qp->qp_hash_cnt;
    global->ops_cstate.opc_qp->qp_hash_cnt++;
}

/*{
** Name: OPC_TEMPTABLE	- Add a temporary table to the DSH.
**
** Description:
**	Reserve structures needed for temporary tables.
**
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_tempTableCount -
**	Number of temporary tables so far
**  tempTable -
**	Pointer to an uninitialized QEN_TEMP_TABLE.
**  tupleRow -
**	NULL or pointer to an uninitialized OPC_OBJECT *.
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_tempTableCount -
**	This is increased by one.
**  tempTable -
**	Temp table number and dsh row for the table's tuple assigned.
**  tupleRow -
**	Pointer to OPC_OBJECT_ID describing row buffer for temptable.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-aug-92 (rickh)
**	    Created.
**	12-feb-93 (jhahn)
**	    Added extra parameter for support for statement level rules.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs -
**	    specifically to replace QEN_TEMP_TABLE structs by pointers.
**
[@history_template@]...
*/
VOID
opc_tempTable(
		OPS_STATE	*global,
		QEN_TEMP_TABLE	**tempTable,
		i4		tupleSize,
		OPC_OBJECT_ID	**tupleRow)
{
    *tempTable = (QEN_TEMP_TABLE *)opu_qsfmem(global, sizeof(QEN_TEMP_TABLE));
    (*tempTable)->ttb_tempTableIndex =
	global->ops_cstate.opc_qp->qp_tempTableCount;
    global->ops_cstate.opc_qp->qp_tempTableCount++;

    opc_ptrow( global, &(*tempTable)->ttb_tuple, tupleSize );
    if (tupleRow != NULL)
	*tupleRow = global->ops_cstate.opc_rows->opc_oprev;
    (*tempTable)->ttb_pagesize = DB_MINPAGESIZE;	/* default size */
}

/*{
** Name: OPC_PTSORT	- Add a small sort buffer index for the DSH.
**
** Description:
**      Same as for opc_ptrow() only for small sort buffer descriptions. 
**      The current implementation of this file is a little stupid (ie. 
**      why bother with the file) but I feel compeled to follow with the
**	pattern of the other routines that fill this info in (holds, rows,
**	etc).
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query
**  global->ops_cstate.opc_qp->qp_sort_cnt -
**	Number of sort files added so far
**  sortno -
**	Pointer to an uninitialized i4
**
** Outputs:
**  global->ops_cstate.opc_qp->qp_sort_cnt -
**	This is increased by one.
**  sortno -
**	The hold file number is filled in and returned
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-oct-88 (eric)
**          written
[@history_template@]...
*/
VOID
opc_ptsort(
		OPS_STATE	*global,
		i4		*sortno)
{
    *sortno = global->ops_cstate.opc_qp->qp_sort_cnt;
    global->ops_cstate.opc_qp->qp_sort_cnt++;
}

/*{
** Name: OPC_PTPARM	- Add a repeat query parameter for this QP.
**
** Description:
**      Opc_ptparm() fills in the description of the given repeat query 
**      parameter into the list of parameter descriptions. In addition,  
**      if this isn't a real repeat query parameter, but is a fake one that 
**      OPF has created (to be used in simple aggregates, for example) then
**      a row of the correct size is also added to the dsh row list. 
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query, including any info that opc_ptrow needs.
**  global->ops_cstate.opc_rparms
**	The array for the repeat query parameter descriptions. This must
**	already be allocated and initialized to zeros, and possibly filled
**	in with come parameter info.
**  global->ops_qheader->pst_numparm
**	The number of real repeat query parameters.
**  qconst -
**	Pointer to a constant qtree node for the repeat query parameter
**	that needed to be processed.
**  rindex -
**	A flag telling whether or not a new row should be created if the
**	repeat query parameter be processed is an internal OPF one or not.
**	If rindex is less than zero, then a new row is created. Otherwise,
**	rindex is >= zero, a new row isn't created but instead the row
**	number of rindex is used for the repeat query parameter.
**  roffset -
**	If rindex is >= 0 and we are dealing with an internal OPF repeat
**	query parameter, then roffset indicated where in the rindex row
**	that the parameter value should be located.
**
** Outputs:
**  global
**	any changes that opc_ptrow() makes to global or below.
**  global->ops_cstate.opc_parms -
**	A description for the repeat query parameter is added.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-oct-86 (eric)
**          written
**	20-june-01 (inkdo01)
**	    qconst may be a PST_AOP if called from opcagg.c
[@history_template@]...
*/
VOID
opc_ptparm(
		OPS_STATE	*global,
		PST_QNODE	*qconst,
		i4		rindex,
		i4		roffset)
{
    OPC_BASE	*parms = global->ops_cstate.opc_rparms;
    i4		parmno;
    bool	aop = (qconst->pst_sym.pst_type == PST_AOP);

    if (aop)
	parmno = qconst->pst_sym.pst_value.pst_s_op.pst_opparm;
    else parmno = qconst->pst_sym.pst_value.pst_s_cnst.pst_parm_no;

    if (!aop && qconst->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_RQPARAMNO ||
	    parms[parmno].opc_bvalid == TRUE
	)
    {
	return;
    }

    parms[parmno].opc_bvalid = TRUE;
    STRUCT_ASSIGN_MACRO(qconst->pst_sym.pst_dataval, parms[parmno].opc_dv);

    if (parmno > global->ops_qheader->pst_numparm)
    {
	/* These are OPF generated repeat query paramters. QEF can
	** only handle user generated repeat params, so these need to be
	** mapped into DSH rows.
	*/
	parms[parmno].opc_array = QEN_ROW;
	if (rindex >= 0)
	{
	    parms[parmno].opc_index = rindex;
	    parms[parmno].opc_offset = roffset;
	}
	else
	{
	    opc_ptrow(global, &parms[parmno].opc_index, 
					    parms[parmno].opc_dv.db_length);
	    parms[parmno].opc_offset = 0;
	}
    }
    else
    {
	parms[parmno].opc_array = QEN_PARM;
	parms[parmno].opc_index = parmno;
	parms[parmno].opc_offset = 0;
    }
}

/*{
** Name: OPC_LSPARMS	- Make a list of the repeat query param info
**
** Description:
**      Opc_lsparms fills in a array of repeat query parameter descriptions 
**      for all of the RQP that are used in the given query tree. For each 
**      RQP, opc_ptparm is called. 
[@comment_line@]...
**
** Inputs:
**  global -
**	State info for this query, esp. what is needed by opc_ptparm()
**  global->ops_cstate.opc_rparms
**	The array of repeat query parameter descriptions, which has been
**	allocated, initialized, and optionally partially filled.
**  root -
**	The query tree that may or may not contain any repeat query parameter
**	constant query tree nodes.
**
** Outputs:
**  global->ops_cstate.opc_rparms
**	This now contains descriptions for all of the repeat query parameters
**	that are listed in root.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-DEC-86 (ERIC)
**          written
**      18-jan-87 (eric)
**          changed to set an array of OPC_BASEs from DB_DATA_VALUEs
**      16-oct-92 (ed)
**          - fix bug 40835 - wrong number of duplicates when union views and
**          subselects are used,  FIXME rework repeat query parameter handling
**	    and database procedure handling so that they can co-exist
**	28-jan-04 (wanfr01)
**	    Bug 36414, INGSRV582
**          Avoid an in-clause stack overflow by only recursing right leaf,
**          and using a while loop to go down the left leafs.
[@history_template@]...
*/
static VOID
opc_lsparms(
	OPS_STATE	*global,
	PST_QNODE	*root )
{
    PST_QNODE *curnode = root;

    while (curnode != NULL) 
    {
    
        if (curnode->pst_sym.pst_type == PST_CONST)
        {
	    i4		pno;

	    pno = curnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no;

	    if (curnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_RQPARAMNO &&
		pno > 0 && pno <= global->ops_parmtotal &&
		global->ops_cstate.opc_rparms[pno].opc_bvalid == FALSE
	       )
	    {
	        bool	call_ptparm;
	        call_ptparm = (pno <= global->ops_qheader->pst_numparm); /* TRUE
				** if this is a user/PSF generated
				** repeat query parameter */
	        if (!call_ptparm)
	        {	/* only allow special ii_row_count repeat query
	    	 	** parameter to be processed in this case, otherwise
			** SEjoins break */
		    OPS_RQUERY	*rqueryp;
		    call_ptparm = FALSE;
		    for (rqueryp = global->ops_rqlist; rqueryp;
		         rqueryp = rqueryp->ops_rqnext)
		    {   /* search list of repeat queries for this
		        ** special case */
		        if (pno == curnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
		        {
		   	    call_ptparm = (rqueryp->ops_rqmask & OPS_RQSPECIAL)
			        != 0;
			    break;
		        }
		    }
	        }
	        if (call_ptparm)
	  	    opc_ptparm(global, curnode, -1, -1);
	    }
        }
   
        if (curnode->pst_right != NULL)
        {
	   opc_lsparms(global, curnode->pst_right);
        }
	curnode = curnode->pst_left; 
    } 
}

/*{
** Name: OPC_RESOLVEIF	- Resolve all IF actions by seting the TRUE and FALSE 
**				actions
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
**      20-may-88 (eric)
**          created
**      13-dec-88 (seputis)
**          fixed lint error
**	8-july-96 (inkdo01)
**	    Minor change for statement level rules support.
**	26-oct-98 (inkdo01)
**	    Changes to resolve FOR actions (row producing procedures).
**	26-Sep-2007 (kschendel) SIR 122512
**	    Track FOR-loop nesting depth, which will make qee's life
**	    somewhat easier when it's time for hash reservations.
**	    Remove unused "parent" parameter.
**	30-oct-2007 (dougi)
**	    Set "first n"/"offset n" values in FOR action header.
**	16-Jul-08 (kibro01) b120598
**	    In the case of a complex query which requires additional actions
**	    to perform, the repeatable FOR loop may not be the front of the
**	    forhead list.  It is necessary to shuffle the QEA_FOR action to
**	    be directly prior to the QEA_GET action to that prevfor gets sets
**	    correctly during the execution phase, and the move the IF TRUE
**	    branch of the associated IF to skip the initialisation phases.
**      03-Aug-09 (coomi01) b122392
**          The body of an if stmt may be emptied if constant folding detects
**          a fixed 'false' in the boolean condition. We need to defend against
**          the empty adh that results.
*/
static VOID
opc_resolveif(
	OPS_STATE	*global,
	i4		for_depth,
	PST_STATEMENT	*cur_stmt,
	PST_STATEMENT	*last_stmt )
{
    QEF_QP_CB		    *qp = global->ops_cstate.opc_qp;
    OPC_PST_STATEMENT	    *opc_pst;
    QEF_AHD		    *next_ahd;
    QEF_AHD		    *cur_ahd;
    bool		    in_for = FALSE;
    bool		    need_to_move_for = FALSE;

    if (cur_stmt == NULL)
    {
	return;
    }

    /* find the current ahd */
    opc_pst = (OPC_PST_STATEMENT *) cur_stmt->pst_opf;
    cur_ahd  = opc_pst->opc_stmtahd;

    /* while there are more statements to process, and we have not reached the
    ** last statement that we were told to process, and we have not processed
    ** the current action yet
    */

    while (cur_stmt != NULL && cur_stmt != last_stmt && 
		    (cur_ahd == NULL || cur_ahd->ahd_prev != NULL)
	  )
    {
	/* Find the first ahd for the next statement 
	**
	** Bug 122392, but be careful, the statement may have been expunged
	** through constant folding detecting a redundant block.
	*/
	if ((cur_stmt->pst_next == NULL) || (cur_stmt->pst_next->pst_opf == NULL))
	{
	    next_ahd = NULL;
	}
	else
	{
	    opc_pst = (OPC_PST_STATEMENT *) cur_stmt->pst_next->pst_opf;
	    next_ahd = opc_pst->opc_stmtahd;
	}

	/* FOR statement resolution: change pst_next to "for"s pst_forhead,
	** then get "for"s action header to address next action (select GET)
	** qep. Finally, set AHD_FORGET in "get" ahd_qepflag to prevent 
	** re-init'ing query tree on each "fetch".
	*/
	if (cur_stmt->pst_type == PST_FOR_TYPE)
	{
            QEF_AHD *scan_ahd;
            cur_stmt->pst_next = cur_stmt->pst_specific.pst_for.pst_forhead;
            opc_pst = (OPC_PST_STATEMENT *) cur_stmt->pst_next->pst_opf;
            next_ahd = opc_pst->opc_stmtahd;    /* reset these guys next */

            scan_ahd = next_ahd;
            while (scan_ahd->ahd_atype != QEA_GET)
	    {
                scan_ahd = scan_ahd->ahd_next;
		need_to_move_for = TRUE;
	    }
            cur_ahd->qhd_obj.qhd_qep.ahd_qep = scan_ahd->qhd_obj.qhd_qep.ahd_qep;
            scan_ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_FORGET;
	    if (++for_depth > qp->qp_for_depth)
		qp->qp_for_depth = for_depth;
	    /* We'll see a GET, then an IF with the loop body in the if-true
	    ** part.  After that IF, we're out of the for-loop.
	    */
	    in_for = TRUE;
	}

	if (cur_stmt->pst_type != PST_DV_TYPE && cur_ahd != NULL)
	{
	    /* Point the next ahd point of the last ahd for this statement to
	    ** the next ahd, and the prev pointer of the first ahd to the
	    ** last ahd of the previous statment.
	    */
	    cur_ahd->ahd_prev->ahd_next = next_ahd;
	    cur_ahd->ahd_prev = NULL;
	    if (qp->qp_ahd == NULL)
	    {
		qp->qp_ahd = cur_ahd;
	    }

	    /* If this is an ordinary qep-node action, record the FOR
	    ** nesting level for qee reservations.  (Don't need to do this
	    ** for rule actions, so checking nodeact flag suffices.)
	    */
	    if (cur_ahd->ahd_flags & QEA_NODEACT)
		cur_ahd->qhd_obj.qhd_qep.ahd_for_depth = for_depth;

	    /* If this is an if statement then recursivly do the same on
	    ** the True and False statements
	    */
	    if (cur_stmt->pst_type == PST_IF_TYPE)
	    {
		if (cur_stmt->pst_specific.pst_if.pst_true != NULL)
		{
		    opc_pst = (OPC_PST_STATEMENT *)
				cur_stmt->pst_specific.pst_if.pst_true->pst_opf;
		    cur_ahd->qhd_obj.qhd_if.ahd_true = opc_pst->opc_stmtahd;
		    opc_resolveif(global, for_depth,
				    cur_stmt->pst_specific.pst_if.pst_true,
				    cur_stmt->pst_next);
		}

		if (cur_stmt->pst_specific.pst_if.pst_false != NULL)
		{
		    opc_pst = (OPC_PST_STATEMENT *)
			    cur_stmt->pst_specific.pst_if.pst_false->pst_opf;
		    cur_ahd->qhd_obj.qhd_if.ahd_false = opc_pst->opc_stmtahd;
		    opc_resolveif(global, for_depth,
				    cur_stmt->pst_specific.pst_if.pst_false,
				    cur_stmt->pst_next);
		}
		/* if this IF was really a for-loop body, we're now done
		** with the FOR loop, reduce nesting.
		*/
		if (in_for)
		{
		    in_for = FALSE;
		    -- for_depth;
		}
	    }
	}

	/* Now lets prepare for the next loop. We point prev_ahd to the
	** last ahd of the current statement (what we remembered), and we
	** point cur_ahd to the first ahd of the next statement.
	*/
	cur_ahd = next_ahd;

	/* Do the likewise for the statment pointers. */
	cur_stmt = cur_stmt->pst_next;
    }

    /* We have a FOR loop with additional preliminary actions, so we have
    ** to do some clean up before we can execute the actions - moving the
    ** QEA_FOR to preced the QEA_GET, and ensuring that the IF loop does
    ** not include the initialisation actions (kibro01) b120598
    ** e.g.
    ** For a complex subquery you might have
    **    QEA_FOR -> QEA_DMU -> QEA_xxx -> QEA_GET -> QEA_IF -false-> ...
    **                 ^                                   -true-V
    **                 ^-----------------------------------------<
    **
    ** This needs to be amended to be
    **    QEA_DMU -> QEA_xxx -> QEA_FOR -> QEA_GET -> QEA_IF -false-> ...
    **                                        ^            -true-V
    **                                        ^------------------<
    */

    if (need_to_move_for)
    {
        QEF_AHD		    **prev_ahd_ptr;
        QEF_AHD		    *if_seek, *if_seek2;

	prev_ahd_ptr = &qp->qp_ahd;
	cur_ahd  = qp->qp_ahd;
	while (cur_ahd)
	{
	    if (cur_ahd->ahd_atype == QEA_FOR)
	    {
		if_seek = cur_ahd->ahd_next;
		while (if_seek && if_seek != cur_ahd &&
			if_seek->ahd_atype != QEA_IF)
		{
			if_seek = if_seek->ahd_next;
		}

		while (cur_ahd->ahd_next && cur_ahd != opc_pst->opc_stmtahd &&
		       cur_ahd->ahd_next->ahd_atype != QEA_GET)
		{
		    /* Found a FOR not attached to a GET, so move it */
		    next_ahd = cur_ahd->ahd_next;
		    /* ...but first move the IF TRUE branch to avoid it */
		    if (if_seek)
		    {
			if_seek2 = if_seek->qhd_obj.qhd_if.ahd_true;
			while (if_seek2 && 
				if_seek2->ahd_next != next_ahd &&
				if_seek2 != if_seek)
			{
			    if_seek2 = if_seek2->ahd_next;
			}
			if (if_seek2->ahd_next == next_ahd)
			{
			    if_seek2->ahd_next = if_seek2->ahd_next->ahd_next;
			}
		    }
		    *prev_ahd_ptr = next_ahd;
		    prev_ahd_ptr = &next_ahd->ahd_next;
		    cur_ahd->ahd_next = next_ahd->ahd_next;
		    next_ahd->ahd_next = cur_ahd;
		}
	    }
	    prev_ahd_ptr = &cur_ahd->ahd_next;
	    cur_ahd = cur_ahd->ahd_next;
	}
    }
}

/*{
** Name: OPC_MTLIST 	- Remove duplicate resdom nodes.
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
**	    Modifies target list by removing resdom nodes with duplicate
**	    pst_rsno.
**
** History:
**      12-jun-90 (stec)
**          created
[@history_template@]...
*/
static VOID
opc_mtlist(
	OPS_STATE   *global,
	PST_QNODE   *root )
{
    PST_QNODE	*rsdm, *prev_rsdm;
    char	mp[DB_MAX_COLS/BITSPERBYTE + 1];
    bool	found;

    /* initialize bit map of resdom numbers. */
    MEfill(sizeof(mp), '\0', (PTR)mp);

    for (prev_rsdm = rsdm = root;
	 (rsdm != NULL) && (rsdm->pst_sym.pst_type != PST_TREE);
	 prev_rsdm = rsdm, rsdm = rsdm->pst_left
	)
    {
	found = BTtest((i4)rsdm->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
	    (char *)mp);

	if (found)
	{
	    /* remove the current resdom from tree */
	    prev_rsdm->pst_left = rsdm->pst_left;

	    /* in order to process the new current
	    ** that has not been checked yet.
	    */
	    rsdm = prev_rsdm;
	}
	else
	{
	    /* set bit for this resdom number */
	    BTset((i4)rsdm->pst_sym.pst_value.pst_s_rsdm.pst_rsno, (char *)mp);
	}
    }

    return;
}


/*{
** Name: OPC_RESDOM_MODIFIER 	- Possibly modify LOB or LOCATOR RESDOM.
**
** Description:
**      SCF set opf_locator based on value of (gca_query_modifier & GCA_LOCATOR_MASK)
**      This flag indicates if LOB or LOCATOR data should be returned.
**      Modify RESDOM node according to value of opf_locator.
**
** Inputs:
**	global
**	    A pointer to the OPS_STATE struct for this optimization.
**      resdom
**          A pointer to a RESDOM node
**
** Outputs:
**	none
**
** Side Effects:
**      Possibly modifies the RESDOM node, and recalculates the datatype bits 
**
** History:
**      06-nov-2006 (stial01)
**          created.
*/
static VOID 
opc_resdom_modifier(
OPS_STATE	*global,
PST_QNODE	*resdom,
i4		*bits)
{
    DB_DATA_VALUE	*rdv = &resdom->pst_sym.pst_dataval;
    i2			orig_datatype = rdv->db_datatype;
    DB_STATUS		status;
	
    if (global->ops_caller_cb->opf_locator && (*bits & AD_LOCATOR))
	return;
    else if (!global->ops_caller_cb->opf_locator && (*bits & AD_PERIPHERAL))
	return;
    
    switch (abs(orig_datatype))
    {
	case	DB_LNVCHR_TYPE:
		rdv->db_datatype = DB_LNLOC_TYPE; 
		rdv->db_length = ADP_LOC_PERIPH_SIZE;
		break;
	case	DB_LBYTE_TYPE:
		rdv->db_datatype = DB_LBLOC_TYPE;
		rdv->db_length = ADP_LOC_PERIPH_SIZE;
		break;
	case	DB_LVCH_TYPE:
		rdv->db_datatype = DB_LCLOC_TYPE;
		rdv->db_length = ADP_LOC_PERIPH_SIZE;
		break;
	case	DB_LNLOC_TYPE:
		rdv->db_datatype = DB_LNVCHR_TYPE;
		rdv->db_length = sizeof(ADP_PERIPHERAL);
		break;
	case	DB_LBLOC_TYPE:
		rdv->db_datatype = DB_LBYTE_TYPE;
		rdv->db_length = sizeof(ADP_PERIPHERAL);
		break;
	case	DB_LCLOC_TYPE:
		rdv->db_datatype = DB_LVCH_TYPE;
		rdv->db_length = sizeof(ADP_PERIPHERAL);
		break;


	default: /* FIX ME should we be able to make locator for long udts? */
		break;
    }

    if (rdv->db_datatype == abs(orig_datatype))
	return;

    if (orig_datatype < 0)
    {
	rdv->db_datatype = -rdv->db_datatype;
	rdv->db_length += 1;
    }

    status = adi_dtinfo(global->ops_adfcb, rdv->db_datatype, bits);

    return;
}
