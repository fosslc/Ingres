/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <cs.h>
#include    <lk.h>
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
#include    <dmucb.h>
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
#include    <gwf.h>
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
#define             OPC_JOINS           TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCJOINS.C - Routine to build all types of join nodes.
**
**  Description:
**      This file  
**
**          opc_tjoin_build() - Compile a QEN_TJOIN node.
**          opc_kjoin_build() - Compile a QEN_KJOIN node.
**          opc_hjoin_build() - Compile a QEN_HJOIN node.
**	    opc_fsmjoin_build() - Compile a QEN_FSMJOIN node.
**          opc_cpjoin_build() - Compile a QEN_CPJOIN node.
**          opc_isjoin_build() - Compile a QEN_ISJOIN node.
**
**  History:    
**      17-oct-86 (eric)
**          created
**	23-oct-89 (stec)
**	    In opc_tjoin_build commented out the call to opc_jnotret procedure
**	    to fix the problem of "eqc not available" when a function
**	    attribute is involved.
**	30-oct-89 (stec)
**	    Implement new QEN_NODE types and outer joins.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	13-jul-90 (stec)
**	    Modified opc_tjoin_build() and opc_kjoin_build() to prevent integer
**	    overflow when calling opc_valid().
**	08-jan-91 (stec)
**	    Modified opc_join_build() to fix bug 34959. Also modified
**	    opc_sijoin_build() to prevent similiar problem.
**	29-jan-91 (stec)
**	    Changed opc_join_build(), opc_sijoin_build().
**	16-apr-91 (davebf)
**	    Added parm to opc_valid() call in order to enable vl_size_sensitive
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to jn->sjn_hget and jn->sjn_hcreate,
**	    which weren't used anywhere in QEF.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      17-aug-93 (anitap)
**          Typecast the values in the conditional tests involving dissimilar
**          types like float and MAXI4. The lack of these casts caused by
**          floating point exceptions on a certain machine in opc_tjoin_build()
**	    and opc_kjoin_build(). 
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**      08-may-2002 (huazh01)
**          Apply the fix for bug 98395 to opc_isjoin_build(). This fixes 
**          bug 107046. 
**	31-dec-04 (inkdo01)
**	    Added parm to opc_materialize() calls.
**	1-Aug-2007 (kschendel) SIR 122513
**	    Overhaul partition qualification, allow join-time partition
**	    qualification from joins that read the entire outer first:
**	    hash joins, outer-sorting FSM joins.  Pass partition-qual
**	    var bitmap up thru all joins.
**	8-Jan-2008 (kschendel) SIR 122513
**	    Add join-time partition pruning to K/T-joins.
**	12-may-2008 (dougi)
**	    Added parm to opc_jinouter() calls for table procedures.
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanups, delete unused params from jinouter calls.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_tjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_kjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_hjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_fsmjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_cpjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_isjoin_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
static QEN_PART_QUAL *opc_join_pqual(
	OPS_STATE *global,
	OPC_NODE *cnode,
	OPC_EQ *oceq,
	i4 node_type);
static QEN_PART_QUAL *opc_source_pqual(
	OPS_STATE *global,
	OPV_VARS *varp);

/*{
** Name: OPC_TJOIN_BUILD	- Compile a CO node into a QEN_TJOIN node.
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
**      17-oct-86 (eric)
**          written
**	23-oct-89 (stec)
**	    Commented out the call to opc_jnotret procedure
**	    to fix the problem of "eqc not available" when
**	    a function attribute is involved.
**	24-may-90 (stec)
**	    Added checks to use OPC_EQ, OPC_ATT contents only when filled in.
**	13-jul-90 (stec)
**	    Modified call to opc_valid() to prevent integer overflow.
**	16-apr-91 (davebf)
**	    Added parm to opc_valid() call in order to enable vl_size_sensitive
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**      17-aug-93 (anitap)
**          Typecast the values in the conditional tests involving dissimilar
**          types like float and MAXI4. The lack of these casts caused by
**          floating point exceptions on a certain machine. 
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**	2-feb-04 (inkdo01)
**	    Init tjoin_part for table partitioning.
**	3-feb-04 (inkdo01)
**	    Change buffer to hold TID8 for table partitioning.
**	13-apr-04 (inkdo01)
**	    Changes to QEN_PART_INFO inits.
**	3-may-05 (inkdo01)
**	    Init tjoin_flag and set TJOIN_4BYTE_TIDP to fix bug in post
**	    partitioned table Tjoins.
**	24-Jun-2005 (schka24)
**	    Extend above fix to properly handle OJ T-joins where the tidp
**	    is nullable.  (The size is then 5, not 4.)
**	22-Jul-2005 (schka24)
**	    Init new work-bitmap in partinfo.
**	16-Jun-2006 (kschendel)
**	    part-info qual stuff moved around, fix here.
**	 4-sep-06 (hayke02)
**	    Remove non-inner outer join equivalence classes from those used to
**	    compile tjoin_jqual ADE_COMPARE instructions. This prevents OJ
**	    outer rows, with ON clause attribute NULLs, being removed when we
**	    execute these erroneous base table/secondary index ADE_COMPAREs and
**	    return from ade_execute_cx() with an excb_value of ADE_BOTHNULL.
**	    This change fixes bug 116570.
**	30-apr-08 (hayke02)
**	    Back out the previous change (fix for bug 116570) and instead let
**	    the fix for bug 114467 (in opcjcommon.c) fix it. This change fixes
**	    bug 119834.
[@history_template@]...
*/
VOID
opc_tjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_TJOIN		*jn = &cnode->opc_qennode->node_qen.qen_tjoin;
    RDR_INFO		*rel;
    OPV_VARS		*opv;
    QEF_VALID		*valid;
    QEN_PART_INFO	*partp;
    i4			tidatt;
    i4			ev_size;
    i4			specialrow;
    i4			tidlength;

    /* compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->tjoin_out, &oceq, (OPC_EQ *) NULL);

    /* set up the inner relation to be read */
    opv = subqry->ops_vars.opv_base->opv_rt[co->opo_inner->opo_union.opo_orig];
    rel = opv->opv_grv->opv_relation;

    {
	i4	pages_touched;

	/* Prevent integer overflow */
	pages_touched =
	    (co->opo_cost.opo_pagestouched < (OPO_BLOCKS)MAXI4) ?
	    (i4)co->opo_cost.opo_pagestouched : MAXI4;
	valid = opc_valid(global, opv->opv_grv, DMT_S, 
			  pages_touched, (bool)TRUE);
    }

    jn->tjoin_get = valid->vl_dmr_cb;

    if (rel->rdr_parts)
    {
	/* Allocate/init QEN_PART_INFO for partitioned table. */
	partp = (QEN_PART_INFO *)opu_qsfmem(global, sizeof(QEN_PART_INFO));
	jn->tjoin_part = partp;
	partp->part_gcount = 0;
	partp->part_pcdim = NULL;
	partp->part_groupmap_ix = -1;
	partp->part_knokey_ix = -1;
	partp->part_dmrix = valid->vl_dmr_cb;
	partp->part_dmtix = valid->vl_dmf_cb;
	partp->part_totparts = valid->vl_partition_cnt;
	partp->part_threads = 1;	/* for now, Tjoin executes under
					** a single thread */
	/* Allocate a bitmap row for constant holding stuff. */
	opc_ptrow(global, &partp->part_ktconst_ix, (partp->part_totparts + 7) / 8);
    }
    else jn->tjoin_part = (QEN_PART_INFO *) NULL;

    jn->tjoin_pqual = NULL;

    /* An overall note on partition qualification with T-joins:
    ** Since the outer row contains the inner tid, with partition, the
    ** only real benefit of qualification occurs when we're able to
    ** exclude the targeted partition and avoid a random disk read.
    ** The hope is that the extra CPU needed for doing the whichpart
    ** and bitmap manipulation will be negligible for small outers,
    ** and will exclude enough inner reads to matter for large outers.
    */

    /* If potential partition qualification predicates were identified, build
    ** data structures to select the qualified inner partitions.  Pass the
    ** bitmap of inner restriction boolean factors.
    ** If qualification is generated, the tjoin pqual will be set.
    */
    if (opv->opv_pqbf != NULL)
    {
	QEN_PART_QUAL *pqual;

	pqual = opc_pqmat(global, cnode, opv,
			co->opo_inner->opo_variant.opo_local->opo_bmbf);
	jn->tjoin_pqual = pqual;
	if (pqual != NULL)
	{
	    /* Complete the hookup of orig and partition qual info */
	    pqual->part_orig = qn;	/* Link qual back to Tjoin node */
	    pqual->part_node_type = QE_TJOIN;
	}
	opv->opv_pqbf->opb_source_node = qn;	/* Yuck, but needed */
    }

    /* FIXME
    ** This is where the call to opc-join-pqual would go, exactly the same
    ** as the K-join call except for the node type.
    ** For now, we don't do that, and the reason is that if the secondary
    ** index contains the partitioning column, join-pqual will think
    ** that there is a join and generate useless join-time qualification.
    ** (Useless because the secondary index row necessarily exists in
    ** and qualifies the appropriate base table partition!)
    ** The necessary change would not be too hard - simply exclude
    ** eqclass pairs where the outer eqclass contains only columns
    ** from a secondary index of the inner table.  This is left as
    ** an exercise for someone who thinks they would benefit from
    ** the change.
    ** (kschendel Jan 2008)
    */

    if (oceq[co->opo_sjeqc].opc_eqavailable == FALSE)
    {
	opx_error(E_OP0889_EQ_NOT_HERE);
    }
    if (oceq[co->opo_sjeqc].opc_attavailable == FALSE)
    {
	opx_error(E_OP0888_ATT_NOT_HERE);
    }
    jn->tjoin_flag = 0;

    /* record where the uncoerced outer tid will be */
    jn->tjoin_orow = oceq[co->opo_sjeqc].opc_dshrow;
    jn->tjoin_ooffset = oceq[co->opo_sjeqc].opc_dshoffset;
    tidlength = oceq[co->opo_sjeqc].opc_eqcdv.db_length;
    if (oceq[co->opo_sjeqc].opc_eqcdv.db_datatype < 0)
	-- tidlength;			/* Nullable, don't count null byte */
    if (tidlength == 4)
	jn->tjoin_flag |= TJOIN_4BYTE_TIDP;

    /*
    ** Find a spot for the inner row, and fill in its eq's and att's.
    ** Remember that the TID needs to be materialized, so row width
    ** needs to be increased.
    */
    {
	i4 align;

	qn->qen_rsz = (i4) rel->rdr_rel->tbl_width;

	/* Make sure that we can store TID at an aligned location. */ 
	align = qn->qen_rsz % sizeof (ALIGN_RESTRICT);
	if (align != 0)
	    align = sizeof (ALIGN_RESTRICT) - align;
	qn->qen_rsz += (align + DB_TID8_LENGTH);

	opc_ptrow(global, &qn->qen_row, qn->qen_rsz);
    }

    /* Figure out what is coming out of the inner relation */
    ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);
    iceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)iceq);

    opc_ratt(global, iceq, co->opo_inner, qn->qen_row, qn->qen_rsz);

    /* record where the inner tid will be */
    jn->tjoin_irow = iceq[co->opo_sjeqc].opc_dshrow;
    jn->tjoin_ioffset = iceq[co->opo_sjeqc].opc_dshoffset;

    /* Generate the CX to determine if true TID value is null. */
    opc_tisnull(global, cnode, oceq, co->opo_inner->opo_union.opo_orig,
	&jn->tjoin_isnull);

    /* merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->tjoin_oj != (QEN_OJINFO *) NULL)
	jn->tjoin_oj->oj_specialEQCrow = specialrow;

    /* Qualify the joined tuple. Remember to compare only non-key eqcs. */
    {
	OPE_BMEQCLS	keqcmp;
	OPE_IEQCLS	eqcno;

	/* Figure out key eqcs at this node */
	MEcopy((PTR)&cnode->opc_cco->opo_inner->opo_maps->opo_eqcmap,
	    sizeof(keqcmp), (PTR)&keqcmp);
	BTand((i4)subqry->ops_eclass.ope_ev,
	    (char *)&cnode->opc_cco->opo_outer->opo_maps->opo_eqcmap, (char *)&keqcmp);

	for (eqcno = -1; (eqcno = BTnext(eqcno,
	     (char *)&keqcmp, subqry->ops_eclass.ope_ev)) > -1; )
	{
	    if (subqry->ops_eclass.ope_base->ope_eqclist[eqcno]->ope_eqctype
		== OPE_NONTID)
	    {
		BTclear((i4)eqcno, (char *)&keqcmp);
	    }
	}

	opc_jqual(global, cnode, (OPE_BMEQCLS *)&keqcmp, iceq,
	    oceq, &jn->tjoin_jqual, &tidatt);
    }

    /* eliminate the eqcs and atts from ceq that won't be returned from
    **	    this node;
    **
    **	EJLFIX: We can't call opc_jnotret() right now because if there is
    **	    a func att that the inner orig node need to create then we
    **	    can't eliminate the attributes that are needed to create the
    **	    func att. Find a better way around this (like get 
    **	    opc_qentree_build() to call opc_jnotret for all (or most) nodes.
    **
    ** opc_jnotret(global, co, ceq);
    */
}

/*{
** Name: OPC_KJOIN_BUILD	- Compile an QEN_KJOIN node.
**
** Description:
**      This routine builds a QEN_KJOIN node from a co node. This is done 
**      by first compiling the outer node. Then we pick a DSH row to hold 
**      the inner row, and decide which of the inner attributes will be 
**      needed. Next we figure out which of the inner and outer attributes 
**      the calling procedure needs to see. Then we fill in the keying
**	information for the inner relation (kjoin_key, kjoin_kcompare, and
**	kjoin_kys). Finally, we compile the qualification to be applied 
**	at this node. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable
**	cnode -
**	    This describes various info about the QEN node that we are
**	    compiling.
**
** Outputs:
**	cnode->opc_qennode -
**	    This will be completely filled in.
**	cnode->opc_ceq -
**	    These will tell what eqcs and attrs that this node returns.
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
**      25-aug-86 (eric)
**          written
**	13-jul-90 (stec)
**	    Modified call to opc_valid() to prevent integer overflow.
**	16-apr-91 (davebf)
**	    Added parm to opc_valid() call in order to enable vl_size_sensitive
**      17-aug-93 (anitap)
**          Typecast the values in the conditional tests involving dissimilar
**          types like float and MAXI4. The lack of these casts caused by
**          floating point exceptions on a certain machine. 
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**	6-jun-96 (inkdo01)
**	    Added support of spatial key joins.
**	28-may-97 (inkdo01)
**	    Fix TID qualification stuff for key joins.
**	12-nov-98 (inkdo01)
**	    Set kjoin_ixonly flag for Btree base table access which doesn't
**	    need access to data pages.
**	14-may-99 (inkdo01)
**	    Changes to speed up non-outer kjoins. Call to opc_ijkqual() (if
**	    this is not a spatial key join nor an outer join) which builds
**	    a single qualification expression from all non-outer key join
**	    predicates. This helps non-outer key joins to run much faster.
**	    This change fixes bug 96840.
**      25-jun-99 (inkdo01)
**          Identify key columns in join predicates to remove them from
**          key join kcompare code (since compare is already done in DMF).
**      15-oct-2003 (chash01)
**          Add rel->rdr_rel->tbl_relgwid == DMGW_RMS as last arg of 
**          opc_ijkqual() to fix rms gateway bug 110758 
**	2-feb-04 (inkdo01)
**	    Init kjoin_part for table partitioning.
**	3-feb-04 (inkdo01)
**	    Change buffer to hold TID8 for table partitioning.
**	13-apr-04 (inkdo01)
**	    Fill out the QEN_PART_INFO init.
**	22-Jul-2005 (schka24)
**	    Init new work-bitmap in partinfo.
**	16-Jun-2006 (kschendel)
**	    part-info qual stuff moved around, fix here.
**	8-Jan-2008 (kschendel) SIR 122513
**	    Implement partition qualification on the K-join inner.
**	    The framework has been there since last summer's work on
**	    partition qualification, just never got around to doing it
**	    for K-joins and T-joins.
**	13-May-2009 (kschendel) b122041
**	    Compiler caught bad bool assignment, fix.
[@history_template@]...
*/
VOID
opc_kjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_KJOIN		*jn = &cnode->opc_qennode->node_qen.qen_kjoin;
    RDR_INFO		*rel;
    QEN_PART_INFO	*partp;
    OPV_VARS		*opv;
    QEF_VALID		*valid;
    i4			align;
    i4			ev_size;
    i4			tidatt;
    i4			specialrow;
    OPE_IEQCLS		sjeqc;
    OPE_BMEQCLS		keymap;
    bool		spjoin = FALSE;

    /* compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->kjoin_out, &oceq, (OPC_EQ *) NULL);

    /* set up the inner relation to be read */
    opv = subqry->ops_vars.opv_base->opv_rt[co->opo_inner->opo_union.opo_orig];
    rel = opv->opv_grv->opv_relation;

    {
	i4	pages_touched;

	/* Prevent integer overflow */
	pages_touched =
	    (co->opo_cost.opo_pagestouched < (OPO_BLOCKS)MAXI4) ?
	    (i4)co->opo_cost.opo_pagestouched : MAXI4;
	valid = opc_valid(global, opv->opv_grv, DMT_S, 
			  pages_touched, (bool)TRUE);
    }

    jn->kjoin_get = valid->vl_dmr_cb;

    if (rel->rdr_parts)
    {
	i4 mapbytes;

	/* Allocate/init QEN_PART_INFO for partitioned table. */
	partp = (QEN_PART_INFO *)opu_qsfmem(global, sizeof(QEN_PART_INFO));
	jn->kjoin_part = partp;
	partp->part_gcount = 0;
	partp->part_pcdim = NULL;
	partp->part_groupmap_ix = -1;
	partp->part_dmrix = valid->vl_dmr_cb;
	partp->part_dmtix = valid->vl_dmf_cb;
	partp->part_totparts = valid->vl_partition_cnt;
	partp->part_threads = 1;	/* for now, Kjoin executes under
					** a single thread */
	mapbytes = (partp->part_totparts + 7) / 8;
	opc_ptrow(global, &partp->part_ktconst_ix, mapbytes);
	opc_ptrow(global, &partp->part_knokey_ix, mapbytes);
    }
    else jn->kjoin_part = (QEN_PART_INFO *) NULL;
    jn->kjoin_pqual = NULL;

    /* If potential partition qualification predicates were identified, build
    ** data structures to select the qualified inner partitions.  Pass the
    ** bitmap of inner restriction boolean factors.
    ** If qualification is generated, the kjoin pqual will be set.
    */
    if (opv->opv_pqbf != NULL)
    {
	QEN_PART_QUAL *pqual;

	pqual = opc_pqmat(global, cnode, opv,
			co->opo_inner->opo_variant.opo_local->opo_bmbf);
	jn->kjoin_pqual = pqual;
	if (pqual != NULL)
	{
	    /* Complete the hookup of orig and partition qual info */
	    pqual->part_orig = qn;	/* Link qual back to Kjoin node */
	    pqual->part_node_type = QE_KJOIN;
	}
	opv->opv_pqbf->opb_source_node = qn;	/* Barfo! */
    }

    /* K-joins are funky because they have both constant partition-qual
    ** maps (for the inner), and join-time partition qualification.
    */
    if (BTcount((char *) &co->opo_inner->opo_variant.opo_local->opo_partvars,
      		global->ops_cstate.opc_subqry->ops_vars.opv_rv) > 0)
    {
	/* A bit weird here, the inner of a K-join is its own orig.
	** join-pqual creates a pqual for the "orig" if one isn't
	** there, so the upshot is that upon return, kjoin_pqual is
	** set (or still null) and we don't need the return value.
	** (join-pqual can return NULL if there is no join partition
	** qualification to generate, which is why we can't use its
	** return value blindly.)
	*/
	(void) opc_join_pqual(global, cnode, oceq, QE_KJOIN);
    }

    jn->kjoin_nloff = -1;
    jn->kjoin_tqual = FALSE;
    jn->kjoin_ixonly = (opv->opv_mask & OPV_BTSSUB) != 0;
			/* set "index access only" flag */
    jn->kjoin_pcompat = FALSE;

    /* Find a spot for the inner row */
    qn->qen_rsz = (i4) rel->rdr_rel->tbl_width;
    if (opv->opv_primary.opv_tid == OPE_NOEQCLS)
    {
	jn->kjoin_tid = -1;
    }
    else
    {
	align = qn->qen_rsz % sizeof (ALIGN_RESTRICT);
	if (align != 0)
	    align = sizeof (ALIGN_RESTRICT) - align;
	qn->qen_rsz += (align + DB_TID8_LENGTH);

	jn->kjoin_tid = qn->qen_rsz - DB_TID8_LENGTH;
    }

    opc_ptrow(global, &qn->qen_row, qn->qen_rsz);

    /* fill in the inner rows eqc's */
    ev_size = subqry->ops_eclass.ope_ev * sizeof (OPC_EQ);
    iceq = (OPC_EQ *) opu_Gsmemory_get(global, ev_size);
    MEfill(ev_size, (u_char)0, (PTR)iceq);

    opc_ratt(global, iceq, co->opo_inner, qn->qen_row, qn->qen_rsz);

    /* Now lets merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* fill in kjoin_key, kjoin_kcompare, and kjoin_kys */
    /* EJLFIX: use a smaller portion of the tree than subqry->ops_root */
    /* First step is to distinguish spatial join from regular key join,
    ** and split the logic accordingly. */

    MEfill(sizeof(keymap), (u_char)0, (PTR)&keymap);

    if (rel->rdr_rel->tbl_storage_type == DB_RTRE_STORE &&
	(sjeqc = co->opo_sjeqc) >= 0 &&
	subqry->ops_eclass.ope_base->ope_eqclist[sjeqc]->ope_mask &
	OPE_SPATJ)
    {
	if (opc_spjoin(global, cnode, co, oceq, ceq)) spjoin = TRUE;
	else opx_error(E_OP08AD_SPATIAL_ERROR);
    }
    else opc_kjkeyinfo(global, cnode, rel, subqry->ops_root->pst_right, 
	               oceq, iceq, &keymap);

    /* If this is a key join that has no keys, then change the type to a
    ** physical join. This is done, as a mild hack, for QEF since keyed
    ** accesses to relations and full scans are very different.
    */
    if (jn->kjoin_kys == NULL)
    {
	/*FIXME*/
	/*qn->qen_type = QE_PJOIN;*/

	/* physical joins are not supported any longer */
	opx_error(E_OP089B_PJOIN_FOUND);
    }

    /* For non-outer joins, combine all the qualification code into 
    ** kjoin_iqual and set the rest to NULL. This makes kjoins 
    ** considerably faster. */

    if (cnode->opc_cco->opo_variant.opo_local->opo_ojid == OPL_NOOUTER
	&& !spjoin)
    {
	jn->kjoin_kqual = NULL;
	jn->kjoin_jqual = NULL;
	jn->kjoin_oj = NULL;
	opc_ijkqual(global, cnode, iceq, oceq, &keymap, &jn->kjoin_iqual,
                    &jn->kjoin_tqual, rel->rdr_rel->tbl_relgwid == GW_RMS);
	return;
    }

    /* qualify the retrieved tuple; */
    if (!spjoin) opc_iqual(global, cnode, iceq, &jn->kjoin_iqual,
							&jn->kjoin_tqual);
    else jn->kjoin_iqual = NULL;

    /* Apply full key qualification to the joined tuple. */
    if (!spjoin) opc_kqual(global, cnode, iceq, oceq, &jn->kjoin_kqual);
    else jn->kjoin_kqual = NULL;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->kjoin_oj != (QEN_OJINFO *) NULL)
	jn->kjoin_oj->oj_specialEQCrow = specialrow;

    if (!spjoin) opc_jqual(global, cnode, (OPE_BMEQCLS *)NULL, iceq,
	oceq, &jn->kjoin_jqual, &tidatt);
    else jn->kjoin_jqual = NULL;

    /* eliminate the eqcs and atts from ceq that won't be returned from
    **	    this node;
    **
    **	EJLFIX: We can't call opc_jnotret() right now because if there is
    **	    a func att that the inner orig node need to create then we
    **	    can't eliminate the attributes that are needed to create the
    **	    func att. Find a better way around this (like get 
    **	    opc_qentree_build() to call opc_jnotret for all (or most) nodes.
    **
    ** opc_jnotret(global, co, ceq);
    */
}

/*{
** Name: OPC_HJOIN_BUILD	- Compile a QEN_HJOIN node.
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
**      26-feb-99 (inkdo01)
**	    Cloned from opc_fsmjoin_build for hash joins.
**	8-feb-00 (inkdo01)
**	    Add support for build source duplicates removal.
**	26-june-01 (inkdo01)
**	    Added code to augment join node EQCs for outer joins in case not
**	    null col from inner is in "is null" pred in where clause (copied 
**	    from fsmjoin_build).
**	29-oct-01 (inkdo01)
**	    Silly me - I had left and right joins reversed in flag sent to 
**	    hash_materialize. This screwed up hash buffer oj indicators and 
**	    made some non-null columns look null.
**	19-apr-04 (inkdo01)
**	    Partition compatible join support.
**	7-Dec-2005 (kschendel)
**	    Build a compare-list instead of an attribute-list for keys.
**	10-Apr-2007 (kschendel) SIR 122513
**	    Turn on nested-PC-join flag when needed.
**	4-Aug-2007 (kschendel) SIR 122513
**	    Tack on join-time partitioning capability;  for now, equals
**	    only.  (no generic jquals yet.)
**	4-Aug-2007 (kschendel) SIR 122512
**	    Compute and set inner-check flag.  Turn on contains-hashop flag
**	    in parent action.
**	21-Aug-2007 (kschendel) b122118
**	    Fiddle with join estimate when it's a PC-join.
*/
VOID
opc_hjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_HJOIN		*jn = &cnode->opc_qennode->node_qen.qen_hjoin;
    OPE_BMEQCLS		keqcmp;
    OPC_EQ		*okceq;
    OPO_TUPLES		btups;
    i4			njeqcs;
    i4			tidatt;
    i4			brows;
    i4			specialrow;
    bool		ljoin, rjoin;

    /* Record that the parent action has a hash operation that will need
    ** to be dealt with at query allocation time
    */
    global->ops_cstate.opc_subqry->ops_compile.opc_ahd->qhd_obj.qhd_qep.ahd_qepflag |= AHD_CONTAINS_HASHOP;

    /* Compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->hjn_out, &oceq, cnode->opc_oceq);

    /* Compile the inner node and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_inner, OPC_COOUTER,
	&jn->hjn_inner, &iceq, cnode->opc_oceq);

    /* Fill in the hash structure number */
    opc_pthash(global, &jn->hjn_hash);

    /* CB number for hash partition file CB */
    opc_ptcb(global, &jn->hjn_dmhcb, sizeof(DMH_CB));

    /* Initialize keqcmp, okceq. */
    if (co->opo_sjeqc == OPE_NOEQCLS)
	njeqcs = 0;
    else
	njeqcs = 1;

    /* If outer join, make sure node EQCs include all the EQCs of the inner 
    ** side of the outer join. This assures that "not null" columns from the
    ** inner whose only reference is in the where clause (in a "is null" pred)
    ** get null indicator space allocated. */
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
     switch (co->opo_variant.opo_local->opo_type) {
      case OPL_RIGHTJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_outer->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      case OPL_LEFTJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_inner->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      case OPL_FULLJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_outer->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_inner->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      default:
	break;
    }

    opc_jkinit(global, &co->opo_sjeqc, njeqcs, oceq, 
	&keqcmp, &okceq);

    /* build compare-list array to describe hash keys */
    jn->hjn_nullablekey = opc_hashatts(global, &keqcmp, oceq, iceq,
				 &jn->hjn_cmplist, &jn->hjn_hkcount);

    /* Set oj flags for build/probe tuple materialization. */
    ljoin = rjoin = FALSE;
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
     switch (co->opo_variant.opo_local->opo_type) {
      case OPL_LEFTJOIN:
	ljoin = TRUE;
	break;
      case OPL_RIGHTJOIN:
	rjoin = TRUE;
	break;
      case OPL_FULLJOIN:
	ljoin = rjoin = TRUE;
	break;
      default:
	break;
    }
    /* Fill in the CO flag */
    jn->hjn_kuniq = (co->opo_existence != 0);

    /* materialize the outer tuple; */
    opc_hash_materialize(global, cnode, &jn->hjn_btmat, &co->opo_outer->opo_maps->opo_eqcmap, 
	&keqcmp, oceq, jn->hjn_cmplist, &jn->hjn_brow, &qn->qen_rsz, (ljoin | rjoin));
    /*	(rjoin) ? 1 : 0); */

    /* Compute size of build source as row count times row size, allowing 
    ** for build size > MAXI4.  This is used for runtime space allocation.
    **
    ** If this is a partition compatible join, the total is probably too
    ** big, since we're doing N smaller joins.  On the other hand,
    ** assuming uniform distribution can land us into hot water if
    ** wrong.  A reasonable compromise heuristic seems to be to take
    ** 2/3 of the full tups if it's a PC-join.
    ** (Only if # of groups is at least 5.)
    ** (FIXME ideally we'd get opj to pass along its SWAG as to
    ** the number of logical partitions)
    */
    btups = co->opo_outer->opo_cost.opo_tups;
    if (co->opo_variant.opo_local->opo_pgcount >= 5)
	btups = 2.0 * btups / 3.0;
    brows = (btups < (OPO_TUPLES)MAXI4) ?  (i4)btups : MAXI4;
    if (brows <= 0)
	brows = 1;
    jn->hjn_bmem = (MAXI4 / qn->qen_rsz > brows) ? brows * qn->qen_rsz : MAXI4;

    /* Join-time qualification: */
    jn->hjn_pqual = NULL;
    if (co->opo_variant.opo_local->opo_pgcount == 0
      && BTcount((char *) &co->opo_inner->opo_variant.opo_local->opo_partvars,
		global->ops_cstate.opc_subqry->ops_vars.opv_rv) > 0)
    {
	jn->hjn_pqual = opc_join_pqual(global, cnode, oceq, QE_HJOIN);
    }

    /* materialize the inner tuple; */
    opc_hash_materialize(global, cnode, &jn->hjn_ptmat, &co->opo_inner->opo_maps->opo_eqcmap, 
	&keqcmp, iceq, jn->hjn_cmplist, &jn->hjn_prow, &qn->qen_rsz, (ljoin | rjoin));
    /*	(ljoin) ? 1 : 0); */
    qn->qen_row = jn->hjn_prow;		/* stick something there */

    /* Merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->hjn_oj != (QEN_OJINFO *) NULL)
	jn->hjn_oj->oj_specialEQCrow = specialrow;

    /* qualify the joined tuple; */
    opc_jqual(global, cnode, &keqcmp, iceq, oceq, 
	&jn->hjn_jqual, &tidatt);

    /* If CO node has opo_local->opo_pgcount is non-0, set partition
    ** group processing flag - this is partition compatible join. */
    if (co->opo_variant.opo_local->opo_pgcount)
    {
	qn->qen_flags |= QEN_PART_SEPARATE;
	if (co->opo_variant.opo_local->opo_mask & OPO_PCJ_NESTED)
	    qn->qen_flags |= QEN_PART_NESTED;
	if (jn->hjn_out->qen_type == QE_TSORT)
	    jn->hjn_out->qen_flags |= (QEN_PART_SEPARATE | QEN_PART_NESTED);
	if (jn->hjn_inner->qen_type == QE_TSORT)
	    jn->hjn_inner->qen_flags |= (QEN_PART_SEPARATE | QEN_PART_NESTED);
    }

    /* Decide whether hash join should do an inner-is-empty test
    ** by reading a row from the inner before reading the build (left) side.
    ** Anything that might make the get-row do work defeats the purpose,
    ** particularly since the right side is supposed to be the larger.
    ** (After all, it's just as likely that the left side is empty, and
    ** much work could be wasted on the right subtree if the left is empty.)
    ** So, only do this check if:
    ** - the inner is an orig
    ** - the inner orig has no qualification CX (*)
    ** - there are no join-time partition quals (prefetch would break these)
    ** - the join type is inner or right join, not left or full
    **
    ** (*) A possible refinement would be to allow a qualification CX, but
    ** only if the inner is some small size, say one group-read or less.
    ** We don't want to scan a giant inner only to find the outer is empty.
    */
    jn->hjn_innercheck = FALSE;
    if ((co->opo_variant.opo_local->opo_ojid == OPL_NOOUTER
	 || co->opo_variant.opo_local->opo_type == OPL_RIGHTJOIN)
      && jn->hjn_pqual == NULL)
    {
	QEN_NODE *rhs = jn->hjn_inner;
	if (rhs->qen_type == QE_ORIG && rhs->node_qen.qen_orig.orig_qual == NULL)
	    jn->hjn_innercheck = TRUE;
    }

    /* 
    ** Eliminate the eqcs from ceq that
    ** won't be returned from this node. However, because of function attribute
    ** processing we must not do this. The call is commented out to fix
    ** bug 34959.
    */
    /* opc_jnotret(global, co, ceq ); */

    /* Finally, if no keys are nullable, reduce the join key to a single
    ** BYTE(n) key of the proper length.  We can get away with this
    ** because hash-materialize has to preprocess the keys for hashing,
    ** so a straight bit-comparison on the full join key works.
    ** (Do this even on a single key, since BYTE is always easy to compare.)
    */
    if (! jn->hjn_nullablekey)
    {
	DB_CMP_LIST *cmp = &jn->hjn_cmplist[0];

	cmp->cmp_type = DB_BYTE_TYPE;
	cmp->cmp_length = jn->hjn_cmplist[jn->hjn_hkcount-1].cmp_offset
		+ jn->hjn_cmplist[jn->hjn_hkcount-1].cmp_length;
	cmp->cmp_precision = 0;
	cmp->cmp_collID = -1;
	jn->hjn_hkcount = 1;
    }
}

/*{
** Name: OPC_FSMJOIN_BUILD	- Compile an QEN_JOIN node.
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
**      31-oct-89 (stec)
**          written
**	08-jan-91 (stec)
**	    Call to opc_jnotret() has been commented out to fix
**	    bug 34959.
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize() to reflect interface change.
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to jn->sjn_hget and jn->sjn_hcreate,
**	    which weren't used anywhere in QEF.
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**	13-aug-99 (inkdo01)
**	    Added code to augment join node EQCs for outer joins in case not
**	    null col from inner is in "is null" pred in where clause.
**	10-Apr-2007 (kschendel) SIR 122513
**	    Turn on nested-PC-join flag when needed.
[@history_template@]...
*/
VOID
opc_fsmjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_SJOIN		*jn = &cnode->opc_qennode->node_qen.qen_sjoin;
    OPE_BMEQCLS		keqcmp;
    OPC_EQ		*okceq;
    i4			dummy;
    i4			njeqcs;
    i4			tidatt;
    i4			specialrow;

    /* Compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->sjn_out, &oceq, (OPC_EQ *) NULL);

    /* Compile the inner node and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_inner, OPC_COINNER,
	&jn->sjn_inner, &iceq, (OPC_EQ *) NULL);

    /* Fill in the hold file info */
    opc_pthold(global, &jn->sjn_hfile);

    /* Fill in the uniqueness info */
    jn->sjn_kuniq = (i4) co->opo_existence;

    /* Initialize keqcmp, okceq. */
    if (co->opo_sjeqc == OPE_NOEQCLS)
	njeqcs = 0;
    else
	njeqcs = 1;

    /* If outer join, make sure node EQCs include all the EQCs of the inner 
    ** side of the outer join. This assures that "not null" columns from the
    ** inner whose only reference is in the where clause (in a "is null" pred)
    ** get null indicator space allocated. */
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
     switch (co->opo_variant.opo_local->opo_type) {
      case OPL_RIGHTJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_outer->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      case OPL_LEFTJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_inner->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      case OPL_FULLJOIN:
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_outer->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	BTor( (i4) BITS_IN( OPE_BMEQCLS ),
	    (char *)&co->opo_inner->opo_maps->opo_eqcmap,
	    (char *)&co->opo_maps->opo_eqcmap);
	break;
      default:
	break;
    }

    opc_jkinit(global, &co->opo_sjeqc, njeqcs, oceq, 
	&keqcmp, &okceq);

    /* Join-time partition qualification, only if not a PC-join.
    ** For FSM joins we actually attach this stuff to the outer tsort,
    ** since that's the collection point for the outer rows.
    */
    if (jn->sjn_out->qen_type == QE_TSORT
      && co->opo_variant.opo_local->opo_pgcount == 0
      && BTcount((char *) &co->opo_inner->opo_variant.opo_local->opo_partvars,
		global->ops_cstate.opc_subqry->ops_vars.opv_rv) > 0)
    {
	/* FIXME stuff here, then attach to tsort */
    }

    /* materialize the inner tuple; */
    opc_materialize(global, cnode, &jn->sjn_itmat, &co->opo_inner->opo_maps->opo_eqcmap, 
	iceq, &qn->qen_row, &qn->qen_rsz, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* materialize the outer key; */
    opc_materialize(global, cnode, &jn->sjn_okmat, &keqcmp, okceq, 
	&jn->sjn_krow, &dummy, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* compare the materialized outer key to the current outer key; */
    opc_kcompare(global, cnode, &jn->sjn_okcompare, co->opo_sjeqc, 
	okceq, oceq);

    /* compare the inner and outer tuples on the join key; */
    opc_kcompare(global, cnode, &jn->sjn_joinkey, co->opo_sjeqc, 
	okceq, iceq);

    /* Merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->sjn_oj != (QEN_OJINFO *) NULL)
	jn->sjn_oj->oj_specialEQCrow = specialrow;

    /* qualify the joined tuple; */
    opc_jqual(global, cnode, &keqcmp, iceq, oceq, 
	&jn->sjn_jqual, &tidatt);

    /* If CO node has opo_local->opo_pgcount is non-0, set partition
    ** group processing flag - this is partition compatible join. */
    if (co->opo_variant.opo_local->opo_pgcount)
    {
	qn->qen_flags |= QEN_PART_SEPARATE;
	if (co->opo_variant.opo_local->opo_mask & OPO_PCJ_NESTED)
	    qn->qen_flags |= QEN_PART_NESTED;
	if (jn->sjn_out->qen_type == QE_TSORT)
	    jn->sjn_out->qen_flags |= (QEN_PART_SEPARATE | QEN_PART_NESTED);
	if (jn->sjn_inner->qen_type == QE_SORT)
	    jn->sjn_inner->qen_flags |= (QEN_PART_SEPARATE | QEN_PART_NESTED);
    }

    /* 
    ** Eliminate the eqcs from ceq that
    ** won't be returned from this node. However, because of function attribute
    ** processing we must not do this. The call is commented out to fix
    ** bug 34959.
    */
    /* opc_jnotret(global, co, ceq ); */
}

/*{
** Name: OPC_CPJOIN_BUILD	- Compile an QEN_CPJOIN node.
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
**      25-aug-86 (eric)
**          written
**	08-jan-91 (stec)
**	    Call to opc_jnotret() has been commented out to fix
**	    bug 34959.
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize() to reflect interface change.
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to jn->sjn_hget and jn->sjn_hcreate,
**	    which weren't used anywhere in QEF.
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**	12-may-2008 (dougi)
**	    Added support for extra opc_jinouter() parm when inner of CPjoin
**	    is table procedure with parm dependencies on current outers.
**	25-sept-2008 (dougi)
**	    Don't use hold file for TPROC joins.
**	13-May-2010 (kiria01) b123721
**	    Revert to using hold for with TPROC if right or full outer joins
**	    implied.
*/
VOID
opc_cpjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPO_CO		*co = cnode->opc_cco, *coi;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    OPC_EQ		*tceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_SJOIN		*jn = &cnode->opc_qennode->node_qen.qen_sjoin;
    OPE_BMEQCLS		keqcmp;
    OPC_EQ		*okceq;
    i4			dummy;
    i4			njeqcs;
    i4			tidatt;
    i4			specialrow;

    /* Compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->sjn_out, &oceq, (OPC_EQ *) NULL);

    /* If the inner is a table procedure, make outer EQs visible to it. */
    if (co->opo_inner->opo_sjpr == DB_PR)
	coi = co->opo_inner->opo_outer;
    else
	coi = co->opo_inner;
    if (co->opo_tprocjoin)
	tceq = oceq;
    else
	tceq = (OPC_EQ *) NULL;

    /* Compile the inner node and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_inner, OPC_COINNER,
	&jn->sjn_inner, &iceq, tceq);

    /* Fill in the hold file info */
    if (co->opo_tprocjoin &&
	!(co->opo_variant.opo_local->opo_type == OPL_FULLJOIN ||
	  co->opo_variant.opo_local->opo_type == OPL_RIGHTJOIN))
	jn->sjn_hfile = -1;		/* no hold for TPROC joins */
    else
	opc_pthold(global, &jn->sjn_hfile);

    /* Fill in the uniqueness info */
    jn->sjn_kuniq = (i4) co->opo_existence;

    /* Initialize keqcmp and okceq. */
    if (co->opo_sjeqc == OPE_NOEQCLS)
	njeqcs = 0;
    else
	njeqcs = 1;

    opc_jkinit(global, &co->opo_sjeqc, njeqcs, oceq, 
	&keqcmp, &okceq);

    /* materialize the inner tuple; */
    opc_materialize(global, cnode, &jn->sjn_itmat, &co->opo_inner->opo_maps->opo_eqcmap, 
	iceq, &qn->qen_row, &qn->qen_rsz, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* materialize the outer key; */
    opc_materialize(global, cnode, &jn->sjn_okmat, &keqcmp, okceq, 
	&jn->sjn_krow, &dummy, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* compare the materialized outer key to the current outer key; */
    opc_kcompare(global, cnode, &jn->sjn_okcompare, co->opo_sjeqc, 
	okceq, oceq);

    /* compare the inner and outer tuples on the join key; */
    opc_kcompare(global, cnode, &jn->sjn_joinkey, co->opo_sjeqc, 
	okceq, iceq);

    /* Merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->sjn_oj != (QEN_OJINFO *) NULL)
	jn->sjn_oj->oj_specialEQCrow = specialrow;

    /* qualify the joined tuple; */
    opc_jqual(global, cnode, (OPE_BMEQCLS *)NULL, iceq, oceq,
	&jn->sjn_jqual, &tidatt);

    /* 
    ** Eliminate the eqcs from ceq that
    ** won't be returned from this node. However, because of function attribute
    ** processing we must not do this. The call is commented out to fix
    ** bug 34959.
    */
    /* opc_jnotret(global, co, ceq); */
}

/*{
** Name: OPC_ISJOIN_BUILD	- Compile an QEN_ISJOIN node.
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
**      25-aug-86 (eric)
**          written
**	08-jan-91 (stec)
**	    Call to opc_jnotret() has been commented out to fix
**	    bug 34959.
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize() to reflect interface change.
**	25-mar-92 (rickh)
**	    Right joins now store tids in a real hold file.  Some related
**	    cleanup:  removed references to jn->sjn_hget and jn->sjn_hcreate,
**	    which weren't used anywhere in QEF.
**	5-jan-94 (rickh)
**	    The WHERE clause is executed AFTER the ON clause and outer join null
**	    fabricators.  Therefore, it's clearer if we compile the WHERE
**	    clause before the outer join CXs.  One happy advantage of this
**	    approach is that it forces the WHERE clause to refer to the
**	    same locations as the outer join CXs.
**      08-may-2002 (huazh01)
**          Apply the fix for bug 98395 to opc_isjoin_build(). This fixes
**          bug 107046.
[@history_template@]...
*/
VOID
opc_isjoin_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    OPC_EQ		*oceq;
    OPC_EQ		*iceq;
    QEN_SJOIN		*jn = &cnode->opc_qennode->node_qen.qen_sjoin;
    QEN_NODE		*qn = cnode->opc_qennode;
    OPE_BMEQCLS		keqcmp;
    OPC_EQ		*okceq;
    i4			notused;
    i4			njeqcs;
    i4			tidatt;
    i4			specialrow;

    /* compile the outer node, and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_outer, OPC_COOUTER, 
	&jn->sjn_out, &oceq, (OPC_EQ *) NULL);

    /* now lets compile the inner node and fill in the sorted info */
    opc_jinouter(global, cnode, co->opo_inner, OPC_COINNER,
	&jn->sjn_inner, &iceq, (OPC_EQ *) NULL);

    /* Fill in the hold file info */
    opc_pthold(global, &jn->sjn_hfile);

    /* Fill in the uniqueness info */
    jn->sjn_kuniq = (i4) co->opo_existence;

    /* Initialize keqcmp, okceq. */
    if (co->opo_sjeqc == OPE_NOEQCLS)
	njeqcs = 0;
    else
	njeqcs = 1;
 
    /* If outer join, make sure node EQCs include all the EQCs of the inner
     ** side of the outer join. This assures that "not null" columns from the
     ** inner whose only reference is in the where clause (in a "is null" pred)
     ** get null indicator space allocated. */ 
    if (co->opo_variant.opo_local->opo_ojid != OPL_NOOUTER)
     switch (co->opo_variant.opo_local->opo_type) {
      case OPL_RIGHTJOIN:
        BTor( (i4) BITS_IN( OPE_BMEQCLS ),
            (char *)&co->opo_outer->opo_maps->opo_eqcmap,
            (char *)&co->opo_maps->opo_eqcmap);
        break;
      case OPL_LEFTJOIN:
        BTor( (i4) BITS_IN( OPE_BMEQCLS ),
            (char *)&co->opo_inner->opo_maps->opo_eqcmap,
            (char *)&co->opo_maps->opo_eqcmap);
        break;
      case OPL_FULLJOIN:
        BTor( (i4) BITS_IN( OPE_BMEQCLS ),
            (char *)&co->opo_outer->opo_maps->opo_eqcmap,
            (char *)&co->opo_maps->opo_eqcmap);
        BTor( (i4) BITS_IN( OPE_BMEQCLS ),
            (char *)&co->opo_inner->opo_maps->opo_eqcmap,
            (char *)&co->opo_maps->opo_eqcmap);
        break;
      default:
	break;
    } 

    opc_jkinit(global, &co->opo_sjeqc, njeqcs, oceq,
	&keqcmp, &okceq);

    /* materialize the inner tuple; */
    opc_materialize(global, cnode, &jn->sjn_itmat, &co->opo_inner->opo_maps->opo_eqcmap, 
	iceq, &qn->qen_row, &qn->qen_rsz, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* materialize the outer key; */
    opc_materialize(global, cnode, &jn->sjn_okmat, &keqcmp, okceq, 
	&jn->sjn_krow, &notused, (i4)TRUE, (bool)TRUE, (bool)FALSE);

    /* compare the materialized outer key to the current outer key; */
    opc_kcompare(global, cnode, &jn->sjn_okcompare, co->opo_sjeqc, 
	okceq, oceq);

    /* compare the inner and outer tuples on the join key; */
    opc_kcompare(global, cnode, &jn->sjn_joinkey, co->opo_sjeqc, 
	okceq, iceq);

    /* Merge iceq and oceq into ceq */
    opc_jmerge(global, cnode, iceq, oceq, ceq);
    cnode->opc_ceq = ceq;

    /* Initialize the opc info for special OJ eqcs. */
    opc_ojspfatts(global, cnode, &specialrow);

    /*
    ** Initialize the outer join info. Do this before we build the WHERE
    ** clause so that the WHERE clause can refer to variables filled in
    ** by the LNULL, RNULL, and OJEQUAL compiled expressions.
    */
    opc_ojinfo_build(global, cnode, iceq, oceq);
    if (jn->sjn_oj != (QEN_OJINFO *) NULL)
	jn->sjn_oj->oj_specialEQCrow = specialrow;

    /* qualify the joined tuple; */
    opc_jqual(global, cnode, &keqcmp, iceq, oceq, 
	&jn->sjn_jqual, &tidatt);

    /* 
    ** Eliminate the eqcs from ceq that
    ** won't be returned from this node;
    ** won't be returned from this node. However, because of function attribute
    ** processing we must not do this. The call is commented out to fix
    ** bug 34959.
    */
    /* opc_jnotret(global, co, ceq); */
}

/*
** Name: opc_join_pqual - Build join-time partition qualification
**
** Description:
**
**	This routine builds join-time partition qualification info
**	into the query plan when it's appropriate.  The idea here is
**	to better solve queries like:
**
**	    select ... from cal, fact
**	    where cal.caldate between 'date1' and 'date2'
**	        and fact.date_id = cal.date_id
**
**	where "fact" is some large table partitioned on date_id, and
**	"cal" is usually a much smaller table that only produces
**	a handful of rows for the between.  This sort of thing seems
**	to be popular in data warehousing schemas.  As long as the
**	partitioned table is on the INNER side of a join type that
**	reads ALL of the outer before reading ANY of the inner,
**	we can compile a which-partitions bitmap for all rows of the
**	outer, and then read only those partitions from the inner.
**
**	So much for motivations.  This routine is given two main things,
**	a bitmap of variables (tables) that are on the inner side and
**	might be interested in partitioning;  and the updated OPC_EQ
**	eqclass state array from the outer side.  (Updated in the
**	sense that materialization should have been compiled already,
**	so that all the available flags and row offset/type poop is
**	up to date.)  We loop through the listed tables, use the
**	PQBF partitioning-analysis structure to match up eqclasses,
**	and if there's a match we generate "equals" partitioning
**	qualification directives and partition qualification info.
**	Multiple info blocks may be generated, if so they are linked
**	together.  (There's one PART_QUAL per table.)
**
**	If we end up generating anything, our pqual info will refer
**	back to the ORIG node.  It's possible that the ORIG node had
**	no partition qualification of its own to do, in which case
**	we have to generate the relevant pqual stuff for it as well.
**
**	By the way, don't call this for partition-compatible (grouped)
**	joins.  PC-joining is a sort of partition restriction in itself,
**	especially since empty inputs are usually handled very quickly.
**	Also, the qualification runtime doesn't know how to deal with the
**	end-of-group signals, and it's not worth trying to teach it.
**
**	In the current version, we'll only join against simple, single-
**	column dimensions.  The extension to composite dimensions is
**	not wildly difficult, but I don't want to deal with it now.
**
** Inputs:
**	global			The usual compiler state CB
**	cnode			More state for current node
**	oceq			EQC availability array for join outer;
**				taken after any outer materialization and
**				*before* inner/outer merge
**	node_type		Join node type for putting into pqual blocks
**
** Outputs:
**	Returns first QEN_PART_QUAL * in the list, or NULL if
**	no qualification generated.
**
** History:
**	4-Aug-2007 (kschendel) SIR 122513
**	    Invent initial try at join time partition qualification.
**	31-Dec-2008 (kschendel)
**	    Oops!  right or full joins are not permitted to do join
**	    time qualification, since that would imply the inner
**	    qualifying an outer, which is incorrect.  Amazing that this
**	    has gone unnoticed until recently...
*/

static QEN_PART_QUAL *
opc_join_pqual(OPS_STATE *global,
	OPC_NODE *cnode, OPC_EQ *oceq,
	i4 node_type)
{

    bool anyeqcs;
    i4 numeqc;				/* Max # of eqc's for bit ops */
    i4 numvar;				/* Max # of vars for bit ops */
    OPE_BMEQCLS pqcand_eqcs;		/* Partition qual candidate EQC's */
    OPE_IEQCLS eqc;			/* An eqclass number */
    OPV_BMVARS *rpartvars;		/* Inner's want-pqual bitmap */
    OPV_IVARS varno;
    OPV_RT *vbase;			/* Base of var ptr array */
    QEN_PART_QUAL *first_pqual;

    /* Allow disabling with trace point OP131 */
    if (opt_strace(global->ops_cb, OPT_T131_NO_JOINPQUAL))
	return (NULL);

    /* Check that the left side isn't an inner to the right side, ie
    ** that it's not a right or full join.  Inners aren't permitted to
    ** qualify outers.
    */
    if (cnode->opc_cco->opo_variant.opo_local->opo_ojid != OPL_NOOUTER
      && (cnode->opc_cco->opo_variant.opo_local->opo_type == OPL_RIGHTJOIN
	  || cnode->opc_cco->opo_variant.opo_local->opo_type == OPL_FULLJOIN) )
	return (NULL);

    rpartvars = &cnode->opc_cco->opo_inner->opo_variant.opo_local->opo_partvars;
    numeqc = global->ops_cstate.opc_subqry->ops_eclass.ope_ev;
    numvar = global->ops_cstate.opc_subqry->ops_vars.opv_rv;
    vbase = global->ops_cstate.opc_subqry->ops_vars.opv_base;
    first_pqual = NULL;

    /* First, scan the oceq array for available eqc's, and mark those
    ** in the candidate bitmap.
    ** eqavailable ought to suffice, but check attavailable as a safety.
    */
    MEfill(sizeof(OPE_BMEQCLS), 0, (PTR) &pqcand_eqcs);
    anyeqcs = FALSE;
    for (eqc = 0; eqc < numeqc; ++eqc)
    {
	if (oceq[eqc].opc_eqavailable && oceq[eqc].opc_attavailable)
	{
	    BTset(eqc, (char *) &pqcand_eqcs);
	    anyeqcs = TRUE;
	}
    }
    if (! anyeqcs)
	return (NULL);

    /* Now chug thru the right-side partitioned tables and look for
    ** an intersection between the candidates and the partitioning columns.
    ** Note that this loop can run at most once for K/T-joins.
    */
    varno = -1;
    while ( (varno = BTnext(varno, (char *) rpartvars, numvar)) != -1)
    {
	ADE_OPERAND ops[2];		/* for materializing values */
	bool need_cx;
	DB_PART_DEF *pdefp;		/* Partition definition */
	i4 cur_offset;			/* Offset in value row */
	i4 i;
	i4 map_bytes;			/* Bytes in a phys-partition bitmap */
	i4 map_row[2], mrix;		/* PP bitmaps for use in directives */
	i4 nqual_eqcs;			/* # ones in qual_eqcs */
	i4 value_row, value_base;
	OPB_PQBF *pqbfp;		/* var's partition qual analysis */
	OPC_ADF cadf;			/* CX compilation state */
	OPE_BMEQCLS qual_eqcs;
	OPV_VARS *varp;
	QEN_PART_QUAL *orig_pqual;	/* pqual info for source orig node */
	QEN_PART_QUAL *pqual;		/* Partition qual CB for the qp */
	QEN_PQ_EVAL *pqe;		/* Current evaluation directive */
	QEN_PQ_EVAL *pqe_base;		/* Base of directives array */
	QEN_PQ_EVAL *pqe_end;		/* End of same */

	varp = vbase->opv_rt[varno];
	pqbfp = varp->opv_pqbf;
	MEcopy((PTR) &pqbfp->opb_sdimeqcs, sizeof(OPE_BMEQCLS), (PTR) &qual_eqcs);
	BTand(numeqc, (char *) &pqcand_eqcs, (char *) &qual_eqcs);
	nqual_eqcs = BTcount((char *) &qual_eqcs, numeqc);
	if (nqual_eqcs == 0)
	    continue;

	/* It appears that we have a winner.  Churn out the QEN_PART_QUAL
	** and QEN_PQ_EVAL stuff, not to mention a CX to line up values.
	*/

	orig_pqual = opc_source_pqual(global, varp);
	if (node_type == QE_KJOIN || node_type == QE_TJOIN)
	{
	    /* This is a special case, K-joins and T-joins only.
	    ** K/T-joins are both a join and an orig, with nothing on
	    ** the inner but the keyed table.  So either there was a
	    ** pqual there already (describing constant quals against
	    ** the innner), or opc-source-pqual just now created one.
	    ** We want to add join-time qual info to that pqual, not
	    ** make another one.
	    */
	    pqual = orig_pqual;
	}
	else
	{
	    /* The normal case, create a new pqual and set it up */
	    pqual = (QEN_PART_QUAL *) opu_qsfmem(global, sizeof(QEN_PART_QUAL));
	    /* pqual is zeroed, don't need to null out unused stuff */
	    pqual->part_qnext = first_pqual;
	    pqual->part_orig = orig_pqual->part_orig; /* Link back to orig node */
	    pqual->part_pqual_ix = global->ops_cstate.opc_qp->qp_pqual_cnt++;
	    pqual->part_node_type = node_type;
	}

	/* This partition def is the one in the QP, with the row
	** offsets set up.
	*/
	pdefp = orig_pqual->part_qdef;
	map_bytes = (pdefp->nphys_parts + 7) / 8;
	/* lresult already allocated if orig-like K/T-join pqual... */
	if (node_type == QE_KJOIN || node_type == QE_TJOIN)
	{
	    /* for K/T-join, lresult is already set.
	    ** Also, since lresult is recomputed on each row, arrange for
	    ** the bitmap directives to compute right into lresult.
	    */
	    map_row[0] = pqual->part_lresult_ix;
	}
	else
	{
	    opc_ptrow(global, &pqual->part_lresult_ix, map_bytes);

	    /* Join pqual doesn't need the constmap or the work1 row */
	    /* We will however need two more physical-partition bitmaps,
	    ** one for the this-row result and one as a temp for AND'ing.
	    */
	    opc_ptrow(global, &map_row[0], map_bytes);
	}
	opc_ptrow(global, &map_row[1], map_bytes);

	first_pqual = pqual;		/* Link multiple pquals together */

	/* Allocate the directives array.  Start with an initial size guess
	** of #eqcs values + #eqcs-1 AND's, plus a trailing OR, plus a
	** header.  Using the unadjusted entry size leaves some extra
	** just in case.  (If it's too small, new-pqe will reallocate.)
	*/
	i = sizeof(QEN_PQ_EVAL) * (nqual_eqcs * 2 + 1);
	pqe_base = (QEN_PQ_EVAL *) opu_Gsmemory_get(global, i);
	pqe = pqe_base;
	pqe_end = (QEN_PQ_EVAL *) ((char *) pqe_base + i);
	/* Always starts with a header */
	pqe->pqe_eval_op = PQE_EVAL_HDR;
	pqe->pqe_size_bytes = sizeof(QEN_PQ_EVAL) - sizeof(pqe->un)
				+ sizeof(pqe->un.hdr);
	pqe->pqe_result_row = map_row[0];
	pqe->un.hdr.pqe_nbytes = pqe->pqe_size_bytes;
	pqe->un.hdr.pqe_ninstrs = 1;
	pqe->un.hdr.pqe_cx = NULL;

	/* Do a pre-scan of the eqclasses to see if they all
	** have identical types / lengths / precisions to the
	** matching partitioned column.  If so, we can eliminate
	** the CX altogether, which is a nice optimization.
	** I suppose we could mix-and-match, but it's likely that
	** the majority of the CX execution overhead is startup
	** cost; just saving a move or two doesn't matter much.
	** NOTE: the eqc's also have to all be in the same source
	** row.  Could cure that by moving the pqe value-row # into
	** the value union, instead of being fixed for the entire
	** eval.
	*/
	eqc = -1;
	need_cx = FALSE;
	value_row = -1;
	while ( (eqc = BTnext(eqc, (char *) &qual_eqcs, numeqc)) != -1)
	{
	    DB_PART_LIST *pcolp;	/* Partition column info */
	    DB_PART_DIM *dimp;		/* Partition dimension info */
	    i4 abstype;
	    i4 dim, col_index;
	    i4 source_row;
	    OPC_EQ *eqc_ceq;		/* oceq[eqc] eqc where-state */

	    eqc_ceq = &oceq[eqc];
	    source_row = eqc_ceq->opc_dshrow;
	    if (value_row == -1)
		value_row = source_row;
	    else if (value_row != source_row)
	    {
		need_cx = TRUE;
		break;
	    }
	    /* Get dimension, column-index (always 1!) for this eqc */
	    opb_pqbf_findeqc(pdefp, pqbfp, eqc, &dim, &col_index);
	    dimp = &pdefp->dimension[dim];
	    pcolp = &dimp->part_list[col_index];
	    abstype = abs(pcolp->type);
	    if (pcolp->type != eqc_ceq->opc_eqcdv.db_datatype
	      || abstype == DB_NCHR_TYPE
	      || abstype == DB_NVCHR_TYPE
	      || pcolp->length != eqc_ceq->opc_eqcdv.db_length
	      || (pcolp->precision != eqc_ceq->opc_eqcdv.db_prec
		  && abstype == DB_DEC_TYPE) )
	    {
		need_cx = TRUE;
		break;
	    }
	}

	if (need_cx)
	{
	    /* Open a CX to generate value materializers.  Since we're
	    ** working entirely with already-materialized eqc's, this CX
	    ** will (usually) only be asked to move things around, or in
	    ** some cases perhaps coerce a data value to a different length.
	    */
	    opc_adalloc_begin(global, &cadf, &pqe->un.hdr.pqe_cx,
		    nqual_eqcs, 2*nqual_eqcs, 0, 0,
		    cnode->opc_below_rows, OPC_STACK_MEM);
	    value_row = global->ops_cstate.opc_qp->qp_row_cnt;
	    opc_adbase(global, &cadf, QEN_ROW, value_row);
	    value_base = cadf.opc_row_base[value_row];
	}

	/* Loop thru the nominated EQC's.  Generate a "move" to put
	** the value in the right place with the right type in the value
	** row, and generate a VALUE directive to test it.  (Also,
	** an AND directive to combine the results, unless it's the
	** very first one.)
	*/

	eqc = -1;
	mrix = -1;
	cur_offset = 0;
	while ( (eqc = BTnext(eqc, (char *) &qual_eqcs, numeqc)) != -1)
	{
	    DB_PART_LIST *pcolp;	/* Partition column info */
	    DB_PART_DIM *dimp;		/* Partition dimension info */
	    i4 abstype;
	    i4 dim, col_index;
	    i4 moveop;
	    i4 source_row;
	    OPC_EQ *eqc_ceq;		/* oceq[eqc] eqc where-state */

	    ++mrix;
	    eqc_ceq = &oceq[eqc];
	    /* Get dimension, column-index (always 1!) for this eqc */
	    opb_pqbf_findeqc(pdefp, pqbfp, eqc, &dim, &col_index);
	    dimp = &pdefp->dimension[dim];
	    pcolp = &dimp->part_list[col_index];
	    opc_pqe_new(global, &pqe_base, &pqe_end, &pqe, PQE_EVAL_VALUE);
	    pqe->pqe_result_row = map_row[mrix];
	    pqe->un.value.pqe_dim = dim;
	    pqe->un.value.pqe_relop = PQE_RELOP_EQ;
	    pqe->un.value.pqe_nvalues = 1;
	    pqe->un.value.pqe_value_len = DB_ALIGN_MACRO(pcolp->row_offset + pcolp->length);

	    if (!need_cx)
	    {
		/* If the values can come directly out of the EQC row,
		** just adjust the offset.  The whichpart is going to
		** look at pqe_start_offset + pcolp row offset.
		*/
		pqe->un.value.pqe_start_offset = eqc_ceq->opc_dshoffset - pcolp->row_offset;
	    }
	    else
	    {
		/* Normal case, building a CX.
		** Do the usual operand fiddling.  I suppose we could use
		** orig's opc-pq-move utility, but it is serious overkill for
		** this simple case.  Maybe if we implement composite dim...
		*/

		/* if (the row for the eqc isn't in the row to base map) */
		pqe->un.value.pqe_start_offset = cur_offset;
		source_row = eqc_ceq->opc_dshrow;
		if (cadf.opc_row_base[source_row] < ADE_ZBASE ||
		    cadf.opc_row_base[source_row] >= 
			    ADE_ZBASE + pqe_base->un.hdr.pqe_cx->qen_sz_base )
		{
		    /* add the row to the base array and give it a mapping */
		    opc_adbase(global, &cadf, QEN_ROW, source_row);
		}

		ops[1].opr_dt = eqc_ceq->opc_eqcdv.db_datatype;
		ops[1].opr_len = eqc_ceq->opc_eqcdv.db_length;
		ops[1].opr_prec = eqc_ceq->opc_eqcdv.db_prec;
		ops[1].opr_collID = eqc_ceq->opc_eqcdv.db_collID;
		ops[1].opr_base = cadf.opc_row_base[source_row];
		ops[1].opr_offset = eqc_ceq->opc_dshoffset;
		ops[0].opr_dt = pcolp->type;
		ops[0].opr_len = pcolp->length;
		ops[0].opr_prec = pcolp->precision;
		ops[0].opr_collID = ops[1].opr_collID;
		ops[0].opr_base = value_base;
		/* Note that this row-offset should be zero for simple dims */
		ops[0].opr_offset = cur_offset + pcolp->row_offset;
		abstype = abs(ops[0].opr_dt);
		/* Not 100% sure that we need unicode norming here,
		** but I'll leave it in until shown wrong.
		*/
		moveop = ADE_MECOPY;
		if (abstype == DB_NCHR_TYPE || abstype == DB_NVCHR_TYPE
		  || ( (global->ops_adfcb->adf_utf8_flag & AD_UTF8_ENABLED)
			&& (abstype == DB_VCH_TYPE
			    || abstype == DB_CHA_TYPE
			    || abstype == DB_TXT_TYPE
			    || abstype == DB_CHR_TYPE)) )
		    moveop = ADE_UNORM;

		/* If the source and result datatype/length/precision
		** matches, just move.  If datatypes are the same unicode
		** type, just unorm (even if lengths / nullabilities differ).
		** Otherwise, coerce to target position.
		*/
		if (abstype == abs(ops[1].opr_dt)
		  && (moveop == ADE_UNORM
		      || (ops[0].opr_len == ops[1].opr_len
			  && ops[0].opr_dt == ops[1].opr_dt
			  && (ops[0].opr_prec == ops[1].opr_prec
			      || abstype != DB_DEC_TYPE) ) ) )
		{
		    opc_adinstr(global, &cadf, moveop, ADE_SMAIN,
				2, ops, 1);
		}
		else if (ops[0].opr_dt > 0 && ops[1].opr_dt < 0)
		{
		    /* Nullable column qualifying a non-nullable partitioning
		    ** column is weird and takes special handling to
		    ** avoid AD1012's.
		    ** If a NULL does in fact arrive from the join, obviously
		    ** it doesn't qualify any partitions.  Since we're
		    ** OR'ing all the row results together, a NULL ideally
		    ** would produce a zero bitmap, or skip the OR altogether.
		    ** Unfortunately the CX can't communicate this to the
		    ** evaluation driver running the whichparts and bitmaps.
		    ** The answer to avoid AD1012's is to coerce to a
		    ** nullable variant of the data type, and let adtransform
		    ** put the answer in the temp row (setresop TRUE).
		    ** We'll then move all but the null indicator to the
		    ** value row.  This way, non-null inputs qualify properly;
		    ** and if a NULL arrives, some random junk value qualifies,
		    ** but no AD1012 error.  (Qualifying too much is OK,
		    ** just means that we might read an extra partition.)
		    */
		    ADE_OPERAND save_op0;

		    STRUCT_ASSIGN_MACRO(ops[0], save_op0);
		    ops[0].opr_dt = -ops[0].opr_dt;
		    ++ ops[0].opr_len;
		    opc_adtransform(global, &cadf, &ops[1], &ops[0],
				ADE_SMAIN, (bool) TRUE, (bool) FALSE);
		    STRUCT_ASSIGN_MACRO(ops[0], ops[1]);
		    STRUCT_ASSIGN_MACRO(save_op0, ops[0]);
		    ops[1].opr_dt = abs(ops[1].opr_dt);
		    --ops[1].opr_len;
		    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN,
				2, ops, 1);
		}
		else
		{
		    opc_adtransform(global, &cadf, &ops[1], &ops[0],
				ADE_SMAIN, (bool) FALSE, (bool) FALSE);
		}
		cur_offset += pqe->un.value.pqe_value_len;
	    }
	    /* Finish up with an AND to the join-row-result bitmap */
	    if (mrix > 0)
	    {
		opc_pqe_new(global, &pqe_base, &pqe_end, &pqe, PQE_EVAL_ANDMAP);
		pqe->pqe_result_row = map_row[mrix-1];
		pqe->un.andor.pqe_source_row = map_row[mrix];
		--mrix;
	    }
	} /* BTnext on eqcs */
	/* Generate one last OR directive to combine the row result
	** with the overall join result.  (We can skip this for K/T-joins
	** as they generate right into lresult for each row;  they
	** don't accumulate for the duration of the join.)
	*/
	if (node_type != QE_KJOIN && node_type != QE_TJOIN)
	{
	    opc_pqe_new(global, &pqe_base, &pqe_end, &pqe, PQE_EVAL_ORMAP);
	    pqe->pqe_result_row = pqual->part_lresult_ix;
	    pqe->un.andor.pqe_source_row = map_row[0];
	}

	/* Close the compilation and the directives array.  "pqe"
	** points to the last directive generated.
	*/

	if (!need_cx)
	{
	    pqe_base->un.hdr.pqe_value_row = value_row;
	}
	else
	{
	    opc_ptrow(global, &value_row, cur_offset);
	    cadf.opc_qeadf->qen_output = value_row;
	    pqe_base->un.hdr.pqe_value_row = value_row;
	    opc_adend(global, &cadf);
	}

	pqe = (QEN_PQ_EVAL *) ((char *) pqe + pqe->pqe_size_bytes);
	i = (char *)pqe - (char *) pqe_base;
	i = DB_ALIGN_MACRO(i);		/* Just in case */
	pqual->part_join_eval = (QEN_PQ_EVAL *) opu_qsfmem(global, i);
	MEcopy((PTR) pqe_base, i, (PTR) pqual->part_join_eval);

    } /* BTnext on vars */

    return (first_pqual);

} /* opc_join_pqual */

/*
** Name: opc_source_pqual - Find/Build QEN_PART_QUAL for source
**
** Description:
**	This routine is called when a join-time partition qualification
**	is being constructed.  The join-time QEN_PART_QUAL isn't
**	any good on its own, there has to be a QEN_PART_QUAL in the
**	source ORIG node as well.  Actually, it might not be an ORIG,
**	it might be a K-join or T-join inner.  In any case, when
**	that source was compiled, if it didn't have any constant
**	partition qualification predicates, no QEN_PART_QUAL was
**	built.
**
**	This routine checks the source node in the QP, which being
**	down underneath somewhere has already been compiled, and
**	checks to make sure it has a QEN_PART_QUAL.  If it doesn't,
**	one is constructed and glued onto the source node.
**
** Inputs:
**	global		Optimizer session state info
**	varp		OPV_VARS pointer to local range var info
**
** Outputs:
**	Returns QEN_PART_QUAL out of source node recorded for that var.
**	(Constructs one if necessary.)
**
** History:
**	4-Aug-2007 (kschendel) SIR 122513
**	    Write join time partition qualification.
*/

static QEN_PART_QUAL *
opc_source_pqual(OPS_STATE *global, OPV_VARS *varp)

{

    DB_PART_DEF *pdefp;		/* Partition definition pointer */
    i4 i;
    i4 map_bytes;
    i4 totparts;
    OPB_PQBF *pqbfp;		/* Var's partition analysis info */
    QEN_NODE *source;		/* Source node as recorded in the pqbf */
    QEN_PART_QUAL *pqual;	/* New part-qual info block */
    QEN_PART_QUAL **pqualpp;	/* part-qual slot in source node */

    pqbfp = varp->opv_pqbf;
    source = pqbfp->opb_source_node;
    if (source->qen_type == QE_ORIG)
	pqualpp = &source->node_qen.qen_orig.orig_pqual;
    else if (source->qen_type == QE_KJOIN)
	pqualpp = &source->node_qen.qen_kjoin.kjoin_pqual;
    else if (source->qen_type == QE_TJOIN)
	pqualpp = &source->node_qen.qen_tjoin.tjoin_pqual;
    else
	opx_error(E_OP0885_COTYPE);

    if (*pqualpp != NULL)
	return (*pqualpp);

    /* Looks like we get to invent a QEN_PART_QUAL */
    pqual = (QEN_PART_QUAL *) opu_qsfmem(global, sizeof(QEN_PART_QUAL));
    /* pqual is zeroed, don't need to null out unused stuff */
    pqual->part_orig = source;	/* "Self" link to source node */
    pqual->part_node_type = source->qen_type;
    pqual->part_pqual_ix = global->ops_cstate.opc_qp->qp_pqual_cnt++;
    pdefp = varp->opv_grv->opv_relation->rdr_parts;
    totparts = pdefp->nphys_parts;
    map_bytes = (totparts + 7) / 8;
    opc_ptrow(global, &pqual->part_lresult_ix, map_bytes);
    opc_ptrow(global, &pqual->part_constmap_ix, map_bytes);
    pqual->part_work1_ix = -1;
    if (pdefp->ndims > 1)
	opc_ptrow(global, &pqual->part_work1_ix, map_bytes);
    /* Just copy the whole partition def to the QP, even though we might not
    ** need some dimensions.  It's very unusual to have more than a couple
    ** of dimensions anyway.  Then, fix up column row-offsets to start at
    ** zero and increment by column length plus alignment, that's how
    ** we'll compile values into the value row.
    */
    opc_partdef_copy(global, pdefp, opu_qsfmem, &pqual->part_qdef);
    pdefp = pqual->part_qdef;		/* Work with the copy now */
    for (i = 0; i < pdefp->ndims; ++i)
    {
	DB_PART_DIM *dimp;
	DB_PART_LIST *pcol;
	i4 offset = 0;

	dimp = &pdefp->dimension[i];
	for (pcol = &dimp->part_list[0];
	     pcol < &dimp->part_list[dimp->ncols];
	     ++pcol)
	{
	    pcol->row_offset = offset;
	    /* Alignment isn't strictly needed, but probably can't hurt */
	    offset += DB_ALIGN_MACRO(pcol->length);
	}
    }

    /* Attach to source node.  For true orig's, one extra complication:
    ** if the orig is running a PC-join, an additional bitmap is needed
    ** to track the grouping vs the overall qualification.
    */
    *pqualpp = pqual;
    if (source->qen_type == QE_ORIG)
    {
	QEN_PART_INFO *partp = source->node_qen.qen_orig.orig_part;
	if (partp != NULL && partp->part_gcount > 0
	  && partp->part_groupmap_ix < 0)
	    opc_ptrow(global, &partp->part_groupmap_ix, map_bytes);
    }
    return (pqual);
} /* opc_source_pqual */
