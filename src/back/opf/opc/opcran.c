/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/
/*
NO_OPTIM=sos_us5
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
#include    <clfloat.h>
#include    <bt.h>
#include    <me.h>
#include    <st.h>
#include    <ex.h>

/* external routine declarations definitions */
#define        OPC_RAN		TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCRAN.C - Routines to compile remote action node (ie. QEN_QP).
**
**  Description:
**
**
**  History:
**      4-mar-87 (eric)
**          written
**	30-mar-89 (paul)
**	    Add resource contyrol support to opc_lsahd_build
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual in opc_ran_build.
**	24-may-90 (stec)
**	    Modified opc_ran_build to return only printing resdoms.
**	25-jun-90 (stec)
**	    Changed code in opc_lsahd_build() to avoid integer overflow
**	    problems. Fixes bug 31067.
**	13-jul-90 (stec)
**	    Changed code in opc_lsahd_build() to avoid floating point 
**	    overflow problems.
**	10-jan-91 (stec)
**	    Added include for <clfloat.h>. Changed code in opc_lsahd_build().
**	02-jul-91 (nancy)
**	    Bug 36834, -- modified opc_lsahd_build(), see comment below.
**	09-sep-91 (anitap)
**          Bug 38392, -- modified opc_ran_build(). Removed the line 
**          global->ops_cstate.opc_rows->opc_oprev->opc_olen = qn->qen_rsz
**          which was allocating the size of previous row buffer to the 
**          current one.
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**      16-jul-92 (anitap)
**          Cast naked NULL in opc_ran_build() so that compilers will consider
**          this as a pointer.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      09-aug-93 (terjeb)
**          Type cast the values in the conditional tests involving dissimilar
**	    types like float and MAXI4. The lack of these casts caused
**	    floating point exceptions on a certain machine in opc_lsahd_build().
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	28-jan-97 (toumi01)
**	    Remove (again) assignment of qn->qen_rsz to opc_olen that
**	    fixed bug 38392 (see above).  Integrations dropped deletion.
**	13-aug-97 (ocoro02)
**	    Added NO_OPTIM for sos_us5 as a workaround to bug 83054.
**      24-Jun-1999 (hanal04)
**          Use resdom information to construct the ceq elements in
**          opc_ran_build() to ensure the correct nullability of the
**          output columns. b97219.
**	21-May-2003 (bonro01)
**	    Add prototype for opc_lsahd_build() to prevent SEGV on 
**	    HP Itanium and other 64-bit platforms.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanup, delete unused gtqual parameters.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_ran_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_lsahd_build(
	OPS_STATE *global,
	OPS_SUBQUERY *subqry_list,
	QEF_AHD **pahd_list,
	i4 is_top_ahd);
static void opc_qfatt_search(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE **orig_qual,
	OPE_BMEQCLS *fattmap);
static void opc_set_resettable(
	QEN_NODE *node,
	bool resettable);


/*{
** Name: OPC_RAN_BUILD	- Build a QEN_QP (remote action) node.
**
** Description:
**      This routine builds a QEN_QP node; I refer to these nodes as remote 
**      action nodes, since I feel this more descriptive of what's going on. 
[@comment_line@]...
**
** Inputs:
**  global
**	A pointer the top level OPF state variable for this query.
**  global->ops_cstat.opc_subqry
**	A pointer to the current subquery struct that is a part of the 
**	current query.
**  global->ops_rangetab
**	The global range table for this query.
**  cnode
**	Pointer to the compilation info for the current co/qen node
**  cnode->opc_cco
**	A pointer to the CO node that is OPFs representation of the remote
**	action to be compiled.
**  cnode->opc_ceq
**	Pointers to arrays that hold current eqc and attribute information
**	that has been initialized to zero.
**  cnode->opc_qennode
**	A pointer to the QEN_NODE to be filled in.
**
** Outputs:
**  cnode->opc_qennode->node_qen.qen_qp
**	The .qen_qp struct will be completely filled in.
**  cnode->opc_qennode->qen_row
**  cnode->opc_qennode->qen_rsz
**	Qen_row and qen_rsz will be filled in to hold the info on how big and
**	where the result row is.
**  cnode->opc_ceq
**	These will hold the descriptions of the data being returned.
**  cnode->opc_cco->opo_maps->opo_eqcmap
**	This will be filled in with the correct eqc map, as OPF doesn't do this.
**  global->ops_cstate.opc_qp->qp_row_cnt
**  global->ops_cstate.opc_rows
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none EJLFIX: what about errors?
**
** Side Effects:
**	    none
**
** History:
**      4-mar-87 (eric)
**          written
**      16-June-87 (eric)
**          added support for opz_attnm.db_att_id == OPZ_SNUM. In this case,
**          the remote action is a subselect and, in this case, one attribute
**          will be coming up. Therefore, we pretend that the ingres attno is
**          one and not OPZ_SNUM.
**      8-aug-88 (eric)
**          I fixed the resdom number-attno consitency check.
**      23-sept-88 (eric)
**          add a temporary work around for a possible QEF bug.
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual.
**	24-may-90 (stec)
**	    When a remote action node is built, only printing resdoms
**	    should be returned.
**      09-sep-91 (anitap)
**	    Removed the line global->ops_cstate.opc_rows->opc_oprev->opc_olen =
**	    qn->qen_rsz which was allocating the size of previous row buffer to
**	    the current one. Fix for bug 38392.	
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**      16-jul-92 (anitap)
**          Cast the naked NULL being passed to opc_gtqual().
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	13-may-96 (inkdo01)
**	    Don't set eqcmap for multi-var func attrs (bug 74003).
**	28-jan-97 (toumi01)
**	    Remove (again) assignment of qn->qen_rsz to opc_olen that
**	    fixed bug 38392 (see above).  Integrations dropped deletion.
**	23-oct-97 (inkdo01)
**	    Add code to track func attrs computed in this node, but also 
**	    ref'd in the qual code (bug 86478).
**	27-oct-97 (inkdo01)
**	    Minor fix to previous change. Copy qualification parse tree 
**	    fragment prior to diddling it.
**      24-Jun-1999 (hanal04)
**          Use the db_datatype and db_length of the resdom when constructing
**          the ceq as the output resdom may have been changed to nullable
**          when the underlying column is non-nullable (i.e. we have an
**          outer-join). b97219.
**	28-Sep-2007 (kschendel) SIR 122512
**	    Propagate contains-hashop flag from any QP child up to QP parent
**	    action header.
*/
VOID
opc_ran_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPS_SUBQUERY	*rsubqry;
    OPO_CO		*tco;
    OPO_CO		*co;
    OPC_EQ		*ceq = cnode->opc_ceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_QP		*ran = &cnode->opc_qennode->node_qen.qen_qp;
    QEF_AHD		*parent_ahd = subqry->ops_compile.opc_ahd;
    QEF_AHD		*ahd;
    OPV_GRV		*opvgrv;
    OPV_VARS		*opvvar;
    OPV_SUBSELECT	*opvsub;
    PST_QNODE		*resdom;
    OPZ_IATTS		jattno;
    i4			iattno;
    OPZ_ATTS		*jatt;
    OPE_IEQCLS		eqcno;
    OPC_TGINFO		tginfo;
    OPE_BMEQCLS		qual_eqcmp;
    OPE_BMEQCLS		fattmap;
    PST_QNODE		*orig_qual;

    /* Lets figure out where the real orig node is and point opc_cco at
    ** it. This is because this code is almost never interested in the
    ** PR node.
    */
    tco = cnode->opc_cco;
    if (tco->opo_outer != NULL)
    {
	co = tco->opo_outer;
    }
    else if (tco->opo_inner != NULL)
    {
	co = tco->opo_inner;
    }
    else
    {
	co = tco;
    }
    cnode->opc_cco = co;

    /* Set up some variables that will keep line length down */
    opvvar = subqry->ops_vars.opv_base->opv_rt[co->opo_union.opo_orig];
    opvsub = opvvar->opv_subselect;
    opvgrv = global->ops_rangetab.opv_base->opv_grv[opvsub->opv_sgvar];

    opc_lsahd_build(global, opvgrv->opv_gsubselect->opv_subquery, 
							&ran->qp_act, FALSE);
    opvgrv->opv_gsubselect->opv_ahd = ran->qp_act;

    /* Now that we have completed the action headers, lets make the
    ** actions into a non-circular doubly linked list, as QEF expects it.
    ** I have made it a circular list up til now since that makes OPC's life
    ** easier.
    */
    ran->qp_act->ahd_prev->ahd_next = NULL;
    ran->qp_act->ahd_prev = NULL;

    /* Propagate the CONTAINS_HASHOP flag from any of the QP child
    ** actions, up to the QP parent action.  Don't bother if the
    ** QP parent already shows contains-hashop.
    */
    if ((parent_ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP) == 0)
    {
	ahd = ran->qp_act;
	while (ahd != NULL)
	{
	    if (ahd->ahd_flags & QEA_NODEACT
	      && ahd->qhd_obj.qhd_qep.ahd_qepflag & AHD_CONTAINS_HASHOP)
	    {
		parent_ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_CONTAINS_HASHOP;
		break;
	    }
	    ahd = ahd->ahd_next;
	}
    }

    /* find the first subquery struct in the remote action that contains
    ** a query tree
    */
    for (rsubqry = opvgrv->opv_gsubselect->opv_subquery; 
	    rsubqry != NULL;
	    rsubqry = rsubqry->ops_next
	)
    {
	if (rsubqry->ops_root != NULL)
	{
	    break;
	}
    }

    /* If there isn't any subquery struct with a qtree then we've got an error
    ** since QEN_QP nodes are supposed to return data.
    */
    if (rsubqry == NULL)
    {
	opx_error(E_OP0687_NOQTREE);
    }

    /* lets find out where the data is coming from */
    opc_tloffsets(global, rsubqry->ops_root->pst_left, &tginfo, FALSE);
    qn->qen_rsz = tginfo.opc_tlsize;

    /* lets find out how many attributes we will be returning, and set
    ** co->opo_maps->opo_eqcmap correctly since OPF doesn't
    */
    qn->qen_natts = 0;
    MEfill(sizeof (co->opo_maps->opo_eqcmap), (u_char)0, (PTR)&co->opo_maps->opo_eqcmap);
    for (jattno = 0; jattno < subqry->ops_attrs.opz_av; jattno += 1)
    {
	OPZ_FATTS	*fatt;
	jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];
	if (jatt->opz_func_att >= 0) fatt = subqry->ops_funcs.opz_fbase->
					opz_fatts[jatt->opz_func_att];
	else fatt = NULL;

	/* if (this jatt belongs to the current gvarno && isn't
	** a multi-variate function attribute) */
	if (jatt->opz_gvar == opvsub->opv_sgvar && 
		(jatt->opz_attnm.db_att_id > 0 ||
		    jatt->opz_attnm.db_att_id == OPZ_FANUM &&
		    fatt && fatt->opz_type != OPZ_MVAR ||
		    jatt->opz_attnm.db_att_id == OPZ_SNUM
		)
	    )
	{
	    qn->qen_natts += 1;
	    BTset((i4)jatt->opz_equcls, (char *)&co->opo_maps->opo_eqcmap);
	}
    }

    /* now that we know the number of att that are being returned and the
    ** result tuple size, lets add a row to hold the result tuple.
    */
    opc_ptrow(global, &qn->qen_row, qn->qen_rsz);

    /* Lets find out where the attributes
    ** will go and fill in the ceq info.
    */
    for (jattno = 0; jattno < subqry->ops_attrs.opz_av; jattno += 1)
    {
	jatt = subqry->ops_attrs.opz_base->opz_attnums[jattno];

	/* if (this jatt belongs to the current gvarno) */
	if (jatt->opz_gvar == opvsub->opv_sgvar && 
		(jatt->opz_attnm.db_att_id > 0 ||
		    jatt->opz_attnm.db_att_id == OPZ_SNUM
		)
	    )
	{
	    eqcno = jatt->opz_equcls;

	    if (jatt->opz_attnm.db_att_id == OPZ_SNUM)
	    {
		iattno = 1;
	    }
	    else
	    {
		iattno = jatt->opz_attnm.db_att_id;
	    }

	    /* Check for non-printing resdoms here. If any are to be
	    ** returned, flag error (bug 20948).
	    */
	    for (resdom = rsubqry->ops_root->pst_left;
		    resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
		    resdom = resdom->pst_left
		)
	    {
		if ((resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno == iattno) &&
		   (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
		{
		    break;
		}
	    }
	    if (resdom == NULL ||
		resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno != iattno ||
		!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		)
	    {
		opx_error(E_OP068C_BAD_TARGET_LIST);
	    }

	    /* fill in the eqc and att info; */
	    ceq[eqcno].opc_eqavailable = TRUE;
	    ceq[eqcno].opc_attno = jattno;

	    ceq[eqcno].opc_attavailable = TRUE;
	    ceq[eqcno].opc_dshrow = qn->qen_row;
	    ceq[eqcno].opc_dshoffset = 
		tginfo.opc_tsoffsets[iattno].opc_toffset;
	    STRUCT_ASSIGN_MACRO(resdom->pst_sym.pst_dataval,
                                ceq[eqcno].opc_eqcdv);
	}
    }

    MEfill(sizeof (fattmap), (u_char)0, (PTR)&fattmap);
    opc_fatts(global, cnode, co, qn, &qn->qen_fatts, &fattmap);

    /* Now that the available eqcs have been filled in, lets get the 
    ** qualification that we need to apply at this node.
    */
    opc_gtqual(global, cnode,
	tco->opo_variant.opo_local->opo_bmbf,
	&orig_qual, &qual_eqcmp);
    if (orig_qual == NULL)
    {
	ran->qp_qual = (QEN_ADF *)NULL;
    }
    else
    {
	if (qn->qen_fatts != NULL)
	{
	    /* If func atts are computed here, copy the qual parse tree
	    ** fragment and analyze it to replace funcatt refs by 
	    ** underlying expression (to fix 86478). */
	    opv_copytree(global, &orig_qual);
	    opc_qfatt_search(global, cnode, &orig_qual, &fattmap);
	}
	/* Then, compile the ADF code to qualify the tuple */
	opc_ccqtree(global, cnode, orig_qual, &ran->qp_qual, -1);
    }

    /* Since we played around with the setting of opc_cco at the beginning
    ** of this routine, we must restore it to its original setting before
    ** we return.
    */
    cnode->opc_cco = tco;
}

/*{
** Name: OPC_LSAHD_BUILD	- Build a list of QEF_AHD structs
**
** Description:
**		EJLFIX: Put me into OPCAHD.C, rename me, and 
**			force opc_qp_build() to call me.
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
**      4-mar-87 (eric)
**          written
**	30-mar-89 (paul)
**	    Add support for setting query cost estimates in the first
**	    action for a statement. This information is used for resource
**	    control.
**	25-jun-90 (stec)
**	    Changed code to avoid integer overflow problems. Local variables
**	    for calculation of various costs will be defined in the same way
**	    as OPF defines them (floats) and then before action header is
**	    initialized, a check will be performed to verify that overflow will
**	    not occur; if the values are too large, a maximum possible value 
**	    for i4 will be used for initialization. Fixes bug 31067.
**	13-jul-90 (stec)
**	    Changed code to avoid floating point overflow problems.
**	10-jan-91 (stec)
**	    Changed code to use new defines in <clfloat.h> (FMAX -> DBL_MAX).
**	    Modified code to use opc_exhandle() to handle floating-point
**	    number overflow.
**	02-jul-91 (nancy)
**	    bug 36834 -- added QEA_LOAD to list of actions to fill in cost
**	    estimates to be used later for resource control checks so that
**	    create table as select may enforce resource control.
**      09-aug-93 (terjeb)
**          Type cast the values in the conditional tests involving dissimilar
**	    types like float and MAXI4. The lack of these casts caused
**	    floating point exceptions on a certain machine.
**	16-May-2010 (kschendel) b123565
**	    For "top" actions, re-walk the generated query plan and set
**	    the node-is-resettable flag on each node, as needed.
*/
VOID
opc_lsahd_build(
		OPS_STATE	*global,
		OPS_SUBQUERY	*subqry_list,
		QEF_AHD		**pahd_list,
		i4		is_top_ahd)
{
    QEF_AHD	    *ahd_list;
    OPS_SUBQUERY    *outer_sq;
    OPS_SUBQUERY    *union_sq;
    QEF_AHD	    *ahd;
    QEF_AHD	    *tmpahd;
    QEF_AHD	    *proj_ahd;
    OPS_SUBQUERY    *save_subqry;
    OPO_CPU	    cpu_cost = 0.0;
    OPO_BLOCKS	    dio_cost = 0.0;
    OPO_TUPLES	    row_cost = 0.0;
    OPO_COST	    cost_cost = 0.0;
    OPO_BLOCKS	    page_cost = 0.0;
    
    ahd_list = NULL;
    proj_ahd = NULL;
    save_subqry = global->ops_cstate.opc_subqry;

    /* For resource control checking, we must create a summary of the cost  */
    /* to run the user query. This cost is the sum of the costs to run each */
    /* of the subqueries. We keep running totals of the costs for each	    */
    /* subquery and place these in the first action header compiled for	    */
    /* this user statement. */
    
    for (outer_sq = subqry_list;
	    outer_sq != NULL; 
	    outer_sq = outer_sq->ops_next
	)
    {
	for (union_sq = outer_sq; 
		union_sq != NULL; 
		union_sq = union_sq->ops_union
	    )
	{
	    /*
	    ** Cost estimates only exist for query 
	    ** plans involving variables (tables).
	    */
	    if (union_sq->ops_bestco)
	    {
		EX_CONTEXT	ex;

		/* Code below will prevent floating point number overflow. */

		if (EXdeclare(opc_exhandler, &ex) != OK)
		{
		    cost_cost = FLT_MAX;
		}
		else
		{
		    cost_cost += union_sq->ops_cost;
		}
		EXdelete();

		if (EXdeclare(opc_exhandler, &ex) != OK)
		{
		    cpu_cost = FLT_MAX;
		}
		else
		{
		    cpu_cost += union_sq->ops_bestco->opo_cost.opo_cpu;
		}
		EXdelete();

		if (EXdeclare(opc_exhandler, &ex) != OK)
		{
		    dio_cost = FLT_MAX;
		}
		else
		{
		    dio_cost += union_sq->ops_bestco->opo_cost.opo_dio;
		}
		EXdelete();

		if (EXdeclare(opc_exhandler, &ex) != OK)
		{
		    page_cost = FLT_MAX;
		}
		else
		{
		    page_cost +=
			union_sq->ops_bestco->opo_cost.opo_pagestouched;
		}
		EXdelete();

		if (EXdeclare(opc_exhandler, &ex) != OK)
		{
		    row_cost = FLT_MAX;
		}
		else
		{
		    row_cost += union_sq->ops_bestco->opo_cost.opo_tups;
		}
		EXdelete();

	    }
	    
 	    global->ops_cstate.opc_subqry = union_sq;
	    if (is_top_ahd)
	    {
		global->ops_cstate.opc_topahd = NULL;
	    }

	    opc_ahd_build(global, &ahd, &proj_ahd);

	    /* Run thru the action(s) just made and do post-processing */
	    if (is_top_ahd)
	    {
		QEF_AHD *action = ahd;
		do
		{
		    if (action->ahd_flags & QEA_NODEACT)
			opc_set_resettable(action->qhd_obj.qhd_qep.ahd_qep, FALSE);
		    action = action->ahd_next;
		} while (action != ahd);
	    }

	    if (ahd_list == NULL)
	    {
		ahd_list = ahd;
	    }
	    else
	    {
		/* Put the doublely linked list pointed 
		** to by 'ahd' on to the end
		** of the list pointed to by 'ahd_list'.
		*/
		ahd_list->ahd_prev->ahd_next = ahd;
		ahd->ahd_prev->ahd_next = ahd_list;
		tmpahd = ahd_list->ahd_prev;
		ahd_list->ahd_prev = ahd->ahd_prev;
		ahd->ahd_prev = tmpahd;
	    }
	}
    }

    /* Now fill in the cost estimates in the first QEP action of the	    */
    /*statement. If there is no QEP action there is no resource checking    */
    for (tmpahd = ahd_list; ; )
    {
	switch (tmpahd->ahd_atype)
	{
	case QEA_APP:
	case QEA_SAGG:
	case QEA_PROJ:
	case QEA_AGGF:
	case QEA_UPD:
	case QEA_DEL:
	case QEA_GET:
	case QEA_RAGGF:
	case QEA_PUPD:
	case QEA_PDEL:
	case QEA_LOAD:
	    tmpahd->qhd_obj.qhd_qep.ahd_estimates = 1;

	    /* The code below is supposed to prevent integer overflow.
	    ** We are assumming that a sizeof(i4) == sizeof(i4)
	    ** and will therefore use MAXI4 value when necessary. Lack
	    ** od MAXLONGNAT was discussed with a CL committee member
	    ** and the above solution was considered safe. Besides
	    ** ANSI "C" defines i4 as a 4 byte long entity.
	    ** 25-jun-90 (stec). Fixes bug 31067.
	    */
	    tmpahd->qhd_obj.qhd_qep.ahd_cost_estimate =
                (cost_cost < (OPO_COST)MAXI4) ? (i4)cost_cost : MAXI4;
            tmpahd->qhd_obj.qhd_qep.ahd_cpu_estimate =
                (cpu_cost < (OPO_CPU)MAXI4)  ? (i4)cpu_cost  : MAXI4;
            tmpahd->qhd_obj.qhd_qep.ahd_dio_estimate =
                (dio_cost < (f4)MAXI4)  ? (i4)dio_cost  : MAXI4;
            tmpahd->qhd_obj.qhd_qep.ahd_row_estimate =
                (row_cost < (OPO_TUPLES)MAXI4)  ? (i4)row_cost  : MAXI4;
            tmpahd->qhd_obj.qhd_qep.ahd_page_estimate =
                (page_cost < (OPO_BLOCKS)MAXI4) ? (i4)page_cost : MAXI4;

            break;

        default:
            tmpahd = tmpahd->ahd_next;
            if (tmpahd == ahd_list)
                break;
            continue;
        }
        break;
    }

    global->ops_cstate.opc_subqry = save_subqry;
    *pahd_list = ahd_list;
}

/*{
** Name: OPC_QFATT_SEARCH:	- Search qualification for func atts
**		and replace with underlying descriptions
**
** Description: Locate VAR nodes in qualification exprs which represent
**		func attrs, and replace them with func attr parse tree 
**		fragment. This fixes bug 86478 which assumes func attrs
**		are built prior to evaluation of qual code in QP nodes,
**		when they are NOT built until after the qual code is 
**		executed.
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**	modified qualification parse tree fragment
**
**	Returns:
**	nothing
**	
**	Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	23-oct-97 (inkdo01)
**	    Written.
**
*/
static VOID	
opc_qfatt_search(
	OPS_STATE	*global, 
	OPC_NODE	*cnode, 
	PST_QNODE	**orig_qual, 
	OPE_BMEQCLS	*fattmap)

{
    OPS_SUBQUERY	*subquery = global->ops_cstate.opc_subqry;
    OPC_EQ		*ceq = cnode->opc_ceq;
    PST_QNODE		**tmptr = orig_qual;
    PST_QNODE		*newnode;
    OPZ_ATTS		*jatt;
    OPE_IEQCLS		eqcno;



    /* Simply descend the qualification tree fragment, looking for PST_VAR 
    ** nodes. All else iterate on left and recurse on right. */

    while (*tmptr)
    {
	if ((*tmptr)->pst_sym.pst_type == PST_VAR)
	{
	    /* Check this VAR out. If it represents a funcatt compiled
	    ** in this node, replace by funcatt expression */
	    jatt = subquery->ops_attrs.opz_base->opz_attnums
		[(*tmptr)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    eqcno = jatt->opz_equcls;
	    if (!(BTtest((i4)eqcno, (char *)fattmap))) break;
				/* quick exit if attr NOT a fattr */
	    jatt = subquery->ops_attrs.opz_base->opz_attnums
		[ceq[eqcno].opc_attno];
	    newnode = subquery->ops_funcs.opz_fbase->opz_fatts
		[jatt->opz_func_att]->opz_function->pst_right;
	    (*tmptr) = newnode;		/* replace by expression */
	    break;
	}
	if ((*tmptr)->pst_right)
		opc_qfatt_search(global, cnode, &(*tmptr)->pst_right,
				fattmap);  /* recurse on right */
	tmptr = &(*tmptr)->pst_left;	/* and iterate on left */
    }

}

/*
** Name: opc_set_resettable - Set "resettable" flag in plan nodes
**
** Description:
**	Query nodes below an exchange want to know if they are in a
**	resettable plan fragment (meaning, resettable during one
**	instantiation / execution of the plan;  any plan is resettable
**	the first time through, if it's repeated.)  Parallel query
**	child threads that are not resettable can exit as soon as
**	they have generated rows.  Resettable child threads have to
**	wait around until told that they aren't needed any more.
**	(We may find other uses for the resettable indicator too,
**	so it's set for all plans, parallel or not.)
**
**	A sub-plan is resettable if it's the inner of a CPJOIN,
**	PSMJOIN, or SEJOIN.
**
** Inputs:
**	node			Top of sub-plan to mark
**	resettable		TRUE if node itself is resettable
**
** Outputs:
**	None
**	(sets QEN_PQ_RESET flag where needed)
**
** History:
**	16-May-2010 (kschendel) b123565
**	    Written to fix various obscure parallel query bugs that occur
**	    when an EXCH manages to get generated way down inside a
**	    resettable sub-plan.
*/

static void
opc_set_resettable(QEN_NODE *node, bool resettable)
{
    QEF_AHD *act;
    QEN_NODE *outer;

    /* Loop on outer, recurse on inner / only - helps reduce recursion depth */
    do
    {
	if (resettable)
	    node->qen_flags |= QEN_PQ_RESET;

	switch (node->qen_type)
	{
	    case QE_CPJOIN:
	    case QE_ISJOIN:
		opc_set_resettable(node->node_qen.qen_sjoin.sjn_inner, TRUE);
		outer = node->node_qen.qen_sjoin.sjn_out;
		break;

	    case QE_SEJOIN:
		opc_set_resettable(node->node_qen.qen_sejoin.sejn_inner, TRUE);
		outer = node->node_qen.qen_sejoin.sejn_out;
		break;

	    case QE_FSMJOIN:
		opc_set_resettable(node->node_qen.qen_sjoin.sjn_inner, resettable);
		outer = node->node_qen.qen_sjoin.sjn_out;
		break;

	    case QE_HJOIN:
		opc_set_resettable(node->node_qen.qen_hjoin.hjn_inner, resettable);
		outer = node->node_qen.qen_hjoin.hjn_out;
		break;

	    case QE_KJOIN:
		outer = node->node_qen.qen_kjoin.kjoin_out;
		break;

	    case QE_TJOIN:
		outer = node->node_qen.qen_tjoin.tjoin_out;
		break;

	    case QE_SORT:
		outer = node->node_qen.qen_sort.sort_out;
		break;

	    case QE_TSORT:
		outer = node->node_qen.qen_tsort.tsort_out;
		break;

	    case QE_EXCHANGE:
		outer = node->node_qen.qen_exch.exch_out;
		break;

	    case QE_QP:
		/* Apply resettable to all actions under the QP */
		act = node->node_qen.qen_qp.qp_act;
		while (act != NULL)
		{
		    if (act->ahd_flags & QEA_NODEACT)
			opc_set_resettable(act->qhd_obj.qhd_qep.ahd_qep, resettable);
		    act = act->ahd_next;
		}
		/* Nothing else under QP */
		return;

	    default:
		/* orig, tproc, anything else with nothing under it */
		return;
	} /* switch */

	node = outer;
    } while (node != NULL);

} /* opc_set_resettable */
