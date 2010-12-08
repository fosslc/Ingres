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
#define             OPA_COMPAT          TRUE
#include    <me.h>
#include    <bt.h>
#include    <tr.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPACOMPAT.C - routines to check if two aggregates are compatible
**
**  Description:
**      This file contains all routines which check whether two aggregates
**	are compatible.
**
**  History:    
**	- 21-feb-85 (seputis)    
**	    initial creation
**	- fix bug 44850 - allow two global range variables to share the
**	    same descriptor.
**      8-sep-92 (ed)
**          removed OPF_DIST as part of sybil project
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          Remove the dependency on DB_MAXTUP for increased tuple size 
**	    project. Also use width type consistently. 
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
**	27-Oct-09 (smeke01) b122794
**	    Correct and improve OP147. Make all output go to the front-end. 
**	    Display addresses in hex. Correct #define name. 
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static bool opa_cvar(
	OPS_STATE *global,
	OPV_GBMVARS *vmap,
	PST_QNODE *treep);
static void opa_dups(
	OPS_SUBQUERY *header,
	OPS_SUBQUERY *subquery);
static OPS_SUBQUERY *opa_sameagg(
	OPS_SUBQUERY *header,
	PST_QNODE *newaop);
static bool opa_csimple(
	OPS_SUBQUERY *header,
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY **previous);
static void opa_substitute(
	OPS_SUBQUERY *subquery,
	OPV_IGVARS gvarno);
static bool opa_cfunction(
	OPS_SUBQUERY *header,
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY **previous);
static void opa_cselect(
	OPS_SUBQUERY *header,
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY **previous);
static void opa_igmap(
	OPS_STATE *global);
static bool opa_corrcheck(
	OPS_STATE *global,
	OPV_IGVARS lvar,
	OPV_IGVARS rvar);
static bool opa_cmaps(
	OPS_STATE *global,
	OPV_GBMVARS *lmap,
	OPV_GBMVARS *rmap);
static bool opa_together(
	OPS_SUBQUERY *header,
	OPS_SUBQUERY *subquery);
static void opa_findcompat(
	OPS_SUBQUERY *header);
void opa_agbylist(
	OPS_SUBQUERY *subquery,
	bool agview,
	bool project);
static void opa_coagview(
	OPS_SUBQUERY *subquery);
void opa_gsub(
	OPS_SUBQUERY *subquery);
void opa_compat(
	OPS_STATE *global);

/*{
** Name: opa_cvar	- convert var nodes
**
** Description:
**      This routine converts var nodes to the same table ID as the main
**	query of the concurrent list.
**
** Inputs:
**      global                          ptr to global state variable
**	vmap				from list which can be used to
**					convert variables which have not
**					been mapped
**	treep			        query tree to convert
**
** Outputs:
**
**	Returns:
**          TRUE - if there was at least one change
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-nov-90 (seputis)
**          initial creation to support common table ID detection
**	16-feb-93 (ed)
**	    fix bug 49505 - var bit map consistency check
**	27-oct-10 (smeke01) b124362
**	    This function should only convert var numbers for vars that are
**	    correlated vars and that appear in both queries being compared.
**	    The rest it should leave unchanged, and not raise an error.
[@history_template@]...
*/
static bool
opa_cvar(
	OPS_STATE          *global,
	OPV_GBMVARS	   *vmap,
	PST_QNODE          *treep)
{
    OPV_GRT	    *gbase;
    bool	    ret_val;
    gbase = global->ops_rangetab.opv_base;
    ret_val = FALSE;
    for (;treep; treep = treep->pst_left)
    {
	if (treep && (treep->pst_sym.pst_type == PST_VAR))
	{
	    OPV_IGVARS	    gvar;
	    OPV_GRV	    *gvarp;
	    i4		    vno;

	    vno = treep->pst_sym.pst_value.pst_s_var.pst_vno;
	    gvarp = gbase->opv_grv[vno];
	    gvar = gvarp->opv_compare;
	    if (gvar == OPV_NOGVAR)
	    {
		if (BTtest(vno, (char *)vmap) || gvarp->opv_same == OPV_NOGVAR)
		{
		    /*
		    ** This var is either the same in both queries, or it is
		    ** not a correlated var anyway.
		    */
		    gvar = vno;
		}
		else
		{
		    /*
		    ** The varno for this var is not in the master query, but
		    ** the var IS a correlated var. Check whether the var is
		    ** in the master query but with a different vno...
		    */
		    for (gvar = -1;
			(gvar = BTnext((i4)gvar, (char *)vmap, (i4)BITS_IN(*vmap)))>=0;)
		    {
			/* ..if it is, we use the vno from the master query...*/
			if (gbase->opv_grv[gvar]->opv_same == gvarp->opv_same)
			{
			    /*
			    ** Map the var from the candidate query to the
			    ** var used in the master query. This will make it
			    ** quicker to find if there are subsequent
			    ** occurences of this vno in the candidate query.
			    */
			    gbase->opv_grv[vno]->opv_compare = gvar;
			    break;
			}
		    }
		    /*
		    ** ...if it isn't, this is the same case as the var only
		    ** being in the second query, so keep the original vno.
		    */
		    if (gvar < 0)
		    {
			gvar = vno;
		    }
		}
	    }
	    /* Assign the result of our processing. */
	    treep->pst_sym.pst_value.pst_s_var.pst_vno = gvar;
	    ret_val = TRUE;
	}
	else if (treep->pst_right)
	    ret_val |= opa_cvar(global, vmap, treep->pst_right);
    }    
    return(ret_val);
}

/*{
** Name: opa_dups	- determine new duplicate handling for combined aggregates
**
** Description:
**      Duplicate handling may require a sort node were normally this would not 
**      be required if duplicates do not matter on a min,max or any, also TIDs
**	may need to be brought up whereas normally they may not be required
**
** Inputs:
**      header				ptr to header state structure
**      subquery->dups			ptr to duplicate handling of aggregate to
**					be combined
**
** Outputs:
**      header->ops_duplicates		new duplicate handling of combined aggregate
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-dec-88 (seputis)
**          initial creation
**	21-may-90 (seputis)
**          - fix incorrect use of define
**      12-apr-93 (ed)
**          - bug 50965 - incorrect duplicate handling when
**	    "don't care" and "keep dups" aggregates are combined
**	6-apr-94 (ed)
**	    - bug 58967 - apply duplicate handling in all cases
**	    since aggregate views can cause duplicate to be removed
**	3-dec-02 (inkdo01)
**	    Changes for range table extension.
[@history_template@]...
*/
static VOID
opa_dups(
	OPS_SUBQUERY	    *header,
	OPS_SUBQUERY	    *subquery)
{
    switch (subquery->ops_duplicates)
    {	
    case OPS_DREMOVE:
	header->ops_duplicates = OPS_DREMOVE; /* remove duplicates */
	break;	
    case OPS_DUNDEFINED:
	return;				/* just use original duplicate
					** handling */
    case OPS_DKEEP:
    default:
	header->ops_duplicates = OPS_DKEEP; /* keep duplicates
					** unless otherwise indicated */
	break;
    }
    if (!header->ops_agg.opa_tidactive
	&&
	subquery->ops_agg.opa_tidactive)
    {
	/* if tid handling is required then copy the tid info */
	header->ops_agg.opa_tidactive = TRUE;
	if (subquery->ops_global->ops_gmask & OPS_IDSAME) /* duplicate
					** handling requires translation
					** of relations */
	{
	    OPV_GBMVARS	    targetmap;	/* map of variables which should be
				    ** used,... change names to variable in
				    ** this list with same ID */
	    PST_QNODE	    varnode;
	    OPV_IVARS	    from_grv;
	    
	    /* there is a possibility that the ID's are the same
	    ** so traverse the keep tid map and translate to obtain
	    ** the correct relation map */
	    opv_submap(header, &targetmap);
	    MEfill(sizeof(OPV_GBMVARS), 0, 
			(char *)&header->ops_agg.opa_keeptids);
	    MEfill(sizeof(varnode), (u_char)0, (PTR)&varnode);
	    varnode.pst_sym.pst_type = PST_VAR; /* create VAR node type */
	    for (from_grv = -1;
		(from_grv = BTnext((i4)from_grv, 
		    (char *)&subquery->ops_agg.opa_keeptids,
		    (i4)BITS_IN(subquery->ops_agg.opa_keeptids)))>= 0;)
	    {
		varnode.pst_sym.pst_value.pst_s_var.pst_vno = from_grv;
		(VOID) opa_cvar(subquery->ops_global, &targetmap, &varnode);
		BTset((i4)varnode.pst_sym.pst_value.pst_s_var.pst_vno,
		    (char *)&header->ops_agg.opa_keeptids);
	    }
	}
	else
	{   /* subquery has duplicate handling but header query does not */
	    MEcopy((char *)&subquery->ops_agg.opa_keeptids, sizeof(OPV_GBMVARS),
		(char *)&header->ops_agg.opa_keeptids);
	}
    }
}

/*{
** Name: opa_sameagg	- check if aggregates are identical
**
** Description:
**	Test two aggregates to see if they are identical.  This information is
**	useful to avoid duplicate computations of aggregates.
**
** Inputs:
**      header                          ptr to subquery which will be
**                                      evaluated concurrently
**      new_aop                         aop node of aggregate to be combined
**
** Outputs:
**	Returns:
**	    - pointer to subquery which representing the identical aggregate 
**	    result 
**          - NULL if no result is identical
**	Exceptions:
**	    none
**
** Side Effects:
**	    If an identical aggregate result is found then the subquery and
**	    and underlying structures are discarded; i.e. they become
**          dangling pointers.
**
** History:
**	9-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
static OPS_SUBQUERY *
opa_sameagg(
	OPS_SUBQUERY       *header,	
	PST_QNODE          *newaop)
{
    OPS_SUBQUERY        *subquery; /* used to traverse list of subqueries to
                                ** be executed concurrently */
    OPS_STATE 		*global = header->ops_global;


    /* the aggregates to be computed together are connected in list using
    ** right branch of the aop query tree node
    */
    for (subquery = header;	    /* get first subquery which
				    ** will be executed concurrently
				    */
	subquery;		    /* exit if this is the last subquery */
	subquery = subquery->ops_agg.opa_concurrent) /* get next subquery 
				    ** in list */
    {
	PST_QNODE           *aop;   /* ptr to current aop node to be compared
				    ** against the new_aop node */
	aop = subquery->ops_agg.opa_aop;
	if (opv_ctrees(subquery->ops_global,
                       aop->pst_left, newaop->pst_left) /* if the 
                                    ** aggregate expressions are identical */
	    &&
		(aop->pst_sym.pst_value.pst_s_op.pst_opno 
                 ==
	         newaop->pst_sym.pst_value.pst_s_op.pst_opno /* and the
                                    ** aggregate operation is the same */
                )
           )
	{
# ifdef OPT_F019_AGGCOMPAT
	    if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
		TRformat(opt_scc, 
		    (i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		    (i4)sizeof(global->ops_trace.opt_trformat), 
		    "\nMatch found (0x%p) for aggregate op 0x%p\n", aop, newaop);
# endif
	    return (subquery);
	}
    }
# ifdef OPT_F019_AGGCOMPAT
	if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
		"\nNo match found for aggregate op 0x%p\n", newaop);
# endif
    return (NULL); /* no match */
}

/*{
** Name: opa_csimple	- combine simple aggregates
**
** Description:
**      This routine will combine two simple aggregrates which can be
**	run together, unless a system limit is reached.
**
** Inputs:
**      header			        subquery representing aggregate to
**                                      be executed
**      subquery                        subquery representing the aggregate
**					to be combined with the header
**					aggregate
**      previous                        back pointer to previous aggregate
**                                      which will be used to remove
**                                      the "subquery" from the list of
**                                      aggregates
**
** Outputs:
**	Returns:
**	    TRUE if aggregates where successfully combined
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-apr-86 (seputis)
**          initial creation from add_simple_agg
**	29-jan-91 (seputis)
**	    - fix b41904 - use complete target list for subquery when
**	    attempting to detect identical subqueries which have different
**	    range table numbers but similiar table IDs
**	22-june-01 (inkdo01)
**	    Changes to avoid clobbering pst_right of OLAP binary PST_AOP node.
[@history_line@]...
*/
static bool
opa_csimple(
	OPS_SUBQUERY       *header,
	OPS_SUBQUERY       *subquery,
	OPS_SUBQUERY       **previous)
{
    OPS_SUBQUERY	*same;	    /* points to a subquery which produces
				    ** an identical result if it exists
				    ** otherwise NULL
				    */
    OPS_STATE 		*global = header->ops_global;

    if ( same = opa_sameagg(header, subquery->ops_agg.opa_aop) )
    {
	/* Found identical aggregate so the same result can be substituted
	** in both cases.  The "substitution" will occur by using a ptr to the
	** same constant node.
	*/
	*subquery->ops_agg.opa_graft = *same->ops_agg.opa_graft;
	subquery->ops_agg.opa_father->ops_agg.opa_nestcnt--; /* since the
				    ** aggregate is not placed in the
				    ** concurrent list, the nesting count should
				    ** be decremented at this point */
        /* FIXME - memory is lost - the link to subquery is dangling */
# ifdef OPT_F019_AGGCOMPAT
	if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nSimple aggregates are IDENTICAL\n");
# endif
    }
    else
    {
	/* put this aggregate in the concurrent list with the others */
# ifdef OPT_F019_AGGCOMPAT
	if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT) )
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nSimple aggregates may be run together... add to aggregate list\n");
# endif
        {
	    OPS_WIDTH         temp_width;  /* new width of tuple in subquery */
	    /* first make sure it won't exceed tuple length */
	    temp_width = header->ops_width + 
		subquery->ops_agg.opa_aop->pst_sym.pst_dataval.db_length;
	    if (temp_width > subquery->ops_global->ops_cb->
		    ops_server->opg_maxtup)
		return(FALSE); /* tuple width exceeded so cannot add to list */
	    subquery->ops_width = temp_width;
	}
	*subquery->ops_agg.opa_graft = opv_constnode(subquery->ops_global, 
	    &subquery->ops_agg.opa_aop->pst_sym.pst_dataval,
	    (++subquery->ops_global->ops_parmtotal));	    /* this routine
					    ** also allocates a parameter const
                                            ** number */

	subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opparm =
	    (*subquery->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_cnst.pst_parm_no;
        subquery->ops_agg.opa_concurrent = header->ops_agg.opa_concurrent;
	header->ops_agg.opa_concurrent = subquery;

	if (subquery->ops_global->ops_gmask & OPS_IDSAME)
	{
	    OPV_GBMVARS	    targetmap;	/* map of variables which should be
				    ** used,... change names to variable in
				    ** this list with same ID */
	    /* there is a possibility that the ID's are the same
	    ** so traverse the expression and convert any var nodes
	    ** to the translated vars */
	    opv_submap(header, &targetmap);
	    if (opa_cvar(subquery->ops_global, &targetmap,
		subquery->ops_agg.opa_aop->pst_left))
		subquery->ops_vmflag = FALSE;
	}
	opa_dups(header, subquery); /* determine change in duplicate
				    ** handling if any */

        /* place aop node in list associated with the header subquery 
	** - the order does not matter since this "link" is used for "bitmaps"
        ** only, the concurrent list is used to build "resdom" nodes
	**
	** We don't do this anymore, because it clobbers the root addr
	** of the 2nd parm of a binary OLAP aggregate.
	*/
    }

    header->ops_vmflag = FALSE;	    /* there may be a new variable introduced
                                    ** in the target list */
    (*previous) = subquery->ops_next; /* remove subquery
				    ** from list of subqueries to be evaluated
				    ** independently
				    */
    return(TRUE);
}

/*{
** Name: opa_substitute	- replace aggregate/subselect by result node 
**
** Description:
**	Replace the aggregate/subselect represented by this subquery, by a 
**      result node in the outer aggregate (or main query).  Simple aggregates
**      will be replaced by a parameter constant node.  Function attributes
**      will be replaced by a var node representing the result attribute
**      of the aggregate expression in the temporary result relation.
**
** Inputs:
**      subquery                        ptr to aggregrate subquery being
**                                      processed
**
** Outputs:
**      subquery->ops_agg.opa_graft     replaced by a query tree node
**                                      representing the result
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-feb-86 (seputis)
**          initial creation from init_aggcomp
**	22-sep-89 (seputis)
**	    modified to pick up the resdom datatype from a subselect
**      23-aug-90 (seputis)
**          - fix b32738, opa_graft was changed by opa_optimize
**	4-jun-91 (seputis)
**	    - fix for b37443, find first printing resdom for subselect
**	22-june-01 (inkdo01)
**	    Changes to avoid clobbering pst_right of OLAP binary PST_AOP node.
**	22-mar-10 (smeke01) b123457
**	    opu_qtprint signature has changed as a result of trace point op214.
[@history_line@]...
*/
static VOID
opa_substitute(
	OPS_SUBQUERY       *subquery,   /* subquery to be processed */
	OPV_IGVARS	    gvarno)
{
    PST_QNODE             *result;      /* used to create a new sub tree to be
                                        ** grafted into the original query    
                                        ** to replace the AGHEAD node         
                                        */
    OPS_STATE             *global;      /* ptr to global state variable */
    DB_DATA_VALUE	  *datatypep;	/* ptr to data type of subsitututed
					** varnode */

    datatypep = &subquery->ops_agg.opa_aop->pst_sym.pst_dataval;
    global = subquery->ops_global;      /* get ptr to global state variable */
    switch (subquery->ops_sqtype)
    {

    case OPS_SAGG:
    {   
        /* create node for representing the result of the simple aggregate */
        result = opv_constnode(global, datatypep,
			    ++global->ops_parmtotal); /*
					** this routine will also allocate a
                                        ** parameter constant number */
	subquery->ops_dist.opd_dv_base = global->ops_parmtotal; /* this will be
					** the first constant allocated for
					** the subquery */
	subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opparm =
	    result->pst_sym.pst_value.pst_s_cnst.pst_parm_no; /* to avoid the
					** need for a PST_CONST attached to
					** pst_right of the AOP (which clobbers
					** 2nd operand of OLAP binary aggs) */
# ifdef OPT_F004_SIMPLE
        if ( opt_strace(global->ops_cb, OPT_AGSIMPLE) )
            TRdisplay("simple aggregate initialized\n");
# endif
	break;
    }
    case OPS_SELECT:
    {
	PST_QNODE   *printing;	    /* need to find first printing resdom */
	for (printing = subquery->ops_root->pst_left;
		printing 
		&& 
		(printing->pst_sym.pst_type == PST_RESDOM) 
		&& 
		(!(printing->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT));
	    printing = printing->pst_left)
	    ;			    /* find the first print resdom, in which their
					** should only be one for a subselect */
	datatypep = &printing->pst_sym.pst_dataval; /*
				    ** pick up the resdom result type since
				    ** implicit coercions can occur at this
				    ** point */
    }
	/* FALL THRU */
    case OPS_RSAGG:
	/* for corelated simple aggregates allocate a global range variable
        ** so that a subquery structure can be associated with the aggregate
        ** which gets evaluated every time the corelated variables are
	** available */
    case OPS_HFAGG:
    case OPS_RFAGG:
    case OPS_FAGG:
    {	/* process function aggregate / subselect */
	OPV_IGVARS	  variable; /* save range variable number to be
				    ** used to create var node for
				    ** substitution */
	DB_ATT_ID	  attribute;/* save attribute number to be used
				    ** to create var node for substitution */

	subquery->ops_gentry =
	variable = opv_agrv(global,  /* create global range variable for the
				    ** aggregate temporary result or subselect
                                    */
	    (DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL, /* NULL - implies a 
                                    ** temporary table should be created */
	    (DB_TAB_ID *)NULL,
			subquery->ops_sqtype, /* type of global range variable
				    */
			TRUE,	    /* TRUE - means abort if an error
				    ** occurs */
			(OPV_GBMVARS *)NULL,
			gvarno);

        attribute.db_att_id = ++subquery->ops_agg.opa_attlast; /* 
                                    ** attribute number assigned - 
                                    ** so that aggregate expressions 
				    ** or subselect result expressions which are
                                    ** are computed together will have
                                    ** distinct attribute numbers 
                                    */
	if (attribute.db_att_id >= DB_MAX_COLS) 
	    /* attribute numbered from 1 to DB_MAX_COLS
            ** FIXME - can make >= into a > */
	    opx_error( E_OP0201_ATTROVERFLOW );

	STRUCT_ASSIGN_MACRO(attribute, subquery->ops_agg.opa_atno); /* save original
				    ** attribute number - b32738 */
	result = opv_varnode(global, datatypep,
		     variable, &attribute); /* create a var node with the
				    ** appropriate attribute number
                                    ** - this var node will be referenced
                                    ** whenever this attribute number is
                                    ** is required; this attribute
                                    ** number will be used as the
                                    ** resdom number eventually */
	break;
    }
#ifdef E_OP0281_SUBQUERY
    default:
	opx_error(E_OP0281_SUBQUERY);	    /* unexpected subquery type */
#endif
    }
    /* Link result into tree... ops_agg.opa_graft holds the address
    ** &(pst_node->pst_left) or &(pst_node->pst_right), of the node in
    ** the outer aggregate which points to us
    */
    *(subquery->ops_agg.opa_graft) = result;
    subquery->ops_agg.opa_father->ops_vmflag = FALSE; /* indicate that
				    ** varmap needs to be updated for the 
                                    ** subquery */
# ifdef OPT_F011_PRINTAGG
    if ( opt_strace(global->ops_cb, OPT_AGSUB) )
    {
        TRdisplay("Aggregate we are processing is ...\n");
	opu_qtprint(global, subquery->ops_root, (char *)NULL);
	TRdisplay("looking for an identical aggregate ...\n");
    }
# endif
}

/*{
** Name: opa_cfunction	- combine function aggregates
**
** Description:
**      Combine the function aggregates unless a system limit has been
**      exceeded e.g. number of domains per relation.  The subquery will
**	be removed from the list of subqueries to be executed independently
**	and placed in the list of subqueries to be executed concurrently
**	with the header subquery.
**
** Inputs:
**      header                          subquery representing aggregate
**                                      which will be evaluated
**      subquery                        subquery representing the aggregate
**					to be combined with the current
**					header aggregate if possible
**      previous                        back pointer to previous aggregate
**                                      which will be used to remove
**                                      the "subquery" from the linked list
**                                      of header aggregates
**
** Outputs:
**	Returns:
**	    TRUE if aggregates where successfully combined
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the subquery from the list of subqueries
**	    which are executed independently
**
** History:
**	9-apr-86 (seputis)
**	    initial creation
**	15-may-89 (seputis)
**          make sure opv_created is in sync with ops_sqtype
**      16-feb-90 (seputis)
**	    fix b9597
**      23-aug-90 (seputis)
**          - fix b32738, opa_graft was changed by opa_optimize
**	29-jan-91 (seputis)
**	    - fix b41904 - use complete target list for subquery when
**	    attempting to detect identical subqueries which have different
**	    range table numbers but similiar table IDs
**	15-sep-92 (ed)
**	    - fix 44850 - separate range tables needed on a self join of
**	    a view calculating an aggregate
**	18-sep-92 (ed)
**	    - fix bug 44850 - allow two global range variables to share the
**	    same descriptor.
**	17-may-93 (ed)
**	    - bug 51879 - mark subquery which is evaluated together with
**	    another but still requires a new range variable
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	2-Jul-2008 (kibro01) b120572
**	    Check for any added RESDOM nodes ( for duplicate reduction) in the
**	    subquery being removed which aren't in the original, and if we find
**	    any copy them across.
**	22-Jan-2009 (kibro01) b121356
**	    If both queries to be combined are type RFAGG, they are relying on
**	    the lower results already being in order.  When combining them, 
**	    there is no tuple to expand, so you get a crash in opcorig, later.
**	    Leave the combined type as RFAGG.
[@history_line@]...
*/
static bool
opa_cfunction(
	OPS_SUBQUERY       *header,
	OPS_SUBQUERY       *subquery,
	OPS_SUBQUERY       **previous)
{
    OPS_SUBQUERY	*same;	    /* points to a subquery which produces
				    ** an identical result if it exists
				    ** otherwise NULL
				    */
    DB_ATT_ID           attribute;  /* attribute number of the aggregate
                                    ** expression of the aggregate function 
                                    ** represented by "subquery"
                                    ** - this attribute number is associated
                                    ** with the range variable of the combined
                                    ** subqueries
                                    */
    OPV_IGVARS		gvarno;	    /* global variable number of function
				    ** aggregate */
    OPS_STATE		*global;
    bool		newvar_needed;
    PST_QNODE		*resdom1, *resdom2;
    bool		resdom_needed;

    global = subquery->ops_global;
    newvar_needed = ((global->ops_gmask & OPS_IDCOMPARE) != 0)/* this will be true when
				    ** the range variables are not identical
				    ** between subqueries, so that the
				    ** underlying aggregate may need a
				    ** different range variable */
	    &&
	    (header->ops_agg.opa_father == subquery->ops_agg.opa_father)
	    &&
	    (	(header->ops_sqtype == OPS_FAGG)
		||
		(header->ops_sqtype == OPS_HFAGG)
		||
		(header->ops_sqtype == OPS_RFAGG)
	    );
				    /* since the same parent exists, there
				    ** will need to be separate range variables
				    ** on the temporary - bug 44850 */
    gvarno = (*header->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_var.pst_vno;
    
    if (newvar_needed)
    {
	/* create a separate range variable for the subquery which points
	** to the same global variable descriptor, so that the self join
	** of the aggregate is possible without evaluating the aggregate
	** twice */
	header->ops_sqtype = OPS_FAGG;	/* need to compute temporary */
	opa_substitute(subquery, gvarno);
	subquery->ops_mask |= OPS_AGGNEWVAR; /* new var created in list of
					** concurrent subqueries */
    }
    else
        subquery->ops_gentry = gvarno;   /* save global range variable associated
				    ** with the result */
    /* check if the subquery to be evaluated concurrently with 
    ** subquery->opa_subquery is identical to a subquery which exists 
    ** in the current list of subqueries to be evaluated together
    */
    if (same = opa_sameagg(header, subquery->ops_agg.opa_aop))
    {
	/* Found identical aggregate so the same result can be substituted
	** in both cases.  The "substitution" will occur by using the
	** same attribute number in the temporary relation.
	*/
	STRUCT_ASSIGN_MACRO((*same->ops_agg.opa_graft)
	    ->pst_sym.pst_value.pst_s_var.pst_atno, attribute);
# ifdef OPT_F019_AGGCOMPAT
	if (global->ops_cb->ops_check 
            && 
 	    opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nFunction aggregates are IDENTICAL\n");
# endif
        subquery->ops_agg.opa_redundant = TRUE; /* set flag indicating that
					** this aggregate is already being
                                        ** computed ... but the aggregate
                                        ** still needs to be linked into the
                                        ** outer aggregate, so it still needs
                                        ** to be put into the subquery list
                                        ** however a resdom node does not need
                                        ** to be created since it would be
                                        ** redundant */
    }
    else
    {   /* assign a new attribute number for the aggregate expression
        ** of the function attribute to be combined
        */
	OPS_WIDTH       temp_width;	/* new width of tuple in subquery */
	i4		temp_attribute; /* attribute for result */
	/* first make sure it won't exceed tuple length */
	temp_width = header->ops_width + 
	    subquery->ops_agg.opa_aop->pst_sym.pst_dataval.db_length;
	/* check that the number of attributes in the temporary relation
	** does not exceed the maximum
	*/
	temp_attribute = header->ops_agg.opa_attlast + 1; /* next available
					** attribute
					** in temporary for aggregate
					** function */
	if ((temp_width > subquery->ops_global->ops_cb->ops_server->opg_maxtup)
	    || (temp_attribute >= DB_MAX_COLS))
	    return(FALSE); /* tuple width exceeded so cannot add to list */
	header->ops_width = temp_width;
	attribute.db_att_id =
	header->ops_agg.opa_attlast = temp_attribute; /* save the number
					** of attributes defined so far */

# ifdef OPT_F019_AGGCOMPAT
	if (global->ops_cb->ops_check 
            && 
	    opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nAdd aggregate into tree\n");
# endif

	if (subquery->ops_global->ops_gmask & OPS_IDSAME)
	{
	    OPV_GBMVARS	    targetmap;	/* map of variables which should be
				    ** used,... change names to variable in
				    ** this list with same ID */
	    /* there is a possibility that the ID's are the same
	    ** so traverse the expression and convert any var nodes
	    ** to the translated vars */
	    opv_submap(header, &targetmap);
	    if (opa_cvar(subquery->ops_global, &targetmap,
		subquery->ops_agg.opa_aop->pst_left))
		subquery->ops_vmflag = FALSE;
	}
    }

    /* add subquery to list to be executed concurrently */
    subquery->ops_agg.opa_concurrent = header->ops_agg.opa_concurrent;
    header->ops_agg.opa_concurrent = subquery;
    opa_dups(header, subquery); /* determine change in duplicate
				    ** handling if any */
    header->ops_vmflag = FALSE;	    /* there may be a new variable introduced
                                    ** in the target list */
    STRUCT_ASSIGN_MACRO(attribute, subquery->ops_agg.opa_atno); /* save original
				    ** attribute number - b32738 */
    *subquery->ops_agg.opa_graft = opv_varnode(global, 
	&subquery->ops_agg.opa_aop->pst_sym.pst_dataval, /* datatype of
                                    ** aggregate result */
	subquery->ops_gentry,	    /* use the same global range variable 
				    ** number */
	&attribute);		    /* create a var node with the
				    ** appropriate attribute number
				    ** - this var node will be referenced
				    ** whenever this attribute number is
				    ** is required; this attribute
				    ** number will be used as the
				    ** resdom number eventually */

    subquery->ops_agg.opa_father->ops_vmflag = FALSE; /* outer aggregate 
				    ** needs to be remapped since a var node
				    ** substitution occurred */

    /* Search the subquery being removed for any RESDOM nodes with 
    ** ~PST_RS_PRINT, since they represent duplicate-reduction information
    ** and must be preserved (kibro01) b120572
    ** This can happen if the first subquery was a min() which didn't
    ** trigger the requirement for duplicate reduction, and the second was
    ** a count(), which did.
    */
    for (resdom1 = subquery->ops_root->pst_left;
	 resdom1 && resdom1->pst_sym.pst_type == PST_RESDOM;
	 resdom1 = resdom1->pst_left)
    {
	if (resdom1->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT)
	    continue;
	/* Found a non-printing RESDOM node - check if we know about it */
	resdom_needed = TRUE;
	for (resdom2 = header->ops_root->pst_left;
	     resdom2 && resdom2->pst_sym.pst_type == PST_RESDOM;
	     resdom2 = resdom2->pst_left)
	{
	    if (opv_ctrees(global, resdom1->pst_right, resdom2->pst_right))
	    {
		/* Found non-printing node already - don't need it again */
		resdom_needed = FALSE;
		break;
	    }
	}
	if (resdom_needed)
	{
	    PST_QNODE	*new_resdom;
	    new_resdom = opv_resdom(global, resdom1->pst_right,
			&resdom1->pst_sym.pst_dataval);
	    new_resdom->pst_left = header->ops_root->pst_left;
	    MEcopy(&resdom1->pst_sym.pst_value.pst_s_rsdm,
			sizeof(PST_RSDM_NODE),
		   &new_resdom->pst_sym.pst_value.pst_s_rsdm);
	    header->ops_root->pst_left = new_resdom;
	}
    }

    (*previous) = subquery->ops_next; /* remove subquery
				    ** from list of subqueries to be evaluated
				    ** independently
				    */
    if (header->ops_agg.opa_father != subquery->ops_agg.opa_father)
    {
	header->ops_agg.opa_multi_parent = TRUE;
	subquery->ops_agg.opa_multi_parent = TRUE;  /* TRUE if aggregates being
				    ** computed together have different
				    ** outer aggregates
				    */
    }
    {
	OPS_SQTYPE	sqtype;

	if (BTcount((char *)&header->ops_correlated, OPV_MAXVAR) != 0)
	{
	    sqtype = header->ops_sqtype; /* if correlated then subquery type
					** cannot change, i.e. cannot create
					** temp and need OPC to correlate */
	} else
	{
	    sqtype = OPS_FAGG;		/* temp can be created */
	    /* If neither query required a table for data, the resultant
	    ** table can remain without one (kibro01) b121356
	    */
	    if (header->ops_sqtype == OPS_RFAGG &&
		subquery->ops_sqtype == OPS_RFAGG)
			sqtype = OPS_RFAGG;
	}
	header->ops_sqtype = sqtype;  /* create temporary if used in more than
					** one place */
	subquery->ops_sqtype = sqtype;  /* create temporary if used in more than
					** one place */
					/* FIXME - can be more intelligent about
					** nested queries in that temporaries do
					** not need to be created if parent is
					** the same */
	global->ops_rangetab.opv_base->opv_grv[gvarno]->opv_created = sqtype;
					/* assign new creation type to
					** global range variable */
    }
    return(TRUE);
}

/*{
** Name: opa_cselect	- combine subselect references
**
** Description:
**      Combine the subselect references, since subselects will
**      not be compatible unless they are identical.
**
** Inputs:
**      header                          subquery representing subselect
**                                      which will be evaluated
**      subquery                        subquery representing the subselect
**					to be combinded with the current
**					header subselect if possible
**      previous                        back pointer to previous subselect
**                                      which will be used to remove
**                                      the "subquery" from the linked list
**                                      of header subselects
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the subquery from the list of subqueries
**	    which are executed independently
**
** History:
**	9-apr-86 (seputis)
**	    initial creation
**	22-sep-89 (seputis)
**	    modified to get datatype from resdom instead of expression
**      23-aug-90 (seputis)
**          - fix b32738, opa_graft was changed by opa_optimize
**	4-jun-91 (seputis)
**	    - fix for b37443, look for first printing resdom
**	27-jul-98 (hayke02)
**	    Add the subquery correlated gvars (ops_correlated) into
**	    the outer aggregate (father) subquery 'from' list bitmap
**	    (pst_tvrm). This change fixes bug 88252.
**	15-jan-01 (hayke02)
**	    Modify the fix for 88252 so that we only test for an ops_sqtype
**	    of OPS_RSAGG. This change fixes bug 103365.
**      26-feb-2004 (huazh01)
**          Modify the fix for b103365 so that 'ops_correlated' was added
**          to the 'from' list bitmap for the case of 'OPS_SELECT'. 
**          b111537, INGSRV2652.
{@history_line@}...
*/
static VOID
opa_cselect(
	OPS_SUBQUERY       *header,
	OPS_SUBQUERY       *subquery,
	OPS_SUBQUERY       **previous)
{
    /* check if the subquery to be evaluated concurrently with 
    ** subquery->opa_subquery is identical to a subquery which exists 
    ** in the current list of subqueries to be evaluated together
    */
    DB_ATT_ID           *attribute;  /* attribute number of the select */
    OPV_IGVARS		gvarno;
    PST_QNODE		*printing;  /* used to find first printing resdom */
    PST_QNODE		*root;
    OPS_STATE 		*global = header->ops_global;

# ifdef OPT_F019_AGGCOMPAT
    if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	TRformat(opt_scc, 
	    (i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
	    (i4)sizeof(global->ops_trace.opt_trformat), 
	    "\nSubselects are IDENTICAL\n");
# endif
    subquery->ops_agg.opa_redundant = TRUE; /* set flag indicating that
				    ** this subselect is already being
				    ** computed ... but the subselect
				    ** still needs to be linked into the
				    ** outer query to reduce the search
                                    ** space which is smaller if a cartesean
                                    ** product does not exist */

    /* add subquery to list to be executed concurrently */
    subquery->ops_agg.opa_concurrent = header->ops_agg.opa_concurrent;
    header->ops_agg.opa_concurrent = subquery;
    opa_dups(header, subquery); /* determine change in duplicate
				    ** handling if any */
    subquery->ops_gentry =	    /* save global range variable associated
				    ** with the result */
    gvarno = (*header->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_var.pst_vno;
    attribute = &(*header->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_var.pst_atno;
    STRUCT_ASSIGN_MACRO(*attribute, subquery->ops_agg.opa_atno); /* save original
				    ** attribute number - b32738 */
    for (printing = subquery->ops_root->pst_left;
	    printing 
	    && 
	    (printing->pst_sym.pst_type == PST_RESDOM) 
	    && 
	    (!(printing->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT));
	printing = printing->pst_left)
	;			    /* find the first print resdom, in which their
				    ** should only be one for a subselect */
    *subquery->ops_agg.opa_graft = opv_varnode(subquery->ops_global, 
	&printing->pst_sym.pst_dataval, /* datatype of
				    ** subselect result */
	gvarno,			    /* use the same global range variable 
				    ** number */
	attribute);		    /* create a var node with the
				    ** appropriate attribute number
				    ** - this var node will be referenced
				    ** whenever this attribute number is
				    ** is required */
    subquery->ops_agg.opa_father->ops_vmflag = FALSE; /* outer aggregate 
				    ** needs to be remapped since a var node
				    ** substitution occurred */
    (*previous) = subquery->ops_next; /* remove subquery
				    ** from list of subqueries to be evaluated
				    ** independently
				    */
    if (header->ops_agg.opa_father != subquery->ops_agg.opa_father)
    {
	header->ops_agg.opa_multi_parent = TRUE;
	subquery->ops_agg.opa_multi_parent = TRUE;  /* TRUE if subselects whicha
                                    ** are identical have different parents
				    */
    }

    if ((subquery->ops_agg.opa_father->ops_sqtype == OPS_RSAGG) ||
        ((subquery->ops_agg.opa_father->ops_sqtype == OPS_SELECT) &&
         (header->ops_agg.opa_father) &&
         (header->ops_agg.opa_father->ops_sqtype == OPS_MAIN)))
    {
	root = subquery->ops_agg.opa_father->ops_root;
	BTor(OPV_MAXVAR, (char *)&subquery->ops_correlated,
		(char *)&root->pst_sym.pst_value.pst_s_root.pst_tvrm);
    }
}

/*{
** Name: opa_igmap	- init global map
**
** Description:
**      This routine resets all variable numbers used for comparison of 2 trees 
**
** Inputs:
**      global                          ptr to global range variable
**
** Outputs:
**
**	Returns:
**	    TRUE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-nov-90 (seputis)
**          initial creation
**	16-aug-91 (seputis)
**	    changed function to VOID since boolean was not used
*/
static VOID
opa_igmap(global)
OPS_STATE          *global;
{
    OPV_GRV	    **grvpp;
    OPV_GRT	    *gbase;

    gbase = global->ops_rangetab.opv_base;
    for (grvpp = &gbase->opv_grv[0]; grvpp != &gbase->opv_grv[OPV_MAXVAR]; grvpp++)
	if (*grvpp)
	    (*grvpp)->opv_compare = OPV_NOGVAR;	/* mark all variables as available */
}



/*
** Name: opa_corrcheck - compare correlation names for range values
**
** Description:
**	This routine compares the correlation names of two tables to
**	determine if they are compatible with each other (for aggregate      
**	matching) 
**
** Inputs:
**	global			ptr to global state descriptor
**	lvar			first global range var to be compared 
**	rvar			first global range var to be compared 
**
** Outputs:
**	TRUE if correlation name is the same for both variables
**
** Side effects:
**	none
**
** History:
**	21-jul-00 (wanfr01)
**		created
*/

static bool
opa_corrcheck(
	OPS_STATE          *global,
        OPV_IGVARS 		   lvar,
        OPV_IGVARS 		   rvar)
{
   OPV_GRT	    *gbase = global->ops_rangetab.opv_base;

   short l_qrv = gbase->opv_grv[lvar]->opv_qrt;
   short r_qrv = gbase->opv_grv[rvar]->opv_qrt;
   short r_namelen,l_namelen;

   if ((l_qrv < 0) &&
       (r_qrv < 0))
      return(TRUE); 
   else if (!((r_qrv >= 0) && 
  	     (l_qrv >= 0)))
      return (FALSE);


   r_namelen = opt_noblanks( 
	sizeof(global->ops_qheader->pst_rangetab[r_qrv]->pst_corr_name), 
      (char *)&global->ops_qheader->pst_rangetab[r_qrv]->pst_corr_name);

   l_namelen = opt_noblanks( 
	sizeof(global->ops_qheader->pst_rangetab[l_qrv]->pst_corr_name), 
      (char *)&global->ops_qheader->pst_rangetab[l_qrv]->pst_corr_name);

   return( (r_namelen == l_namelen) && 
           !MEcmp(
              &global->ops_qheader->pst_rangetab[r_qrv]->pst_corr_name,  
              &global->ops_qheader->pst_rangetab[l_qrv]->pst_corr_name,
              l_namelen)
         );
}

/*{
** Name: opa_cmaps	- compare range variable maps
**
** Description:
**      This routine compares range variable maps taking into account
**	that some table ID's may be the same but have different range
**	variables 
**
** Inputs:
**      global                          ptr to global state descriptor
**      lmap                            ptr to global variable bitmap
**      rmap                            ptr to global variable bitmap
**
** Outputs:
**
**	Returns:
**	    TRUE - if the variable bitmaps are logically equal
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-nov-90 (seputis)
**          initial creation
**	21-jul-00 (wanfr01)
**	    bug 102154, INGSRV 1232
**	    added opa_corrcheck to compare correlation names
[@history_template@]...
*/

static bool
opa_cmaps(
	OPS_STATE          *global,
	OPV_GBMVARS        *lmap,
	OPV_GBMVARS        *rmap)
{
    OPV_IGVARS	    lvar;
    OPV_GRT	    *gbase;

    if (BTcount((char *)lmap, (i4)BITS_IN(*lmap))
	!=
	BTcount((char *)rmap, (i4)BITS_IN(*rmap)))
	return(FALSE);			/* must have the same number
					** of variables */
    gbase = global->ops_rangetab.opv_base;
    opa_igmap(global);

    for (lvar = -1; 
	(lvar = BTnext((i4)lvar, (char *)lmap, (i4)BITS_IN(*lmap)))>= 0;
	)
    {
	OPV_IGVARS  rvar;
	if (BTtest((i4)lvar, (char *)rmap))
	{
	    if (gbase->opv_grv[lvar]->opv_compare == OPV_NOGVAR)
	    {
		gbase->opv_grv[lvar]->opv_compare = lvar;
		continue;		/* mark variable as being found in
					** the other map */
	    }
	}
	for (rvar = -1; 
	    (rvar = BTnext((i4)rvar, (char *)rmap, (i4)BITS_IN(*rmap)))>= 0;
	    )
	{
	    if ((gbase->opv_grv[rvar]->opv_compare == OPV_NOGVAR)
		&&
		(   gbase->opv_grv[rvar]->opv_same 
		    ==
		    gbase->opv_grv[lvar]->opv_same
		)
		&&
		(gbase->opv_grv[rvar]->opv_same != OPV_NOGVAR)
		&&
		opa_corrcheck(global,lvar,rvar)
	    )
	    {
		gbase->opv_grv[rvar]->opv_compare = lvar;
		break;			/* mark variable as being found in
					** the other map */
	    }
	}
	if (rvar < 0)
	    return(FALSE);		/* maps are not equivalent */
    }
    return(TRUE);
}

/*{
** Name: opa_together	- check aggregate subqueries for compatibility
**
** Description:
**      This routine will check all the conditions required to determine
**	if two aggregate subqueries are compatible i.e. can be executed 
**      together. 
**
** Inputs:
**      header                          ptr to subquery representing aggregates
**                                      which are being computed together
**      subquery                        ptr to aggregate which will be
**                                      checked for compatibility with
**                                      aggregates in header
**
** Outputs:
**	Returns:
**	    TRUE if aggregates can be combined
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-apr-86 (seputis)
**          initial creation from comb_simple_agg, comb_agg_func, checkagg
**	12-apr-89 (seputis)
**          include from list in var list comparison for count(*)
**	16-jan-91 (seputis)
**	    remove subselect test since OPC/QEF can deal with common hold files
**	18-mar-91 (seputis)
**	    - using common hold files in SELECTs require that target
**	    lists match until OPC can handle multi-attribute SEjoin
**	    target lists (FIXME)
**	28-mar-92 (seputis)
**	    - fix b43267 - make sure subquery expressions are converted
**	    to header expressions when detecting and mapping table IDs
**	8-sep-92 (ed)
**	    -fix b44801, do not remap table elements which are in common
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**      30-mar-03 (wanfr01)
**          Bug 108353, INGSRV 2201
**          Added OPF_NOAGGCOMBINE to allow reoptimizing the query without
**          attempting to combine aggregates.
**      01-jul-05 (gnagn01)
**	    Profound check if distinct and non distinct aggregate are to be
**	    executed together.This fixes the bug 114008.
**	18-Oct-2005 (schka24)
**	    Above didn't compile, and was improperly indented.  Fix.
**	1-Apr-2009 (kibro01) b121869
**	    Confirm that subqueries to be joined have the same distinctness
**	    and, if eliminating distincts from a count query, have the same
**	    column(s) in the resdom list.
**      10-sept-2010 (huazh01)
**          if 'header' and 'subquery' are two idential sub-select queries
**          but under different outer join id, then they are not compatible
**          to be run together. (b123772)
[@history_line@]...
*/
static bool
opa_together(
	OPS_SUBQUERY       *header,
	OPS_SUBQUERY       *subquery)
{
    bool	    subselect;	    /* TRUE if subqueries being tested are
                                    ** subselects */
    OPS_STATE	    *global;
    OPS_SUBQUERY    *header_test = header;
    bool	    equal_fromlist; /* TRUE if the from lists between the
				    ** two queries are equal */
# ifdef OPT_F019_AGGCOMPAT
    bool	    trace;  /* TRUE if tracing desired */
# endif

    global = subquery->ops_global;

# ifdef OPT_F019_AGGCOMPAT
    trace = opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT );
# endif
    /* Only include aggregates that do not still require a nested
    ** aggregate to be analyzed.  Otherwise, the aggregate will be
    ** using an uncomputed (ie garbage) value.
    ** When the nested aggregate has been analyzed, the nest count
    ** will be decremented.
    ** FIXME- is this needed, and does it make sense for subselects
    */
    if (subquery->ops_agg.opa_nestcnt > 0)
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	        "\nCandidate contains nested aggregates -- so exclude it.\n");
# endif
	return(FALSE);
    }

    /* make sure both subqueries are simple aggregates
    ** or both are function aggregates, or both are subselects
    */
    if ((header->ops_sqtype != subquery->ops_sqtype)
	&&
	(  !(header->ops_sqtype == OPS_FAGG /* header may have already been
					    ** changed to OPS_FAGG if it was
					    ** combined with another subquery
					    */
	&&
	(subquery->ops_sqtype == OPS_RFAGG || subquery->ops_sqtype == OPS_HFAGG))
	)
	&&
	( ! ((header->ops_sqtype == OPS_RFAGG || header->ops_sqtype == OPS_HFAGG)
	&&
	subquery->ops_sqtype == OPS_FAGG)
	)
       )
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCannot combine a simple and function aggregate\n");
# endif
	return(FALSE); 
    }
    
    /* check that the corelation variables are the same */
    if (MEcmp((PTR)&header->ops_correlated,
	      (PTR)&subquery->ops_correlated,
	      sizeof(OPV_GBMVARS)
             )
#if 0
Correlation variables need to match exactly or some other way is needed to check
for this type of query... this may lose some deeply nested optimizations.

select r2.relid from iirelation r1, iirelation r2 where 
	    r1.relid < (select min(relid) from iirelation where r1.reltups = iirelation.reltups)
	and r1.relid > (select min(relid) from iirelation where r2.reltups = iirelation.reltups)\p\g
note that r1 and r2 are not equivalent.
        &&
	(   !(global->ops_gmask & OPS_IDSAME)
	    ||
	    !opa_cmaps(global, &header->ops_correlated, &subquery->ops_correlated)
	)
#endif
       )
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCannot combine - corelation variables differ \n");
# endif
	return(FALSE); 
    }

    /* check that the FROM list is the same */
    equal_fromlist = !MEcmp((PTR)&header->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
              (PTR)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
              sizeof(OPV_GBMVARS));
    if (!equal_fromlist
        &&
	(   !(global->ops_gmask & OPS_IDSAME)
	    ||
	    !opa_cmaps(global, 
	      (OPV_GBMVARS *)&header->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	      (OPV_GBMVARS *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm)
	)
       )
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCannot combine - range variables do not match \n");
# endif
	return(FALSE); 
    }


    /* FIXME need to test for compatible unions eventually */
    if (header->ops_union || subquery->ops_union)
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCannot combine - unions \n");
# endif
	return(FALSE); 
    }

    subselect = header->ops_sqtype == OPS_SELECT;   /* is this a subselect */

    /* check that the duplicate handling is the same */
    if (subselect
	&&
	(   header->ops_root->pst_sym.pst_value.pst_s_root.pst_dups 
	    != 
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_dups
	)
       )
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCannot combine - duplicate handling does not match \n");
# endif
	return(FALSE); 
    }

    if ((global->ops_gmask & OPS_IDSAME)
	&&
	!equal_fromlist)
    {
	OPV_IGVARS	gvno;
	OPV_GBMVARS	fixedmap;

	MEcopy((PTR)&header->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
	    sizeof(fixedmap), (PTR)&fixedmap);
	BTand((i4)(BITS_IN(fixedmap)), 
	    (char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
	    (char *)&fixedmap);		/* if there are any table elements in
					** common, then they should not be 
					** remapped, bug 44801 */
	BTor((i4)(BITS_IN(fixedmap)), (char *)&subquery->ops_correlated,
	    (char *)&fixedmap);
	opa_igmap(global);		/* init map so that different range
					** variables with the same table ID
					** can be found to be compatible */
	for (gvno = -1; (gvno = BTnext((i4)gvno, (char *)&fixedmap,
	    sizeof(subquery->ops_correlated))) >= 0;)
	{   /* mark all correlated values as being mapped to itself, since
	    ** a mechanism does not exist to match table IDs of correlated values */
	    global->ops_rangetab.opv_base->opv_grv[gvno]->opv_compare = gvno;
	}
        if (global->ops_caller_cb->opf_smask & OPF_NOAGGCOMBINE)
           return(FALSE); 
        else
	   global->ops_gmask |= OPS_IDCOMPARE;
    }

   
/*
**      two aggregate functions can be run together
**      according to the following table:
**
**			prime	!prime	don't care
**
**		 prime	afcn?	never	afcn?	
**		!prime	never	always	always
**	    don't care	afcn?	always	always
**
**	don't care includes: ANY, MIN, MAX
**	afcn? means only if aggregate-function's are identical
*/
    if (!subselect)
    {
	bool	    ok;			/* TRUE if aggregates can be run 
                                        ** together */
	ok = !opa_prime(header) && !opa_prime(subquery); /* run together
                                        ** if neither is prime */
        /* FIXME - why does opv_ctrees need to be called ?? */
	if (!ok 
	    && 
	    opv_ctrees(header->ops_global, header->ops_agg.opa_aop->pst_left, 
		       subquery->ops_agg.opa_aop->pst_left)
	   )
	    /* run together if same AGG_EXPR and prime or don't care */
	    ok =    (header->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.
			pst_distinct == PST_DISTINCT
		    ||
		    header->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.
			pst_distinct == PST_DONTCARE
		    )
		&&
		    (subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.
			pst_distinct == PST_DISTINCT
		    ||
		    subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.
			pst_distinct == PST_DONTCARE
		    );			/* TRUE if prime or don't care */


/*Profound check if distinct and non distinct aggregates are together*/

        if(subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.

                                    pst_distinct == PST_DISTINCT)
	{

	    while (header_test != NULL)
	    {
		if (header_test->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_distinct == PST_NDISTINCT)
		{
		    ok = FALSE;
		    break;
		}
		header_test = header_test->ops_agg.opa_concurrent;
	    }
	}
	if (!ok)
	{
# ifdef OPT_F019_AGGCOMPAT
	    if (trace)
		TRformat(opt_scc, 
		    (i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		    (i4)sizeof(global->ops_trace.opt_trformat), 
		    "\nAggregates incompatible due to PRIME/NONPRIME rules\n");
# endif
	    return(FALSE); /* aggregates not compatible so continue */
	}

	if (header->ops_root->pst_sym.pst_value.pst_s_root.pst_project
	    !=
	    subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_project)
	    return(FALSE);  /* must have same projection semantics */
    }


    {   
	OPV_GBMVARS	lmap;
	OPV_GBMVARS	rmap;
	opv_smap(subquery);		    /* map the subquery */
	opv_smap(header);                   /* map the subquery */

	MEcopy((char *)&header->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm,
		sizeof(lmap), (char *)&lmap);
	BTor(OPV_MAXVAR, (char *)&header->ops_root->pst_sym.pst_value.
		pst_s_root.pst_tvrm, (char *)&lmap);
	BTor(OPV_MAXVAR, (char *)&header->ops_root->pst_sym.pst_value.
		pst_s_root.pst_rvrm, (char *)&lmap);
	MEcopy((char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm,
		sizeof(rmap), (char *)&rmap);
	BTor(OPV_MAXVAR, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_tvrm, (char *)&rmap);
	BTor(OPV_MAXVAR, (char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_rvrm, (char *)&rmap);
	/* The two aggregates must range over the same variables */
	if (MEcmp((char *)&lmap, (char *)&rmap, sizeof(lmap)) 
	    &&
	    (	!(global->ops_gmask & OPS_IDSAME)
		||
		!opa_cmaps(global, &lmap, &rmap)
	    ))
	{
	    /* note that "select count (*), min(r.a) from r" would not be
	    ** evaluated together unless pst_tvrm is included
	    ** since count(*) would not have any
	    ** variables defined in the agg expression */
	    
# ifdef OPT_F019_AGGCOMPAT
	    if (trace)
		TRformat(opt_scc, 
		    (i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		    (i4)sizeof(global->ops_trace.opt_trformat), 
		    "\nAggregates do not range over same variables, so exclude it.\n");
# endif
	    return(FALSE);
	}

	/* check the qualifications. They must be identical */
	if (!opv_ctrees(header->ops_global, header->ops_root->pst_right, 
			subquery->ops_root->pst_right)
	   )
	{
# ifdef OPT_F019_AGGCOMPAT
	    if (trace)
		TRformat(opt_scc, 
		    (i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		    (i4)sizeof(global->ops_trace.opt_trformat), 
		    "\nAggregate qualification lists differ, so exclude it.\n");
# endif
	    return(FALSE);
	}
    }

    /* if this is a function aggregate then bylists exists so
    ** check for same by_list 
    */
    if ((   (header->ops_sqtype == OPS_FAGG)
	    ||
	    (header->ops_sqtype == OPS_HFAGG)
	    ||
	    (header->ops_sqtype == OPS_RFAGG)
	)
	&& 
        !opv_ctrees(header->ops_global, header->ops_agg.opa_byhead->pst_left, 
		    subquery->ops_agg.opa_byhead->pst_left)
       )
    {   
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCandidate by lists differ, so exclude it.\n");
# endif
	return(FALSE);
	    
    }
    else if ((	subselect
		||
		BTcount((char *)&header->ops_correlated, OPV_MAXVAR) != 0)
				/* correlation requires
				** that target list items match */
	&&
        !opv_ctrees(header->ops_global, header->ops_root->pst_left,
		    subquery->ops_root->pst_left)
       )
    {
# ifdef OPT_F019_AGGCOMPAT
	if (trace)
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCandidate subselect target lists differ, so exclude it.\n");
# endif
	return(FALSE);
    }

    /* If this is a straightforward COUNT, we need to ensure that the duplicate
    ** elimination is the same (kibro01) b121869
    */
    if (header->ops_agg.opa_aop && subquery->ops_agg.opa_aop &&
	header->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opno == 
            header->ops_global->ops_cb->ops_server->opg_scount &&
	subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opno == 
            header->ops_global->ops_cb->ops_server->opg_scount)
    {
	PST_QNODE *h, *s;
	/* One is count(distinct) and the other just count - can't combine */
	if (header->ops_duplicates != subquery->ops_duplicates)
		return (FALSE);
	/* Even if we can combine, we need to check if this is count(distinct
	** that we're counting the same thing - otherwise it will still give
	** the wrong answer
	*/
	h = header->ops_root->pst_left;
	s = subquery->ops_root->pst_left;
	while (h && s)
	{
	    if (s->pst_sym.pst_type == PST_RESDOM && h->pst_sym.pst_type == PST_RESDOM)
	    {
		if (!s->pst_right || !h->pst_right)
		{
		    return (FALSE);
		}
		if (s->pst_right->pst_sym.pst_type == PST_VAR ||
		    h->pst_right->pst_sym.pst_type == PST_VAR)
		{
		    if (s->pst_right->pst_sym.pst_type == PST_VAR &&
		        h->pst_right->pst_sym.pst_type == PST_VAR)
		    {
			if (s->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
			    h->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
				return (FALSE);
		    } else
		    {
			return (FALSE);
		    }
		}
	    }
	    s = s->pst_left;
	    h = h->pst_left;
	}
	if (h || s)
		return (FALSE);
    }

    /* b123772:
    ** if two sub-select queries are identical but under different 
    ** oj id, then they are not compatible to be run together. 
    ** 'opl_aggid' gets set in opa_rnode().
    */
    if (header->ops_sqtype == OPS_SELECT &&
        subquery->ops_sqtype == OPS_SELECT &&
        header->ops_oj.opl_aggid != subquery->ops_oj.opl_aggid)
       return (FALSE);

    return(TRUE); /* return TRUE if aggregates are compatible */

}

/*{
** Name: opa_findcompat	- find queries which can be run with current
**
** Description:
**	Look through all the remaining aggregates for those
**	that can be run with the current aggregate. When we find 
**	such an aggregate, take it off of the remaining subquery list,
**	so we don't try to process it twice.
**
** Inputs:
**      header			local aggregate processing state
**                                      variable
**
** Outputs:
**      header->ops_agg.opa_concurrent  list of aggregates to be processed
**                                      concurrently
**      header->ops_agg.opa_aop       list of aop nodes representing the
**                                      aggregate expressions
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    This list of subqueries may be reduced, when certain aggregates
**          which can be computed together are combined.
**
** History:
**	8-apr-86 (seputis)
**          initial creation
**	27-mar-92 (seputis)
**	    -b43339 - compare only, do not translate during scanning of
**	    target lists
**	18-feb-93 (ed)
**	    -b49505 - initialize common ID optimization array
**	13-nov-01 (hayke02)
**	    We now skip the removal of a duplicate subselect - call to
**	    opa_cselect() - if the query has had a 'not exists'/'not in'
**	    subselect transformed into an outer join (ops_smask &
**	    OPS_SUBSELOJ). This change fixes bug 106354.
**      01-jul-2005 (gnagn01)
**          Backing out fix for 106354 as call to opa_cselect 
**          is crucial for complex nested queries
**          (Union select with not in/not exists and aggregates).
**          Bug 106354 appears to be fixed even without the backed 
**          out fix.This change fixes the bug 114043. 
**	22-mar-10 (smeke01) b123457
**	    opu_qtprint signature has changed as a result of trace point op214.
[@history_line@]...
*/
static VOID opa_findcompat(
	OPS_SUBQUERY         *header)
{
    OPS_SUBQUERY        **previous;     /* used to remove a subquery from
                                        ** the linked list
                                        */
    OPS_STATE		*global;

    global = header->ops_global;
    if (global->ops_gmask & OPS_IDSAME)
	opa_igmap(global);		/* fix bug 49505, initialize common
					** ID optimization array */
    for ( previous = &header->ops_next; (*previous)->ops_sqtype != OPS_MAIN;)
    {
	/* traverse the subquery list until the main query is reached */
# ifdef OPT_F019_AGGCOMPAT
	if (opt_strace( global->ops_cb, OPT_F019_AGGCOMPAT ) )
	{
	    TRformat(opt_scc, 
		(i4 *)global, (char *)&global->ops_trace.opt_trformat[0], 
		(i4)sizeof(global->ops_trace.opt_trformat), 
	    	"\nCandidate identical aggregate is ...\n\n");
	    opu_qtprint(global, (*previous)->ops_root, (char *)NULL);
	}
# endif
	global->ops_gmask &= (~(OPS_IDCOMPARE|OPS_CIDONLY));/* reset flag which may have
						** been set inside opa_together */

	if( opa_together( header, *previous ))
	{
	    /* aggregates or subselects are compatible so combine them */
	    global->ops_gmask |= OPS_CIDONLY;	/* b43339 - compare only, do
						** not translate common IDs */
	    switch( header->ops_sqtype )
	    {
	    case OPS_SAGG:
	    {	/* combine the simple aggregate subquery */
		if ( opa_csimple(header, *previous, previous) )
		    continue;			/* subquery was removed from
                                                ** list successfully */
		break;
	    }
	    case OPS_HFAGG:
	    case OPS_RFAGG:
	    case OPS_RSAGG: /* FIXME - it may be worthwhile to create a
			    ** temporary for OPS_RFAGG, OPS_RSAGG if it is
			    ** used in more than one spot */
	    case OPS_FAGG:
	    {	/* combine the function aggregate subquery */
		if (opa_cfunction(header, *previous, previous))
		    continue;                   /* subquery was removed from
                                                ** list successful */
		break;
	    }
	    case OPS_SELECT:
            { 
		opa_cselect(header, *previous, previous);
		continue;
	    }
#ifdef E_OP0281_SUBQUERY
	    default:
		opx_error(E_OP0281_SUBQUERY);	/* invalid subquery type */
#endif
	    }
	}
	previous = &(*previous)->ops_next;	/* keep current subquery in the
                                                ** list and get next subquery */
    }
    global->ops_gmask &= (~(OPS_IDCOMPARE|OPS_CIDONLY)); /* reset flag which may have
						** been set inside opa_together */
}

/*{
** Name: opa_agbylist	- place agheads results into by list
**
** Description:
**      This routine was created to place aghead results which are 
**      correlated into a by list, normally VAR nodes are correlated 
**      but a user can specify an aggregate in a view and correlated 
**      on this attribute 
**
** Inputs:
**      subquery                        ptr to subquery with correlated
**					aghead
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-may-89 (seputis)
**          initial creation
**      12-jun-90 (seputis)
**          fix shape query causing OPF scope error
**      26-sep-91 (seputis)
**          Fix for bug 40049 - do not add correlated variables to
**	the from list
**      15-dec-91 (seputis)
**          fix b40402 - incorrect answer with corelated group by
**	11-apr-94 (ed)
**	    fix b59399 - duplicate handling problem, move endlistpp
**	    code so that correlated function aggregates do not link
**	    back relations in the group by list
**	 5-dec-00 (hayke02)
**	    Test for PST_IFNULL_AOP in pst_flags and add a project node
**	    if this flag is set. This allows correct results from the script
**	    for bug 91232 when the subselect is flattened - the aggregate
**	    passes a NULL to the ifnull() which is now handled correctly.
**	    This change fixes bug 103255.
**	3-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	22-aug-05 (inkdo01)
**	    Change to avoid OPS_FAGG for count's from avg() transform.
**      21-apr-2006 (huazh01)
**          Restrict the fix for b40402. for aggregate view, do not store
**          'endlistpp' to opa_fbylist. Otherwise, it will cause server to
**          link the qualification using the wrong 'bylist' and cause wrong
**          result. This change rewrites the original fix for b109262 and
**          fixes 116010.
[@history_template@]...
*/
VOID
opa_agbylist(
	OPS_SUBQUERY       *subquery,
	bool		    agview,
	bool		    project)
{
    OPS_STATE		*global;
    PST_QNODE		**endlistpp;
    OPS_SQTYPE		sqtype;

    subquery->ops_vmflag = FALSE; /* since corelated aggregates
				** are involved and variables added;
				** the var map may change */
    global = subquery->ops_global;
    endlistpp = NULL;
    sqtype = subquery->ops_sqtype;
    switch(sqtype)
    {
    case OPS_SAGG:
    case OPS_RSAGG:
    {
	/* convert simple aggregate into function aggregate and
	** add variables to by list */
	PST_QNODE	*old_list;
	PST_QNODE	*byhead;

	old_list = subquery->ops_root->pst_left;
	byhead =
	subquery->ops_root->pst_left = 
	subquery->ops_agg.opa_byhead = opv_resdom(global,
	    subquery->ops_agg.opa_aop, 
	    &subquery->ops_agg.opa_aop->pst_sym.pst_dataval);
	byhead->pst_left = old_list;
	byhead->pst_sym.pst_type = PST_BYHEAD;
	byhead->pst_right = subquery->ops_agg.opa_aop;
	for (byhead->pst_left = old_list;
	    byhead->pst_left; )
	{   /* remove all AOP nodes since this will appear on
	    ** the right side of the byhead */
	    if (byhead->pst_left->pst_sym.pst_type == PST_AOP)
		/* remove the AOP node for function aggregates */
		byhead->pst_left = (PST_QNODE *)NULL; /* the left
				** child is the constant expression
				** for the simple aggregate */
	    else
		byhead = byhead->pst_left;
	}
	opa_ifagg(subquery, subquery->ops_agg.opa_byhead); /*
				** init the function aggregate
				** portion of this subquery structure */
	/* fall thru */
    }
    case OPS_HFAGG:
    case OPS_RFAGG:
    case OPS_FAGG:
    {
	PST_QNODE	    *corelp;	/* ptr to var node to be used
				    ** as corelated variable */
	subquery->ops_mask |= OPS_COAGG;    /* mark as having contained
				    ** corelated variables */
	if (global->ops_cb->ops_check 
	    && 
	    opt_strace( global->ops_cb, OPT_F068_OLDFLATTEN)
	    )
	    subquery->ops_mask |= OPS_FLSQL; /* mark as candidate for
					** aggregate flattening */
	if (!subquery->ops_agg.opa_tidactive 
	    && 
	    (subquery->ops_duplicates == OPS_DKEEP))
	{   /* init the keeptid map unless view processing has already set
	    ** it */
	    MEcopy((char *)&subquery->ops_root->pst_sym.pst_value.
		pst_s_root.pst_tvrm, sizeof(OPV_GBMVARS),
		(char *)&subquery->ops_agg.opa_keeptids);
					/* only look at from list since left
					** and right bit maps will contain
					** corelated variables which
					** should not be in the map */
	    subquery->ops_agg.opa_tidactive = TRUE; /* need to
				    ** remove the duplicates introduced
				    ** by the corelated values */
	}
	if ((sqtype == OPS_RFAGG)
	    || 
	    (sqtype == OPS_HFAGG)
	    ||
	    (sqtype == OPS_FAGG))
	{
	    for(endlistpp = &subquery->ops_agg.opa_byhead->pst_left;
		(*endlistpp) && ((*endlistpp)->pst_sym.pst_type == PST_RESDOM);
		endlistpp = &((*endlistpp)->pst_left))
		;
				/* beginning of list of variables which
				** do not become part of projection action */
	}
	if ( project		    /* - save whether a subquery
				    ** requires a projection, so that
				    ** this test can be made */   
				    /* this means that this node is below
				    ** an OR so a projection is required 
				    ** since even if a NULL is returned
				    ** the tuple should not necessarily
				    ** be eliminated */
	    ||
	    (   subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_flags
		&
		PST_IFNULL_AOP
	    )
	    ||
	    (	subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opno 
		==
		global->ops_cb->ops_server->opg_count /* count (attr) */
		&&
		!(subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_flags
		& PST_AVG_AOP)
	    )
	    || 
	    (	subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_opno 
		==
		global->ops_cb->ops_server->opg_scount /* count(*) */
		&&
		!(subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_flags
		& PST_AVG_AOP)
	    ))			    /* the count aggregate always requires a
				    ** projection since it does not return a
				    ** NULL for the empty set, but instead
				    ** returns a 0. Exception is count's in
				    ** avg() transform - we know what we're 
				    ** doing then */
	{   /* this node is below a OR which implies that on the link-back
	    ** tuples can be eliminated which should not be eliminated...
	    ** avoid this by creating a projection so that the link back
	    ** can be guaranteed not to eliminate tuples, but only the
	    ** evaluation of the aggregate result can eliminate the tuple
	    */
	    if (!endlistpp || project)
	    {	/* avoid projection in case of corelated function aggregate
		** if projection action not required due to OR, since function
		** aggregate tuples are eliminated unless at least one tuple
		** qualifies */
		subquery->ops_sqtype = OPS_FAGG;
		subquery->ops_agg.opa_projection = TRUE;
		subquery->ops_agg.opa_mask |= OPA_APROJECT;
		subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_project = TRUE;
	    }
	}
	if (agview)
	{   /* process the correlated aggregate views */
	    OPA_SUBLIST	    *coagp;
	    for (coagp = subquery->ops_agg.opa_agcorrelated; coagp; coagp = coagp->opa_next)
	    {
		OPS_SUBQUERY	*agsub;	/* aggregate subquery which is correlated */
		OPV_IGVARS	gvarno;

		agsub = coagp->opa_subquery;
		if ((*agsub->ops_agg.opa_graft)->pst_sym.pst_type != PST_VAR)
		    opx_error( E_OP0280_SCOPE); /* expecting all child aggregates
				    ** to be resolved at this point */

		gvarno = (*agsub->ops_agg.opa_graft)->pst_sym.pst_value.pst_s_var.pst_vno;
		switch (coagp->opa_subquery->ops_sqtype)
		{
		case OPS_FAGG:
		    break;
		case OPS_HFAGG:
		case OPS_RFAGG:
		{   /* since this aggregate is used in more than one spot it must be
		    ** changed to a function aggregate */
		    OPV_GRV	*grvp;
		    coagp->opa_subquery->ops_sqtype = OPS_FAGG;
		    grvp = global->ops_rangetab.opv_base->opv_grv[gvarno];
		    grvp->opv_created = OPS_FAGG;
		    grvp->opv_gsubselect = (OPV_GSUBSELECT *)NULL;
		    break;
		}
		default:
		    opx_error( E_OP0280_SCOPE); /* expecting only function aggregates */
		}
		opa_covar(subquery, *agsub->ops_agg.opa_graft, gvarno);
	    }
	    subquery->ops_agg.opa_agcorrelated = (OPA_SUBLIST *)NULL;
	}
	for (corelp = subquery->ops_agg.opa_corelated; corelp; )
	{
	    PST_QNODE	*resdomp;
	    OPV_IGVARS	varno;
	    resdomp = opv_resdom (global,
		corelp, &corelp->pst_sym.pst_dataval);
	    if (endlistpp)
	    {
		resdomp->pst_left = *endlistpp;
		*endlistpp = resdomp;
	    }
	    else
	    {
		resdomp->pst_left = subquery->ops_agg.opa_byhead->pst_left;
		subquery->ops_agg.opa_byhead->pst_left = resdomp;
	    }
	    varno = resdomp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
	    BTset((i4)varno, (char *)&subquery->ops_fcorelated);
#if 0
    - to not add to from list or else bug 40049 results, due to opa_aggmap
    being used
	    BTset((i4)varno, (char *)&subquery->ops_root->pst_sym.
		pst_value.pst_s_root.pst_tvrm);
		/* update from list to include this variable which is now
		** visible because of the by list, but not the corelation */
#endif
	    BTset((i4)varno, (char *)&subquery->ops_root->pst_sym.
		pst_value.pst_s_root.pst_lvrm);
		/* update left list to include this variable which is now
		** visible because of the by list, but not the corelation */
	    BTset((i4)varno, (char *)&subquery->ops_agg.opa_blmap);
		/* update bylist to include this variable */
	    subquery->ops_width += resdomp->pst_sym.pst_dataval.db_length; /*add 
					    ** datatype length to total */
	    if (subquery->ops_width > subquery->ops_global->ops_cb->
					ops_server->opg_maxtup)
		opx_error( E_OP0200_TUPLEOVERFLOW );
	    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO; 
					/* relabel for OPC */
	    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =  /* this
				    ** should always be a copy of
				    ** pst_rsno for OPC */
	    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
		++subquery->ops_agg.opa_attlast;
	    corelp = corelp->pst_left;
	    resdomp->pst_right->pst_left = NULL; /* destroy PST_VAR node 
				    ** link */
	}
	if (endlistpp && !agview)
	{
	    PST_QNODE	*resdom1p;
	    i4		atno;
	    atno = subquery->ops_agg.opa_attlast;
	    subquery->ops_agg.opa_fbylist = *endlistpp;
	    for (resdom1p = subquery->ops_agg.opa_byhead->pst_left;
		resdom1p && (resdom1p->pst_sym.pst_type == PST_RESDOM);
		resdom1p = resdom1p->pst_left)
	    {
		resdom1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno = atno;
		resdom1p->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = atno--;
	    }
	}
	break;
    }
    default:
#ifdef    E_OP0280_SCOPE
/*	if (newmap) */
	    opx_error( E_OP0280_SCOPE); /* expecting corelation
				    ** in SQL constructs only */
#endif
    }
    subquery->ops_agg.opa_corelated = (PST_QNODE *)NULL;
/* the following statements are useless since ops_correlated is 0 at this point */
    BTor(OPV_MAXVAR, (char *)&subquery->ops_correlated,
	(char *)&subquery->ops_fcorelated);
    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_correlated);
				    /* subquery is not corelated anymore */
}


/*{
** Name: opa_coagview	- find correlated aggregate views
**
** Description:
**      This routine will traverse the remaining unprocessed subquery nodes 
**      and look for subqueries which still have opa_agcorrelated lists.  It 
**      will process any list which contains all VARs since these need to be 
**      resolved prior to compatibility checks since BY LIST elements will be 
**      changed... so all remaining subqueries need to be processed because 
**      if only one is, then it will not be compatible with any remaining 
**      unprocessed aggregate views. 
**
** Inputs:
**      subquery                        ptr to subquery with correlated aggregate views
**	    .ops_agg.opa_correlated     must be non-empty list
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-may-89 (seputis)
**          initial creation to fix b5655
[@history_template@]...
*/
static VOID
opa_coagview(
	OPS_SUBQUERY       *subquery)
{
    OPS_SUBQUERY	*first;
    first = subquery;
    for (;subquery;subquery = subquery->ops_next)
    {	/* traverse the remainder of the subqueries to resolve any aghead nodes */
	OPA_SUBLIST	    *sublistp;
	sublistp = subquery->ops_agg.opa_agcorrelated;
	if (sublistp)
	{
	    for (;sublistp; sublistp = sublistp->opa_next)
	    {	/* loop to check that all aghead nodes have been resolved */
		if ((*sublistp->opa_subquery->ops_agg.opa_graft)->pst_sym.pst_type != PST_VAR)
		    break;
	    }
	    if (!sublistp)
		opa_agbylist(subquery, TRUE, FALSE); /* if there is still an
				    ** aghead node which has not been
				    ** resolved then wait to resolve it
				    ** on another pass */
	}
    }
#ifdef    E_OP0280_SCOPE
    if (first->ops_agg.opa_agcorrelated)
	opx_error( E_OP0280_SCOPE); /* the first query should have been
				    ** resolved */
#endif
}

/*{
** Name: opa_gsub	- create global subselect descriptor
**
** Description:
**      Create a global subselect descriptor for the subquery 
**
** Inputs:
**      subquery                        ptr to subquery associated
**					with the subselect descriptor
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-sep-92 (ed)
**          initial creation
[@history_template@]...
*/
VOID
opa_gsub(
	OPS_SUBQUERY       *subquery)
{
    OPV_GSUBSELECT	*gsubselect; /* subselect descriptor for
				    ** the global range variable */
    OPV_IGVARS          variable;   /* result variable of subquery */
    OPV_GRV		*tgvarp;   /* ptr to global range variable */

    variable = subquery->ops_gentry;
    gsubselect = (OPV_GSUBSELECT *)opu_memory(subquery->ops_global,
		    (i4) sizeof(OPV_GSUBSELECT));
    gsubselect->opv_subquery = subquery; /* save ptr to subquery
				    ** plan used to evaluate
				    ** the select */
    gsubselect->opv_parm = (OPV_SEPARM *)NULL; /* list of correlated variables
				    ** used in subquery, and their
				    ** respective constant nodes */
    gsubselect->opv_ahd = (QEF_AHD *)NULL; /* initialize query plan struct
				    ** for query compilation */
    tgvarp = subquery->ops_global->ops_rangetab.opv_base->opv_grv[variable];
    tgvarp->opv_gsubselect = gsubselect; /* attach subselect descriptor
				    ** to the global range variable */
    tgvarp->opv_gquery = subquery;
    if (subquery->ops_sqtype == OPS_SELECT)
	BTset((i4)variable, 
	    (char *)&subquery->ops_global->ops_rangetab.opv_msubselect);
				    /* set subselect bitmap */
}

/*{
** Name: opa_compat	- find aggregates which can be computed together
**
** Description:
**      This is the main entry point for the phase of aggregate processing 
**      which determines that aggregates can be computed together
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**      global->...ops_subquery->ops_agg.opa_concurrent - aggregates which can
**                                      be computed together will be attached
**                                      to this list and removed from the
**                                      global->ops_subquery list
**      global->ops_subquery            each inner aggregate in this list will 
**                                      be substituted by a result node in the
**                                      outer aggregate.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-jun-86 (seputis)
**          initial creation from agg_findcompat in aggregate.c
**      12-may-89 (seputis)
**          fix b5655
**	10-feb-93 (ed)
**	    fix b49455 - due to quel-like interface, need to remove
**	    outer join variable map from main query if variable is defined
**	    in aggregate, since from lists are copied into the main
**	    query
**      7-dec-93 (ed)
**          b56139 - remove code which is resetting range var to -1
**	17-jan-94 (ed)
**	    - changed outer join processing to be more restrictive
**	    in resetting of outer join maps to correct problem with
**	    and outer join defined on an aggregate within a view
[@history_line@]...
*/
VOID
opa_compat(
	OPS_STATE          *global)
{
/* Process each aggregate in the subquery list.
** The aggregates are in a 'leftmost innermost' order for this traversal.
*/
    OPS_SUBQUERY    *subquery;		    /* ptr to ptr to current 
					    ** subquery being analyzed
					    */

    for ( subquery = global->ops_subquery;  /* start at beginning of 
					    ** subquery list */
	  subquery->ops_sqtype != OPS_MAIN; /* if next subquery does not
                                            ** exist then this is main query */
	  subquery = subquery->ops_next)    /* get next subquery in list  */
    {
	OPS_PNUM	    sagg_count;	    /* number of subqueries to be
					    ** calculated together, used for
					    ** a consistency check for simple
					    ** aggregates */

	sagg_count = 0;
# ifdef OPT_F003_AGGREGATE
	if (opt_strace( global->ops_cb, OPT_AGLOOP ) )
	    TRdisplay("begin processing next aggregate\n");
# endif
	subquery->ops_dist.opd_dv_cnt = global->ops_parmtotal; /* send all
					    ** the currently defined constants
					    ** to the LDB */
	if (subquery->ops_agg.opa_agcorrelated)
	    opa_coagview(subquery);	    /* call this routine if a query
					    ** mod substituted a view which
					    ** was a group by, and an aghead
					    ** was used as a correlated
					    ** variable */
	if ((subquery->ops_sqtype != OPS_VIEW)
	    &&
	    (subquery->ops_sqtype != OPS_UNION)
	    )
	{
	    opa_substitute(subquery, OPV_NOGVAR); /* substitute nodes for result
					    ** into outer aggregate */
	    opa_findcompat(subquery);	    /* look for aggregates or
					    ** subselects that we can
					    ** compute at the same time with
					    ** this aggregate or subselect */
	    {
		OPS_SUBQUERY       *concurrent; /* used to traverse aggregates which
					    ** will be computed together to
					    ** decrement the nesting count of
					    ** the outer aggregate - needs to be
					    ** decremented here otherwise a
					    ** garbage value may be used */
		for (concurrent = subquery; 
		    concurrent; 
		    concurrent = concurrent->ops_agg.opa_concurrent)
		{
		    OPL_PARSER	*child_iojp; /* ptr to maps of outer join definitions
					    ** for child subquery */
		    OPL_PARSER	*father_iojp; /* ptr to maps of outer join definitions
                                            ** for parent subquery */
		    if ((global->ops_qheader->pst_numjoins > 0)
			&&
			(child_iojp = concurrent->ops_oj.opl_pinner)
			&&
			(father_iojp = concurrent->ops_agg.opa_father->ops_oj.opl_pinner)
			)
		    {	/* OPF/PSF interface is quel like, in that the
			** aggregrate root node from list is a subset of the main query
			** root node from list, but outer join semantics should
			** only be performed in the aggregate node, so remove
			** the outer join info from the main query,... this
			** becomes important when aggregate results cannot substitute
			** the main query variables for SQL, such as
			** select avg(a), avg(distinct a) from s left join r
			**   group by b */
			OPV_IGVARS	gvar;
			OPL_PARSER	*child_oojp;
			OPL_PARSER	*father_oojp;

			child_oojp = concurrent->ops_oj.opl_pouter;
			father_oojp = concurrent->ops_agg.opa_father->ops_oj.opl_pouter;
			for (gvar = -1; (gvar = BTnext((i4)gvar, (char *)&concurrent
			    ->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
			    (i4)BITS_IN(concurrent->ops_root->pst_sym.pst_value.
				pst_s_root.pst_tvrm)))>=0;)
			{
			    if (BTtest((i4)gvar, (char *)&concurrent->ops_agg.opa_father
				->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
			    {	/* variable is copied from the aggregate in parent by
				** the parser, the outer joins will be evaluated in the
				** aggregate so remove variables from the parent */
				/* clear only those outer join bits which are in the
				** child and parent since view substitution can introduce
				** outer joins which should not be cleared */
				OPL_IOUTER	ojid;
				for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)
				    &child_iojp->opl_parser[gvar],
				    (i4)global->ops_goj.opl_glv)) >= 0;)
				    BTclear((i4)ojid, (char *)&father_iojp->opl_parser[gvar]);
				for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)
				    &child_oojp->opl_parser[gvar],
				    (i4)global->ops_goj.opl_glv)) >= 0;)
				    BTclear((i4)ojid, (char *)&father_oojp->opl_parser[gvar]);
			    }
			}
		    }
		    concurrent->ops_agg.opa_father->ops_agg.opa_nestcnt--;
		    sagg_count++;
		}
	    }
	}

	switch (subquery->ops_sqtype)
	{   /* create virtual table descriptors */
	case OPS_UNION:			    /* only first element of union
                                            ** list requires a descriptor */
	    break;
	case OPS_SELECT:
	case OPS_RFAGG:
	case OPS_HFAGG:
	case OPS_RSAGG:
	case OPS_VIEW:
	{   /* if this is a subselect then create a subselect descriptor
            ** - it is needed prior to the final pass so that repeat query
            ** parameters can be requested from nested subselects 
	    ** - this structure is also created for corelated aggregates so that
	    ** a query plan can be looked up for these types of aggregates
	    */
	    opa_gsub(subquery);
	    break;
	}
	case OPS_FAGG:
	{
	    global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry]->
		opv_gquery = subquery;	    /* save ptr to subquery used to
					    ** evaluate function aggregate */
#ifdef E_OP0281_SUBQUERY
	    if (BTcount((char *)&subquery->ops_correlated, OPV_MAXVAR) != 0)
		opx_error(E_OP0281_SUBQUERY);   /* unexpected subquery type */
#endif
	    break;
	}
	case OPS_SAGG:
	{
	    if (BTcount((char *)&subquery->ops_correlated, OPV_MAXVAR) == 0)
	    {
		subquery->ops_dist.opd_sagg_cnt = global->ops_parmtotal -
		    subquery->ops_dist.opd_dv_base + 1; /* this is the number
					    ** of simple aggregate results
					    ** which will be computed at this
					    ** node, since all simple aggregates
					    ** to be evaluated together have been
					    ** determined */
                if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
                    &&
                    (   (subquery->ops_dist.opd_sagg_cnt <= 0)
                        ||
                        (subquery->ops_dist.opd_sagg_cnt != sagg_count)
                    )
                    )
                    opx_error(E_OP0A89_SAGG);
		break;			    /* consistency check - break if
                                            ** not correlated otherwise fall
                                            ** through and report error */
	    }
	}
#ifdef E_OP0281_SUBQUERY
	default:
	    opx_error(E_OP0281_SUBQUERY);   /* unexpected subquery type */
#endif
	}

#if 0
	/* seems like this code propagates a -1 */
	if (subquery->ops_union && subquery->ops_sqtype != OPS_UNION)
	{   /* check for head of list of union subqueries */
	    OPS_SUBQUERY    *unionp; /* traverse union nodes if they exist*/

	    /* if union nodes exist then propagate the result var so that
            ** the union subqueries can navigate to the global range var */
	    for (unionp =subquery->ops_union;
		unionp;
		unionp = unionp->ops_union)
	    {
		unionp->ops_gentry = subquery->ops_result;
	    }
	}
#endif
# ifdef OPT_F007_CONCURRENT
	if (opt_strace( global->ops_cb, OPT_F007_CONCURRENT ) )
	{
	    TRdisplay("The aggregates begin processed are:\n");
	    opa_pconcurrent(subquery);
	}
# endif
    }
    subquery->ops_dist.opd_dv_cnt = global->ops_parmtotal; /* this should be
					    ** OPS_MAIN , send all
					    ** the currently defined constants
					    ** to the LDB */
}
