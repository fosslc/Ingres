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
#include    <cui.h>

/**
**
**  Name: OPCSORTS.C - Routines to create QEN_SORT and QEN_TSORTS nodes.
**
**  Description:
**      This file holds all of the routines that are specific to creating 
**      sort nodes (both QEN_SORT and QEN_TSORT). Routines from files are 
**      used that include: OPCQP.C, OPCQEN.C, OPCTUPLE.C, plus files 
**      from other facilies and the rest of OPF. 
**
**	Externally visable routines:
**          opc_sort_build() - Build a QEN_SORT node
**          opc_tsort_build() - Build a QEN_TSORT node.
**          opc_tloffset() - Fill in OPCs target list offset info structure.
**
**	Internal only routines:
**          opc_tscatts() - Build atts from a target list to create for sort.
**          opc_escatts() - Build atts from eqcs to create for sort.
**          opc_tsoatts() - Build atts from a sort tree list 
**						    that gives the sort order
**          opc_esoatts() - build atts from eqc(s) that gives the sort order
**          opc_tstarget() - Materialize a target list for a top sort node.
**          opc_tscresdom() - Compile a resdom for a top sort
**          opc_tlqnatts() - Fill in a qen_atts array from a qtree target list
**          opc_sortable() - Is datatype in question sortable?
**
**  History:
**      20-oct-86 (eric)
**          written
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	18-jul-89 (jrb)
**	    Additional precision field initialization for decimal project.
**      31-Oct-89 (fred)
**          Added support to opc_{ts,es}oatts() for unsortable datatypes.  For
**	    these datatypes, we change the sort description to specify a single
**	    DB_DIFFERENT_TYPE at the end.  Thus, all current duplicates tuples,
**	    ignoring the different attributes are deleted, and all different
**	    attributes are kept. To this end, added the opc_sortable() routine.
**	26-mar-90 (stec)
**	    Removed unused forward reference to opc_tlnext().
**	27-apr-90 (stec)
**	    Fix the problem related to generating MEcopy instructions
**	    with length value of -1 for repeat updates and deletes in
**	    opc_tscresdom().
**	20-jun-90 (stec)
**	    Fixed a potential problem in opc_esoatts() and opc_escatts()
**	    related to the fact that BTcount() in opc_jqual was counting
**	    bits in the whole bitmap.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization in opc_tlqnatts(),
**	    opc_tscresdom().
**	29-jan-91 (stec)
**	    Changed opc_sort_build(), opc_tsort_build().
**      18-mar-92 (anitap)
**          Changed opc_tscresdom() to fix bug 42363. See opc_cqual() in
**          OPCQUAL.C for details.
**      17-mar-1993 (stevet)
**          Changed "db_datatype == DB_DEC_TYPE" to abs(db_datatype) 
**          == DB_DEC_TYPE to support nullable DB_DEC_TYPE.
**      29-Jun-1993 (fred)
**          Added include of adp.h to facilitate correct calculations
**          of target list offsets for queries involving large objects.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**       7-Apr-1994 (fred)
**          Corrected bug fix (which now correctly fixes 59662) in
**          opc_tscresdom().
**	1-nov-95 (inkdo01)
**	    Changed code to only sort on the "key" columns (ORDER BY, 
**	    GROUP BY, join key columns), rather than all columns.
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
**      06-mar-1996 (nanpr01)
**          Use the appropriate page size for sort/tsort.
**      06-Nov-98 (hanal04)
**          When a DSQL query has a sort node based on a variable supplied
**          at open cursor time, the pst_stlist may have unresolved
**          dataval information. Resolve this information in opc_tscatts().
**          b91409.
**	17-dec-04 (inkdo01)
**	    Added many instances of collID.
**	31-dec-04 (inkdo01)
**	    Added parm to opc_materialize() calls.
**	2-Dec-2005 (kschendel)
**	    Build DB_CMP_LIST's here as well.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	3-Jun-2009 (kschendel) b122118
**	    Delete unused adbase parameters.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_sort_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
void opc_tsort_build(
	OPS_STATE *global,
	OPC_NODE *cnode);
static void opc_tscatts(
	OPS_STATE *global,
	i4 ntargs,
	PST_QNODE *target,
	DMF_ATTR_ENTRY ***patts,
	i4 *pacount);
static void opc_escatts(
	OPS_STATE *global,
	OPE_BMEQCLS *seqcmp,
	OPC_EQ *ceq,
	DMF_ATTR_ENTRY ***patts,
	i4 *pacount);
static void opc_tsoatts(
	OPS_STATE *global,
	i4 ntargs,
	PST_QNODE *target,
	PST_QNODE *sortlist,
	DMT_KEY_ENTRY ***pkeys,
	DB_CMP_LIST **pcmplist,
	i4 *pkcount,
	i4 dups);
static void opc_esoatts(
	OPS_STATE *global,
	OPE_BMEQCLS *peqcmp,
	OPC_EQ *ceq,
	OPE_IEQCLS *ordeqcs,
	i4 *nordeqcs,
	DMT_KEY_ENTRY ***pkeys,
	DB_CMP_LIST **pcmplist,
	i4 dups);
static void opc_tstarget(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	QEN_ADF **qadf,
	i4 *poutrow,
	i4 *poutsize);
void opc_tloffsets(
	OPS_STATE *global,
	PST_QNODE *root,
	OPC_TGINFO *tginfo,
	i4 topsort);
static void opc_tscresdom(
	OPS_STATE *global,
	OPC_NODE *cnode,
	PST_QNODE *root,
	OPC_ADF *cadf,
	i4 *outsize);
static void opc_tlqnatts(
	OPS_STATE *global,
	OPC_NODE *cnode,
	QEN_NODE *qn,
	OPC_TGINFO *tginfo,
	PST_QNODE *top_resdom);
static i4 opc_sortable(
	OPS_STATE *global,
	DB_DT_ID datatype);

/*
**  Defines of other constants.
*/
#define     OPC_UNKNOWN_LEN    0
#define     OPC_UNKNOWN_PREC   0

/*{
** Name: OPC_SORT_BUILD	- Build a QEN_SORT node.
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
**      20-oct-86 (eric)
**          written
**	14-nov-86 (daved)
**	    change use of DMS_CB to DMR_CB.
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize() to reflect interface change.
**	1-nov-95 (inkdo01)
**	    Changed code to only sort on the "key" columns (ORDER BY, 
**	    GROUP BY, join key columns), rather than all columns.
**      06-mar-1996 (nanpr01)
**          Use the appropriate page size for sort/tsort.
[@history_template@]...
*/
VOID
opc_sort_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_SORT		*st = &cnode->opc_qennode->node_qen.qen_sort;
    OPC_NODE		outer_cnode;

    /* compile the outer tree */
    STRUCT_ASSIGN_MACRO(*cnode, outer_cnode);
    if (cnode->opc_invent_sort == TRUE)
    {
	outer_cnode.opc_cco = co;
    }
    else
    {
	outer_cnode.opc_cco = co->opo_outer;
    }
    outer_cnode.opc_invent_sort = FALSE;
    opc_qentree_build(global, &outer_cnode);
    st->sort_out = outer_cnode.opc_qennode;
    cnode->opc_below_rows += outer_cnode.opc_below_rows;

    /* get the cb's for the creation and loading of the sorter */
    opc_ptcb(global, &st->sort_create, sizeof (DMT_CB));
    opc_ptcb(global, &st->sort_load, sizeof (DMR_CB));
    opc_ptsort(global, &st->sort_shd);

    /* materialize the inner tuple */
    opc_materialize(global, cnode, &st->sort_mat, &co->opo_maps->opo_eqcmap, ceq,
	&qn->qen_row, &qn->qen_rsz, (i4)FALSE, (bool)TRUE, (bool)FALSE);

    /* only drop dups if explicitly told to do so. This also disables the 
    ** "only sort on key columns" change (because of odd cases where it seems
    ** to rely on the additional sort keys in its dups dropping strategies),
    ** so it's to our advantage to limit the dups dropping cases. */
    if ((subqry->ops_duplicates == OPS_DKEEP || subqry->ops_duplicates == 
		OPS_DUNDEFINED) && co->opo_odups == OPO_DDEFAULT
	)
    {
	st->sort_dups = 0;
    }
    else if (co->opo_odups == OPO_DREMOVE ||
		subqry->ops_duplicates == OPS_DREMOVE
	    )
    {
	st->sort_dups = DMR_NODUPLICATES;
    }
    else
    {
	/* error */
    }

    /* Fill in the attributes to give to the sorter */
    opc_escatts(global, &co->opo_maps->opo_eqcmap, ceq, &st->sort_atts, &st->sort_acount);

    /* Fill in the atts to sort on */
    opc_esoatts(global, &co->opo_maps->opo_eqcmap, ceq, &co->opo_ordeqc, 
		&st->sort_sacount, &st->sort_satts, &st->sort_cmplist,
		st->sort_dups);
    st->sort_pagesize = co->opo_cost.opo_pagesize; 
    
}

/*{
** Name: OPC_TSORT_BUILD	- Build a QEN_TSORT node.
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
**      20-oct-86 (eric)
**          written
**      10-may-89 (eric)
**          Changed the usage of opc_qtqnatts() to opc_tlqnatts().
**	29-jan-91 (stec)
**	    Changed code calling opc_materialize() to reflect interface change.
**	1-nov-95 (inkdo01)
**	    Changed code to only sort on the "key" columns (ORDER BY, 
**	    GROUP BY, join key columns), rather than all columns.
**      06-mar-1996 (nanpr01)
**          Use the appropriate page size for sort/tsort.
**      02-nov-2002 (wanfr01)
**	    Bug 107964, INGSRV 1797
**          Orig node page size may not be big enough to hold required tuple.
**          Use appropriate page size to avoid E_US07FD
**	06-Jul-06 (kiria01) b116230
**	    Look for resdoms marked with PST_RS_DEFAULTED as not supplied by user.
**	    This is to enable INSERT DISTINCT to exclude these from the
**	    uniqueness checks.
**	30-Jul-2007 (kschendel) SIR 122513
**	    Init partition qual pointer for tsort.
**	14-Mar-2008 (kiria01) b119899
**	    Move PST_RS_DEFAULTED check into opc_tsoatts() as not all DEFAULTED
**	    will be at the head of the list now.
[@history_template@]...
*/
VOID
opc_tsort_build(
		OPS_STATE   *global,
		OPC_NODE    *cnode)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    OPO_CO		*co = cnode->opc_cco;
    OPC_EQ		*ceq = cnode->opc_ceq;
    QEN_NODE		*qn = cnode->opc_qennode;
    QEN_TSORT		*st = &cnode->opc_qennode->node_qen.qen_tsort;
    OPC_NODE		outer_cnode;
    PST_QNODE		*root;

    /* compile the outer tree */
    STRUCT_ASSIGN_MACRO(*cnode, outer_cnode);
    if (cnode->opc_invent_sort == TRUE)
    {
	outer_cnode.opc_cco = co;
    }
    else
    {
	outer_cnode.opc_cco = co->opo_outer;
    }
    outer_cnode.opc_invent_sort = FALSE;
    opc_qentree_build(global, &outer_cnode);
    st->tsort_out = outer_cnode.opc_qennode;
    cnode->opc_below_rows += outer_cnode.opc_below_rows;
    st->tsort_pqual = NULL;		/* FSM parent will fill in if needed */

    /* get the cb's for the creation and loading of the sorter */
    opc_ptcb(global, &st->tsort_create, sizeof (DMT_CB));
    opc_ptcb(global, &st->tsort_load, sizeof (DMR_CB));
    opc_ptcb(global, &st->tsort_get, sizeof (DMR_CB));
    opc_ptsort(global, &st->tsort_shd);

    /* only drop dups if explicitly told to do so. This also disables the 
    ** "only sort on key columns" change (because of odd cases where it seems
    ** to rely on the additional sort keys in its dups dropping strategies),
    ** so it's to our advantage to limit the dups dropping cases. */
    if ((subqry->ops_duplicates == OPS_DKEEP || subqry->ops_duplicates ==
		OPS_DUNDEFINED) && co->opo_odups == OPO_DDEFAULT
	)
    {
	st->tsort_dups = 0;
    }
    else if (co->opo_odups == OPO_DREMOVE ||
		subqry->ops_duplicates == OPS_DREMOVE
	    )
    {
	st->tsort_dups = DMR_NODUPLICATES;
    }
    else
    {
	/* error */
    }

    if (cnode->opc_level == OPC_COOUTER)
    {
	/* materialize the outer tuple */
	opc_materialize(global, cnode, &st->tsort_mat, &co->opo_maps->opo_eqcmap, 
	    ceq, &qn->qen_row, &qn->qen_rsz, (i4)FALSE, (bool)TRUE, (bool)FALSE);

	/* Fill in the attributes to give to the sorter */
	opc_escatts(global, &co->opo_maps->opo_eqcmap, ceq, &st->tsort_atts,
	    &st->tsort_acount);

	/* Fill in the atts to sort on */
	opc_esoatts(global, &co->opo_maps->opo_eqcmap, ceq, &co->opo_ordeqc, 
		&st->tsort_scount, &st->tsort_satts, &st->tsort_cmplist,
		st->tsort_dups);
    }
    else if (cnode->opc_level == OPC_COTOP)
    {
	PST_QNODE     *sortcols;
	i4            sccount;
	root = subqry->ops_root;

 	opc_tstarget(global, cnode, root->pst_left, &st->tsort_mat, 
						&qn->qen_row, &qn->qen_rsz);

	opc_tscatts(global, subqry->ops_compile.opc_sortinfo.opc_nresdom,
			    root->pst_left, &st->tsort_atts, &st->tsort_acount);

	sccount = subqry->ops_compile.opc_sortinfo.opc_nresdom;
	sortcols = root->pst_left;
	/* If grouping column sort for func. aggs, assure we only sort
	** on grouping columns! */
	if ((subqry->ops_sqtype == OPS_FAGG || subqry->ops_sqtype == OPS_RFAGG) 
		&& subqry->ops_byexpr && st->tsort_dups != DMR_NODUPLICATES)
	{
	    /* compute count of group by cols */
	    for (sccount = 0, sortcols = subqry->ops_byexpr; sortcols &&
		sortcols->pst_sym.pst_type == PST_RESDOM;
		sccount++, sortcols = sortcols->pst_left)
		    ;  /* Note: null for-loop */

	    sortcols = subqry->ops_byexpr;   /* start of group by cols */
	}
	opc_tsoatts(global, sccount, sortcols,	
			global->ops_qheader->pst_stlist,
			&st->tsort_satts, &st->tsort_cmplist,
			&st->tsort_scount, st->tsort_dups);

	/* Fill in qen_atts and qen_natts */
	opc_tlqnatts(global, cnode, qn, 
			&subqry->ops_compile.opc_sortinfo, root->pst_left);
    }
    else
    {
	/*error;*/
    }
    st->tsort_pagesize = opc_pagesize(global->ops_cb->ops_server, qn->qen_rsz);

}

/*{
** Name: OPC_TSCATTS	- Build atts from a target list to create for sort
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
**      20-oct-86 (eric)
**          written
**      5-jul-95 (inkdo01)
**          added null termination test to RESDOM for-loop (b64915)
**      06-Nov-98 (hanal04)
**          Resolve PST_SORT nodes that did not have dataval information
**          available at prepare time. b91409.
**	7-Apr-2004 (schka24)
**	    Fill in attr default stuff in case anyone looks at it
**	2-Dec-2005 (kschendel)
**	    Be a little more clever about memory allocation.
**	    Sortlist entries are sorts, not resdom's!  Worked by accident.
**      09-Jun-2009 (coomi01) b121805,b117256
**          Supress SEQOP code generation.
**      11-May-2010 (coomi01) b123652
**          Adjust number of attributes present downwards on a SEQOP skip.
**
[@history_template@]...
*/
static VOID
opc_tscatts(
	OPS_STATE	*global,
	i4		ntargs,
	PST_QNODE	*target,
	DMF_ATTR_ENTRY	***patts,    /* ptr to a ptr to an array of ptrs to atts */
	i4		*pacount )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DMF_ATTR_ENTRY	**atts;
    DMF_ATTR_ENTRY	*att;
    PST_QNODE		*resdom;
    PST_QNODE           *sort = NULL;
    i4			rsno;
    OPC_TGPART		*tsoffsets = 
				subqry->ops_compile.opc_sortinfo.opc_tsoffsets;
    i4			dmf_mem_size;

    *pacount = ntargs;

    /* allocate space for DMF attr pointers, entries */
    dmf_mem_size = DB_ALIGN_MACRO(sizeof (DMF_ATTR_ENTRY *) * ntargs)
     + (sizeof (DMF_ATTR_ENTRY ) * ntargs);

    atts = ( DMF_ATTR_ENTRY **) opu_qsfmem(global, dmf_mem_size);
    att = (DMF_ATTR_ENTRY *) ME_ALIGN_MACRO((atts + ntargs), DB_ALIGN_SZ);

    for (resdom = target;
	    resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
	    resdom = resdom->pst_left
	)
    {
 	if (resdom->pst_right && (resdom->pst_right->pst_sym.pst_type == PST_SEQOP))
	{
            *pacount -= 1;              /* Bug 123652, One less attribute now present */
 	    continue;			/* skip sequences */
	}

	rsno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;

	STprintf(att->attr_name.db_att_name, "a%x", rsno);
	STmove(att->attr_name.db_att_name, ' ', 
		sizeof(att->attr_name.db_att_name), att->attr_name.db_att_name);
	att->attr_type = tsoffsets[rsno].opc_tdatatype;
	att->attr_size = tsoffsets[rsno].opc_tlength;
	att->attr_precision = tsoffsets[rsno].opc_tprec;
	att->attr_collID = tsoffsets[rsno].opc_tcollID;
	att->attr_flags_mask = 0;
	SET_CANON_DEF_ID(att->attr_defaultID, DB_DEF_NOT_DEFAULT);
	att->attr_defaultTuple = NULL;

        /* b91409 - PST_SORT nodes may need to be resolved due to DSQL vars */
        for(sort = global->ops_qheader->pst_stlist; sort != NULL;
                        sort = sort->pst_right)
        {
           if((sort->pst_sym.pst_dataval.db_datatype == DB_NODT) &&
              (sort->pst_sym.pst_value.pst_s_sort.pst_srvno == rsno))
           {
              sort->pst_sym.pst_dataval.db_datatype = att->attr_type;
              sort->pst_sym.pst_dataval.db_length = att->attr_size;
              sort->pst_sym.pst_dataval.db_prec = att->attr_precision;
              sort->pst_sym.pst_dataval.db_collID = att->attr_collID;
           }
        }

	rsno = tsoffsets[rsno].opc_sortno - 1;	/* sort order index, now */
	if (rsno < 0 || rsno >= ntargs)
	{
	    opx_error(E_OP0897_BAD_CONST_RESDOM);
	}
	atts[rsno] = att;
	++ att;
    }

    *patts = atts;
}

/*{
** Name: OPC_ESCATTS	- Build atts from eqcs to create for sort
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
**      20-oct-86 (eric)
**          written
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
**	7-Apr-2004 (schka24)
**	    Fill in attr default stuff in case anyone looks at it
**	2-Dec-2005 (kschendel)
**	    Do fewer get-memory calls.
[@history_template@]...
*/
static VOID
opc_escatts(
	OPS_STATE	*global,
	OPE_BMEQCLS	*seqcmp,
	OPC_EQ		*ceq,
	DMF_ATTR_ENTRY	***patts,    /* ptr to a ptr to an array of ptrs to atts */
	i4		*pacount )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DMF_ATTR_ENTRY	**atts;
    DMF_ATTR_ENTRY	*att;
    i4			attno;
    OPE_IEQCLS		eqcno;

    *pacount = BTcount((char *)seqcmp, (i4)subqry->ops_eclass.ope_ev);
    atts = (DMF_ATTR_ENTRY **) opu_qsfmem(global, 
			DB_ALIGN_MACRO((*pacount) * sizeof(DMF_ATTR_ENTRY *)) +
			(*pacount) * sizeof(DMF_ATTR_ENTRY) );
    att = (DMF_ATTR_ENTRY *) ME_ALIGN_MACRO( (atts + *pacount), DB_ALIGN_SZ);

    for (attno = 0, eqcno = -1;
	    (eqcno = BTnext((i4)eqcno, (char *)seqcmp, 
		(i4)subqry->ops_eclass.ope_ev)) != -1;
	    attno += 1
	)
    {
	STprintf(att->attr_name.db_att_name, "a%x", eqcno);
	STmove(att->attr_name.db_att_name, ' ', 
		sizeof(att->attr_name.db_att_name), att->attr_name.db_att_name);
	att->attr_type = ceq[eqcno].opc_eqcdv.db_datatype;
	att->attr_size = ceq[eqcno].opc_eqcdv.db_length;
	att->attr_precision = ceq[eqcno].opc_eqcdv.db_prec;
	att->attr_collID = ceq[eqcno].opc_eqcdv.db_collID;
	att->attr_flags_mask = 0;
	SET_CANON_DEF_ID(att->attr_defaultID, DB_DEF_NOT_DEFAULT);
	att->attr_defaultTuple = NULL;

	atts[attno] = att;
	++ att;
    }

    *patts = atts;
}

/*{
** Name: OPC_TSOATTS	- Build atts from a sort tree list 
**						    that gives the sort order.
**
** Description:
**	Build key-attribute list and compare list for a topsort.
**	Caller has prepared all the opc_tsoffsets stuff, and built the
**	list of columns being sorted.  (So we know types and offsets.)
**	We try to emit just key columns, if there's a given order-by
**	or key list.  For dup removal sorts, we have to include all
**	the other attributes too (in no particular order), no matter
**	what the order-by list stops at.  If this is a sort for a
**	query fragment (eg func attribute), just sort everything in
**	result-list order.
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
**      20-oct-86 (eric)
**          written
**      31-Oct-1989 (fred)
**          Added code to support unsortable attributes.  If an unsortable
**	    attribute is found in the sort list, then generate an error.
**	    If found as a "regular" attribute, then change the datatype to
**	    DB_DIFFERENT so that the sort comparer will find it easily and just
**	    return that this att is different than any other attribute.
**      5-jul-95 (inkdo01)
**          added null termination test to RESDOM for-loops (b64915)
**	1-nov-95 (inkdo01)
**	    Changed code to only sort on the "key" columns (ORDER BY, 
**	    GROUP BY, join key columns), rather than all columns.
**	24-july-03 (inkdo01)
**	    Fix to drop unsortable columns (e.g. BLOBs, UDTs) from key
**	    list of dups removal sort - bug 110618.
**	2-Dec-2005 (kschendel)
**	    Compute the compare-list here, why make qef do it?
**	14-Mar-2008 (kiria01) b119899
**	    Move PST_RS_DEFAULTED check into here as not all DEFAULTED
**	    will be at the head of the list now.
**	05-Aug-2008 (smeke01) b120433
**	    Do the same handling of non-sortable attributes in duplicate 
**	    removal in OPS_MAIN subquery as in other types of subquery.
**	5-Sep-2009 (kibro01) b120867
**	    Allocate extra space for sort columns even if NODUPLICATES is
**	    set since there's no guarantee that the sort columns actually
**	    come from the result column list.
**      09-Jun-2009 (coomi01) b121805,b117256
**          Supress SEQOP code generation.
[@history_template@]...
*/
static VOID
opc_tsoatts(
	OPS_STATE	*global,
	i4		ntargs,
	PST_QNODE	*target,
	PST_QNODE	*sortlist,
	DMT_KEY_ENTRY	***pkeys,    /* ptr to a ptr to an array of ptr's to keys */
	DB_CMP_LIST	**pcmplist,
	i4		*pkcount,
	i4		dups )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DB_CMP_LIST		*cmp;		/* Entry being built */
    DMT_KEY_ENTRY	**keys;
    DMT_KEY_ENTRY	*key;
    i4			keyno;
    i4			sattno;
    i4			size;
    PST_QNODE		*sort;
    PST_QNODE		*resdom;
    OPZ_BMATTS		targatts;
    OPC_TGPART		*tso;

    if (subqry->ops_sqtype == OPS_MAIN && sortlist == NULL)
    {
	/* EJLFIX: put error here; */
    }

    *pkcount = 0;   /* init */

    /* Allocate one pointer, one key entry, and one compare-list entry
    ** per sort key.  Everything is a sort key unless this is the main
    ** subquery with a specified sort-list and it's not a dups removal
    ** sort;  for that case, do a quick pre-count of keys.
    */
    /* Allocate the extra space for sort columns even for NODUPLICATES cases,
    ** because there is no guarantee that the sort columns come from the
    ** result columns themselves - e.g. a result column of varchar(x) but the
    ** query being sorted on x. (kibro01) b120867
    */

    if (subqry->ops_sqtype == OPS_MAIN && sortlist != NULL)
    {
	/* Start at 0 for normal sort, or add to the original result col
	** list count for NODUPLICATES case
	*/
	if (dups != DMR_NODUPLICATES)
	    ntargs = 0;
	for (sort = sortlist; sort != NULL; sort = sort->pst_right)
	    ++ ntargs;
    }
    size = DB_ALIGN_MACRO(ntargs * sizeof(DMT_KEY_ENTRY *))
	+ DB_ALIGN_MACRO(ntargs * sizeof(DMT_KEY_ENTRY))
	+ ntargs * sizeof(DB_CMP_LIST);

    keys = (DMT_KEY_ENTRY **) opu_qsfmem(global, size);
    key = (DMT_KEY_ENTRY *) ME_ALIGN_MACRO( (keys + ntargs), DB_ALIGN_SZ);
    cmp = (DB_CMP_LIST *) ME_ALIGN_MACRO( (key + ntargs), DB_ALIGN_SZ);
    *pkeys = keys;
    *pcmplist = cmp;

    if (subqry->ops_sqtype == OPS_MAIN && sortlist != NULL)
    {
	MEfill(sizeof (targatts), (u_char)0, (PTR)&targatts);

	for (keyno = 0, sort = sortlist;
		sort != NULL;
		sort = sort->pst_right
	    )
	{
	    if (opc_sortable(global, (DB_DT_ID)
			sort->pst_sym.pst_dataval.db_datatype) == FALSE)
	    {
		opx_error(E_OP0A97_OPCSORTTYPE);
	    }

	    if (sort->pst_right && sort->pst_right->pst_sym.pst_type == PST_SEQOP)
		continue;

	    sattno = sort->pst_sym.pst_value.pst_s_sort.pst_srvno;
	    BTset((i4)sattno, (char *)&targatts);
	
	    if (sort->pst_sym.pst_value.pst_s_sort.pst_srasc == TRUE)
	    {
		key->key_order = DMT_ASCENDING;
		cmp->cmp_direction = 0;
	    }
	    else
	    {
		key->key_order = DMT_DESCENDING;
		cmp->cmp_direction = 1;
	    }
	    STprintf(key->key_attr_name.db_att_name, "a%x", sattno);
	    STmove(key->key_attr_name.db_att_name, ' ', 
			sizeof(key->key_attr_name.db_att_name), 
			key->key_attr_name.db_att_name);
	    tso = &subqry->ops_compile.opc_sortinfo.opc_tsoffsets[sattno];
	    cmp->cmp_offset = tso->opc_toffset;
	    cmp->cmp_type = tso->opc_tdatatype;
	    cmp->cmp_length = tso->opc_tlength;
	    cmp->cmp_precision = tso->opc_tprec;
	    cmp->cmp_collID = tso->opc_tcollID;
	    keys[keyno] = key;
	    ++ keyno;
	    ++ key;
	    ++ cmp;
	    ++ (*pkcount);
	}

	if (dups == DMR_NODUPLICATES)   /* dup removal means add rest of cols */
  		for (resdom = target; 
		resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
		resdom = resdom->pst_left
	    )
	{
	    sattno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    if (BTtest((i4)sattno, (char *)&targatts) == FALSE)
	    {
		if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_DEFAULTED)
		    continue;		/* skip if not user specified */

		if (resdom->pst_right && resdom->pst_right->pst_sym.pst_type == PST_SEQOP)
		    continue;

		key->key_order = DMT_ASCENDING;
		STprintf(key->key_attr_name.db_att_name, "a%x", sattno);
		STmove(key->key_attr_name.db_att_name, ' ', 
				sizeof(key->key_attr_name.db_att_name), 
				key->key_attr_name.db_att_name);
		tso = &subqry->ops_compile.opc_sortinfo.opc_tsoffsets[sattno];
		cmp->cmp_direction = 0;
		cmp->cmp_offset = tso->opc_toffset;
		cmp->cmp_type = tso->opc_tdatatype;
		if (! opc_sortable(global, (DB_DT_ID) resdom->pst_sym.pst_dataval.db_datatype))
		{
		    /* Unsortable column, tell sorter to just return "different" */
		    cmp->cmp_type = DB_DIF_TYPE;
		    if (resdom->pst_sym.pst_dataval.db_datatype < 0) cmp->cmp_type = -DB_DIF_TYPE;
		}
		cmp->cmp_length = tso->opc_tlength;
		cmp->cmp_precision = tso->opc_tprec;
		cmp->cmp_collID = tso->opc_tcollID;

		keys[keyno] = key;
		++ key;
		++ cmp;
		keyno += 1; 
		(*pkcount)++;
	    }
	}
    }    /* sortlist style */
    else
    {
	/* else this is a top sort for a projection, function attribute, or
	** some other unknown type of subquery. In this situation, we just
	** sort in increasing resdom number. Ie. resdom 1 is the first key att,
	** resdom 2 is the next key att, etc.  Remember that the last
	** resdom is the rightmost, where we're starting.
	** Also, if eliminating duplicates, defaulted attributes should not
	** contribute so we count these and adjust as approprate 
	*/
	i4 defaulted = 0;

	for (resdom = target;
		resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
		resdom = resdom->pst_left
	    )
	{
	    if (resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_DEFAULTED ||
		(resdom->pst_right && resdom->pst_right->pst_sym.pst_type == PST_SEQOP))
		defaulted++;
	}
	keyno = ntargs - 1;		/* Zero origin indexes */
	cmp = cmp + ntargs - 1;		/* Work backwards from the end */
	if (dups == DMR_NODUPLICATES)
	{
	    /* Adjust the max keyno if we'll be skipping defaults */
	    keyno -= defaulted;
	    cmp -= defaulted;
	}
	for (resdom = target;
		resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
		resdom = resdom->pst_left
	    )
	{
	    if (dups == DMR_NODUPLICATES &&
	           resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_DEFAULTED)
		continue;

	    if (resdom->pst_right && resdom->pst_right->pst_sym.pst_type == PST_SEQOP)
		continue;

	    sattno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    key->key_order = DMT_ASCENDING;
	    STprintf(key->key_attr_name.db_att_name, "a%x", sattno);
	    STmove(key->key_attr_name.db_att_name, ' ', 
			sizeof(key->key_attr_name.db_att_name), 
			key->key_attr_name.db_att_name);
	    tso = &subqry->ops_compile.opc_sortinfo.opc_tsoffsets[sattno];
	    cmp->cmp_direction = 0;
	    cmp->cmp_offset = tso->opc_toffset;
	    cmp->cmp_type = tso->opc_tdatatype;
	    if (! opc_sortable(global, (DB_DT_ID) tso->opc_tdatatype))
	    {
		/* Unsortable column, tell sorter to just return "different" */
		cmp->cmp_type = DB_DIF_TYPE;
		if (tso->opc_tdatatype < 0) cmp->cmp_type = -DB_DIF_TYPE;
	    }
	    cmp->cmp_length = tso->opc_tlength;
	    cmp->cmp_precision = tso->opc_tprec;
	    cmp->cmp_collID = tso->opc_tcollID;

	    keys[keyno] = key;
	    ++ key;			/* "keys" handles backwardness */
	    -- cmp;			/* We're going backwards */
	    -- keyno;
	    ++ (*pkcount);
	}
    }
}

/*{
** Name: OPC_ESOATTS	- Build atts from eqc(s) that gives the sort order.
**
** Description:
**	Build key-attribute list and compare list for a sort or tsort
**	that's somewhere in the middle of a query plan.  The sort eqcs
**	is given by an input bit-map, and the keys are given by the usual
**	single/multi eqcno business.
**
**	We try to emit just key columns.  For dup removal sorts, we have
**	to include all the non-key attributes too (in no particular order).
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
**      20-oct-86 (eric)
**          written
**      01-nov-89 (fred)
**          Added check for unsortable attribute in sort key.  Added new
**	    parameter (ceq) to allow us to check attribute's datatype for
**	    sortability.
**	20-jun-90 (stec)
**	    Fixed a potential problem related to the fact that BTcount()
**	    in opc_jqual was counting bits in the whole bitmap. Number
**	    of bits counted is determined by ops_eclass.ope_ev in the
**	    subquery struct.
**	1-nov-95 (inkdo01)
**	    Changed code to only sort on the "key" columns (ORDER BY, 
**	    GROUP BY, join key columns), rather than all columns.
**	24-july-03 (inkdo01)
**	    Fix to drop unsortable columns (e.g. BLOBs, UDTs) from key
**	    list of dups removal sort - bug 110618.
**	 5-sep-03 (hayke02)
**	    Modify the above change so that we also decrement neqcs - this
**	    ensures that nordeqcs is correct and prevents SEGV in qes_skbuild()
**	    due to the attempt to access a NULL tsort_satts key when the
**	    tsort_scount (nordeqcs) is incorrect (too large). This change
**	    fixes bug 110849.
**	2-Dec-2005 (kschendel)
**	    Compute the compare-list here, why make qef do it?
[@history_template@]...
*/
static VOID
opc_esoatts(
	OPS_STATE	*global,
	OPE_BMEQCLS	*peqcmp,
	OPC_EQ		*ceq,
	OPE_IEQCLS	*ordeqcs,
	i4		*nordeqcs,
	DMT_KEY_ENTRY	***pkeys,  /* ptr to a ptr to an array of ptr's to keys */
	DB_CMP_LIST	**pcmplist,
	i4		dups)
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    DB_CMP_LIST		*cmp;		/* Entry being built */
    DMT_KEY_ENTRY	**keys;
    DMT_KEY_ENTRY	*key;
    i4			keyno;
    i4			neqcs;
    i4			size;
    i4			nkeyeqcs, nkeys;
    OPE_BMEQCLS		eqcmp;
    OPE_IEQCLS		eqcno;
    OPC_EQ		*ceq_entry;	/* ceq entry for key eqcls */

    MEcopy((char *)peqcmp, sizeof (eqcmp), (char *)&eqcmp);
    neqcs = BTcount((char *)&eqcmp, (i4)subqry->ops_eclass.ope_ev);

    /* if we are ordering on a multi-attribute sort definition, then use
    ** that for our ordering
    */
    nkeyeqcs = 1;
    if (ordeqcs[0] >= subqry->ops_eclass.ope_ev)
    {
	nkeyeqcs = neqcs;
	ordeqcs = &subqry->ops_msort.opo_base->
			opo_stable[ordeqcs[0] - subqry->ops_eclass.ope_ev]->
							opo_eqlist->opo_eqorder[0];
    }

    /* Assume one key-entry per eqclass.  If it's not a dups removal sort,
    ** prescan the eqcs to see how many there really are.
    ** Then allocate one pointer + one DMT_KEY_ENTRY + one DB_CMP_LIST
    ** for each key.  (can be somewhat over-estimated if blobs are being
    ** sorted, but that's rare.)
    */

    nkeys = neqcs;
    if (dups != DMR_NODUPLICATES)
    {
	nkeys = 0;
	for (keyno = 0; keyno < nkeyeqcs && ordeqcs[keyno] != OPE_NOEQCLS; 
		++keyno)
	    ++ nkeys;
    }

    size = DB_ALIGN_MACRO(nkeys * sizeof(DMT_KEY_ENTRY *))
	+ DB_ALIGN_MACRO(nkeys * sizeof(DMT_KEY_ENTRY))
	+ nkeys * sizeof(DB_CMP_LIST);

    keys = (DMT_KEY_ENTRY **) opu_qsfmem(global, size);
    key = (DMT_KEY_ENTRY *) ME_ALIGN_MACRO( (keys + nkeys), DB_ALIGN_SZ);
    cmp = (DB_CMP_LIST *) ME_ALIGN_MACRO( (key + nkeys), DB_ALIGN_SZ);
    *pkeys = keys;
    *pcmplist = cmp;

    /* first, add the key attributes in the key order as specified by
    ** ordeqcs.
    */
    for (keyno = 0; 
	    keyno < nkeyeqcs && ordeqcs[keyno] != OPE_NOEQCLS; 
	    keyno += 1)
    {
	eqcno = ordeqcs[keyno];
	BTclear((i4)eqcno, (char *)&eqcmp);

	ceq_entry = &ceq[eqcno];
	if (! opc_sortable(global, (DB_DT_ID) ceq_entry->opc_eqcdv.db_datatype))
	{
		opx_error(E_OP0A97_OPCSORTTYPE);
	}
	
	key->key_order = DMT_ASCENDING;
	STprintf(key->key_attr_name.db_att_name, "a%x", eqcno);
	STmove(key->key_attr_name.db_att_name, ' ', 
			sizeof(key->key_attr_name.db_att_name), 
					key->key_attr_name.db_att_name);
	cmp->cmp_direction = 0;
	cmp->cmp_offset = ceq_entry->opc_dshoffset;
	cmp->cmp_type = ceq_entry->opc_eqcdv.db_datatype;
	cmp->cmp_length = ceq_entry->opc_eqcdv.db_length;
	cmp->cmp_precision = ceq_entry->opc_eqcdv.db_prec;
	cmp->cmp_collID = ceq_entry->opc_eqcdv.db_collID;

	keys[keyno] = key;
	++ key;
	++ cmp;
    }

    /* Unless duplicates elimination was explicitly requested (implying 
    ** that the previous sortkey algorithm must be followed to avoid 
    ** subverting Ingres semantics), return now with sort key made up
    ** only of key columns (and not all the rest, because that would be 
    ** just plain silly). */

    if (dups != DMR_NODUPLICATES)
    {
	*nordeqcs = keyno;
	return;
    }

    /* Now that we've specified the attribute to sort on that matter, lets
    ** add the rest of the attributes, so that we don't lose duplicate
    ** in the bizarre way that DMF can eliminate them.
    */
    for (eqcno = -1; 
	    (eqcno = BTnext((i4)eqcno, (char *)&eqcmp, (i4)subqry->ops_eclass.ope_ev)) != -1;
	    keyno += 1
	)
    {
	ceq_entry = &ceq[eqcno];
	if (! opc_sortable(global, (DB_DT_ID) ceq_entry->opc_eqcdv.db_datatype))
	{
	    keyno -= 1;
	    continue;		/* skip unsortables */
	}

	key->key_order = DMT_ASCENDING;
	STprintf(key->key_attr_name.db_att_name, "a%x", eqcno);
	STmove(key->key_attr_name.db_att_name, ' ', 
			sizeof(key->key_attr_name.db_att_name), 
					key->key_attr_name.db_att_name);
	cmp->cmp_direction = 0;
	cmp->cmp_offset = ceq_entry->opc_dshoffset;
	cmp->cmp_type = ceq_entry->opc_eqcdv.db_datatype;
	cmp->cmp_length = ceq_entry->opc_eqcdv.db_length;
	cmp->cmp_precision = ceq_entry->opc_eqcdv.db_prec;
	cmp->cmp_collID = ceq_entry->opc_eqcdv.db_collID;

	keys[keyno] = key;
	++ key;
	++ cmp;
    }

    *nordeqcs = keyno;
}

/*{
** Name: OPC_TSTARGET	- Materialize a target list for a top sorts
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
**      11-feb-87 (eric)
**          written
**      10-nov-95 (inkdo01)
**	    Changes to support replacement of QEN_ADF instances by pointers.
[@history_template@]...
*/
static VOID
opc_tstarget(
	OPS_STATE   *global,
	OPC_NODE    *cnode,
	PST_QNODE   *root,
	QEN_ADF	    **qadf,
	i4	    *poutrow,
	i4	    *poutsize )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    i4			ninstr, nops, nconst;
    i4		szconst;
    i4			max_base;
    i4			outsize;
    i4			outrow;
    OPC_ADF		cadf;

    /* if there is nothing to compile, then return */
    if (root == NULL)
    {
	/* EJLFIX: error here */
	(*qadf) = (QEN_ADF *)NULL;
	return;
    }

    opc_tloffsets(global, root, &subqry->ops_compile.opc_sortinfo, TRUE);

    /* initialize an QEN_ADF struct; */
    ninstr = 0;
    nops = 0;
    nconst = 0;
    szconst = 0;
    opc_cxest(global, cnode, root, &ninstr, &nops, &nconst, &szconst);

    max_base = cnode->opc_below_rows + 2;
    opc_adalloc_begin(global, &cadf, qadf, 
			ninstr, nops, nconst, szconst, max_base, OPC_STACK_MEM);


    outrow = global->ops_cstate.opc_qp->qp_row_cnt;
    opc_adbase(global, &cadf, QEN_ROW, outrow);
    (*qadf)->qen_output = outrow;

    opc_tscresdom(global, cnode, root, &cadf, &outsize);

    *poutrow = outrow;
    *poutsize = outsize;
    opc_ptrow(global, &outrow, outsize);
    opc_adend(global, &cadf);
}

/*{
** Name: OPC_TLOFFSET	- Fill in target list offset information.
**
** Description:
**	Given a resdom parse-tree list, create the tsoffsets array
**	for those resdoms and fill it in, at least partly.
**	The resdom pointer in each array entry is always filled in.
**	If it's not a topsort we're doing this for some other
**	caller, and the data type, length, offset poop is filled in.
**	(opc_tscresdom() does it if it's a topsort.)
**
**	Non-printing resdoms are skipped and ignored.
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
**      11-apr-87 (eric)
**          written
**      29-Jun-1993 (fred)
**          Munged to correctly account for the size of peripheral objects.
**	18-dec-00 (inkdo01)
**	    Added init of opc_aggwoffset, opc_aggwlen fields of OPC_TGPART for
**	    inline aggregate code.
[@history_template@]...
*/
VOID
opc_tloffsets(
		OPS_STATE	*global,
		PST_QNODE	*root,
		OPC_TGINFO	*tginfo,
		i4		topsort)
{
    PST_QNODE		*resdom;
    i4			rsno;
    i4			tlsize;
    OPC_TGPART		*tsoffsets;
    i4			sortno;
    DB_STATUS           status;

    tginfo->opc_tlsize = 0;
    tginfo->opc_nresdom = 0;
    tginfo->opc_hiresdom = 0;
    for (resdom = root; 
	    resdom != NULL && resdom->pst_sym.pst_type != PST_TREE;
	    resdom = resdom->pst_left
	)
    {
	if (topsort == FALSE && 
		!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
	    )
	{
	    continue;
	}

	tginfo->opc_nresdom += 1;
	if (tginfo->opc_hiresdom < 
				resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno
	    )
	{
	    tginfo->opc_hiresdom =
				resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	}
    }
    tlsize = sizeof (*tginfo->opc_tsoffsets) * (tginfo->opc_hiresdom + 1);
    tginfo->opc_tsoffsets = tsoffsets = 
				    (OPC_TGPART *)opu_memory(global, tlsize);

    MEfill(tlsize, (u_char)0, (PTR)tsoffsets);


    for (resdom = root; 
	    resdom != NULL && resdom->pst_sym.pst_type != PST_TREE;
	    resdom = resdom->pst_left
	)
    {
	if (topsort == FALSE && 
		!(resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)

	    )
	{
	    continue;
	}

	rsno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	tsoffsets[rsno].opc_resdom = resdom;
    }

    if (topsort == FALSE)
    {
	/* If this isn't a topsort that we are doing this for, then fill
	** in the data type length offset, etc info for the target list.
	** If this is a topsort then don't bother because opc_tscresdom()
	** will do it.
	*/
	sortno = 1;
	for (rsno = 0;
		rsno <= tginfo->opc_hiresdom;
		rsno += 1
	    )
	{
	    resdom = tsoffsets[rsno].opc_resdom;
	    if (resdom == NULL)
            {
                continue;
            }
	    
	    tsoffsets[rsno].opc_sortno = sortno;
	    sortno += 1;

	    tsoffsets[rsno].opc_tdatatype = 
				resdom->pst_sym.pst_dataval.db_datatype;
	    tsoffsets[rsno].opc_tprec = 
				resdom->pst_sym.pst_dataval.db_prec;
	    tsoffsets[rsno].opc_tcollID = 
				resdom->pst_sym.pst_dataval.db_collID;
	    tsoffsets[rsno].opc_toffset = tginfo->opc_tlsize;
	    tsoffsets[rsno].opc_tlength = 
				resdom->pst_sym.pst_dataval.db_length;
	    tsoffsets[rsno].opc_aggwoffset = 0;
	    tsoffsets[rsno].opc_aggwlen = 0;
	    if (tsoffsets[rsno].opc_tlength == ADE_LEN_UNKNOWN)
	    {
		i4       bits;
		/*
		** This case can happen when there is a
		** target list which contains a large object.  In
		** this case, we verify that it is indeed a large object.
		** If so, then change the length to match that of the
		** source (sizeof(ADP_PERIPHERAL) [+ 1 for nullable]).
		** This will be the length of the
		** coupon used to carry around the large object.
		*/

		if ((status = adi_dtinfo(global->ops_adfcb,
					 tsoffsets[rsno].opc_tdatatype,
					 &bits))
		    != E_DB_OK)
		{
		    opx_verror(status, E_OP0783_ADI_FICOERCE,
			       global->ops_adfcb->adf_errcb.ad_errcode);
		}
		
		if (bits & AD_PERIPHERAL)
		{
		    tsoffsets[rsno].opc_tlength = sizeof(ADP_PERIPHERAL);
		    if (tsoffsets[rsno].opc_tdatatype < 0)
			tsoffsets[rsno].opc_tlength += 1;
		}
	    }
	    tginfo->opc_tlsize += tsoffsets[rsno].opc_tlength;
	}
    }
}

/*
** {
** Name: OPC_TSCRESDOM	- Compile a resdom for a top sort.
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
**      12-feb-87 (eric)
**          created
**	15-dec-89 (jrb)
**	    Added support for DB_ALL_TYPE in fi descs. (For outer join project.)
**	27-apr-90 (stec)
**	    Fix the problem related to corrupting database data (bug 21338)
**	    caused by generating MEcopy instructions for repeat query parms
**	    The right child of a given resdom in case of repeat update and 
**	    insert will have length set to -1 indicating unknown value,
**	    this must not be compiled into MEcopy; instead if opc_cqual comes
**	    up with datatype, length, precision different from the parent 
**	    resdom node, coercion will be done.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**      18-mar-92 (anitap)
**          Added opr_prec and opr_len initializationas as a fix for bug 42363.
**      29-Jun-1993 (fred)
**          Munged to correctly account for the size of peripheral objects.
**       7-Apr-1994 (fred)
**          Corrected above Mung (Bug 59662).  Previously, the code
**          was just propagating the length of the input thru.  This
**          didn't work in the cases where the input was being
**          coerced.  Consequenly, the code now correctly calls ADF to
**          deal with the case when a peripheral datatype of `unknown'
**          length moves thru here.
**	17-Aug-2006 (kibro01) Bug 116481
**	    Mark that opc_adtransform is not being called from pqmat
**      09-Jun-2009 (coomi01) b121805,b117256
**          Supress SEQOP code generation.
[@history_template@]...
*/
static VOID
opc_tscresdom(
	OPS_STATE	*global,
	OPC_NODE	*cnode,
	PST_QNODE	*root,
	OPC_ADF		*cadf,
	i4		*outsize )
{
    OPS_SUBQUERY	*subqry = global->ops_cstate.opc_subqry;
    PST_QNODE		*resdom;
    ADE_OPERAND		ops[2];
    i4			tidatt;
    i4			rsno;
    OPC_TGINFO		*tginfo = &subqry->ops_compile.opc_sortinfo;
    OPC_TGPART		*tsoffsets = tginfo->opc_tsoffsets;
    DB_DATA_VALUE	dv;
    i4			sortno;
    i4                  bits;
    DB_STATUS           status;

    *outsize = 0;

    dv.db_data = NULL;
    dv.db_datatype = 0;
    dv.db_length = 0;
    dv.db_prec = 0;
    dv.db_collID = -1;

    sortno = 1;
    for (rsno = 0;
	    rsno <= tginfo->opc_hiresdom;
	    rsno += 1
	)
    {
	resdom = tsoffsets[rsno].opc_resdom;

	/* Don't materialize sequence values in sort
	** - they will interfere with duplicates elimination.
        */
        if (resdom == NULL || (resdom->pst_right && (resdom->pst_right->pst_sym.pst_type == PST_SEQOP)))
        {
            continue;
        }

	if (resdom->pst_right->pst_sym.pst_type == PST_AOP)
	{
	    if (resdom->pst_right->pst_left == NULL)
	    {
		/* if this is an agg op that takes no input (count* for example)
		** then we need to create some data for the sorter. This is
		** because we are sorting a target list, and everyone
		** expects there to be data for this resdom, and it's not
		** polite to disappoint them
		*/
		opc_adempty(global, &dv, 
			resdom->pst_sym.pst_dataval.db_datatype,
			resdom->pst_sym.pst_dataval.db_prec,
			resdom->pst_sym.pst_dataval.db_length);
		opc_adconst(global, cadf, &dv, &ops[1], DB_QUEL, ADE_SMAIN);
	    }
	    else
	    {
		ops[1].opr_dt = resdom->pst_right->
		    pst_sym.pst_value.pst_s_op.pst_fdesc->adi_dt[0];

		if (ops[1].opr_dt == DB_ALL_TYPE)
		{
		    ops[1].opr_dt =
			resdom->pst_right->pst_left->
				    pst_sym.pst_dataval.db_datatype;
		}

		ops[1].opr_len = OPC_UNKNOWN_LEN;
                ops[1].opr_prec = OPC_UNKNOWN_PREC;
                ops[1].opr_collID = -1;

		opc_cqual(global, cnode, resdom->pst_right->pst_left, 
		    cadf, ADE_SMAIN, &ops[1], &tidatt);
	    }

	    ops[0].opr_dt   = ops[1].opr_dt;
	    ops[0].opr_len  = ops[1].opr_len;
	    ops[0].opr_prec = ops[1].opr_prec;
	    ops[0].opr_collID = ops[1].opr_collID;
	}
	else
	{
	    ops[1].opr_dt = resdom->pst_right->pst_sym.pst_dataval.db_datatype;
	    ops[1].opr_len = OPC_UNKNOWN_LEN;
            ops[1].opr_prec = OPC_UNKNOWN_PREC;
            ops[1].opr_collID = -1;
	    opc_cqual(global, cnode, resdom->pst_right, 
		cadf, ADE_SMAIN, &ops[1], &tidatt);

	    ops[0].opr_dt   = resdom->pst_sym.pst_dataval.db_datatype;
	    ops[0].opr_len  = resdom->pst_sym.pst_dataval.db_length;
	    ops[0].opr_prec = resdom->pst_sym.pst_dataval.db_prec;
	    ops[0].opr_collID = resdom->pst_sym.pst_dataval.db_collID;
	}

	if (ops[0].opr_len == ADE_LEN_UNKNOWN)
	{
	    /*
	    ** This case can happen when there is a top sort node
	    ** for a target list which contains a large object.  In
	    ** this case, we verify that it is indeed a large object.
	    ** If so, then change the length to match that of the
	    ** source (ops[1]...).  This will be the length of the
	    ** coupon used to carry around the large object.
	    ** opc_target() will later arrange for the redemption of
	    ** the large object itself.  We need to set the length
	    ** correctly to ensure that the coupon passes unchanged
	    ** thru the sorter.
	    ** 
	    ** Note that we can also arrive here with a coercion to
	    ** do.  Therefore, we should set the output length to that
	    ** of the appropriately sized peripheral object, not just
	    ** to the input length.  That is, we set the output length
	    ** to peripheral size, +1 if nullable.  (Bug 59662 - fred)
	    */
	    
	    if ((status = adi_dtinfo(global->ops_adfcb, ops[0].opr_dt, &bits))
		!= E_DB_OK)
	    {
		opx_verror(status, E_OP0783_ADI_FICOERCE,
			   global->ops_adfcb->adf_errcb.ad_errcode);
	    }
	    
	    if (bits & AD_PERIPHERAL)
	    {
		DB_DATA_VALUE	    idv;
		DB_DATA_VALUE	    odv;

		idv.db_datatype = odv.db_datatype = ops[0].opr_dt;
		idv.db_length = 0;
		idv.db_prec = odv.db_prec = ops[0].opr_prec;
		idv.db_collID = odv.db_collID = ops[0].opr_collID;
		idv.db_data = odv.db_data = NULL;

		status = adc_lenchk(global->ops_adfcb,
				    TRUE,
				    &idv,
				    &odv);

		if (DB_FAILURE_MACRO(status))
		{
		    opx_verror(status, E_OP0783_ADI_FICOERCE,
			       global->ops_adfcb->adf_errcb.ad_errcode);
		}
		ops[0].opr_len = odv.db_length;
	    }
	}
	
	ops[0].opr_base = cadf->opc_row_base[cadf->opc_qeadf->qen_output];
	ops[0].opr_offset = *outsize;

	/*
	** If (type and length of the resdom != type and length of the
	** attribute and prec of same for decimal datatype).
	*/
	if (ops[0].opr_dt   != ops[1].opr_dt	||
	    ops[0].opr_len  != ops[1].opr_len	||
	    /*
	    ** At this point opr_dt && opr_len are equal, so for
	    ** decimal datatype we want to check precision.
	    */
	    (abs(ops[0].opr_dt) == DB_DEC_TYPE && 
	     ops[0].opr_prec != ops[1].opr_prec)
	   )
	{
	    /* transformation is necessary */
	    opc_adtransform(global, cadf, &ops[1],
		&ops[0], ADE_SMAIN, (bool)FALSE, (bool)FALSE);
	}
	else
	{
	    /* a simple MEcopy is sufficient */
	    opc_adinstr(global, cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	}

	tsoffsets[rsno].opc_sortno = sortno;
	sortno += 1;

	rsno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	tginfo->opc_tsoffsets[rsno].opc_tdatatype = ops[0].opr_dt;
	tginfo->opc_tsoffsets[rsno].opc_tlength = ops[0].opr_len;
	tginfo->opc_tsoffsets[rsno].opc_tprec = ops[0].opr_prec;
	tginfo->opc_tsoffsets[rsno].opc_tcollID = ops[0].opr_collID;
	tginfo->opc_tsoffsets[rsno].opc_toffset = tginfo->opc_tlsize;

	tginfo->opc_tlsize += ops[0].opr_len;
	*outsize += ops[0].opr_len;
    }
}
/*{
** Name: OPC_TLQNATTS	- Fill in the qen_atts array from a qtree target list.
**
** Description:
**      This routine allocates and fills in a DMT_ATT_ENTRY array for the 
**      qen_atts field in a given QEN_NODE. In addition, the qen_natts field 
**      is also filled in. These fields are filled in based on the information 
**      in a query tree target list, and opc's target list info structure.
**        
**      This routine is usefull for top sort nodes only. This is because  
**      top sort nodes are the only QEN nodes that base their returning
**      attributes on the current queries target list. 
[@comment_line@]...
**
** Inputs:
**	global -
**	    This is the query wide state variable for OPF and OPC.
**	qn -
**	    This is the QEN node that is currently being built
**	tginfo -
**	    This is OPCs description of the target list for the current query.
**	top_resdom -
**	    This is the top qtree resdom.
**
** Outputs:
**	qn -
**	    The qen_atts and qen_natts will be filled in
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except those that opx_error raises
**
** Side Effects:
**	    none
**
** History:
**      10-may-89 (eric)
**          created from opc_qtqnatts() which was in OPCRAN.C. This was moved
**	    and renamed because only sort nodes use this routine. In the
**	    process, a bug was fixed where we were trying to fill in an
**          array element that didn't exist.
**      17-may-89 (eric)
**          added support for qen_prow in QEN_NODE structs.
**	16-jan-91 (stec)
**	    Added missing opr_prec initialization.
**	20-jul-92 (rickh)
**	    Pass adc_tmlen a pointer to an i2, not an i4.
**	06-apr-01 (inkdo01)
**	    Pass adc_tmlen a pointer to an i4, not an i2.
**      09-Jun-2009 (coomi01) b121805,b117256
**          Supress SEQOP code generation.
[@history_template@]...
*/
static VOID
opc_tlqnatts(
	OPS_STATE	*global,
	OPC_NODE	*cnode,
	QEN_NODE	*qn,
	OPC_TGINFO	*tginfo,
	PST_QNODE	*top_resdom )
{
	/* Standard vars */
    DB_STATUS	    status;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;

    i4			dmt_mem_size;
    char		*nextname;
    char		attbuf[sizeof(DB_ATT_NAME) + 1];
    i4			namelen;

	/* iatt points to the current DMT_ATT_ENTRY being built. */
    DMT_ATT_ENTRY	*iatt;
    DMT_ATT_ENTRY	*att;

	/* iattno holds the attribute number of the current attribute
	** in the virtual relation that this QEN node creates.
	*/
    i4			iattno;

	/* resno holds the resdom number for the current resdom */
    i4			rsno;

	/* resdom points to the current resdom */
    PST_QNODE		*resdom;

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

	/* ops is used to compile the qen_prow CX. Dv is used to desribe 
	** data that is in progress of being compiled.
	*/
    ADE_OPERAND		ops[2];
    DB_DATA_VALUE	dv;

	/* prowno is the result DSH row number for qen_prow, and prowsize is
	** the size for the current attribute that is being tmcvt'ed
	*/
    i4			prowno;

	/* Prow_worstcase holds the additional space required
	** for the worst case size of a data value. Prow_wctemp holds
	** intermediate values for the worst case calculation.
	*/
    i4			prow_worstcase;
    i4			prow_wctemp;
    i4			tempLength;

	/* op_virtbar and dv_virtbar describe the vertical bar seperators that
	** are used for qen_prow
	*/
    ADE_OPERAND		op_vertbar;
    DB_DATA_VALUE	dv_vertbar;


    qn->qen_natts = tginfo->opc_nresdom;

    /* allocate space for att pointers, entries and fake names like 'ra%d' */
    dmt_mem_size = DB_ALIGN_MACRO(sizeof (DMT_ATT_ENTRY *) * qn->qen_natts)
     + (sizeof (DMT_ATT_ENTRY ) * qn->qen_natts)
     + (32 * qn->qen_natts);

    qn->qen_atts = (DMT_ATT_ENTRY **) opu_qsfmem(global, dmt_mem_size);
    att = (DMT_ATT_ENTRY *) ME_ALIGN_MACRO((qn->qen_atts + qn->qen_natts), 
		DB_ALIGN_SZ);
    nextname = (char *)att + (sizeof(DMT_ATT_ENTRY) * qn->qen_natts);
    MEfill(dmt_mem_size, (u_char)0, (PTR)qn->qen_atts);

    /* begin the compilation of the qen_prow CX */
#ifdef OPT_F045_QENPROW
    if (opt_strace(global->ops_cb, OPT_F045_QENPROW) == TRUE)
    {
	/* estimate the size of the compilation; */
	ninstr = (2 * qn->qen_natts) + 1;
	nops = (2 * ninstr);
	nconst = 1;
	szconst = 1;
	max_base = 2;

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
	opc_adbase(global, &cadf, QEN_ROW, qn->qen_row);
	ops[0].opr_base = cadf.opc_row_base[prowno];
	ops[0].opr_offset = 0;

	/* add the vertical bar seperator to the CX */
	dv_vertbar.db_datatype = DB_CHA_TYPE;
	dv_vertbar.db_length = 1;
	dv_vertbar.db_prec = 0;
	dv_vertbar.db_collID = -1;
	dv_vertbar.db_data = "|";
	opc_adconst(global, &cadf, &dv_vertbar, &op_vertbar, DB_SQL, ADE_SMAIN);
    }
#endif

    /* For each resdom in the target list */
    for (resdom = top_resdom; 
	    resdom != NULL && resdom->pst_sym.pst_type == PST_RESDOM;
	    resdom = resdom->pst_left, att++)
    {
 	if (resdom->pst_right && resdom->pst_right->pst_sym.pst_type == PST_SEQOP)
 	    continue;			/* skip sequences */

	/* set some variables to keep line length down */
	rsno = resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	iattno = tginfo->opc_tsoffsets[rsno].opc_sortno;

	if (rsno < 0 || rsno > tginfo->opc_hiresdom || 
		iattno < 1 || iattno > tginfo->opc_nresdom
	    )
	{
	    opx_error(E_OP0899_BAD_TARGNO);
	}

	qn->qen_atts[iattno - 1] = iatt = att;

	/* The fake resdom name 'ra%d' */
	STprintf(attbuf, "ra%d", iattno);
	namelen = STlength(attbuf);
	iatt->att_nmstr = nextname;
	iatt->att_nmlen = namelen;
	cui_move(namelen, attbuf, '\0', namelen + 1, iatt->att_nmstr);
	nextname += (namelen + 1);

	iatt->att_number = iattno;
	iatt->att_offset = tginfo->opc_tsoffsets[rsno].opc_toffset;
	iatt->att_type = tginfo->opc_tsoffsets[rsno].opc_tdatatype;
	iatt->att_prec = tginfo->opc_tsoffsets[rsno].opc_tprec;
	iatt->att_collID = tginfo->opc_tsoffsets[rsno].opc_tcollID;
	iatt->att_width = tginfo->opc_tsoffsets[rsno].opc_tlength;
	iatt->att_flags = 0;
	iatt->att_key_seq_number = 0;
    }

#ifdef OPT_F045_QENPROW
    if (opt_strace(global->ops_cb, OPT_F045_QENPROW) == TRUE)
    {
	for (iattno = 0; iattno < qn->qen_natts; iattno += 1)
	{
	    iatt = qn->qen_atts[iattno];

	    /* first, add a veritical bar to the output row */
	    ops[0].opr_len = dv_vertbar.db_length;
	    ops[0].opr_prec = dv_vertbar.db_prec;
	    ops[0].opr_collID = dv_vertbar.db_collID;
	    STRUCT_ASSIGN_MACRO(op_vertbar, ops[1]);
	    opc_adinstr(global, &cadf, ADE_MECOPY, ADE_SMAIN, 2, ops, 1);
	    ops[0].opr_offset += dv_vertbar.db_length;

	    /* Now add the current attribute value to be converted into
	    ** a printable form.
	    */

	    /* Fill in the output operand (ops[0]) */
	    dv.db_datatype = iatt->att_type;
	    dv.db_prec = iatt->att_prec;
	    dv.db_length = iatt->att_width;
	    dv.db_collID = iatt->att_collID;
	    dv.db_data = NULL;

	    tempLength = ops[0].opr_len;
	    status = adc_tmlen(global->ops_adfcb, 
		&dv, &tempLength, &prow_wctemp);
	    ops[0].opr_len = tempLength;
	    if (status != E_DB_OK)
	    {
		opx_verror(status, E_OP0795_ADF_EXCEPTION,
		    global->ops_adfcb->adf_errcb.ad_errcode);
	    }

	    /* fill in the input operand (ops[1]) */
	    ops[1].opr_offset = iatt->att_offset;
	    ops[1].opr_dt = iatt->att_type;
	    ops[1].opr_prec = iatt->att_prec;
	    ops[1].opr_collID = iatt->att_collID;
	    ops[1].opr_len = iatt->att_width;
	    ops[1].opr_base = cadf.opc_row_base[qn->qen_row];

	    opc_adinstr(global, &cadf, ADE_TMCVT, ADE_SMAIN, 2, ops, 1);

	    ops[0].opr_offset += ops[0].opr_len;
	    if ((prow_wctemp - ops[0].opr_len) > prow_worstcase)
	    {
		prow_worstcase = (prow_wctemp - ops[0].opr_len);
	    }
	}

	/* add the final vertical bar to the output row */
	ops[0].opr_len = dv_vertbar.db_length;
	ops[0].opr_prec = dv_vertbar.db_prec;
	ops[0].opr_collID = dv_vertbar.db_collID;
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

/*{
** Name: opc_sortable	- Determine whether an attribute is sortable
**
** Description:
**      This routine determines whether a datatype is sortable.  It does so by
**	obtaining the datatype status bits from ADF, and looking at the
**	AD_NOSORT bit.  If set, then the attribute is not sortable, otherwise,
**	it is sortable. 
**
** Inputs:
**      global                          Opf center of session/invocation
**					information.
**      datatype                        Datatype about which information is
**					desired.
**
** Outputs:
**      None.                           No output -- returns boolean.
**
**	Returns:
**	    i4  -- used as true/false
**	Exceptions:
**	    Errors from the ADF routine (adi_dtstatus()) result in a call to
**	    opx_verror() -- which reports an error and EXsignal's out.
**
** Side Effects:
**	    none
**
** History:
**      31-Oct-1989 (fred)
**          Created for large object support.
[@history_template@]...
*/
static	i4
opc_sortable(
	OPS_STATE          *global,
	DB_DT_ID           datatype )
{
    i4                  dt_bits = 0;
    DB_STATUS           status;

    status = adi_dtinfo(global->ops_adfcb, datatype, &dt_bits);
    if (status != E_DB_OK)
    {
	opx_verror(status, E_OP0795_ADF_EXCEPTION,
			    global->ops_adfcb->adf_errcb.ad_errcode);
    }
    if (dt_bits & AD_NOSORT)
    {
	return(FALSE);
    }
    return(TRUE);
}
