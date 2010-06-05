/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <gca.h>
#include    <cut.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <adfops.h>
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
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <qefexch.h>
#include    <bt.h>
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
#define        OPC_SORTS	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCEXCH.C - Routines to create QEN_EXCH nodes.
**
**  Description:
**      This file holds all of the routines that are specific to creating 
**      exchange nodes to parallelize a query plan. Routines from files are 
**      used that include: OPCQEN.C, OPCTUPLE.C, plus files 
**      from other facilies and the rest of OPF. 
[@comment_line@]...
**
**	Externally visable routines:
**          opc_exch_build() - Build a QEN_EXCH node
[@func_list@]...
**
**	Internal only routines:
**          opc_eexatts() - Build atts from eqcs to create for exchange.
[@func_list@]...
**
**  History:
**      13-nov-03 (inkdo01)
**	    Written for parallel query processing (cloned from opcsorts.c).
**	9-feb-04 (toumi01)
**	    Add cbs context for exchange parent persistent context block.
**	13-may-04 (toumi01)
**	    Add exch_tot_cnt for bit map reading in QEF.
**	    Fix "stat count" for bit map construction.
**	14-may-04 (toumi01)
**	    For partitioned query orig nodes add a DSH bit map entry for the
**	    n-1 cbs DMR_CB entry.
**	17-may-04 (toumi01)
**	    Replace ORIG-only logic for bit map setting for master DMR_CB
**	    with Doug's version that also handles TJOIN and KJOIN.
**	    For parallel processing DSH copying we always need the temp
**	    result row buffer, so always set the bit for OPC_CTEMP.
**	19-may-04 (inkdo01)
**	    Add QEN_TKEY structures from dsh_cbs to bit map.
**	2-Jun-2009 (kschendel) b122118
**	    Remove unused gcount parm from pushcount.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**	19-May-2010 (kschendel) b123565
**	    oj_ijFlagsFile is a hold, not a row, fix in a couple places.
**	19-May-2010 (kschendel) b123759
**	    Include (new style) partition qualification data in exch counts.
**	    DSH rows can pop up multiple times, especially in pquals, use
**	    a bitmap to avoid listing a row more than once (would leak
**	    memory in QEF, at least for 1:N exchanges).
**/

/*
**  Forward and/or External function references.
*/

/* Indexes to counter array. */
#define	IX_ROW	0
#define	IX_HSH	1
#define	IX_HSHA	2
#define	IX_STAT	3
#define IX_TTAB	4
#define	IX_HLD	5
#define	IX_SHD	6
#define IX_PQUAL 7
#define	IX_CX	8
#define	IX_DMR	9
#define	IX_DMT	10
#define	IX_DMH	11
#define	IX_MAX	12	/* # of indexes, not max index */

static VOID
opc_eexatts(
	OPS_STATE	*global,
	OPE_BMEQCLS	*eeqcmp,
	OPC_EQ		*ceq,
	DMF_ATTR_ENTRY	***patts,    /* ptr to a ptr to an array of ptrs to atts */
	i4		*pacount );

static VOID
opc_pushcount(
	QEN_NODE	*node,
	i4		tcount);

static VOID
opc_exunion_arrcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	QEF_AHD		**action,
	i4		*arrcnts,
	PTR		rowmap);

static VOID
opc_exnodearrcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i4		*arrcnts,
	PTR		rowmap);

static VOID
opc_exnodearrset(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts);

static VOID
opc_exnheadcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i4		*arrcnts,
	PTR		rowmap);

static VOID
opc_exnheadset(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts);

static VOID
opc_exactarrcnt(
	OPS_STATE	*global,
	QEF_AHD		*action,
	i4		*arrcnts,
	PTR		rowmap);

static VOID
opc_exactarrset(
	OPS_STATE	*global,
	QEF_AHD		*action,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts);


/*{
** Name: OPC_EXCH_BUILD	- Build a QEN_EXCH node.
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
**      13-nov-03 (inkdo01)
**	    Written for parallel query processing (cloned from opcsorts.c).
**	1-mar-04 (inkdo01)
**	    Allocate and format bit map to identify DSH-based structures
**	    to allocate with child DSHs.
**	21-july-04 (inkdo01)
**	    Change to use index arrays, rather than bit map.
**	6-aug-04 (inkdo01)
**	    Change setting of || union to opo_punion.
**	31-dec-04 (inkdo01)
**	    Added parm to opc_materialize to assure there's always exch_mat 
**	    code (fixes 113694).
**	27-Aug-2007 (kschendel) SIR 122512
**	    Count subplan headers, info used by hashop reservation.
**	14-May-2010 (kschendel) b123565
**	    Node nthreads now defaults to zero, push counts even for 1:1
**	    exchange so that QEF knows whether a given node is under an
**	    exchange or not.
**	19-May-2010 (kschendel) b123759
**	    Include (new style) partition qualification data in exch counts.
**	    DSH rows can pop up multiple times, especially in pquals, use
**	    a bitmap to avoid listing a row more than once (would leak
**	    memory in QEF, at least for 1:N exchanges).
*/
VOID
opc_exch_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_EXCH		*ex = &cnode->opc_qennode->node_qen.qen_exch;
    QEN_QP		*qpp;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    OPC_NODE		outer_cnode;
    QEF_AHD		*union_action = NULL;
    PTR			rowmap;		/* DSH row tracker */
    i4			arr1size, arr2size, i, j, ucount;
    i4			arrcnts[IX_MAX];
    i4			mapbits, mapbytes;
    bool		weset_exchn = FALSE;

    /* compile the outer tree */
    STRUCT_ASSIGN_MACRO(*cnode, outer_cnode);

    if (!(global->ops_cstate.opc_flags & OPC_EXCH_N) &&
	co->opo_ccount > 1)
    {
	weset_exchn = TRUE;
	global->ops_cstate.opc_flags |= OPC_EXCH_N;
    }
    qp->qp_status |= QEQP_PARALLEL_PLAN;
    outer_cnode.opc_cco = co->opo_outer;
    outer_cnode.opc_invent_sort = FALSE;
    opc_qentree_build(global, &outer_cnode);
    ex->exch_out = outer_cnode.opc_qennode;
    ex->exch_ccount = co->opo_ccount;
    ex->exch_flag = 0;
    cnode->opc_below_rows += outer_cnode.opc_below_rows;
    if (global->ops_cstate.opc_flags & OPC_EXCH_N)
	ex->exch_flag |= EXCH_NCHILD;
    if (weset_exchn)
	global->ops_cstate.opc_flags &= ~OPC_EXCH_N;
    if (co->opo_punion)
    {
	ex->exch_flag |= EXCH_UNION;
	ex->exch_out->node_qen.qen_qp.qp_flag |= QP_PARALLEL_UNION;
	ucount = ex->exch_ccount;
    }
    else ucount = 1;

    /* Count up subplan headers below here, normally 1, but N if this is
    ** a parallel union.  This is purely to help out join reservation,
    ** so don't bother if child is just an orig.
    */
    if (ex->exch_out->qen_type != QE_ORIG)
	qp->qp_pqhead_cnt += ucount;

    /* Post exch thread-counts to the sub-plan underneath this exch.
    ** (even if it's a 1:1 exch).  This tells everyone that the sub-plan
    ** will be executed in a child thread.  In addition, for the 1:n
    ** case, the ORIG and partition-compatible joins need to know N.
    */
    opc_pushcount(ex->exch_out, ex->exch_ccount);

    /* materialize the inner tuple */
    opc_materialize(global, cnode, &ex->exch_mat, 
	&co->opo_maps->opo_eqcmap, ceq,
	&qn->qen_row, &qn->qen_rsz, (i4)FALSE, (bool)TRUE, (bool)TRUE);

    /* only drop dups if explicitly told to do so. This also disables the 
    ** "only sort on key columns" change (because of odd cases where it seems
    ** to rely on the additional sort keys in its dups dropping strategies),
    ** so it's to our advantage to limit the dups dropping cases. */
    if ((subqry->ops_duplicates == OPS_DKEEP || subqry->ops_duplicates == 
		OPS_DUNDEFINED) && co->opo_odups == OPO_DDEFAULT
	)
    {
	ex->exch_dups = 0;
    }
    else if (co->opo_odups == OPO_DREMOVE ||
		subqry->ops_duplicates == OPS_DREMOVE
	    )
    {
	ex->exch_dups = DMR_NODUPLICATES;
    }
    else
    {
	/* error */
    }

    /* CB number for exchange context CB */
    opc_ptcb(global, &ex->exch_exchcb, sizeof(EXCH_CB));

    /* Fill in the attributes to place in the exchange buffer. */
    opc_eexatts(global, &co->opo_maps->opo_eqcmap, ceq, 
	&ex->exch_atts, &ex->exch_acount);

    /* Fill in the atts to merge on. */
    ex->exch_macount = 0;
    ex->exch_matts = (DMT_KEY_ENTRY **) NULL;

    /* To avoid double-counting a row, which is especially easy if PQUAL's
    ** are around, use a bitmap to track rows.  There usually aren't a
    ** huge number of rows, even in complex query plans.
    ** Double-counting a row is nonfatal, but it leaks memory for unused
    ** row buffers in QEF for 1:N plans.
    */
    mapbits = global->ops_cstate.opc_qp->qp_row_cnt;
    mapbytes = (mapbits + 7) / 8;
    rowmap = opu_Gsmemory_get(global, mapbytes);

    /* Build buffer/structure index arrays for each child (if || union,
    ** one set of arrays per child, else just one set of arrays). */
    for (i = 0; i < ucount; i++)
    {
	/* Allocate arrays to identify buffers and structures to attach
	** to child DSHs resulting from this EXCH node. First build vector
	** of subarray counts for each class of buffers/structs. */
	for (j = 0; j < IX_MAX; j++)
	    arrcnts[j] = 0;		/* init count array */
	MEfill(mapbytes, 0, rowmap);	/* and row tracker */

	/* Set subarray counts. */
	opc_exnheadcnt(global, qn, arrcnts, rowmap); /* count for EXCH node hdr */
	arrcnts[IX_CX]++;		/* + 1 for the exch_mat CX */
	BTset(OPC_CTEMP, rowmap);	/* + the temp row under exch */

	if (ex->exch_flag & EXCH_UNION)
	    opc_exunion_arrcnt(global, qn, &union_action, arrcnts, rowmap);
					/* counts for curr union select */
	else
	    opc_exnodearrcnt(global, qn->node_qen.qen_exch.exch_out, arrcnts, rowmap);
					/* counts everything under exch */

	/* Determine total size of i2 and i4 arrays and alloc memory. */
	arrcnts[IX_ROW] = BTcount(rowmap, mapbits);
	for (j = 0, arr1size = 0; j < IX_CX; j++)
	    arr1size += arrcnts[j];
	for (j = IX_CX, arr2size = 0; j < IX_MAX; j++)
	    arr2size += arrcnts[j];
	ex->exch_ixes[i].exch_array1 = (i2 *)opu_qsfmem(global, arr1size *
				sizeof(i2));
	ex->exch_ixes[i].exch_array2 = (i4 *)opu_qsfmem(global, arr2size *
				sizeof(i4));

	/* Set counts into EXCH. */
	ex->exch_ixes[i].exch_row_cnt = arrcnts[IX_ROW];
	ex->exch_ixes[i].exch_hsh_cnt = arrcnts[IX_HSH];
	ex->exch_ixes[i].exch_hsha_cnt = arrcnts[IX_HSHA];
	ex->exch_ixes[i].exch_stat_cnt = arrcnts[IX_STAT];
	ex->exch_ixes[i].exch_ttab_cnt = arrcnts[IX_TTAB];
	ex->exch_ixes[i].exch_hld_cnt = arrcnts[IX_HLD];
	ex->exch_ixes[i].exch_shd_cnt = arrcnts[IX_SHD];
	ex->exch_ixes[i].exch_pqual_cnt = arrcnts[IX_PQUAL];
	ex->exch_ixes[i].exch_cx_cnt = arrcnts[IX_CX];
	ex->exch_ixes[i].exch_dmr_cnt = arrcnts[IX_DMR];
	ex->exch_ixes[i].exch_dmt_cnt = arrcnts[IX_DMT];
	ex->exch_ixes[i].exch_dmh_cnt = arrcnts[IX_DMH];

	/* Determine start of each subarray. */
	for (j = IX_MAX; j > IX_CX; j--)
	{
	    arr2size -= arrcnts[j-1];
	    arrcnts[j-1] = arr2size;
	}
	for (j = IX_CX; j > 0; j--)
	{
	    arr1size -= arrcnts[j-1];
	    arrcnts[j-1] = arr1size;
	}

	/* Load subarrays - note EXCH nodes stop array loading, so this
	** EXCH node is loaded here, not in opc_ex...arrset(). Likewise,
	** the QP node on top of a || union is loaded here. The subtrees
	** below the EXCH and QP nodes are handled by the opc_ex...arrset()
	** functions.  The row indexes are listed via the row bitmap.
	*/

	j = BTnext(-1, rowmap, mapbits);
	while (j != -1)
	{
	    ex->exch_ixes[i].exch_array1[arrcnts[IX_ROW]++] = j;
	    j = BTnext(j, rowmap, mapbits);
	}
	opc_exnheadset(global, qn, ex->exch_ixes[i].exch_array1,
		ex->exch_ixes[i].exch_array2, arrcnts);
					/* for EXCH node header */
	ex->exch_ixes[i].exch_array2[arrcnts[IX_CX]++] = ex->exch_mat->
			qen_pos;	/* exch_mat CX */
	
	if (ex->exch_flag & EXCH_UNION)
	{
	    qpp = &ex->exch_out->node_qen.qen_qp;
	    opc_exnheadset(global, ex->exch_out, ex->exch_ixes[i].exch_array1,
		ex->exch_ixes[i].exch_array2, arrcnts);
					/* for QP node header */
	    if (qpp->qp_qual != (QEN_ADF *) NULL)
		ex->exch_ixes[i].exch_array2[arrcnts[IX_CX]++] = 
			qpp->qp_qual->qen_pos;
	    opc_exactarrset(global, union_action, 
		ex->exch_ixes[i].exch_array1,
		ex->exch_ixes[i].exch_array2, arrcnts);
	}
	else opc_exnodearrset(global, qn->node_qen.qen_exch.exch_out,
		ex->exch_ixes[i].exch_array1,
		ex->exch_ixes[i].exch_array2, arrcnts);
    }

}

/*{
** Name: OPC_EEXCATTS	- Build atts from eqcs that go to exchange
**			buffer
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
**      13-nov-03 (inkdo01)
**	    Written for parallel query processing (cloned from opcsorts.c).
**	7-Apr-2004 (schka24)
**	    Set attr default stuff just in case someone looks at it.
[@history_template@]...
*/
static VOID
opc_eexatts(
	OPS_STATE	*global,
	OPE_BMEQCLS	*eeqcmp,
	OPC_EQ		*ceq,
	DMF_ATTR_ENTRY	***patts,    /* ptr to a ptr to an array of ptrs 
					to atts */
	i4		*pacount )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DMF_ATTR_ENTRY	**atts;
    DMF_ATTR_ENTRY	*att;
    i4			attno;
    OPZ_AT		*at = global->ops_cstate.opc_subqry->ops_attrs.opz_base;
    OPE_IEQCLS		eqcno;

    *pacount = BTcount((char *)eeqcmp, (i4)subqry->ops_eclass.ope_ev);
    if (*pacount == 0)
	return;		/* count(*) or sommat - no atts to materialize */
    atts = (DMF_ATTR_ENTRY **) opu_qsfmem(global, 
	*pacount * sizeof (DMF_ATTR_ENTRY *));

    for (attno = 0, eqcno = -1;
	    (eqcno = BTnext((i4)eqcno, (char *)eeqcmp, 
		(i4)subqry->ops_eclass.ope_ev)) != -1;
	    attno += 1
	)
    {
	att = (DMF_ATTR_ENTRY *) opu_qsfmem(global, sizeof (DMF_ATTR_ENTRY));

	STprintf(att->attr_name.db_att_name, "a%x", eqcno);
	STmove(att->attr_name.db_att_name, ' ', 
		sizeof(att->attr_name.db_att_name), att->attr_name.db_att_name);
	att->attr_type = ceq[eqcno].opc_eqcdv.db_datatype;
	att->attr_size = ceq[eqcno].opc_eqcdv.db_length;
	att->attr_precision = ceq[eqcno].opc_eqcdv.db_prec;
	att->attr_flags_mask = 0;
	SET_CANON_DEF_ID(att->attr_defaultID, DB_DEF_NOT_DEFAULT);
	att->attr_defaultTuple = NULL;

	atts[attno] = att;
    }

    *patts = atts;
}


/*{
** Name: OPC_PUSHCOUNT	- post child partition group/thread counts to
**	ORIG nodes
**
** Description: Descend query tree fragment under an exchange node,
**	posting partition group/groups per thread counts into join
**	and ORIG nodes below.
**
**	1:n exchanges occur in three situations:
**	- over a partitioned ORIG, which is a fairly trivial case;
**	- over a PC-join fragment, with no exch's in the fragment;
**	  only PC-joinable nodes will occur, in particular we won't
**	  see an SE-joins or QP nodes;
**	- over a union QP, where each action of the sub-plan is
**	  run in a different thread.  As we descend past the QP node
**	  (which will be tagged with the parallel union flag), each
**	  action under the QP is executed by ONE thread, so the count
**	  becomes 1.
**
**	Even a 1:1 exch posts counts;  a nonzero count indicates that
**	the node will be executed in a child thread.  This is important
**	in some cases (e.g. to suppress AGG/ORIG -> ORIGAGG transformation.)
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      24-apr-04 (inkdo01)
**	    Written for || query processing.
**	27-may-05 (inkdo01)
**	    Update qen_nthreads for each node.
**	14-May-2010 (kschendel) b123565
**	    Push all the way down, and even for 1:1 exchanges.  The default
**	    node nthreads is now zero.
*/
static VOID
opc_pushcount(
	QEN_NODE	*node,
	i4		tcount)

{
    QEF_AHD *act;

    /* Update count of threads node executes under and then
    **  recursively descend the tree in search of ORIG nodes. */
    if (node->qen_nthreads == 0)
	node->qen_nthreads = 1;		/* node now known to be under EXCH */
    node->qen_nthreads *= tcount;

    switch(node->qen_type) {
	case QE_SORT:
	    opc_pushcount(node->node_qen.qen_sort.sort_out, 
				tcount);
	    return;

	case QE_TSORT:
	    opc_pushcount(node->node_qen.qen_tsort.tsort_out, 
				tcount);
	    return;

	case QE_EXCHANGE:
	    opc_pushcount(node->node_qen.qen_exch.exch_out, 
				tcount);
	    return;

	case QE_KJOIN:
	    opc_pushcount(node->node_qen.qen_kjoin.kjoin_out, 
				tcount);
	    return;

	case QE_TJOIN:
	    opc_pushcount(node->node_qen.qen_tjoin.tjoin_out, 
				tcount);
	    return;

	case QE_HJOIN:
	    opc_pushcount(node->node_qen.qen_hjoin.hjn_out, 
				tcount);
	    opc_pushcount(node->node_qen.qen_hjoin.hjn_inner, 
				tcount);
	    return;

	case QE_SEJOIN:
	    opc_pushcount(node->node_qen.qen_sejoin.sejn_out, 
				tcount);
	    opc_pushcount(node->node_qen.qen_sejoin.sejn_inner, 
				tcount);
	    return;

	case QE_CPJOIN:
	case QE_FSMJOIN:
	case QE_ISJOIN:
	    opc_pushcount(node->node_qen.qen_sjoin.sjn_out, 
				tcount);
	    opc_pushcount(node->node_qen.qen_sjoin.sjn_inner, 
				tcount);
	    return;

	case QE_ORIG:
	    if (node->node_qen.qen_orig.orig_part != NULL)
		node->node_qen.qen_orig.orig_part->part_threads = tcount;
	    return;

	case QE_QP:
	    act = node->node_qen.qen_qp.qp_act;
	    if (node->node_qen.qen_qp.qp_flag & QP_PARALLEL_UNION)
		tcount = 1;
	    while (act != NULL)
	    {
		if (act->ahd_flags & QEA_NODEACT)
		    opc_pushcount(act->qhd_obj.qhd_qep.ahd_qep, tcount);
		act = act->ahd_next;
	    }
	    return;

	default:
	    return;
    }

}


/*{
** Name: OPC_EXUNION_ARRCNT	- drives opc_exnodebitset for || union.
**
** Description: Loop over action headers of union under QP node being
**	made into parallel union, setting dshmap for each select.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      22-july-04 (inkdo01)
**	    Written for || query processing (replaces opc_exunion_bitset).
*/
static VOID
opc_exunion_arrcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	QEF_AHD		**action,
	i4		*arrcnts,
	PTR		rowmap)

{

    /* Count EXCH node header bits, QP node header bits and then
    ** locate action header for this select of the union and count it, too. */

    opc_exnheadcnt(global, node->node_qen.qen_exch.exch_out, arrcnts, rowmap);
				/* count QP node header indexes */
    if (node->node_qen.qen_exch.exch_out->node_qen.qen_qp.qp_qual != 
							(QEN_ADF *) NULL)
	arrcnts[IX_CX]++;	/* QP owner's CX */

    if (*action == (QEF_AHD *) NULL)
	*action = node->node_qen.qen_exch.exch_out->node_qen.qen_qp.qp_act;
				/* 1st action from QP node under EXCH */
    else *action = (*action)->ahd_next; /* otherwise, next action */

    opc_exactarrcnt(global, *action, arrcnts, rowmap);

}


/*{
** Name: OPC_EXNODEARRCNT	- count no. entries in each subarray
**
** Description: Analyze QP subtree accumulating counts of the buffers
**	and structures that need to be allocated from child thread DSH
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      21-july-04 (inkdo01)
**	    Written for || query processing.
**	10-sep-04 (inkdo01)
**	    Remove QEN_PART_INFO entities from arrays.
**	13-Dec-2005 (kschendel)
**	    Remaining inline QEN_ADF structs moved to pointers, fix here.
**	15-May-2010 (kschendel) b123565
**	    Add missing TPROC case to prevent looping;  add default.
**	    Continue below 1:1 exch, as they depend on parents.
**	19-May-2010 (kschendel) b123759
**	    ijFlagsFile is a hold, not a row.
*/

/* First, a tiny helper routine to deal with the mini-program pointed
** to by a QEN_PART_QUAL.
*/
static void
opc_arrcnt_pqe(QEN_PQ_EVAL *pqe, i4 *arrcnts, PTR rowmap)
{
    i4 ninstrs;

    arrcnts[IX_CX]++;
    BTset(pqe->un.hdr.pqe_value_row, rowmap);
    BTset(pqe->pqe_result_row, rowmap);
    ninstrs = pqe->un.hdr.pqe_ninstrs;
    while (--ninstrs > 0)
    {
	pqe = (QEN_PQ_EVAL *) ((char *)pqe + pqe->pqe_size_bytes);
	BTset(pqe->pqe_result_row, rowmap);
	if (pqe->pqe_eval_op == PQE_EVAL_ANDMAP
	  || pqe->pqe_eval_op == PQE_EVAL_ORMAP)
	    BTset(pqe->un.andor.pqe_source_row, rowmap);
    }
} /* opc_arrcnt_pqe */

/* And now the real thing */
static VOID
opc_exnodearrcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i4		*arrcnts,
	PTR		rowmap)

{
    QEN_OJINFO	*ojinfop;
    QEN_PART_INFO *partp;
    QEN_PART_QUAL *pqual;
    QEN_SJOIN	*sjnp;
    QEN_KJOIN	*kjnp;
    QEN_TJOIN	*tjnp;
    QEN_HJOIN	*hjnp;
    QEN_SEJOIN	*sejnp;
    QEN_SORT	*srtp;
    QEN_TPROC	*tprocp;
    QEN_TSORT	*tsrtp;
    QEN_ORIG	*origp;
    QEN_QP	*qpp;
    QEN_EXCH	*exchp;
    QEF_QP_CB	*qp = global->ops_cstate.opc_qp;
    QEF_RESOURCE *resp;
    QEF_VALID	*vlp;
    QEF_AHD	*act;

    i4		i, j, k;
    i4		dmrix;
    bool	endloop;


    /* Loop (recurse on left, iterate on right) and switch to process
    ** each node in subtree. */

    for ( ; ; )
    {
	if (node == (QEN_NODE *) NULL)
	    return;		/* just in case */

	opc_exnheadcnt(global, node, arrcnts, rowmap);
				/* count node header indexes */

	ojinfop = (QEN_OJINFO *) NULL;
	partp = (QEN_PART_INFO *) NULL;
	pqual = NULL;
	dmrix = -1;

	switch (node->qen_type) {
	  case QE_CPJOIN:
	  case QE_FSMJOIN:
	  case QE_ISJOIN:
	    sjnp = &node->node_qen.qen_sjoin;
	    ojinfop = sjnp->sjn_oj;
	    if (sjnp->sjn_krow >= 0)
		BTset(sjnp->sjn_krow, rowmap);
	    if (sjnp->sjn_hfile >= 0)
		arrcnts[IX_HLD]++;
	    if (sjnp->sjn_itmat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sjnp->sjn_okmat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sjnp->sjn_okcompare != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sjnp->sjn_joinkey != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sjnp->sjn_jqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    opc_exnodearrcnt(global, sjnp->sjn_out, arrcnts, rowmap);
	    node = sjnp->sjn_inner;
	    break;

	  case QE_KJOIN:
	    kjnp = &node->node_qen.qen_kjoin;
	    ojinfop = kjnp->kjoin_oj;
	    partp = kjnp->kjoin_part;
	    pqual = kjnp->kjoin_pqual;
	    if ((dmrix = kjnp->kjoin_get) >= 0)
		arrcnts[IX_DMR]++;
	    if (kjnp->kjoin_krow >= 0)
		BTset(kjnp->kjoin_krow, rowmap);
	    if (kjnp->kjoin_key != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (kjnp->kjoin_kqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (kjnp->kjoin_kcompare != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (kjnp->kjoin_iqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (kjnp->kjoin_jqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    node = kjnp->kjoin_out;
	    break;

	  case QE_TJOIN:
	    tjnp = &node->node_qen.qen_tjoin;
	    ojinfop = tjnp->tjoin_oj;
	    partp = tjnp->tjoin_part;
	    pqual = tjnp->tjoin_pqual;
	    if ((dmrix = tjnp->tjoin_get) >= 0)
		arrcnts[IX_DMR]++;
	    if (tjnp->tjoin_orow >= 0)
		BTset(tjnp->tjoin_orow, rowmap);
	    if (tjnp->tjoin_irow >= 0)
		BTset(tjnp->tjoin_irow, rowmap);
	    if (tjnp->tjoin_jqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (tjnp->tjoin_isnull != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    node = tjnp->tjoin_out;
	    break;

	  case QE_HJOIN:
	    hjnp = &node->node_qen.qen_hjoin;
	    ojinfop = hjnp->hjn_oj;
	    pqual = hjnp->hjn_pqual;
	    arrcnts[IX_HSH]++;
	    if (hjnp->hjn_brow >= 0)
		BTset(hjnp->hjn_brow, rowmap);
	    /* prow is probably already counted as qen_row but make sure */
	    if (hjnp->hjn_prow >= 0)
		BTset(hjnp->hjn_prow, rowmap);
	    if (hjnp->hjn_dmhcb >= 0)
		arrcnts[IX_DMH]++;
	    if (hjnp->hjn_btmat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (hjnp->hjn_ptmat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (hjnp->hjn_jqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    opc_exnodearrcnt(global, hjnp->hjn_out, arrcnts, rowmap);
	    node = hjnp->hjn_inner;
	    break;

	  case QE_SEJOIN:
	    sejnp = &node->node_qen.qen_sejoin;
	    ojinfop = (QEN_OJINFO *) NULL;
	    partp = (QEN_PART_INFO *) NULL;
	    /* if (sejnp->sejn_hget >= 0) - these aren't ref'ed in QEF
		arrcnts[IX_DMR]++; */
	    if (sejnp->sejn_hfile >= 0)
		arrcnts[IX_HLD]++;
	    if (sejnp->sejn_itmat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sejnp->sejn_ccompare != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sejnp->sejn_oqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (sejnp->sejn_okmat != NULL)
		arrcnts[IX_CX]++;
	    if (sejnp->sejn_kcompare != NULL)
		arrcnts[IX_CX]++;
	    if (sejnp->sejn_kqual != NULL)
		arrcnts[IX_CX]++;
	    opc_exnodearrcnt(global, sejnp->sejn_out, arrcnts, rowmap);
	    node = sejnp->sejn_inner;
	    break;

	  case QE_TSORT:
	    tsrtp = &node->node_qen.qen_tsort;
	    pqual = tsrtp->tsort_pqual;
	    if (tsrtp->tsort_get >= 0)
		arrcnts[IX_DMR]++;
	    if (tsrtp->tsort_load >= 0)
		arrcnts[IX_DMR]++;
	    if (tsrtp->tsort_create >= 0)
		arrcnts[IX_DMT]++;
	    if (tsrtp->tsort_shd >= 0)
		arrcnts[IX_SHD]++;
	    if (tsrtp->tsort_mat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    node = tsrtp->tsort_out;
	    break;

	  case QE_SORT:
	    srtp = &node->node_qen.qen_sort;
	    if (srtp->sort_load >= 0)
		arrcnts[IX_DMR]++;
	    if (srtp->sort_create >= 0)
		arrcnts[IX_DMT]++;
	    if (srtp->sort_shd >= 0)
		arrcnts[IX_SHD]++;
	    if (srtp->sort_mat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    node = srtp->sort_out;
	    break;

	  case QE_ORIG:
	  case QE_ORIGAGG:
	    origp = &node->node_qen.qen_orig;
	    if ((dmrix = origp->orig_get) >= 0)
	    {
		arrcnts[IX_DMR]++;
	    }
	    partp = origp->orig_part;
	    pqual = origp->orig_pqual;
	    if (origp->orig_qual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    node = (QEN_NODE *) NULL;
	    break;

	  case QE_QP:
	    qpp = &node->node_qen.qen_qp;
	    if (qpp->qp_qual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    /* Process action headers anchored in QP node. */
	    for (act = node->node_qen.qen_qp.qp_act; act; 
						act = act->ahd_next)
		opc_exactarrcnt(global, act, arrcnts, rowmap);
	    return;

	  case QE_EXCHANGE:
	    exchp = &node->node_qen.qen_exch;
	    if (exchp->exch_mat != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    /* Don't probe below 1:N exchanges, they'll do their own setup.
	    ** 1:1 exchange depends on parent, so keep going.
	    */
	    if (exchp->exch_ccount > 1)
		return;
	    node = exchp->exch_out;
	    break;

	  case QE_TPROC:
	    tprocp = &node->node_qen.qen_tproc;
	    if (tprocp->tproc_parambuild != NULL)
		arrcnts[IX_CX]++;
	    if (tprocp->tproc_qual != NULL)
		arrcnts[IX_CX]++;
	    return;		/* Nothing else interesting */

	  default:
	    TRdisplay("Unexpected QP node type %d under exch\n",node->qen_type);
	    opx_error(E_OP068E_NODE_TYPE);
	}	/* end of switch */

	/* Node specific bits have been set - now go over OJ and
	** partition stuff (if any). */
	if (ojinfop)
	{
	    if (ojinfop->oj_heldTidRow >= 0)
		BTset(ojinfop->oj_heldTidRow, rowmap);
	    if (ojinfop->oj_ijFlagsRow >= 0)
		BTset(ojinfop->oj_ijFlagsRow, rowmap);
	    if (ojinfop->oj_resultEQCrow >= 0)
		BTset(ojinfop->oj_resultEQCrow, rowmap);
	    if (ojinfop->oj_specialEQCrow >= 0)
		BTset(ojinfop->oj_specialEQCrow, rowmap);
	    if (ojinfop->oj_tidHoldFile >= 0)
		arrcnts[IX_HLD]++;
	    if (ojinfop->oj_ijFlagsFile >= 0)
		arrcnts[IX_HLD]++;
	    if (ojinfop->oj_innerJoinedTIDs)
		arrcnts[IX_TTAB]++;
	    if (ojinfop->oj_oqual != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (ojinfop->oj_equal != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (ojinfop->oj_lnull != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	    if (ojinfop->oj_rnull != (QEN_ADF *) NULL)
		arrcnts[IX_CX]++;
	}

	if (partp)
	{
	    if (partp->part_groupmap_ix >= 0)
		BTset(partp->part_groupmap_ix, rowmap);
	    if (partp->part_ktconst_ix >= 0)
		BTset(partp->part_ktconst_ix, rowmap);
	    if (partp->part_knokey_ix >= 0)
		BTset(partp->part_knokey_ix, rowmap);
	}

	/* Part-qual structures contain mini-programs that must be
	** scanned to gather up row numbers.
	*/
	if (pqual != NULL)
	{
	    QEN_PQ_EVAL *pqe;

	    arrcnts[IX_PQUAL]++;
	    if (pqual->part_constmap_ix >= 0)
		BTset(pqual->part_constmap_ix, rowmap);
	    if (pqual->part_lresult_ix >= 0)
		BTset(pqual->part_lresult_ix, rowmap);
	    if (pqual->part_work1_ix >= 0)
		BTset(pqual->part_work1_ix, rowmap);
	    pqe = pqual->part_const_eval;
	    if (pqe != NULL)
		opc_arrcnt_pqe(pqe, arrcnts, rowmap);
	    pqe = pqual->part_join_eval;
	    if (pqe != NULL)
		opc_arrcnt_pqe(pqe, arrcnts, rowmap);
	}

	/* If TJOIN, KJOIN or ORIG, locate DMT_CB index in valid's. */
	if (dmrix >= 0)
	 for (resp = qp->qp_resources, endloop = FALSE; resp && !endloop; 
			resp = resp->qr_next)
	  if (resp->qr_type == QEQR_TABLE)
	   for (vlp = resp->qr_resource.qr_tbl.qr_lastvalid; vlp && !endloop;
			vlp = vlp->vl_next)
	    if (dmrix == vlp->vl_dmr_cb)
	    {
		arrcnts[IX_DMT]++;
		endloop = TRUE;
		if (vlp->vl_partition_cnt > 1)
		    arrcnts[IX_DMR]++;
				/* set master DMR_CB, too */
	    }

	if (node == (QEN_NODE *) NULL)
	    return;
    }	/* end of for ( ; ; ) */

}


/*{
** Name: OPC_EXNODEARRSET	- set DSH ptr array indexes in exch_array1/2
**
** Description: Analyze QP subtree saving DSH ptr array indexes to indicate 
**	the buffers and structures that need to be allocated for child 
**	thread DSH
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      1-mar-04 (inkdo01)
**	    Written for || query processing.
**	28-apr-04 (inkdo01)
**	    Forgot to set bits for QEN_STATUS and hash structures.
**	11-june-04 (inkdo01)
**	    Code to recurse on actions owned by QP node.
**	15-june-04 (inkdo01)
**	    Remove sejn_hget from bit map - not used (and not filled in)
**	    in QEF.
**	17-june-04 (inkdo01)
**	    Add logic to set QEN_PQ_RESET to allow earlier thread shutdown.
**	22-july-04 (inkdo01)
**	    Reworked to produce arrays of DSH ptr array indexes.
**	27-aug-04 (inkdo01)
**	    Forgot QEN_PART_INFOs addr'ed from ORIG nodes.
**	10-sep-04 (inkdo01)
**	    Remove QEN_PART_INFO entities from arrays.
**	15-May-2010 (kschendel) b123565
**	    Add missing TPROC case to prevent looping;  add default.
**	    Continue below 1:1 exch, as they depend on parents.
**	    Delete "resettable", done as a separate pass in opcran now.
**	19-May-2010 (kschendel) b123759
**	    ijFlagsFile is a hold, not a row.
**	    Don't need to do rows here, done via bitmap.
*/
static VOID
opc_exnodearrset(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts)

{
    QEN_OJINFO	*ojinfop;
    QEN_PART_QUAL *pqual;
    QEN_SJOIN	*sjnp;
    QEN_KJOIN	*kjnp;
    QEN_TJOIN	*tjnp;
    QEN_HJOIN	*hjnp;
    QEN_SEJOIN	*sejnp;
    QEN_SORT	*srtp;
    QEN_TPROC	*tprocp;
    QEN_TSORT	*tsrtp;
    QEN_ORIG	*origp;
    QEN_QP	*qpp;
    QEN_EXCH	*exchp;
    QEF_QP_CB	*qp = global->ops_cstate.opc_qp;
    QEF_RESOURCE *resp;
    QEF_VALID	*vlp;
    QEF_AHD	*act;

    i4		i, j, k;
    i4		dmrix;
    bool	endloop;


    /* Loop (recurse on left, iterate on right) and switch to process
    ** each node in subtree. */

    for ( ; ; )
    {
	if (node == (QEN_NODE *) NULL)
	    return;		/* just in case */

	opc_exnheadset(global, node, array1, array2, arrcnts);
				/* set node header indexes */

	ojinfop = (QEN_OJINFO *) NULL;
	pqual = NULL;
	dmrix = -1;

	switch (node->qen_type) {
	  case QE_CPJOIN:
	  case QE_FSMJOIN:
	  case QE_ISJOIN:
	    sjnp = &node->node_qen.qen_sjoin;
	    ojinfop = sjnp->sjn_oj;
	    if (sjnp->sjn_hfile >= 0)
		array1[arrcnts[IX_HLD]++] = sjnp->sjn_hfile;
	    if (sjnp->sjn_itmat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sjnp->sjn_itmat->qen_pos;
	    if (sjnp->sjn_okmat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sjnp->sjn_okmat->qen_pos;
	    if (sjnp->sjn_okcompare != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sjnp->sjn_okcompare->qen_pos;
	    if (sjnp->sjn_joinkey != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sjnp->sjn_joinkey->qen_pos;
	    if (sjnp->sjn_jqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sjnp->sjn_jqual->qen_pos;
	    opc_exnodearrset(global, sjnp->sjn_out, array1, array2, arrcnts);
	    node = sjnp->sjn_inner;
	    break;

	  case QE_KJOIN:
	    kjnp = &node->node_qen.qen_kjoin;
	    ojinfop = kjnp->kjoin_oj;
	    pqual = kjnp->kjoin_pqual;
	    if ((dmrix = kjnp->kjoin_get) >= 0)
		array2[arrcnts[IX_DMR]++] = kjnp->kjoin_get;
	    if (kjnp->kjoin_key != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = kjnp->kjoin_key->qen_pos;
	    if (kjnp->kjoin_kqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = kjnp->kjoin_kqual->qen_pos;
	    if (kjnp->kjoin_kcompare != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = kjnp->kjoin_kcompare->qen_pos;
	    if (kjnp->kjoin_iqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = kjnp->kjoin_iqual->qen_pos;
	    if (kjnp->kjoin_jqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = kjnp->kjoin_jqual->qen_pos;
	    node = kjnp->kjoin_out;
	    break;

	  case QE_TJOIN:
	    tjnp = &node->node_qen.qen_tjoin;
	    ojinfop = tjnp->tjoin_oj;
	    pqual = tjnp->tjoin_pqual;
	    if ((dmrix = tjnp->tjoin_get) >= 0)
		array2[arrcnts[IX_DMR]++] = tjnp->tjoin_get;
	    if (tjnp->tjoin_jqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = tjnp->tjoin_jqual->qen_pos;
	    if (tjnp->tjoin_isnull != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = tjnp->tjoin_isnull->qen_pos;
	    node = tjnp->tjoin_out;
	    break;

	  case QE_HJOIN:
	    hjnp = &node->node_qen.qen_hjoin;
	    ojinfop = hjnp->hjn_oj;
	    pqual = hjnp->hjn_pqual;
	    array1[arrcnts[IX_HSH]++] = hjnp->hjn_hash;
	    if (hjnp->hjn_dmhcb >= 0)
		array2[arrcnts[IX_DMH]++] = hjnp->hjn_dmhcb;
	    if (hjnp->hjn_btmat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = hjnp->hjn_btmat->qen_pos;
	    if (hjnp->hjn_ptmat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = hjnp->hjn_ptmat->qen_pos;
	    if (hjnp->hjn_jqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = hjnp->hjn_jqual->qen_pos;
	    opc_exnodearrset(global, hjnp->hjn_out, array1, array2, arrcnts);
	    node = hjnp->hjn_inner;
	    break;

	  case QE_SEJOIN:
	    sejnp = &node->node_qen.qen_sejoin;
	    ojinfop = (QEN_OJINFO *) NULL;
	    /* if (sejnp->sejn_hget >= 0) - these aren't ref'ed in QEF
		array2[arrcnts[IX_DMR]++] = sejnp->sejn_hget; */
	    if (sejnp->sejn_hfile >= 0)
		array1[arrcnts[IX_HLD]++] = sejnp->sejn_hfile;
	    if (sejnp->sejn_itmat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_itmat->qen_pos;
	    if (sejnp->sejn_ccompare != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_ccompare->qen_pos;
	    if (sejnp->sejn_oqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_oqual->qen_pos;
	    if (sejnp->sejn_okmat != NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_okmat->qen_pos;
	    if (sejnp->sejn_kcompare != NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_kcompare->qen_pos;
	    if (sejnp->sejn_kqual != NULL)
		array2[arrcnts[IX_CX]++] = sejnp->sejn_kqual->qen_pos;
	    opc_exnodearrset(global, sejnp->sejn_out, array1, array2, arrcnts);
	    node = sejnp->sejn_inner;
	    break;

	  case QE_TSORT:
	    tsrtp = &node->node_qen.qen_tsort;
	    pqual = tsrtp->tsort_pqual;
	    if (tsrtp->tsort_get >= 0)
		array2[arrcnts[IX_DMR]++] = tsrtp->tsort_get;
	    if (tsrtp->tsort_load >= 0)
		array2[arrcnts[IX_DMR]++] = tsrtp->tsort_load;
	    if (tsrtp->tsort_create >= 0)
		array2[arrcnts[IX_DMT]++] = tsrtp->tsort_create;
	    if (tsrtp->tsort_shd >= 0)
		array1[arrcnts[IX_SHD]++] = tsrtp->tsort_shd;
	    if (tsrtp->tsort_mat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = tsrtp->tsort_mat->qen_pos;
	    node = tsrtp->tsort_out;
	    break;

	  case QE_SORT:
	    srtp = &node->node_qen.qen_sort;
	    if (srtp->sort_load >= 0)
		array2[arrcnts[IX_DMR]++] = srtp->sort_load;
	    if (srtp->sort_create >= 0)
		array2[arrcnts[IX_DMT]++] = srtp->sort_create;
	    if (srtp->sort_shd >= 0)
		array1[arrcnts[IX_SHD]++] = srtp->sort_shd;
	    if (srtp->sort_mat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = srtp->sort_mat->qen_pos;
	    node = srtp->sort_out;
	    break;

	  case QE_ORIG:
	  case QE_ORIGAGG:
	    origp = &node->node_qen.qen_orig;
	    pqual = origp->orig_pqual;
	    if ((dmrix = origp->orig_get) >= 0)
	    {
		array2[arrcnts[IX_DMR]++] = origp->orig_get;
	    }
	    if (origp->orig_qual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = origp->orig_qual->qen_pos;
	    node = (QEN_NODE *) NULL;
	    break;

	  case QE_QP:
	    qpp = &node->node_qen.qen_qp;
	    if (qpp->qp_qual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = qpp->qp_qual->qen_pos;
	    /* Process action headers anchored in QP node. */
	    for (act = node->node_qen.qen_qp.qp_act; act; 
						act = act->ahd_next)
		opc_exactarrset(global, act, array1, array2, arrcnts);
	    return;

	  case QE_EXCHANGE:
	    exchp = &node->node_qen.qen_exch;
	    if (exchp->exch_mat != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = exchp->exch_mat->qen_pos;
	    /* Don't probe below 1:N exchanges, they'll do their own setup.
	    ** 1:1 exchange depends on parent, so keep going.
	    */
	    if (exchp->exch_ccount > 1)
		return;
	    node = exchp->exch_out;
	    break;

	  case QE_TPROC:
	    tprocp = &node->node_qen.qen_tproc;
	    if (tprocp->tproc_parambuild != NULL)
		array2[arrcnts[IX_CX]++] = tprocp->tproc_parambuild->qen_pos;
	    if (tprocp->tproc_qual != NULL)
		array2[arrcnts[IX_CX]++] = tprocp->tproc_qual->qen_pos;
	    return;		/* Nothing underneath */

	  default:
	    TRdisplay("Unexpected QP node type %d under exch\n",node->qen_type);
	    opx_error(E_OP068E_NODE_TYPE);
	}	/* end of switch */

	/* Node specific bits have been set - now go over OJ and
	** partition stuff (if any). */
	if (ojinfop)
	{
	    if (ojinfop->oj_tidHoldFile >= 0)
		array1[arrcnts[IX_HLD]++] = ojinfop->oj_tidHoldFile;
	    if (ojinfop->oj_ijFlagsFile >= 0)
		array1[arrcnts[IX_HLD]++] = ojinfop->oj_ijFlagsFile;
	    if (ojinfop->oj_innerJoinedTIDs)
		array1[arrcnts[IX_TTAB]++] = ojinfop->oj_innerJoinedTIDs->
			ttb_tempTableIndex;
	    if (ojinfop->oj_oqual != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = ojinfop->oj_oqual->qen_pos;
	    if (ojinfop->oj_equal != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = ojinfop->oj_equal->qen_pos;
	    if (ojinfop->oj_lnull != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = ojinfop->oj_lnull->qen_pos;
	    if (ojinfop->oj_rnull != (QEN_ADF *) NULL)
		array2[arrcnts[IX_CX]++] = ojinfop->oj_rnull->qen_pos;
	}

	if (pqual != NULL)
	{
	    array1[arrcnts[IX_PQUAL]++] = pqual->part_pqual_ix;
	    if (pqual->part_const_eval != NULL)
		array2[arrcnts[IX_CX]++] = pqual->part_const_eval->un.hdr.pqe_cx->qen_pos;
	    if (pqual->part_join_eval != NULL)
		array2[arrcnts[IX_CX]++] = pqual->part_join_eval->un.hdr.pqe_cx->qen_pos;
	}

	/* If TJOIN, KJOIN or ORIG, locate DMT_CB index in valid's. */
	if (dmrix >= 0)
	 for (resp = qp->qp_resources, endloop = FALSE; resp && !endloop; 
			resp = resp->qr_next)
	  if (resp->qr_type == QEQR_TABLE)
	   for (vlp = resp->qr_resource.qr_tbl.qr_lastvalid; vlp && !endloop;
			vlp = vlp->vl_next)
	    if (dmrix == vlp->vl_dmr_cb)
	    {
		array2[arrcnts[IX_DMT]++] = vlp->vl_dmf_cb;
		endloop = TRUE;
		if (vlp->vl_partition_cnt > 1)
		    array2[arrcnts[IX_DMR]++] = dmrix - 1;
				/* set master DMR_CB, too */
	    }

	if (node == (QEN_NODE *) NULL)
	    return;
    }	/* end of for ( ; ; ) */

}


/*{
** Name: OPC_EXNHEADCNT	- count array entriesfor node headers
**
** Description: Counts array entries for node headers - allows code to be 
**	called from several places in module
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      22-july-04 (inkdo01)
**	    Written for || query processing.
*/
static VOID
opc_exnheadcnt(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i4		*arrcnts,
	PTR		rowmap)

{

    /* Simply analyze possible array entries for node header and
    ** add (as necessary) to counts. */

    if (node == (QEN_NODE *) NULL)
	return;		/* just in case */

    arrcnts[IX_STAT]++;
    if (node->qen_row >= 0)
	BTset(node->qen_row, rowmap);
    if (node->qen_frow >= 0)
	BTset(node->qen_frow, rowmap);
    if (node->qen_fatts != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;
    if (node->qen_prow != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;

}


/*{
** Name: OPC_EXNHEADSET	- set DSH ptr array indexes in exch_array1/2 for
**	node header
**
** Description: Sets array indexes for node headers - allows code to be 
**	called from several places in module
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      22-july-04 (inkdo01)
**	    Written for || query processing.
*/
static VOID
opc_exnheadset(
	OPS_STATE	*global,
	QEN_NODE	*node,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts)

{

    /* Simply analyze possible array entries for node header and
    ** store (as necessary) in arrays. */

    if (node == (QEN_NODE *) NULL)
	return;		/* just in case */

    array1[arrcnts[IX_STAT]++] = node->qen_num;
    if (node->qen_fatts != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = node->qen_fatts->qen_pos;
    if (node->qen_prow != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = node->qen_prow->qen_pos;

}


/*{
** Name: OPC_EXACTARRCNT	- count no. of entries in each subarray 
**
** Description: Analyze action headers under QP nodes counting size
**	of subarrays to guide buffer/structure allocation for child DSH.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      22-july-04 (inkdo01)
**	    Written for || processing.
*/
static VOID
opc_exactarrcnt(
	OPS_STATE	*global,
	QEF_AHD		*action,
	i4		*arrcnts,
	PTR		rowmap)

{


    /* Build counts for ADF CX's in action header. */
    if (action->ahd_mkey != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;
    if (action->qhd_obj.qhd_qep.ahd_current != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;
    if (action->qhd_obj.qhd_qep.ahd_constant != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;
    if (action->qhd_obj.qhd_qep.ahd_by_comp != (QEN_ADF *) NULL)
	arrcnts[IX_CX]++;
    if (action->ahd_atype == QEA_HAGGF)
    {
	if (action->qhd_obj.qhd_qep.u1.s2.ahd_agoflow != (QEN_ADF *) NULL)
	    arrcnts[IX_CX]++;
	arrcnts[IX_HSHA]++;
    }

    opc_exnodearrcnt(global, action->qhd_obj.qhd_qep.ahd_qep, arrcnts, rowmap);
			/* node anchored in action header */

}


/*{
** Name: OPC_EXACTARRSET	- set indexes in exch_array1/2 for action
**	headers
**
** Description: Analyze action headers under QP nodes setting indexes
**	in exchange arrays to guide buffer/structure allocation for
**	child DSH.
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      11-june-04 (inkdo01)
**	    Written for || processing.
**	22-july-04 (inkdo01)
**	    Reworked to produce arrays of DSH ptr array indexes.
**	16-May-2010 (kschendel) b123565
**	    Don't set resettable here, do separately.
*/
static VOID
opc_exactarrset(
	OPS_STATE	*global,
	QEF_AHD		*action,
	i2		*array1,
	i4		*array2,
	i4		*arrcnts)

{


    /* Set dsh_cbs indexes for ADF CX's in action header. */
    if (action->ahd_mkey != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = action->ahd_mkey->qen_pos;
    if (action->qhd_obj.qhd_qep.ahd_current != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = action->qhd_obj.qhd_qep.ahd_current->qen_pos;
    if (action->qhd_obj.qhd_qep.ahd_constant != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = action->qhd_obj.qhd_qep.ahd_constant->qen_pos;
    if (action->qhd_obj.qhd_qep.ahd_by_comp != (QEN_ADF *) NULL)
	array2[arrcnts[IX_CX]++] = action->qhd_obj.qhd_qep.ahd_by_comp->qen_pos;
								
    if (action->ahd_atype == QEA_HAGGF)
    {
	if (action->qhd_obj.qhd_qep.u1.s2.ahd_agoflow != (QEN_ADF *) NULL)
	    array2[arrcnts[IX_CX]++] = action->qhd_obj.qhd_qep.u1.s2.
				ahd_agoflow->qen_pos;
	array1[arrcnts[IX_HSHA]++] = action->qhd_obj.qhd_qep.u1.s2.ahd_agcbix;
    }
    opc_exnodearrset(global, action->qhd_obj.qhd_qep.ahd_qep, array1, array2,
		arrcnts);	/* node anchored in action header */

}
