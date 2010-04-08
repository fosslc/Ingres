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
#include    <me.h>
#include    <bt.h>
#include    <st.h>
#include    <cui.h>
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
#define        OPC_QEN		TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCQEN.C - Routines to create QEN_NODE structures.
**
**  Description:
**      This file holds the top level routine for compiling QEN nodes, and 
**      routines for filling in the common portion of a QEN node. The
**      node type specific portions (ie. for orig nodes, or one of the various 
**      join nodes, etc) are filled in by routines in OPCORIG.C, OPCJOIN.C, 
**      OPCSEJOIN.C, OPCRAN.C, and OPCSORT.C.
[@comment_line@]...
**
**	Externally visable routines:
**          opc_qentree_build() - Build a QEN_NODE tree from a CO tree.
**          opc_qnatts() - Fill in the qen_atts and qen_natts of a QEN_NODE.
**          opc_fatts() - Materialize the function atts for a CO/QEN node.
**	    opc_getFattDataType() - Fill in data type info for a function attribute.
[@func_list@]...
**
**	Internal only routines:
**	    none yet.
[@func_list@]...
**
**
**  History:
**      4-aug-86 (eric)
**          created
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	30-oct-89 (stec)
**	    Modify opc_qentree_build.
**	01-mar-90 (stec)
**	    Fixed BTcount on a stack allocated bitmap.
**      30-apr-90 (eric)
**          Added support for initializing fields to tell qef what the cpu
**          and dio estimates for each node are.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	20-jun-90 (stec)
**	    Fixed a potential problem in opc_qnatts() related to the fact that
**	    BTcount() in opc_jqual was counting bits in the whole bitmap.
**	13-july-90 (stec)
**	    Fixed an integer overflow problem in opc_qentree_build().
**	14-jan-91 (stec)
**	    Modified opq_qnatts().
**	18-jan-91 (stec)
**	    Added missing opr_prec initialization to opc_qnatts(), opc_fatts().
**	24-feb-92 (rickh)
**	    Cut a paragraph out of opc_fatts and made a new routine out of
**	    it:  opc_getFattDataType.  This routine is called both by opc_fatts
**	    and by opc_eminstrs.
**	04-aug-92 (anitap)
**          Commented out NO and OUT after #endif in opc_qentree_build() and
**          opc_qnatts() to shut up HP compiler.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid" in opc_qnatts()
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-jul-93 (rickh)
**	    EQCs represented by function attributes should inherit the
**	    function attribute's datavalue.
**      17-aug-93 (terjeb)
**          Typecast the values in the conditional tests involving dissimilar
**          types like float and MAXI4. The lack of these casts caused by
**          floating point exceptions on a certain machine in 
**	    opc_qentree_build().
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	26-oct-93 (rickh)
**	    In the function attribute compiler, we weren't correctly resetting
**	    the function attribute descriptor.  As a consequence, all function
**	    attributes for a QEN node were being compiled with the query tree 
**	    of the last function attribute for that node.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	06-jul-95 (canor01)
**	    Change OUT to xOUT to avoid conflicts with Microsoft NT compiler
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanup, delete unused adbase parameters.
[@history_template@]...
**/

/*
**  Forward and/or External function references.
*/

/*
**  Defines of other constants.
*/

/*{
** Name: OPC_QENTREE_BUILD	- Build a QEN_NODE (sub)tree from a CO (sub)tree
**
** Description:
**      This routine coordinates all of the work for compiling a CO tree into 
**      a QEN_NODE tree. As is typical with coordinators, it doesn't actually 
**      do that much work. It first allocates a QEN node, gives it some initial
**	values, and does some bookkeeping with the action node and some other
**	data structures. Next, it decides what kind of QEN node that this 
**      needs to be, which is probably the most important thing that it
**	does. It then it calls one of a variety of routines that will
**      fill in the node type specific portion of the QEN node. Finally, it 
**      fills in the common portion of the QEN node, which includes compiling 
**      function atts, filling in the qen_atts array, and other bookkeeping. 
[@comment_line@]...
**
** Inputs:
**  global
**	The global state information for this optimization/compilation.
**  cnode
**	A pointer to a OPC_NODE struct that contains information about the
**	current co/qen node.
**  cnode->opc_cco
**	The co node/tree to be converted into a QEN_NODE struct/tree.
**  cnode->opc_cnnum
**	The number of the QEN_NODE struct to be created.
**  cnode->opc_level
**	Gives information about the relationship between cnode->opc_cco and it's
**	parent.
**  cnode->opc_endpostfix
**	Gives the previous node in a post order traversal of the 
**	the tree that this subtree will belong to. Used by the routines that 
**	opc_qentree_build calls
**  cnode->opc_ceq
**	pointers to zero filled arrays of sufficient size.
**
** Outputs:
**  cnode->opc_qennode
**	pointer to the root of the QEN_NODE tree that was translated from 
**	cnode->opc_cco.
**  cnode->opc_bgpostfix
**	A pointer to the first QEN_NODE struct in a post order traversal of the
**	cnode->opc_qennode tree if cnode->opc_level == OPC_COTOP. If
**	cnode->opc_level != OPC_COTOP then cnode->opc_bgpostfix will be
**	unchanged.
**  cnode->opc_ceq
**	The eq class and attribute maps that have been filled out for node
**	cnode->opc_qennode.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except those that opx_error raise.
**
** Side Effects:
**	    none
**
** History:
**      4-aug-86 (eric)
**          created
**      20-july-87 (eric)
**          added calls to opu_Msmemory_mark() and opu_Rsmemory_reclaim().
**	30-oct-89 (stec)
**	    Modify code to add support to new qen nodes and remove support
**	    for the old ones.
**	13-july-90 (stec)
**	    Fixed an integer overflow problem related to cost initialization.
**      04-aug-92 (anitap)
**          Commented out NO after #endif as HP compiler was complaining.
**      17-aug-93 (terjeb)
**          Typecast the values in the conditional tests involving dissimilar
**          types like float and MAXI4. The lack of these casts caused by
**          floating point exceptions on a certain machine.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	10-nov-95 (inkdo01)
**	    Changes to support reorganization of QEF_AHD, QEN_NODE structs.
**	23-oct-97 (inkdo01)
**	    Fiddle opc_fatts call (for 86478).
**	26-feb-99 (inkdo01)
**	    Add support for hash joins.
**	17-dec-03 (inkdo01)
**	    Add qen_high/low to determine buffer sets for allocation under
**	    1:n exchange node.
**	2-mar-04 (inkdo01)
**	    Support for exchange nodes under PR nodes.
**	21-july-04 (inkdo01)
**	    Change EXCH node allocation to account for || unions.
**	14-Mar-2005 (schka24)
**	    Don't mess with 4byte tidp flag here, let top level figure it out.
**	27-may-05 (inkdo01)
**	    Init qen_nthreads for parallel query.
**	6-may-2008 (dougi)
**	    Add support for table procedure nodes.
*/
VOID
opc_qentree_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
	/* Some generally usefull vars that you should already
	** understand
	*/
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    QEF_AHD		*ahd = subqry->ops_compile.opc_ahd;

	/* tco and bco point to the top and bottom co nodes that we
	** are interested in here. In practice, tco is the same a
	** cnode->opc_cco, and bco is the same or it's child if
	** tco points to a PR node.
	*/
    OPO_CO		*tco = cnode->opc_cco;
    OPO_CO		*bco = cnode->opc_cco;

	/* var is used in deciding whether we need a KJOIN, TJOIN, or an SEJOIN
	*/
    OPV_VARS		*var;

	/* qn points to the QEN node that we are building. */
    QEN_NODE		*qn;
	/* qntype is node type and qnsize is the required size */
    i4			qntype;
    i4			qnsize;
	/* Mem_mark is used to reclaim memory that was used during the
	** processing of this node.
	*/
    ULM_SMARK		mem_mark;

	/* below_rows is used to correctly calculate the number of DSH rows
	** that are used by this node and it's children.
	*/
    i4			below_rows;

    /* First, lets mark where we are in the ULM memory stack memory stream */
    opu_Msmemory_mark(global, &mem_mark);

    /* Lets begin the calculation of opc_below_rows */
    below_rows = qp->qp_row_cnt;
    cnode->opc_below_rows = 1;

    /* Moving right along, lets figure out the type of this node, and the
    ** the address of the function for QEF to call to execute this node.
    */
    if (cnode->opc_invent_sort == TRUE)
    {
	/* We need to "invent" a sort node. This means that OPF set a bit
	** in this co nodes parent indicating that a sort node should be
	** added here. Let's figure out what kind of sort node that we
	** need.
	*/
	if (cnode->opc_level == OPC_COTOP || 
		cnode->opc_level == OPC_COOUTER
	    )
	{
	    qntype = QE_TSORT;
	    qnsize = sizeof(QEN_TSORT);
	}
	else
	{
	    qntype = QE_SORT;
	    qnsize = sizeof(QEN_SORT);
	}
    }
    else
    {
	switch (tco->opo_sjpr)
	{
	 case DB_PR:
	    if (tco->opo_inner == NULL)
	    {
		bco = tco->opo_outer;
	    }
	    else
	    {
		bco = tco->opo_inner;
	    }
	    /* Don't break! Drop through to the DB_ORIG case */

	case DB_ORIG:
	    /* If the range variable info for this node doesn't have a
	    ** subselect, then this must be an orig or a tproc node. Otherwise,
	    ** the subselect must mean that this is a QEN_QP node (like,
	    ** for a view).
	    */
	    if (bco->opo_storage == DB_TPROC_STORE)
	    {
		qntype = QE_TPROC;
		qnsize = sizeof(QEN_TPROC);
	    }
	    else if (subqry->ops_vars.opv_base->opv_rt[bco->opo_union.opo_orig]->opv_subselect
									== NULL
		)
	    {
		qntype = QE_ORIG;
		qnsize = sizeof(QEN_ORIG);
	    }
	    else
	    {
		qntype = QE_QP;
		qnsize = sizeof(QEN_QP);
	    }
	    break;

	case DB_SJ:
	    switch (tco->opo_variant.opo_local->opo_jtype)
	    {
		case OPO_SJFSM:
		    qntype = QE_FSMJOIN;
		    qnsize = sizeof(QEN_SJOIN);
		    break;

		case OPO_SJPSM:
		    qntype = QE_ISJOIN;
		    qnsize = sizeof(QEN_SJOIN);
		    break;

		case OPO_SEJOIN:
		    qntype = QE_SEJOIN;
		    qnsize = sizeof(QEN_SEJOIN);
		    break;

		case OPO_SJTID:
		    qntype = QE_TJOIN;
		    qnsize = sizeof(QEN_TJOIN);
		    break;

		case OPO_SJKEY:
		    qntype = QE_KJOIN;
		    qnsize = sizeof(QEN_KJOIN);
		    break;

		case OPO_SJHASH:
		    qntype = QE_HJOIN;
		    qnsize = sizeof(QEN_HJOIN);
		    break;

		case OPO_SJCARTPROD:
		    qntype = QE_CPJOIN;
		    qnsize = sizeof(QEN_SJOIN);
		    break;

		case OPO_SJNOJOIN:
		default:
		    opx_error( E_OP08AB_BAD_JOIN_TYPE );
		    break;
	    }
	    break;

	case DB_EXCH:
	    qntype = QE_EXCHANGE;
	    qnsize = sizeof(QEN_EXCH);
	    if (tco->opo_punion)
	    {
		/* Parallel union - add space for more array descriptors. */
		qnsize += (tco->opo_ccount-1) * 
			sizeof(qn->node_qen.qen_exch.exch_ixes[0]);
	    }
	    break;

	case DB_RSORT:
	case DB_RISAM:	    /* EJLFIX: get rid of this */
	    if (cnode->opc_level == OPC_COTOP || 
		    cnode->opc_level == OPC_COOUTER
		)
	    {
		qntype = QE_TSORT;
		qnsize = sizeof(QEN_TSORT);
	    }
	    else
	    {
		qntype = QE_SORT;
		qnsize = sizeof(QEN_SORT);
	    }
	    break;

	default:
	    opx_error(E_OP0885_COTYPE);
	    break;
	}
    }

    /* Ok, let's allocate the QEN_NODE struct */
    qn = cnode->opc_qennode = (QEN_NODE *)opu_qsfmem(global, sizeof (QEN_NODE) -
		sizeof(qn->node_qen) + qnsize);

    /* Now lets give initialize values to the simple stuff */
    qn->qen_num = global->ops_cstate.opc_qnnum;
    qn->qen_type = qntype;
    qn->qen_size = sizeof (QEN_NODE) -
		sizeof(qn->node_qen) + qnsize;
    qn->qen_postfix = NULL;
    qn->qen_natts = 0;
    qn->qen_atts = NULL;
    qn->qen_row = -1;
    qn->qen_frow = -1;
    qn->qen_low = qp->qp_row_cnt;
    qn->qen_nthreads = 1;

    /* The code below is supposed to prevent integer overflow.
    ** We are assuming that a sizeof(i4) == sizeof(i4)
    ** and will therefore use MAXI4 value when necessary. Lack
    ** of MAXLONGNAT was discussed with a CL committee member
    ** and the above solution was considered safe. Besides
    ** ANSI "C" defines i4 as a 4 byte long entity.
    */ 
    qn->qen_est_tuples = 
	(tco->opo_cost.opo_tups < (OPO_TUPLES)MAXI4) ? 
			(i4)tco->opo_cost.opo_tups : MAXI4;
    qn->qen_dio_estimate = 
	(tco->opo_cost.opo_dio < (OPO_BLOCKS)MAXI4) ? 
			(i4)tco->opo_cost.opo_dio : MAXI4;
    qn->qen_cpu_estimate = 
	(tco->opo_cost.opo_cpu < (OPO_CPU)MAXI4) ? 
			(i4)tco->opo_cost.opo_cpu : MAXI4;

    qn->qen_prow = (QEN_ADF *)NULL;

    /* update the qep count in the action header.
    */
    ahd->qhd_obj.qhd_qep.ahd_qep_cnt += 1;
    global->ops_cstate.opc_qnnum += 1;
    global->ops_cstate.opc_curnode = qn;

    /*
    **  Check if joins have an inner or outer that
    **  specifies no eqcs; this must never happen.
    */
    switch (qn->qen_type)
    {
     case QE_TJOIN:
     case QE_KJOIN:
     case QE_HJOIN:
     case QE_FSMJOIN:
     case QE_CPJOIN:
     case QE_ISJOIN:
     case QE_SEJOIN:
     {
	if (BTcount((char *)&cnode->opc_cco->opo_inner->opo_maps->opo_eqcmap,
	    (i4)subqry->ops_eclass.ope_ev) == 0
	   )
	{
	    opx_error( E_OP08AA_EMPTY_NODE );
	    return;
	}
	if (BTcount((char *)&cnode->opc_cco->opo_outer->opo_maps->opo_eqcmap,
	    (i4)subqry->ops_eclass.ope_ev) == 0
	   )
	{
	    opx_error( E_OP08AA_EMPTY_NODE );
	    return;
	}
     }

     default:
	break;
    }

    /*
    ** Now that we know the type of this node,
    ** let's fill in the type specific stuff.
    */
    cnode->opc_cco = tco;
    switch (qn->qen_type)
    {
     case QE_ORIG:
	opc_orig_build(global, cnode);
	break;

     case QE_TPROC:
	opc_tproc_build(global, cnode);
	break;

     case QE_EXCHANGE:
	opc_exch_build(global, cnode);
	break;

     case QE_QP:
	opc_ran_build(global, cnode);
	break;

     case QE_TJOIN:
	opc_tjoin_build(global, cnode);
	break;

     case QE_KJOIN:
	opc_kjoin_build(global, cnode);
	break;

     case QE_HJOIN:
	opc_hjoin_build(global, cnode);
	break;

     case QE_FSMJOIN:
	opc_fsmjoin_build(global, cnode);
	break;

     case QE_CPJOIN:
	opc_cpjoin_build(global, cnode);
	break;

     case QE_ISJOIN:
	opc_isjoin_build(global, cnode);
	break;

     case QE_SEJOIN:
	opc_sejoin_build(global, cnode);
	break;

     case QE_SORT:
	opc_sort_build(global, cnode);
	break;

     case QE_TSORT:
	opc_tsort_build(global, cnode);
	break;

     default:
	opx_error(E_OP0880_NOT_READY);
	break;
    }
    qn = cnode->opc_qennode;
    global->ops_cstate.opc_curnode = (QEN_NODE *) NULL;

    /* If this isn't a remote action node (a qe_qp) then lets materialize 
    ** the function atts created by this node. We had to add the check for
    ** QE_QP nodes because of the strange way that their eqc maps are
    ** filled in by OPF.
    */
    if (qn->qen_type != QE_QP && cnode->opc_invent_sort == FALSE)
    {
	/*
	** ORIG nodes must have non empty eqc maps. With an 
	** empty one bco should be used instead of tco.
	*/
	opc_fatts(global, cnode, tco, qn, &qn->qen_fatts, NULL);
    }

    /* Now that the rest of the node/tree is built, we can set the postfix
    ** list.
    */
    if (subqry->ops_compile.opc_endpostfix == NULL)
    {
	subqry->ops_compile.opc_bgpostfix = qn;
    }
    else
    {
	subqry->ops_compile.opc_endpostfix->qen_postfix = qn;
    }
    subqry->ops_compile.opc_endpostfix = qn;

    /* Now lets fill in the
    ** returning attributes info in this node if it hasn't already been filled
    ** in. Currently, orig node fills this in themselves.
    */
    cnode->opc_below_rows = qp->qp_row_cnt - below_rows;
    if (qn->qen_atts == NULL)
    {
	opc_qnatts(global, cnode, tco, qn);
    }

    /* Now lets finish the calculation of opc_below_rows. This is done
    ** by subtracting the number of rows that were used at the beginning
    ** of this routine from the current number of rows.
    */
    cnode->opc_below_rows = qp->qp_row_cnt - below_rows;
    qn->qen_high = qp->qp_row_cnt-1;

    /* Now lets return all of the memory that we used in the stack ULM
    ** memory stream to the global pool
    */
    opu_Rsmemory_reclaim(global, &mem_mark);
}

/*{
** Name: OPC_QNATTS	- fill in the qen_atts and qen_natts in a QEN_NODE.
**
** Description:
**      This routine fills in the qen_atts and qen_natts fields for a QEN_NODE. 
**      Qen_atts describes the attributes that the given QEN_NODE struct  
**      returns. Qen_natts tell how many atts are returned. These fields are 
**      filled in from the eqc map of the QEN_NODEs corresponding CO node. 
**      Each eqc in the COs map represents an attribute that is being returned 
**      from the QEN_NODE. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable for OPF and OPC.
**	cnode -
**	    This holds information about the compilation state of the 
**	    current co/qen node.
**	tco -
**	    The co node that holds the eqcmp that we use to fill in the
**	    qen_atts and qen_natts fields
**	qn -
**	    The QEN node that holds the qen_atts and qen_natts fields to
**	    be filled in.
**
** Outputs:
**	qn -
**	    The qen_atts and qen_natts will be filled in to describe the
**	    returning attributes.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except for those that opx_error raises.
**
** Side Effects:
**	    none
**
** History:
**      12-apr-87 (eric)
**          written
**      17-may-89 (eric)
**          added support for qen_prow in QEN_NODE structs.
**	17-jul-89 (jrb)
**	    Correctly set precision fields for decimal project.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
**	14-jan-91 (stec)
**	    Fixed code related to op173 trace point; sometimes we would
**	    try to fill qen_prow CX without executing opc_adbegin().
**	20-jul-92 (rickh)
**	    Pass adc_tmlen a pointer to an i2, not an i4.
**      04-aug-92 (anitap)
**          Commented out OUT after #endif to shut up HP compiler.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:  Use appropriate case for "tid" in opc_qnatts()
**	06-apr-01 (inkdo01)
**	    Pass adc_tmlen ptrs to i4, not i2.
[@history_template@]...
*/
VOID
opc_qnatts(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPO_CO		*tco,
		QEN_NODE	*qn)
{
	/* Standard variables */
    DB_STATUS		status;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;

	/* eqcno holds the current eqc number that is being converted into a
	** an attribute description.
	*/
    OPE_IEQCLS		eqcno;

	/* Jatt points to joinops attribute description for the current
	** attribute.
	*/
    OPZ_ATTS		*jatt;
    OPZ_IATTS		jattno;

	/* Rel points to a description of the relation for the current 
	** attribute.
	*/
    RDR_INFO		*rel;

	/* dmt_size holds the size of the array that is required to hold
	** the attribute description that we are producing.
	*/
    i4			dmt_size;

	/* Iatt and prev_iatt point the the current and previous attribute
	** descriptions that we are building.
	*/
    DMT_ATT_ENTRY	*iatt;
    DMT_ATT_ENTRY	*prev_iatt;

	/* Used to insure the alignment of data. */
    i4			align;

	/* qadf is a pointer to the QEN_ADF that we will allocate to
	** hold the qen_prow instructions. Cadf is the OPC version of the
	** same thing.
	*/
    QEN_ADF		**qadf;
    OPC_ADF		cadf;

	/* Ninstr, nops, nconst, szconst, and maxbase are used to estimate
	** the size of the CX.
	*/
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;

	/* ops is used to compile the qen_prow CX. */
    ADE_OPERAND		ops[2];

	/* prowno is the result DSH row number for qen_prow, and prowsize is
	** the size for the current attribute that is being tmcvt'ed
	*/
    i4			prowno;

	/* Prow_worsecase holds the additional space required
	** for the worst case size of a data value. Prow_wctemp holds
	** intermediate values for the worst case calculation.
	*/
    i4			prow_worstcase;
    i4			prow_wctemp;
    i4			tempLength;

	/* op_vertbar and dv_vertbar describe the vertical bar seperators that
	** are used for qen_prow
	*/
    ADE_OPERAND		op_vertbar;
    DB_DATA_VALUE	dv_vertbar;

    /* First, find out how many attributes we will be describing */
    qn->qen_natts = BTcount((char *)&tco->opo_maps->opo_eqcmap, 
	(i4)subqry->ops_eclass.ope_ev);

    /* If there are one or more atts */
    if (qn->qen_natts > 0)
    {
	/* figure out how big our attribute description will be */
	dmt_size = sizeof (qn->qen_atts[0]) * qn->qen_natts;
	qn->qen_atts = (DMT_ATT_ENTRY **) opu_qsfmem(global, dmt_size);

	/* begin the compilation of the qen_prow CX */
#ifdef OPT_F045_QENPROW
	if (opt_strace(global->ops_cb, OPT_F045_QENPROW) == TRUE)
	{
	    /* estimate the size of the compilation; */
	    ninstr = (2 * qn->qen_natts) + 1;
	    nops = (2 * ninstr);
	    nconst = 1;
	    szconst = 1;
	    max_base = cnode->opc_below_rows + 1;

	    /* begin the compilation */
	    qadf = &qn->qen_prow;
	    opc_adalloc_begin(global, &cadf, qadf, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

		/* set up the prow information */
	    prowno = qp->qp_row_cnt;
	    prow_worstcase = 0;
	    (*qadf)->qen_output = prowno;

		/* set up the result base. */
	    opc_adbase(global, &cadf, QEN_ROW, prowno);
	    ops[0].opr_base = cadf.opc_row_base[prowno];
	    ops[0].opr_offset = 0;

	    /* add the vertical bar seperator to the CX */
	    dv_vertbar.db_datatype = DB_CHA_TYPE;
	    dv_vertbar.db_length = 1;
	    dv_vertbar.db_prec = 0;
	    dv_vertbar.db_data = "|";
	    opc_adconst(global, &cadf, &dv_vertbar, &op_vertbar, DB_SQL, ADE_SMAIN);
	}
#endif

	/* for (each eqc that we return) */
	qn->qen_natts = 0;
	for (eqcno = -1; 
		(eqcno = (OPE_IEQCLS) BTnext((i4) eqcno, 
				(char *)&tco->opo_maps->opo_eqcmap, 
				(i4)subqry->ops_eclass.ope_ev)) != -1;
		qn->qen_natts += 1
	    )
	{
	    /* figure out which attribute we have for the current
	    ** eqcno, and find the joinop attribute description for it
	    */
	    if (cnode->opc_ceq[eqcno].opc_eqavailable == FALSE)
	    {
		opx_error(E_OP0889_EQ_NOT_HERE);
	    }
	    jattno = cnode->opc_ceq[eqcno].opc_attno;

	    if (jattno != OPC_NOATT)
	    {
		jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];
		if (jatt->opz_varnm >= 0)
		{
		    /* there is a relation description for this attribute.
		    ** ie. this isn't a multi-variable function att.
		    */
		    rel = subqry->ops_vars.opv_base->
			opv_rt[jatt->opz_varnm]->opv_grv->opv_relation;
		}
		else
		{
		    rel = NULL;
		}
	    }
	    else
	    {
		rel = NULL;
	    }

	    /* fill in the next element in qn->qen_atts; */
	    qn->qen_atts[qn->qen_natts] = 
	       (DMT_ATT_ENTRY *) opu_qsfmem(global, sizeof (DMT_ATT_ENTRY));

	    iatt = qn->qen_atts[qn->qen_natts];

	    if (rel == NULL)
	    {
		/* If this isn't a function attribute, and we don't have
		** a relation descriptior for some other reason, then
		** make up a name for the attribute. This can happen
		** for data that is returned through QEN_QP nodes.
		*/
		STprintf(iatt->att_name.db_att_name, "ratt%d", eqcno);
		STmove(iatt->att_name.db_att_name, ' ', 
				sizeof (iatt->att_name.db_att_name), 
						iatt->att_name.db_att_name);
	    }
	    else if (jatt->opz_attnm.db_att_id == DB_IMTID)
	    {
		/* if the attribute is a tid, then fill in a hard coded
		** name, because it doesn't appear elsewhere.
		*/
		STmove(((*global->ops_cb->ops_dbxlate & CUI_ID_REG_U) ?
			"TID" : "tid"), ' ',
		       sizeof (iatt->att_name.db_att_name), 
		       iatt->att_name.db_att_name);
	    }
	    else if (jatt->opz_attnm.db_att_id == OPZ_FANUM)
	    {
		/* Like tids, if this is a function att, then we don't
		** have a name for this att, so make one up.
		*/
		STprintf(iatt->att_name.db_att_name, "fatt%d", eqcno);
		STmove(iatt->att_name.db_att_name, ' ', 
				sizeof (iatt->att_name.db_att_name), 
						iatt->att_name.db_att_name);
	    }
	    else
	    {
		/* Otherwise, we have a relation description, and it holds
		** a name for our attibute, so use it.
		*/
		STRUCT_ASSIGN_MACRO(*rel->rdr_attr[jatt->opz_attnm.db_att_id],
		    *qn->qen_atts[qn->qen_natts]);
	    }

	    
	    /* fill in the rest of the info in a fairly straight forward
	    ** way.
	    */
	    iatt->att_number = qn->qen_natts + 1;
	    if (qn->qen_natts == 0)
	    {   
		iatt->att_offset = 0;
	    }
	    else
	    {
		prev_iatt = qn->qen_atts[qn->qen_natts - 1];
		align = prev_iatt->att_width % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		iatt->att_offset = 
		    prev_iatt->att_offset + prev_iatt->att_width + align;
	    }
	    iatt->att_type = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
	    iatt->att_width = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;
	    iatt->att_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
	    iatt->att_flags = 0;
	    iatt->att_key_seq_number = 0;

#ifdef OPT_F045_QENPROW
	    if (opt_strace(global->ops_cb, OPT_F045_QENPROW) == TRUE)
	    {
		/* first, add a veritical bar to the output row */
		ops[0].opr_len = dv_vertbar.db_length;
		ops[0].opr_prec = dv_vertbar.db_prec;
		STRUCT_ASSIGN_MACRO(op_vertbar, ops[1]);
		opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
		ops[0].opr_offset += dv_vertbar.db_length;

		/* Now add the current attribute value to be converted into
		** a printable form.
		*/

		/* Fill in the output operand (ops[0]) */
		tempLength = ops[0].opr_len;
		status = adc_tmlen(global->ops_adfcb, 
			&cnode->opc_ceq[eqcno].opc_eqcdv,
			&tempLength, &prow_wctemp);
		ops[0].opr_len = tempLength;
		if (status != E_DB_OK)
		{
		    opx_verror(status, E_OP0795_ADF_EXCEPTION,
			global->ops_adfcb->adf_errcb.ad_errcode);
		}

		if (cnode->opc_ceq[eqcno].opc_attavailable == FALSE)
		{
		    opx_error(E_OP0888_ATT_NOT_HERE);
		}

		/* fill in the input operand (ops[1]) */
		ops[1].opr_offset = cnode->opc_ceq[eqcno].opc_dshoffset;
		ops[1].opr_dt = cnode->opc_ceq[eqcno].opc_eqcdv.db_datatype;
		ops[1].opr_prec = cnode->opc_ceq[eqcno].opc_eqcdv.db_prec;
		ops[1].opr_len = cnode->opc_ceq[eqcno].opc_eqcdv.db_length;

		ops[1].opr_base = 
			cadf.opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
		if (ops[1].opr_base < ADE_ZBASE || 
			ops[1].opr_base >= 
				    ADE_ZBASE + cadf.opc_qeadf->qen_sz_base
		    )
		{
		    /* if the CX base that we need hasn't already been added
		    ** to this CX then add it now.
		    */
		    opc_adbase(global, &cadf, QEN_ROW, 
				    cnode->opc_ceq[eqcno].opc_dshrow);
		    ops[1].opr_base = 
			cadf.opc_row_base[cnode->opc_ceq[eqcno].opc_dshrow];
		}

		opc_adinstr(global, &cadf, ADE_TMCVT, ADE_SMAIN, 2, ops, 1);

		ops[0].opr_offset += ops[0].opr_len;
		if ((prow_wctemp - ops[0].opr_len) > prow_worstcase)
		{
		    prow_worstcase = (prow_wctemp - ops[0].opr_len);
		}
	    }
#endif /* OPT_F045_QENPROW */
	}

#ifdef OPT_F045_QENPROW
	if (opt_strace(global->ops_cb, OPT_F045_QENPROW) == TRUE)
	{
	    /* add the final vertical bar to the output row */
	    ops[0].opr_len = dv_vertbar.db_length;
	    ops[0].opr_prec = dv_vertbar.db_prec;
	    STRUCT_ASSIGN_MACRO(op_vertbar, ops[1]);
	    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	    ops[0].opr_offset += dv_vertbar.db_length;

	    /* now add the row and close the compilation for prow */
	    ops[0].opr_offset += prow_worstcase;
	    opc_ptrow(global, &prowno, ops[0].opr_offset);
	    opc_adend(global, &cadf);
	}
#endif /* OPT_F045_QENPROW */
    }

#ifdef xOUT
    /* The number of attributes should be non-zero, unless it is
    ** ANY aggregate.
    */
    else
    {
        /* FIXME */
        opx_error( E_OP08AA_EMPTY_NODE );
    }
#endif /* xOUT */
}

/*{
** Name: OPC_FATTS	- Materialize all function atts that this node generates
**
** Description:
**      This routine compile the instructions necessary to evalutate the 
**      fucntion attributes that the given CO/QEN node is required to 
**      provide. 
**        
**      The first question that needs to be answered is: what are the function 
**      atts (fatts) that need to be materialized here? This is determined 
**      by comparing the eqcs that this node gets from below (either inner 
**      and/or outer nodes or from a relation) to the eqcs that this node
**      is supposed to return (the opo_eqcmap). Specifically, we calculate 
**      eqcs that this node *doesn't* get from below and we AND this with 
**      opo_maps->opo_eqcmap. This gives us the eqc fatts. 
**        
**      Once we have figured out which eqcs we will be materializing here, 
**      we then need to figure out which attribute (fatt) for each eqc 
**      will be materialized. This is done by looking for all the fatts 
**      that belong to each eqc. 
**        
**      Once we know which fatts are are to be materialized, it is a fairly 
**      simple matter to estimate the size of the CX, begin the compilation, 
**      compile the instructions, and end the compilation. 
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC.
**  cnode -
**	This holds info for the current state of compilation of the current
**	co/qen node.
**  co -
**	The co node that is currently being compiled. The field opo_maps->opo_eqcmap
**	is of the most importance, since it helps us figure out what fatts
**	to materialize.
**  qn -
**	The qen node that is currently being compiled. This is not being
**	used by this routine right now.
**  pqadf -
**	a pointer to an uninitialized pointer.
**  
**
** Outputs:
**  pqadf -
**	This points to the QEN_ADF, which holds the compiled CX, as well as
**	other info that is needed for the execution of the CX.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except for those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**      22-oct-86 (eric)
**          written
**      8-mar-89 (eric)
**          Changed the calculation of which eqc were available for orig
**          nodes to which eqc and atts, not just eqcs. This allows
**          orig nodes to include fatts in qualifications before they're
**          evaluated.
**	17-jul-89 (jrb)
**	    Correctly set precision fields for decimal project.
**	31-dec-91 (rickh)
**	    Fixed the merge of outer join and 6.4 code.
**	22-jul-93 (rickh)
**	    EQCs represented by function attributes should inherit the
**	    function attribute's datavalue.
**	26-oct-93 (rickh)
**	    In the function attribute compiler, we weren't correctly resetting
**	    the function attribute descriptor.  As a consequence, all function
**	    attributes for a QEN node were being compiled with the query tree 
**	    of the last function attribute for that node.
**	15-jan-94 (ed)
**	    Simplify interface by removing references to outer join
**	    descriptor
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	15-apr-94 (ed)
**	    b59588 Make special eqc function attribute available for calculation
**	    of other function attributes
**      28-apr-95 (inkdo01)
**          fix b68296 by assuring we've got right joinop attr to build func
**          code from
**	23-oct-97 (inkdo01)
**	    Added fattmap parm for QE_QP nodes to track func attrs compiled
**	    in current node (for 86478).
**	13-Dec-2005 (kschendel)
**	    Update adbegin call.
**      12-Feb-2009 (huazh01)
**          if a ceq[] element has already been filled in with an 
**          attribute number and its 'opc_eqavailable' is true, then
**          check if such attribute number is in the attrmap associated 
**          with the current eqcls in fatts_eqcmp, and don't overwrite 
**          it if it is in the attrmap. 
**          (bug 118363)
[@history_line@]...
[@history_template@]...
*/
VOID
opc_fatts(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		OPO_CO		*co,
		QEN_NODE	*qn,
		QEN_ADF		**pqadf,
		OPE_BMEQCLS	*fattmap)
{
	/* place standard data structures into vars to keep line length down */
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    OPC_EQ		*ceq = cnode->opc_ceq;
    DB_DATA_VALUE	*equivClassDataValue;

	/* Fatts_eqcmp holds the eqcs that should be evaluated to 
	** materialize the required fatts. Notavail_eqcmp holds the eqcs
	** that are not provided by the nodes and/or relations below the
	** current node.
	*/
    OPE_BMEQCLS		fatts_eqcmp;
    OPE_BMEQCLS		notavail_eqcmp;

	/* cadf is the ADF compilation tracking structure for the CX that
	** will hold the fatt instructions.
	*/
    OPC_ADF		cadf;

	/* Ninstr, nops, nconst, szconst, and maxbase are used to estimate
	** the size of the CX.
	*/
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;

	/* Ops is use to hold a description of the operands for the 
	** instructions to be compiled.
	*/
    ADE_OPERAND		ops[2];

	/* eqcno, jatt, fatt refer to the current eqcno, joinop attribute
	** description, and function att description for each fatt
	** being materialized.
	*/
    OPE_IEQCLS		eqcno;
    OPZ_ATTS		*jatt;
    OPZ_FATTS		*fatt;

	/* align is used to insure the alignment of data */
    i4			align;

	/* rowno holds the DSH row number that holds the fatts. */
    i4			rowno;

	/* tidatt is a dummy variable for opc_cqual() */
    i4			tidatt;

	/* fbase is the base of the fuction attribute array */
    OPZ_FT		*fbase;

	/* Ingres attribute number and the joinop attribute numbers. Iattno
	** is not currently used.
	*/
    i4			iattno;
    OPZ_IATTS		jattno;
       
        /* If this is DB_PR or DB_ORIG, save varno for later */
    OPV_IVARS           varno = -1;

    /* Lets init some variables to keep line length down */
    fbase = subqry->ops_funcs.opz_fbase;

    /* Figure out what eqc's we are *not* getting from the inner and/or 
    ** outer node(s) and put it in notavail_eqcmp. The first step in this
    ** is to figure out what eqcs we are getting from below and then to
    ** NOT this info.
    */
    MEfill(sizeof (notavail_eqcmp), 0, (PTR)&notavail_eqcmp);

    /*
    ** If this is an ORIG node or its child is (i.e., this is a
    ** ProjectRestrict, TIDjoin, or KEYjoin node), then don't trust
    ** the equivalence classes which OPF marked as available.  They may
    ** contain function attributes.  By hand, figure out what the real
    ** (i.e., non-function) attributes are.
    */

    if ( ( co->opo_sjpr == DB_ORIG ) ||
	 ( co->opo_sjpr == DB_PR ) ||
	 ( co->opo_inner != NULL && co->opo_inner->opo_sjpr == DB_ORIG ) )
    {
        if (co->opo_sjpr == DB_ORIG) varno = co->opo_union.opo_orig;
         else if (co->opo_sjpr == DB_PR && co->opo_outer && 
             co->opo_outer->opo_sjpr == DB_ORIG) 
                   varno = co->opo_outer->opo_union.opo_orig;
                                 /* save varno for ORIG's */
	for (eqcno = 0; eqcno < subqry->ops_eclass.ope_ev; eqcno += 1)
	{
	    /* If both the eqc and the att for that eqc is available
	    ** then remember that. An eqc can be available, but not it's
	    ** att for function atts (since they can be evaluated the
	    ** hard way (recursively) in opc_cqual()). In which case,
	    ** we want to evaluate the fatt here if we can.
	    */
	    if (ceq[eqcno].opc_eqavailable == TRUE &&
		ceq[eqcno].opc_attavailable == TRUE
		)
	    {
		BTset((i4)eqcno, (char *)&notavail_eqcmp);
	    }
	}
    }
    else
    {
	if (co->opo_inner != NULL)
	{
	    MEcopy((PTR)&co->opo_inner->opo_maps->opo_eqcmap, sizeof (notavail_eqcmp),
		(char *)&notavail_eqcmp);
	}

    }

    if ( ( co->opo_sjpr != DB_PR ) && ( co->opo_outer != NULL ) )
    {
        BTor((i4)(BITS_IN(notavail_eqcmp)), (char *)&co->opo_outer->opo_maps->opo_eqcmap, 
	    (char *)&notavail_eqcmp);
    }
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
    {	/* make special eqcs available for function attribute
	** calculations which may depend on them */
	BTor((i4)(BITS_IN(notavail_eqcmp)), 
	    (char *)co->opo_variant.opo_local->opo_special,
	    (char *)&notavail_eqcmp);
    }

    BTnot((i4)BITS_IN(notavail_eqcmp), (char *)&notavail_eqcmp);

    /* Now let's figure out what function atts should be created by this 
    ** qen node. This is done by first getting all the eqc's that we aren't
    ** getting from the inner and/or outer node(s) and 'and'ing this with the
    ** eqc's that the current node wants to return. The result is the eqc's
    ** for fatts that this node creates.
    */
    MEcopy((PTR)&notavail_eqcmp, sizeof (fatts_eqcmp), 
	(char *)&fatts_eqcmp);
    BTand((i4)(BITS_IN(fatts_eqcmp)), (char *)&co->opo_maps->opo_eqcmap, 
	(char *)&fatts_eqcmp);


    /*
    ** In the outer join case, there may be some special OJ eqcs;
    ** let's eliminate them from the function attribute map, since
    ** we have already dealt with them.
    */
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
    {
	OPE_BMEQCLS	tmp;

	MEcopy((PTR)co->opo_variant.opo_local->opo_special, 
	    sizeof (tmp), (char *)&tmp);

	BTnot((i4)BITS_IN(tmp), (char *)&tmp);
	BTand((i4)(BITS_IN(fatts_eqcmp)), (char *)&tmp, 
	    (char *)&fatts_eqcmp);
    }

    /* if (one or more eqc is set in the resulting map) */
    if (BTcount((char *)&fatts_eqcmp, (i4)subqry->ops_eclass.ope_ev) <= 0)
    {
	*pqadf = NULL;
    }
    else
    {
	/* We need to figure out the attributes that we are creating,
	** using fatts_eqcmp.
	*/
	for (eqcno = -1; 
		(eqcno = BTnext((i4)eqcno, (char *)&fatts_eqcmp, 
				(i4)subqry->ops_eclass.ope_ev)) != -1;
	    )
	{
	    OPZ_BMATTS		*attmap;
	    OPE_BMEQCLS		need_eqcmp;
	    OPZ_IATTS		attno;
            bool                foundAttr = FALSE;

	    /* get the joinop attribute number for the fatt to be created */
	    attmap = (OPZ_BMATTS *)
		&subqry->ops_eclass.ope_base->ope_eqclist[eqcno]->ope_attrmap;
	    for (attno = -1;
		    (attno = BTnext((i4)attno, (char *)attmap, 
					(i4)subqry->ops_attrs.opz_av)) != -1;
		)
	    {
		jatt = subqry->ops_attrs.opz_base->opz_attnums[attno];

		/* If this is a function attibute */
		if (jatt->opz_func_att >= 0 && (varno < 0 ||
                        jatt->opz_varnm == varno))  /* get correct attno */
		{
		    MEcopy((PTR)&fbase->opz_fatts[jatt->opz_func_att]->opz_eqcm,
			    sizeof (need_eqcmp), (PTR)&need_eqcmp);
		    BTand((i4)(BITS_IN(need_eqcmp)), (char *)&notavail_eqcmp,
			(char *)&need_eqcmp);

                    foundAttr = FALSE;
                    if (ceq[eqcno].opc_attno != OPC_NOATT &&
                        ceq[eqcno].opc_eqavailable)
                    {
                        OPZ_IATTS idx;

                        for (idx = -1; 
                               (idx = BTnext((i4)idx, (char *)attmap, 
                                 (i4)subqry->ops_attrs.opz_av)) != -1;)
                        {
                            if (idx == ceq[eqcno].opc_attno)
                            {
                               foundAttr = TRUE; 
                               break; 
                            }
                        }
                    }

		    /* if there aren't any eqcs that we need for this
		    ** fatt then we have found the attno that we are looking
		    ** for.
                    **
                    ** don't overwrite the opc_attno if it is already in
                    ** the attrmap associated with current eqc. (b118363)
		    */
		    if (BTnext((i4)-1, (char *)&need_eqcmp, 
			(i4)subqry->ops_eclass.ope_ev) == -1 &&
                        !foundAttr)
		    {
			ceq[eqcno].opc_attno = attno;
			ceq[eqcno].opc_eqavailable = TRUE;
			break;
		    }
		}
	    }
	}

	/* estimate the size of the compilation; */
	ninstr = 0;
	nops = 0;
	nconst = 0;
	szconst = 0;
	for (eqcno = -1; 
		(eqcno = BTnext((i4)eqcno, (char *)&fatts_eqcmp,
				(i4)subqry->ops_eclass.ope_ev)) != -1;
	    )
	{
	    /* get the joinop attribute number for the fatt to be created */
	    /* get the joinop attribute number for the fatt to be created */
	    if (ceq[eqcno].opc_eqavailable == FALSE)
	    {
		opx_error(E_OP0889_EQ_NOT_HERE);
	    }

	    if (ceq[eqcno].opc_attno == OPC_NOATT)
	    {
		/* FIXME */
		opx_error( E_OP08A6_NO_ATTRIBUTE );
	    }

	    jatt = 
		subqry->ops_attrs.opz_base->opz_attnums[ceq[eqcno].opc_attno];

	    if (jatt->opz_func_att < 0)
		opx_error(E_OP0888_ATT_NOT_HERE);

	    fatt = subqry->ops_funcs.opz_fbase->opz_fatts[jatt->opz_func_att];

	    /* estimate the size required for the qtree for the current fatt */
	    opc_cxest(global, cnode, fatt->opz_function, 
					    &ninstr, &nops, &nconst, &szconst);

	    /* Now add in the size required to copy the results of the tree
	    ** into the fatt tuple.
	    */
	    ninstr += 1;
	    nops += 2;
	}

	/* begin the adf compilation; */
	max_base = cnode->opc_below_rows + 2;
        opc_adalloc_begin(global, &cadf, pqadf, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

	/* init ops[0], choosing a row for the fatts to go into; */
	opc_adbase(global, &cadf, QEN_ROW, qp->qp_row_cnt);
	ops[0].opr_base = cadf.opc_row_base[qp->qp_row_cnt];
	ops[0].opr_offset = 0;

	/* for (each eqcno in the resulting map) */
	for (eqcno = -1; 
		(eqcno = BTnext((i4)eqcno, (char *)&fatts_eqcmp,
				(i4)subqry->ops_eclass.ope_ev)) != -1;
	    )
	{
	    /* get the qtree for the fatt */

	    if (ceq[eqcno].opc_attno == OPC_NOATT)
	    {
		/* FIXME */
		opx_error( E_OP08A6_NO_ATTRIBUTE );
	    }
	    equivClassDataValue = &ceq[eqcno].opc_eqcdv;

	    /* retrieve the function attribute descriptor */

	    jatt = 
		subqry->ops_attrs.opz_base->opz_attnums[ceq[eqcno].opc_attno];
	    fatt = subqry->ops_funcs.opz_fbase->opz_fatts[jatt->opz_func_att];

	    /* fill in the target data value based on this function att */

	    opc_getFattDataType( global, ceq[eqcno].opc_attno, &ops[ 1 ] );

	    /* compile the qtree for the fatt into ops[1] */

	    opc_cqual(global, cnode, fatt->opz_function->pst_right, 
		&cadf, ADE_SMAIN, &ops[1], &tidatt);

	    /* compile an MEcopy instruction to move from ops[1] to ops[0]; */
	    ops[0].opr_dt = ops[1].opr_dt;
	    ops[0].opr_prec = ops[1].opr_prec;
	    ops[0].opr_len = ops[1].opr_len;
	    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);

	    /* Now that we have compiled this function att (with the exception
	    ** of special `outer join' function attributes, lets record where
	    ** it is in to OPC_EQ array.
	    */
	    ceq[eqcno].opc_attavailable = TRUE;
	    ceq[eqcno].opc_dshrow = qp->qp_row_cnt;
	    ceq[eqcno].opc_dshoffset = ops[0].opr_offset;
	    equivClassDataValue->db_datatype = ops[1].opr_dt;
	    equivClassDataValue->db_prec = ops[1].opr_prec;
	    equivClassDataValue->db_length = ops[1].opr_len;

	    /* Set bit in caller's map (for bug 86478) */
	    if (fattmap) BTset((i4)eqcno, (char *)fattmap);

	    /* Now that we have everythig else done, lets increase the tuple
	    ** size.
	    */
	    align = ops[1].opr_len % sizeof (ALIGN_RESTRICT);
	    if (align != 0)
		align = sizeof (ALIGN_RESTRICT) - align;
	    ops[0].opr_offset += ops[1].opr_len + align;
	}

	/* end the adf compilation; */
	opc_adend(global, &cadf);

	opc_ptrow(global, &rowno, ops[0].opr_offset);
	qn->qen_frow = rowno;
    }
}

/*{
** Name: opc_getFattDataType - Fill in data value for a function attribute
**
** Description:
**        This routine looks up the type, length, and precision of a
**        function attribute and returns them in an ADE_0PERAND.
**        
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC.
**  attno
**	Attribute number of this function attribute
**  adeOp
**      And ADE_OPERAND into which to stuff the type, length, and precision
**	of this function attribute.
**
** Outputs:
**	Returns:
**  adeOp
**      And ADE_OPERAND into which to stuff the type, length, and precision
**	of this function attribute.
**
**	Exceptions:
**	    none, except for those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**	25-feb-92 (rickh)
**	    Created from a paragraph in opc_fatts.
**	31-Oct-2007 (kschendel) b122118
**	    Copy the collID too, just in case.
[@history_line@]...
[@history_template@]...
*/
VOID
opc_getFattDataType(
		OPS_STATE	*global,
		i4		attno,
		ADE_OPERAND	*adeOp)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPZ_ATTS		*jatt;
    OPZ_FATTS		*fatt;

    jatt = 
        subqry->ops_attrs.opz_base->opz_attnums[ attno ];
    fatt = subqry->ops_funcs.opz_fbase->opz_fatts[jatt->opz_func_att];

    /*
    ** The qtree that opz_function points to a resdom that has
    ** the expression that we're interested in off of the pst_right.
    ** The only value that the resdom holds to the give the data type
    ** and length information.
    */

    adeOp->opr_dt = fatt->opz_function->pst_sym.pst_dataval.db_datatype;
    adeOp->opr_prec = fatt->opz_function->pst_sym.pst_dataval.db_prec;
    adeOp->opr_len = fatt->opz_function->pst_sym.pst_dataval.db_length;
    adeOp->opr_collID = fatt->opz_function->pst_sym.pst_dataval.db_collID;
}

/*{
** Name: OPC_OJSPFATTS	- Materialize all special OJ function atts that this
**			  node generates.
**
** Description:
**      This routine compile the instructions necessary to evalutate the 
**      function attributes that the given CO/QEN node is required to 
**      provide. 
**        
**      The first question that needs to be answered is: what are the function 
**      atts (fatts) that need to be materialized here? This is determined 
**      by comparing the eqcs that this node gets from below (either inner 
**      and/or outer nodes or from a relation) to the eqcs that this node
**      is supposed to return (the opo_eqcmap). Specifically, we calculate 
**      eqcs that this node *doesn't* get from below and we AND this with 
**      opo_maps->opo_eqcmap. This gives us the eqc fatts. 
**        
**      Once we have figured out which eqcs we will be materializing here, 
**      we then need to figure out which attribute (fatt) for each eqc 
**      will be materialized. This is done by looking for all the fatts 
**      that belong to each eqc. 
**        
**      Once we know which fatts are are to be materialized, it is a fairly 
**      simple matter to estimate the size of the CX, begin the compilation, 
**      compile the instructions, and end the compilation. 
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC.
**  cnode -
**	This holds info for the current state of compilation of the current
**	co/qen node.
**  co -
**	The co node that is currently being compiled. The field opo_maps->opo_eqcmap
**	is of the most importance, since it helps us figure out what fatts
**	to materialize.
**  qn -
**	The qen node that is currently being compiled. This is not being
**	used by this routine right now.
**  pqadf -
**	a pointer to an uninitialized pointer.
**  
**
** Outputs:
**  pqadf -
**	This points to the QEN_ADF, which holds the compiled CX, as well as
**	other info that is needed for the execution of the CX.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except for those that opx_error() raises.
**
** Side Effects:
**	    none
**
** History:
**      26-feb-90 (stec)
**          written
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	22-dec-03 (inkdo01)
**	    Added ojspecialEQCrow parm.
*/
VOID
opc_ojspfatts(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		i4		*ojspecialEQCrow)
{
	/* place standard data structures into vars to keep line length down */
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPO_CO		*co = cnode->opc_cco;

	/* Fatts_eqcmp holds the eqcs that should be evaluated to 
	** materialize the required fatts.
	*/
    OPE_BMEQCLS		fatts_eqcmp;

	/* eqcno, jatt, fatt refer to the current eqcno, joinop attribute
	** description, and function att description for each fatt
	** being materialized.
	*/
    OPE_IEQCLS		eqcno;
    OPZ_ATTS		*jatt;
    OPZ_FATTS		*fatt;

	/* align is used to insure the alignment of data */
    i4			align;

	/* fbase is the base of the fuction attribute array */
    OPZ_FT		*fbase;

	/* rowno holds the DSH row number that holds the fatts. */
    i4			rowno;

    i4	    offset;

    *ojspecialEQCrow = -1;

    /* If a regular join, just return. */
    if (co->opo_variant.opo_local->opo_ojid == OPL_NOOUTER)
    {
	/* Not an outer join. */
	return;	
    }

    /* Lets init some variables to keep line length down */
    fbase = subqry->ops_funcs.opz_fbase;

    MEcopy((PTR)co->opo_variant.opo_local->opo_special, 
	sizeof (fatts_eqcmp), (char *)&fatts_eqcmp);

    /* if (one or more eqc is set in the resulting map) */
    if (BTcount((char *)&fatts_eqcmp, (i4)BITS_IN(fatts_eqcmp)) > 0)
    {
	/* We need to figure out the attributes that we are creating,
	** using fatts_eqcmp.
	*/
	for (eqcno = -1; 
		(eqcno = BTnext((i4)eqcno, (char *)&fatts_eqcmp, 
		(i4)subqry->ops_eclass.ope_ev)) != -1;
	    )
	{
	    OPZ_BMATTS		*attmap;
	    OPE_BMEQCLS		need_eqcmp;
	    OPZ_IATTS		attno;

	    /* get the joinop attribute number for the fatt to be created */
	    attmap = (OPZ_BMATTS *)
		&subqry->ops_eclass.ope_base->ope_eqclist[eqcno]->ope_attrmap;
	    for (attno = -1;
		    (attno = BTnext((i4)attno, (char *)attmap, 
					(i4)subqry->ops_attrs.opz_av)) != -1;
		)
	    {
		ceq[eqcno].opc_attno = attno;
		ceq[eqcno].opc_eqavailable = TRUE;
		break;
	    }
	}

	offset = 0;

	/* for (each eqcno in the resulting map) */
	for (eqcno = -1; 
	     (eqcno = BTnext((i4)eqcno, (char *)&fatts_eqcmp,
		(i4)subqry->ops_eclass.ope_ev)) != -1;
	    )
	{
	    if (ceq[eqcno].opc_attno == OPC_NOATT)
	    {
		/* FIXME */
		opx_error( E_OP08A6_NO_ATTRIBUTE );
	    }

	    jatt = 
		subqry->ops_attrs.opz_base->opz_attnums[ceq[eqcno].opc_attno];
	    fatt = subqry->ops_funcs.opz_fbase->opz_fatts[jatt->opz_func_att];

	    /* Verify that it is a special OJ eqc function attribute. */ 
	    if (fatt->opz_mask & OPZ_OJFA)
	    {
		ceq[eqcno].opc_attavailable = TRUE;
		ceq[eqcno].opc_dshrow = qp->qp_row_cnt;
		ceq[eqcno].opc_dshoffset = offset;
		STRUCT_ASSIGN_MACRO(jatt->opz_dataval, ceq[eqcno].opc_eqcdv);

		align = sizeof(OPL_OJSPEQC) % sizeof (ALIGN_RESTRICT);
		if (align != 0)
		    align = sizeof (ALIGN_RESTRICT) - align;
		offset += sizeof(OPL_OJSPEQC) + align;
	    }
	}

	opc_ptrow(global, ojspecialEQCrow, offset);
    }
}
