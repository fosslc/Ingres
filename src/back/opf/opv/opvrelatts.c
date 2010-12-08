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
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
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

/* external routine declarations definitions */
#define             OPV_RELATTS         TRUE
#define             OPV_VIRTUAL_TABLE   TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPVRELATTS.C - create and initialize a joinop range variable
**
**  Description:
**      Routine to create and initialize a joinop range variable
**
**  History:
**      3-jul-86 (seputis)    
**          initial creation from relatts.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**    02-dec-98 (sarjo01)
**        Fix for bug 93767: in opv_relatts, initialize var_ptr->opv_itemp
**        to OPD_NOTEMP rather than 0 to prevent inadvertant use of
**        temp table vars in tree-to-text.
**      12-oct-2000 (stial01)
**          Init opv_kpleaf
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opv_virtual_table(
	OPS_SUBQUERY *subquery,
	OPV_VARS *lrvp,
	OPV_IGVARS grv);
static void opo_pco(
	OPS_SUBQUERY *subquery,
	OPV_VARS *varp,
	OPV_IVARS joinopvar,
	OPE_IEQCLS eqcls,
	bool init);
static void opv_rls(
	OPS_SUBQUERY *subquery,
	OPV_VARS *varp,
	OPV_IVARS varno,
	bool init);
OPV_IVARS opv_relatts(
	OPS_SUBQUERY *subquery,
	OPV_IGVARS grv,
	OPE_IEQCLS eqcls,
	OPV_IVARS primary);
void opv_tstats(
	OPS_SUBQUERY *subquery);

/*{
** Name: opv_virtual_table	- initialize virtual table structure
**
** Description:
**      Routine will allocate and initialize the virtual table descriptor
**      portion of the local range variable.
**
** Inputs:
**      subquery                        subquery descriptor
**      lrvp                            ptr to local range variable which
**                                      represents the virtual table
**
** Outputs:
**      lrvp->opv_subselect             structure allocated and initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-apr-87 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opv_virtual_table(
	OPS_SUBQUERY       *subquery,
	OPV_VARS           *lrvp,
	OPV_IGVARS          grv)
{
    OPV_SUBSELECT	    *subp; /* ptr to subselect descriptor
			    ** being created */
    /* add subselect structure to list associated with the local
    ** range variable and initialize */
    subquery->ops_joinop.opj_virtual = TRUE;
    subp = (OPV_SUBSELECT *)opu_memory(subquery->ops_global, 
	(i4)sizeof(OPV_SUBSELECT));
    subp->opv_nsubselect = lrvp->opv_subselect; /* add subselect to
				** list to be processed at this node */
    lrvp->opv_subselect = subp;
    subp->opv_sgvar = grv;	/* set global range variable associated
                                ** with virtual table */
    subp->opv_atno = OPZ_NOATTR;
    subp->opv_parm = NULL;	    /* add all corelated descriptors as
				** they are added to the joinop
				** attributes array */
    subp->opv_eqcrequired = NULL;
    subp->opv_order = OPE_NOEQCLS;
    MEfill( sizeof(OPE_BMEQCLS), (u_char)0, 
	(PTR)&subp->opv_eqcmp); /* init the equivalence class map */
}

/*{
** Name: OPO_PCO	- initialize CO node for joinop range variable
**
** Description:
**      This routine will initialize a CO node for a joinop range variable.
**      Each relation has one entry in the global range table, but the CO
**      node has references to equivalence classes, etc. which have
**      meaning only in the context of the local joinop range table.  Thus
**      a separate CO node is created and placed in the joinop range table
**      rather than the global range table.  Query compilation will receive
**      all the CO trees at the same time so these nodes cannot be reused.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varp                            ptr to joinop range var element to be
**                                      associated with the co node
**      joinopvarno                     local range variable number
**      eqcls                           ordering attribute of relation
**
** Outputs:
**      varp.opv_tcop			CO node initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-86 (seputis)
**          initial creation
**	26-dec-1990 (seputis)
**	    init CO node associated with variable only if necessary
**      18-sep-92 (ed)
**          - bug 44850 - changed mechanism for global to local mapping
**      06-mar-96 (nanpr01)
**          - removed extra line not needed. Initialize the pagesize field. 
**	14-jun-96 (inkdo01)
**	    Added Rtree support (handled same as Btree).
**	29-jul-2005 (toumi01)
**	    Sanity check opo_relprim to avoid division by zero when computing
**	    cost, leading to infinite cost and thus rejection of the query,
**	    and thus E_OP0491 or chosing another and perhaps worse plan.
**	30-Jun-2006 (kschendel)
**	    Take cost from global range variable when appropriate.
**	5-may-2008 (dougi)
**	    Flag table procedure CO-node.
*/
static VOID
opo_pco(
	OPS_SUBQUERY       *subquery,
	OPV_VARS	   *varp,
	OPV_IVARS          joinopvar,
	OPE_IEQCLS	   eqcls,
	bool		   init)
{
    OPO_CO              *cop;		    /* ptr to co node associated with
                                            ** relation */
    OPS_STATE		*global;

    global = subquery->ops_global;
    if (init)
    {
	varp->opv_tcop = cop = (OPO_CO *) opu_memory(global, (i4) sizeof(OPO_CO) );
					    /* allocate memory for the CO node
					    ** and save it in the global range
					    ** variable */
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{   /* allocate site array memory */
	    cop->opo_variant.opo_scost = (OPD_SCOST *) opu_memory( global, 
		sizeof( cop->opo_variant.opo_scost->opd_intcosts[0]) * 
		    global->ops_gdist.opd_dv);
	}

	opo_initco(subquery, cop);		/* use standard initialization routine
					    ** for CO node */
    }
    else
	cop = varp->opv_tcop;
    if (!varp->opv_grv->opv_relation)
    {
	/* if there is no RDF information then this is a temporary relation
        ** so the global range variable should have a CO node for the
        ** estimated size of the intermediate */
	if (varp->opv_grv->opv_gcost.opo_pagesize > 0)
	{
	    OPO_PARM	    *gcostp;
            OPV_GRT         *vbase;

            vbase = global->ops_rangetab.opv_base;
	    gcostp = &varp->opv_grv->opv_gcost;
	    if (varp->opv_grv->opv_created == OPS_FAGG)
	    {	/* get the correct tuple count for QUEL function aggregates 
		** which is the estimate for the "projection" OPS_PROJECTION 
		** rather than the estimate for the temporary , FIXME set
                ** up a pointer to the subquery associated with the global
                ** range variable */
		OPS_SUBQUERY	    *fsubquery;
		for (fsubquery = global->ops_subquery;
                    (fsubquery->ops_gentry < 0)
                    ||
                    vbase->opv_grv[fsubquery->ops_gentry] != varp->opv_grv;
		    fsubquery = fsubquery->ops_next)
		{   /* found subquery associated with the function aggregate */
		    ;
		}
		if (fsubquery->ops_projection != OPV_NOGVAR)
		    gcostp = &vbase->opv_grv[fsubquery->ops_projection ]
				->opv_gcost; /* get the pointer to the
					    ** descriptor for the projection
					    */
	    }
	    MEcopy( (PTR) gcostp, sizeof(OPO_PARM), (PTR)&cop->opo_cost);
	}
	else
	{   /* no global CO node exists, probably because it represented a
            ** subquery which did not contain any range variable , so use
            ** default values for the CO node */
	    cop->opo_cost.opo_cvar.opo_relprim = 1.0;
	    cop->opo_cost.opo_tups = 1.0;
	    cop->opo_cost.opo_reltotb = 1.0;
	    cop->opo_cost.opo_adfactor = OPO_DADFACTOR;
	    cop->opo_cost.opo_sortfact = OPO_DSORTFACT;
	    cop->opo_cost.opo_pagesize = DB_MINPAGESIZE;
	}
	cop->opo_storage = DB_HEAP_STORE; /* FIXME - can take advantage of
					** sortedness of bylist attributes to
					** avoid a sort node - need to create
					** an RDD_KEY_ARRAY key array */
    }
    else
    {
	DMT_TBL_ENTRY          *dmfptr;	    /* ptr to DMF table info */
	dmfptr = varp->opv_grv->opv_relation->rdr_rel; /* get ptr to RDF 
					** relation description info */
	cop->opo_storage = dmfptr->tbl_storage_type;
	cop->opo_cost.opo_reltotb = dmfptr->tbl_page_count;
	/*
	** if btree, set relprim to # of pages in relation so that we think 
	** there are no overflow pages instead of all overflow pages.  This 
	** is temporary fix until optimizer is smarter about btrees.
	*/
	if (dmfptr->tbl_storage_type == DB_BTRE_STORE ||
		dmfptr->tbl_storage_type == DB_RTRE_STORE)
	    cop->opo_cost.opo_cvar.opo_relprim = dmfptr->tbl_page_count - 1;
	else
	    cop->opo_cost.opo_cvar.opo_relprim = dmfptr->tbl_dpage_count
		+ dmfptr->tbl_ipage_count;  /* for heap, hash tbl_ipage_count is
					    ** zero, but for isam it is the number
					    ** of index pages in the initial creation
					    ** so for isam dpage == relmain and
					    ** ipage == (relprim-relmain) so that
					    ** dpage + ipage == relprim */
	if (cop->opo_cost.opo_cvar.opo_relprim <= 1.0)
	    cop->opo_cost.opo_cvar.opo_relprim = 1.0;
        if ((cop->opo_cost.opo_tups = dmfptr->tbl_record_count) <= 1.0)
	    cop->opo_cost.opo_tups = 1.0;
	cop->opo_cost.opo_adfactor = OPO_DADFACTOR;
	cop->opo_cost.opo_sortfact = OPO_DSORTFACT;
	cop->opo_cost.opo_pagesize = dmfptr->tbl_pgsize;
    }
    cop->opo_sjpr = DB_ORIG;

    if (varp->opv_mask & OPV_LTPROC)
    {
	DMT_TBL_ENTRY          *dmfptr;	    /* ptr to DMF table info */
	dmfptr = varp->opv_grv->opv_relation->rdr_rel; /* get ptr to RDF 
					** relation description info */
	cop->opo_storage = DB_TPROC_STORE;
	cop->opo_cost.opo_tups = dmfptr->tbl_record_count;
	cop->opo_cost.opo_reltotb = dmfptr->tbl_page_count;
    }

    cop->opo_cost.opo_pagestouched = OPO_NOBLOCKS;
    if (cop->opo_cost.opo_reltotb < 1.0)
	cop->opo_cost.opo_reltotb = 1.0;
    cop->opo_cost.opo_tpb 
	= cop->opo_cost.opo_tups / cop->opo_cost.opo_reltotb;
    if (cop->opo_cost.opo_tpb < 1.0)
	cop->opo_cost.opo_tpb = 1.0;
    if (init)
	cop->opo_ordeqc = eqcls;
    cop->opo_union.opo_orig = joinopvar;    /* if co node is defined then make
                                            ** make sure it references the
                                            ** correct local range variable
                                            */
    cop->opo_maps = &varp->opv_maps;	    /* equivalence classes
                                            ** needed will be at this location
                                            */
}

/*{
** Name: OPV_RLS	- initialize the RLS structure
**
** Description:
**      The OPN_RLS structure is the same one used to model 
**      intermediate relations in the join tree.  It is convenient to model
**      all relations this way.  This routine initializes the OPN_RLS structure
**      associated with the joinop range variable.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varp                            ptr to joinop range variable
**      varno                           joinop range variable number
**
** Outputs:
**      varp.opv_trl                    relation structure is initialized
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jun-86 (seputis)
**          initial creation
**	7-mar-96 (inkdo01)
**	    Initialize trl->opn_relsel (introduced to improve complete flag
**	    processing).
[@history_line@]...
*/
static VOID
opv_rls(
	OPS_SUBQUERY       *subquery,
	OPV_VARS           *varp,
	OPV_IVARS          varno,
	bool		   init)
{
    OPN_RLS             *trl;		/* descriptor associated with joinop
                                        ** range var */

    if (init)
    {
	varp->opv_trl = trl = (OPN_RLS *) opu_memory(subquery->ops_global, 
	    (i4)sizeof( OPN_RLS) );
	MEfill( sizeof(OPN_RLS), (u_char)0, (PTR)trl);
	BTset( (i4)varno, (char *)&trl->opn_relmap);
    }
    else
	trl = varp->opv_trl;
    if (varp->opv_grv->opv_relation)	/* check if this is a temporary */
	trl->opn_reltups 
	    = varp->opv_grv->opv_relation->rdr_rel->tbl_record_count; /* RDF
                                        ** information exists so get DMF
                                        ** table descriptor */
    else if (varp->opv_tcop)
	trl->opn_reltups = varp->opv_tcop->opo_cost.opo_tups; /* get
					** estimated tups from temporary
                                        ** relation */
    else
	trl->opn_reltups = 1.0;		/* if temporary relation has no range
                                        ** variables then it will return one
					** tuple */
    if (trl->opn_reltups < 1.0)
	trl->opn_reltups = 1.0;         /* to avoid division by zero */
    trl->opn_relsel = OPN_D100PC;
}

/*{
** Name: opv_relatts	- create and initialize a joinop range variable
**
** Description:
**      This procedure will create and initialize a joinop range variable
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      grv                             global range variable element to be
**                                      associated with joinop range variable
**      eqcls                           equivalence class of ordering attribute
**                                      for relation
**      primary                         range variable number of the primary
**                                      relation associated with this index
**                                      - OPV_NOVAR - if this is the primary
**
** Outputs:
**	Returns:
**	    joinop range variable number
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation from relatts
**	16-may-90 (seputis)
**	    b21483, b21485 fix estimate for ISAM primary pages
**	15-feb-94 (ed)
**          - bug 59598 - correct mechanism in which boolean factors are
**          evaluated at several join nodes
**	12-may-00 (inkdo01)
**	    Init opv_index.opv_histogram for composite histos. 
**	19-jan-04 (inkdo01)
**	    Init opv_pqbf for table partitioning.
**	7-apr-04 (inkdo01)
**	    Init opv_pcjdp for partitioning/parallel query processing.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
**	5-may-2008 (dougi)
**	    Set up table procedure local var.
*/
OPV_IVARS
opv_relatts(
	OPS_SUBQUERY       *subquery,
	OPV_IGVARS         grv,
	OPE_IEQCLS         eqcls,
	OPV_IVARS          primary)
{
    OPV_IVARS	       localrv;	    /* index into local range variable table
				    ** of next available slot
				    */
    OPS_STATE          *global;     /* ptr to global state variable */
    OPV_VARS           *var_ptr;    /* ptr to local range variable
				    ** element */

    localrv = subquery->ops_vars.opv_rv++; /* assign a new range variable */
    global = subquery->ops_global;  /* get ptr to global state variable */

    /* create "local" range variable elements for all global range
    ** variables referenced in the query
    */
    if (localrv >= OPV_MAXVAR)
	opx_error(E_OP0303_RTOVERFLOW); /* is max elements exceeded
				    ** - should only happen if too many
                                    ** function aggregates are defined since
                                    ** table is larger than parser table */
    subquery->ops_vars.opv_base->opv_rt[localrv] = var_ptr
	= (OPV_VARS *) opu_memory( global, (i4)sizeof(OPV_VARS)); /* assign
				    ** range variable element to joinop
				    ** range table slot */
    MEfill( sizeof(OPV_VARS), (u_char)0, (PTR)var_ptr); /* initialize
				    ** all fields */
    var_ptr->opv_grv 
	= global->ops_rangetab.opv_base->opv_grv[grv];/* save pointer to global
                                    ** range variable element */
    if (var_ptr->opv_grv->opv_gmask & OPV_ONE_REFERENCE)
	var_ptr->opv_grv->opv_gmask |= OPV_MULTI_REFERENCE;
    else
	var_ptr->opv_grv->opv_gmask |= OPV_ONE_REFERENCE;

    /* Check for table procedure. */
    if (var_ptr->opv_grv->opv_gmask & OPV_TPROC)
    {
	var_ptr->opv_mask |= OPV_LTPROC;
	var_ptr->opv_parmmap = (OPV_BMVARS *)opu_memory(global, 
						(i4) sizeof(OPV_BMVARS));
	MEfill(sizeof(OPV_BMVARS), (u_char)0, (PTR)var_ptr->opv_parmmap);
	subquery->ops_mask2 |= OPS_TPROC;
    }
    else var_ptr->opv_parmmap = (OPV_BMVARS *) NULL;

    var_ptr->opv_gvar = grv;        /* save global range variable number */
    global->ops_rangetab.opv_lrvbase[grv] = localrv; /* save joinop range variable
                                    ** number in global table element */
    var_ptr->opv_pqbf = (OPB_PQBF *) NULL;
    var_ptr->opv_pcjdp = (OPV_PCJDESC *) NULL;
    var_ptr->opv_itemp = OPD_NOTEMP;
    var_ptr->opv_kpb = 0.0;
    var_ptr->opv_kpleaf = 0.0;
    var_ptr->opv_leafcount = 0.0;
    var_ptr->opv_dircost = 0.0;
    var_ptr->opv_ojeqc = OPE_NOEQCLS; /* outer join NULL INDICATOR equivalence
                                    ** class */
    var_ptr->opv_ojattr = OPZ_NOATTR; /* outer join NULL INDICATOR attribute
                                    ** associated with the variable */
    var_ptr->opv_svojid = OPL_NOOUTER; /* init single variable inner join id */
    opo_pco(subquery, var_ptr, localrv, eqcls, TRUE); /* initialize the
				    ** CO node associated with this var */
    opv_rls(subquery, var_ptr, localrv, TRUE); /* initialize the rls structure */
    if (primary == OPV_NOVAR)
	var_ptr->opv_index.opv_poffset = localrv; /* this is probably not
				    ** used - FIXME */
    else
        var_ptr->opv_index.opv_poffset = primary; /* save primary relation */
    var_ptr->opv_index.opv_iindex = OPV_NOINDEX;  /* index into RDF array of
                                    ** ptrs to index tuples not defined yet */
    var_ptr->opv_index.opv_eqclass = eqcls;  /* ordering of index */
    var_ptr->opv_index.opv_histogram = (OPH_INTERVAL *) NULL;
    var_ptr->opv_primary.opv_tid = OPE_NOEQCLS; /* equivalence class not
                                    ** assigned yet */
    if (var_ptr->opv_grv->opv_relation)
    {
	var_ptr->opv_primary.opv_indexcnt = (var_ptr->opv_grv->opv_relation
	    ->rdr_rel->tbl_status_mask & DMT_IDXD) != 0; /* TRUE if table has
				    ** secondary indexes */
	if ((subquery->ops_global->ops_cb->ops_server->opg_qeftype == OPF_GQEF)
	    &&
	    (var_ptr->opv_grv->opv_relation->rdr_rel->tbl_status_mask & DMT_GATEWAY)
	    &&
	    var_ptr->opv_primary.opv_indexcnt)
	    subquery->ops_gateway.opg_smask |= OPG_INDEX; /* set the index
				    ** flag for this subquery since at least
				    ** one GATEWAY table with secondaries
				    ** is involved */
    }
/*  else
**	var_ptr->opv_primary.opv_indexcnt = FALSE; for temporary relation 
*/
    if (var_ptr->opv_grv->opv_gsubselect)
    {	/* This global range variable represents a virtual table which
        ** has a subquery structure associated with it, so initialize
        ** the local range variable virtual table descriptor */
        switch (var_ptr->opv_grv->opv_created)
	{
	case OPS_SELECT:
	case OPS_UNION:
	    break;		    /* these cannot be correlated */
	case OPS_VIEW:		    /* views are treated as virtual tables */
	{
	    OPS_SUBQUERY	*unionp; /* traverse all partitions in union */
	    subquery->ops_mask |= OPS_UVREFERENCE; /* mark subquery as
				    ** referencing a view for distributed OPC */ 
	    /* save ptr to parent subquery in each partition of the view
	    ** so that aggregate optimizations can be performed */
	    for (unionp = var_ptr->opv_grv->opv_gquery; unionp;
		unionp = unionp->ops_union)
		unionp->ops_agg.opa_father = subquery;
	    /* Fall through to do virtual table stuff. */
	}
	case OPS_HFAGG:
	case OPS_RFAGG:
	case OPS_RSAGG:
	{
	    opv_virtual_table(subquery, var_ptr, grv); /* init virtual table
				    ** descriptor */
	    break;
	}
#ifdef    E_OP0281_SUBQUERY
	default:
	    opx_error(E_OP0281_SUBQUERY);
#endif
	}
    }
    return( localrv );
}

/*{
** Name: opv_tstats	- temporary table statistics
**
** Description:
**      When union view optimization changed the flow of control, this routine
**      was created update the statistics on temporary tables given that the 
**      optimization is done on the second pass.  This routine will traverse
**	the local range table and update the statistics on all temporary tables
**
** Inputs:
**      subquery                        to be analyzed
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-jul-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opv_tstats(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS		    varno;
    OPV_RT		    *vbase;

    vbase = subquery->ops_vars.opv_base;
    for (varno=subquery->ops_vars.opv_rv; --varno>=0;)
    {
	OPV_VARS		*var_ptr;
	var_ptr = vbase->opv_rt[varno];
	if (var_ptr->opv_grv->opv_relation)
	    continue;			/* no new information if this
					** is not a temporary */
	opo_pco(subquery, var_ptr, varno, OPE_NOEQCLS, FALSE); /* initialize the
					** CO node associated with this var */
	opv_rls(subquery, var_ptr, varno, FALSE); /* initialize the rls structure */
	if (!var_ptr->opv_grv->opv_gsubselect)
	    var_ptr->opv_subselect = (OPV_SUBSELECT *)NULL;
    }
}
