/*
**Copyright (c) 2004, 2010 Ingres Corporation
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

/* external routine declarations definitions */
#define        OPC_SEJOIN		TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCSEJOIN.C - Routine to build sejoin nodes.
**
**  Description:
**      The routines in this file should be integrated into OPCJOINS.C 
**      when this is integrated from the SQL path to the RPLUS path. 
**
**
**
**  History:
**      20-feb-87 (eric)
**          created
**	18-jul-89 (jrb)
**	    Initialized prec fields properly for decimal project.
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual in opc_sejoin_build.
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	13-apr-90 (stec)
**	    Fixed opc_serip() for 20526 bug.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	23-oct-90 (stec)
**	    Fixed bugs 33612, 33135 in opc_serip().
**	22-dec-90 (stec)
**	    Fixed SEjoin related bug showing up when several SEjoins are
**	    present in qualification. Code modified is opc_serip(). Modified
**	    opc_sejoin_build() to use correct output row info when same QP
**	    is used more than once in the final QEP. Modified opc_jseinner()
**	    to initialize additional fields that were added to OPC_SEINNER struct.
**	29-jan-91 (stec)
**	    Changed opc_sejoin_build(), opc_sekeys(). Fixed bug # 32340.
**      18-mar-92 (anitap)
**          Changed opc_sekeys(). Fixed bug # 42363.
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-sep-93 (anitap)
**          Changed opc_sejoin_build(). Fixed bugs 49074/53020/53658/54517.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	29-dec-94 (wilri01)
**	    Fixed bug 66186, which fixed a problem introduced by Eric's fix
**	    to bug 55730.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**	20-oct-98 (nicph02)
**	    Fix to bug 66186 caused bug 91007. Backing it out is a safe change
**	    as there is now no problem with fix to bug 55730 (66186).
**	31-dec-04 (inkdo01)
**	    Added parm to opc_materialize() calls.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanup, delete unused adbase, gtqual parameters.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - pick up generic cardinality from QEN_NODE
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/*
** Forward Structure Definitions:
*/
typedef struct _OPC_SEINNER OPC_SEINNER;

/* TABLE OF CONTENTS */
void opc_rsequal(
	PST_QNODE *qual);
void opc_sejoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
static void opc_serip(
	OPS_STATE *global,
	PST_QNODE **proot,
	PST_QNODE **sequal,
	PST_QNODE **seop,
	i4 *junc_type);
static void opc_jseinner(
	OPS_STATE *global,
	OPC_NODE *pcnode,
	OPO_CO *co,
	i4 level,
	OPC_EQ *oceq,
	struct _OPC_SEINNER *seinner);
void opc_ccqtree(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	QEN_ADF **qadf,
	i4 dmf_alloc_row);
i4 opc_eqqtree(
	OPS_STATE *global,
	OPE_BMEQCLS *eqcmap,
	PST_QNODE **inroot,
	PST_QNODE **outroot);
void opc_sekeys(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *seop,
	OPC_EQ *iceq,
	OPC_EQ *oceq);

struct _OPC_SEINNER
{
    QEN_NODE	*opc_seinner;
    OPC_EQ	*opc_ceq;
    OPC_EQ	*opc_wceq;
    i4		opc_below_rows;
    i4		opc_hfile;
    i4		opc_rno;
    i4		opc_rsz;

	/* opc_nsejoins tells how many sejoins opc_seinner is below */
    i4		opc_nsejoins;
};

/*
**  Defines of constants used in this file
*/
#define     OPC_UNKNOWN_LEN  0
#define     OPC_UNKNOWN_PREC 0
#define	    OPC_TRUE	    1
#define	    OPC_FALSE	    2
#define	    OPC_UNKNOWN	    3

#define	    OPC_ACONT_AND   QESE1_CHECK_AND
#define	    OPC_ARET_AND    QESE4_CHECK_OR
#define	    OPC_OCONT_OR    QESE2_RETURN_AND
#define	    OPC_ORET_OR	    QESE3_RETURN_OR

/*{
** Name: opc_rsequal - Reorganize SE qualification tree.
**
** Description:
**      This procedure will reorganize the OR factors so that ones
**	without subselect are placed at the top of the OR list.
**
** Inputs:
**      qual		    Pointer to the qualification tree
**
** Outputs:
**      qual		    Modified qualification with non-
**			    subselect factors at the top of
**			    the list.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-apr-90 (stec)
**	    Created.
*/
VOID
opc_rsequal(
		PST_QNODE *qual)
{
    register PST_QNODE  *lp;
    register PST_QNODE	*rp;
    register PST_QNODE	*q;
    PST_QNODE		*swap_node;
    i4			reorg;

    /*
    ** Traverse the rightmost "AND" branch
    ** of the normalized qualification tree.
    */
    while (qual && qual->pst_sym.pst_type != PST_QLEND)
    {
	q = qual->pst_left;
	reorg = 0;

	/*
	** Traverse the rightmost branch of the 
	** subtree; we are interested in ORs only.
	*/
	while (q && q->pst_sym.pst_type == PST_OR)
	{
	    rp = q->pst_right;
	    lp = q->pst_left;

	    if (lp->pst_sym.pst_type == PST_BOP &&
		lp->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA
	       )
	    {
		if (!reorg)
		{
		    /* remember the first candidate for swap */
		    swap_node = q;
		    reorg++;
		}
	    }
	    else
	    {
		if (reorg)
		{
		    PST_QNODE *tmp;

		    /* Swap left child */
		    tmp = swap_node->pst_left;
		    swap_node->pst_left = lp;
		    q->pst_left = tmp;

		    /* set next swap candidate */
		    swap_node = swap_node->pst_right;	    
		}
	    }

	    /* Go down the right branch */
    	    if (rp->pst_sym.pst_type != PST_OR)
	    {
		/* This is the end of the tree; requires special processing */
		if (rp->pst_sym.pst_type != PST_BOP ||
		    rp->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_NOMETA)
		{
		    if (reorg)
		    {
			PST_QNODE *tmp;

			/* Swap right child */
			tmp = swap_node->pst_left;
			swap_node->pst_left = rp;
			q->pst_right = tmp;
		    }
		}

		/* exit loop */
		break;
	    }
	    else
	    {
		q = rp;
	    }
	} /* while OR */

	qual = qual->pst_right;

    } /* while AND */
}

/*{
** Name: OPC_SEJOIN_BUILD	- Compile an QEN_SEJOIN node.
**
** Description:
**		EJLFIX: Move me to OPCJOINS.C
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
**      20-feb-87 (eric)
**          created
**      11-aug-89 (eric)
**          Added fix for bug 6154 by incrementing ahd_qep_cnt when building
**          multiple SEJOIN nodes.
**      05-aug-89 (eric)
**          Fixed the handling of the opc_endpostfix field so that the qen
**          nodes are in their correct postfix order.
**      09-oct-89 (eric)
**          Fixed the fix to the opc_endpostfix fix. This fixes the QEN CBs
**          problems (no bugno).
**	11-oct-89 (stec)
**	    Added boolean factor bitmap ptr to the list of args
**	    passed to opc_gtqual.
**	22-dec-90 (stec)
**	    Added code to copy eqc and att arrays of the inner to a temporary
**	    ceq and att arrays before invoking opc_materialize(). See comments
**	    in code.
**	    Removed a call to opc_jnotret() to prevent possible problems showing
**	    up as 'eqc not found' when processing function attributes.
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize(), since in the case of hold
**	    file sharing we want to reuse the DSH row into which the inner is
**	    to be materialized. To accomplish this, an extra parameter was added
**	    to opc_materialize().
**	13-may-92 (rickh)
**	    Added cnode as an argument to opc_gtqual and opc_gtiqual.  This
**	    enables these routines to tell which outer join boolean factors
**	    they shouldn't touch.
**      15-sep-93 (anitap)
**          Fixed bugs 49074/53020/53658/54517. The offset for the
**          dsh_row for the outer row in sejn_okmat was same for both
**          sejoins. Pass in the correct subselect to opc_sekeys()
**          according to the jattno.
**	28-july-04 (inkdo01)
**	    Invent sort on outer subtree if it is exchange or hash join
**	    and thinks it is already sorted (opo_ordeqc >= 0).
**      31-Mar-05 (hanal04) Bug 113745 INGSRV3119
**          opc_serip() may return a section of the sejoin tree in which the
**          seop->pst_left->pst_right is a PST_CONST. We must not refer
**          to this as a PST_VAR in order to establish jattno.
**	08-may-06 (toumi01)
**	    Use calculated params_size for allocation of qn and also for
**	    the initing MEcopy to avoid trashing memory. The old code
**	    worked by accident in the past because sizeof(qn->node_qen)
**	    used to be equal to sizeof(QEN_SEJOIN);
**	12-may-2008 (dougi)
**	    Added parm to opc_jinouter() for table procedures.
**	13-Nov-2009 (kiria01) 121883
**	    Distinguish between LHS and RHS card checks for 01.
*/
VOID
opc_sejoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEF_AHD		*ahd = subqry->ops_compile.opc_ahd;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_NODE		*prev_qen;
    QEN_SEJOIN		*jn = &cnode->opc_qennode->node_qen.qen_sejoin;
    OPZ_IATTS		hijattno;
    OPZ_IATTS		jattno;
    OPV_VARS		*opvvar;
    OPV_SUBSELECT	*opvsub;
    OPV_SUBSELECT	*topsub;
    PST_QNODE		*seop;
    PST_QNODE		*sequal;
    PST_QNODE		*seqtree;
    OPE_BMEQCLS		inner_eqcmp;
    OPE_BMEQCLS		qual_eqcmp;
    OPC_SEINNER		*sein;
    i4			current_junc;
    i4			last_junc;
    i4			below_outer;
    QEN_NODE		*bgpostfix;
    QEN_NODE		*endpostfix;
    i4			ev_size;

    /* Set up some variables that will keep line length down */
    opvvar = subqry->ops_vars.opv_base->opv_rt[co->opo_inner->opo_union.opo_orig];
    ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);

    /* Compile the outer node, and fill in the sorted info.
    ** First, check for exchange/hash join outer (that scramble the 
    ** order) and force sort on correlation column(s)(if any). */
    if (co->opo_outer->opo_ordeqc >= 0 &&
	(co->opo_outer->opo_sjpr == DB_SJ && 
		co->opo_outer->opo_variant.opo_local->opo_jtype == OPO_SJHASH ||
	co->opo_outer->opo_sjpr == DB_PR && 
		co->opo_outer->opo_outer->opo_sjpr == DB_SJ &&
		co->opo_outer->opo_outer->opo_variant.opo_local->
						opo_jtype == OPO_SJHASH ||
	co->opo_outer->opo_sjpr == DB_EXCH))
      co->opo_osort = TRUE;

    opc_jinouter(global, cnode, co->opo_outer,
	OPC_COOUTER, &jn->sejn_out, &oceq, (OPC_EQ *) NULL);

    below_outer = cnode->opc_below_rows;
    qn->qen_atts = jn->sejn_out->qen_atts;
    qn->qen_natts = jn->sejn_out->qen_natts;
    if (jn->sejn_out->qen_prow)
	STRUCT_ASSIGN_MACRO(jn->sejn_out->qen_prow, qn->qen_prow);

    /* Find max no. of joining atts. */
    for (hijattno = 0, opvsub = opvvar->opv_subselect;
	 opvsub != NULL;
	 opvsub = opvsub->opv_nsubselect)
    {
	if (hijattno < opvsub->opv_atno)
	    hijattno = opvsub->opv_atno;
    }

    /* Allocate memory to hold info about the inner */
    {
	i4	len;	

	len = sizeof (*sein) * (hijattno + 1);
	sein = (OPC_SEINNER *)opu_Gsmemory_get(global, len);
	MEfill(len, (u_char)0, (PTR)sein);
    }

    /*
    ** Create all of the inner qen nodes and 
    ** record all info about the inner.
    */
    bgpostfix = subqry->ops_compile.opc_bgpostfix;
    endpostfix = subqry->ops_compile.opc_endpostfix;
    MEfill(sizeof (inner_eqcmp), (u_char)0, (PTR)&inner_eqcmp);
    /* Save the start of the subselect list. */
    topsub = opvvar->opv_subselect;

    for (opvsub = opvvar->opv_subselect;
	 opvsub != NULL;
	 opvsub = opvsub->opv_nsubselect)
    {
	/* now lets compile the inner node, figure out if this join 
	** is correlated, and, if so, record the location of the
	** correlated values.
	*/
	opvvar->opv_subselect = opvsub;
	jattno = opvsub->opv_atno;
	opc_jseinner(global, cnode, co->opo_inner, (i4)OPC_COINNER,
	    oceq, &sein[jattno]);
	BTset((i4)subqry->ops_attrs.opz_base->
	    opz_attnums[jattno]->opz_equcls, (char *)&inner_eqcmp);
    }

    /* Restore the start of the subselect list. */
    opvvar->opv_subselect = topsub;
    subqry->ops_compile.opc_bgpostfix = bgpostfix;
    subqry->ops_compile.opc_endpostfix = endpostfix;

    /*
    ** Now that the inner and outer qen trees are built, we can 
    ** get the clauses out of the qtree that apply to this node.
    */
    seqtree = NULL;
    opc_gtqual(global, cnode,
	cnode->opc_cco->opo_variant.opo_local->opo_bmbf,
	&seqtree, &qual_eqcmp);
    if (seqtree == NULL)
    {
	opx_error(E_OP0895_NO_SEJQTREE);
    }
    /*
    ** Modify the SE join qualification tree 
    ** to move ORs into a linear list.
    */
    opj_mvors(seqtree);

    /*
    ** Reorganize OR factors so that ones without
    ** subselects are placed at the top of the tree.
    */
    opc_rsequal(seqtree);

    /*
    ** Fill in a (possibly newly allocated) qen tree for 
    ** each segment in the seqtree that contains a subselect.
    */
    last_junc = PST_AND;
    prev_qen = NULL;
    while (seqtree != NULL && seqtree->pst_sym.pst_type != PST_QLEND)
    {
	/* get the next section of the sejoin tree to process */
	opc_serip(global, &seqtree, &sequal, &seop, &current_junc);

	/* Ensure seop->pst_left->pst_right is a VAR before trying to
	** set jattno and executing the rest of this loop.
	*/
	if(seop->pst_left->pst_right->pst_sym.pst_type != PST_VAR)
	{
	    if(seop->pst_left->pst_left->pst_sym.pst_type == PST_VAR)
	    {
		/* Switch the left and right nodes under the OP node
		** to meet the assumption in the following code that
		** we have a VAR in seop->pst_left->pst_right
		*/
                PST_QNODE *tmp;

	        tmp = seop->pst_left->pst_right;
	        seop->pst_left->pst_right = seop->pst_left->pst_left;
	        seop->pst_left->pst_left = tmp;
	    }
	    else
	    {
		/* Neither node is a VAR drop out with an error */
		opx_error(E_OP0798_OP_TYPE);
	    }
	}

	jattno = seop->pst_left->pst_right->
	    pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

#ifdef OPT_F030_SQLNAMES
	if (opt_strace(global->ops_cb, OPT_F030_SQLNAMES) == TRUE)
	{
	    DB_ERROR	psterr;
	    DB_STATUS	status;

	    status = pst_prmdump(seop, (PST_QTREE *) NULL, &psterr, DB_OPF_ID);

	    if (status == E_DB_OK)
	    {
		status = pst_prmdump(sequal, (PST_QTREE *) NULL,
		    &psterr, DB_OPF_ID);
	    }

	    if (status != E_DB_OK)
	    {
		opx_verror(status, E_OP0683_DUMPQUERYTREE, psterr.err_code);
	    }
	}
#endif

	/*
	** If this isn't the first time through then allocate 
	** a qen node and start initializing it.
	*/
	if (prev_qen != NULL)
	{
	    i4 params_size;
	    /* allocate a new qen node; */
	    params_size = sizeof (QEN_NODE) - sizeof(qn->node_qen)
			+ sizeof(QEN_SEJOIN);
	    qn = (QEN_NODE *) opu_qsfmem(global, params_size);
	    MEcopy((PTR)prev_qen, params_size, (PTR)qn);
	    qn->qen_num = global->ops_cstate.opc_qnnum;
	    qn->qen_size = sizeof(QEN_NODE) -
		sizeof(qn->node_qen) + sizeof(QEN_SEJOIN);

	    jn = &qn->node_qen.qen_sejoin;
	    jn->sejn_out = prev_qen;

	    ahd->qhd_obj.qhd_qep.ahd_qep_cnt += 1;
	    global->ops_cstate.opc_qnnum += 1;
	    cnode->opc_qennode = qn;
	}
	else
	{
	    jn = &qn->node_qen.qen_sejoin;
	}
	prev_qen = qn;

	/*
	** If this is the first time that this inner QEN_NODE has been
	** used then include it into the postfix list.
	*/
	if (sein[jattno].opc_nsejoins == 0)
	{
	    subqry->ops_compile.opc_endpostfix->qen_postfix = 
					    sein[jattno].opc_seinner;
	    subqry->ops_compile.opc_endpostfix = sein[jattno].opc_seinner;
	}
	sein[jattno].opc_nsejoins += 1;

	/*
	** If this isn't the last qen node that we will be generating,
	** then put it on the post fix list. If this is the last qen node
	** that we will generate, then opc_qentree_build() will put it 
	** on the post fix list.
	*/
	if (seqtree != NULL && seqtree->pst_sym.pst_type != PST_QLEND)
	{
	    subqry->ops_compile.opc_endpostfix->qen_postfix = qn;
	    subqry->ops_compile.opc_endpostfix = qn;
	}

	jn->sejn_inner = sein[jattno].opc_seinner;

	/*
	** Since opc_materialize() executed below will update eqc/att info, we
	** have to make a copy of the relevant info from the sein[] struct.
	** This is necessary because there may be a number of SEjoin quals
	** specified for a given attribute (normalization of the qual may cause
	** this, too) and on subsequent executions, row data would be
	** materialized from wrong location.
	*/
	MEcopy((PTR)sein[jattno].opc_ceq, ev_size, (PTR)sein[jattno].opc_wceq);
	iceq = sein[jattno].opc_wceq;

	cnode->opc_below_rows = sein[jattno].opc_below_rows + below_outer;

	/* Fill in the DMR_CB info related to hold file usage. */
	opc_ptcb(global, &jn->sejn_hget, sizeof (DMR_CB));

	/* The IF statement below implements hold file sharing. */
	if (sein[jattno].opc_nsejoins <= 1)
	{
	    jn->sejn_hcreate = TRUE;
	    opc_pthold(global, &jn->sejn_hfile);
	    sein[jattno].opc_hfile = jn->sejn_hfile;

	    /* Materialize the inner tuple into a new DSH row */
	    opc_materialize(global, cnode, &jn->sejn_itmat, &inner_eqcmp,
		iceq, &qn->qen_row, &qn->qen_rsz, (i4)TRUE, (bool)TRUE,
		(bool)FALSE);

	    /* Save DSH row info that may have to be reused. */
	    sein[jattno].opc_rno = qn->qen_row;
	    sein[jattno].opc_rsz = qn->qen_rsz;
	}
	else
	{
	    jn->sejn_hcreate = FALSE;
	    jn->sejn_hfile = sein[jattno].opc_hfile;
	    qn->qen_row = sein[jattno].opc_rno;
	    qn->qen_rsz = sein[jattno].opc_rsz;

	    /* Materialize the inner tuple into an existing DSH row */
	    opc_materialize(global, cnode, &jn->sejn_itmat, &inner_eqcmp,
		iceq, &qn->qen_row, &qn->qen_rsz, (i4)TRUE, (bool)FALSE,
		(bool)FALSE);
	}

	/* Added as fix for bugs 49074/53020/53658/54517. Though OPF
        ** was correctly generating two different separms, we were
        ** not passing the correct subselect to opc_sekeys(). So the
        ** top subselect was always compiled into sejn_okmat for both
        ** subselects.
        */
        for (opvsub = opvvar->opv_subselect;
                opvsub != NULL;
                opvsub = opvsub->opv_nsubselect)
        {
           if (jattno == opvsub->opv_atno)
               opvvar->opv_subselect = opvsub;
        }

	/* Compile sejn_okmat, sejn_kcompare, sejn_ccompare, and sejn_kqual */
	opc_sekeys(global, cnode, seop, iceq, oceq );

	/* Part of fix for bugs 49074/53020/53658/54517. Restore the
        ** top subselect so that we can pass in the next subselect to
        ** opc_skeys().
	*/
        
	/* restore the start of the subselect list */
        opvvar->opv_subselect = topsub;

	/* Merge iceq and oceq into ceq and merge icatt and ocatt into catt */
	opc_jmerge(global, cnode, iceq, oceq, ceq);

	/* Now that we've merged all ceq and catt info into 'cnode',
	** but haven't eliminated any info that won't be returned from
	** this node, we can compile sequal into sejn_oqual.
	*/
	cnode->opc_ceq = ceq;
	opc_ccqtree(global, cnode, sequal, &jn->sejn_oqual, -1);

	/* Fill in SELTYPE */
	qn->qen_flags &= ~QEN_CARD_MASK;
#if 1
	switch (seop->pst_left->pst_sym.pst_value.pst_s_op.pst_opmeta)
	{
	case PST_CARD_ALL:
	    qn->qen_flags |= QEN_CARD_ALL;
	    break;

	case PST_CARD_ANY:
	    qn->qen_flags |= QEN_CARD_ANY;
	    break;

	case PST_CARD_01R:
	    qn->qen_flags |= QEN_CARD_01R;
	    break;

	case PST_CARD_01L:
	    qn->qen_flags |= QEN_CARD_01L;
	    break;

	case PST_NOMETA:
	default:
	    /* error; */
	    /* qn->qen_flags |= QEN_CARD_NONE */
	    break;
	}
#else
	/* Once opo tracking works this can be used */
	if (co->opo_card_inner == OPO_CARD_01)
	    qn->qen_flags |= QEN_CARD_01R;
	else if (co->opo_card_inner == OPO_CARD_ANY)
	    qn->qen_flags |= QEN_CARD_ANY;
	else if (co->opo_card_inner == OPO_CARD_ALL)
	    qn->qen_flags |= QEN_CARD_ALL;
	else if (co->opo_card_outer == OPO_CARD_01)
	    qn->qen_flags |= QEN_CARD_01L;
	else if (co->opo_card_own == OPO_CARD_01)
	    qn->qen_flags |= QEN_CARD_01;
#endif

	switch (last_junc)
	{
	 case -1:
	 case PST_AND:
	    switch (current_junc)
	    {
	     case -1:
	     case PST_AND:
		jn->sejn_junc = OPC_ACONT_AND;
		break;

	     case PST_OR:
		jn->sejn_junc = OPC_ARET_AND;
		break;

	     default:
		/* error; */
		break;
	    }
	    break;

	 case PST_OR:
	    switch (current_junc)
	    {
	     case -1:
	     case PST_AND:
		jn->sejn_junc = OPC_OCONT_OR;
		break;

	     case PST_OR:
		jn->sejn_junc = OPC_ORET_OR;
		break;

	     default:
		/* error; */
		break;
	    }
	    break;

	 default:
	    /* error; */
	    break;
	}

	last_junc = current_junc;

	/* Fill in sejn_oper */
	switch (seop->pst_left->pst_sym.pst_value.pst_s_op.pst_opno)
	{
	 case ADI_EQ_OP:
	    jn->sejn_oper = SEL_EQUAL;
	    break;

	 case ADI_LT_OP:
	 case ADI_LE_OP:
	    jn->sejn_oper = SEL_LESS;
	    break;

	 case ADI_GE_OP:
	 case ADI_GT_OP:
	    jn->sejn_oper = SEL_GREATER;
	    break;

	default:
	    jn->sejn_oper = SEL_OTHER;
	    break;
	}
    }

    /* Eliminate eqcs and atts from ceq that won't be
    ** returned from this node. As a precaution this call was
    ** commented out to avoid 'eqc not found' message when processing
    ** function attributes.
    */
    /* opc_jnotret(global, co, ceq ); */
}

/*{
** Name: OPC_SERIP - Rip the qualification tree apart.
**
** Description:
**	This procedure rips the qualification tree apart. On each call a ptr to
**	a subtree related to 1 subselect will be returned. The most interesting
**	case is when there are PST_OR nodes in the tree. The tree will have
**	been modified in such a way that all OR nodes appear along the right
**	edge of the tree below AND nodes and their left children will have been
**	ordered in such a way that the subselect related PST_BOP nodes are as
**	far to the the bottom and right as possible.
**	Each left child of the AND node may have to be visited more than once,
**	i.e., it may be ripped apart in stages. Each call will return a
**	pointer to a subtree with 1 subselect related PST_BOP node (pst_opmeta
**	!= PST_NOMETA), and possibly a qualification subtree with PST_BOP(s) and
**	other nodes that do not have any subselect nodes associated
**	with them. The qualification tree will be returned only with the first
**	subtree.
**	Each subtree returned will have an AND node at the top, its right child
**	being NULL and the left child being the interesting subtree.
**	If a given AND subtree has no PST_BOP nodes related to subselects we
**	do not consider it interesting and move on to check the next AND node.
**	
**
** Inputs:
**	global	    ptr to OPS_STATE global structure
**	proot	    ptr to location containing the address of the tree 
**	sequal	    location for a qualification tree ptr
**	seop	    location for SE subtree ptr
**	junc_type   location for type of join (see below)
**
** Outputs:
**	proot	    ptr to the qualification tree where the ripping process
**		    should resume 
**	sequal	    qualification tree ptr (no subselects)
**	seop	    SE subtree ptr (1 subselect represented)
**	junc_type   type of "join" (AND, OR) with the rest of the qualification 
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    qualification tree will be modified
**
** History:
**      05-APR-87 (eric)
**          written
**	24-apr-90 (stec)
**	    Fixed bug 20526.
**	23-oct-90 (stec)
**	    Fixed bugs 33612, 33135. The problem was that junc_type
**	    that determines whether the tree is to be ORed or ANDed
**	    with the rest of the qualification was not set correctly
**	    for the case when one PST_BOP (no-meta) was ORed with
**	    one PST_BOP (meta). See comment in the code to locate fix.
**	    Additional comments were added to the routine to clarify
**	    code.
**	22-dec-90 (stec)
**	    Modified code to return correct junction type.
*/
static VOID
opc_serip(
	OPS_STATE   *global,
	PST_QNODE   **proot,
	PST_QNODE   **sequal,
	PST_QNODE   **seop,
	i4	    *junc_type )
{
    PST_QNODE	*root = *proot;
    PST_QNODE	*q, *top_or, *p1, *p2;
 
    /* Tree must exist */
    if (root == NULL || root->pst_sym.pst_type == PST_QLEND)
    {
	opx_error(E_OP0895_NO_SEJQTREE);
    }

    *junc_type = -1;
    *seop = NULL;
    *sequal = NULL;

    /*
    ** Traverse the rightmost "AND" branch of the normalized qualification
    ** tree. To understand the code it is important to bear in mind the fact
    ** that the tree is normalized and all ANDs are on the rightmost edge of
    ** the tree. Furthermore each left child of an AND node is normalized
    ** in the same way with respect to the OR nodes. Each OR tree will have
    ** PST_BOP nodes representing SE joins as close to the right and the bottom
    ** of the tree as possible.
    */
    while (root && root->pst_sym.pst_type != PST_QLEND)
    {
	q = root->pst_left;

	switch (q->pst_sym.pst_type)
	{
	case PST_BOP:
	    if (q->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA)
	    {
		/*
		** We have a PST_BOP representing an SEjoin
		** no need to walk the tree further.
		*/
		*proot = root->pst_right;
		root->pst_right = NULL;
		*seop = root;
		*junc_type = PST_AND;

		return;
	    }
	    else
	    {
		/*
		** This subtree is not interesting, 
		** let's move on to the next AND node.
		*/
	    }
	    break;

	case PST_OR:
	    /* Traverse the rightmost branch of the subtree.*/
	    top_or = q;		/* remember the top of the OR tree  */
	    p1 = top_or;	/* p1 and p2 are two auxiliary	    */
	    p2 = root;		/* pointers helpful in cutting	    */
				/* and pasting the tree.	    */

	    /*
	    ** We want to extract qualification and PST_BOP representing
	    ** an SE join, so we will be walking along the right branch
	    ** of the tree. Top of the OR subtree will be saved in top_or
	    ** and q ptr will be used to traverse the tree down. Pointers
	    ** p1 and p2 mark nodes in the OR tree above current q node
	    ** where p1 represents node 1 level above and p2 a node two
	    ** levels above.
	    **
	    ** Returning junction type is another issue. Correct way of
	    ** viewing it is by conceptually modifying the qualification
	    ** tree and prunning all subtrees that do not represent subselects,
	    ** ie., if subselect is ORed with some non-subselect expression
	    ** we remove (conceptually) the expression and replace the OR node
	    ** with subselect. On the practical level we accomplish this
	    ** by comparing original root ptr with the one to be returned;
	    ** if they are identical, then there must be another subselect
	    ** to be returned on subsequent call, and because of the way
	    ** the trees are structured, it must be ORed with the current one,
	    ** if root value returned is different from the original one, we
	    ** are done with this subtree (representing a conjunct) and PST_AND
	    ** junction type is returned since all conjucts are ANDed.
	    */

	    while (q && q->pst_sym.pst_type == PST_OR)
	    {
		/*
		** We are looking for the first PST_BOP representing 
		** an SE join, massage the tree and return.
		*/

		if (q->pst_left->pst_sym.pst_type == PST_BOP &&
		    q->pst_left->pst_sym.pst_value.pst_s_op.pst_opmeta
			!= PST_NOMETA
		   )
		{
		    *seop = q;
		    *proot = root;
		    root->pst_left = q->pst_right;
		    q->pst_right = NULL;
		    q->pst_sym.pst_type = PST_AND;

		    if (top_or != q)
		    {
			if (q == top_or->pst_right)
			{
			    /* only one factor in qualification */
			    top_or->pst_sym.pst_type = PST_AND;
			    top_or->pst_right = NULL;
			    *sequal = top_or;
			}
			else
			{
			    p2->pst_right = p1->pst_left;
			    p1->pst_sym.pst_type = PST_AND;
			    p1->pst_right = NULL;
			    p1->pst_left = top_or;
			    *sequal = p1;
			}
		    }

		    *junc_type = PST_OR;

		    return;
		}

		if (q->pst_right->pst_sym.pst_type == PST_BOP &&
		    q->pst_right->pst_sym.pst_value.pst_s_op.pst_opmeta
			!= PST_NOMETA
		   )
		{
		    /* We are done with this OR subtree */

		    *proot = root->pst_right;
		    root->pst_right = NULL;

		    if (top_or != q)
		    {
			p1->pst_right = q->pst_left;
			*sequal = root;
			q->pst_left = q->pst_right;
			q->pst_right = NULL;
			q->pst_sym.pst_type = PST_AND;
			*seop = q;
		    }
		    else
		    {
			root->pst_left = top_or->pst_left;
			*sequal = root;
			q->pst_left = q->pst_right;
			q->pst_right = NULL;
			q->pst_sym.pst_type = PST_AND;
			*seop = q;
		    }

		    *junc_type = PST_AND;

		    return;
		}

		/*
		** Go down the right side of the OR tree.
		** Remember last two predecessors of the q.
		*/
		p2 = p1;
		p1 = q;
		q = q->pst_right;

	    } /* while OR */

	    break;

	default:
	    /*
	    ** This subtree is not interesting, let's move on to 
	    ** the next AND node. In fact this code should be never
	    ** reached if the tree is normalized.
	    */
	    break;

	}   /* switch */

	root = root->pst_right;

    }	/* while AND */
}

/*{
** Name: OPC_JSEINNER	- Set up the inner of an sejoin node
**
** Description:
**      Compile a given inner or outer co node and return the eq and att 
**      information. 
[@comment_line@]...
**		    EJLFIX: Move me to OPCJCOMMON.c
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
**      16-oct-86 (eric)
**          written
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	22-dec-90 (stec)
**	    Added initialization for opc_wceq and opc_wcatt.
**	03-feb-94 (rickh)
**	    Replaced 9999 diagnostics with real error messages.
**	29-dec-94 (wilri01)
**	    Fixed bug 66186, which fixed a problem introduced by Eric's fix
**	    to bug 55730.
**      20-oct-98 (nicph02)
**          Fix to bug 66186 caused bug 91007. Backing it out is a safe change
**          as there is now no problem with fix to bug 55730 (66186).
**	19-Apr-2007 (kschendel) b122118
**	    Nobody seems to care whether the inner is correlated or not.
**	    Delete the computation.
**
[@history_template@]...
*/
static VOID
opc_jseinner(
	OPS_STATE	*global,
	OPC_NODE	*pcnode,
	OPO_CO		*co,
	i4		level,
	OPC_EQ		*oceq,
	OPC_SEINNER	*seinner )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPC_NODE		cnode;
    i4			ev_size;
    OPV_VARS		*opvvar;
    OPV_SEPARM		*vparm;
    OPE_IEQCLS		eqcno;
    OPZ_IATTS		vattno;
    PST_QNODE		seconst;

    /* fill in cnode to compile the inner or outer node */
    STRUCT_ASSIGN_MACRO(*pcnode, cnode);
    cnode.opc_cco = co;
    cnode.opc_level = level;
    cnode.opc_invent_sort = FALSE;

    ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);

    /* Allocate memory for the node */
    cnode.opc_ceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)cnode.opc_ceq);

    /* Allocate memory for the writeable eqc and attribute arrays */
    seinner->opc_wceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)seinner->opc_wceq);

    /*
    ** Compile parameter stuff if needed.
    */
    opvvar = subqry->ops_vars.opv_base->opv_rt[co->opo_union.opo_orig];
    if (opvvar->opv_subselect->opv_parm != NULL)
    {

	for (vparm = opvvar->opv_subselect->opv_parm; 
	     vparm != NULL;
	     vparm = vparm->opv_senext)
	{
	    /* In the future we would like to be able to process
	    ** an expression here.
	    */
	    if (vparm->opv_sevar->pst_sym.pst_type == PST_VAR)
	    {
		vattno = vparm->opv_sevar->pst_sym.
			pst_value.pst_s_var.pst_atno.db_att_id;
	    }
	    else if (vparm->opv_sevar->pst_sym.pst_type == PST_BOP)
	    {
		OPZ_IATTS	spattno;
		OPE_IEQCLS	speqcno;

		spattno = vparm->opv_sevar->pst_left->pst_sym.
		    pst_value.pst_s_var.pst_atno.db_att_id;

		speqcno = subqry->ops_attrs.opz_base->
		    opz_attnums[spattno]->opz_equcls;

		if (oceq[speqcno].opc_eqavailable == FALSE)
		{
		    opx_error(E_OP0889_EQ_NOT_HERE);
		}
		if (oceq[speqcno].opc_attavailable == FALSE)
		{
		    opx_error(E_OP0888_ATT_NOT_HERE);
		}

		vattno = vparm->opv_sevar->pst_right->pst_sym.
			pst_value.pst_s_var.pst_atno.db_att_id;
	    }
	    else
	    {
		opx_error( E_OP08AC_UNKNOWN_OPERATOR );
	    }

	    eqcno = subqry->ops_attrs.opz_base->opz_attnums[vattno]->opz_equcls;

	    if (oceq[eqcno].opc_eqavailable == FALSE)
	    {
		opx_error(E_OP0889_EQ_NOT_HERE);
	    }
	    if (oceq[eqcno].opc_attavailable == FALSE)
	    {
		opx_error(E_OP0888_ATT_NOT_HERE);
	    }

	    if (global->ops_procedure->pst_isdbp == FALSE)
	    {
		STRUCT_ASSIGN_MACRO(*vparm->opv_seconst, seconst);
		STRUCT_ASSIGN_MACRO(oceq[eqcno].opc_eqcdv, 
					seconst.pst_sym.pst_dataval);

	    	opc_ptparm(global, &seconst, oceq[eqcno].opc_dshrow,
			oceq[eqcno].opc_dshoffset);
	    }
	}
    }

    /* compile the inner subselect */
    opc_qentree_build(global, &cnode);

    /* return the information generated from the compilation */
    seinner->opc_seinner = cnode.opc_qennode;
    seinner->opc_ceq = cnode.opc_ceq;
    seinner->opc_below_rows = cnode.opc_below_rows;
    seinner->opc_hfile = -1;
    seinner->opc_nsejoins = 0;

    return;
}

/*{
** Name: OPC_CCQTREE	- Completely compile a qtree from beginning to end.
**
** Description:
{@comment_line@}...
**		    EJLFIX: Move me to OPCQTREE.c
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
**      3-feb-87 (eric)
**          created
[@history_template@]...
*/
VOID
opc_ccqtree(
		OPS_STATE   *global,
		OPC_NODE    *cnode,
		PST_QNODE   *root,
		QEN_ADF	    **qadf,
		i4	    dmf_alloc_row)
{
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    i4			tidatt;

    /* if there is nothing to compile, then return */
    if (root == NULL)
    {
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    /* initialize an QEN_ADF struct; */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    opc_cxest(global, cnode, root, &ninstr, &nops, &nconst, &szconst);
    max_base = cnode->opc_below_rows + 1;
    opc_adalloc_begin(global, &cadf, qadf, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

    if (dmf_alloc_row >= 0)
    {
	/* If there is a row that is really in a dmf_alloc buffer, then
	** let the base array know.
	*/
	opc_adbase(global, &cadf, QEN_DMF_ALLOC, 0);
	cadf.opc_row_base[dmf_alloc_row] = cadf.opc_dmf_alloc_base;
    }

    /* compile the comp_list; */
    opc_cqual(global, cnode, root, &cadf, ADE_SMAIN, 
					    (ADE_OPERAND *) NULL, &tidatt);

    opc_adend(global, &cadf);
}

/*{
** Name: OPC_EQQTREE	- Return a qtree whose clauses only refer to given eqcs
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
**      4-mar-87 (eric)
**          written
[@history_template@]...
*/
i4
opc_eqqtree(
		OPS_STATE	*global,
		OPE_BMEQCLS	*eqcmap,
		PST_QNODE	**inroot,
		PST_QNODE	**outroot)
{
    OPS_SUBQUERY    *subqry;
    PST_QNODE	    *root = *inroot;
    OPZ_IATTS	    jattno;
    OPE_IEQCLS	    eqcno;
    i4		    lret;
    i4		    rret;
    i4		    ret;
    
    switch (root->pst_sym.pst_type)
    {
     case PST_AND:
	lret = opc_eqqtree(global, eqcmap, &root->pst_left, outroot);
	if (root->pst_right != NULL && 
		root->pst_right->pst_sym.pst_type != PST_QLEND
	    )
	{
	    rret = opc_eqqtree(global, eqcmap, &root->pst_right, outroot);
	}
	else
	{
	    rret = OPC_FALSE;
	}

	if (lret == OPC_TRUE)
	{
	    *inroot = root->pst_right;
	    root->pst_right = *outroot;
	    *outroot = root;
	}

	if (rret == TRUE || lret == TRUE)
	{
	    ret = TRUE;
	}
	else
	{
	    ret = FALSE;
	}
    	break;

     case PST_VAR:
	subqry = global->ops_cstate.opc_subqry;
	jattno = root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	eqcno = subqry->ops_attrs.opz_base->opz_attnums[jattno]->opz_equcls;
	if (BTtest((i4)eqcno, (char *)eqcmap) == TRUE)
	{
	    ret = OPC_TRUE;
	}
	else
	{
	    ret = OPC_FALSE;
	}
	break;

     default:
	if (root->pst_left == NULL)
	{
	    lret = OPC_TRUE;
	}
	else
	{
	    lret = opc_eqqtree(global, eqcmap, &root->pst_left, outroot);
	}

	if (root->pst_right == NULL)
	{
	    rret = OPC_TRUE;
	}
	else
	{
	    rret = opc_eqqtree(global, eqcmap, &root->pst_right, outroot);
	}

	if (rret == TRUE && lret == TRUE)
	{
	    ret = TRUE;
	}
	else
	{
	    ret = FALSE;
	}
	break;
    }

    return(ret);
}

/*{
** Name: OPC_SEKEYS	- Compile the sejn_okmat, sejn_kcompare, sejn_ccompare,
**						and sejn_kqual for an SEJOIN.
**
** Description:
**
**	Generate 3 or 4 CX's for SE-join:
**
**	sejn_okmat - A two-part CX.  The INIT segment materializes the
**		current outer (join) key.  The MAIN segment compares the
**		current and previous outer keys to see if they are the
**		same.
**
**	sejn_cvmat - Materialize the current correlation values from
**		the outer row.
**
**	sejn_ccompare - See if the new correlation values are the same as
**		the current correlation values.
**
**	sejn_kqual - See if an inner compares successfully to the current
**		outer key.  This might be any testing operator (=, ne, etc).
**
**	The reason for putting all of the current-outer-key stuff into
**	one CX is to deal with VLUP outer keys;  consider a repeated
**	query with a clause like:  ? NOT IN (subselect).
**	The current key vs previous key comparison has to look at the
**	entire contents of the key value and there's no way to materialize
**	it into any sort of fixed length holding area.  Also, it's impossible
**	for one CX to refer to the VTEMP areas of another;  but it IS
**	possible for one segment of a CX to refer to a VTEMP value
**	generated by a different segment!
**
**	The kqual is a separate CX (although I suppose one might imagine
**	adding it to the okmat as well), and thus has to look at some sort
**	of actually materialized key.  This leads to the problem of what
**	to do when the VLUP was longer than the materialized length, and
**	the answer is that if the materialized-key compare is EQUAL,
**	we'll check the materialized vs actual current-outer values to
**	complete the comparison.  Thus for example, if the outer is "foobar",
**	but we actually materialize only "foo" and the inner is "foo" as
**	well, the kqual compares "foo" and "foobar" before returning a
**	compare answer.  (In this case, announcing "not equal".)
**	The kqual machinations for not-equal and range (< <= > >=) operators
**	are similar, but the details are slightly different;  see inline.
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
**      7-JUNE-87 (eric)
**          created
**      18-dec-88 (seputis)
**          changed mat_key_ to matkey_ for uniqueness
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	29-jan-91 (stec)
**	    Added code to move correlated values to DSH rows associated with
**	    local vars concerned (bug #32340).
**	02-jan-92 (rickh)
**	    When compiling the CX that materializes correlated attributes,
**	    don't assume that an attribute explodes into an expression.
**	11-feb-92 (rickh)
**	    Don't bother verifying that the correlated attributes have
**	    corresponding equivalence classes.  Estimate the CX size
**	    needed to evaluate each correlated attribute.
**      18-mar-92 (anitap)
**          Added code to initialize opr_prec ad opr_len (bug #42363).
**	13-Dec-2005 (kschendel)
**	    Update adf compile begin calls.
**	9-Sep-2010 (cast of thousands) b124341
**	    When the outer key is a VLUP, we were generating nonsense.
**	    Example:  where ? not in (subselect) in a context that
**	    suppresses flattening, with repeated or cache_dynamic on.
**	    The original code assumed that the outer key had a known fixed
**	    length which led to all sorts of problems.  Revise the
**	    CX scheme so that we can properly deal with VLUP's.
*/
VOID
opc_sekeys(
		OPS_STATE	*global,
		OPC_NODE	*cnode,
		PST_QNODE	*seop,
		OPC_EQ		*iceq,
		OPC_EQ		*oceq)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    QEN_SEJOIN		*jn = &cnode->opc_qennode->node_qen.qen_sejoin;
    OPO_CO		*ico = cnode->opc_cco->opo_inner;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    OPC_ADF		cadf;
    i4			tidatt;
    ADE_OPERAND		outer_sekey;
    ADE_OPERAND		key_comparand;
    ADE_OPERAND		outer_proto;
    ADE_OPERAND		ops[2];
    OPC_EQ		*save_ceq = cnode->opc_ceq;
    i4			matkey_row;
    OPV_VARS		*opvvar;
    OPV_SEPARM		*correlated_atts;
    OPV_SEPARM		*corratt;
    i4			matkey_len;
    i4			parmno;
    DB_DATA_VALUE	*subsel_var;
    i4			sekey_len;
    i4			corratt_offset;
    bool		vlup_key = FALSE;

    /* initialized some vars to keep line length down. */
    opvvar = subqry->ops_vars.opv_base->opv_rt[ico->opo_union.opo_orig];
    correlated_atts = opvvar->opv_subselect->opv_parm;
    subsel_var = &seop->pst_left->pst_right->pst_sym.pst_dataval;

    max_base = cnode->opc_below_rows + 1;

    /* First, materialize the outer key using sejn_okmat; */
    ninstr = 1;
    nops = 2;
    nconst = 0;
    szconst = 0;
    cnode->opc_ceq = oceq;
    opc_cxest(global, cnode, seop->pst_left->pst_left, 
	&ninstr, &nops, &nconst, &szconst);

    /* Determine the length of key row; besides key
    ** it will hold correlation values, if any.
    */
    matkey_len = 0;
    for (corratt = correlated_atts; 
	 corratt != NULL; 
	 corratt = corratt->opv_senext
	)
    {
	ninstr += 1;
	nops += 2;

	if (global->ops_procedure->pst_isdbp == TRUE)
	{
	    /* Account for additional MOVEs to local var rows (bug #32340). */
	    ninstr += 1;
	    nops += 2;
	}

	/* account for size of CX that evaluates the correlated attribute */

	opc_cxest(global, cnode, corratt->opv_sevar,
	    &ninstr, &nops, &nconst, &szconst);

	matkey_len += corratt->opv_sevar->pst_sym.pst_dataval.db_length;
	matkey_len = DB_ALIGN_MACRO(matkey_len);
    }

    /* Compile the okmat.
    ** Normal non-VLUP case:
    **    INIT segment has: get outer[1], move to matrow.
    **    MAIN segment has: get outer[2], compare to matrow
    ** VLUP case does:
    **    INIT segment has: get outer[1], coerce to matrow.
    **    MAIN segment has: get outer[2], compare to outer[1].
    **
    ** Thus the init segment is really the materialize part, and the
    ** main segment is what used to be the kcompare (compare previous and
    ** current key).
    */

    opc_adalloc_begin(global, &cadf, &jn->sejn_okmat, ninstr, nops,
	nconst, szconst, max_base, OPC_STACK_MEM);

    /* Get the outer select join key data to be moved */
    ops[1].opr_dt = 
	    seop->pst_left->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];

    /* Prevent unnecessary coercion. */
    if (ops[1].opr_dt == DB_ALL_TYPE)
    {
	ops[1].opr_dt =
	    seop->pst_left->pst_left->pst_sym.pst_dataval.db_datatype;
    }

    ops[1].opr_len = OPC_UNKNOWN_LEN;
    ops[1].opr_prec = OPC_UNKNOWN_PREC;	
    STRUCT_ASSIGN_MACRO(ops[1], outer_proto);

    /* compile the outer key (can be an expression).
    ** This one goes into the INIT segment of the okmat CX.
    */
    opc_cqual(global, cnode, seop->pst_left->pst_left, &cadf,
	ADE_SINIT, &ops[1], &tidatt);
    sekey_len = ops[1].opr_len;
    if (sekey_len == ADE_LEN_UNKNOWN)
    {
	sekey_len = subsel_var->db_length;
	vlup_key = TRUE;
    }

    /* Determine location for the outer select join key */
    matkey_len += sekey_len;
    matkey_len = DB_ALIGN_MACRO(matkey_len);
    opc_ptrow(global, &matkey_row, matkey_len);

    /* Materialize the outer select join key.
    ** For the normal case just MOVE the generated key expression to a
    ** holding area.  For the VLUP case, coerce it to a holding area that
    ** is the size of the subselect result.
    */
    opc_adbase(global, &cadf, QEN_ROW, matkey_row);
    if (vlup_key)
    {
	ops[0].opr_base = cadf.opc_row_base[matkey_row];
	ops[0].opr_offset = 0;
	ops[0].opr_dt = subsel_var->db_datatype;
	ops[0].opr_len = sekey_len;
	ops[0].opr_prec = subsel_var->db_prec;
	ops[0].opr_collID = subsel_var->db_collID;
	opc_adtransform(global,&cadf,&ops[1],&ops[0],ADE_SINIT,(bool)FALSE,(bool)FALSE);
	/* Now let's remember where we materialized the outer key value */
	STRUCT_ASSIGN_MACRO(ops[0], outer_sekey);
	matkey_len = DB_ALIGN_MACRO(sekey_len);
	/* MAIN will compare with the expression we just generated */
	STRUCT_ASSIGN_MACRO(ops[1], key_comparand);
    }
    else
    {
	/* Normal case, no VLUP */
	STRUCT_ASSIGN_MACRO(ops[1], ops[0]);
	ops[0].opr_base = cadf.opc_row_base[matkey_row];
	ops[0].opr_offset = 0;
	opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SINIT, 2, ops, 1);
	/* Now let's remember where we materialized the outer key value */
	STRUCT_ASSIGN_MACRO(ops[0], outer_sekey);
	matkey_len = DB_ALIGN_MACRO(sekey_len);
	/* MAIN will compare with the materialized key */
	STRUCT_ASSIGN_MACRO(ops[0], key_comparand);
    }

    /* Now do the key compare part of the okmat CX, which is in the main
    ** CX segment.
    */
    STRUCT_ASSIGN_MACRO(outer_proto, ops[1]);
    opc_cqual(global, cnode, seop->pst_left->pst_left, &cadf,
	ADE_SMAIN, &ops[1], &tidatt);
    STRUCT_ASSIGN_MACRO(key_comparand, ops[0]);
    opc_adinstr(global, &cadf, ADE_COMPARE, ADE_SMAIN, 2, ops, 0);
    opc_adinstr(global, &cadf, ADE_AND, ADE_SMAIN, 0, ops, 0);
    opc_adend(global, &cadf);

    /* Remember where we're putting correlation values for the ccompare */
    corratt_offset = matkey_len;

    /* Now lets materialize any and all correlated values */

    if (correlated_atts == NULL)
    {
	jn->sejn_cvmat = NULL;
    }
    else
    {
	opc_adalloc_begin(global, &cadf, &jn->sejn_cvmat, ninstr, nops,
		nconst, szconst, max_base, OPC_STACK_MEM);
	opc_adbase(global, &cadf, QEN_ROW, matkey_row);
	matkey_len = corratt_offset;
	for (corratt = correlated_atts; 
	     corratt != NULL; 
	     corratt = corratt->opv_senext
	    )
	{
	    /* In the future we would like to be able to process
	    ** an expression here.
	    */
	    ops[1].opr_dt = 
		    corratt->opv_sevar->pst_sym.pst_dataval.db_datatype;
	    ops[1].opr_len = OPC_UNKNOWN_LEN;
	    ops[1].opr_prec = OPC_UNKNOWN_PREC;

	    opc_cqual(global, cnode, corratt->opv_sevar, &cadf,
		    ADE_SMAIN, &ops[1], &tidatt);

	    STRUCT_ASSIGN_MACRO(ops[1], ops[0]);

	    ops[0].opr_base = cadf.opc_row_base[matkey_row];
	    ops[0].opr_offset = matkey_len;

	    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);

	    matkey_len += corratt->opv_sevar->pst_sym.pst_dataval.db_length;
	    matkey_len = DB_ALIGN_MACRO(matkey_len);

	    /* 
	    ** In the case of db procs also MOVE correlated values 
	    ** to DSH rows representing local variables (bug #32340).
	    */
	    if (global->ops_procedure->pst_isdbp == TRUE)
	    {
		parmno = corratt->opv_seconst->
		    pst_sym.pst_value.pst_s_cnst.pst_parm_no;

		opc_lvar_row(global, parmno, &cadf, &ops[0]);

		/* ops[1] has already been initialized by the code above. */

		opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	    }
	}
	opc_adend(global, &cadf);
    }

    /*
    ** Next, compare the correlated values that were just materialized
    ** with a newly materialized set of values using sejn_ccompare.
    */
    if (correlated_atts == NULL)
    {
	jn->sejn_ccompare = NULL;
    }
    else
    {
	opc_adalloc_begin(global, &cadf, &jn->sejn_ccompare, ninstr, nops,
	    nconst, szconst, max_base, OPC_STACK_MEM);

	/*
	** Add the base for the materialized key 
	** value and reset the base number.
	*/
	opc_adbase(global, &cadf, QEN_ROW, matkey_row);

	/* Now lets compare the correlated values */
	matkey_len = corratt_offset;
	for (corratt = correlated_atts; 
	     corratt != NULL; 
	     corratt = corratt->opv_senext)
	{
	    /* In the future we would like to be able to process
	    ** an expression here.
	    */

	    ops[1].opr_dt = 
		corratt->opv_sevar->pst_sym.pst_dataval.db_datatype;
	    ops[1].opr_len = OPC_UNKNOWN_LEN;
	    ops[1].opr_prec = OPC_UNKNOWN_PREC;
	    
	    opc_cqual(global, cnode, corratt->opv_sevar, &cadf,
	        ADE_SMAIN, &ops[1], &tidatt);

	    STRUCT_ASSIGN_MACRO(ops[1], ops[0]);

	    ops[0].opr_base = cadf.opc_row_base[matkey_row];
	    ops[0].opr_offset = matkey_len;

	    opc_adinstr(global, &cadf, ADE_COMPARE, ADE_SMAIN, 2, ops, 1);

	    matkey_len += corratt->opv_sevar->pst_sym.pst_dataval.db_length;
	    matkey_len = DB_ALIGN_MACRO(matkey_len);
	}

	opc_adinstr(global, &cadf, ADE_AND, ADE_SMAIN, 0, ops, 0);
	opc_adend(global, &cadf);
    }

    /*
    ** Now, compare the materialized outer key
    ** with the inner key, using sejn_kqual;
    */
    ninstr = 2;
    nops = 2;
    nconst = 0;
    szconst = 0;
    cnode->opc_ceq = iceq;
    opc_cxest(global, cnode, seop->pst_left->pst_right, 
	&ninstr, &nops, &nconst, &szconst);

    opc_adalloc_begin(global, &cadf, &jn->sejn_kqual,
	ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);

    /* Add the base for the materialized key value and reset the base number */
    opc_adbase(global, &cadf, QEN_ROW, matkey_row);
    STRUCT_ASSIGN_MACRO(outer_sekey, ops[0]);
    ops[0].opr_base = cadf.opc_row_base[matkey_row];

    ops[1].opr_dt = 
	seop->pst_left->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[1];

    /* Prevent unnecessary coercion. */
    if (ops[1].opr_dt == DB_ALL_TYPE)
    {
	ops[1].opr_dt =
	    seop->pst_left->pst_right->pst_sym.pst_dataval.db_datatype;
    }

    ops[1].opr_len = OPC_UNKNOWN_LEN;
    ops[1].opr_prec = OPC_UNKNOWN_PREC;

    opc_cqual(global, cnode, seop->pst_left->pst_right,
	&cadf, ADE_SMAIN, &ops[1], &tidatt);
    opc_adinstr(global, &cadf, 
	seop->pst_left->pst_sym.pst_value.pst_s_op.pst_opinst, 
	ADE_SMAIN, 2, ops, 0);
    if (vlup_key)
    {
	/* For VLUP outer key, things get real tricky here.  We need to
	** include a test of coerced key vs actual (full length) key,
	** in case the coerced key was truncated in some manner.  This
	** amounts to a multi-key comparison.  We test the coerced value
	** against the subselect result first, and (for most operators)
	** if it's equal, we further qualify with the coerced-vs-full key
	** test.
	** (a1,b1) < (a2,b2) --> a1 < a1 || (a1 == a2 && b1 < b2)
	** and likewise for the others.
	** Equal is: (a1,b1) == (a2,b2) --> a1 == a2 && b1 == b2
	** NE is: (a1,b1) != (a1,b2) --> a1 != a1 || b1 != b2
	*/
	ADE_OPERAND local_ops[2];
	ADE_OPERAND locallab;
	ADI_RSLV_BLK ar;
	ADI_OP_ID opno;

	locallab.opr_base = ADE_NOBASE;
	opno = seop->pst_left->pst_sym.pst_value.pst_s_op.pst_opno;
	if (opno == ADI_EQ_OP)
	    opc_adinstr(global, &cadf, ADE_AND, ADE_SMAIN, 0, local_ops, 0);
	else if (opno == ADI_NE_OP)
	    opc_adinstr(global, &cadf, ADE_OR, ADE_SMAIN, 0, local_ops, 0);
	else
	{
	    MEfill(sizeof(locallab), 0, (PTR) &locallab);
	    locallab.opr_base = ADE_LABBASE;
	    opc_adinstr(global, &cadf, ADE_ORL, ADE_SMAIN, 1, &locallab, 0);
	    /* Need to compare the key strings for equality */
	    ar.adi_op_id = ADI_EQ_OP;
	    ar.adi_fidesc = NULL;
	    ar.adi_num_dts = 2;
	    ar.adi_dt[0] = ops[0].opr_dt;
	    ar.adi_dt[1] = ops[1].opr_dt;
	    (void) adi_resolve(global->ops_adfcb, &ar, FALSE);
	    opc_adinstr(global, &cadf, ar.adi_fidesc->adi_finstid,
			ADE_SMAIN, 2, ops, 0);
	    opc_adinstr(global, &cadf, ADE_AND, ADE_SMAIN, 0, local_ops, 0);
	}
	/* Generate actual vlup outer key: */
	STRUCT_ASSIGN_MACRO(outer_proto, local_ops[1]);
	cnode->opc_ceq = oceq;	/* outer side context now */
	opc_cqual(global, cnode, seop->pst_left->pst_left,
		&cadf, ADE_SMAIN, &local_ops[1], &tidatt);

	/* Compare coerced and actual outers, same operator as used on the
	** coerced vs inner.  Conceivably could be a different fi though.
	*/

	STRUCT_ASSIGN_MACRO(outer_sekey, local_ops[0]);
	local_ops[0].opr_base = cadf.opc_row_base[matkey_row];
	ar.adi_op_id = opno;
	ar.adi_fidesc = NULL;
	ar.adi_num_dts = 2;
	ar.adi_dt[0] = local_ops[0].opr_dt;
	ar.adi_dt[1] = local_ops[1].opr_dt;
	(void) adi_resolve(global->ops_adfcb, &ar, FALSE);
	opc_adinstr(global, &cadf,
		ar.adi_fidesc->adi_finstid,
		ADE_SMAIN, 2, local_ops, 0);
	/* Resolve TRUE label from ADE_ORL if we generated one */
	if (locallab.opr_base != ADE_NOBASE)
	    opc_adlabres(global, &cadf, &locallab);
    }
    opc_adinstr(global, &cadf, ADE_AND, ADE_SMAIN, 0, ops, 0);
    opc_adend(global, &cadf);

    /* now clean up things before we return */
    cnode->opc_ceq = save_ceq;

    return;
}
