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
#include    <adfops.h>
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
#define             OPA_GENERATE        TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPAGEN.C - routines to generate list of all aggregates
**
**  Description:
**      Generate list of all aggregates
**
**  History:    
**      25-jun-86 (seputis)    
**          initial creation from findagg in aggregate.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
{@history_line@}
**      14-jul-93 (ed)
**          replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      24-nov-93 (rickh)
**          When copying result domains, make sure you also copy the
**          default id.
**      04-mar-94 (davebf)
**          check for singlesite distributed for flattening decision.
**      06-mar-96 (nanpr01)
**          Do not check the maxtuple size with the ops_width.
**          Increase tuple size project.
**      06-jun-96 (kch)
**          In the function opa_qview(), we now set gstate->opa_joinid to
**          OPL_NOINIT, to indicate that it has not been initialized. This
**          change fixes bugs 76337.
**      6-june-97 (inkdo01 for hayke02)
**          In the function opa_cunion(), when setting gvar, we now check
**          that the ops_rangetab.opv_base does not already use this element
**          and increment gvar if this is the case. This occurs for a 
**          'create table ... as select ... union select ... union all
**          select ...' style query. This change fixes bug 82637.
**      22-Feb-99 (hanal04)
**          b91232 - Don't flatten a query if the subselect contains
**          a correlated aggregate inside an "ifnull" statement.
**       5-dec-00 (hayke2)
**          The fix for bug 91232 (hanal04's ingres!oping20 change 442351)
**          has been modifed so that we now do not switch off flattening
**          when we find an ifnull(<aggfunc>()) in a subselect - rather
**          we mark the aggregate node with the new flag PST_IFNULL_AOP
**          and add a project node if this is the case in opa_agbylist().
**          The function opa_ifnullcagg() has been modified so that we now
**          find the address of the PST_IFNULL_AOP node. This prevents
**          performance degradation when flattened subselects become
**          non-flattened in the presence of the fix for 91232. This 
**          change fixes bug 103255.
**      19-jun-01 (hayke02)
**          Fix up duplicate handling for simple aggregate views on
**          union views. This change fixes bug 100100.
**      11-jan-02 (huazh01/hayke02)
**          Modify the fix for b74103(change 425180). 
**          For simple aggregate on an aggregate view, we don't need
**          to remove duplicates. This fix b106767, which happens when we
**          "select sum(col) on view where col = condition". 
**      11-dec-2002 (huazh01)
**          Extend the fix for bug 91232. Don't flatten a query for
**          the case of subselect statement containing a 'max' 
**          aggregate.
**          This fixes bug 109262. INGSRV / 2028. 
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG, even though they shouldn't
**	    appear yet this early in opa.
**      10-may-07 (huazh01) 
**          back out the fix for b109262, it is now in opa_agbylist().
**          this fixes 116010. 
**      10-sep-08 (gupsh01,stial01)
**          opa_uop() ignore inconsistent joinid for ADI_UNORM_OP
**	2-Jun-2009 (kschendel) b122118
**	    Use clearmask in a few places instead of copy / not / and.
**	13-Nov-2009 (kiria01) SIR 121883
**	    Distinguish between LHS and RHS card checks for 01.
**	9-Dec-2009 (drewsturgiss) 
**	    Replaced occurences of 
**		subquery->ops_global->ops_cb->ops_server->opg_ifnull with the new
**		ADI_F32768_IFNULL flag
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
**
**      23-Mar-2010 (horda03) B123416
**          Remove check of PSF's corelation check in opa_exists.
**/

/*}
** Name: OPA_RECURSION - state of processing parse tree
**
** Description:
**      This structure describes the subquery state for the information 
**      gathering pass of the parse tree.  It is structured as a control 
**      block to reduce the recursive memory requirements for long 
**      unbalanced trees, like 300 column resdom lists 
**
** History:
**      20-jan-89 (seputis)
**          initial creation
**      30-oct-89 (seputis)
**          added support for union view optimization
[@history_template@]...
*/
typedef struct _OPA_RECURSION
{
    OPS_STATE       *opa_global; /* ptr to state of query processing */
    OPS_SUBQUERY    *opa_gfather;/* ptr to outer aggregate of this node or
                                ** NULL if "outer aggregate" is root node
                                */
    OPV_GBMVARS     *opa_gvarmap; /* ptr to var bitmap on left or right side
                                ** which needs to be updated
                                */
    bool            opa_demorgan; /* boolean indicating demorgan's
                                ** law should be applied */
    bool            opa_gflatten; /* TRUE if node is not below an OR, which means
                                ** a projection is required */
    i4              opa_nestcount; /* number of inner aggregates found
                                ** within this tree */
    OPS_SUBQUERY    *opa_newsqptr;  /* temp variable to reduce recursion 
                                ** memory requirements - not really needed */
    bool            opa_gaggregate; /* TRUE if the father subquery is an aggregate
                                */
    PST_QNODE       *opa_ojbegin;   /* beginning of qualifications which are
                                ** part of outer joins being flattened */
    PST_QNODE       **opa_ojend; /* ptr to ptr to end of qualification list */
    PST_J_ID        opa_joinid; /* joinid from parent which should be used to
                                ** initialize all fields of PST_OP_NODEs which
                                ** are substituted in view handling */
    OPS_SUBQUERY    *opa_simple; /* list of simple non-correlated aggregates
                                ** defined within the context of this subquery */
    OPS_SUBQUERY    *opa_uvlist; /* list of union view definitions defined within
                                ** the scope of this query */
} OPA_RECURSION;

static VOID opa_rnode(
        OPA_RECURSION       *gfather,   /* ptr to state of father subquery */
        PST_QNODE          **agg_qnode  /* ptr to ptr to query tree node to be 
                                        ** analyzed
                                        */ );
static VOID opa_generate(
        OPA_RECURSION       *gstate,
        PST_QNODE          **agg_qnode);
                                /* recursive routine to gather info
                                ** on query tree */

/*{
** Name: opa_rsmap      - create map of available resdom numbers
**
** Description:
**      Routine will create a map of resdom numbers which are not used in the
**      resdom list associated with the subquery
**
** Inputs:
**      subquery                        subquery being analyzed
**
** Outputs:
**      attrmap                         ptr to bitmap of resdom numbers which
**                                      are not being used
**      Returns:
**          TRUE - if a non-printing resdom was found
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      30-jun-87 (seputis)
**          initial creation
**      02-feb-99 (wanfr01)
**          Change for bug 83929:
**          Byhead nodes will no longer prematurely abort tree traversal. 
**
[@history_template@]...
*/
bool
opa_rsmap(
        OPS_STATE          *global,
        PST_QNODE          *root,
        OPZ_BMATTS         *attrmap)
{
    /* resdom numbers on main query are set by parser can should not be
    ** be changed so that any non-printing resdoms which are added should
    ** use unused resdom numbers */
    PST_QNODE   *resdomptr;         /* used to traverse the list of query
                                    ** tree resdom nodes */
    bool        non_printing;       /* set TRUE if non printing resdom is found */

    non_printing = FALSE;
    MEfill( sizeof(*attrmap), (u_char)0, (PTR)attrmap);
    for(resdomptr = root->pst_left;
        resdomptr;
        resdomptr = resdomptr->pst_left)
    {
        if (resdomptr->pst_sym.pst_type != PST_RESDOM) continue;
#ifdef    E_OP0280_SCOPE
        /* scope consistency check */
        if (resdomptr->pst_sym.pst_value.pst_s_rsdm.pst_rsno >=
            (BITS_IN(OPZ_BMATTS)))
            opx_error( E_OP0280_SCOPE); /* check if resdom number is in
                                    ** range */
#endif
        BTset((i4)resdomptr->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
            (char *) attrmap);
        if (!(resdomptr->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
            non_printing = TRUE;
    }
    BTnot( (i4)BITS_IN(OPZ_BMATTS), (char *) attrmap);
    return(non_printing);
}

/*{
** Name: opa_ctarget    - copy target list from master list
**
** Description:
**      This routine copies a resdom list from a union view and 
**      and creates varnodes off the right of side of each resdom 
**      which reference the respective element of the union view 
[@comment_line@]...
**
** Inputs:
**      global                          ptr to global state variable
**      masterp                         ptr to resdom element to copy
**      gvar                            range var of union view
[@PARAM_DESCR@]...
**
** Outputs:
**      resdompp                        return location for resdom list
[@PARAM_DESCR@]...
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-sep-88 (seputis)
**          initial creation
**      24-may-90 (seputis)
**          - b20948, do not copy non-printing resdoms to parent target list
**      24-nov-93 (rickh)
**          When copying result domains, make sure you also copy the
**          default id.
**      1-mar-94 (robf)
**          When copying result domains, make sure you also copy the
**          dmuflags field.
[@history_line@]...
[@history_template@]...
*/
static VOID
opa_ctarget(
        OPS_STATE          *global,
        PST_QNODE          *masterp,
        PST_QNODE          **resdompp,
        OPV_IGVARS         gvar,
        bool               copytarget)
{
    for (; masterp; masterp = masterp->pst_left)
    {
        /* create a resdom list for the main query which reads the tuples
        ** from the union */
        PST_QNODE       *varp;              /* var node used to represent
                                            ** the view attribute */
        DB_ATT_ID       dmfattr;            /* resdom attribute number */
        PST_QNODE       *copyp;             /* ptr to resdom copy */

        if (masterp->pst_sym.pst_type != PST_RESDOM)
        {
            if (masterp->pst_sym.pst_type == PST_TREE)
                *resdompp = masterp;        /* use the same PST_TREE node
                                            ** to terminate the list */
            else
                *resdompp = (PST_QNODE *) NULL;
            break;
        }
        else
            if (!(masterp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT))
                continue;                   /* do not copy non-printing resdoms
                                            */
        dmfattr.db_att_id = 
            masterp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
        varp = opv_varnode( global, &masterp->pst_sym.pst_dataval, gvar, 
            &dmfattr);
        copyp =
        *resdompp = opv_resdom(global, varp, &masterp->pst_sym.pst_dataval);
        MEcopy((PTR)&masterp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0],
            sizeof(copyp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
            (PTR)&copyp->pst_sym.pst_value.pst_s_rsdm.pst_rsname[0]);
                                            /* copy attribute name */
        copyp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =  
            masterp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno; /* match the target
                                            ** number of the original query */
        if (copytarget)
            copyp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype =  
                masterp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype; /* match the resdom
                                            ** type of the original query */
        else
            copyp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype =  PST_ATTNO;
        masterp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;  /* relabel
                                            ** this interior node for OPC */
        copyp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
            masterp->pst_sym.pst_value.pst_s_rsdm.pst_rsno; /* match the resdom
                                            ** number of the original query */
        COPY_DEFAULT_ID( masterp->pst_sym.pst_value.pst_s_rsdm.pst_defid,
                copyp->pst_sym.pst_value.pst_s_rsdm.pst_defid);

        copyp->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags=
                masterp->pst_sym.pst_value.pst_s_rsdm.pst_dmuflags;

        resdompp = &copyp->pst_left;
        copyp->pst_left = NULL;             /* terminate resdom list of
                                            ** new main query list */
    }
}

/*{
** Name: opa_agview     - maintain duplicate semantics for function aggregate views
**
** Description:
**      This traversal is only done in SQL, but an attempt is made to find an aghead node
**      which will contain the group by list... this list should be used to determine the 
**      duplicate semantics for the  view 
**
** Inputs:
**      global                          ptr to global state variables
**      viewlist                        ptr to return group by list
**
** Outputs:
**
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      3-jun-89 (seputis)
**          initial creation to fix b6177, b6178, b6179
**      24-jul-89 (seputis)
**          fix b7080
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
[@history_template@]...
*/
static VOID
opa_agview(
        OPS_STATE          *global,
        PST_QNODE          **agheadp,
        OPV_GBMVARS        *from_map,
        PST_QNODE          *qnode)
{
    for (;qnode; qnode = qnode->pst_left)
    {
        switch (qnode->pst_sym.pst_type)
        {
        case PST_AGHEAD:
        {   /* make some checks to ensure that the AGHEAD has the expected
            ** bit maps */
#ifdef    E_OP0280_SCOPE
                /* scope consistency check */
            if ((MEcmp((char *)from_map, (char *)&qnode->pst_sym.pst_value.
			pst_s_root.pst_tvrm, sizeof(*from_map)))
                ||
                (qnode->pst_left->pst_sym.pst_type != PST_BYHEAD)
                ||
                (   *agheadp
                    &&
                    (   !opv_ctrees(global, (*agheadp)->pst_left->pst_left, qnode->pst_left->pst_left)
                        ||
                        !opv_ctrees(global, (*agheadp)->pst_right, qnode->pst_right)
                    )
                    /* the target lists should be equal here since view
                    ** was created with an SQL group by */
                ))
                opx_error( E_OP0280_SCOPE); /* this view should have been
                                        ** created by a group by statement
                                        ** function aggregate, in which case
                                        ** the target list of the view should
                                        ** be the same as the aggregate */
#endif
            *agheadp = qnode;
        }
        case PST_ROOT:
        case PST_SUBSEL:
            return;             /* this subselect has its' own scope */
        default:
            break;
        }
        if (qnode->pst_right)
            opa_agview(global, agheadp, from_map, qnode->pst_right);
    }
}

/*{
** Name: opa_uview      - copy target list of view
**
** Description:
**      At least one view exists which is distinct but all the parents are 
**      not so the target list of the view must be brought to the top of 
**      the subquery so that when duplicates are removed, the target
**      list of the view will keep the correct contribution of the view.
**      It is not enough to bring up only a projection of the view.
**      If any parent of a view removes duplicates then the target list
**      does not need to be copied. 
**          Note that TIDs of all the views which are distinct from the
**      subquery root, have bits set in keeptids, which will be used to
**      avoid removing duplicates for those relations. 
**          Thus, there are 3 classes of views
**      1) views in which all parents are non-distinct but the view is distinct
**         - need to bring up the target list of the view
**      2) views in which all parents and view are non-distinct 
**         - need to bring up the TIDs of any base relations in view
**      3) views in which at least one parent is distinct
**         - do not bring anything up
**
** Inputs:
**      subquery                        subquery being processed
**         ->ops_agg.opa_uview          bitmap of views to copy
**
** Outputs:
**      subquery->ops_root              non-printing resdoms added for
**                                      opa_uview target list
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-mar-86 (seputis)
**          initial creation
**      1-jun-89 (seputis)
**          fix b6281
**      3-jun-89 (seputis)
**          fix b6177 b6178 b6179, keep duplicate semantics for function aggregate views
**      10-sep-89 (seputis)
**          place resdoms in decreasing order since OPC assumes this
**      24-may-90 (seputis)
**          - b20948 do not copy resdom list to parent for union views
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      13-may-91 (seputis)
**          moved 3-jun-89 modification to opa_qview for bug 37469
**      13-may-92 (seputis)
**          - fix for bug 44095, obtain duplicate handling from by list
**          of aggregate instead of view query to perhaps eliminate the
**          evaluation of the aggregate
**      16-may-96 (inkdo01)
**          Looks like PST_NODUPS must also be set in root node of current
**          subquery to force approp. dups semantics (bug 74103)
**      12-jan-2001 (huazh01/hayke02)
**          Modify the fix for b74103.
**          For simple aggregate on an aggregate view, we don't need
**          to remove duplicates. This fix b106767, which happens when we
**          "select sum(col) on view where col = condition".
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**         
**      [@history_template@]...
*/
static VOID
opa_uview(
        OPA_RECURSION      *gstate)
{
    OPV_IGVARS      viewvar;        /* global var representing view */
    OPV_GBMVARS     (*uviewmap);    /* ptr to map of views to copy */
    OPS_STATE       *global;        /* ptr to OPF global state */
    OPZ_BMATTS      attrmap;        /* bit map of resdom number on main
                                    ** query which are not used */
    OPZ_IATTS       nextattr;       /* next available attribute number which
                                    ** can be used as a resdom identifier */
    OPS_SUBQUERY    *subquery;

    subquery = gstate->opa_gfather;
    global = subquery->ops_global;
    if (subquery->ops_agg.opa_father && 
        (
            (subquery->ops_sqtype == OPS_RFAGG)
            ||
            (subquery->ops_sqtype == OPS_HFAGG)
            ||
            (subquery->ops_sqtype == OPS_FAGG)
            ||
            (subquery->ops_sqtype == OPS_SAGG)
            ||
            (subquery->ops_sqtype == OPS_RSAGG)
        ))
            BTor(OPV_MAXVAR, (char *)&subquery->ops_agg.opa_uviews,
                (char *)&subquery->ops_agg.opa_father->ops_aggmap);
                                        /* set all variables reference by
                                        ** the function aggregate into the
                                        ** SELECT parent, do not include
                                        ** the left and right maps since
                                        ** these could contain correlated
                                        ** variables */
    uviewmap = &subquery->ops_agg.opa_uviews;
    (VOID) opa_rsmap( global, subquery->ops_root, &attrmap); /* get a map of 
                                ** resdom numbers which
                                ** have not been used in the target list */
    nextattr = 0;

    for (viewvar = -1;
         (viewvar = BTnext((i4)viewvar, 
                            (char *)uviewmap, 
                            (i4)BITS_IN(OPV_GBMVARS)
                           )
         )
         >= 0;
        )
    {   /* for each view which requires duplicates perserved
        ** copy the target list to the parent query */
        PST_QNODE           *viewlist;
        PST_QNODE           *oldroot;
        PST_QNODE           **viewpp;
        PST_QNODE           *union_view;
        PST_QNODE           *root;

        oldroot = subquery->ops_root->pst_left;
        viewlist=
            global->ops_qheader->pst_rangetab[viewvar]->pst_rgroot->pst_left;
        root = global->ops_qheader->pst_rangetab[viewvar]->pst_rgroot;
        union_view = root->pst_sym.pst_value.pst_s_root.pst_union.pst_next;

        if (!union_view)
        {
            if  ((root->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
                &&
                (root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
                &&
                (root->pst_sym.pst_value.pst_s_root.pst_dups != PST_NODUPS))
                                                /* this view is a group by
                                                ** aggregate so uniqueness can be
                                                ** determined by the group by list
                                                ** of the aggregate, thus perhaps
                                                ** eliminating evaluation of an aggregate
                                                */
            {
                PST_QNODE   *agheadp;           /* list of aggregates which
                                                ** may need to be placed into
                                                ** the target list as non-printing
                                                ** resdoms */
                agheadp = (PST_QNODE *)NULL;
                opa_agview(subquery->ops_global, &agheadp,
                    &root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    root->pst_right);           /* get the bylist of the
                                                ** aggregate */
                opa_agview(subquery->ops_global, &agheadp,
                    &root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    root->pst_left);            /* get the bylist of the
                                                ** aggregate */
#ifdef    E_OP0280_SCOPE
                /* scope consistency check */
                if (!agheadp)
                    opx_error( E_OP0280_SCOPE); /* if some base relations
                                            ** are already in the bview map
                                            ** then there is a conflict
                                            ** between views */
#endif
                viewlist = agheadp->pst_left->pst_left; /* get target list of aggregate */

            }
            opv_copytree(subquery->ops_global, &viewlist);
        }
        else
            /* union view is found so create a PST_VAR resdom list
            ** to remove duplicates from the view */
            opa_ctarget(global, union_view->pst_left, &viewlist, viewvar, TRUE);

        subquery->ops_root->pst_left = (PST_QNODE *)NULL;
        {
            i4              ttarg_type;         /* this needs to match the
                                                ** existing target list or
                                                ** OPC will complain about
                                                ** a resdom labelling error */
            PST_QNODE       *viewp;
            if (oldroot && (oldroot->pst_sym.pst_type == PST_RESDOM))
                ttarg_type = oldroot->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype;
            else
                ttarg_type = viewlist->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype;
            viewpp = (PST_QNODE **)NULL;
            for(viewp = viewlist;
                viewp && (viewp->pst_sym.pst_type == PST_RESDOM);)
            {   
                PST_QNODE       *oldnodep;
                /* traverse the resdom list to determine if this 
                ** non-printing resdom is duplicated */
                for (oldnodep = oldroot; oldnodep; oldnodep = oldnodep->pst_left)
                {
                    if (oldnodep->pst_sym.pst_type == PST_RESDOM)
                    {
                        if (opv_ctrees(global, oldnodep->pst_right, viewp->pst_right))
                            break;
                    }
                }
                if (oldnodep)
                {
                    viewp = viewp->pst_left;    /* skip over node since expression
                                                ** is already in target list */
                    continue;
                }
                viewp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= ~PST_RS_PRINT; /* non
                                                ** printing resdom needed just to
                                                ** preserve duplicates */
                if (!viewpp)
                    viewpp = &viewp->pst_left;  /* need to save the last element 
                                                ** in the list to link up the oldroot
                                                ** in decreasing resdom ordering */
                /* need to assign an unused resdom number to the node so
                ** that query compilation can uniquely label columns
                ** to the sorter */
                viewp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = /* this
                                            ** should always be a copy of
                                            ** pst_rsno for OPC */
                viewp->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 
                nextattr = BTnext(  (i4)nextattr, (char *)&attrmap, 
                                    (i4)BITS_IN(attrmap) ); /* check if
                                            ** too many attributes are being
                                            ** defined */
                viewp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = ttarg_type;
                if (nextattr <=0 )
                    opx_error(E_OP0201_ATTROVERFLOW); /* too many attributes
                                            ** are being defined */
                {
                    PST_QNODE   *tviewp;
                    tviewp = viewp->pst_left;
                    viewp->pst_left = subquery->ops_root->pst_left;
                    subquery->ops_root->pst_left = viewp; 
                                            /* need link into resdom
                                            ** list so that numbers will be
                                            ** decreasing for a retrieve for OPC */
                    viewp = tviewp;
                }
            }
        }
        {
            OPV_GBMVARS     *oldmap;
            i4              nestcount;

            oldmap = gstate->opa_gvarmap;
            nestcount = gstate->opa_nestcount;
            gstate->opa_gvarmap = (OPV_GBMVARS *)&subquery->ops_root->
                pst_sym.pst_value.pst_s_root.pst_lvrm;
            gstate->opa_demorgan = FALSE;   /* just in case */
            opa_generate(gstate, &subquery->ops_root->pst_left);
                                            /* traverse the new sub tree */
            gstate->opa_gvarmap = oldmap;
            subquery->ops_agg.opa_nestcnt += gstate->opa_nestcount - nestcount; 
                                            /* set number of
                                            ** aggregates in the subtree 
                                            ** rooted by this aggregate 
                                            ** node */
        }

        if (viewpp)
            *viewpp = oldroot;              /* attach the old resdom list */
        else
            subquery->ops_root->pst_left = oldroot;
    }
    /* Force dups removal into current subquery */
    if ((global->ops_union == FALSE) &&
        ((subquery->ops_root->pst_sym.pst_type == PST_ROOT) ||
        (subquery->ops_root->pst_sym.pst_type == PST_AGHEAD)))
    {
      /* b106767 */
      if ( subquery->ops_sqtype != OPS_SAGG )
        subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_dups = PST_NODUPS;
    }
    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_agg.opa_uviews);
                                            /* reinit field after processing
                                            ** so it can be used when flattening
                                            ** subselects */
}

/*{
** Name: opa_fojid      - find the outer join id
**
** Description:
**      Find the outer join ID associated with this view if any.  If the view
**      is used within the scope of an outer join then the first joinid 
**      for which this view is contained in the inner would be used.  If the
**      view is outer to all joinid's then the view should be querymoded into
**      the where clause.  For FULL JOINs it becomes a special case in which
**      a joinid is created for the view if a where clause exists.
**      
**
** Inputs:
**      gstate                          ptr to aggregate state variable
**      rangep                          ptr to parser range table entry for view
**
** Outputs:
**
**      Returns:
**          outer join id or PST_NOJOIN
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      13-feb-90 (seputis)
**          initial creation
**      8-feb-93 (ed)
**          - fix view flattening bug with outer joins
**      15-apr-93 (ed)
**          - fixed bug 49736 - used inner instead of outer maps
**      15-feb-94 (ed)
**          - bug 58847 - assumption was made that no "holes"
**          in the PSF range table existed, but this was not
**          correct
**      1-mar-94 (ed)
**          - ojid's which have variable defined as inner should be considered
**          as possible ojid's for where clause labelling 
[@history_template@]...
*/
static PST_J_ID
opa_fojid(
        OPA_RECURSION      *gstate,
        PST_RNGENTRY       *rangep,
        OPV_IGVARS         viewid)
{
    OPS_STATE       *global;        /* ptr to OPF global state */
    OPS_SUBQUERY    *subquery;
    PST_J_ID        inner;
    PST_RNGENTRY    **endpp;        /* used to terminate search of range table */
    OPV_IGVARS      original;       /* the original viewid to be used for
                                    ** full join processing */
    PST_J_ID        joinid;         /* joinid to be returned for view*/

    original = viewid;
    subquery = gstate->opa_gfather;
    global = subquery->ops_global;
    inner = PST_NOJOIN;
    for(;(inner = BTnext((i4)-1, (char *)&rangep->pst_inner_rel, (i4)BITS_IN(rangep->pst_inner_rel)))<0;)
    {
        /* look for the view, or view parent which has an
        ** outer join definition */
        if (rangep->pst_rgparent >= 0)
        {
            viewid = rangep->pst_rgparent;
            rangep = global->ops_qheader->pst_rangetab[viewid];
        }
        else
            return(PST_NOJOIN);
    }
    /* a non-empty inner map has been found, so use it to determine the
    ** outer join ID */

    endpp = &global->ops_qheader->pst_rangetab[global->ops_qheader->pst_rngvar_count]; /* used to 
                                    ** terminate search of range table */
    if (BTnext((i4)inner, (char *)&rangep->pst_inner_rel, (i4)BITS_IN(rangep->pst_inner_rel))<0)
        joinid = inner;             /* if only one inner exists then this must be the
                                    ** correct joinid for the view */
    else
    {   /* inner map has multiple entries at this point but since it is
        ** possible that and INNER JOIN or FULL JOIN existed.
        ** The joinid's for each bit position needs to be counted in order
        ** to eliminate the joinid's */
        PST_J_ID        numjoins;   /* number of outer joins in query */
        i4             size;        /* size of outer join count array */
        PST_RNGENTRY    **innerpp;  /* ptr to range variable being analyzed */
        PST_J_ID        ojid;       /* current joinid being analyzed */
        i4              best_count; /* smallest number of variables associated
                                    ** with ojid join id */

        innerpp = global->ops_qheader->pst_rangetab;

        numjoins = global->ops_qheader->pst_numjoins;
        size = sizeof(OPL_IOUTER) * (numjoins+1); /* PSF outer join IDs
                                    ** start at 1 */
        if (!global->ops_goj.opl_view)
        {
            global->ops_goj.opl_view = (OPL_GOJT *)opu_memory(global, size); 
                                        /* allocate enough for view ID count
                                        ** table, which starts
                                        ** at "1" rather and "0" index */
            MEfill(size, (u_char)0, (PTR)global->ops_goj.opl_view);
            /* since outer joins are nested due to syntax, each outer join
            ** position has a unique relation count depending on the nesting 
            ** with respect to any parent or any children */
            for (; innerpp != endpp; innerpp++)
            {
                if (*innerpp 
                    && 
                    ((*innerpp)->pst_rgtype != PST_UNUSED))
                {
                    PST_J_MASK      both_masks;
                    MEcopy((PTR)&(*innerpp)->pst_inner_rel, sizeof(both_masks),
                            (PTR)&both_masks);
                    BTor((i4)(BITS_IN(both_masks)), (char *)&(*innerpp)->pst_outer_rel,
                            (char *)&both_masks);
                    for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)&both_masks, (i4)BITS_IN(both_masks))) >= 0;)
                    {
                        if (ojid > numjoins)
                            opx_error(E_OP0395_PSF_JOINID);
                        global->ops_goj.opl_view->opl_gojt[ojid]++;
                    }
                }
            }
        }
        best_count = PST_NUMVARS+1;
        for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)&rangep->pst_inner_rel,
             (i4)BITS_IN(rangep->pst_inner_rel))) >= 0;)
        {
            i4      new_count;
            new_count = global->ops_goj.opl_view->opl_gojt[ojid];
            if (best_count == new_count)
                opx_error(E_OP0398_OJ_SCOPE); /* joinid's must be nested so
                                            ** this situation should not occur */
            if ((joinid == PST_NOJOIN)
                ||
                (best_count > new_count))
            {
                best_count = new_count;
                joinid = ojid;
            }
        }
    }
    if (BTtest((i4)joinid, (char *)&rangep->pst_outer_rel))
    {   /* since this relation is also outer to the joinid it must
        ** be a full join so the view qualification cannot be
        ** inserted directly into the tree.  A new joinid needs to
        ** be invented to contain the qualification */
        OPL_IOUTER          newjoinid;
        PST_RNGENTRY        **basepp; /* ptr to base variable which may
                                    ** be within the context of original
                                    ** view */
        OPL_PARSER          *pinner;
        OPV_IGVARS          view_count;

        if (!global->ops_goj.opl_fjview)
        {   /* allocate and initialize joinid array if first time */
            i4  fjsize;

            fjsize = sizeof(OPL_IOUTER) * PST_NUMVARS;
            global->ops_goj.opl_fjview = (OPL_GOJT *)opu_memory(global, fjsize); 
            MEfill(fjsize, (u_char)0, (PTR)global->ops_goj.opl_fjview);
        }
        if (global->ops_goj.opl_fjview->opl_gojt[viewid] < 1)
        {   /* create a new view id, so full join semantics are preserved */
            newjoinid = ++global->ops_goj.opl_glv;
            global->ops_goj.opl_fjview->opl_gojt[viewid] = newjoinid;
        }
        else
            newjoinid = global->ops_goj.opl_fjview->opl_gojt[viewid];
        basepp = global->ops_qheader->pst_rangetab;
        view_count = 0;
        pinner = subquery->ops_oj.opl_pinner;
        for (; basepp != endpp; basepp++, view_count++)
        {   /* update joinid bitmaps for all variables defined in the
            ** context of this view */
            if (*basepp
                &&
                ((*basepp)->pst_rgtype != PST_UNUSED)
                &&
                ((*basepp)->pst_rgparent >= 0))
            {
                PST_RNGENTRY    *view2p;

                for(view2p = (*basepp); view2p->pst_rgparent >= 0; 
                    view2p = global->ops_qheader->pst_rangetab[view2p->pst_rgparent])
                {   /* if base var is in scope of view which is being substituted then
                    ** mark the base variable as being an inner to this new join id */
                    if (original == view2p->pst_rgparent)
                    {
                        BTset((i4)newjoinid, (char *)&pinner->opl_parser[view_count]);
                        break;
                    }
                }

            }
        }
        return (newjoinid);
    }
    else
        return(joinid);
}

/*{
** Name: opa_ojsub      - substitute joinid into sub-tree
**
** Description:
**      The view variable was substituted into an expression 
**      which is part of an outer join ID, so the outer join ID
**      needs to be proprogated
**
** Inputs:
**      gstate                          aggregate state variable
**      nodep                           ptr to node being substituted
**
** Outputs:
**
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      9-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opa_ojsub(
        PST_J_ID            joinid,
        PST_QNODE           *nodep)
{
    for (;nodep; nodep = nodep->pst_left)
    {
        switch (nodep->pst_sym.pst_type)
        {
        case PST_ROOT:
        case PST_AGHEAD:
        case PST_SUBSEL:
            return;         /* root nodes have their own scope */
        case PST_AND:
        case PST_OR:
        case PST_UOP:
        case PST_BOP:
        case PST_AOP:
        case PST_COP:
            if (nodep->pst_sym.pst_value.pst_s_op.pst_joinid == PST_NOJOIN)
                nodep->pst_sym.pst_value.pst_s_op.pst_joinid = joinid;
        default:
            ;
        }
        if (nodep->pst_right)
            opa_ojsub(joinid, nodep->pst_right);
    }
}

/*{
** Name: opa_qview      - add view qualifications to subquery
**
** Description:
**      Add the qualification of the all views to the subquery
**
** Inputs:
**      subquery                        subquery being processed
**         ->ops_agg.opa_views          bitmap of views to add
**
** Outputs:
**      subquery->ops_root              qualification added to root
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-mar-86 (seputis)
**          initial creation
**      28-aug-89 (seputis)
**          fix b7772 view substitution lookup
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      13-may-91 (seputis)
**          fix for b37469 - moved some code from opa_uview here
**      06-jun-96 (kch)
**          We now set gstate->opa_joinid to OPL_NOINIT, to indicate that it
**          has not been initialized. This change fixes bugs 76337.
**      1-aug-97 (inkdo01)
**          Set joinid of introduced (linking) AND to same as what's under it.
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**	29-aug-08 (hayke02)
**	    Move the fix for bug 76337 to before the IF, so that the ELSE will
**	    also have a gstate->opa_joinid of OPL_NOINIT. This change fixes
**	    bug 120633.
[@history_template@]...
*/
static VOID
opa_qview(
        OPA_RECURSION      *gstate)
{
    OPV_IGVARS      viewvar;        /* global var representing view */
    OPV_GBMVARS     (*qviewmap);    /* ptr to map of views to add */
    OPS_STATE       *global;        /* ptr to OPF global state */
    OPS_SUBQUERY    *subquery;

    subquery = gstate->opa_gfather;
    qviewmap = &subquery->ops_agg.opa_views;
    global = subquery->ops_global;

    for (viewvar = -1;
         (viewvar = BTnext((i4)viewvar, 
                            (char *)qviewmap, 
                            (i4)BITS_IN(OPV_GBMVARS)
                           )
         )
         >= 0;
        )
    {
        PST_QNODE           *viewlist;
        PST_QNODE           *qlend;
        i4                  nestcount;
        PST_QNODE           **nodepp;
        PST_QNODE           *root;
        PST_RNGENTRY        *rangep;
        PST_J_ID            joinid = PST_NOJOIN;

        rangep = global->ops_qheader->pst_rangetab[viewvar];
        root = rangep->pst_rgroot;
        if ((root->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
            &&
            (root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
            )
        {
            PST_QNODE   *agheadp;
            PST_QNODE   *qual;
            agheadp = (PST_QNODE *)NULL;
            opa_agview(subquery->ops_global, &agheadp, 
                &root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
                root->pst_right);           /* get the bylist of the
                                            ** aggregate */
            opa_agview(subquery->ops_global, &agheadp, 
                &root->pst_sym.pst_value.pst_s_root.pst_tvrm, 
                root->pst_left);            /* get the bylist of the
                                            ** aggregate */
#ifdef    E_OP0280_SCOPE
            /* scope consistency check */
            if (!agheadp)
                opx_error( E_OP0280_SCOPE); /* if some base relations
                                        ** are already in the bview map
                                        ** then there is a conflict 
                                        ** between views */
#endif
            viewlist = agheadp->pst_left->pst_left; /* get target list of aggregate */
            qual = agheadp->pst_right;
            if (qual
                &&
                (qual->pst_sym.pst_type != PST_QLEND)
                &&
                !BTtest((i4)viewvar, (PTR)&subquery->ops_agg.opa_aview))
            {   /* the assumption is that if an aggregate has been substituted
                ** into the query by query mod, then the duplicate semantics will be
                ** correct, given that the by list is brought to the top,... however
                ** if no aghead is found then the where clause of the aggregate must
                ** be querymoded in order to remove the partitions which have no
                ** tuples */
                OPV_GBMVARS         *oldmap1;
                i4                  nest1count;
                opv_copytree(subquery->ops_global, &qual);
                oldmap1 = gstate->opa_gvarmap;
                nest1count = gstate->opa_nestcount;
                gstate->opa_gvarmap = (OPV_GBMVARS *)&subquery->ops_root->
                    pst_sym.pst_value.pst_s_root.pst_rvrm;
                gstate->opa_demorgan = FALSE;   /* just in case */
                /* 76337 & 120633 - joinid has not been inited, mark it
                ** as such */
                gstate->opa_joinid = OPL_NOINIT;
                if (!subquery->ops_root->pst_right
                    ||
                    (subquery->ops_root->pst_right->pst_sym.pst_type == PST_QLEND))
                {
                    subquery->ops_root->pst_right = qual;
                    opa_generate(gstate, &subquery->ops_root->pst_right);
                                                /* traverse the new sub tree */
                }
                else
                {
                    PST_QNODE       *and_node;
                    and_node = opv_opnode(global, PST_AND, (ADI_OP_ID)0, 
                        (OPL_IOUTER)PST_NOJOIN);
                    and_node->pst_left = subquery->ops_root->pst_right;
                    and_node->pst_right = qual;
                    subquery->ops_root->pst_right = and_node;
                    opa_generate(gstate, &and_node->pst_right);
                                                /* traverse the new sub tree */
                }
                gstate->opa_gvarmap = oldmap1;
                subquery->ops_agg.opa_nestcnt += gstate->opa_nestcount - nest1count; 
                                                /* set number of
                                                ** aggregates in the subtree 
                                                ** rooted by this aggregate 
                                                ** node */
            }
        }
        viewlist = root->pst_right;
        if ((!viewlist) || (viewlist->pst_sym.pst_type == PST_QLEND))
        {
            if (global->ops_qheader->pst_numjoins > 0)
                (VOID) opa_fojid(gstate, rangep, viewvar); /* find
                                            ** joinid of the parent query, relabel
                                            ** the range table to identify the join
                                            ** variables, since it is important to
                                            ** keep the scope of the join correct
                                            ** in order to process FULL joins
                                            ** properly */
            gstate->opa_joinid = OPL_NOINIT;
            continue;
        }
        opv_copytree(subquery->ops_global, &viewlist);
        /* place qualification at end of target list so that the left to
        ** right traversal of the query tree will pick this up */
        if (global->ops_qheader->pst_numjoins > 0)
        {
            gstate->opa_joinid = opa_fojid(gstate, rangep, viewvar); /* find
                                        ** joinid of the parent query */
            if (gstate->opa_joinid >= 1)
                opa_ojsub(gstate->opa_joinid, viewlist); /* substitute all uninitialized
                                        ** joinids to be as required by the
                                        ** usage of the parent query */
            joinid = gstate->opa_joinid;  /* save for linking AND, too */
        }
        gstate->opa_joinid = OPL_NOINIT;

        for(qlend = subquery->ops_root;
            qlend->pst_right
            &&
            (qlend->pst_right->pst_sym.pst_type == PST_AND);
            qlend = qlend->pst_right)
            ;                               /* find end of view list */
        if (qlend->pst_right
            &&
            (qlend->pst_right->pst_sym.pst_type == PST_QLEND))
        {
            qlend->pst_right = viewlist;        /* attach the new qual list */
            nodepp = &qlend->pst_right;
        }
        else
        {   /* not in conjunctive form so add to top of list */
            PST_QNODE       *and_node;
            and_node = opv_opnode(global, PST_AND, (ADI_OP_ID)0,
                (OPL_IOUTER)joinid); /* use parser outer join operator
                                        ** since OPF joinids have not been
                                        ** substituted yet */
            and_node->pst_left = subquery->ops_root->pst_right;
            and_node->pst_right = viewlist;
            subquery->ops_root->pst_right = and_node;
            nodepp = &and_node->pst_right;
        }
        {
            OPV_GBMVARS     *oldmap;

            nestcount = gstate->opa_nestcount;
            oldmap = gstate->opa_gvarmap;
            gstate->opa_gvarmap = 
                (OPV_GBMVARS *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_rvrm;
            gstate->opa_demorgan = FALSE;   /* just in case */
            opa_generate(gstate, nodepp); /* traverse the new sub tree */
            gstate->opa_gvarmap = oldmap;
        }

        subquery->ops_agg.opa_nestcnt += gstate->opa_nestcount - nestcount; /* set number of
                                            ** aggregates in the subtree 
                                            ** rooted by this aggregate 
                                            ** node */
    }
    MEfill(sizeof(OPV_GBMVARS), 0, (char *)&subquery->ops_agg.opa_views);
                                            /* re-init after processing so that
                                            ** subselect can be flattened */
}

/*{
** Name: opa_sview      - substitute view variable
**
** Description:
**      Routine will lookup the view and substitute the expression for
**      the view varnode
**
** Inputs:
**      global                          ptr to global state variable
**      viewpp                          ptr to var node for view attribute
**
** Outputs:
**      viewpp                          a copy of the view attribute expression
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-mar-87 (seputis)
**          initial creation
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      13-apr-93 (andre)
**          if presented with t reference to TID pof a base table through an
**          updatable view defined over it, simply translate reference to the
**          TID of the view to a reference to the TID of the view's underlying
**          table or view
**      26-apr-93 (ed)
**          use symbolic name for tid attribute
**	22-mar-10 (smeke01) b123457
**	    opu_qtprint signature has changed as a result of trace point op214.
[@history_line@]...
[@history_template@]...
*/
static VOID
opa_sview(
        OPS_STATE          *global,
        PST_QNODE          **viewpp)
{
    OPV_IGVARS      viewvar;        /* global var representing view */
    PST_QNODE       *expr;          /* use to find the correct view expression
                                    */
    DB_ATT_ID       viewattr;       /* attribute of view variable */

if (opt_strace(global->ops_cb, OPT_F001_DUMP_QTREE))
      opu_qtprint(global, *viewpp, (char *)NULL);
    viewattr.db_att_id
        = (*viewpp)->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;

    viewvar = (*viewpp)->pst_sym.pst_value.pst_s_var.pst_vno;

    /* referencing TID */
    if (viewattr.db_att_id == DB_IMTID)
    {
        /*
        ** translate reference to the TID of the view to a reference to the TID
        ** of the view's underlying table or view
        */
        (*viewpp)->pst_sym.pst_value.pst_s_var.pst_vno =
            BTnext((i4)-1, 
                (char *)&global->ops_qheader->pst_rangetab[viewvar]
                    ->pst_rgroot->pst_sym.pst_value.pst_s_root.pst_tvrm, 
                (i4)BITS_IN(global->ops_qheader->pst_rangetab[viewvar]
                    ->pst_rgroot->pst_sym.pst_value.pst_s_root.pst_tvrm));

        return;
    }
    
    for (expr = global->ops_qheader->pst_rangetab[viewvar]->pst_rgroot->pst_left;
        expr->pst_sym.pst_value.pst_s_rsdm.pst_rsno != viewattr.db_att_id;
        expr = expr->pst_left)
        ;                           /* search for attribute in resdom list of
                                    ** the view */

    if (expr->pst_right->pst_sym.pst_type == PST_VAR)
    {   /* copy the var node if possible */
        MEcopy((PTR)expr->pst_right, 
            (sizeof(PST_VAR_NODE)+sizeof(PST_SYMBOL)-sizeof(PST_SYMVALUE)),
            (PTR)(*viewpp));
    }
    else
    {   /* copy the expression */
        *viewpp = expr->pst_right;
        opv_copytree(global, viewpp);
    }
}

/*{
** Name: opa_ojattr     - check for constant expression
**
** Description:
**      traverse query tree and determine if a constant expression exists
**      making sure that simple aggregates are considered to be constants
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      root                            ptr to query tree to traverse
**
** Outputs:
**
**      Returns:
**          TRUE if constant expression is found
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      5-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opa_ojattr(
        OPS_SUBQUERY       *subquery,
        PST_QNODE          *root)
{
    for (;root; root = root->pst_left)
    {
        switch (root->pst_sym.pst_type)
        {
        case PST_VAR:
            return(FALSE);      /* if a var node is found then
                                ** constant expression problem can be
                                ** delayed to query mod of this relation */
                                /* FIXME, expressions which convert
                                ** variables with NULL into non-null values
                                ** could have problems */
        case PST_AGHEAD:
            return(root->pst_left->pst_sym.pst_type == PST_AOP); /* an
                                ** PST_AOP implies a simple aggregrate which
                                ** implies a constant */
        case PST_ROOT:
        case PST_SUBSEL:
            return(TRUE);       /* subselect have their own scope so do not
                                ** look past this point */
        default:
            break;
        }
        if (root->pst_right
            &&
            !opa_ojattr(subquery, root->pst_right))
            return(FALSE);      /* recurse down right side and return
                                ** if constant expression is not found */
    }
    return(TRUE);
}

/*{
** Name: opa_ojconstattr        - look for the constant attributes in query tree
**
** Description:
**      This routine will traverse the tree in which the view is defined and
**      search for attributes which are referenced which are used in the 
**      query tree and are in the constant attr bitmap.  This implies that
**      view cannot be flattened since NULL attributes may be involved. 
**      FIXME - this routine can probably be smarter and determine if
**      null semantics would be affected, OR perhaps a new constant view
**      can be created which can represent the constant attributes only
**      and the remainder of the view can be substituted.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      viewvar                         var number of view to check
**      attrmap                         ptr to map of constant attributes in view
**      root                            ptr to query tree to traverse
**
** Outputs:
**
**      Returns:
**          TRUE - if view cannot be flattened due to outer join semantics
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      9-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opa_ojconstattr(
        OPS_SUBQUERY       *subquery,
        OPV_IGVARS         viewvar,
        OPZ_BMATTS         *attrmap,
        PST_QNODE          *root)
{
    for (;root; root = root->pst_left)
    {
        switch (root->pst_sym.pst_type)
        {
        case PST_VAR:
        {
            return((viewvar == root->pst_sym.pst_value.pst_s_var.pst_vno)
                &&
                BTtest((i4)root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id,
                    (char *)attrmap));  /* return TRUE if a constant attribute
                                    ** is referenced */
        }
        default:
            break;
        }
        if (root->pst_right
            &&
            opa_ojconstattr(subquery, viewvar, attrmap, root->pst_right))
            return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: opa_ojview     - check for constant expression in view target list
**
** Description:
**      The routine will determine if a constant expression exists in the 
**      view target list and if so, if it is used in the query tree which 
**      the view will be substituted.  If this is the case and the view
**      is involved in an outer join, then do not flatten the view since 
**      it may lose NULL semantics when handling the constant expressions 
**      since no reference is made to a base variable 
**          select v1.const from (r left join v1 on v1.const=r.a1)
**      should return NULL for tuples of r which do not match the constant
**      attribute in v1.const... which would not be possible if this
**      query was flattened.
**
** Inputs:
**      subquery                        ptr to scope of view being substituted
**      viewvar                         global variable number of view
**      root                            parse tree which defines scope
**
** Outputs:
**
**      Returns:
**          TRUE - if view should not be substituted
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      6-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opa_ojview(
        OPS_SUBQUERY       *subquery,
        OPV_IGVARS         viewvar,
        PST_QNODE          *root)
{
    /* Traverse the target list of the view and get map of attributes
    ** which are constant expressions */
    bool            one_found;      /* TRUE is at least one constant expression
                                    ** is found */
    OPZ_BMATTS      attrmap;        /* bit map of resdom numbers which
                                    ** are constant expressions in the view */
    PST_QNODE       *viewrootp;

    MEfill(sizeof(attrmap), (u_char)0, (PTR)&attrmap);
    viewrootp = subquery->ops_global->ops_qheader->pst_rangetab[viewvar]->pst_rgroot;
    one_found = FALSE;
    {
        PST_QNODE       *resdomp;
        for (resdomp = viewrootp->pst_left;
            resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
            resdomp = resdomp->pst_left)
        {
            if (opa_ojattr(subquery, resdomp->pst_right))
            {
                one_found = TRUE;
                BTset((i4)resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno,
                    (char *)&attrmap);
            }
        }
    }

    /* if there are any constant expressions then traverse the
    ** query tree to see if these attributes are visible */
    if (one_found)
    {
        one_found = opa_ojconstattr(subquery, viewvar, &attrmap, root);
    }
    return(one_found);
}

/*{
** Name: opa_pview      - update variable bitmaps for view processing
**
** Description:
**      Update bitmaps used to process view and determine the correct 
**      duplicate handling.
**
** Inputs:
**      subquery                        subquery being processed
**      gvar                            global range var representing view
**      viewid                          the initial call to opa_view requires
**                                      a unique viewid
**      base_view                       TRUE - if the parent from-list occurred
**                                      in a view definition
**                                      FALSE - if the parent from-list occurred
**                                      somewhere in the query, i.e. not the
**                                      in a view query tree in the range table
**
** Outputs:
**      subquery->ops_agg.opa_view      view bitmap
**      subquery->ops_agg.opa_bview     base relations in view
**      subquery->ops_agg.opa_uview     views needing target list copied
**                                      used for views with distinct
**      subquery->ops_keeptids          base relations needing tids brought up
**                                      used for view with keep duplicates
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      13-mar-87 (seputis)
**          initial creation
**      7-sep-88 (seputis)
**          do not process union views twice
**      27-sep-88 (seputis)
**          set opa_uview for union views
**      3-mar-89 (seputis)
**          update to use new interface for opa_rnode for 300 columns
**      3-jun-89 (seputis)
**          fix b6177 b6178 b6179, keep duplicate semantics for function aggregate views
**      13-oct-89 (seputis)
**          fix for b8160 added viewid for duplicate semantics with quel aggregates
**      14-nov-89 (seputis)
**          fix b8700 - remove view expansion if view is in aggregate
**      24-jan-90 (seputis)
**          fix b4555 - union views should not be removed from the from list
**      1-jun-90 (seputis)
**          OPC process the entire union view once the initial subquery is
**          found, so all simple aggregates etc. for all partitions need
**          to be evaluated before the first partition
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      11-dec-91 (seputis)
**          fix bug 41179 - remove duplicates for nested views
**      18-sep-92 (ed)
**          bug 43852 - make sure internal maps updated but avoid processing
**          union view definition more than once.
**      2-apr-93 (ed)
**          b49513 - fix constant view with no flattening flag problem, i.e.
**          an SEjoin needs at least one outer relation so do not merge the view
**          in this case
**      7-dec-93 (ed)
**          b56139 - mark UNION ALL views in which projection actions 
**          can be made
**      15-jan-94 (ed)
**          - reset local outer join maps for view variable being substituted
**          since the global range variable may be subsequently allocated for
**          an aggregate temporary
**      26-jan-95 (inkdo01)
**          - temporarily disabled OPV_UVPROJECT because it is incompatible with
**          a solitary COUNT(*) (bug 66500). Hopefully someday we'll figure out 
**          a more precise solution which will allow selective use of UVPROJECT
**	12-jul-99 (hayke02)
**	    If a constant view (!pst_tvrm) involved in an outer join is
**	    found we now do not flatten this view. This prevents incorrect
**	    rows from an outer join query involving a constant view where
**	    no attributes from the const view are included in the select
**	    list (and so opa_ojview() returns FALSE). This change fixes bug
**	    97371.
**      16-dec-03 (hayke02)
**	    We now do not flatten group'ed by aggregate views which are
**	    involved in outer joins. This change fixes problems INGSRV 1556,
**	    1949 and 2587 (bugs 105847, 108953 and 108119).
**	27-apr-04 (hayke02)
**	    We now do not flatten the view if we find it contains an outer join 
**	    with an ifnull() on an attribute which is from an inner relation in
**	    the OJ. this change fixes problem INGSRV 2671, bug 111627.
**	20-apr-05 (inkdo01)
**	    Fix previous fix to recognize 128 variable range table (must use
**	    BTcount on pst_j_mask's).
**	12-sep-05 (hayke02)
**	    Back out the fix for bug 111627 by commenting out setting
**	    oj_ifnull_view TRUE. This change fixes bugs 114912 and 115165.
**	    Also, remove the spurious declaration of the boolean oj_gby_aview
**	    from the cross integration of the fix for 111627.
**	19-jan-06 (dougi)
**	    Re-added declaration of oj_gby_aview which was still being 
**	    referenced. There was likely some confusion over 2.5 and 2.6 diffs.
**	29-dec-2006 (dougi)
**	    Add support for derived tables and WITH list elements.
**	 9-nov-07 (hayke02)
**	    If we have an unflattened union view containing only 'UNION'
**	    partitions, and it is involved in an outer join, ensure duplicates
**	    are removed when the temporary table is created by setting
**	    ops_duplicates to OPS_DREMOVE for the OPS_VIEW new_subquery. This
**	    prevents incorrect results when the top sort fails to remove
**	    duplicate rows containing NULLs from the outer join(s). This change
**	    fixes bug 119031.
**	14-Dec-2009 (drewsturgiss)
**		Altered the tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
*/
static VOID
opa_pview(
        OPA_RECURSION      *gstate,
        OPS_SUBQUERY       *subquery,
        OPV_IGVARS         gvar,
        bool               nondistinct,
        OPA_VID            viewid,
        bool               base_view,
        PST_QNODE          *root)
{
    PST_QNODE           *viewp;     /* ptr to root node of view */
    bool                distinctview; /* TRUE - if view removes duplicates 
                                    ** and if the caller does not */
    PST_QNODE           **unionpp;  /* used to traverse union nodes */
    OPS_STATE           *global; /* OPF global state variable */
    bool                union_view; /* TRUE - if this is a union view */
    bool		mixture;    /* TRUE - if mixture of union all and
				    ** union are used */
    bool		oj_view = FALSE; /* view involved in an OJ */
    bool		oj_ifnull_view = FALSE; /* view contains an OJ and an
						** ifnull on an inner - don't
						** flatten */
    bool		oj_gby_aview = FALSE;	/*group'ed by aggregate view in
						** an OJ - don't flatten */
    OPV_IGVARS		child; /* var referenced by view */
    OPV_GBMVARS		(*from_map);
    OPV_GBMVARS		ojvarmap;
    PST_QNODE		*resdomp;

    global = subquery->ops_global;
    unionpp = &global->ops_qheader->pst_rangetab[gvar]->pst_rgroot;/*
                                    ** used to traverse the linked list of union
                                    ** parse trees */
    viewp = *unionpp;

    if (subquery->ops_oj.opl_pinner
	&&
	((BTnext((i4)-1, (char *)&subquery->ops_oj.opl_pinner->opl_parser[gvar],
	    (i4)BITS_IN(subquery->ops_oj.opl_pinner->opl_parser[gvar])) >= 0)
	||
	(BTnext((i4)-1, (char *)&subquery->ops_oj.opl_pouter->opl_parser[gvar],
	    (i4)BITS_IN(subquery->ops_oj.opl_pouter->opl_parser[gvar])) >= 0)
	))
	oj_view = TRUE;

    if (oj_view
	&&
	(viewp->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
	&&
	(viewp->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL))
	oj_gby_aview = TRUE;

    from_map = (OPV_GBMVARS *)&viewp->pst_sym.pst_value.pst_s_root.pst_tvrm;
    MEfill(sizeof(ojvarmap), (u_char)0, (PTR)&ojvarmap);

    /* fill ojvarmap with vars that are inners in OJs */
    for (child = -1;
	(child = BTnext((i4)child, (char *)from_map,
					    (i4)BITS_IN(OPV_GBMVARS))) >= 0;)
    {
	if (BTcount((char *)&global->ops_qheader->pst_rangetab[child]->
		pst_inner_rel.pst_j_mask, OPV_MAXVAR) != 0)
	    BTset((i4)child, (char *)&ojvarmap); 
    }

    /* search for a PST_VAR from ojvarmap under an ifnull() */
    for (resdomp = viewp->pst_left;
	BTcount((char *)&ojvarmap, OPV_MAXVAR) > 0 && resdomp &&
		(resdomp->pst_sym.pst_type == PST_RESDOM);
	resdomp = resdomp->pst_left)
    {
	PST_QNODE	*nodep;

	for (nodep = resdomp->pst_right; nodep; nodep = nodep->pst_left)
	{
	    PST_QNODE		*ifnullp;
	   
	    if ((nodep->pst_sym.pst_type == PST_BOP) &&
		(nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
		& ADI_F32768_IFNULL))
	    {
		for (ifnullp = nodep->pst_left; ifnullp;
						    ifnullp = ifnullp->pst_left)
		{
		    PST_QNODE		*bopp;
		    if ((ifnullp->pst_sym.pst_type == PST_VAR) &&
		    BTtest((i4)ifnullp->pst_sym.pst_value.pst_s_var.pst_vno,
							    (char *)&ojvarmap))
		    {
			/*oj_ifnull_view = TRUE;*/
			break;
		    }
		    if (ifnullp->pst_sym.pst_type == PST_BOP)
		    {
			for (bopp = ifnullp->pst_right; bopp;
							bopp = bopp->pst_left)
			{
			    if ((bopp->pst_sym.pst_type == PST_VAR) &&
				BTtest((i4)bopp->pst_sym.pst_value.pst_s_var.pst_vno, (char *)&ojvarmap))
			    {
				/*oj_ifnull_view = TRUE;*/
				break;
			    }
			}
			if (oj_ifnull_view)
			    break;
		    }
		}
	    }
	    if (oj_ifnull_view)
		break;
	}
	if (oj_ifnull_view)
	    break;
    }

    mixture = FALSE;
    if (nondistinct)
    {   /* may need need special processing to keep duplicate semantics correct
        ** for SQL after query mod substitution,
        ** - make sure that opa_uviews is set for union views, but do not
        ** set other bitmaps for union views since union views cannot be
        ** substituted */
        mixture = 
            (   viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next
                &&
                (viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS)
                /* since this is a union view, the duplicate semantics is
                ** defined by pst_union, note that the first implementation
                ** assumes this field is the same for all members of the union
                ** note that pst_s_root.pst_dups defines the duplicate semantics
                ** for only that component of the union view */
            );

	distinctview = 
	    (	!viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next
		&&
		(   (viewp->pst_sym.pst_value.pst_s_root.pst_dups == PST_NODUPS)
		    /* if this is not a union view then the duplicate semantics for
		    ** this view is defined by the above field */
		    ||
		    (	(viewp->pst_sym.pst_value.pst_s_root.pst_mask1 & PST_3GROUP_BY)
			&&
			(viewp->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
			&&
			!oj_gby_aview
		    )
		    /* this means that there is an aggregate in the view which causes
		    ** duplicates to be removed, fix SQL only for now FIXME quel */
		)
	    )
	    ||
	    mixture;

        while (viewp = viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
        {   /* all elements of union view need to be distinct to allow the optimizer
            ** to move duplicate elimination to the parent... otherwise sorts are
            ** needed on "sub view" to remove duplicates prior to allowing tuples
            ** to be available to parent, i.e. the view is considered a union all
            ** of "sub views" which have duplicates removed */
            bool        removedups;

            removedups = (viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS);
            if (viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
            {
                mixture |= removedups;
                distinctview &= removedups;
            }
        }
        mixture = (!distinctview && mixture);
        viewp = *unionpp;
        if (distinctview)
        {   /* duplicates are removed in view but not in parents so mark
            ** bitmap indicating that target list of view must be copied to
            ** the parent target list, as non-printing resdoms to preserve
            ** duplicates */
            subquery->ops_duplicates = OPS_DREMOVE; /* remove duplicates now
                                    ** that view processing requires that
                                    ** duplicates on the target list be
                                    ** unique */
            subquery->ops_agg.opa_tidactive = TRUE; /* need to sort to remove
                                    ** duplicates in this view */
            BTset((i4)gvar, (char *)&subquery->ops_agg.opa_uviews); 
        }
    }
    else
    {
        if (subquery->ops_root->pst_sym.pst_type == PST_AGHEAD && !base_view)
            subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_dups = PST_NODUPS;
        distinctview = TRUE;        /* fix bug 41179, nested views should
                                    ** inherit correct duplicate mode
                                    ** - caller removes duplicates so it
                                    ** is not required that any children
                                    ** of this view do special processing */
    }
    union_view = viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next != (PST_QNODE *)NULL;
    if (union_view
        ||
        (
            /* if this view is involved in an outer join then check that
            ** none of the attributes are constant expressions since then the
            ** NULL semantics of the outer join could not be enforced, in this
            ** case do not flatten the view */
	    oj_view
            &&
            (
                viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next
                ||
                BTcount((char *)&viewp->pst_sym.pst_value.pst_s_root.pst_tvrm,
			OPV_MAXVAR) == 0 /* const view */
                ||
                opa_ojview(subquery, gvar, root)
		||
		oj_gby_aview
            )
        )
	||
	(   /* if flattening is turned off and a constant view exists, with
	    ** the possibility of an SEjoin then do not flatten the constant
	    ** view since it may be needed as the left hand operand of an
	    ** SEjoin if the from list becomes empty, b49513 */
	    (global->ops_gmask & OPS_QUERYFLATTEN) /* is flattening turned
				    ** off? */
	    &&
	    BTcount((char *)&viewp->pst_sym.pst_value.pst_s_root.pst_tvrm,
				OPV_MAXVAR) == 0 /* constant view */
	    &&
	    global->ops_qheader->pst_subintree /* is there a
				    ** subselect in the query, which will require
				    ** an SEjoin */
	)
	||
	oj_ifnull_view
        )
    {   /* if unions exist in view then it cannot be substituted
        ** so create a global range variable for it */
        OPS_SUBQUERY    *new_subquery; /* assign a new subquery structure for
                                    ** beginning of the union view */
        OPA_RECURSION   tempstate;  /* do not modify the state of the parent */
        OPS_SUBQUERY    *uvp;       /* used to mark the end of the union view
                                    ** list */
        OPS_SUBQUERY    *agg_list;  /* list of aggregates needed to evaluate
                                    ** at least one partition of the view */
        OPS_SUBQUERY    **end_agg_listpp;  /* end of list of aggregates needed to evaluate
                                    ** at least one partition of the view */
        OPV_GRV         *gvarp;

        subquery->ops_sunion.opu_mask |= OPU_REFUNION;  /* remember that a union
                                    ** view was found in this subquery */
        if (base_view)
            BTset((i4)gvar, (char *)&subquery->ops_agg.opa_bviews); /* fix b4555
                                    ** union views will appear as a "base relation"
                                    ** after view substitution has occurred so that
                                    ** it is not removed from the from list */
        if (global->ops_rangetab.opv_base->opv_grv[gvar])
            return;                 /* the global range variable has already been
                                    ** processed for the union view so just return
                                    ** and do not reprocess */
        agg_list = (OPS_SUBQUERY *) NULL;
        end_agg_listpp = &agg_list;
        (VOID)opv_parser(global, gvar, OPS_VIEW, 
            FALSE,                  /* no RDF info */
            FALSE,                  /* do not look in PSF range table */
            TRUE);                  /* abort if an error occurs */
        gvarp = global->ops_rangetab.opv_base->opv_grv[gvar];
        gvarp->opv_gvid = viewid; /*
                                    ** store the viewid so that any reference
                                    ** to the view in quel link-backs can be
                                    ** handled correctly for duplicates */
        if (viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_next
            &&
            (viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_ALLDUPS))
        {   /* mark union view as one in which a projection can be made since duplicates
            ** need to be kept,... if duplicates are removed then all attributes
            ** of view are copied to parent target list anyways */
            /* the UVPROJECT optimization conflicts with the use of count(*) above
            ** the UNION ALL. The combination of the dropped projections and 
            ** COUNT(*) seems to leave it with no output eqclasses at all, leading
            ** to a consistency check. The better solution would seem to be to
            ** avoid UVPROJECT when under a COUNT(*), but I'm guessing we're at
            ** a different level of recursion here. So the trick will be to 
            ** determine when we're under a COUNT(*). Until we figure out how to do
            ** that, UVPROJECT will be put on the shelf. */
         /* gvarp->opv_gmask |= OPV_UVPROJECT; */
        }

        new_subquery = NULL;
        uvp = global->ops_subquery; /* this marks the last subquery
                                        ** which was not part of the union
                                        ** view */
        for ( ;*unionpp;
             unionpp = &(*unionpp)->pst_sym.pst_value.pst_s_root.
                pst_union.pst_next)
        {   
            PST_QNODE   *nextunion;     /* temp to save next union
                                        ** subtree, to prevent recursion
                                        ** of union processing at lower
                                        ** levels */
            OPS_SUBQUERY *union_subquery; /* subquery representing the current
                                        ** union being processed */

            nextunion = (*unionpp)->pst_sym.pst_value.pst_s_root.pst_union.
                pst_next;
            (*unionpp)->pst_sym.pst_value.pst_s_root.pst_union.pst_next = NULL; 
                                        /* do not recurse at lower 
                                        ** level */
            if ((gvarp->opv_gmask & OPV_UVPROJECT)
                &&
                ((*unionpp)->pst_sym.pst_value.pst_s_root.pst_union.pst_dups != PST_ALLDUPS)
                &&
                nextunion               /* if there is no next union then pst_dups is
                                        ** not applicable */
                )
            {   /* this is not a totally UNION ALL view so eliminate it from
                ** the projection optimization */
                gvarp->opv_gmask &= ~OPV_UVPROJECT;
            }
            tempstate.opa_global = global;
            tempstate.opa_gfather = (OPS_SUBQUERY *)NULL;
            tempstate.opa_gvarmap = (OPV_GBMVARS *)NULL;
            tempstate.opa_demorgan = FALSE;
            tempstate.opa_gaggregate = FALSE;
            tempstate.opa_gflatten = TRUE;
            tempstate.opa_nestcount = 0;
            tempstate.opa_ojbegin = (PST_QNODE *)NULL;
            tempstate.opa_ojend = &tempstate.opa_ojbegin;
            tempstate.opa_joinid = OPL_NOINIT;
            tempstate.opa_simple = (OPS_SUBQUERY *)NULL;
            tempstate.opa_uvlist = (OPS_SUBQUERY *)NULL;
            opa_rnode(&tempstate, unionpp); /* unionpp should be a root node */
            union_subquery = tempstate.opa_newsqptr;
            if (global->ops_subquery != union_subquery)
            {   /* move any aggregates etc. from the list until
                ** the entire union has been calculated */
                OPS_SUBQUERY        *next_partition;
                for (next_partition = global->ops_subquery; 
                    next_partition->ops_next != union_subquery;
                    next_partition = next_partition->ops_next)
                    ;
                *end_agg_listpp = global->ops_subquery;
                end_agg_listpp = &next_partition->ops_next;
                global->ops_subquery = union_subquery;
            }
            if (tempstate.opa_simple || tempstate.opa_uvlist)
                opx_error(E_OP0285_UNIONAGG); /* FIXME - may need to get smarter
                                        ** about processing union views defined
                                        ** on union views, by carefully placing
                                        ** opa_uvlist into parent list */
            (*unionpp)->pst_sym.pst_value.pst_s_root.pst_union.pst_next 
                = nextunion;            /* restore the union link */
            union_subquery->ops_gentry = gvar; /* global range variable
                                        ** associated with union view */
            gvarp->opv_gquery = union_subquery;
            if (union_view && mixture)
                union_subquery->ops_mask |= OPS_SUBVIEW; /* if there is a
                                        ** possibility of a mixture of union
                                        ** and unionall then mark this */
            if (!new_subquery)
            {   /* first time set up a VIEW subquery */
                new_subquery = union_subquery;
                new_subquery->ops_sqtype = OPS_VIEW;
		if (!mixture && oj_view &&
		    viewp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups ==
								    PST_NODUPS)
		    new_subquery->ops_duplicates = OPS_DREMOVE;
            }
            else
            {
                new_subquery->ops_union = union_subquery;
                new_subquery->ops_union->ops_sqtype = OPS_UNION; /* the head
                                        ** of this linked list will be
                                        ** OPS_VIEW */
                new_subquery = union_subquery;
                global->ops_rangetab.opv_base->opv_grv[gvar]->opv_gquery = new_subquery;
            }
        }
        if (gvarp->opv_gmask & OPV_UVPROJECT)
        {   /* if this is a UNION ALL view then enable the projection optimization */
            gvarp->opv_attrmap = (OPZ_BMATTS *)opu_memory(global, 
                (i4)sizeof(*(gvarp->opv_attrmap)));
            MEfill(sizeof(*(gvarp->opv_attrmap)), (u_char)0,
                (PTR)gvarp->opv_attrmap);
        }

        if (agg_list)
        {   /* save all aggregates needed to evaluate this union view */
            global->ops_subquery->ops_sunion.opu_agglist = agg_list;
            *end_agg_listpp = (OPS_SUBQUERY *)NULL;
        }
        subquery->ops_global->ops_union = TRUE; /* union views need to be 
                                        ** processed */
        {
            /* need to move the union view subqueries so that they will be
            ** optimized prior to any references, this means optimizing
            ** all union views first, prior to any normal query, and when
            ** nested union views are involved make sure that the child
            ** is always optimized first... if we do not do this it really
            ** does not cause a problem until "set qep" is executed since
            ** sometimes with aggregate column names being printed it
            ** looks at the union view, and the union view may be
            ** optimized after the aggregate which means the union view
            ** structures are not initialized as in:
            ** set qep
            ** select sum(id),name from unionview group by name;
            */
            OPS_SUBQUERY        **endlistpp; /* find the end of the current
                                        */
#if 0
            for (endlistpp = &global->ops_astate.ops_uview; *endlistpp;
               endlistpp = &(*endlistpp)->ops_next);
#endif
            /* changed from using opa_uview since if the opa_rnode resets
            ** this value so use local variables i.e. gstate instead */
            for (endlistpp = &gstate->opa_uvlist; *endlistpp;
                endlistpp = &(*endlistpp)->ops_next);
                ;                       /* look for the end of the current
                                        ** list of union view subqueries
                                        ** and their children... since the
                                        ** children may have already been
                                        ** processed, any new union views need
                                        ** to be placed at the end of the list
                                        ** for nesting purposes */
            *endlistpp = global->ops_subquery; /* place the union view subquery list
                                        ** at the end */
            for (endlistpp = &global->ops_subquery;
                *endlistpp != uvp;
                endlistpp = &(*endlistpp)->ops_next)
                ;                       /* look for the end of the subquery list
                                        ** for this union view */
            *endlistpp = (OPS_SUBQUERY *) NULL; /* terminate the list */
            global->ops_subquery = uvp; /* remove all references of
                                        ** the union view for now */
        }
        return;
    }
    BTset ((i4)gvar, (char *)&subquery->ops_agg.opa_views); /* define a view
                                    ** for this subquery */

    {	/* traverse from list of view and process range variables in view */
	from_map = (OPV_GBMVARS *)&viewp->pst_sym.pst_value.pst_s_root.pst_tvrm;

        for (child = -1;
             (child = BTnext((i4)child, 
                                (char *)from_map, 
                                (i4)BITS_IN(OPV_GBMVARS)
                               )
             )
             >= 0;
            )
        {
            if (subquery->ops_oj.opl_pinner)
            {
                PST_J_MASK      *target;
                target = &subquery->ops_oj.opl_pouter->opl_parser[child];
                MEcopy((PTR)&subquery->ops_oj.opl_pouter->opl_parser[gvar],
                    sizeof(*target), (PTR)target);
                BTor((i4)BITS_IN(*target),
                    (char *)&global->ops_qheader->pst_rangetab[child]->pst_outer_rel,
                    (char *)target);
                target = &subquery->ops_oj.opl_pinner->opl_parser[child];
                MEcopy((PTR)&subquery->ops_oj.opl_pinner->opl_parser[gvar],
                    sizeof(*target), (PTR)target);
                BTor((i4)BITS_IN(*target),
                    (char *)&global->ops_qheader->pst_rangetab[child]->pst_inner_rel,
                    (char *)target);
            }
            if ((   global->ops_qheader->pst_rangetab[child]->pst_rgtype
                    ==
                    PST_RTREE ||
		    global->ops_qheader->pst_rangetab[child]->pst_rgtype
		    == PST_DRTREE ||
		    global->ops_qheader->pst_rangetab[child]->pst_rgtype
		    == PST_WETREE
                )
                )
            {   /* view has been found so update bitmaps used to
                ** process views */
                opa_pview(gstate, subquery, child, !distinctview, viewid, TRUE, viewp);
            }
            else
            {   /* real relation has been found  */
                (VOID) opv_parser(global, child, OPS_MAIN, TRUE, TRUE, TRUE);
#ifdef    E_OP0280_SCOPE
                /* scope consistency check */
                if (BTtest((i4)gvar, (char *)&subquery->ops_agg.opa_bviews))
                    opx_error( E_OP0280_SCOPE); /* if some base relations
                                            ** are already in the bview map
                                            ** then there is a conflict 
                                            ** between views */
#endif
                global->ops_rangetab.opv_base->opv_grv[child]->opv_gvid = viewid;
                BTset((i4)child, (char *)&subquery->ops_agg.opa_bviews);
                if (!distinctview)
                    BTset((i4)child, (char *)&subquery->ops_agg.opa_keeptids);
                                        /* may need tids from base relation
                                        ** since all parents are nondistinct*/
            }
        }
        if (subquery->ops_oj.opl_pinner)
        {   /* outer join in query */
            MEfill(sizeof(subquery->ops_oj.opl_pouter->opl_parser[gvar]),
                (u_char)0, (PTR)&subquery->ops_oj.opl_pouter->opl_parser[gvar]);
            MEfill(sizeof(subquery->ops_oj.opl_pinner->opl_parser[gvar]),
                (u_char)0, (PTR)&subquery->ops_oj.opl_pinner->opl_parser[gvar]);
                                        /* initialize since global variable may
                                        ** later be used for aggregate */
        }
    }
}

/*{
** Name: opa_cunion     - create union view from main query
**
** Description:
**      This routine will convert the main query into a union view.  The
**      union view will be read into the sorter in order to remove duplicates 
**      from the union.
**
** Inputs:
**      global                          ptr to global state variable
**      agg_qnode                       ptr to main query parse tree
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      30-apr-87 (seputis)
**          initial creation
**      8-aug-88 (seputis)
**          copy all resdoms fields for first member of union view into main query
**              for OPC resdom naming
**          initial all resdoms for OPC to use PST_ATTNO
**          create union views, even for UNION ALL since the mechanism works
**              FIXME do not create a union view for UNION ALL if possible
**      9-may-89 (seputis)
**          remove reference to pst_rtuser, since field has been redefined
**      25-jan-90 (seputis)
**          -fix b9474
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      6-june-97 (inkdo01 for hayke02)
**          When setting gvar, we now check that the ops_rangetab.opv_base
**          does not already use this element and increment gvar if this
**          is the case. This occurs for a 'create table ... as select
**          ... union select ... union all select ...' style query. This
**          change fixes bug 82637.
[@history_line@]...
[@history_template@]...
*/
OPV_IGVARS
opa_cunion(
        OPS_STATE          *global,
        PST_QNODE          **rootpp,
        bool               copytarget)
{
    OPV_IGVARS          gvar;       /* parser range variable which is not used*/
    PST_QNODE           *rootp;     /* ptr to root node */
    PST_QNODE           *mainp;     /* ptr to newly created parse tree for main
                                    ** query */
    PST_QNODE           *unionp;    /* ptr to root node of remaining unions to
                                    ** relabel */
    PST_RNGENTRY        *uvp;       /* range table entry to be used for
                                    ** union view */
    bool                removedups; /* true only if all partitions require
                                    ** duplicates to be removed i.e. only "union"
                                    ** but not any "union all"s */
    rootp = *rootpp;
#if 0
    if(rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups != PST_NODUPS)
    {                               /* if duplicates need to be removed then
                                    ** create view, otherwise do nothing, FIXME
                                    ** need to handle multiple combinations
                                    ** of UNION, and UNION ALL */
        do not need to create a union view for non-distinct queries but do
        it anyways for UNION ALL since that mechanism works
    }
#endif
    for (gvar = 0; 
         (gvar < PST_NUMVARS) 
         && 
         (gvar < global->ops_qheader->pst_rngvar_count);
        gvar++)
    {
        uvp = global->ops_qheader->pst_rangetab[gvar];
        if (!uvp || (uvp->pst_rgtype == PST_UNUSED))
            break;                  /* search for an unused parser range 
                                    ** variable */
    }
    if (gvar >= PST_NUMVARS)
        opx_error(E_OP0202_VAR_OVERFLOW);   /* report error if no parser range
                                            ** variables are available */
    if (gvar == global->ops_qheader->pst_rngvar_count)
    {   /* create new array of pointers since one more element needed */
        PST_RNGENTRY        **tempptr;
        /*
        ** Bug 82637 -  Check for 'create table ... as select ... union ...
        **              select ... union all select ...' style query.
        */
        if (global->ops_rangetab.opv_base->opv_grv[gvar])
            gvar++;
        tempptr = (PST_RNGENTRY **)opu_memory(global, 
            (i4)(gvar+1) * sizeof(PST_RNGENTRY *));
        MEcopy((PTR) global->ops_qheader->pst_rangetab, 
            (gvar * sizeof(PST_RNGENTRY *)),
            (PTR)tempptr);
        global->ops_qheader->pst_rangetab = tempptr; /* FIXME - try to
                                            ** reuse memory lost here */
        global->ops_qheader->pst_rangetab[gvar] = uvp = NULL;
        global->ops_qheader->pst_rngvar_count++;
    }
    if (!uvp)
    {   /* allocate new range table entry */
        global->ops_qheader->pst_rangetab[gvar] =
        uvp = (PST_RNGENTRY *)opu_memory(global, (i4)sizeof(PST_RNGENTRY));
        MEfill(sizeof(PST_RNGENTRY), 0, (char *)uvp);
    }
    global->ops_gmask |= OPS_UV;            /* indicate this is an artifical
                                            ** union view */
    mainp = *rootpp = (PST_QNODE *) opu_memory(global, (i4) sizeof(PST_QNODE));
    mainp->pst_right = opv_qlend(global);

    mainp->pst_sym.pst_type = PST_ROOT;     /* new root node */
    /* pst_dataval is not defined for root node */
    MEfill(sizeof(mainp->pst_sym.pst_value.pst_s_root.pst_tvrm), (u_char)0, 
        (PTR)&mainp->pst_sym.pst_value.pst_s_root.pst_tvrm);
    BTset((i4)gvar, (char *)&mainp->pst_sym.pst_value.pst_s_root.pst_tvrm); /*
                                            ** set global var for view in from
                                            ** list of query */
    mainp->pst_sym.pst_value.pst_s_root.pst_qlang =
        rootp->pst_sym.pst_value.pst_s_root.pst_qlang; /* same query language
                                            ** as main query */
    mainp->pst_sym.pst_value.pst_s_root.pst_project = FALSE;
    mainp->pst_sym.pst_value.pst_s_root.pst_union.pst_next = NULL;

    uvp->pst_rgtype = PST_RTREE;
    uvp->pst_rgroot = rootp;                /* use the
                                            ** main query tree as the definition
                                            ** for this view */
    MEfill(sizeof(uvp->pst_corr_name), (u_char)' ', (PTR)&uvp->pst_corr_name);
    MEcopy((PTR)"union", 5, (PTR)&uvp->pst_corr_name);
    removedups = (rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_dups == PST_NODUPS);
    unionp = rootp->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
    /* rootp->pst_sym.pst_type = PST_SUBSEL;*/   /* keep as a root node since union views
                                            ** are also like this, b9474 convert main query parse tree
                                            ** to a select node */
    opa_ctarget(global, rootp->pst_left, &mainp->pst_left, gvar, copytarget);
    for (;unionp; unionp = unionp->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {   /* Traverse the remaining resdoms in the remaining union
        ** phrases, and relabel to PST_ATTNO for OPC */
        PST_QNODE           *resdomp;
        if (unionp->pst_sym.pst_value.pst_s_root.pst_union.pst_next) 
            removedups &= (rootp->pst_sym.pst_value.pst_s_root.
                pst_union.pst_dups == PST_NODUPS); /* last partition dup flag is
                                            ** undefined */
        for (resdomp = unionp->pst_left; 
            resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
            resdomp = resdomp->pst_left)
        {
            resdomp->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;  /* relabel
                                            ** this interior node for OPC */
        }
    }
    mainp->pst_sym.pst_value.pst_s_root.pst_dups = removedups; /* if there is at least
                                            ** one union all, then sub "union views" will
                                            ** eventually be created to eliminate duplicates
                                            ** prior to evaluating this "union all" view */
    return(gvar);
}

/*{
** Name: opa_grtable    - process from list for global range table
**
** Description:
**      Routine which will process a from list for a subquery.  This from 
**      list may be supplied by an internal subquery which is being 
**      flattened into the main query.
**
** Inputs:
**      new_subquery                    subquery to be associated with
**                                      the from list
**      from_list                       ptr to from list to be processed
**      duplicates                      determines whether duplicates should
**                                      be kept, if so, then TIDs, and view
**                                      target lists may need to be copied
**                                      to the subquery target list
**      varmap                          ptr to bitmap to be updated, AND
**                                      means that this is a query which is
**                                      being flatten... NULL means this is a
**                                      normal query and update the bitmap in
**                                      root node 
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-jul-87 (seputis)
**          initial creation
**      4-dec-88 (seputis)
**          added varmap to ensure proper bitmap is updated for flattening
**          passed in parse tree root node instead of from list for flattening
**      4-may-89 (seputis)
**          add opg_scount fix b5582
**      8-may-89 (seputis)
**          make flattening the default
**      17-aug-89 (seputis)
**          added viewmap to fix duplicate handling problem in particular the
**          following query failed
**          create view supdistinct (sno,status,city)
**              as select distinct s.sno, s.status, s.city
**              from s where s.status > 15;
**              \p\g
**              select count(sp.sno), count(distinct sp.sno) from supdistinct,sp
**                      where sp.sno=supdistinct.sno
**                      group by supdistinct.city
**              \p\g
**      12-mar-90 (seputis)
**          - fix b20034, a view qualification was not being querymoded
**          since it did not appear in target list
**
**          create table b20034a (name c10, age integer);\p\g
**          insert b20034a set (name=c10, age=integer) values('1',1);
**          insert b20034a set (name=c10, age=integer) values('2',2);
**          create view b20034v as select a.name, a.age, b.salary
**                  from b20034a a, b20034a b
**                  where a.name=b.name;\p\g
**          select * from b20034v;\p\g
**          select count(name) from b20034v;\p\g
**          select count(*) from b20034v;\p\g
**
**      26-feb-90 (seputis)
**          - b35862 - modified for new range table entry
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**	29-dec-2006 (dougi)
**	    Add support for derived tables and WITH list elements.
**	18-april-2008 (dougi)
**	    Add support for table procedures.
**	21-aug-2008 (dougi)
**	    Tiny fix for agg queries with tprocs.
**	2-Jun-2009 (kschendel) b122118
**	    Quel vs sql viewmap code ended up being identical.  Combine.
*/
static VOID
opa_grtable(
        OPA_RECURSION      *gstate,
        PST_QNODE          *root,
        OPS_DUPLICATES     duplicates,
        bool               newmap,
        OPV_GBMVARS        *viewmap,
        bool               orflag)
{   /* an SQL query may have a variable in the from list
    ** but not in the query, so define all global range variables
    ** in the from list here */
    OPS_SUBQUERY       *new_subquery;
    OPV_IGVARS          from_grv;
    OPS_STATE           *global;
    OPV_GBMVARS         *from_map;
    bool                initoj;         /* true if outer joins exist */
    bool                remove_oj;      /* true if outer join masks should
                                        ** be used to strip parents of selected
                                        ** outer join IDs */
    PST_J_MASK          temp_ojmask;    /* outer mask valid if remove_oj true*/
    PST_J_MASK          temp_ijmask;    /* inner mask valid if remove_oj true*/

    new_subquery = gstate->opa_gfather;
    from_map = (OPV_GBMVARS *)&root->pst_sym.pst_value.pst_s_root.pst_tvrm;
    global = gstate->opa_global;
    initoj = global->ops_qheader->pst_numjoins > 0;
    remove_oj = initoj && newmap && gstate->opa_gfather && gstate->opa_gaggregate &&
        gstate->opa_gfather->ops_agg.opa_father; /* TRUE if duplicate outer join
                            ** IDs made exist in the parent due to PSF/OPF
                            ** quel variable map interface, so parent query
                            ** should have outer join ID's stripped */
    if (remove_oj)
    {
        MEfill(sizeof(temp_ojmask), (u_char)0, (char *)&temp_ojmask);
        MEfill(sizeof(temp_ijmask), (u_char)0, (char *)&temp_ijmask);
    }
    for (from_grv = -1;
         (from_grv = BTnext((i4)from_grv, 
                            (char *)from_map, 
                            (i4)BITS_IN(OPV_GBMVARS)
                           )
         )
         >= 0;
        )
    {
        PST_RNGENTRY    *rangep;
        rangep = global->ops_qheader->pst_rangetab[from_grv];
        if (initoj)
        {
            MEcopy((PTR)&rangep->pst_outer_rel,
                sizeof(new_subquery->ops_oj.opl_pouter->opl_parser[0]),
                (PTR)&new_subquery->ops_oj.opl_pouter->opl_parser[from_grv]);
            MEcopy((PTR)&rangep->pst_inner_rel,
                sizeof(new_subquery->ops_oj.opl_pinner->opl_parser[0]),
                (PTR)&new_subquery->ops_oj.opl_pinner->opl_parser[from_grv]);
            if (remove_oj)
            {   /* aggregate from list is copied into the parent from list
                ** so eliminate outer join definitions in aggregate since
                ** this operation is not applicable to parent */
                BTor(BITS_IN(temp_ijmask), (char *)&new_subquery->ops_oj.opl_pinner
                    ->opl_parser[from_grv], (char *)&temp_ijmask);
                BTor(BITS_IN(temp_ojmask), (char *)&new_subquery->ops_oj.opl_pouter
                    ->opl_parser[from_grv], (char *)&temp_ojmask);
            }
            else if (!newmap)
            {   /* this is a subselect which is being flattened into the parent query
                ** so determine the outer join IDs to be associated with the tables
                ** in the subselect from list */
                OPV_IGVARS      view_var;
                for (view_var = rangep->pst_rgparent;
                    view_var >= 0 
                        && 
                    BTtest((i4)view_var, (char *)&new_subquery->ops_agg.opa_views); /* make
                                        ** view was flattened and not part of a non-flattened
                                        ** aggregate */
                    view_var = global->ops_qheader->pst_rangetab[view_var]->pst_rgparent)
                {
                    BTor(BITS_IN(new_subquery->ops_oj.opl_pouter->opl_parser[0]),
                        (char *)&global->ops_qheader->pst_rangetab[view_var]->pst_outer_rel,
                        (char *)&new_subquery->ops_oj.opl_pouter->opl_parser[from_grv]);
                    BTor(BITS_IN(new_subquery->ops_oj.opl_pinner->opl_parser[0]),
                        (char *)&global->ops_qheader->pst_rangetab[view_var]->pst_inner_rel,
                        (char *)&new_subquery->ops_oj.opl_pinner->opl_parser[from_grv]);
                }
            }
        }
        if (rangep->pst_rgtype
            ==
            PST_RTREE ||
	    rangep->pst_rgtype
	    == PST_DRTREE ||
	    rangep->pst_rgtype
	    == PST_WETREE
            )
        {   /* view has been found so update bitmaps used to
            ** process views , bitmaps are opa_uviews, opa_views 
            ** opa_bviews, and ops_keeptids */
            opa_pview(gstate, new_subquery, from_grv, duplicates == OPS_DKEEP, 
                    ++global->ops_astate.opa_vid, FALSE, root);
        }
        else
        {
            /* normal (i.e. non-view) range variable found */
            (VOID) opv_parser(global, from_grv, OPS_MAIN, TRUE, TRUE, TRUE); 
                                    /* init the global range table info */
            if (duplicates == OPS_DKEEP)
                BTset((i4)from_grv, 
                    (char *)&new_subquery->ops_agg.opa_keeptids);
                                    /* may need tids from base relation
                                    ** since all parents are nondistinct*/

	    /* Table procedures need new subqueries and other details. */
	    if (rangep->pst_rgtype == PST_TPROC)
	    {
		OPV_GRV	*gvarp = global->ops_rangetab.opv_base->
							opv_grv[from_grv];
        	OPV_GBMVARS *tvarmap;   
		bool	save_gaggregate;

		/* OPS_SUBQUERY	*tprocsq; skip this next bit

		tprocsq = opa_initsq(global, &rangep->pst_rgroot, 
							(OPS_SUBQUERY *) NULL);
		tprocsq->ops_sqtype = OPS_TPROC;
		tprocsq->ops_gentry = from_grv;
		gvarp->opv_gmask |= OPV_TPROC;
		opa_gsub(tprocsq);	** attach OPV_GVAR and OPS_SUBQUERY */

		if (newmap)
		{
		    tvarmap = gstate->opa_gvarmap;
		    gstate->opa_gvarmap = &root->pst_sym.pst_value.
							pst_s_root.pst_rvrm;
		}
		save_gaggregate = gstate->opa_gaggregate;
		gstate->opa_gaggregate = FALSE;		/* disable */
		opa_generate(gstate, &rangep->pst_rgroot);
		gstate->opa_gaggregate = save_gaggregate; /* restore */
		if (newmap)
		    gstate->opa_gvarmap = tvarmap;
	    }
        }
    }

    if (BTcount((char *)&new_subquery->ops_agg.opa_views, OPV_MAXVAR))
    {
        OPV_GBMVARS     tempmap;
#ifdef    E_OP0280_SCOPE
        /* scope consistency check */

        MEcopy((char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_tvrm, sizeof(OPV_GBMVARS), (char *)&tempmap);
        BTand(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_bviews,
                (char *)&tempmap);
        if (BTcount((char *)&tempmap, OPV_MAXVAR))
            opx_error( E_OP0280_SCOPE); /* if some base relations
                                    ** are already in the from list
                                    ** then there is a conflict */
#endif
        BTor(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_views,
                                        (char *)from_map);
        BTor(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_bviews,
                                        (char *)from_map);
                                    /* modify the from 
                                    ** list of the subquery to 
                                    ** reflect the view
                                    ** substitutions which will be
                                    ** made - this will be used
                                    ** to enforce proper scoping
                                    ** of correlated variables */
        MEfill(sizeof(OPV_GBMVARS), 0, 
                (char *)&new_subquery->ops_agg.opa_bviews); 
                                    /* reset variable so that it
                                    ** does not conflict with view
                                    ** substitution, and flattened queries
                                    ** b5655 */

        BTor(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_views,
                        (char *)&global->ops_astate.opa_allviews);
                                    /* keep track of all 
                                    ** views referenced in query
                                    ** so far */
	/* FIXME huh?  we just put these in, above?? */
	BTclearmask(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_views,
			(char *)from_map);
                                    /* remove views from the from list */
    }
    if (!newmap)
    {   /* only need to execute this in case of query flattening */
#ifdef    E_OP0280_SCOPE
        OPV_GBMVARS     tempmap;
        MEcopy((char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_tvrm, sizeof(tempmap), (char *)&tempmap);
        BTand(OPV_MAXVAR, (char *)from_map, (char *)&tempmap);
        if (BTcount((char *)&tempmap, OPV_MAXVAR))
            opx_error( E_OP0280_SCOPE); /* expecting corelation
                                    ** in SQL constructs only */
                                /* FIXME - this check and the following statement
                                ** may not be necessary since when opa_flatten 
                                ** calls this routine, it already has combined
                                ** the from lists, check this assertion */
#endif
        BTor(OPV_MAXVAR, (char *)from_map,
            (char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm);
                                /* add new variables to from list from flattened
                                ** query */
        BTor(OPV_MAXVAR, (char *)from_map, 
            (char *)&new_subquery->ops_agg.opa_flatten);
                            /* save the relations which were flattened
                            ** into the parent query, since these should
                            ** not be used when determining whether an aggregate
                            ** is a correlated variable */
    }
    /* all the view and from list bitmaps have been set, but since the
    ** simple aggregate from list is "copied" in the parent by PSF
    ** we need to find out what these are prior to adding view qualifications
    ** and target list items since the variables may be removed
    ** e.g. 
    ** create table a (a int);\p\g
    ** create table b (b int);\p\g
    ** create view v (v) as select a from a union select b from b;\p\g
    ** select count(*) from v;\p\g
    ** - from list of the main query needs to be reset to empty prior to
    ** copying the target list
    */
    {
        /* find all aggregates/subselects in subtree below this node 
        ** - update the var bit maps so that corelated variables can be
        ** determined */
        i4            nestcount;        /*number of aggregates in this subtree */
        OPV_GBMVARS    (*tvarmap);   
        PST_J_ID       old_joinid;      /* save joinid for parent */

        if (newmap)
        {
            tvarmap = gstate->opa_gvarmap;
            gstate->opa_gvarmap = &root->pst_sym.pst_value.pst_s_root.pst_lvrm;
        }
        nestcount = gstate->opa_nestcount;
        new_subquery->ops_vmflag = TRUE; /* set prior to traversing tree so that
                                        ** it can be reset in cases where
                                        ** maps become invalid, normally it
                                        ** will be true after the first
                                        ** traversal */
        old_joinid = gstate->opa_joinid;
        if (root->pst_left)
        {
            gstate->opa_demorgan = FALSE;   /* just in case */
            gstate->opa_joinid = OPL_NOINIT;
            opa_generate(gstate, &root->pst_left); /* traverse left sub tree  */
        }
        if (newmap)
            gstate->opa_gvarmap = &root->pst_sym.pst_value.pst_s_root.pst_rvrm;
        if (root->pst_right)
        {
            gstate->opa_demorgan = FALSE;   /* just in case */
            gstate->opa_joinid = OPL_NOINIT;
            opa_generate(gstate, &root->pst_right); /* traverse right sub tree*/
        }
        if (newmap)
            gstate->opa_gvarmap = tvarmap;      /* restore old map */
        gstate->opa_joinid = old_joinid;

        new_subquery->ops_agg.opa_nestcnt +=gstate->opa_nestcount - nestcount; 
                                        /* set number of
                                        ** aggregates in the subtree 
                                        ** rooted by this aggregate 
                                        ** node */
    }
    if (newmap)                         /* this would only be NULL for a
                                        ** non-flattened query */
    {
        OPV_GBMVARS     fromlist;       /* variables which have not been
                                        ** inserted just because they
                                        ** belonged to an aggregate */

	MEcopy((char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                        sizeof(fromlist), (char *)&fromlist);
	BTor(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_rview, (char *)&fromlist);
	BTor(OPV_MAXVAR, (char *)&new_subquery->ops_agg.opa_views, (char *)&fromlist);
                                        /* fix b20034 - need to
                                        ** querymod all views even if not referenced
                                        ** in query tree */
	BTclearmask(OPV_MAXVAR, (char *)&new_subquery->ops_aggmap, (char *)&fromlist);
	BTor(OPV_MAXVAR, (char *)&new_subquery->ops_root->pst_sym.pst_value.
			pst_s_root.pst_lvrm, (char *)&fromlist);
	BTor(OPV_MAXVAR, (char *)&new_subquery->ops_root->pst_sym.pst_value.
			pst_s_root.pst_rvrm, (char *)&fromlist);
                                        /* b8700 demonstrated that views should not
                                        ** be querymoded if they appear in aggregates
                                        ** since the GROUP BY restrictions of SQL
                                        ** will allow the aggregate to optimized
                                        ** the parent variables */

	BTclearmask(OPV_MAXVAR, (char *)&new_subquery->ops_aggmap,
                (char *)&new_subquery->ops_agg.opa_uviews);   /* need
                                        ** to apply duplicate handling rules unless this
                                        ** from list was copied by PSF aggregate handling */
        if (BTcount((char *)&new_subquery->ops_agg.opa_uviews, OPV_MAXVAR))
            opa_uview(gstate);          /* this routine will copy 
                                        ** target list of views which
                                        ** require special duplicate
                                        ** handling
                                        ** to the current subquery as
                                        ** non-printing resdoms */
        BTand(OPV_MAXVAR, (char *)&fromlist, (char *)&new_subquery->ops_agg.opa_views);
        MEcopy((char *)&new_subquery->ops_agg.opa_views, sizeof(*viewmap),
                        (char *)viewmap);
        if (BTcount((char *)&new_subquery->ops_agg.opa_views, OPV_MAXVAR))
            opa_qview(gstate);          /* add qualifications to the 
                                        ** subquery */
        if (!(global->ops_gmask & OPS_QUERYFLATTEN)
            &&
            new_subquery->ops_agg.opa_corelated
            )
            opa_agbylist( new_subquery, FALSE, !orflag);
    }
    if (remove_oj)
    {   /* remove duplicated outer join indicators from parent, which is caused
        ** by the quel like interface causing variable to appear in both
        ** aggregate and parent from list */
        OPL_PARSER      *pinnerp;
        OPL_PARSER      *pouterp;
        pinnerp = gstate->opa_gfather->ops_agg.opa_father->ops_oj.opl_pinner;
        pouterp = gstate->opa_gfather->ops_agg.opa_father->ops_oj.opl_pouter;
        for (from_grv = OPV_MAXVAR; --from_grv >= 0;)
        {
	    BTclearmask((i4)BITS_IN(temp_ojmask), (char *)&temp_ojmask,
                (char *)&pouterp->opl_parser[from_grv]);
	    BTclearmask((i4)BITS_IN(temp_ijmask), (char *)&temp_ijmask,
                (char *)&pinnerp->opl_parser[from_grv]);
        }
    }
}

/*{
** Name: opa_rnode      - process a root node
**
** Description:
**      Procedure to create a subquery and initialize it.
**
** Inputs:
**      global                          ptr to global state variable
**      agg_qnode                       ptr to ptr to root node to process
**      father                          subquery struct containing this
**                                      root node, or NULL if main query
**
** Outputs:
**      newsqptr                        ptr to newly initialized subquery
**                                      struct for this root node
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      23-mar-87 (seputis)
**          initial creation
**      24-may-89 (seputis)
**          saved opa_viewid to fix b5655
**      17-aug-89 (seputis)
**          added viewmap to fix duplicate handling problem
**      7-sep-89 (seputis)
**          added ops_agg.opa_flatten to help detect correlated aggregates properly
**      29-mar-90 (seputis)
**          unix porting change so that non-byte aligned machines work
**      12-jun-90 (seputis)
**          fix shape query causing OPF scoping error
**      11-oct-91 (seputis)
**          fix bug 40049 , incorrect duplicate handling
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**      16-dec-03 (hayke02)
**	    Changes so that aggregate views, unflattened because they are
**	    involved in outer joins, are now dealt with correctly by the 'for'
**	    loop which adds the union views. This change fixes problem INGSRV
**	    2604 (bug 111315), and also stops the fix for 105847 et al in
**	    opa_pview() returning E_OP0681.
**	10-jan-2007 (dougi)
**	    Fix bad "sizeof" on range variable bit map.
*/
static VOID
opa_rnode(
        OPA_RECURSION       *gfather,   /* ptr to state of father subquery */
        PST_QNODE          **agg_qnode  /* ptr to ptr to query tree node to be 
                                        ** analyzed
                                        */ )
{
/* allocate a new subquery node and place on the subquery list
** so that the final ordering will be innermost first so that
** if this is the evaluation order then the values will be
** ready once the substitution occurs
** - initialize all components of the subquery node
*/
    OPS_STATE       *global;    /* state variable used for memory management*/
    OPA_RECURSION    gstate;    /* every root node should have its' own state
                                ** to isolate it from other nodes, these variables
                                ** could be passed as parameters, but the goal
                                ** is to reduce recursive stack space usage */
    register PST_QNODE  *q;     /* query tree node to be analyzed */
    OPS_SUBQUERY   *new_subquery;/* used to create a new subquery node */
    bool            is_sql;     /* TRUE if an SQL root */
    OPS_SUBQUERY       *father; /* ptr to outer aggregate of this node or
                                ** NULL if "outer aggregate" is root node
                                */
    OPS_SQTYPE      sqtype;
    OPV_GBMVARS     viewmap;    /* map of views which were substituted in the
                                ** from list */
    global = gfather->opa_global;
    father = gfather->opa_gfather;
    if (!father 
        && 
        (*agg_qnode)->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {   /* if this main query has a union node then create a "union view" 
        ** out of the main query... make the main query a scan of this view 
        ** which will be used to read tuples into the sorter.  Unions need
        ** to remove duplicates */
        (VOID)opa_cunion( global, agg_qnode, TRUE);
    }
    q = *agg_qnode;
    q->pst_sym.pst_value.pst_s_root.pst_rtuser = global->ops_astate.opa_viewid;
                                        /* if this root node was created due
                                        ** to view substitution then mark the
                                        ** node with the view id */
    gfather->opa_newsqptr =
    new_subquery = opa_initsq( global, agg_qnode, father); /* allocate
                                        ** and initialize
                                        ** a new subquery element */
    if (gfather->opa_joinid == OPL_NOINIT)
        new_subquery->ops_oj.opl_aggid = PST_NOJOIN;
    else
        new_subquery->ops_oj.opl_aggid = gfather->opa_joinid;

    new_subquery->ops_next = global->ops_subquery; /* link into list*/
    global->ops_subquery = new_subquery; /* the new sub-query must
                                        ** be evaluated before all 
                                        ** others which have been 
                                        ** created thus far */
    if (father && (global->ops_astate.opa_viewid >= 0))
        BTset((i4)global->ops_astate.opa_viewid, (PTR)&father->ops_agg.opa_aview);
                                        /* this gives the parent a map of which
                                        ** aggregates found in its' scope, which will
                                        ** determine the type of querymod operation
                                        ** used to keep duplicate semantics correct */
    sqtype = new_subquery->ops_sqtype;
    {
        /* initialize the OPA_RECURSION parameter for this subquery */
        gstate.opa_global = global;     /* ptr to state of query processing */
        gstate.opa_gfather = new_subquery; /* ptr to outer aggregate of this node or
                                ** NULL if "outer aggregate" is root node
                                */
        gstate.opa_gvarmap = (OPV_GBMVARS *)NULL; /* ptr to var bitmap 
                                ** on left or right side
                                ** which needs to be updated
                                */
        gstate.opa_demorgan = FALSE;/* boolean indicating demorgan's
                                ** law should be applied */
        gstate.opa_gflatten = TRUE; /* TRUE if node is not below an OR, which means
                                ** a projection is required */
        gstate.opa_nestcount = 0; /* number of inner aggregates found
                                ** within this tree */
        gstate.opa_newsqptr = (OPS_SUBQUERY *)NULL;  /* temp variable to reduce recursion 
                                ** memory requirements - not really needed */
        gstate.opa_simple = (OPS_SUBQUERY *)NULL;   /* list of simple
                                ** non-correlated aggregates */
        gstate.opa_uvlist = (OPS_SUBQUERY *)NULL;   /* list of union view
                                ** definitions and related subqueries */
        gstate.opa_gaggregate = /* TRUE if the father subquery is an aggregate
                                */
            (sqtype == OPS_RFAGG)
            || 
            (sqtype == OPS_HFAGG)
            || 
            (sqtype == OPS_FAGG)
            ||
            (sqtype == OPS_RSAGG) /* simple aggregates can
                                        ** have resdoms added for 
                                        ** duplicate semantics */
            ||
            (sqtype == OPS_SAGG);
        gstate.opa_ojbegin = (PST_QNODE *)NULL;
        gstate.opa_ojend = &gstate.opa_ojbegin;
        gstate.opa_joinid = PST_NOJOIN;
    }

    opa_grtable(&gstate, new_subquery->ops_root,
        new_subquery->ops_duplicates, TRUE, &viewmap, gfather->opa_gflatten);
                                        /* process from list for views
                                        ** and setup global range table
                                        ** elements */
   if (gstate.opa_ojbegin)
    {   /* attach outer join conjuncts to main query after recursion since
        ** ops_root cannot be changed until then */
        *(gstate.opa_ojend) = new_subquery->ops_root->pst_right;
        new_subquery->ops_root->pst_right = gstate.opa_ojbegin;
    }
    {	/* order the subqueries so that simple aggregates occur before
	** any union view definition which occur before any other
	** subquery queries */
	OPS_SUBQUERY	    *sq_ptr;
	OPS_SUBQUERY	    **uvpp;
	OPS_SUBQUERY	    *savep;
	if (gfather->opa_uvlist
	    &&
            (!BTcount((char *)&new_subquery->ops_correlated, OPV_MAXVAR))
	    &&
	    (new_subquery->ops_sqtype == OPS_SAGG)
	    )
	{   /* since simple aggregates and their parent queries share the from
	    ** list, union views which should be evaluated for the simple
	    ** aggregate, actually get evaluated for the parent query, so
	    ** these need to be extracted */
	    {
		OPV_GBMVARS	*fromlistp;
		OPV_IGVARS	gvar;
		fromlistp = &new_subquery->ops_root->pst_sym.
					pst_value.pst_s_root.pst_tvrm;
		for (gvar = -1; (gvar = BTnext((i4)gvar, 
			(char *)fromlistp, (i4)BITS_IN(*fromlistp)))>=0;)
		{
		    for (uvpp = &gfather->opa_uvlist; *uvpp; uvpp = &(*uvpp)->ops_next)
			if ((*uvpp)->ops_gentry == gvar)
			    break;
		    if (*uvpp)
		    {	/* found union view which belongs in this simple aggregate */
			OPS_SUBQUERY	*enduvp;
			for (enduvp = *uvpp; enduvp->ops_next && 
				(enduvp->ops_next->ops_gentry == gvar); 
			    enduvp = enduvp->ops_next)
			    ;
			savep = *uvpp;
			*uvpp = enduvp->ops_next;   /* from union view from parent list */
			enduvp->ops_next = gstate.opa_uvlist;
			gstate.opa_uvlist = savep;  /* add to the simple aggregate list */
		    }
		}
	    }
	}
	if (gstate.opa_uvlist)
	{   /* add union views */
	    for (uvpp = &gstate.opa_uvlist; *uvpp; uvpp = &(*uvpp)->ops_next)
	    {
		if ((*uvpp)->ops_sunion.opu_agglist)
		{   /* place into list of subqueries for the union view */
		    savep = *uvpp;
		    *uvpp = (*uvpp)->ops_sunion.opu_agglist;
		    for (sq_ptr = *uvpp; sq_ptr->ops_next; sq_ptr = sq_ptr->ops_next)
			;
		    sq_ptr->ops_next = savep;
		    (*uvpp)->ops_sunion.opu_agglist = (OPS_SUBQUERY *)NULL;
		    uvpp = &sq_ptr->ops_next;
		}
		if ((*uvpp)->ops_next == NULL)
		    break;
	    }
	    (*uvpp)->ops_next = global->ops_subquery;
	    global->ops_subquery = gstate.opa_uvlist;
	}
	if (gstate.opa_simple)
	{   /* add simple aggregates */
	    for (sq_ptr = gstate.opa_simple; sq_ptr->ops_next; sq_ptr = sq_ptr->ops_next)
		;
	    sq_ptr->ops_next = global->ops_subquery;
	    global->ops_subquery = gstate.opa_simple;
	}
    }
    sqtype = new_subquery->ops_sqtype;
    is_sql = q->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL;

    if (!new_subquery->ops_agg.opa_tidactive)
    {   /* init the keeptid map unless view processing has already set
        ** it */
        if (is_sql)
            MEcopy((char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_tvrm, sizeof(OPV_GBMVARS),
                (char *)&new_subquery->ops_agg.opa_keeptids); /* only look
                                    ** at from list since left and
                                    ** right bit maps will contain
                                    ** corelated variables which
                                    ** should not be in the map */
        else
        {
            MEcopy((char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_tvrm, sizeof(OPV_GBMVARS),
                (char *)&new_subquery->ops_agg.opa_keeptids);
            BTor(OPV_MAXVAR, (char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_lvrm, (char *)&new_subquery->ops_agg.opa_keeptids);
            BTor(OPV_MAXVAR, (char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_rvrm, (char *)&new_subquery->ops_agg.opa_keeptids);
                                    /* if view processing has already
                                    ** set the keeptid map then do not
                                    ** reset it */
	}
        /* FIXME - I do not think this statement does anything
        ** since the opa_tidactive flag is not set, check this */
    }

    if (father && 
        (   (sqtype == OPS_RFAGG)
            ||
            (sqtype == OPS_HFAGG)
            ||
            (sqtype == OPS_SAGG)
            ||
            (sqtype == OPS_FAGG)
            ||
            (sqtype == OPS_RSAGG)
        ))
    {
        BTor(OPV_MAXVAR, (char *)&new_subquery->ops_root->pst_sym.pst_value.
                pst_s_root.pst_tvrm, (char *)&father->ops_aggmap);
        BTor(OPV_MAXVAR, (char *)&viewmap, (char *)&father->ops_aggmap);
                                        /* set all variables reference by
                                        ** the function aggregate into the
                                        ** SELECT parent, do not include
                                        ** the left and right maps since
                                        ** these could contain correlated
                                        ** variables, but include variables
                                        ** in the view which were substituted */
    }

    if (is_sql && father)
    {   /* if this is an SQL node and not the main query then
        ** determine the corelated variables and propagate to
        ** outer selects and aggregates */
        OPV_GBMVARS     corelated;      /* bitmap of corelated
                                        ** variables that need to be
                                        ** resolved */
        /* the corelated variables are those which are referenced in
        ** the subquery (.i.e pst_rvrm || pst_lvrm), but not
        ** referenced in the subselect FROM list (i.e. pst_tvrm) 
        */
        MEcopy((PTR)&q->pst_sym.pst_value.pst_s_root.pst_lvrm,
            sizeof(OPV_GBMVARS), (PTR)&corelated);
        BTor((i4)BITS_IN(corelated), 
            (char *)&q->pst_sym.pst_value.pst_s_root.pst_rvrm,
            (char *) &corelated);       /* corelated now contains
                                        ** a map of all variables
                                        ** which need to be resolved
                                        ** in scope */
        {   
            OPS_SUBQUERY        *outer;
            for (outer = new_subquery; outer; 
                outer = outer->ops_agg.opa_father)
            {
                OPV_GBMVARS     tempcorelated; /* temp bitmap used to
                                        ** calculate corelated 
                                        ** variables */
                MEcopy((PTR)&outer->ops_root->pst_sym.pst_value.
                        pst_s_root.pst_tvrm,
                    sizeof(OPV_GBMVARS), 
                    (PTR)&tempcorelated);
                BTor((i4)(BITS_IN(tempcorelated)),
                    (char *)&outer->ops_fcorelated,
                    (char *)&tempcorelated); /* part of b40059 fix,
                                        ** add variables to from list
                                        ** which were resolved by flattening
                                        */
		BTclearmask((i4)BITS_IN(corelated), (char *)&tempcorelated,
                    (char *)&corelated); /* bitmap of corelated
                                        ** variables i.e. variables
                                        ** which have not been resolved
                                        ** at this scope level */
                if (BTnext((i4)-1, 
                           (char *)&corelated, 
                           (i4)BITS_IN(corelated)
                          )
                    < 0)
                    break;              /* if no variables are
                                        ** corelated then break */
#ifdef    E_OP0280_SCOPE
                if (outer->ops_root->pst_sym.pst_value.pst_s_root.
                        pst_qlang != DB_SQL)
                    opx_error( E_OP0280_SCOPE); /* expecting corelation
                                        ** in SQL constructs only */
#endif
                BTor((i4)BITS_IN(corelated), (char *)&corelated,
                    (char *)&outer->ops_correlated); /* at least
                                        ** one corelated variable
                                        ** exists so propagate */
                if (outer->ops_sqtype == OPS_SAGG)
                {
                    outer->ops_sqtype = OPS_RSAGG;  /* mark correlated simple
                                        ** aggregate */
                }
            }
        }
        if ((sqtype == OPS_RFAGG || sqtype == OPS_FAGG || sqtype == OPS_HFAGG)
            &&
            (!(global->ops_gmask & OPS_QUERYFLATTEN))
            )
        {   /* check if this is an AGHEAD node which originated from the target
            ** list of a view, in which case the AGHEAD result may be required
            ** to become correlated */
            for(;father
                &&
                !BTsubset((char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    (char *)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    sizeof(father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
                                        /* if this is an SQL aggregate and the from list
                                        ** is not a subset of the immediate parent then
                                        ** this is a correlated aggregate result which needs
                                        ** to be linked back to the parent */
                ; father = father->ops_agg.opa_father
                )
            {
                OPA_SUBLIST         *sub_correlated;    /* ptr to element to be placed
                                        ** in a list of correlated subqueries */
                OPV_GBMVARS         tempmap;
                OPV_GBMVARS         nestedmap;
                MEcopy ((PTR)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    sizeof(tempmap), (PTR)&tempmap);
                BTor((i4)BITS_IN(tempmap), (char *)&new_subquery->ops_agg.opa_flatten,
                    (char *)&tempmap);
                if (BTsubset((char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                        (char *)&tempmap, (i4)BITS_IN(tempmap)))
                    break;
                                        /* if this is an SQL aggregate and the from list
                                        ** is not a subset of the immediate parent then
                                        ** this is a correlated aggregate result which needs
                                        ** to be linked back to the parent */
                /* there should be no intersection of the from lists at this point or
                ** the current model of view substition has been changed, check this
                ** assertion */
                MEcopy((PTR)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    sizeof(nestedmap),
                    (PTR)&nestedmap);
                BTand((i4)BITS_IN(nestedmap),
                    (char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                    (char *)&nestedmap);
                if (BTnext((i4)-1, (char *)&nestedmap, (i4)BITS_IN(nestedmap)) >= 0)
                {   /* this is the case of a doubly nested select with aggregates in which
                    ** the inner most select references a variable in the grandparent or
                    ** above... if there is some intersection then all the variables
                    ** should intersect if the correlated variables are included */
                    BTor((i4)BITS_IN(tempmap), (char *)&new_subquery->ops_fcorelated,
                        (char *)&tempmap);
#ifdef    E_OP0280_SCOPE
                    if (!BTsubset((char *)&new_subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                            (char *)&tempmap, (i4)BITS_IN(tempmap)))
                        opx_error(E_OP0280_SCOPE);
#endif
                    break;
                }
#ifdef    E_OP0280_SCOPE
                if (father->ops_root->pst_sym.pst_type != PST_AGHEAD) /* only an aggregate
                                        ** type is expected to be the parent after flattening
                                        */
                    opx_error(E_OP0280_SCOPE);
#endif
                sub_correlated = (OPA_SUBLIST *)opu_memory(global, (i4)sizeof(OPA_SUBLIST));
                sub_correlated->opa_subquery = new_subquery;
                sub_correlated->opa_next = father->ops_agg.opa_agcorrelated;
                father->ops_agg.opa_agcorrelated = sub_correlated;
            }
        }
    }
    if ((new_subquery->ops_sqtype == OPS_SAGG)
        &&
        (!BTcount((char *)&new_subquery->ops_correlated, OPV_MAXVAR))
	)
    {   /* if this is a correlated simple aggregate, then it might participate
        ** as part of a qualification forced into a union view, take all
        ** subqueries associated with this and place into context of parent
        ** so that is can be placed correctly with respect to  union views
        */
        OPS_SUBQUERY    **endsq_pp;
        for (endsq_pp = &gfather->opa_simple; *endsq_pp; endsq_pp = &(*endsq_pp)->ops_next)
            ;
        (*endsq_pp) = global->ops_subquery;     /* need to add to end of list since union
                                        ** views will be defined before the first simple aggregate
                                        ** subquery */
        global->ops_subquery = new_subquery->ops_next;
        new_subquery->ops_next = (OPS_SUBQUERY *)NULL;
    }
}

/*{
** Name: opa_sagg       - remove simple aggregate subselect if possible
**
** Description:
**      Check for simple aggregate subselects and determine if they can be
**      removed, by substituting them directly in the parent subselect. 
**      This can be done if the from list of the subselect is equal to 
**      the from list of the aggregate.  This check will ensure that aggregation
**      is occuring and that a simple aggregate that is part of a view was
**      not substituted, which would mean that the result would be multi
**      valued.  If no aggregation is occuring then the result could be
**      multi-valued and the subselect cannot be removed at this time.  If
**      a function aggregate is contained within then the result could
**      also multi-valued.
**      FIXME - OPF still cannot handle subselects with no variables defined
**
** Inputs:
**      rootp                           ptr to subselect node defining scope
**      nodep                           ptr to current node being analyzed
**
** Outputs:
**      substitute                      set FALSE if some condition preventing
**                                      removal of subselect is found
**      agg_found                       set TRUE if simple aggregate found which
**                                      has same scope as subselect so that
**                                      all values in subselect will be
**                                      aggregated
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      21-may-86 (seputis
**          initial creation
**      8-may-89 (seputis)
**          make flattening the default
[@history_line@]...
[@history_template@]...
*/
#if 0
static VOID
opa_sagg(
        OPS_SUBQUERY       *subquery,
        PST_QNODE          *rootp,
        PST_QNODE          *nodep,
        bool               *substitute,
        bool               *agg_found)
{
    if (global->ops_gmask & OPS_QUERYFLATTEN)
    {
        *substitute = FALSE;
        return;
FIXME
Need to fix this problem prior to enabling code, i.e. correlated
simple aggregates should not be substituted
select distinct pno
from sp spx
where 1 <
        ( select count(*)
          from sp spy
          where spy.pno = spx.pno );
    }

    switch (nodep->pst_sym.pst_type)
    {
    case PST_SUBSEL:
    case PST_ROOT:
        break;
    case PST_AGHEAD:
    {   /* check aggregate and set flags to caller */
        if (    (rootp->pst_sym.pst_value.pst_s_root.pst_tvrm
                ==
                nodep->pst_sym.pst_value.pst_s_root.pst_tvrm
                )
            &&
                nodep->pst_left->pst_sym.pst_type == PST_AOP
            )
            *agg_found = TRUE;      /* simple aggregate is found with
                                    ** matching from list so subselect
                                    ** is an aggregation */
        else
            *substitute = FALSE;    /* this aggregate could cause
                                    ** multi-valued results */
        break;
    }
    case PST_VAR:
    {
        *substitute = FALSE;        /* if a var node is seen then
                                    ** this subselect may be multi-valued*/
        break;
    }
    default:
        if (nodep->pst_left)
            opa_sagg(rootp, nodep->pst_left, substitute, agg_found);
        if (nodep->pst_right)
            opa_sagg(rootp, nodep->pst_right, substitute, agg_found);
    }
}
#endif

/*{
** Name: opa_resolve    - resolve data types for a newly created node
**
** Description:
**      Resolve data types for a newly created OP node
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nodep                           ptr to node to resolve
**
** Outputs:
**      Returns:
**          none
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      3-nov-89 (seputis)
**          initial creation
**      09-jul-93 (andre)
**          recast first arg to pst_resolve() to (PSF_SESSCB_PTR) to agree
**          with prototype declaration in psfparse.h
**      20-Aug-2009 (coomi01) b122237
**          Do the work via formal interface routine psq_call()
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type psq-call param, fix here.
*/
VOID
opa_resolve(
        OPS_SUBQUERY       *subquery,
        PST_QNODE          *nodep)
{
    /* need to resolve the types for the new BOP equals node */

    PSQ_CB          psqCb;
    DB_STATUS       res_status; /* return status from resolution
				** routine 
				*/

    /* 
    ** Clear the Control block
    */
    MEfill(sizeof(PSQ_CB), (u_char) '\0', (PTR) &psqCb);

    /*
    ** Call pst_resolve() through a formal interface
    */
    CLRDBERR(&psqCb.psq_error);
    psqCb.psq_adfcb = (PTR)subquery->ops_global->ops_adfcb;
    psqCb.psq_qlang = subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang;   

    /* 
    ** Need to initialise two struct pointers
    */
    psqCb.psq_qnode  = nodep; 
    psqCb.psq_qsfcb  = &subquery->ops_global->ops_qsfcb;

    res_status = psq_call(PSQ_OPRESLV,
			  &psqCb,
			  NULL);

    if (res_status != E_DB_OK)
	opx_verror( res_status, E_OP0685_RESOLVE, psqCb.psq_error.err_data);
}
#if 0

/*{
** Name: opa_caggregate - find correlated aggregate
**
** Description:
**      Traverse the subtree to determine if a correlated aggregate 
**      exists.  Hopefully, this routine will be removed when OPC
**      handles correlated aggregates, or a new aggregate processing
**      strategy was implemented.
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**      Returns:
**          TRUE - if correlated aggregate was found
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      4-may-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static bool
opa_caggregate(
        OPS_SUBQUERY    *subquery,
        PST_QNODE       *subselect,
        PST_QNODE       *nodep,
        bool            aggfound)
{
    for(;nodep; nodep = nodep->pst_right)
    {   /* iterate down right side and recurse down left */
        switch (nodep->pst_sym.pst_type)
        {
        case PST_AGHEAD:
            aggfound = TRUE;            /* if a correlated value is now found
                                        ** then the query cannot be flattened
                                        ** since it was embedded in the
                                        ** aggregate */
            subselect = nodep;          /* use new scope for test since
                                        ** aggregate may have come from a
                                        ** quel view, all SQL aggregates
                                        ** should have the same from list
                                        ** as the parent subselect */
            break;

        case PST_SUBSEL:
            if (subselect != nodep)     /* check for initial subselect */
                return(FALSE);          /* a subselect has its' own scope so
                                        ** it can have correlated values */
            break;
        case PST_VAR:
            if (!BTtest((i4)nodep->pst_sym.pst_value.pst_s_var.pst_vno,
                (char *)&subselect->pst_sym.pst_value.pst_s_root.pst_tvrm))
            {   /* variable found which is not in the scope of the
                ** subselect */
                return(aggfound);       /* if this var was found in the
                                        ** context of an aggregate then
                                        ** flattening must be avoided */
            }
            return(FALSE);
        case PST_QLEND:
        case PST_TREE:
        case PST_CONST:
            return(FALSE);
        }
        if (nodep->pst_left 
            &&
            opa_caggregate(subquery, subselect, nodep->pst_left, aggfound))
            return(TRUE);               /* aggregate was found in left subtree
                                        */
    }
    return(FALSE);
}
#endif
/*{
** Name: opa_ifnullcagg - find correlated aggregates inside "ifnull" functions.
**
** Description:
**      Traverse the subtree to determine if a correlated aggregate
**      exists within an "ifnull" function.
[@comment_line@]...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**      Returns:
**          TRUE - if correlated aggregate was found inside an "ifnull"
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	20-Feb-99 (hanal04)
**	    initial creation, used opa_caggregate() as a template. b91232.
**	5-dec-00 (hayke02)
**          Added ifnull_aop, so that this function now returns the address
**          of the AOP PST_QNODE inside the ifnull().
**	11-Dec-2002 (huazh01)
**	    extend the fix for b91232 so that it covers the case the 
**          subselect statement containing 'max' aggregate. 
**          This fixes bug 109262. INGSRV 2028. 
**      10-may-07 (huazh01)
**          back out the fix for b109262, it is now in opa_agbylist().
**          this fixes 116010.
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
[@history_line@]...
[@history_template@]...
*/
static bool
opa_ifnullcagg(
        OPS_SUBQUERY    *subquery,
        PST_QNODE       *subselect,
        PST_QNODE       *nodep,
        bool            ifnfound,
        bool            aggfound,
        PST_QNODE       **ifnull_aop)
{
    for(;nodep; nodep = nodep->pst_right)
    {   /* iterate down right side and recurse down left */
        switch (nodep->pst_sym.pst_type)
        {
        case PST_BOP:
        case PST_MOP:
            if((nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
			& ADI_F32768_IFNULL) &&
               (nodep->pst_left) &&
               (nodep->pst_left->pst_sym.pst_type == PST_AGHEAD))
            {
              ifnfound = TRUE;
            }
            break;
 
        case PST_AGHEAD:
            if(ifnfound)
            {
               ifnfound = FALSE;        /* aggfound will therefore only be
                                        ** set to TRUE if the parent set
                                        ** ifnfound to TRUE. i.e. we have
                                        ** a SAGG in an ifnull. */
               aggfound = TRUE;         /* If a correlated value is now found
                                        ** then the query cannot be flattened
                                        ** since it was embedded in the
                                        ** aggregate, in the ifnull. */
                if (nodep->pst_left->pst_sym.pst_type == PST_AOP)
                    *ifnull_aop = nodep->pst_left;
            }
            subselect = nodep;          /* use new scope for test since
                                        ** aggregate may have come from a
                                        ** quel view, all SQL aggregates
                                        ** should have the same from list
                                        ** as the parent subselect */
            break;
 
        case PST_SUBSEL:
            if (subselect != nodep)     /* check for initial subselect */
                return(FALSE);          /* a subselect has its' own scope so
                                        ** it can have correlated values */
            break;
 
        case PST_VAR:
            if (!BTtest((i4)nodep->pst_sym.pst_value.pst_s_var.pst_vno,
                (char *)&subselect->pst_sym.pst_value.pst_s_root.pst_tvrm))
            {   /* variable found which is not in the scope of the
                ** subselect */
                return(aggfound);       /* if this var was found in the
                                        ** context of an aggregate then
                                        ** flattening must be avoided */
            }
            return(FALSE);
 
        case PST_QLEND:
        case PST_TREE:
        case PST_CONST:
            return(FALSE);
 
        }
 
        if (nodep->pst_left
            &&
            opa_ifnullcagg(subquery, subselect, nodep->pst_left,
                         ifnfound, aggfound, ifnull_aop))
            return(TRUE);               /* aggregate was found in left subtree
                                        */
    }
    return(FALSE);
}

/*{
** Name: opa_ifnullaop - find PST_IFNULL_AOP subquery - switch off flattening
**      
** Description:
**      This routine searches the subquery linked list looking for a
**      subquery that has been marked with PST_IFNULL_AOP in opa_flatten()
**      and switches off query flattening if found, and also switches off
**      projection.
**      
** Inpouts:
**      global                          ptr to global state variable
**
** Outputs:
**
**      Returns:
**          TRUE - if a PST_IFNULL_AOP subquery has been found (and flattening,
**          projection swithced off).
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      24-jan-01 (hayke02)
**          Initial creation as part of fix for bug 103720.
**	23-may-03 (inkdo01)
**	    Further refinement of changes for 91232, 103255, 103720 to permit
**	    projection sq's when a single ifnull(<agg>, x) appears in a
**	    subquery.
[@history_template@]...
*/
static bool
opa_ifnullaop(
        OPS_STATE       *global,
	PST_QNODE	*ifnull_aop)
{
    OPS_SUBQUERY    *subquery;

    for (subquery = global->ops_subquery; subquery;
                                        subquery = subquery->ops_next)
    {
        if (subquery->ops_agg.opa_aop 
	    &&
	    ifnull_aop
            &&
            subquery->ops_agg.opa_aop != ifnull_aop
            &&
            (subquery->ops_agg.opa_aop->pst_sym.pst_value.pst_s_op.pst_flags
            &
            PST_IFNULL_AOP))
        {
            subquery->ops_sqtype = OPS_RFAGG;
            subquery->ops_agg.opa_projection = FALSE;
            subquery->ops_agg.opa_mask &= ~OPA_APROJECT;
            subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_project = FALSE;
            global->ops_gmask |= OPS_QUERYFLATTEN;
            return(TRUE);
        }
    }
    return(FALSE);
}

/*{
** Name: opa_ojflatten  - extract the OJ phrases from the query
**
** Description:
**      In order to support the OPC interface the flattening of outer joins
**      require that no outer join fragment be placed beneath an OR, otherwise
**      it may be possible for 2 subselects to be flattened and a boolean
**      factor containing 2 different join ID's would occur, which OPC cannot
**      handle.  We can take the outer join conjunct out of the "OR" since it
**      is self-contained and cannot be correlated, normally this would not be
**      possible.
**
** Inputs:
**      gstate                          ptr to state variable for aggregate processing
**      non_ojpp                        ptr to non-outer join parse tree.
**      joinid                          joinid context- if set then all qualifications
**                                      with this joinid need to be attached beneath
**                                      the OR
**
** Outputs:
**
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      2-feb-90 (seputis)
**          initial creation
**      8-feb-93 (ed)
**          - fix bug 49317, access violation, incorrect node being checked
**          for join id
**      11-feb-94 (ed)
**          - fix bug 59593 - qualification was incorrectly eliminated
**          due to mishandling of linked list
**      7-mar-94 (ed)
**          - simplified routine by using some internal  normalization routines
**          to place query tree into form in which PST_AND nodes are right most
**          children (without performing full conjunction normal form transformation)
[@history_line@]...
[@history_template@]...
*/
static VOID
opa_ojflatten(
        OPA_RECURSION      *gstate,
        PST_QNODE          **non_ojpp,
        PST_J_ID           joinid)
{
    PST_QNODE       *nodep;
    PST_QNODE       **ojpp;
    PST_QNODE       *ojp;   /* ptr to query tree fragment which may need to
                            ** moved above the OR node due to outer join
                            ** requirements of OPC which do not allow the same
                            ** outer join ID to be evaluated at the same node */
    PST_QNODE       *leftp;
    OPS_SUBQUERY    *father;

    father = gstate->opa_gfather;
    opj_adjust(father, non_ojpp);   /* place PST_QLEND on end of list */
    opj_mvands(father, *non_ojpp);  /* move ands to right side of tree */
    ojpp = &ojp;
    ojp = (PST_QNODE *) NULL;
    while ((nodep = *non_ojpp)
        &&
        (nodep->pst_sym.pst_type == PST_AND))
    {

        leftp = nodep->pst_left;
        if (leftp)
        {
            switch (leftp->pst_sym.pst_type)
            {
            case PST_BOP:
            case PST_AOP:
            case PST_COP:
            case PST_OR:
            case PST_UOP:
            if (leftp->pst_sym.pst_value.pst_s_op.pst_joinid != joinid)
            {   /* detach and place into the outer join list */
                *ojpp = nodep;      /* place conjunct into outer join list */
                *non_ojpp = nodep->pst_right; /* remove outer join
                                    ** from main query list */
                nodep->pst_right = (PST_QNODE *)NULL;
                ojpp = &nodep->pst_right;
                break;
            }
            /* fall thru */
            case PST_QLEND:
                non_ojpp = &nodep->pst_right;
                break;
            default:
                opx_error(E_OP068E_NODE_TYPE);
            }
        }
        else
            non_ojpp = &nodep->pst_right;
    }
    if (ojp)
    {   /* move the qualification if at least one outer join is found */
        *(gstate->opa_ojend) = ojp;
        gstate->opa_ojend = ojpp;
        /* delay attachment of outer join to root, since root is currently
        ** being traversed and unexpected side effects of modifying root
        ** while it is being traversed could occur */
    }
}

/*{
** Name: opa_flatten    - flatten subselects
**
** Description:
**      Routine used to flatten an =ANY or EXISTS subselect.   Currently,
**      a query which contains an aggregate with a correlated value cannot
**      be flattened since OPC is not structured to handle this case.  For
**      this reason, a special traversal of the subselect is made, to
**      determine this case and avoid flattening, it can be removed if OPC
**      handles correlated aggregates without a subselect node.
**
** Inputs:
**      father                          subquery containing subselect
**      subselect                       ptr to subselect node root
**
** Outputs:
**      Returns:
**          TRUE - if query was successfully flattened
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      17-jul-87 (seputis)
**          initial creation
**      4-dec-88 (seputis)
**          passed in varmap for flatten, so proper bitmap gets updated
**      8-may-89 (seputis)
**          make flattening the default
**      17-aug-89 (seputis)
**          added viewmap to fix duplicate handling problem
**      26-sep-89 (seputis)
**          init OPS_ZEROTUP_CHECK for b8003 so that in the case of
**          flattening below an OR node, will cause a retry if zero
**          tuples are returned
**      21-oct-90 (seputis)
**          b33387 - restore nestcount since it is added twice
**      22-Feb-99 (hanal04)
**          b91232 - Don't flatten a query if the subselect contains
**          a correlated aggregate inside an "ifnull" statement.
**	 5-dec-00 (hayke02)
**	    The fix for 91232 has been modified so that we don't switch
**	    off flattening - rather, we mark the aggregate node with the
**	    new flag PST_IFNULL_AOP and add a project node if this is the
**	    case in opa_agbylist(). This change fixes bug 103255.
**	24-jan-01 (hayke02)
**	    The fix for bug 103255 has been modified so that we now switch
**	    off flattening if we have more than one subselect with a
**	    correlated aggregate in an ifnull() or one correlated aggregate
**	    in an ifnull() and another subselect containing a PST_AGHEAD
**	    node. This prevents expensive cart prod nodes when the
**	    ifnull(agg()) nodes are projected. This change fixes bug 103720.
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**      04-jan-2005 (huazh01)
**          Modify the fix for b110577. Decisions on whether or not to
**          flatten query(s) contain ifnull() is now being made in 
**          opa_flatten()l 
**          This fixes bug 113519, INGSRV3055. 
**    7-apr-05 (wanfr01)
**          Partial rollback of fix for bug 8003 to resolve bug 109125.
**    17-sep-2009 (huazh01)
**          if this is a distributed session, then we do not flatten query  
**          if the remote database does not support TID. (b122604)
[@history_line@]...
[@history_template@]...
*/
static bool
opa_flatten(
        OPA_RECURSION      *gstate,
        PST_QNODE          *subselect,
        PST_QNODE          **insertpp,
        PST_QNODE          *conjunct)
{
    OPS_STATE       *global;
    OPS_SUBQUERY    *father;
    PST_J_ID        joinid;     /* joinid of parent to be used
                                ** in flattening  */
    PST_QNODE       *ifnull_aop = NULL;
    PST_QNODE       *qnode = NULL;
    OPS_SUBQUERY    *subquery;
    OPV_GBMVARS     tempmap;
    OPV_IGVARS      gvarno;


    father = gstate->opa_gfather;
#ifdef    E_OP0280_SCOPE
    MEcopy((char *)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
        sizeof(tempmap), (char *)&tempmap);
    BTand(OPV_MAXVAR, (char *)&subselect->pst_sym.pst_value.pst_s_root.pst_tvrm,
        (char *)&tempmap);
    if (BTcount((char *)&tempmap, OPV_MAXVAR))
        opx_error( E_OP0280_SCOPE); /* expecting from lists to
                                ** have no intersection */
#endif
    global = father->ops_global;
    if (global->ops_gmask & OPS_QUERYFLATTEN)
        return(FALSE);          /* do not flatten code since
                                ** too many restriction in
                                ** OPC/QEF to allow this safely */
    joinid = gstate->opa_joinid;

#if 0
/* This query will cause OPC problems if it is flattened
select distinct sno 
from sp spx
where not exists
        ( select *
          from sp spy
          where sno = 's2'
          and not exists
                ( select *
                  from sp spz
                  where spz.sno = spx.sno
                  and spz.pno = spy.pno ));
*/
    if (opa_caggregate(father, subselect, subselect, FALSE))
        return(FALSE);
\p\g
#endif

    if (opa_ifnullcagg(father, subselect, subselect, FALSE, FALSE, &ifnull_aop)
        &&
        ifnull_aop)
    {
	if ((global->ops_gmask & OPS_SUBSEL_AGHEAD)
            ||
            (global->ops_qheader->pst_mask1 & PST_AGHEAD_NO_FLATTEN)
           )
	{
	    global->ops_gmask |= OPS_QUERYFLATTEN;
	    return(FALSE);
	}

        if (opa_ifnullaop(global, ifnull_aop))
            return(FALSE);

        ifnull_aop->pst_sym.pst_value.pst_s_op.pst_flags |= PST_IFNULL_AOP;
    }
    else
    {
        if (subselect->pst_left && !(global->ops_gmask & OPS_SUBSEL_AGHEAD))
        {
            for (qnode = subselect->pst_left->pst_right; qnode;
                                                        qnode = qnode->pst_left)
            {
                if (qnode->pst_sym.pst_type == PST_AGHEAD)
                {
                    global->ops_gmask |= OPS_SUBSEL_AGHEAD;

                    if (opa_ifnullaop(global, NULL))
                        return(FALSE);

                    break;
                }
            }
        }
    }

    if (!father->ops_agg.opa_tidactive && (father->ops_duplicates == OPS_DKEEP))
    {   /* keep tids from the FROM list of the parent. 
        **
        ** if this is a distributed session, check to see if the remote
        ** database supports TID. (b122604)
        */
  
        if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
        {
           for (gvarno = -1; 
                (gvarno = BTnext((i4)gvarno, 
                                  (char *)&father->ops_root->pst_sym.\
                                        pst_value.pst_s_root.pst_tvrm, 
			           (i4)BITS_IN(OPV_GBMVARS))) >= 0 ;
	       )
           {
               if (global->ops_rangetab.opv_base->opv_grv[gvarno]-> \
                    opv_relation->rdr_obj_desc->dd_o9_tab_info.\
                    dd_t9_ldb_p->dd_i2_ldb_plus.\
                    dd_p3_ldb_caps.dd_c1_ldb_caps & DD_5CAP_TIDS)
                  continue; 

               /* remote db doesn't support TID, don't flatten this query */
               return (FALSE); 
           }
        }

        father->ops_agg.opa_tidactive = TRUE;
        MEcopy((char *)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm,
                sizeof(OPV_GBMVARS), (char *)&father->ops_agg.opa_keeptids);
                                /* the from list defines which
                                ** TIDs need to be brought up
                                ** to keep duplicate semantics
                                ** correct, note that view
                                ** processing should have been
                                ** done by this point if
                                ** necessary */
    }
#if 0
/* this information should be set inside the call to opa_grtable and in fact
** conflicts which the consistency checks made */

    father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm
        |= subselect->pst_sym.pst_value.pst_s_root.pst_tvrm;
                                /* combine the from lists of the
                                ** any subselect, this will cause subselects
                                ** var node to be non-corelated since they
                                ** will appear in the from list of the parent
                                */
    father->ops_agg.opa_flatten |= subselect->pst_sym.pst_value.pst_s_root.pst_tvrm;
                                /* save the relations which were flattened
                                ** into the parent query, since these should
                                ** not be used when determining whether an aggregate
                                ** is a correlated variable */
#endif
    if ((global->ops_qheader->pst_numjoins > 0)
        &&
        (joinid > 0)
        )
    {   /* need to ensure that joinid's are consistent prior to traversing
        ** tree, since a subselect is flattened, the joinid of the parent operator
        ** should be used on all child operators */
        if (subselect->pst_right)
            opa_ojsub(joinid, subselect->pst_right);   /* substitute all nodes with
                                    ** the usage of the parent joinid */
        if (subselect->pst_left)
            opa_ojsub(joinid, subselect->pst_left);   /* substitute all nodes with
                                    ** the usage of the parent joinid */
    }
    {
        OPZ_IATTS       saversdm; /* save the resdom count since this must
                                ** less than any AOP attribute or non-printing
                                ** resdom, also opa_grtable may flatten other
                                ** subselects and in the process traverse
                                ** resdom lists which will be thrown away, but
                                ** will increment the resdom count incorrectly
                                ** as in the following query
                                ** select j.jno from j where not exists
                                **  (select * from spj where spj.jno=j.jno
                                **  and spj.pno in 
                                **  (select p.pno from p where p.color = 'red'))
                                */
        OPV_GBMVARS     viewmap;/* not used */
        i4              savenestcount; /* b 33387 - flatten queries not combined since
                                ** the nesting count was added twice */
        saversdm = father->ops_agg.opa_attlast;
        savenestcount = father->ops_agg.opa_nestcnt;
	/* Determine when we need to do an implicit DISTINCT.
	** We don't want to drop duplicates if we have a scalar sub-query
	** with the comparison operator. Special care is needed with IN/NOT IN
	** as these appear as EQ or NE nodes with PST_INLIST set and for these
	** we imply distinct. Other comparisons should not tamper with the
	** pst_dups setting on the root. */
        opa_grtable(gstate, subselect,
		(!global->ops_cb->ops_alter.ops_nocardchk &&
		subselect->pst_sym.pst_value.pst_s_root.pst_dups == PST_ALLDUPS &&
		conjunct &&
		conjunct->pst_sym.pst_value.pst_s_op.pst_opmeta == PST_CARD_01R &&
		!(conjunct->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST))
		?OPS_DKEEP
		:OPS_DREMOVE, FALSE, &viewmap,
            gstate->opa_gflatten); /* process from list
                                ** and views in the subselect
                                ** prior to substituting */
        father->ops_agg.opa_nestcnt = savenestcount;
        father->ops_agg.opa_attlast = saversdm;
    }

    if (conjunct)
    {
        PST_QNODE       *and_node;

        if (sizeof(PST_RSDM_NODE)<sizeof(PST_OP_NODE))
            opx_error(E_OP068D_QTREE); /* make sure this assumption
                            ** is true, note, if it changes then a more
                            ** elaborate scheme is necessary, NOT
                            ** simply just allocating memory and assigning
                            ** values since the address of 
                            ** conjunct->pst_right should not change */
                            /* need to make the resdom node the new
                            ** OP node since and aghead subquery may
                            ** be pointing into the resdom node directly
                            */

	if (conjunct->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_CARD_01R &&
		conjunct->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_CARD_01L)
	    conjunct->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
        and_node = conjunct;
        conjunct = subselect->pst_left;
        conjunct->pst_left = and_node->pst_left; /* this is the left part
                            ** being compared to the subselect expression */
        MEcopy((PTR)&and_node->pst_sym,
            (sizeof(PST_SYMBOL) - sizeof(PST_SYMVALUE)
                +sizeof(PST_OP_NODE)),
            (PTR)&conjunct->pst_sym); /* copy the comparison operator */
        opa_resolve (father, conjunct); /* re-resolve the datatypes
                            ** for this new node */
        if( subselect->pst_right 
            && 
            (subselect->pst_right->pst_sym.pst_type != PST_QLEND))
        {       /* both a conjunct must be created(to be done by caller)
            ** and the where clause must be added */
            and_node->pst_sym.pst_type = PST_AND;
            and_node->pst_sym.pst_value.pst_s_op.pst_opno = (ADI_OP_ID)0;
            and_node->pst_right = subselect->pst_right;
            and_node->pst_left = conjunct; 
            *insertpp = and_node;
        }
        else
        {
            *insertpp = conjunct;
            insertpp = (PST_QNODE **)NULL;
        }
        gstate->opa_demorgan = FALSE;   /* just in case */
        opa_generate(gstate, &conjunct->pst_left);      /* traverse 
                                        ** left sub tree, since the
                                        ** right subtree was already traversed
                                        ** by the flattening code */
    }
    else
    {
        *insertpp = subselect->pst_right;
    }
    if (insertpp
        &&
        (global->ops_qheader->pst_numjoins > 0)
        &&
        (!gstate->opa_gflatten)
        )
        /* there is a potential for an outer join to be flattened at this
        ** point */
        opa_ojflatten(gstate, insertpp, 
            (PST_J_ID)((joinid > 0)?joinid : PST_NOJOIN));
    return (TRUE);
}

/*{
** Name: opa_cosubquery - check for corelation in subquery
**
** Description:
**      This routine is a special case traversal to determine whether a 
**      subquery is corelated so that a simple aggregate can be used if it is
**      not correlated, this will avoid a cartesean product, and also avoids 
**      an OPC problem in which a query like
**  select *
**      from l1_small
**      where exists (select l2_small.ck
**                              from l2_small)
**     would not have any equivalence classes at an interior CO node.
**
**      In distributed, PSF will mark a Select node has being correlated or
**      not so that this pass will not be necessary, except for views which
**      have been stored prior to the PSF feature being implemented.
**    
**
** Inputs:
**      global                          ptr to global state variable
**      varmap                          ptr to from list map which currently
**                                      describes the variable scope
**      qnode                           ptr to current query tree node being
**                                      analyzed
**
** Outputs:
**      Returns:
**        None
**      Exceptions:
**
** Side Effects:
**
** History:
**      16-mar-89 (seputis)
**          initial creation
**      05-aug-93 (shailaja)
**          opa_cosubquery : return status changed to void.
[@history_template@]...
*/
static  VOID
opa_cosubquery(
        OPS_STATE          *global,
        OPV_GBMVARS        *varmap,
        PST_QNODE          *qnode)
{
    for (;qnode; qnode = qnode->pst_left)
    {
        switch (qnode->pst_sym.pst_type)
        {
        case PST_VAR:
        {
            BTset((i4)qnode->pst_sym.pst_value.pst_s_var.pst_vno,
                    (char *) varmap);
            return;
        }
        case PST_AGHEAD:
        case PST_SUBSEL:
        case PST_ROOT:
        {       /* traverse the subquery and reset bits in the from list
            ** which are visible to the parent */
            OPV_GBMVARS     scope;

            MEfill(sizeof(scope), (u_char)0, (PTR)&scope);
            if (qnode->pst_left)
                opa_cosubquery(global, &scope, qnode->pst_left);
            if (qnode->pst_right)
                opa_cosubquery(global, &scope, qnode->pst_right); /* set the
                                        ** bits for variables which need to
                                        ** be resolved */
	    /* Invert the from list to reset all variables which are
                                        ** resolved at this scope level
                                        */
	    BTclearmask((i4)(BITS_IN(scope)),
		(char *)&qnode->pst_sym.pst_value.pst_s_root.pst_tvrm,
		(char *)&scope);
            
            BTor((i4)(BITS_IN(*varmap)), (char *)&scope, (char *)varmap);
                                        /* add any unresolved variables to the
                                        ** parent query */
            return;
        }
        default:
            break;
        }
        if (qnode->pst_right)
            opa_cosubquery(global, varmap, qnode->pst_right); /* set the
                                    ** bits for variables which need to
                                    ** be resolved */
    }
}

/*{
** Name: opa_exists     - translate EXISTS operator
**
** Description:
**      Translate the EXISTS operator 
**                
** Inputs:
**      global                          state variable for optimization
**      agg_qnode                       ptr to ptr to the aghead node
**                                      - this value is saved in the subquery
**                                      and is eventually used for a 
**                                      "subsitution" of the aggregate result
**      father                          ptr to outer aggregate for this subtree
**                                      or NULL if root node is outer aggregate
**
**
** Outputs:
[@PARAM_DESCR@]...
**      Returns:
**          bool - TRUE if tree has been traversed
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      2-jun-87 (seputis)
**          initial creation
**      3-aug-88 (seputis)
**          OPC requires resdom number to be a 1 for a subselect
**      25-aug-88 (seputis)
**          fix exists problem, treepp incorrectly set
**      16-mar-89 (seputis)
**          fix flattening problem for non-corelated exists
**      26-apr-90 (seputis)
**          fix b21365, flattening causes query tree to be traversed twice
**          which causes problems, make bool return indicate if tree has
**          been traversed
**      8-mar-91 (seputis)
**          - use same target list expression so subselects can compare
**          as equal
**      11-jun-91 (seputis)
**          - fix b37545, use correct bitmap size for test
**      8-feb-93 (seputis)
**          - b49317 remove unnecessary cart prod for simple aggregate
**      4-sept-97 (inkdo01)
**          fix OP0200 error with several anded "exists (select *" subselects
**          which have the effect of allocating one great big result buffer.
**      26-sept-97 (inkdo01)
**          Fix to above fix to force pst_rsno of remaining RESDOM to 1, to
**          avoid later OP0284 error.
**       7-jul-99 (hayke02)
**          Ignore PSF/OPF corrrelation test if this is a query containing
**          a 'having exists...' clause (pst_mask1 | PST_HAVING). This
**          prevents E_OP0A491 when the exists subselect contains correlated
**          aggregate(s). This change fixes bug 95906.
**      26-nov-99 (hayke02)
**          Replace PST_HAVING with PST_5HAVING, and correct the logic of
**          the test for this flag.
**	 2-jun-08 (hayke02)
**	    Fix up the test for PST_5HAVING -  test the exists_ptr->pst_left
**	    node, rather than the exists_ptr node, as this will be a
**	    PST_SUBSEL/PST_RT_NODE/pst_s_root node.
**      23-Mar-2010 (horda03) B123416
**          Removed the check on PSF corelation test matching OPF's.
**          Now that Constant Folding has been introduced an AND node
**          could be removed due to the collapse of a BOP.
[@history_line@]...
*/
static bool
opa_exists(
        OPA_RECURSION       *gstate,    /* state variable used for memory management*/
        PST_QNODE          **agg_qnode  /* ptr to ptr to query tree node to be 
                                        ** analyzed
                                        */ )
{
#if 0
    OPS_SUBQUERY       *father; /* ptr to outer aggregate of this node or
                                ** NULL if "outer aggregate" is root node
                                */
    OPV_GBMVARS         *varmap;/* ptr to var bitmap on left or right side
                                ** which needs to be updated
                                */
    bool                flatten;/* TRUE if flattening of the exists should
                                ** occur as opposed to creating a select */
#endif
    PST_QNODE           *exists_ptr; /* ptr to the exists operator which will be
                                ** converted to a PST_BOP */
    i4                  exists; /* 1 if an EXISTS is found,
                                ** 0 if a NOT EXISTS is found */
    PST_QNODE           *const_ptr; /* this constant node is pointed at by
                                ** the any aggregate, AND used to replace the
                                ** any aggregrate expression, this is because
                                ** if ANY will ignore nulls, which is not the
                                ** correct semantics for EXISTS */
    OPS_STATE           *global;
    bool                distributed;    /* TRUE if distributed optimization */
    bool                corelated;      /* TRUE if corelated variable found, valid
                                ** only in distributed case */
    global = gstate->opa_global;
    exists_ptr = *agg_qnode;
    distributed = (global->ops_cb->ops_smask & OPS_MDISTRIBUTED) != 0;
#if 0
    if (distributed)            /* unnecessary cart prod for simple aggregate */
#endif
    {
        /* for distributed it is important to find if the aggregate will be corelated
        ** since it will affect whether flattening will occur in the EXISTs case,
        ** will help determine whether a simple or function ANY aggregate needs to
        ** be simulated, each has different processing */
        OPV_GBMVARS             varmap;
        MEfill(sizeof(varmap), (u_char)0, (PTR)&varmap);
        opa_cosubquery(global, &varmap, exists_ptr->pst_left); /* FIXME - have
                                ** parser tell OPF whether this root node is
                                ** corelated, rather than do a special traversal */
        corelated = BTnext((i4)-1, (char *)&varmap, (i4)BITS_IN(varmap)) >= 0;
    }
#if 0
    else
        corelated = TRUE;       /* should be ignored in LOCAL case since ANY aggregates
                                ** can be used for SQL */
#endif

    /* Before flattening, trim existence subselect's resdom list to one
    ** entry. That's all that is needed, and flattening enough "select *"
    ** existence subselects can actually overflow the internal result 
    ** buffer of the mushed query. In an attempt to limit the damage
    ** this heuristic might unintentially cause, the transformation is 
    ** only performed if all resdoms are PST_VARs.
    */
    {
        PST_QNODE       *nodep;

        if ((nodep = exists_ptr->pst_left)->pst_sym.pst_type == PST_SUBSEL
                &&
            (nodep = nodep->pst_left)->pst_sym.pst_type == PST_RESDOM)
        {
            PST_QNODE   *nodep1;
            bool        trimit = TRUE;

            for (nodep1 = nodep; nodep1 && 
                nodep1->pst_sym.pst_type == PST_RESDOM && trimit; 
                nodep1 = nodep1->pst_left) 
             if (nodep1->pst_right->pst_sym.pst_type != PST_VAR)
                trimit = FALSE;
            if (nodep != nodep1 && trimit)
            {
                nodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 1;
                                /* this prevents consistency check later on */
                nodep->pst_left = nodep1;
                                /* this leaves only the 1st resdom */
            }
        }
    }

    if (exists_ptr->pst_sym.pst_value.pst_s_op.pst_opno 
        == 
        global->ops_cb->ops_server->opg_exists)
    {
        if (!(global->ops_gmask & OPS_QUERYFLATTEN)
            ||
            gstate->opa_gflatten
            )
        {
            if (corelated
                &&
                opa_flatten(gstate, exists_ptr->pst_left, agg_qnode, (PST_QNODE *)NULL)
                )
            {
                /* FIXME - flattening a non-correlated EXISTS would cause a cartesean
                ** product,... the fix would be to check for this case in
                ** in general which could be translated back to an EXISTS
                ** i.e. translate to a simple ANY aggregate */
                /* remove the AND node from the list since the EXISTS has
                ** been flattened */
                gstate->opa_gfather->ops_vmflag = FALSE;/* since the target list of an
                                    ** exists is ignored, when flattening
                                    ** occurs the bitmap may be invalid */
                return(TRUE);
            }
        }
        exists = 1;             /* EXISTS requires one tuple */
    }
    else
        exists = 0;             /* NOT EXISTS requires no tuples */

    global->ops_qheader->pst_agintree = TRUE;

    {   /* create PST_BOP for exact subselect */
        exists_ptr->pst_right = exists_ptr->pst_left;
        exists_ptr->pst_sym.pst_type = PST_BOP;
        if (distributed && (!corelated) && exists)
        {   /* Since SQL does not support the ANY aggregate it needs to be
            ** simulated for distributed, if it is corelated then eventually
            ** the ANY will be a function aggregate after flattening, and
            ** OPC can handle this with a projection action... The simple
            ** aggregate will need to use COUNT, it would be nice to simulate
            ** EXISTS with
            ** select count (1) where EXISTS(SELECT * FROM foobar where qual)
            ** but QEF will always return 1, which is a bug.  Also, if a constant
            ** view is used it will return 1... so this approach cannot be used,
            ** it would be nice since EXISTS exits after the first tuple is found
            ** whereas count has to scan all tuples
            */
            exists_ptr->pst_sym.pst_value.pst_s_op.pst_opno = 
                global->ops_cb->ops_server->opg_le; /* used for 1 <= count (*) */
        }
        else
            exists_ptr->pst_sym.pst_value.pst_s_op.pst_opno = 
                global->ops_cb->ops_server->opg_eq;
        exists_ptr->pst_sym.pst_value.pst_s_op.pst_retnull = FALSE;
        exists_ptr->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL; /* ptr to struct 
                                            ** describing operation */
    }
    exists_ptr->pst_left = const_ptr = opv_intconst(global, exists);
    if (exists)
        const_ptr->pst_sym.pst_value.pst_s_cnst.pst_origtxt = "1";
    else
        const_ptr->pst_sym.pst_value.pst_s_cnst.pst_origtxt = "0"; /* so that
                                            ** ~V parameters do not get generated
                                            ** for distributed */
    {
        PST_QNODE           *sub_qual;      /* ptr to subselect qualification */
        PST_QNODE           *subselect;     /* ptr to subselect */
        PST_QNODE           *aghead;
        subselect = exists_ptr->pst_right;
        sub_qual = subselect->pst_right;
        if (global->ops_gmask & OPS_QUERYFLATTEN)
        {   /* place PST_QLEND on the subselect */
            /* this is the old processing approach which executes queries as
            ** written */
            PST_QNODE       *qlend;
            for (qlend = sub_qual; qlend && qlend->pst_sym.pst_type != PST_QLEND;
                qlend = qlend->pst_right);
            if (qlend)
                subselect->pst_right = qlend;
            else
                subselect->pst_right = opv_qlend(global); /* might not 
                                            ** be normalized */
            subselect->pst_left->pst_right =
            aghead = (PST_QNODE *)opu_memory(global, (i4)sizeof(*aghead));
            exists_ptr->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_CARD_ANY;
        }
        else
        {
            aghead = subselect;         /* the new flattening approach is just
                                        ** to use the aggregate subquery directly
                                        ** until outer joins are introduced
                                        */
            exists_ptr->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
        }
        {   /* place PST_AGHEAD on the resdom for the subselect */
            MEcopy ( (PTR)subselect, sizeof(*aghead), (PTR)aghead);
            aghead->pst_sym.pst_type = PST_AGHEAD;
            aghead->pst_sym.pst_value.pst_s_root.pst_dups = PST_DNTCAREDUPS; /*
                                        ** for any aggregate the duplicates
                                        ** do not matter */
            aghead->pst_right = sub_qual;   /* place qualification into
                                            ** any aggregate */
            {   /* create PST_AOP for simple ANY aggregate */
                PST_QNODE       *aop;
                ADI_OP_ID       operator;

                if (distributed && (!corelated))
                    operator =
                        global->ops_cb->ops_server->opg_count; /* COUNT op id */
                else
                    operator =
                        global->ops_cb->ops_server->opg_any; /* ANY op id */
                aghead->pst_left = 
                aop = opv_opnode(global, PST_AOP, operator, (OPL_IOUTER)PST_NOJOIN);
                if (exists == 0)
                    aop->pst_left = const_ptr;   /* create simple aggregate , use
                                        ** a constant expression instead of the
                                        ** EXISTS expression so that NULLs
                                        ** are not ignored */
                else
                {   /* always use 0 for target list expression since it
                    ** affects subselects which are to be evaluated together
                    ** i.e. if target list is different then comparision
                    ** fails, and 2 any aggregates are evaluated */
                    aop->pst_left = opv_intconst(global, 0);
                    aop->pst_sym.pst_value.pst_s_cnst.pst_origtxt = "0";
                }
                aop->pst_sym.pst_value.pst_s_op.pst_distinct = PST_DONTCARE; /*
                                        ** set to TRUE if distinct agg */
                aop->pst_sym.pst_value.pst_s_op.pst_opinst = 0; /* ADF 
                                        ** function instance id */
                aop->pst_sym.pst_value.pst_s_op.pst_distinct = PST_DONTCARE; /*
                                        ** set to TRUE if distinct agg */
                aop->pst_sym.pst_value.pst_s_op.pst_retnull = FALSE; /* set to
                                        ** TRUE if agg should return
                                        ** null value on empty set.
                                        */
                aop->pst_sym.pst_value.pst_s_op.pst_fdesc = NULL;
                aop->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
                opa_resolve( gstate->opa_gfather, aop);
                STRUCT_ASSIGN_MACRO( aop->pst_sym.pst_dataval,
                    aghead->pst_sym.pst_dataval);
            }
            STRUCT_ASSIGN_MACRO( aghead->pst_sym.pst_dataval,
                subselect->pst_left->pst_sym.pst_dataval); /* init the
                                        ** resdom node */
            STRUCT_ASSIGN_MACRO( aghead->pst_sym.pst_dataval,
                subselect->pst_sym.pst_dataval); /* init the subselect node */
        }
        if (global->ops_gmask & OPS_QUERYFLATTEN)
        {   /* search for the PST_TREE node and ignore remaining resdoms since
            ** they are not needed for an exists, in fact OPC/QEF have an
            ** interface problem in which a multi-attribute target list would
            ** cause a problem 
            ** - this optimization is used on an EXISTS SELECT * */
            PST_QNODE   **treepp;
            for (treepp = &subselect->pst_left->pst_left; 
                *treepp && ((*treepp)->pst_sym.pst_type != PST_TREE);
                )
                *treepp = (*treepp)->pst_left; /* remove all unnecessary resdoms */
            subselect->pst_left->pst_sym.pst_value.pst_s_rsdm.pst_rsno = 1; /* OPC
                                        ** requires this resdom to be 1 */
        }
        else
        {   /* this probably should be initialized without the trace flag but
            ** the ramifications are unknown, currently a projection is not
            ** needed for sql EXISTS since it is really a simple aggregate
            ** in all cases, i.e. either one tuple exists or does not, but
            ** in flattening we need function aggregates for exists */
            aghead->pst_sym.pst_value.pst_s_root.pst_project = !exists; /* if
                                        ** not exists then a projection needed
                                        ** to find the 0 tuple partitions */
            aghead->pst_sym.pst_value.pst_s_root.pst_qlang = DB_QUEL;
        }
    }
    opa_resolve( gstate->opa_gfather, exists_ptr);      /* resolve PST_BOP operator */
    return(FALSE);
}

/*{
** Name: opa_not        - complement the operator node
**
** Description:
**      As part of elimination of PST_NOT nodes, this routine will complement
**      the operator as indicated by DeMorgan's law.
**
** Inputs:
**      global                          ptr to OPF global state variable
**      tree                            ptr to operator node to complement
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      03-dec-87 (seputis)
**          initial creation
**      25-mar-91 (seputis)
**          -fix bug 35867 - update fidesc as well in parse tree node
[@history_template@]...
*/
VOID
opa_not(
        OPS_STATE          *global,
        PST_QNODE          *tree)
{
    /* Look up  the function instance in ADF */
    ADI_FI_DESC         *fidesc;
    DB_STATUS           status;

    status = adi_fidesc(global->ops_adfcb, 
        tree->pst_sym.pst_value.pst_s_op.pst_opinst,
        &fidesc);
# ifdef E_OP0781_ADI_FIDESC
    if (DB_FAILURE_MACRO(status))
        opx_verror( status, E_OP0781_ADI_FIDESC,
            global->ops_adfcb->adf_errcb.ad_errcode); /* error if 
                        ** the function instance descriptor 
                        ** cannot be found */
# endif
    /* Make sure the complement exists */
    if (fidesc->adi_cmplmnt == ADI_NOFI)
        opx_error(E_OP0796_NO_COMPLEMENT);

    /* Look up the complement */
    status = adi_fidesc(global->ops_adfcb, fidesc->adi_cmplmnt,&fidesc);
# ifdef E_OP0781_ADI_FIDESC
    if (DB_FAILURE_MACRO(status))
        opx_verror( status, E_OP0781_ADI_FIDESC,
            global->ops_adfcb->adf_errcb.ad_errcode); /* error if 
                        ** the function instance descriptor 
                        ** cannot be found */
# endif
    /* Put the complementary function in the current node */
    tree->pst_sym.pst_value.pst_s_op.pst_opno = fidesc->adi_fiopid;
    tree->pst_sym.pst_value.pst_s_op.pst_opinst = fidesc->adi_finstid;
    tree->pst_sym.pst_value.pst_s_op.pst_fdesc = fidesc;

    {
        /*
        ** For quantified predicates replace quantifier type
        ** with its complement. That is because following is true
        ** NOT( x BOP QUAN (subsel)) <==> (x cBOP cQUAN (subsel))
        ** where cBOP is a complement of BOP
        **       BOP is binary operator (=, !=, <, >, etc.)
        **       QUAN is a quantifier (ALL, ANY (same as SOME))
        **       cQUAN is a complement of QUAN
        */
        i4                      opmeta;

        opmeta = tree->pst_sym.pst_value.pst_s_op.pst_opmeta;
        if (opmeta == PST_CARD_ANY)
            tree->pst_sym.pst_value.pst_s_op.pst_opmeta 
                = PST_CARD_ALL;
        else if (opmeta == PST_CARD_ALL)
            tree->pst_sym.pst_value.pst_s_op.pst_opmeta 
                = PST_CARD_ANY;
    }
}

/*{
** Name: opa_covar      - place corelated variable in list to be processed
**
** Description:
**      Place the corelated variables on a list to be processed so that 
**      they can be added to be by-list of respective aggregates for 
**      flattening 
**
** Inputs:
**      father                          subquery containing corelated variable
**      varnodep                        ptr to corelated variable
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      1-dec-88 (seputis)
**          initial creation
**      10-may-89 (seputis)
**          removed read from unmapped memory
[@history_template@]...
*/
VOID
opa_covar(
        OPS_SUBQUERY       *father,
        PST_QNODE          *varnodep,
        OPV_IGVARS         varno)
{
    OPS_STATE       *global;

    global = father->ops_global;
    for(;father; father = father->ops_agg.opa_father)
    {
#ifdef    E_OP0280_SCOPE
        switch (father->ops_sqtype)
        {
        case OPS_RFAGG:
        case OPS_HFAGG:
        case OPS_FAGG:
        case OPS_RSAGG:
        case OPS_SAGG:
        case OPS_MAIN:
        case OPS_UNION:
            break;
            default:
            opx_error( E_OP0280_SCOPE); /* expecting aggregate or main
                                    ** query to resolve scope */
        }
#endif
        if (!BTtest((i4)varno, (char *)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm))
        {
            PST_QNODE       *corelated;
            bool            found;
            /* look for corelated variable before adding to the list */
            found = FALSE;
            for (corelated = father->ops_agg.opa_corelated;
                corelated; corelated = corelated->pst_left)
            {
                if ((corelated->pst_sym.pst_value.pst_s_var.pst_vno == varno)
                    &&
                    (corelated->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
                    ==
                    varnodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id))
                {
                    found = TRUE;
                    break;          /* this corelated attribute was already
                                    ** added to the by list */
                }
            }
            if (found)
                continue;           /* avoid adding twice */
            corelated = opv_varnode(global, &varnodep->pst_sym.pst_dataval,
                (OPV_IGVARS)varnodep->pst_sym.pst_value.pst_s_var.pst_vno,
                &varnodep->pst_sym.pst_value.pst_s_var.pst_atno);
            MEcopy((PTR)&varnodep->pst_sym.pst_value.pst_s_var.pst_atname,
                sizeof(varnodep->pst_sym.pst_value.pst_s_var.pst_atname),
                (PTR)&corelated->pst_sym.pst_value.pst_s_var.pst_atname); /* init 
                                    ** attribute name */
            corelated->pst_left = father->ops_agg.opa_corelated;
            father->ops_agg.opa_corelated = corelated;
        }
        else
            return;                 /* corelated variable found in from
                                    ** list so return */
    }
}

/*{
** Name: opa_byhead     - process the byhead node
**
** Description:
**      The routine will process a byhead node, by making a recursive call 
**      This is not an inline routine so avoid defining local variables in the
**      opa_generate routine
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-jan-89 (seputis)
**          initial creation
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
[@history_template@]...
*/
static VOID
opa_byhead(
        OPA_RECURSION      *gstate,
        PST_QNODE          **agg_qnode)
{   /* update the bit map for the by list of the aggregate */
    OPV_GBMVARS     *oldmap;
    oldmap = gstate->opa_gvarmap;
    gstate->opa_gvarmap = &gstate->opa_gfather->ops_agg.opa_blmap;
    gstate->opa_demorgan = FALSE;   /* just in case */
    opa_generate(gstate, &(*agg_qnode)->pst_left);
    BTor(OPV_MAXVAR, (char *)gstate->opa_gvarmap, (char *)oldmap);
                                /* update the subquery
                                ** bit map also */
    gstate->opa_gvarmap = oldmap;       /* restore old map */
    gstate->opa_demorgan = FALSE;   /* just in case */
    opa_generate(gstate, &(*agg_qnode)->pst_right);
}

/*{
** Name: opa_presdom    - process resdom node
**
** Description:
**      Process the resdom node, but do not call any recursive routines 
**      to avoid stack overflow for 300 column tables 
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**      Returns:
**          {@return_description@}
**      Exceptions:
**          [@description_or_none@]
**
** Side Effects:
**          none
**
** History:
**      19-jan-89 (seputis)
**          initial creation
**      30-aug-89 (seputis)
**          changed so that resdom numbers are monotically decreasing
**          for OPC
**      06-mar-96 (nanpr01)
**          Do not check the maxtuple size with the ops_width.
**          Increase tuple size project.
[@history_template@]...
*/
static VOID
opa_presdom(
        OPA_RECURSION      *gstate,
        PST_QNODE          *q)
{
    OPS_SUBQUERY        *father;

    father = gstate->opa_gfather;        
    father->ops_width += q->pst_sym.pst_dataval.db_length; /*add 
                                    ** datatype length to total */
    /* make sure the aggregate does not exceed max dimensions */
    if (father->ops_width > father->ops_global->ops_cb->ops_server->opg_maxtup)
        opx_error( E_OP0200_TUPLEOVERFLOW );
    if (gstate->opa_gaggregate)
    {   
        OPZ_IATTS       resnum;     /* resdom number to use for
                                    ** initializing the node */
        if (q->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
        {
            resnum = q->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
            if (!father->ops_agg.opa_attlast)
            {   /* this is the first resdom so if the parser initialized
                ** everything correctly then this should be the highest
                ** numbered resdom, with everything subsequent being decreasing 
                ** which is required by OPC */
                father->ops_agg.opa_attlast = resnum;
            }
            if (!q->pst_left ||(q->pst_left->pst_sym.pst_type != PST_RESDOM))
            {
                if (resnum != 1)
                    opx_error(E_OP0284_RESDOM); /* resdom numbers should be monotonically
                                    ** decreasing and end at one */
            }
            else
            {   /* the child resdom must be one less than the current resdom */
                if ((resnum-1) != q->pst_left->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
                    opx_error(E_OP0284_RESDOM); /* resdom numbers should be monotonically
                                    ** decreasing and end at one */
            }
        }
        else
        {
            resnum = 0;             /* this resdom needs to be assigned
                                    ** later in OPAFINAL after all printing
                                    ** resdoms are assigned */
            q->pst_sym.pst_value.pst_s_rsdm.pst_rsno = resnum;
        }
        q->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO; /* relabel for
                                    ** OPC */
        /* assign resdom numbers to attribute in temporary relations
        ** created for function aggregate */
        q->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =  resnum; /* this
                                ** should always be a copy of
                                ** pst_rsno for OPC */
    }
}

/*{
** Name: opa_var        - process var node 
**
** Description:
**      Routine used to process varnode... not inline since this will reduce 
**      stack usage during recursion. 
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-jan-89 (seputis)
**          initial creation
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
[@history_template@]...
*/
static VOID
opa_var(
        OPA_RECURSION         *gstate,
        PST_QNODE          **agg_qnode)
{
    /* given this var node, initialize the corresponding
    ** entry in the global range table - given the parser's range 
    ** variable number 
    */
    OPV_IGVARS          varno;
    PST_QNODE           *q;
    OPS_STATE           *global;
    OPS_SUBQUERY        *father;

    global = gstate->opa_global;
    q = *agg_qnode;
    father = gstate->opa_gfather;
    varno = (OPV_IGVARS) q->pst_sym.pst_value.pst_s_var.pst_vno;
    if (BTtest ((i4)varno, (char *)&global->ops_astate.opa_allviews)
        )
    {
        bool        demorgan;
        OPV_IGVARS  old_viewid;     /* temp to save the state of the
                                    ** viewid prior to substitution */
        BTset((i4)varno, (char *)&father->ops_agg.opa_rview); /* update 
                                    ** the variable map
                                    ** for any view referenced
                                    ** directly in the query */
        opa_sview(global, agg_qnode);   /* view has been found so
                                    ** substitute the target
                                    ** list item */
        if (gstate->opa_joinid >= 1)
            opa_ojsub(gstate->opa_joinid, *agg_qnode); /* substitute
                                    ** the joinid for all PST_OP_NODE types
                                    ** in this expression */
        demorgan = gstate->opa_demorgan; /* do not push nots past var nodes */
        gstate->opa_demorgan = FALSE;
        old_viewid = global->ops_astate.opa_viewid; /* save old view id and
                                    ** restore for parent */
        global->ops_astate.opa_viewid = varno; /* mark the aghead nodes
                                    ** with the correct viewid */
        opa_generate(gstate, agg_qnode);
        global->ops_astate.opa_viewid = old_viewid; /* restore the parent's
                                    ** viewid */
        gstate->opa_demorgan = demorgan; /* restore state */
        return;
    }
    if (!(global->ops_gmask & OPS_QUERYFLATTEN)
        &&
        !BTtest((i4)varno, 
            (char *)&father->ops_root->pst_sym.pst_value.pst_s_root.pst_tvrm)
        )
        /* if the variable is not in the from list and flattening is enabled
        ** then the variable must be made visible in a by list for all the
        ** parent aggregates until the corelation is found */
        opa_covar(father, q, varno);
    BTset((i4)varno, (char *)gstate->opa_gvarmap); /* update the variable map
                                    ** for the subquery */
    (VOID) opv_parser(global, varno, OPS_MAIN, TRUE, TRUE, TRUE);
}

/*{
** Name: opa_or_null    - create an OR with an IS NULL node
**
** Description:
**      Used to support flattening of NOT EXISTS or != ALL queries 
**
** Inputs:
**      gstate                          state of aggregate processing
**      qnode                           expression to test for IS NULL
**      targetpp                        insertion point of IS NULL
**
** Outputs:
**
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-nov-90 (seputis)
**          initial creation
**      31-dec-90 (seputis)
**          added nullop, and_or to help avoid cart prod with != ALL
**      20-nov-92 (ed)
**          - fix outer join problem, which looks like uninitialized
**          variable, but actually is caused by assuming node is an OP
**          type but instead it really is a PST_QLEND type
[@history_template@]...
*/
static VOID
opa_or_null(
        OPA_RECURSION      *gstate,
        PST_QNODE          *qnode,
        PST_QNODE          **targetpp,
        i4                 and_or,
        ADI_OP_ID          nullop,
        PST_J_ID           ojid)
{   /* add an IS NULL test to the expression */
    PST_QNODE       *or_node;
    PST_QNODE       *is_null;
    OPS_STATE       *global;

    global = gstate->opa_global;
    if ((*targetpp)->pst_sym.pst_type != PST_QLEND)
    {
        ojid = (*targetpp)->pst_sym.pst_value.pst_s_op.pst_joinid;
        or_node = opv_opnode(global, and_or, (ADI_OP_ID)0, ojid);
        or_node->pst_left = *targetpp;
        *targetpp = or_node;
    }
    is_null = opv_opnode(global, PST_UOP, nullop, ojid);
    is_null->pst_left = qnode;
    opa_resolve( gstate->opa_gfather, is_null);
    if ((*targetpp)->pst_sym.pst_type != PST_QLEND)
        or_node->pst_right = is_null;
    else
        *targetpp = is_null;
}

/*{
** Name: opa_ornull_list        - generate OR's of all NULL
**
** Description:
**      The ALL select is simulated by an ANY aggregate, but the arithmetic for 
**      NULLs is special,...  i.e. r.a = ALL (select s.a...   is translated
**      to an r.a != s.a which is still FALSE for a NULL, so if the expression 
**      is NULLable it needs to be traversed to include all NULLs for the key
**      expression... i.e.  r.a != s.a OR s.a is NULL OR r.a is NULL
**
** Inputs:
**      gstate                          ptr to traversal state
**      qnode                           current node being analyzed
**      ornodepp                        ptr to insertion point of OR's
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      19-apr-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opa_ornull_list(
        OPA_RECURSION      *gstate,
        PST_QNODE          *qnode,
        PST_QNODE          **ornodepp)
{
    for(;qnode; qnode = qnode->pst_left)
    {
        if ((qnode->pst_sym.pst_type == PST_VAR)
            &&
            (qnode->pst_sym.pst_dataval.db_datatype < 0)
            )
        {   /* found a var node so generate a NULL check */
            PST_QNODE       *or_nodep;
            bool            not_found;

            not_found = TRUE;
            for (or_nodep = *ornodepp; 
                or_nodep->pst_sym.pst_type == PST_OR; 
                or_nodep = or_nodep->pst_left)
            {
                if ((   qnode->pst_sym.pst_value.pst_s_var.pst_vno
                        ==
                        or_nodep->pst_right->pst_sym.pst_value.pst_s_var.pst_vno
                    )
                    &&
                    (   qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
                        ==
                        or_nodep->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id
                    ))
                {
                    not_found = FALSE;      /* this attribute is already in the NULL
                                            ** list */
                    break;
                }
            }
            if (not_found)
            {
                PST_QNODE   *varp;

                varp = opv_varnode( gstate->opa_global, &qnode->pst_sym.pst_dataval, 
                    (OPV_IGVARS)qnode->pst_sym.pst_value.pst_s_var.pst_vno, 
                    &qnode->pst_sym.pst_value.pst_s_var.pst_atno);

                opa_or_null(gstate, varp, ornodepp, PST_OR,
                    gstate->opa_global->ops_cb->ops_server->opg_isnull,
                    or_nodep->pst_sym.pst_value.pst_s_op.pst_joinid);
            }
        }
        opa_ornull_list (gstate, qnode->pst_right, ornodepp);
    }
}

/*{
** Name: opa_bop        - process binary operator
**
** Description:
**      Process a BOP node during initial pass. 
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
**      Returns:
**          TRUE - if the children of the node should not be processed;
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-jan-89 (seputis)
**          initial creation
**      1-sep-89 (seputis)
**          add the NULL or list for ALL subselect handling to 
**          fix the "Chris Date" bug
**      20-nov-90 (seputis)
**          - fix b34433, place IS NULL test on entire expression
**      4-dec-02 (inkdo01)
**          Changes for range table expansion.
**	6-dec-2006 (dougi)
**	    Don't flatten scalar subselects - it undermines cardinality
**	    checking.
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects will already have been flattened if in
**	    target list and in that case the joining BOPs will be marked
**	    with PST_CARD_01R but without a PST_SUBSEL node as it'll
**	    be flat already.
**	    Also don't process join id if not a comparison operator.
*/
static bool
opa_bop(
        OPA_RECURSION   *gstate,
        PST_QNODE       **agg_qnode)
{   /* check for subselect and remove it if only simple
    ** aggregates are computed, this should work for simple
    ** aggregates and corelated simple aggregates 
    ** FIXME - OPF will not */
    PST_QNODE   *q;
    OPS_STATE   *global;

    global = gstate->opa_global;
    q = *agg_qnode;
    
    if (q->pst_sym.pst_value.pst_s_op.pst_fdesc &&
	q->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fitype == ADI_COMPARISON)
    {
	if (gstate->opa_joinid == OPL_NOINIT)
	    /* Get joinid to be used for view substitution */
	    gstate->opa_joinid = q->pst_sym.pst_value.pst_s_op.pst_joinid;
	else if (gstate->opa_joinid != q->pst_sym.pst_value.pst_s_op.pst_joinid)
	    /* Check that joinid's are consistent with other nodes */
	    opx_error(E_OP039F_BOOLFACT);
    }
    if (gstate->opa_demorgan)
    {
        gstate->opa_demorgan = FALSE;
        opa_not(global, q);
    }
/*
**          OPC needs to be able to handle correlated aggregates before this
**          flattened code can be enabled, i.e. if the subselect is eliminated
**          and the target list has an aggregate which is correlated, then
**          OPC requires will access violate e.g.
**
**          select r.a from r where 
**          r.a > ANY (select avg(s.a) from s where s.a=r.b)
*/
    if ( !(global->ops_gmask & OPS_QUERYFLATTEN)
        ||
        gstate->opa_gflatten)
    {
        switch (q->pst_sym.pst_value.pst_s_op.pst_opmeta)
        {
        case PST_CARD_01R:
	    /* We allow these to be folded on the assumption that the
	    ** join operators will check cardinality where needed */
	    if (q->pst_right->pst_sym.pst_type == PST_SUBSEL)
	    {
		if (opa_flatten(gstate, q->pst_right, agg_qnode, q))
		    /* FIXME - need to return nestcnt here from the
		    ** flattening code, since currently it is updated
		    ** directly, or another approach would be to increment
		    ** opa_nestcnt */
		    return(TRUE);
	    }
	    break;
        case PST_CARD_01L:
	    /* We allow these to be folded on the assumption that the
	    ** join operators will check cardinality where needed */
	    if (q->pst_left->pst_sym.pst_type == PST_SUBSEL)
	    {
		if (opa_flatten(gstate, q->pst_left, agg_qnode, q))
		    /* FIXME - need to return nestcnt here from the
		    ** flattening code, since currently it is updated
		    ** directly, or another approach would be to increment
		    ** opa_nestcnt */
		    return(TRUE);
	    }
	    break;
        case PST_CARD_ANY:
        {   /* check for = ANY subselect just below an AND node and
            ** flatten query, FIXME, if it is below an OR node then
            ** the conjunct needs to be normalized after flattening, and
            ** this may cause a cartesean product where there was not
            ** one before so look at the OR case more carefully */
            /* the ANY subselect can be flattened if TIDs are brought
            ** up for the parent query */

            if (opa_flatten(gstate, q->pst_right, agg_qnode, q))
            {
                /* FIXME - need to return nestcnt here from the
                ** flattening code, since currently it is updated
                ** directly, or another approach would be to increment
                ** opa_nestcnt */
                return(TRUE);
            }
            break;
        }
        case PST_CARD_ALL:
        {   /* convert the ALL into a non-existence aggregate ANY */
            PST_QNODE       *and_node;
            PST_QNODE       *exists_node;
            PST_QNODE       *null_target;
            i4      original_mask;
            if (global->ops_gmask & OPS_QUERYFLATTEN)
                break;
            exists_node = opv_opnode(global, PST_UOP, 
                (ADI_OP_ID)global->ops_cb->ops_server->opg_nexists,
                (OPL_IOUTER)q->pst_sym.pst_value.pst_s_op.pst_joinid);
            exists_node->pst_left = q->pst_right; /* assigns subselect part
                                ** to exists node */
            and_node = opv_opnode(global, PST_AND, (ADI_OP_ID)0,
                (OPL_IOUTER)q->pst_sym.pst_value.pst_s_op.pst_joinid);
            and_node->pst_right = q->pst_right->pst_right; /* get the qual
                                ** part of the subselect */
            and_node->pst_left = q; /* the "key" for the subselect join
                                ** provides an extra qualification for
                                ** 0 = any aggregate */
            {   /* need to check if the "key" will make this a corelated
                ** subselect, in which case the corelated flag should be
                ** set in the root node */
                OPV_GBMVARS         tempmap;
                MEfill(sizeof(tempmap), (u_char)0, (PTR)&tempmap);
                opa_cosubquery(global, &tempmap, q);
                original_mask = exists_node->pst_left->pst_sym.pst_value.
                    pst_s_root.pst_mask1;
                if (BTnext((i4)-1, (char *)&tempmap, (i4)BITS_IN(tempmap)) >= 0)
                {   /* a corelation was definitely introduced so set the
                    ** root flag */
                    exists_node->pst_left->pst_sym.pst_value.pst_s_root.pst_mask1
                        |= (PST_1CORR_VAR_FLAG_VALID | PST_2CORR_VARS_FOUND);
                }
            }
            q->pst_right = q->pst_right->pst_left->pst_right; /* get the
                                ** resdom expression for the subselect */
            q->pst_sym.pst_value.pst_s_op.pst_opmeta = PST_NOMETA;
            opa_not(global, q); /* complement the operator for the
                                ** not existence check */
            opa_resolve( gstate->opa_gfather, q);       /* resolve PST_BOP operator, just in case
                                ** since a NULLable attribute is normally
                                ** returned from a subselect */
            if ((q->pst_right->pst_sym.pst_dataval.db_datatype < 0)
                &&
                (q->pst_sym.pst_value.pst_s_op.pst_opno == global->ops_cb->ops_server->opg_eq)
                )
            {
                /* if a NULL target list item exists, then a NULL check will
                ** need to be made, if an equi-join is possible then create
                ** another aggregate to avoid the cartesean product */
                null_target = exists_node;
                opv_copytree(global, &null_target);     /* get copy of existing tree */
                null_target->pst_left->pst_sym.pst_value.pst_s_root.pst_mask1
                    = original_mask;                    /* restore correlation mask
                                                        ** since key is not included
                                                        ** here */
            }
            else
                null_target = NULL;
            *agg_qnode = exists_node;
            exists_node->pst_left->pst_right = and_node; /* place modified
                                ** qualification here for an any aggregate */
            if (q->pst_sym.pst_dataval.db_datatype < 0)
#if 0
/* cannot special case EQUALs since this NULL check code should apply to all operators */
                &&
                (q->pst_sym.pst_value.pst_s_op.pst_opno != global->ops_cb->ops_server->opg_eq))
#endif
            {
                if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
                {
                    opa_ornull_list(gstate, q, &and_node->pst_left); /* check for nullable
                                ** attributes, since the expression is NULLable; note that
                                ** if the original expression of != then NULL will always satisfy
                                ** this, so that opg_eq, should not have the NULL check */
                }
                else
                {   /* if local then b34433 shows that a PST_VAR Node in the expression
                    ** can be nullable, but the final expression does not need to
                    ** be nullable, thus the IS NULL test should be applied to the
                    ** entire expression;  For distributed, since the SQL standard is
                    ** such at only a column is allowed for the IS NULL operator, the
                    ** opa_ornull_list will be used until "views" can be defined
                    ** which fix this IS NULL expression problem  FIXME */
                    PST_QNODE       *qnode;
                    if (q->pst_left->pst_sym.pst_dataval.db_datatype < 0)
                    {
                        /* check for the IS NOT NULL outside the aggregate so that
                        ** an equi-join can be used in the aggregate for != ALL */
                        qnode = q->pst_left;
                        opv_copytree(global, &qnode);   /* get copy of existing tree */
                        opa_or_null(gstate, qnode, agg_qnode, PST_AND,
                            global->ops_cb->ops_server->opg_isnotnull,
                            PST_NOJOIN); 
                                                        /* insert is not null expression */
                    }
                    if (null_target)
                    {
                        /* take the copy of the NOT EXISTS node and check for NULLability
                        ** of the target list of the subselect, this is only useful
                        ** when an equi-join is possible, otherwise performing the cartesean
                        ** product once is best */
                        PST_QNODE       *target;
                        qnode = null_target->pst_left->pst_left->pst_right;
                        opa_or_null(gstate, qnode, &null_target->pst_left->pst_right, PST_AND,
                            global->ops_cb->ops_server->opg_isnull,
                            PST_NOJOIN);
                        target = null_target->pst_left->pst_left;
                        target->pst_right = opv_intconst(global, 1);
                                                        /* insert "is null" test */
                        STRUCT_ASSIGN_MACRO(target->pst_right->pst_sym.pst_dataval,
                            target->pst_sym.pst_dataval); /* make sure target list
                                            ** and resdom match */
                        /* insert the aggregate into the qualification */
                        and_node = opv_opnode(global, PST_AND, (ADI_OP_ID)0,
                            (OPL_IOUTER)q->pst_sym.pst_value.pst_s_op.pst_joinid);
                        and_node->pst_right = null_target; /* the NULL test for the
                                            ** subselect */
                        and_node->pst_left = *agg_qnode; /* the NOT EXISTS aggregate
                                            ** with inverted key */
                        *agg_qnode = and_node;
                    }
                    else if (q->pst_right->pst_sym.pst_dataval.db_datatype < 0)
                    {   /* add a test for the NULL values in the target list of the
                        ** select, by adding another aggregate; which is a simple
                        ** aggregate if the subselect is not correlated, FIXME - this
                        ** may not be the best guess if another variable is correlated
                        ** and provides the keying necessary to avoid a relation scan as
                        ** select r.a from r where r.a != ALL (select s.a from s where s.b=r.b)
                        ** so that if keying is provided by s.b then the r.a=s.a equi-join may
                        ** not be useful, and a scan of s will be required */
                        qnode = q->pst_right;
                        opv_copytree(global, &qnode);   /* get copy of existing tree */
                        opa_or_null(gstate, qnode, &and_node->pst_left, PST_AND,
                            global->ops_cb->ops_server->opg_isnull, PST_NOJOIN);
                                                        /* insert null expression */
                    }
                    opa_generate(gstate, agg_qnode);
                    return(TRUE);
                }
            }
            return(opa_exists(gstate, agg_qnode));
            break;
        }
        default:
            break;
        }
    }
    return(FALSE);
#if 0
    if (q->pst_sym.pst_value.pst_s_op.pst_opmeta != PST_NOMETA)
    {   /* subselect was found so check if it can be removed */
        PST_QNODE       *subsel;    /* subselect node */

        subsel = q->pst_right;
#ifdef    E_OP0280_SCOPE
        if ((subsel == NULL)
            ||
            (subsel->pst_sym.pst_type != PST_SUBSEL)
           )
            opx_error( E_OP0280_SCOPE); /* expecting subselect */
#endif
        if (!subsel->pst_right 
            || 
            subsel->pst_right->pst_sym.pst_type == PST_QLEND
            )
        {   /* there is no qualification so that this could
            ** not be function aggregate with a having clause 
            ** - look at the result expression and check for
            ** simple aggregates */
            bool        substitute; /* TRUE if substitution is OK */
            bool        agg_found;  /* TRUE if at least one 
                                    ** simple aggregate
                                    ** is found */

            substitute = TRUE;
            agg_found = FALSE;
            opa_sagg( father, subsel, subsel->pst_left->pst_right, &substitute,
                &agg_found);        /* this routine typically will
                                    ** not recurse too deeply and
                                    ** the subselect substitution is
                                    ** easier to understand if all
                                    ** operations are localized */
            if (substitute && agg_found)
            {   /* the aggregate can be substituted so remove
                ** the subselect and change the subselect
                ** operator */
                q->pst_sym.pst_value.pst_s_op.pst_opmeta
                    = PST_NOMETA;
                q->pst_right = subsel->pst_left->pst_right; /*
                                    ** remove the subselect
                                    ** structure */
                /* find all aggregates/subselects in subtree below 
                ** this node, so that subselect type can be determined
                **/
                if (q->pst_left)
                    nestcount = opa_generate(global, &q->pst_left);
                                    /* traverse left sub tree  */
                {
                    OPS_SUBQUERY    *delimiter; /* end of subquery list
                                        ** associated with subselect*/
                    OPS_SUBQUERY    *next_sq; /* used to traverse
                                        ** subquery list associated
                                        ** with subselect */
                    OPV_GBMVARS     scope; /* bit map of vars visible
                                        ** at this scope level and
                                        ** above */
                    OPV_GBMVARS     tempmap;
                    delimiter = global->ops_subquery; /*
                                    ** this will mark the end of the
                                    ** list of new subqueries added 
                                    ** because of the subselect */
                    if (q->pst_right)
                        nestcount += opa_generate(gstate, &q->pst_right);
                                    /* traverse right sub tree */
                    MEcopy((char *)&father->ops_root->pst_sym.pst_value.
                        pst_s_root.pst_tvrm, sizeof(scope), (char *)&scope);
                    BTor(OPV_MAXVAR, (char *)&father->ops_correlated,
                        (char *)&scope);
                                    /* check if subselect is correlated
                                    ** by seeing if vars in the from
                                    ** list or corelated list are
                                    ** referenced */
                    for (next_sq = global->ops_subquery;
                        next_sq != delimiter; 
                        next_sq = next_sq->ops_next)
                    {
                        MEcopy((char *)&scope, sizeof(tempmap), (char *)&tempmap);
                        BTand(OPV_MAXVAR, (char *)&next_sq->ops_correlated,
                                (char *)&tempmap);
                        if (BTcount((char *)&tempmap, OPV_MAXVAR))
                        {   /* correlated child found so need to 
                            ** keep PST_BOP node marked as a special
                            ** subselect */
                            q->pst_sym.pst_value.pst_s_op.pst_opmeta
                                = PST_CARD_ANY;
                            break;
                        }
                    }
                }
                return(nestcount);
            }
        }
    }
#endif
}

/*{
** Name: opa_uop        - process the unary operator node
**
** Description:
**      Extracted from opa_generate to keep the recursive variable usage 
**      low 
**
** Inputs:
**      gstate                          ptr to query tree processing state
**
** Outputs:
**      Returns:
**          TRUE - means all the children of this node have been process
**	    FALSE - means allow caller to handle left (only) tree
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      20-jan-89 (seputis)
**          initial creation
**      9-feb-93 (ed)
**          - fix b49317 - access violation when outer joins are flattened
**	21-May-2010 (kiria01) b123786
**	    Do not restore .opa_demorgan to TRUE once any negation applied.
**	    The bug only arises if there is a LHS tree with a non-terminal
**	    such as an operator, which tries to apply the opa_not again.
**	    This might seem more likely to happen but it needs both a boolean
**	    UOP (IS NULL, IS DECIMAL etc) AND the parameter must be an expression
[@history_template@]...
*/
static bool
opa_uop(
        OPA_RECURSION       *gstate,
        PST_QNODE          **agg_qnode)
{
    PST_QNODE       *q = *agg_qnode;
    OPS_STATE       *global = gstate->opa_global;
    bool            ret_val = FALSE;

    if (gstate->opa_joinid == OPL_NOINIT)
        gstate->opa_joinid = q->pst_sym.pst_value.pst_s_op.pst_joinid; /* get
                                        ** joinid to be used for view
                                        ** substitution */
    else if (gstate->opa_joinid != q->pst_sym.pst_value.pst_s_op.pst_joinid)
    {
	if (q->pst_sym.pst_value.pst_s_op.pst_opno != ADI_UNORM_OP)
	    opx_error(E_OP039F_BOOLFACT);  /* check that joinid's are consistent
                                        ** with other nodes */
    }

    if (gstate->opa_demorgan)
    {
	/* We have a pending NOT to apply which should amount to
	** switching the operator to its complement.
	/* EXISTS requires special handling because ADF doesn't define
        ** any functions for it - specifically there is no fdesc to
	** hold the complement and so we apply the pending NOT now
	** outside the more general opa_not() routine.
        */
        gstate->opa_demorgan = FALSE;
        if (    q->pst_sym.pst_value.pst_s_op.pst_opno 
                == 
                global->ops_cb->ops_server->opg_exists)
            q->pst_sym.pst_value.pst_s_op.pst_opno
                = global->ops_cb->ops_server->opg_nexists;
        else if (q->pst_sym.pst_value.pst_s_op.pst_opno 
                == 
                global->ops_cb->ops_server->opg_nexists)
            q->pst_sym.pst_value.pst_s_op.pst_opno
                = global->ops_cb->ops_server->opg_exists;
        else
	{
            opa_not(global, q); /* complement the operator
                                ** to complete application of
                                ** demorgan's law */
	}
	/* As we have now applied the NOT, .opa_demorgan 
	** should remain FALSE for the immediate sub-tree. */
    }
    if (    (q->pst_sym.pst_value.pst_s_op.pst_opno 
            == 
            global->ops_cb->ops_server->opg_exists)
        ||
            (q->pst_sym.pst_value.pst_s_op.pst_opno 
            == 
            global->ops_cb->ops_server->opg_nexists)
        )
    {
        /* a SQL EXISTS node has been found so translate */
        ret_val = opa_exists(gstate, agg_qnode);
                                /* check for EXISTS operator which
                                ** needs to be processed, either by
                                ** flattening the query or using
                                ** an ANY aggregate computation */
        if (*agg_qnode          /* outer join may remove subtree
                                ** entirely */
            &&
            ((*agg_qnode)->pst_sym.pst_type == PST_BOP)
                                /* EXISTS translation may create a
                                ** PST_BOP operator which needs to be
                                ** processed */
            &&
            opa_bop(gstate, agg_qnode) /* if a BOP was created then 
                                ** process it, making sure if flattening
                                ** occurs then TRUE is returned which means
                                ** return to the parent node since the
                                ** children have been processed */
            )
            ret_val = TRUE;     /* return if flattening has occurred in
                                ** which case the children have already
                                ** been processed */
    }
    return(ret_val);
}

/*{
** Name: opa_const      - process constant node for first time
**
** Description:
**      This routine was created to reduce stack usage in the main 
**      opa_generate routine 
**
** Inputs:
**      gstate                          state of first query tree pass
**      agg_qnode                       constant node
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      28-mar-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opa_const(
        OPA_RECURSION       *gstate,
        PST_QNODE          **agg_qnode)
{
    PST_QNODE       *qnodep;
    OPS_STATE       *global;

    global = gstate->opa_global;
    qnodep = *agg_qnode;
    /* check for repeat query parameters */
    if (qnodep->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
        opv_rqinsert(global, 
            qnodep->pst_sym.pst_value.pst_s_cnst.pst_parm_no, 
            qnodep);    /* insert repeat query
                                ** node into list for OPC */
    else 
    {
        if (global->ops_cb->ops_check 
            &&
            opt_strace( global->ops_cb, OPT_F043_CONSTANTS))
            qnodep->pst_sym.pst_value.pst_s_cnst.pst_origtxt
                = (char *)NULL; /* used to test ~V case for distributed
                                ** since it is hard to produce in
                                ** terminal monitor */
        if ((!qnodep->pst_sym.pst_value.pst_s_cnst.pst_origtxt)
            &&
            (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
            )
            opv_uparameter(global, qnodep); /* if 
                                ** the original text does
                                ** not exist then assign a
                                ** parameter number to it since
                                ** it is a constant user parm */
    }
}

/*{
** Name: opa_generate   - create list of aggregates from the query
**
** Description:
**      This routine is only interested in the aggregate structure of the
**      query i.e. what does the query look like after all other node
**      types are stripped away and only PST_AGHEAD , PST_ROOT nodes are left
**
**      This routine will build a list of "subquery elements" representing
**      the query subtree represented by the aggregate or root node.
**      The list of subqueries is ordered by leftmost innermost first.
**      Thus, if the subqueries are evaluted in this order the results
**      of all inner aggregates will be available prior to evaluating the
**      outer aggregate.
**
**      Note that the optimizer is very sensitive to this order that the
**      subquery list is generated.
**
**      set trace point op132
**      When the flattening code is enabled, all subselects are eliminated
**      and all corelations are eliminated since OPC/QEF cannot deal with
**      all configurations so it safest to either flatten everything or
**      nothing.
**
** Inputs:
**      global                          state variable for optimization
**         ->ops_subquery               root of list of subqueries found
**                                      so far
**      agg_qnode                       ptr to ptr to the aghead node
**                                      - this value is saved in the subquery
**                                      and is eventually used for a 
**                                      "subsitution" of the aggregate result
**      father                          ptr to outer aggregate for this subtree
**                                      or NULL if root node is outer aggregate
**
** Outputs:
**      global                        
**         ->ops_subquery               new subqueries added to this list
**                                      representing all aggregates contained
**                                      within the parent query tree
**      global
**         ->ops_range_tab              initialized corresponding elements to
**                                      parser range table
**      Returns: The number of aggregates directly reachable from this node i.e.
**          number of inner aggregates within this subtree
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-feb-86 (seputis)
**          initial creation from findagg
**      16-aug-88 (seputis)
**          relabelled resdoms to PST_ATTNO for OPC
**      4-dec-88 (seputis)
**          introduced trace flag to enable some flattening code
**      31-jan-89 (seputis)
**          changed to eliminate large stack usage on 300 column target
**          lists
**      3-mar-89 (seputis)
**          fixed problem with NOT for 300 column rewrite
**      18-jan-93 (ed)
**          fix outer join problem in which consistency check was incorrectly
**          reported indicating a conjunct had more than one join id
**      21-jun-93 (ed)                                           
**          eliminate joinid test for PST_NOT node since this is an
**          operator node and has no joinid.                     
**	16-jan-06 (hayke02)
**	    We now don't set opa_gflatten FALSE if either side of the OR
**	    node is a PST_BOP node. This prevents cart prod join nodes for
**	    the projection of unflattened OR'ed PST_BOP subselects.
**	    This change fixes bug 113941.
**      30-may-01 (inkdo01)
**          Change all "count(column)" f.i.'s to ADFI_090_COUNT_C_ALL so parse
**          trees from earlier releases generate right code.
[@history_template@]...
*/
static VOID
opa_generate(
        OPA_RECURSION       *gstate,
        PST_QNODE          **agg_qnode  /* ptr to ptr to query tree node to be 
                                        ** analyzed
                                        */ )
{
    /* do not define local variable in this routine to avoid recursion
    ** problems for long resdom lists etc. */
    for(;*agg_qnode; agg_qnode = &(*agg_qnode)->pst_left)
    {
        switch ((*agg_qnode)->pst_sym.pst_type)
        {
        case PST_AGHEAD:
        {
            gstate->opa_global->ops_qheader->pst_agintree = TRUE;
            opa_rnode(gstate, agg_qnode);
            gstate->opa_nestcount += 1;
            return;         
        }
        case PST_SUBSEL:
        {
#if 0
	    if (!(global->ops_gmask & OPS_QUERYFLATTEN) ||
		gstate->opa_gflatten)
	    {
		PST_QNODE *conjunct =;
		if (opa_flatten(gstate, (*agg_qnode)->pst_right, agg_qnode, NULL))
		    return;
	    }
#endif
            gstate->opa_global->ops_qheader->pst_subintree = TRUE;
            opa_rnode(gstate, agg_qnode);
            gstate->opa_nestcount += 1;
            return;
        }
        case PST_ROOT:
        {   /* process root node */
            opa_rnode(gstate, agg_qnode);
            gstate->opa_nestcount += 1;
            return;
        }
        case PST_OR:
        {
            if (gstate->opa_joinid == OPL_NOINIT)
                gstate->opa_joinid = (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid; /* get
                                                ** joinid to be used for view
                                                ** substitution */
            else if (gstate->opa_joinid != (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid)
                opx_error(E_OP039F_BOOLFACT);   /* check that joinid's are consistent
                                                ** with other nodes */
	    if (gstate->opa_demorgan)
		/* NOT (A OR B) ==> ((NOT A) AND (NOT B)) */
		(*agg_qnode)->pst_sym.pst_type = PST_AND;
	    else
	    {
		if ((*agg_qnode)->pst_left->pst_sym.pst_type != PST_BOP
		    &&
		    (*agg_qnode)->pst_right->pst_sym.pst_type != PST_BOP)
		    gstate->opa_gflatten = FALSE; /* any subsequent flattening
				    ** will occur below an OR which could
				    ** cause the sybase bug to occur
				    ** also, a cartesean product could result if
				    ** flattening occurred, currently flattening
				    ** is done anyways */
	    }
	    break;
	}
	case PST_AND:
	{
	    if (gstate->opa_demorgan)
	    {
		/* NOT (A AND B) ==> ((NOT A) OR (NOT B)) */
		(*agg_qnode)->pst_sym.pst_type = PST_OR;
		if ((*agg_qnode)->pst_left->pst_sym.pst_type != PST_BOP
		    &&
		    (*agg_qnode)->pst_right->pst_sym.pst_type != PST_BOP)
		    gstate->opa_gflatten = FALSE; /* any subsequent flattening
				    ** will occur below an OR which could
				    ** cause the sybase bug to occur
				    ** also, a cartesean
				    ** product may result, currently flattening
				    ** is done anyways */
	    }
            else /*  if (gstate->opa_gflatten) */
                gstate->opa_joinid = OPL_NOINIT; /* for some "and nodes" the joinid
                                    ** is indeterminate and should be ignored
                                    ** but those below "OR"s and "NOT"s are
                                    ** determinate */
            break;
        }
        case PST_NOT:
        {   /* apply DeMorgan's laws to remove PST_NOT operator */
            gstate->opa_demorgan = (gstate->opa_demorgan == FALSE); /* a not 
                                        ** pushed onto a not
                                        ** cancels both, otherwise
                                        ** push the not */
            *agg_qnode = (*agg_qnode)->pst_left; /* Skip NOT node */
            if ((*agg_qnode)->pst_sym.pst_type != PST_NOT)
            {   /* PST_NOT is not a pst_op_node type */
                if (gstate->opa_joinid == OPL_NOINIT)
                    gstate->opa_joinid = (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid; /* get
                                                    ** joinid to be used for view
                                                    ** substitution */
                else if (gstate->opa_joinid != (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid)
                    opx_error(E_OP039F_BOOLFACT);   /* check that joinid's are consistent
                                                    ** with other nodes */
            }
            if (gstate->opa_demorgan)
            {   
                opa_generate(gstate, agg_qnode);
                gstate->opa_demorgan = FALSE; /* need to restore demorgan state 
                                        ** for the parent - do not declare
                                        ** local variable to keep recursion
                                        ** stack usage low */
            }
            else
            {   /* need to restore demorgan state for the parent */
                opa_generate(gstate, agg_qnode);
                gstate->opa_demorgan = TRUE; /* need to restore demorgan state 
                                        ** for the parent - do not declare
                                        ** local variable to keep recursion
                                        ** stack usage low */
            }
            return;
        }
        case PST_UOP:
        {
            if (opa_uop(gstate, agg_qnode))
                return;                 /* EXISTS translation may create a
                                        ** PST_BOP operator which needs to be
                                        ** processed */
            break;
        }
        case PST_BOP:
        {
            if (opa_bop(gstate, agg_qnode))
                return;                 /* return if flattening has occurred in
                                        ** which case the children have already
                                        ** been processed */
            break;
        }
        case PST_VAR:
        {
            opa_var(gstate, agg_qnode);
            return;
        }
        case PST_RESDOM:
        {
            opa_presdom(gstate, *agg_qnode); /* call this routine to avoid declaration of
                                            ** local variables which may cause stack overflow
                                            */
            break;
        }
        case PST_BYHEAD:
        {
            opa_byhead(gstate, agg_qnode);
            return;
        }
        case PST_CONST:
        {   
            opa_const(gstate, agg_qnode);
            break;
        }
        case PST_AOP:
        {
            if ((*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_opno == ADI_CNT_OP)
                (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_opinst = 90;
                                                /* all count(col)'s are now 
                                                ** ADFI_090_COUNT_ALL */
        }       /* then drop into PST_COP */
        case PST_COP:
        {
            if (gstate->opa_joinid == OPL_NOINIT)
                gstate->opa_joinid = (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid; /* get
                                                ** joinid to be used for view
                                                ** substitution */
            else if (gstate->opa_joinid != (*agg_qnode)->pst_sym.pst_value.pst_s_op.pst_joinid)
                opx_error(E_OP039F_BOOLFACT);   /* check that joinid's are consistent
                                                ** with other nodes */
            break;
        }
        }


        if ((*agg_qnode)->pst_right)
        {   /* store the boolean states in "if statements" so that stack will not
            ** be used for this recursive routine, i.e. do not use local boolean
            ** variables here */
            if (gstate->opa_demorgan)
            {
                if (gstate->opa_gflatten)
                {
                    opa_generate(gstate, &(*agg_qnode)->pst_right); /* recurse down right sub tree */
                    gstate->opa_gflatten = TRUE;
                }
                else
                {
                    opa_generate(gstate, &(*agg_qnode)->pst_right); /* recurse down right sub tree */
                    gstate->opa_gflatten = FALSE;
                }
                gstate->opa_demorgan = TRUE;
            }
            else
            {
                if (gstate->opa_gflatten)
                {
                    opa_generate(gstate, &(*agg_qnode)->pst_right); /* recurse down right sub tree */
                    gstate->opa_gflatten = TRUE;
                }
                else
                {
                    opa_generate(gstate, &(*agg_qnode)->pst_right); /* recurse down right sub tree */
                    gstate->opa_gflatten = FALSE;
                }
                if ((*agg_qnode)->pst_sym.pst_type == PST_AND)
                    gstate->opa_joinid = OPL_NOINIT;    /* and node joinid is indeterminate
                                                        ** if contained in a conjunct which is
                                                        ** not under an OR or NOT */
                gstate->opa_demorgan = FALSE;
            }
        }
    }
}

/*{
** Name: OPA_1PASS      - first pass of parse tree to gather information
**
** Description:
**      This routine initialize OPS_GENSTATE control block for the first 
**      pass of the query tree, this is an information gathering pass. 
**
** Inputs:
**      global                          ptr to state of query processing
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      19-jan-89 (seputis)
**          initial creation
**      27-sep-89 (seputis)
**          consolidate all info about flattening into one flag
**      30-apr-90 (seputis)
**          look for PSF hints on when to flatten the query,
**          currently PSF indicates to avoid flattening if an
**          OR subselect , or an ALL subselect is scanned
**      28-jan-91 (seputis)
**          add support for OPF AGGREGATE_FLATTEN flag
**      04-mar-94 (davebf)
**          check for singlesite distributed for flattening decision.
**      10-aug-99 (inkdo01)
**          Add support for NOT EXISTS to outer join transformation.
**      1-sep-99 (inkdo01)
**          Slight change to OJ transform to do it only if "noflatten" NOT set.
**      30-aug-01 (hayke02)
**          Set OPS_SUBSELOJ if we have transformed a 'not exists'/'not in'
**          subselect into an outer join.
**      19-apr-02 (wanfr01)
**	    Bug 107345, INGSRV1726
**	    Reset OPS_SUBSELOJ if the flag does not apply. 
**	21-mar-03 (hayke02)
**	    Check for OPF_OLDSUBSEL (opf_old_subsel) and switch off the
**	    'not in'/'not exists' subselect to outer join transform if present,
**	    as we already do with trace point op134 (OPT_F006_NONOTEX). This
**	    change fixes bug 109670.
**	 2-mar-05 (hayke02)
**	    Modify the fix for INGSRV3045, bug 113436 so we now only suppress
**	    flattening for single site STAR queries if the parser tells us to
**	    (pst_1mask & PST_NO_FLATTEN) and this is not a PST_MULT_CORR_ATTRS
**	    query. This change fixes problem INGSTR 70, bug 114009.
**	16-jan-06 (hayke02)
**	    We now allow flattening for single site STAR queries with OR'ed
**	    subselects (PST_SUBSEL_IN_OR_TREE). This change fixes bug 113941.
**	4-feb-06 (dougi)
**	    Add support to [no]flatten, [no]ojflatten hints.
**	22-mar-10 (smeke01) b123457
**	    Added trace point op214 call to opu_qtprint.
[@history_template@]...
*/
VOID
opa_1pass(
        OPS_STATE          *global)
{
    OPA_RECURSION         genstate;
    bool                  gotone = FALSE;

    genstate.opa_global = global;
    genstate.opa_gfather = (OPS_SUBQUERY *)NULL;
    genstate.opa_gvarmap = (OPV_GBMVARS *)NULL;
    genstate.opa_demorgan = FALSE;
    genstate.opa_gaggregate = FALSE;
    genstate.opa_gflatten = TRUE;
    genstate.opa_nestcount = 0;
    genstate.opa_ojbegin = (PST_QNODE *)NULL;
    genstate.opa_ojend = &genstate.opa_ojbegin;
    genstate.opa_joinid = OPL_NOINIT;
    genstate.opa_simple = (OPS_SUBQUERY *)NULL;
    genstate.opa_uvlist = (OPS_SUBQUERY *)NULL;
    /* set up control block since recursive parameters take up
    ** too much stack space, especially when resdom lists
    ** are 300 columns long 
    */
    {   /* the optimizer will place TID resdoms for updates,deletes into the
        ** query tree unless some gateway restriction exists */
        OPV_IGVARS          gvno;
        PST_QTREE           *qheader;

        qheader = global->ops_qheader;
        gvno = qheader->pst_restab.pst_resvno;
        if ((qheader->pst_mask1 & PST_TIDREQUIRED)
            &&
            (   (
                    (global->ops_cb->ops_server->opg_qeftype == OPF_GQEF) /* do not add TID node
                                ** if gateway QEF cannot handle it */
                    &&
                    (   (global->ops_rangetab.opv_base->opv_grv[gvno]->opv_relation->rdr_rel->
                        tbl_status_mask & DMT_GATEWAY) == 0
                    )
                )
                ||
                ((global->ops_gdist.opd_gmask & OPD_NOUTID) == 0)
                                    /* the target site does not have TIDs to
                                    ** perform a link back operation so a
                                    ** semi-join operation cannot be considered
                                    ** for the target table to be updated */
            ))
        {   /* TIDs are required in order to process this query, and this is not
            ** the special gateway case in which TIDs are not available */
            opa_tnode(global, (OPZ_IATTS)DB_IMTID, gvno, global->ops_qheader->pst_qtree);
        }
    }
    {
        /* determine whether flattening should be used for this query */
        if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED) /* unless
                                        ** this is a distributed thread
                                        ** then query flattening should 
                                        ** be sensitive to the following
                                        ** knobs */
            &&
            (
                (   global->ops_cb->ops_check 
                    && 
                    opt_strace( global->ops_cb, OPT_F004_FLATTEN) /* or
                                            ** if the NO flattening flag is
                                            ** turned ON */
                )
                ||
                (global->ops_cb->ops_alter.ops_amask & OPS_ANOFLATTEN) /* this
                                        ** originates from the server startup
                                        ** flattening indicator */
                ||
                (global->ops_caller_cb->opf_smask & OPF_SUBOPTIMIZE) /* this
                                        ** is set on a retry due to a zero tuple
                                        ** table being flattened into the parent
                                        ** query i.e. the sybase bug */
                ||
                (   (global->ops_qheader->pst_mask1 & PST_NO_FLATTEN)
                    &&
                    (   !global->ops_cb->ops_check 
                        || 
                        !opt_strace( global->ops_cb, OPT_F005_PARSER) /* ignore
                                            ** the parser flag */
                    )
                )
                ||
                (
                    (global->ops_cb->ops_alter.ops_amask & OPS_ANOAGGFLAT)
                    &&
                    (global->ops_qheader->pst_mask1 & PST_CORR_AGGR)
                )
            )
           )
            global->ops_gmask |= OPS_QUERYFLATTEN; /* this suppresses
                                        ** query flattening from occurring */

	if (global->ops_cb->ops_override & PST_HINT_FLATTEN)
	    global->ops_gmask &= ~OPS_QUERYFLATTEN;

	if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED) &&
	    (global->ops_qheader->pst_distr->pst_stype & PST_1SITE))
	{
	    /* also, suppress flattening if distributed and singlesite */
	    /* and we can handle the mode and the parser tells us to suppress */
	    /* this has to be tested here because mode may change after
	    ** it's tested in pst_header */
	    /* the test in pst_header may be removed later */
	    switch(global->ops_qheader->pst_mode)
	    {
	    case PSQ_RETRIEVE:
	    case PSQ_DEFQRY:
		if ((global->ops_qheader->pst_mask1 & PST_NO_FLATTEN)
		    &&
		    !(global->ops_qheader->pst_mask1 & PST_MULT_CORR_ATTRS)
		    &&
		    !(global->ops_qheader->pst_mask1 & PST_SUBSEL_IN_OR_TREE))
		    global->ops_gmask |= OPS_QUERYFLATTEN; /* this suppresses
					** query flattening from occurring */
		else
		    global->ops_qheader->pst_distr->pst_stype = PST_NSITE;
		break;
	    default:
		/* not supported mode.  Change stype to multisite */
		global->ops_qheader->pst_distr->pst_stype = PST_NSITE;
		break;
	    }

        }
    }

    if (global->ops_cb->ops_override & PST_HINT_NOFLATTEN)
	global->ops_gmask |= OPS_QUERYFLATTEN;


    /* First try the spiffy new NOT EXISTS/NOT IN to outer join transform 
    ** (if not "noflatten" and not specifically turned off). */
    if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED) &&
	!(global->ops_cb->ops_override & PST_HINT_NOOJFLATTEN) &&
	((!global->ops_cb->ops_check || 
	    (!opt_strace( global->ops_cb, OPT_F004_FLATTEN) &&
	     !opt_strace( global->ops_cb, OPT_F006_NONOTEX))) &&
	!(global->ops_cb->ops_server->opg_smask & OPF_OLDSUBSEL) ||
	(global->ops_cb->ops_override & PST_HINT_OJFLATTEN)))
     opa_subsel_to_oj(global, &gotone); 

    if (gotone)
        global->ops_cb->ops_smask |= OPS_SUBSELOJ;
    else
        global->ops_cb->ops_smask &= ~OPS_SUBSELOJ;

# ifdef OPT_F086_DUMP_QTREE2
    if (gotone && global->ops_cb->ops_check &&
	opt_strace(global->ops_cb, OPT_F086_DUMP_QTREE2))
    {	/* op214 - dump parse tree in op146 style */
	opu_qtprint(global, global->ops_qheader->pst_qtree, "after notex transform");
    }
# endif
    if (gotone && global->ops_cb->ops_check &&
        opt_strace(global->ops_cb, OPT_F042_PTREEDUMP))    
    opt_pt_entry(global, global->ops_qheader, global->ops_qheader->pst_qtree, 
                            "after notex transform");

    opa_generate(&genstate, &global->ops_qheader->pst_qtree);
    if (genstate.opa_simple || genstate.opa_uvlist)
        opx_error(E_OP0285_UNIONAGG);
}
