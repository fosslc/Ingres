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
#define             OPB_CREATE          TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPBCREATE.C - create and initialize boolean factor table
**
**  Description:
**      Routines to create and initialize boolean factor table
**
**  History:
**      11-jun-86 (seputis)    
**          initial creation from clauseset.c
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      06-nov-89 (fred)
**          Added support for non-histogrammable datatypes.
**	06-feb-91 (stec)
**	    Removed oph_hdec() - code is in ADF. Modified oph_hvalue().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-jan-94 (ed)
**	    remove iftrue code since iftrue functions will be added
**	    after optimization is complete,... in order to allow
**	    histogramming estimates to be more accurate
**	31-jan-94 (rganski)
**	    Moved oph_hvalue from this module to ophhistos.c, since it is
**	    now called from oph_sarglist as well as opb_ikey, and it is a
**	    histogram-type function.
**	11-feb-94 (ed)
**	    - bug 59593 Remove iftrue code for SEjoins and delay 
**	    expression modification until after enumeration
**      16-nov-94 (inkdo01)
**          - bug 63904 Only follows opl_translate 1 level to move ON 
**          clause of folded outer join. Should pursue until the next 
**          containing outer join is found.
**	13-sep-1996 (inkdo01)
**	    Added code to build keyinfo structures for "<>" (ne) comparisons,
**	    too, to enable more accurate selectivity estimates.
**       7-jul-1998 (sarjo01)
**          Bugs 91176, 91265: Don't allow keyinfo structs for <> (ne)
**          in case of QUEL w/ pattern matching.
**	9-dec-2009
**		Replaced occurences of 
**		subquery->ops_global->ops_cb->ops_server->opg_ifnull with the new
**		ADI_F32768_IFNULL flag
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
[@history_line@]...
**/

/* Some useful constants */
static i4 intc0 = 0;
static double  doublec0 = 0.0;
static char    charc0[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*{
** Name: opb_cbf	- evaluate constant boolean factor
**
** Description:
**      Evaluate the constant boolean factor, and return the constant 
**      result.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bfp                             ptr to boolean factor representing
**                                      the constant expression
**      value                           TRUE if expression will always
**                                      evaluate to TRUE
**
** Outputs:
**	Returns:
**	    TRUE - if the expression does not contain any repeat query
**             parameter or constants whose value is time dependent (e.g. "now")
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-jul-86 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opb_cbf(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT       *bfp,
	bool               *value)
{
    return(FALSE);	    /* FIXME when query compilation is done */
}

/*{
** Name: opb_eqcls	- update boolean factor with equivalence class info
**
** Description:
**      This routine will be used to update the boolean factor bit map
**      of equivalence classes found, and the indication of the number
**      of equivalence classes found.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bfp                             ptr to boolean factor element being
**                                      analyzed
**      var                             ptr to var node which was found
**
** Outputs:
**      bfp->opb_eqcls                  updated to indicate if this is
**                                      multi-equivalence class boolean factor
**      bpf->opb_eqcmap                 updated equivalence class
**	Returns:
**	    equivalence class associated with var node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**      15-may-92 (seputis)
**          b44107 - do not change null join to regular join if
**      ifnull is used in expression
**	11-jun-93 (ed)
**	    added initialization of opb_ojattr for outer joins, to help
**	    in correct boolean factor placement
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
	5-june-03 (inkdo01)
**	    With multiple outer joins, can screw up opv_sevar. Fix avoids
**	    mis-interpretation of iftrue BOP node (bug 110405).
**      08-jul-2005 (gnagn01)
**          Check if the joinop attribute is not function or subselect
**          attribute before resdom lookup is done by comparing joinop attribute with
**          var node result domain number.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
static OPE_IEQCLS
opb_eqcls(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT       *bfp,
	PST_QNODE          *var)
{
    OPE_IEQCLS           eqcls;		/* equivalence class associated with
                                        ** the var node which has been found
                                        */
    OPZ_ATTS             *attrp;        /* ptr to joinop attribute structure */
    OPZ_IATTS		atno;
    OPS_STATE		*global;

    global = subquery->ops_global;
    atno = var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[atno];
    eqcls = attrp->opz_equcls; 		/* this predicate can be applied to
                                        ** all attributes in the equivalence
                                        ** class
                                        */
    if ((global->ops_gmask & OPS_NULLCHANGE)
	&&
	(var->pst_sym.pst_dataval.db_datatype > 0))
    {	/* check for case of outer joins causing a view resdom to change
	** from non-null to nullable
	** - need to traverse target list and function attribute definitions
	** also */
	OPV_GRV		*gvarp;

	gvarp = subquery->ops_vars.opv_base->opv_rt[var->pst_sym.pst_value.
	    pst_s_var.pst_vno]->opv_grv;
	if (gvarp && (gvarp->opv_gmask & OPV_NULLCHANGE))
	{   /* traverse resdom list and determine if the type of this
	    ** var node has changed to nullable */
	    OPZ_IATTS	target_rsno;
	    PST_QNODE	*resdomp;
	    bool	found;

	    found = FALSE;
            target_rsno = attrp->opz_attnm.db_att_id;

	    /*check if not function or subselect attribute before resdom search*/
          if ((target_rsno != OPZ_FANUM) 
             &&
	     (target_rsno != OPZ_SNUM)
	     && 
	     (target_rsno != OPZ_CORRELATED))
          {     
	    for (resdomp = gvarp->opv_gquery->ops_root->pst_left;
		resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
		resdomp = resdomp->pst_left)
	    {
		if (resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno
		    == target_rsno)
		{
		    if (resdomp->pst_sym.pst_dataval.db_datatype < 0)
		    {	/* convert var node to nullable */
			attrp->opz_dataval.db_datatype = 
			var->pst_sym.pst_dataval.db_datatype =
			    resdomp->pst_sym.pst_dataval.db_datatype;
			attrp->opz_dataval.db_length = 
			var->pst_sym.pst_dataval.db_length =
			    resdomp->pst_sym.pst_dataval.db_length;
		    }
		    found = TRUE;
		    break;
		}
	    }
	    if (!found)
		opx_error(E_OP068F_MISSING_RESDOM); /* expected to find
				    ** resdom */
          }
	}
    }
    if (subquery->ops_oj.opl_base)
    {	/* initialize bitmaps used in relation placement */
	if (bfp->opb_ojid >= 0)
	    BTset((i4)atno,(char *)subquery->ops_oj.opl_base->opl_ojt
		[bfp->opb_ojid]->opl_ojattr);
	if (subquery->ops_vars.opv_base->opv_rt[var->pst_sym.pst_value.
	    pst_s_var.pst_vno]->opv_ojeqc != OPE_NOEQCLS)
	{   /* special eqcls exists so boolean factor may need to be
	    ** converted with the iftrue function */
	    if (!bfp->opb_ojattr)
	    {
		bfp->opb_mask |= OPB_OJATTREXISTS;
		bfp->opb_ojattr = (OPZ_BMATTS *)opu_memory(global,
		    (i4)sizeof(*(bfp->opb_ojattr)));
		MEfill(sizeof(*(bfp->opb_ojattr)), (u_char)0, (PTR)bfp->opb_ojattr);
	    }
	    BTset((i4)atno, (char *)bfp->opb_ojattr);
	}
    }
    if (attrp->opz_attnm.db_att_id == OPZ_SNUM)
    {	/* subselect link has been found so mark boolean factor as containing
        ** subselects */
	if (!bfp->opb_virtual)
	{   /* allocate struct to contain virtual table summary for boolfact */
	    bfp->opb_virtual = (OPB_VIRTUAL *) opu_memory(global,
		(i4)sizeof(OPB_VIRTUAL));
	    MEfill(sizeof(OPB_VIRTUAL), (u_char)0, (PTR)bfp->opb_virtual);
	}
	bfp->opb_virtual->opb_subselect = TRUE;
	if (!subquery->ops_eclass.ope_subselect)
	{   /* allocate buffer if this is the first time through this code */
	    subquery->ops_eclass.ope_subselect = (OPE_BMEQCLS *) opu_memory(
		global, (i4)sizeof(OPE_BMEQCLS));
	    MEfill(sizeof(OPE_BMEQCLS), (u_char)0,
		(PTR)subquery->ops_eclass.ope_subselect);
	}
        if (subquery->ops_oj.opl_base
            &&
            !BTtest((i4)eqcls, (char *)subquery->ops_eclass.ope_subselect)
            )
        {   /* there is an outer join in this subquery, so there may
            ** need to be an "IFTRUE" function placed around the
            ** correlated values of the subselect to indicate
            ** where a NULL value should be used
	    ** - for the subselect in the where clause, all the
	    ** attributes need to be traversed to make sure the iftrue
	    ** is used on any "inner relations"
	    ** - currently subselects are not allowed in ON clauses
	    ** but they can still be introduced by views, so there is
	    ** a need to make sure that for nested outer joins with
	    ** views, that any correlated subselect in an ON clause
	    ** has an iftrue modification for those outer joins which
	    ** are in the scope of the outer join ID containing this
	    ** subselect */

            OPV_SUBSELECT   *subselect;
	    OPL_BMOJ	    temp_ojmap;
	    OPL_BMOJ	    *ojmap;
	    OPL_OJT	    *lbase;
	    OPL_IOUTER	    maxoj;

	    lbase = subquery->ops_oj.opl_base;
	    maxoj = subquery->ops_oj.opl_lv;
	    if (bfp->opb_ojid < 0)
	    {	/* boolean factor is at where clause level so that all 
		** attributes which are inner to some outer join need
		** to be evaluated */
		ojmap = &temp_ojmap;
		MEfill(sizeof(*ojmap), 0, (PTR)ojmap);
		BTnot((i4)BITS_IN(*ojmap), (char *)ojmap);
	    }
	    else
		ojmap = lbase->opl_ojt[bfp->opb_ojid]->opl_idmap; /* subselect
			    ** is defined in scope of ON clause, so use only
			    ** outer joins defined within this bfp outer join
			    ** id for determining when an iftrue should be used
			    */
            for (subselect = subquery->ops_vars.opv_base->opv_rt
		[attrp->opz_varnm]->opv_subselect; 
		subselect && (atno != subselect->opv_atno);
                subselect = subselect->opv_nsubselect)
		;	    /* find subselect descriptor */
	    if (!subselect)
		opx_error(E_OP038B_NO_SUBSELECT);   /* expecting to find subselect 
						    **descriptor */
            {
                OPV_SEPARM      *corelated;
		OPL_IOUTER	ojid;
                for (corelated = subselect->opv_parm; corelated;
                    corelated = corelated->opv_senext)
		if (corelated->opv_sevar->pst_sym.pst_type == PST_VAR)
		for (ojid = -1; (ojid = BTnext((i4)ojid, 
		    (char *)ojmap, (i4)maxoj)) >= 0;)
                {   /* for each child ojid, determine if the oj can
		    ** cause an attribute to change values to NULL and
		    ** insert an iftrue if so */
		    OPL_OUTER	    *ojp;
		    OPV_IVARS	    inner_varno;

		    if (corelated->opv_sevar->pst_sym.pst_type != PST_VAR)
			continue;	/* don't fall into an already 
					** generated ii_iftrue */
		    inner_varno = corelated->opv_sevar->pst_sym.pst_value.
			pst_s_var.pst_vno;
		    ojp = lbase->opl_ojt[ojid];
		    if ((ojp->opl_type == OPL_FULLJOIN)
			||
			(ojp->opl_type == OPL_LEFTJOIN))
		    {
			OPE_IEQCLS      special_eqc;
			special_eqc = subquery->ops_vars.opv_base->opv_rt[
			    inner_varno]->opv_ojeqc;
			if ((special_eqc != OPE_NOEQCLS)
			    &&
			    BTtest((i4)inner_varno, (char *)ojp->opl_ivmap))
			{
			    /* found attribute which could be changed to NULL
			    ** due to an outer join */
			    opl_iftrue(subquery, &corelated->opv_sevar, bfp->opb_ojid);
						    /* attach an iftrue
						    ** function to the parse tree */
			    BTset((i4)special_eqc, (char *)&bfp->opb_eqcmap); /* make
						    ** sure special eqcls is available
						    ** for this boolean factor */
			}
		    }
                }
            }
        }
	BTset((i4)eqcls, (char *)subquery->ops_eclass.ope_subselect);
    }
    else
    {	/* check for correlated aggregate */
	OPV_VARS            *varp;

	varp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm];
	if (varp->opv_grv->opv_gsubselect 
	    &&
	    varp->opv_grv->opv_gsubselect->opv_parm)
	{   /* if the global range variable contains a subselect structure
	    ** and parameters exists then the variable is corelated */
	    if (!bfp->opb_virtual)
	    {   /* allocate struct to contain virtual table summary for 
		** boolfact */
		bfp->opb_virtual = (OPB_VIRTUAL *) opu_memory(
		    global, (i4)sizeof(OPB_VIRTUAL));
		MEfill(sizeof(OPB_VIRTUAL), (u_char)0, 
		    (PTR)bfp->opb_virtual);
	    }
	    bfp->opb_virtual->opb_aggregate = TRUE; /* set flag indicating
					** correlated aggregate exists */
#ifdef    E_OP038D_CORRELATED
	    if ((   (varp->opv_grv->opv_created != OPS_RSAGG)
		    &&
		    (varp->opv_grv->opv_created != OPS_HFAGG)
		    &&
		    (varp->opv_grv->opv_created != OPS_RFAGG)
		)
		||
		(varp->opv_subselect->opv_nsubselect)
		)
		opx_error(E_OP038D_CORRELATED); /* expected correlated
					** aggregate at this point */
#endif
	}
    }
    if (eqcls != bfp->opb_eqcls)
    {	/* equivalence class does not match so update boolean factor element*/

	if (bfp->opb_eqcls == OPB_BFNONE)
	    bfp->opb_eqcls = eqcls;	/* this is first equivalence class which
                                        ** has been found so save it
                                        */
	else
	    bfp->opb_eqcls = OPB_BFMULTI; /* more than one equivalence class has
                                        ** been found so indicate this */
	BTset( (i4)eqcls, (char *)&bfp->opb_eqcmap); /* update the equivalence 
                                        ** class  map for the boolean factor 
                                        */
	if (subquery->ops_bfs.opb_ncheck && subquery->ops_bfs.opb_njoins)
	{   /* make check for nullability of equivalence classes, this is used
	    ** to help mark eqcls which can be guaranteed not to have nulls
	    ** included in the final result */
	    if (BTtest((i4)eqcls, (char *)subquery->ops_bfs.opb_njoins))
	    	/* consider only eqcls which have occurred in all the
		** predicates in this boolean factor so far */
		BTset ((i4)eqcls, (char *)subquery->ops_bfs.opb_tjoins);
	}
    }
    return(eqcls);
}

/*{
** Name: opl_bfnull	- check for NULL handling functions
**
** Description:
**      This routine sets up bit maps used to process boolean factors
**	which contain functions which translate NULLs into non-NULLs or
**	otherwise handle NULLs.  The var node of this boolean factor is
**	analyzed as to whether it can be an outer join NULL, and appropriate
**	outer join maps updated
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      root                            var node being processed
**      bfp                             boolean factor being processed
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
**      11-aug-93 (ed)
**          initial creation
**      01-nov-2007 (huazh01)
**          for 'is null' boolean factor on union view subquery, check 
**          if the view itself is referenced in any outer join and enable 
**          OPB_OJMAPACTIVE accordingly. This prevents the 'is null' 
**          restriction from being applied down to the relation defined 
**          in the union view and causes wrong result. (b118366)
**      19-may-2008 (huazh01)
**          modify the fix to 118366. we will still use the special_eqc 
**          associated with the boolean factor to test if OJ needs to be 
**          evaluated, but if there is no special eqc and the subquery is 
**          being referenced under an OJ union view, enable OPB_OJMAPACTIVE
**          to prevent 'IS NULL' being pushed down to the base relation
**          referenced in the view.
**          (b119881)
[@history_template@]...
*/
static VOID
opl_bfnull(
	OPS_SUBQUERY	*subquery, 
	PST_QNODE	*root, 
	OPB_BOOLFACT	*bfp)
{   /* this subtree has operators which depend on NULL
    ** values being correctly set i.e. by outer join 
    ** operations */
    OPE_IEQCLS      special_eqc;
    OPV_VARS	    *varp;
    OPV_IVARS	    varno;
    OPL_IOUTER	    maxoj;
    OPL_OJT	    *lbase;
    OPL_BMOJ	    *ijmap;
    OPS_SUBQUERY    *uv_p = (OPS_SUBQUERY*)NULL;/* ptr to union view subquery */
    bool            union_view = FALSE; 

    lbase = subquery->ops_oj.opl_base;
    maxoj = subquery->ops_oj.opl_lv;
    varno = root->pst_sym.pst_value.pst_s_var.pst_vno;
    varp = subquery->ops_vars.opv_base->opv_rt[varno];
    ijmap = varp->opv_ijmap;
    special_eqc = varp->opv_ojeqc;

    if (subquery->ops_sqtype == OPS_UNION &&
        subquery->ops_oj.opl_smask & OPL_ISNULL)
    {
       uv_p = subquery->ops_onext; 

       while (uv_p && uv_p->ops_sqtype != OPS_MAIN) 
       {
           if (uv_p->ops_sqtype == OPS_VIEW) 
           {
                   union_view = TRUE; 
                   break; 
           }
           uv_p = uv_p->ops_onext; 
       }
    }
    else if (subquery->ops_sqtype == OPS_VIEW && 
             subquery->ops_union              &&
             subquery->ops_oj.opl_smask & OPL_ISNULL)
    {
       union_view = TRUE; 
    }

    if (union_view)
    {
       uv_p = subquery; 

       while (uv_p)
       {
         if (uv_p->ops_mask & OPS_UVREFERENCE &&
             uv_p->ops_oj.opl_lv > 0          &&
             (uv_p->ops_sqtype == OPS_MAIN ||
              uv_p->ops_sqtype == OPS_SAGG )  
            )
         {
            /* enable OPB_OJMAPACTIVE if the union view is being referenced in
            ** an oj query. otherwise, the 'is null' clause could get applied
            ** to the relation defined in the view and cause wrong result.
            */
            bfp->opb_mask |= OPB_OJMAPACTIVE;
            break; 
         }
         uv_p = uv_p->ops_next;
       } 
    }

    if (special_eqc != OPE_NOEQCLS)
    {	/* this attribute is an inner of some outer join
	** which creates the special eqcls, so make sure
	** all necessary outer joins are entirely evaluated
	** so that this boolean factor has the correct
	** eqcls from which to eliminate tuples */
	OPL_BMOJ	*ojmap;
	OPL_BMOJ	tempojmap;
	OPL_IOUTER	ojid;

	if (!ijmap)
	    opx_error(E_OP03A6_OUTER_JOIN); /* expected outer join ID map
				** to be initialized if this point is reached */
	if (bfp->opb_ojid < 0)
	{   /* boolean factor is in where clause, so all
	    ** joinids need to be evaluated 
	    */
	    MEfill(sizeof(tempojmap), (u_char)0, 
		(PTR)&tempojmap);
	    BTnot((i4)BITS_IN(tempojmap), (char *)&tempojmap);
	    ojmap = &tempojmap;
	}
	else
	    ojmap = lbase->opl_ojt[bfp->opb_ojid]->opl_idmap;
	for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)ojmap,
	    (i4)maxoj))>= 0;)
	{
	    OPL_OUTER	*ojp;
	    ojp = lbase->opl_ojt[ojid];
	    if ((ojp->opl_type == OPL_FULLJOIN)
		||
		(ojp->opl_type == OPL_LEFTJOIN))
	    {
		if (BTtest((i4)ojid, (char *)ijmap))
		{
		    bfp->opb_mask |= OPB_OJMAPACTIVE;
		    BTset((i4)ojid, (char *)&bfp->opb_ojmap);
		}
	    }
	}
    }
}

/*{
** Name: opb_isconst	- analyze remainder of query tree for boolean factor
**
** Description:
**      This routine will analyze parts of the query tree for the existance
**      of var nodes and update the equivalence class map accordingly.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bfp                             ptr to boolean factor element being
**                                      analyzed
**      root                            query tree fragment of boolean factor
**	not_first			TRUE if this is not the first level
**					of a recursive call, i.e. a constant
**					expression exists
**
** Outputs:
**      novar                           set to FALSE if this a var node found
**      bfp->opb_eqcmap                 equivalence class map to be updated
**                                      if any var nodes found
**      bfp->opb_eqcls                  equivalence class to be updated if
**                                      any var nodes found
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	23-oct-88 (seputis)
**          test for corelated variable represented by constant
**	10-oct-89 (seputis)
**	    test for pattern matching inside constant expressions
**	    and mark it as non-sargable i.e. novars, if it is found
**	    fix b7956
**	 8-jul-99 (hayke02)
**	    Set new opl_smask value OPL_IFNULL to indicate that we have found
**	    a PST_BOP node that performs the ifnull() function. Use this mask
**	    value to indicate that any PST_VAR nodes to the left of this
**	    PST_BOP node (indicating a restriction or join using the ifnull'ed
**	    values) must have their boolean factors evaluated after the outer
**	    join is evaluated (by calling opl_bfnull()). This change fixes bug
**	    94375.
**	29-may-02 (inkdo01)
**	    Set flag for non-constant funcs with constant/parm-less plists.
**	14-Dec-2009 (drewsturgiss)
**		Repaired tests for ifnull, tests are now 
**		nodep->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags & 
**		nodep->ADI_F32768_IFNULL
[@history_line@]...
*/
static VOID
opb_isconst(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT       *bfp,
	PST_QNODE          **rootpp,
	bool               *novars,
	bool		   not_first)
{
    PST_QNODE       *root;
    bool	    ifnulltest = FALSE;

    root = *rootpp;
    if (root->pst_sym.pst_type == PST_VAR)
    {
	if ((subquery->ops_oj.opl_smask & OPL_ISNULL)
	    ||
	    (subquery->ops_oj.opl_smask & OPL_IFNULL))
	    opl_bfnull(subquery, root, bfp);
	*novars = FALSE;		/* var is found so indicate that this
                                        ** is not a constant qualification
                                        */
	(VOID) opb_eqcls( subquery, bfp, root);/* update the boolean factor 
                                        ** given the var node which has
                                        ** been found
                                        */
    }
    else if (root->pst_sym.pst_type == PST_CONST)
    {
	i4	    parmno;
	if (parmno = root->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
	{
	    PTR		    *parmptrs;	/* ptr to ptr to repeat query parameters
                                        */
	    OPS_STATE	    *global;

	    global = subquery->ops_global;
	    bfp->opb_parms = TRUE;	/* a parameter constant was found in
                                        ** the boolean factor so indicate that
                                        ** this boolean factor cannot be
                                        ** evaluated until runtime
                                        */
	    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
		&&
		global->ops_gdist.opd_repeat)
	    	/* test for corelated variable represented by a constant 
		** for distributed */
		bfp->opb_corelated = bfp->opb_corelated || BTtest((i4)parmno, 
		    (char *)global->ops_gdist.opd_repeat);
	    if ((parmno <= global->ops_qheader->pst_numparm)
		&&
		(parmptrs = global->ops_caller_cb->opf_repeat)
		)
	    {	/* repeat query parameter so that value can be used as
                ** representative of future values 
		** - substitute value into constant node */
		if( !(root->pst_sym.pst_dataval.db_data = parmptrs[parmno]) )
		    opx_error( E_OP038A_REPEAT );
	    }
	}
	if ((root->pst_sym.pst_value.pst_s_cnst.pst_pmspec != PST_PMNOTUSED)
	    &&
	    not_first)
	    *novars = FALSE;		/* this will make the boolean factor
					** none sargable, since an expression
					** exists which can contain pattern
					** matching,... pattern matching is only
					** sargable if it is a single constant
					** and not an expression */
    }
    else
    {
        if ((root->pst_sym.pst_type == PST_BOP || root->pst_sym.pst_type == PST_MOP)
            &&
			(root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
			& ADI_F32768_IFNULL))
        {   /* do not eliminate null join operator if ifnull operator used
            ** over an expression */
            /* FIXME - when UDT support nulls then any user defined function
            ** cannot be trusted either, and reset this flag */
            subquery->ops_bfs.opb_ncheck = FALSE; /* does not matter which eqcls
                                            ** is referenced, this boolean
                                            ** factor cannot guarantee NULLs to
                                            ** be eliminated */
	    if (subquery->ops_oj.opl_base)
	    {
		subquery->ops_oj.opl_smask |= OPL_IFNULL;
		ifnulltest = TRUE;
	    }
        }
	/* Check for constant funcs that aren't really constant. */
	if ((root->pst_sym.pst_type == PST_BOP || 
	    root->pst_sym.pst_type == PST_COP) &&
	    root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags 
		& ADI_F32_NOTVIRGIN)
	 bfp->opb_mask |= OPB_NOTCONST;
	if (root->pst_left)
	    opb_isconst(subquery, bfp, &root->pst_left, novars, TRUE);
	if (ifnulltest)
	    subquery->ops_oj.opl_smask &= (~OPL_IFNULL);
	if (root->pst_right)
	    opb_isconst(subquery, bfp, &root->pst_right, novars, TRUE);
    }
}

/*{
** Name: opb_ikey	- initialize a key value
**
** Description:
**      This routine will create and initialize a key list element.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      keyvalue			ptr to key data value
**      keyhdr                          ptr to head of key value list
**      const                           ptr to const node
**      operator                        operator returned from adc_keybld
**      null_flag                       TRUE if this is an "IS NULL" type key
**
** Outputs:
**      keyhdr.opb_keylist              a new key value is added to this list
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	20-apr-90 (seputis)
**	    fix histogram problem with <= and >= 
**	which occurs when <= is used on an integer, in which oph_interpolate
**	does not have enough information because of the "high key"
**	indication implies both <= or < resulting in an error one way
**	or the other ( same is true with >= and >
**	13-may-91 (seputis)
**	    - init opb_opno as part of fix to b37172
**      18-sep-92 (seputis)
**          - fix bug 45728, indicate OR count to be decremented if
**          constant is equal to existing constant
**	31-jul-97 (hayke02)
**	    Modified the fix for bug 45728 so that the OR count (opb_orcount)
**	    is now only decremented if it is greater than zero. This change
**	    fixes bug 81226.
**	11-may-98 (hayke02)
**	    Modified above fix for bug 81226. We now check for a flattened
**	    aggregate subquery (subquery->ops_agg.opa_flatten) when deciding
**	    whether to decrement the OR count. This prevents overestimation
**	    when duplicate and consecutive boolean factors are used in repeat
**	    queries using host language variables for the key restrictions.
**	    This change fixes bug 89873.
**	19-oct-00 (inkdo01)
**	    Init opb_connector for composite histogram feature.
**	27-jun-02 (hayke02)
**	    Check for a non-zero OR count (opb_orcount) before decrementing
**	    when adc_tykey is ADC_KNOMATCH. This prevents E_OP089B errors
**	    when running queries containing like predicates with no pattern
**	    matching characters or'ed to equals predicates. This change
**	    fixes bug 107869.
**	10-jul-06 (hayke02)
**	    Do not remove duplicate OR'ed constants if OPB_OJATTREXISTS is
**	    set. This prevents a cart prod join and incorrect results from a
**	    query of the form:
**		select ... from a full join b on a.c1 = b.c1 where
**		a.c1 = <cnst> or b.c1 = <cnst>
**	    This change fixes bug 116009.
[@history_line@]...
*/
static VOID
opb_ikey(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT	   *bfp,
	PTR                keyvalue,
	OPB_BFKEYINFO      *keyhdr,
	PST_QNODE          *constant,
	OPB_SARG	   operator,
	ADI_OP_ID	   opid,
	bool		   null_flag)
{
    OPB_BFVALLIST          *bfvalue;/* ptr to element which will contain
				    ** key value
				    */
    
    bfvalue = (OPB_BFVALLIST *) opu_memory( subquery->ops_global,
	    (i4) sizeof(OPB_BFVALLIST));
    bfvalue->opb_operator = operator;
    bfvalue->opb_opno = opid;
    bfvalue->opb_connector = 0;
    {	/* update argument type of the key list header */
	OPB_SARG	    sargtype;    /* existing argument type of
				    ** of predicate
                                    */
	sargtype = keyhdr->opb_sargtype;
	if ((operator == ADC_KHIGHKEY && sargtype == ADC_KLOWKEY)
	    ||
	    (operator == ADC_KLOWKEY && sargtype == ADC_KHIGHKEY))
	    /* cannot apply key if range is not contiguous 
	    ** FIXME - support non-contiguous ranges in future
            */
	{
	    keyhdr->opb_sargtype = ADC_KNOKEY;
	}
	else if (sargtype < operator)
	    keyhdr->opb_sargtype = operator;
    }
    if (null_flag)
	bfvalue->opb_null = opid;
    else
	bfvalue->opb_null = (ADI_OP_ID)0;
    bfvalue->opb_keyvalue = keyvalue;
    if (keyvalue)
	bfvalue->opb_hvalue = oph_hvalue( subquery, keyhdr->opb_bfhistdt, 
	    keyhdr->opb_bfdt, keyvalue, opid); /* create the histogram
				    ** element if key exists */
    else
	bfvalue->opb_hvalue = NULL; /* indicates histogram value not available*/
    if (keyhdr->opb_sargtype == ADC_KEXACTKEY)
    {	/* the entries for an exact key must be sorted */
	OPB_BFVALLIST          **nextvalue;
	i4		       adc_cmp_result;
	DB_DATA_VALUE	       adc_dv1;
	DB_DATA_VALUE	       adc_dv2;

        adc_cmp_result = 1;		/* set to non zero so that result
                                        ** will be linked in */
	if (*(nextvalue = &keyhdr->opb_keylist))
	{
            STRUCT_ASSIGN_MACRO(*(keyhdr->opb_bfdt), adc_dv1);
            STRUCT_ASSIGN_MACRO(adc_dv1, adc_dv2);
            adc_dv2.db_data = keyvalue;
	}

	for ( ; *nextvalue; nextvalue = &(*nextvalue)->opb_next)
	{   /* compare until a greater value is found if equal then 
            ** throw it away */
	    DB_STATUS            comparestatus;

	    adc_dv1.db_data = (*nextvalue)->opb_keyvalue;
	    if (!adc_dv1.db_data || !adc_dv2.db_data)
		continue;		    /* place parameter constants
                                            ** at the end of the list */
            
	    comparestatus = adc_compare(subquery->ops_global->ops_adfcb,
		&adc_dv1, &adc_dv2, &adc_cmp_result);
	    if (comparestatus != E_DB_OK)
		opx_verror( comparestatus, E_OP078C_ADC_COMPARE,
		    subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
            if (adc_cmp_result >= 0)
		break;			    /* value needs to be placed
                                            ** before this element
                                            ** - or it is equal to this element
                                            ** and must be discarded */
	}
	if (adc_cmp_result || (bfp->opb_mask & OPB_OJATTREXISTS))
					    /* if value is equal skip this
                                            ** this element 
                                            ** FIXME - memory lost here */
	{
	    /* value must be inserted into the list at this point so the values
	    ** are in increasing order */
	    bfvalue->opb_next = *nextvalue;
	    *nextvalue = bfvalue;
	}
        else
            bfp->opb_mask |= OPB_DECOR;     /* const not inserted into list
                                            ** so eventually decrement the
                                            ** OR count */
    }
    else
    {
	/* otherwise just place at the beginning of the list */
	bfvalue->opb_next = keyhdr->opb_keylist; /* link into list of keys */
	keyhdr->opb_keylist = bfvalue;
    }

}

/*{
** Name: opb_keybuild	- build a key given the constant value
**
** Description:
**      This procedure will build the keys if possible, given a constant
**      value.  The keybuilding will be delayed until runtime if a constant
**      expression exists, or the node is a parameter constant.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      keyhdr                          ptr to element which will contain
**                                      information about the keys with
**                                      the same (type, length) datatype value
**      opno                            ADF operator id of comparison operator
**                                      used with key
**      const                           ptr to query tree fragment which
**                                      contains constant nodes
**
** Outputs:
**      keyhdr->opb_keylist             will contain one or two new elements
**                                      representing the (opno, const) pair
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	30-apr-90 (seputis)
**	    part of bug fix for 21160
**	16-may-90 (seputis)
**	    part of bug fix for 21160
**	31-dec-90 (seputis)
**	    IS NOT NULL selects several values, so do not mark it
**	    and an exact key
**	17-apr-91 (seputis)
**	    - added code to support isescape keybuilding
**	12-aug-91 (seputis)
**	    - added test for LIKE operator under ALLMATCH to
**	    assume pattern matching, important for CAFS text
**	    retrieval queries
**      28-apr-92 (seputis)
**          - bug 43820 - estimation problem with like operator
**          like '%' is too restrictive.
**	13-sep-1996 (inkdo01)
**	    Build keylist structure for <> (ne) pres, too. 
**	28-jul-98 (hayek02)
**	    The like '%' is still too restrcitive. A non-nullable character
**	    attribute will yield 100% selectivity for like '%' (rather than
**	    the estimated 75%). We now check the attribute's datatype and
**	    adjust the selectivity to 100% if a non-nullable character type
**	    is found. This change fixes bug 86524.
**	3-apr-00 (inkdo01)
**	    Minor tweak to earlier change of ">/<"s to ">=/<="s to fix 
**	    histogram interpolation problem.
**	18-sep-00 (inkdo01)
**	    Remove tweak - it buggers up stupid Star.
**	19-aug-04 (hayke02)
**	    Add back in the above tweak, but for non-STAR queries only. This
**	    change fixes the original bug (101149), and problem INGSRV 2938,
**	    bug 112850.
**	 6-sep-04 (hayke02)
**	    Set selectivity for "like '%<string>'" predicates at 50% rather than
**	    100%/75% for non-nullable/nullable attributes. This change fixes
**	    problem INGSRV 2939, bug 112872.
**	 4-mar-05 (hayke02)
**	    Modify fix for INGSRV 2938, bug 112850 to disable it fully for STAR
**	    queries. This change fixes problem INGSTR 71, bug 114016.
**	5-Mar-2005 (schka24)
**	    Fix SQL query against a QUEL view that involves a quel-ish
**	    pattern match testing operator.  We were feeding keybld the
**	    wrong query language for the QUEL view entities, thus getting
**	    the wrong key type indication, and later potentially deciding
**	    that we had a no-rows query.  Example: define view starts_with_a
**	    (foo.all) where c="a*"; select * from starts_with_a where c='ab'
**	    would incorrectly return no rows even if one row matched.
**	 8-apr-05 (hayke02)
**	    Modify fix for 112872 so that we set OPB_PCLIKE in the opb_mask.
**	    This indicates that the estimate for like '%<string>' predicates
**	    should be set to 10% (ops_range, OPS_INEXSELECT) in oph_relselect().
**	    This change fixes bug 114263, problem INGSRV 3241.
**	19-sep-06 (hayke02)
**	    Yet another tweak to the fix for bugs 101149, 112850, 114016, so
**	    that we now set only opno, and not opndep pst_opno, to adc_opkey.
**	    This ensures that GT/LT BOPs are not changed into GE/LE BOPs,
**	    whilst still giving correct row estimates for queries with GT/LT
**	    restrictions. This change fixes bug 116611.
**	22-may-08 (hayke02)
**	    Modify the fix for bug 107869, and set OPB_KNOMATCH if adc_tykey
**	    is ADC_KNOMATCH, and opb_orcount is 0. This change fixes bug
**	    120294.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**      10-apr-2009 (huazh01)
**          initialize the adc_hikey.db_data and adc_lokey.db_data area
**          allocated by opu_memory(). (bug 121909)
**	30-May-2009 (kiria01) b122128
**	    Adjust LIKE data based on summary info.
**	02-Dec-2009 (kiria01) b122952
**	    Look for sorted PST_INLISTSORT and use its tail end as high range.
[@history_line@]...
*/
static VOID
opb_keybuild(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT	   *bfp,
	OPB_BFKEYINFO      *keyhdr,
	PST_QNODE	   *opqnodep,
	PST_QNODE          *constant)
{
    OPS_STATE           *global;    /* ptr to global state control block
                                    */
    ADC_KEY_BLK         adc_kblk; /* structure used by ADF to build the key 
				    ** and to classify type of key
				    */
    ADI_OP_ID		opno;
    i4			save_qlang;

    global = subquery->ops_global;  /* get ptr to global state var */

    opno = opqnodep->pst_sym.pst_value.pst_s_op.pst_opno; /* operator id of
				** the node */

    adc_kblk.adc_opkey = opno;  /* save ADF comparison operator id */

    adc_kblk.adc_lokey.db_datatype = adc_kblk.adc_hikey.db_datatype = 
	keyhdr->opb_bfdt->db_datatype; /* initialize the key datatypes */
    adc_kblk.adc_lokey.db_prec = adc_kblk.adc_hikey.db_prec =
	keyhdr->opb_bfdt->db_prec; /* initialize the key precs */
    adc_kblk.adc_lokey.db_length = adc_kblk.adc_hikey.db_length = 
	keyhdr->opb_bfdt->db_length; /* initialize the key lengths */
    if (opqnodep->pst_sym.pst_value.pst_s_op.pst_pat_flags & AD_PAT_HAS_ESCAPE)
	adc_kblk.adc_escape = opqnodep->pst_sym.pst_value.pst_s_op.pst_escape;
    adc_kblk.adc_pat_flags = opqnodep->pst_sym.pst_value.pst_s_op.pst_pat_flags;

    /* build key if a constant value exists at optimization time, for now
    ** ignore constant expressions since in some cases they can change
    ** (e.g. the date string "now") ... until we can detect whether
    ** or not the expression is time independent.
    */
    if (opno
	== 
	subquery->ops_global->ops_cb->ops_server->opg_isnull)
    { /*The constant node is null so assume an "IS NULL" is processed */
	opb_ikey( subquery, bfp, (PTR)NULL, keyhdr, (PST_QNODE *)NULL,
	    (OPB_SARG)ADC_KEXACTKEY, opno , TRUE /* IS NULL */);
	return;
    }
    else if(opno
	    == 
	    subquery->ops_global->ops_cb->ops_server->opg_isnotnull)
    { /*The constant node is null so assume an "IS NOT NULL" is processed */
	opb_ikey( subquery, bfp, (PTR)NULL, keyhdr, (PST_QNODE *)NULL,
	    (OPB_SARG)ADC_KHIGHKEY, opno , TRUE /* IS NULL */);
	return;
    }
    save_qlang = global->ops_adfcb->adf_qlang;
    if (!((constant->pst_sym.pst_type == PST_CONST)
	   &&
	   constant->pst_sym.pst_dataval.db_data))
        /* FIXME - parameter constants may have values which will be used
        ** for the initial optimization - make sure if the values are used
        ** that it does not cause a FALSE , or TRUE indication.  Just
        ** ignore the value in that case
        */
    {   /* Keys cannot be built at optimization time so initialize
	** ptrs to indicate this fact ... the keys will be built at
	** runtime... all we need is to find opb_operator for the key
	*/
	STRUCT_ASSIGN_MACRO(constant->pst_sym.pst_dataval, adc_kblk.adc_kdv);
	adc_kblk.adc_lokey.db_data = adc_kblk.adc_hikey.db_data =NULL;
	    /* initialize the key value ptrs to NULL to indicate that the
	    ** keys cannot be computed until runtime
	    */
    }
    else
    {   /* keys can be built at optimization time so convert constants if
        ** necessary and allocate appropriate buffers for keys
        */

	STRUCT_ASSIGN_MACRO(constant->pst_sym.pst_dataval, adc_kblk.adc_kdv);

	/* allocate space for result keys */
	adc_kblk.adc_hikey.db_data = 
	    opu_memory( global, (i4)keyhdr->opb_bfdt->db_length);
        MEfill((i4)keyhdr->opb_bfdt->db_length, (u_char)0, 
               adc_kblk.adc_hikey.db_data); 

	/* check if space is associated with the key list header which can
	** be used for the high key
	*/
	if (keyhdr->opb_bfdt->db_data)
	{
	    adc_kblk.adc_lokey.db_data = keyhdr->opb_bfdt->db_data;
	    keyhdr->opb_bfdt->db_data = NULL;	/* deassign this buffer */
	}
	else
        {
	    adc_kblk.adc_lokey.db_data = opu_memory( global, 
		(i4)keyhdr->opb_bfdt->db_length);
            MEfill((i4)keyhdr->opb_bfdt->db_length, (u_char)0, 
                   adc_kblk.adc_lokey.db_data); 
        }
	/* Make sure that keybld runs in the same query language as the
	** constant -- this is for sql selects from quel views
	*/
	global->ops_adfcb->adf_qlang = constant->pst_sym.pst_value.pst_s_cnst.pst_cqlang;
    }

    if ((opqnodep->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT) &&
		opno == global->ops_cb->ops_server->opg_eq)
    {
	PST_QNODE *p = constant;
	DB_STATUS keybldstatus;
	char *save;
	/* This is a fat equals - create a range manually */
	adc_kblk.adc_opkey = global->ops_cb->ops_server->opg_ge;
	save = adc_kblk.adc_hikey.db_data; adc_kblk.adc_hikey.db_data = NULL;
        keybldstatus = adc_keybld( global->ops_adfcb, &adc_kblk );
	adc_kblk.adc_hikey.db_data = save;
	global->ops_adfcb->adf_qlang = save_qlang;
	if (keybldstatus != E_DB_OK)
	{   /* key cannot be converted so save warning and continue as if
            ** key cannot be built */
	    opx_verror( E_DB_WARN, E_OP0305_NOCONVERSION,
		global->ops_adfcb->adf_errcb.ad_errcode); /* warning if 
					** conversion is unsuccessful*/
	    return;			/* return without adding key to list */
	}

	while (p->pst_left)
	    p = p->pst_left;
	STRUCT_ASSIGN_MACRO(p->pst_sym.pst_dataval, adc_kblk.adc_kdv);
	adc_kblk.adc_opkey = global->ops_cb->ops_server->opg_le;
	save = adc_kblk.adc_lokey.db_data; adc_kblk.adc_lokey.db_data = NULL;
        keybldstatus = adc_keybld( global->ops_adfcb, &adc_kblk );
	adc_kblk.adc_lokey.db_data = save;
	global->ops_adfcb->adf_qlang = save_qlang;
	if (keybldstatus != E_DB_OK)
	{   /* key cannot be converted so save warning and continue as if
            ** key cannot be built */
	    opx_verror( E_DB_WARN, E_OP0305_NOCONVERSION,
		global->ops_adfcb->adf_errcb.ad_errcode); /* warning if 
					** conversion is unsuccessful*/
	    return;			/* return without adding key to list */
	}
	adc_kblk.adc_opkey = opno; /* reset opno */
	adc_kblk.adc_tykey = ADC_KRANGEKEY;
    }
    else
    {
	/* call the key building routine to get keys if necessary and to
	** obtain the opb_operator type for the comparison
	*/
	DB_STATUS		keybldstatus;

        keybldstatus = adc_keybld( global->ops_adfcb, &adc_kblk );
	global->ops_adfcb->adf_qlang = save_qlang;
	if (keybldstatus != E_DB_OK)
	{   /* key cannot be converted so save warning and continue as if
            ** key cannot be built */
	    opx_verror( E_DB_WARN, E_OP0305_NOCONVERSION,
		global->ops_adfcb->adf_errcb.ad_errcode); /* warning if 
					** conversion is unsuccessful*/
	    return;			/* return without adding key to list */
	}
	if (opno == ADI_GT_OP || opno == ADI_LT_OP &&
						    opno != adc_kblk.adc_opkey)
	    opno = adc_kblk.adc_opkey;
				/* reset opno for GT/LT changed to GE/LE */
    }

    /* allocate the element which will contain the key and initialize it */
    switch (adc_kblk.adc_tykey)
    {

    case ADC_KEXACTKEY:
    {
	opb_ikey( subquery, bfp, adc_kblk.adc_lokey.db_data, keyhdr, constant,
	    (OPB_SARG)adc_kblk.adc_tykey, opno, FALSE);
        if (adc_kblk.adc_hikey.db_data)
	    keyhdr->opb_bfdt->db_data = adc_kblk.adc_hikey.db_data; /* 
                                        ** reassign "spare buffer" */
	break;
	
    }
    case ADC_KLOWKEY:
    {
	opb_ikey( subquery, bfp, adc_kblk.adc_lokey.db_data, keyhdr, constant,
	    (OPB_SARG)adc_kblk.adc_tykey, opno, FALSE);
        if (adc_kblk.adc_hikey.db_data)
	    keyhdr->opb_bfdt->db_data = adc_kblk.adc_hikey.db_data; /* 
                                        ** reassign "spare buffer" */
	break;
	
    }
    case ADC_KHIGHKEY:
    {
	opb_ikey( subquery, bfp, adc_kblk.adc_hikey.db_data, keyhdr, constant,
	    (OPB_SARG) adc_kblk.adc_tykey, opno, FALSE);
					/* spare buffer not used */
	break;
    }
    case ADC_KRANGEKEY:
    {
	/* high key must be inserted in list before low key */
	opb_ikey( subquery, bfp, adc_kblk.adc_hikey.db_data, keyhdr, constant,
	    (OPB_SARG) adc_kblk.adc_tykey, 
	    (ADI_OP_ID)subquery->ops_global->ops_cb->ops_server->opg_le, FALSE);
	opb_ikey( subquery, bfp, adc_kblk.adc_lokey.db_data, keyhdr, constant,
	    (OPB_SARG) adc_kblk.adc_tykey, 
	    (ADI_OP_ID)subquery->ops_global->ops_cb->ops_server->opg_ge, FALSE);
					/* make sure assumption that le and ge
					** can be used for histograms */
	if (adc_kblk.adc_lokey.db_data)
	    keyhdr->opb_bfdt->db_data = NULL; /* set to NULL since the memory
                                        ** has been occupied
                                        */
	break;
    }
    case ADC_KNOMATCH:
    {
	if (bfp->opb_orcount)
	    bfp->opb_orcount--;
	else
	    bfp->opb_mask |= OPB_KNOMATCH;
					/* part of b21160 is that the
					** default estimate was 50%, but should
					** be 0% */
	break;
    }
    case ADC_KALLMATCH:
    {
        bfp->opb_mask |= OPB_DNOKEY;    /* do not use this qualification for keying
                                        ** purposes since entire range will need
                                        ** to be scanned */
	bfp->opb_orcount+=1;		/* ALLMATCH is usually associated with
					** wild cards, and probably still do have
					** some selectivity */
        if ((constant->pst_sym.pst_value.pst_s_cnst.pst_pmspec == PST_PMNOTUSED)
            &&
            (opno != global->ops_cb->ops_server->opg_like))
        {
            bfp->opb_orcount +=10;      /* effectivity make this 100%
					** selectivity since pattern matching
                                        ** is not used */
                                        /* - b21160
                                        */
					/* non-nullable character attributes
					** will yield 100% selectivity for
					** like '%' - bug 86524 */
            keyhdr->opb_sargtype = ADC_KALLMATCH; /* since no pattern matching
                                        ** all values match */
        }
	else if (opno == global->ops_cb->ops_server->opg_like)
	{
	    keyhdr->opb_sargtype = ADC_KNOKEY;
	    if (constant->pst_sym.pst_dataval.db_data &&
		constant->pst_sym.pst_dataval.db_datatype == DB_PAT_TYPE)
	    {
		/* Refer back to pattern summary to tell whether this really
		** is an ALL match or not*/
		switch (adu_patcomp_summary(&constant->pst_sym.pst_dataval, NULL))
		{
		case ADU_PAT_IS_MATCH:
		    bfp->opb_orcount += 10;
		    keyhdr->opb_sargtype = ADC_KALLMATCH;
		    break;
		case ADU_PAT_IS_NOMATCH:
		    if (bfp->opb_orcount)
			bfp->opb_orcount--;
		    else
			bfp->opb_mask |= OPB_KNOMATCH;
		    break;
		default:
		    bfp->opb_orcount--;
		    bfp->opb_mask |= OPB_PCLIKE;
		}
	    }
            else if (constant->pst_sym.pst_dataval.db_data &&
		constant->pst_sym.pst_dataval.db_datatype == DB_VCH_TYPE)
	    {
		i4	i;
		bool	all = TRUE;
		for (i = DB_CNTSIZE;
			    i < constant->pst_sym.pst_dataval.db_length; i++)
		{
		    if (constant->pst_sym.pst_dataval.db_data[i] != '%')
		    {
			all = FALSE;
			break;
		    }
		}
		if (!all)
		{
		    bfp->opb_orcount-=1;
		    bfp->opb_mask |= OPB_PCLIKE;
		}
		else
		{
		    if ((bfp->opb_keys->opb_bfdt->db_datatype == DB_CHA_TYPE)
			||
			(bfp->opb_keys->opb_bfdt->db_datatype == DB_VCH_TYPE)
			||
			(bfp->opb_keys->opb_bfdt->db_datatype == DB_CHR_TYPE)
			||
			(bfp->opb_keys->opb_bfdt->db_datatype == DB_TXT_TYPE))
		    {
			bfp->opb_orcount +=10;
			keyhdr->opb_sargtype = ADC_KALLMATCH;
		    }
		}
	     }
	}
        else
            keyhdr->opb_sargtype = ADC_KNOKEY;  /* assume this cannot be used
                                        ** for a key */
	break;
    }
    case ADC_KNOKEY:
    {
	/* check for "<>" operator, and build keylist entry */
	if (opno == subquery->ops_global->ops_cb->ops_server->opg_ne)
	  opb_ikey( subquery, bfp, adc_kblk.adc_lokey.db_data, keyhdr, constant,
	    (OPB_SARG) adc_kblk.adc_tykey, opno, FALSE);
	break;
    }
	    
    default:
	{   /* this is a case of ADC_KALLMATCH, ADC_KNOKEY */
	    /* FIXME - memory is lost here */
	}
    }
}

/*{
** Name: opb_ckilist	- create a list of keyinfo headers
**
** Description:
**      This routine will create a list of keyinfo headers to be associated
**      with a boolean factor.  This routine is called the first time
**      that this equivalence class is found in the limiting predicate
**      associated with this boolean factor.  An element is added to the
**      keyinfo list for each member of the set of unique (type, length)
**      pairs from all joinop attribute belonging to the equivalence class.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      eqcls                           equivalence class index associated
**                                      with equivalence class found in
**                                      the limiting predicate
**      bfp                             ptr to boolean factor which contains
**                                      the limiting predicate
**
** Outputs:
**      bfptr->opb_keys                 a list of keyinfo ptrs is added to
**                                      the beginning which represent the
**                                      unique (type,length) pairs belonging
**                                      to the equivalence class
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	25-apr-94 (rganski)
**	    b59982: for character columns longer than 8 characters, we don't
**	    know what the histogram length is at this point; so create a
**	    keyinfo header for each such column, even if the attribute type and
**	    length is the same as another attribute's. This will avoid the
**	    problem in oph_sarglist when two attributes have the same type and
**	    length but different histogram lengths.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
[@history_line@]...
*/
static VOID
opb_ckilist(
	OPS_SUBQUERY       *subquery,
	OPE_IEQCLS	   eqcls,
	OPB_BOOLFACT       *bfp)
{
    OPE_EQCLIST           *eqcls_ptr;	/* ptr to equivalence class of joinop
                                        ** attribute represented by var node
                                        */
    OPZ_IATTS		   attribute;   /* current joinop attribute in
					** equivalence class being analyzed
					*/
    OPZ_BMATTS		   *attrmap;	/* ptr to joinop attribute map 
					** associated with equivalence
					** class
					*/
    OPZ_IATTS		   maxattr;    /* size in bits of attrmap */

    eqcls_ptr = subquery->ops_eclass.ope_base->ope_eqclist[eqcls]; /* get ptr to
                                        ** equivalence class element to which
					** this boolean factor applies
                                        */
    attrmap = &eqcls_ptr->ope_attrmap;
    maxattr = subquery->ops_attrs.opz_av;
    for( attribute = -1; 
	(attribute = BTnext( (i4)attribute, (char *)attrmap, (i4)maxattr)) >= 0;)
    {
	OPZ_ATTS           *attr_ptr;   /* ptr to current attribute in
					** equivalence class being
					** analyzed
					*/
	DB_DT_ID	   attr_type;   /* ADF datatype of joinop attribute
					*/
	OPS_DTLENGTH       attr_length; /* length of joinop attribute */
	OPB_BFKEYINFO      *nextkey;    /* used to traverse list of keys 
					** associated with boolean factor
					*/
	bool	       found;	    /* TRUE - if keyinfo element exists
					*/

	/* JRBCMT do we need to work prec in here? */
	/* JRBNOINT -- Can't integrate if we check prec			    */
	attr_ptr = subquery->ops_attrs.opz_base->opz_attnums[attribute];
	attr_type = attr_ptr->opz_dataval.db_datatype;
	attr_length = attr_ptr->opz_dataval.db_length;

	found = FALSE;
	if (attr_length > OPH_NH_LENGTH &&
	    (attr_ptr->opz_histogram.oph_dataval.db_datatype == DB_CHA_TYPE ||
	     attr_ptr->opz_histogram.oph_dataval.db_datatype == DB_BYTE_TYPE))
	{
	    /* For long character columns, create new keyinfo header.
	    ** Don't bother looking at previous keyinfo headers - b59982
	    */
	}
	else
	{
	    /* search list for existing (eqcls, type, length) keyinfo element */
	    for( nextkey = bfp->opb_keys; 
		nextkey && ( nextkey->opb_eqc == eqcls ) ; /* exit if end of
					** list associated with equivalence
					** class is reached, note that
					** all new elements are placed
					** at the head of the list so
					** elements with the same
					** equivalence class are contiguous
					*/
		nextkey = nextkey->opb_next)
	    {
		if (nextkey->opb_bfdt->db_datatype == attr_type &&
		    nextkey->opb_bfdt->db_length == attr_length )
		{
		    found = TRUE;	/* equivalence class, type, length
					** already exists in list so exit
					*/
		    break;
		}
	    }
	}
	if (!found)
	{   /* the end of the keylist associated with equivalence class
	    ** list has been reached and element not found so
	    ** create a new element of the appropriate type and length
	    */
	    register OPB_BFKEYINFO     *newkey;

	    newkey = (OPB_BFKEYINFO *) opu_memory( subquery->ops_global, 
		(i4) sizeof(OPB_BFKEYINFO) );
	    newkey->opb_next = bfp->opb_keys;
	    bfp->opb_keys = newkey;	/* link into list of keys */
	    newkey->opb_eqc = eqcls;    /* set equivalence class */
	    newkey->opb_bfdt = &attr_ptr->opz_dataval; /* ptr to datatype
					** to be associated with key list
					*/
	    newkey->opb_bfhistdt = &attr_ptr->opz_histogram.oph_dataval; /*
					** ptr to datatype of histogram to
					** be associated with key list
					*/
	    newkey->opb_used = FALSE;   /* not used yet in selectivity*/
	    newkey->opb_sargtype = ADC_KNOMATCH; /* this value will be
					** overridden by first predicate
					** defined
					*/
	    newkey->opb_keylist = NULL; /* no keys exist yet */
	    newkey->opb_global = NULL;  /* no global information yet */
	}
    }
}

/*{
** Name: opb_bfkey	- create boolean factor key and histogram info
**
** Description:
**      This routine will create the key and convert to a histogram element
**      any non-parameter constant nodes.  Parameter constant nodes are
**      simply saved, and will be defined at query execution time for
**      keyed access.
**
**      The routine operates on every unique joinop attribute (type,length) 
**      pair within the equivalence class associated with the var node.  This
**      will enable keyed access to be used on any attribute within the
**      equivalence class.
**      
**
** Inputs:
**      subquery                        ptr to subquery being processed
**      bfp                             ptr to boolean factor state variable
**      var                             ptr to PST_VAR node of boolean factor
**      op                              ptr to PST_OP operator node for
**                                      boolean factor
**      const                           ptr to tree containing constant nodes
**                                      which will be used to create key
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
**	28-apr-86 (seputis)
**          initial creation
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
[@history_line@]...
*/
static VOID
opb_bfkey(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT       *bfp,
	PST_QNODE          *var,
	PST_QNODE	   *opqnodep,
	PST_QNODE          *constant)
{
    OPE_IEQCLS		eqcls;		/* equivalence class of var node */
    OPB_BFKEYINFO       *keyhdr;	/* used to traverse list of keys 
					** associated with boolean factor
					*/
    OPB_BFKEYINFO       *keyeqcls_ptr;	/* ptr to first element of a contiguous
                                        ** list of keyinfo elements with the
                                        ** same equivalence class
					*/
    eqcls = opb_eqcls( subquery, bfp, var); /* update the boolean factor given
                                        ** the var node which has been found
                                        ** and find the equivalence class
                                        ** associated with the var node
                                        */
    for( keyhdr = bfp->opb_keys; keyhdr; keyhdr = keyhdr->opb_next)
    {	/* Traverse the list of keyinfo ptrs and check whether this particular
	** equivalence class has been processed with respect to this boolean
	** factor.  The creation mechanism guarantees that all unique (type,len)
        ** pairs belonging to one equivalence class will be contiguous in the
        ** list; i.e. until the equivalence class changes
	*/
	if (keyhdr->opb_eqc == eqcls)
	{   /* equivalence class found so exit with beginning of (type,length)
            ** list
	    */
	    keyeqcls_ptr = keyhdr;
	    break;
	}
    }
    if (!keyhdr)
    {	/* equivalence class is not in list so add it */
        opb_ckilist( subquery, eqcls, bfp); /* create a list 
                                            ** of keyinfo ptrs
                                            ** with one for each unique
                                            ** (type, length) pair of the 
                                            ** equivalence class and place
                                            ** list at beginning of 
                                            ** bfp->opb_keys list
                                            */
	keyeqcls_ptr = bfp->opb_keys;	    /* the creation process will have
                                            ** placed all elements from the
                                            ** same equivalence class at the
					    ** beginning of the list
					    */
    }

    /* traverse the list of key descriptors associated with this 
    ** equivalence class 
    ** - add key info associated with this predicate
    ** - add histogram info associated with this predicate
    */
    bfp->opb_mask &= (~OPB_DECOR);
    for (keyhdr = keyeqcls_ptr;		    /* start at beginning of equivalence
					    ** class list
					    */
	 keyhdr 
         && 
         keyhdr->opb_eqc == eqcls;	    /* the list ends if the physical end
					    ** of the list is reached or the
					    ** equivalence class changes
					    */
	keyhdr = keyhdr->opb_next)
    {
	opb_keybuild( subquery, bfp, keyhdr, opqnodep, constant); /* initialize the
					    ** key and histogram information */
    }
    if ((bfp->opb_mask & OPB_DECOR)
	&&
	(bfp->opb_orcount || 
	!BTcount((char *)&subquery->ops_agg.opa_flatten, OPV_MAXVAR)))
    {
        bfp->opb_orcount = bfp->opb_orcount -1; /* decrement OR count if at least
                                            ** one key cannot be inserted, note that
                                            ** the OR count should really be associated
                                            ** with each datatype, but this is too much
                                            ** work,  need to check that opb_orcount
                                            ** usage cannot go negative */
    }
}

/*{
** Name: opb_bfget	- allocate a new boolean factor element
**
** Description:
**      Search the boolean factor array for a free location, assign 
**      a slot, and allocate memory for the boolean factor element.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**          .ops_bfs                    boolean factor information
**              .opb_base               base of boolean factor array
**              .opb_bv                 number of boolean factors allocated
**      bfp                             ptr to boolean factor element
**					being initialize
**					- NULL indicates that the boolean
**					factor element needs to be
**					allocated
**
** Outputs:
**	Returns:
**	    returns a bfp which has been properly allocated and initialized
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-apr-86 (seputis)
**          initial creation
**	22-apr-94 (ed)
**	    b59588 - changed from static
[@history_line@]...
*/
OPB_BOOLFACT *
opb_bfget(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT	   *bfp)
{
    OPB_IBF        bv;		    /* proposed number of  boolean factors 
                                    ** i.e. can be deallocated if a constant
                                    ** expression is found */

    if (!bfp)			    /* get new element if necessary , may
				    ** not be needed if previous qual was
				    ** a constant expression
				    */
    {
	bfp = (OPB_BOOLFACT *) opu_memory( subquery->ops_global,
	    (i4) sizeof(OPB_BOOLFACT) ); /* allocate memory for boolean
				    ** factor element */

	/* assign the boolean factor to the boolean factor array and increment
	** the boolean factor array count
	*/
        if ( (bv = (subquery->ops_bfs.opb_bv++)) >= OPB_MAXBF)
           opx_error( E_OP0302_BOOLFACT_OVERFLOW );
	subquery->ops_bfs.opb_base->opb_boolfact[ bv ] = bfp;
    }
    else
	bv = bfp->opb_bfindex;

    /* initialize the components of the boolean factor element */
    MEfill( sizeof(OPB_BOOLFACT), (u_char) 0,  (PTR)bfp);
                                    /* initialize equivalence class map to 0 */
    bfp->opb_bfindex = bv;
    bfp->opb_eqcls = OPB_BFNONE;    /* no equivalence classes found */
    /*bfp->opb_parms = FALSE;*/	    /* no parameter constants have been
				    ** found in the query tree
				    */
    /*bfp->opb_virtual = NULL;*/    /* no subselect or correlated vars have 
                                    ** been found */
    /*bfp->opb_keys = NULL;*/	    /* no keys have been found */
    /*bfp->opb_corelated = FALSE;*/ /* no corelated constants found yet */
    /*bfp->opb_mask = 0 */          /* mask of various booleans */
    if (subquery->ops_global->ops_goj.opl_mask & OPL_OJFOUND)
        bfp->opb_ojid = OPL_NOINIT; /* init outer join ID for this qualification */
    else
        bfp->opb_ojid = OPL_NOOUTER;
    subquery->ops_bfs.opb_ncheck = TRUE; /* this boolean factor will
				    ** eliminate NULLs unless an IS NULL
				    ** predicate is found */
    return(bfp);
}

/*{
** Name: opb_isnullck	- search for "is null" in "case" under BOP/UOP
**
** Description:
**      Search PST_CASEOP tree for "is null" predicates in an outer join
**	query. Such BOP/UOPs must be flagged for evaluation after all OJ
**	attrs have been materialized.
**
** Inputs:
**      subquery                        ptr to subquery state
**      nodep				ptr to parse tree fragment under BOP
**
** Outputs:
**	Returns:
**	    TRUE	- if there is an "is [not] null" in CASE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-march-2009 (dougi) b121798
**	    Written for "case" support under OJs.
*/
static bool
opb_isnullck(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*nodep)

{

    /* Just look at current node for UOP that is "is [not] null". If it isn't,
    ** recurse left and right. */
    if (nodep->pst_sym.pst_type == PST_UOP &&
	(nodep->pst_sym.pst_value.pst_s_op.pst_opno ==
		subquery->ops_global->ops_cb->ops_server->opg_isnull ||
	nodep->pst_sym.pst_value.pst_s_op.pst_opno ==
		subquery->ops_global->ops_cb->ops_server->opg_isnotnull))
	return(TRUE);

    if (nodep->pst_left && opb_isnullck(subquery, nodep->pst_left))
	return(TRUE);

    if (nodep->pst_right && opb_isnullck(subquery, nodep->pst_right))
	return(TRUE);

    return(FALSE);

}

/*{
** Name: opb_nulljoin	- look for NULL eliminating boolean factors
**
** Description:
**      For distributed it is important to avoid NULL join link backs 
**      for temporaries, so detect which boolean factors will cause 
**      null joins to become regular joins, and mark the equivalence 
**      classes appropriately.  This routine is used to mark equivalence
**	classes as not NULL, if the equivalence class appears in every
**      phrase which is ORed, and is not part of a IS NULL phrase.  This
**      will cause any tuple containing NULL for this equivalence class
**      to be eliminated when this boolean factor is evaluated.
**
** Inputs:
**      subquery                        ptr to subquery state
**      bp                              ptr to boolean factor being
**					considered
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
**      27-dec-88 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opb_nulljoin(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT	   *bp,
	bool		   first_time)
{
    if (!subquery->ops_bfs.opb_njoins)
    {
	subquery->ops_bfs.opb_njoins = (OPE_BMEQCLS *)opu_memory(
	    subquery->ops_global,
	    (i4)sizeof(*subquery->ops_bfs.opb_njoins));
	subquery->ops_bfs.opb_tjoins = (OPE_BMEQCLS *)opu_memory(
	    subquery->ops_global,
	    (i4)sizeof(*subquery->ops_bfs.opb_tjoins));
    }
    if (first_time)
    {	/* initialize the equivalence class bit maps for the NULLable
	** check to be equal to the equivalence classes in the first
	** phrase */
	MEcopy((PTR)&bp->opb_eqcmap, 
	    sizeof(*subquery->ops_bfs.opb_njoins),
	    (PTR)subquery->ops_bfs.opb_njoins);
    }
    else
    {	/* switch the bit maps since opb_tjoins is a subset of opb_njoins
	** and it is the subset of those equivalence classes which still
	** can be made not nullable */
	OPE_BMEQCLS	*temp;
	temp = subquery->ops_bfs.opb_tjoins;
	subquery->ops_bfs.opb_tjoins = subquery->ops_bfs.opb_njoins;
	subquery->ops_bfs.opb_njoins = temp;
	bp->opb_mask |= OPB_ORFOUND;
    }
    MEfill(sizeof(*subquery->ops_bfs.opb_tjoins), (u_char)0,
	(PTR)subquery->ops_bfs.opb_tjoins);
}

/*{
** Name: opb_bfinit	- initialize the boolean factor element
**
** Description:
**      This routine will recursively search for all elements connected
**      OR's in the following form.
**
**            (a1 op c1) OR (a2 op c2) OR ... OR (<boolean expression>)
**
**      If the "<boolean expression>" does not exist, and all the attributes
**      a1, a2, ... belong to the same equivalence class, then the qualification
**      can be used for keyed access.  Otherwise, just convert the constants
**      c1,c2, ... so that histogram processing can use them.  Also, map
**      the equivalence classes used in the qualification.
**
**	In the post-CNF Ingres world, boolean expressions may now also contain
**	nested ANDs (themselves possibly containing nested ORs). This currently
**	triggers reduced optimization; e.g. keylist structures are NOT built
**	for restriction predicates under complex disjuncts. Some analysis is
**	still performed, though, such as the gleaning of eqclass info.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bfp                             ptr to boolean factor being
**                                      initialized
**      root                            ptr to node being analyzed
**	complex				ptr to flag indicating whether we are
**					analyzing a disjunct with nested AND
**
** Outputs:
**	Returns:
**	    TRUE - if query tree fragment contains only expressions of
**	      the form (A <relop> <const expr>) OR (B <relop> <const expr>) ... 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-apr-86 (seputis)
**          initial creation
**      4-mar-90 (seputis)
**	    add more accurate handling for hash keys estimates
**	4-apr-91 (seputis)
**	    added OPB_NOKEY to help correctly estimate keying
**	    cost for project restricts
**      15-may-92 (seputis)
**          b44107 - do not change null join to regular join if
**      ifnull is used in expression
**	28-sep-92 (ed)
**	    b46656 - switch pointer and var node, otherwise
**	    histograms estimates will be calculated the incorrect
**	    way for inequalities if constant is on left
**	    FIXME - parts of this fix should probably be in ADF.
**      5-jan-93 (ed)
**          fix b48049 - constant equivalence handling in joins
**	22-apr-94 (ed)
**	    b59588 - changed from static
**      16-nov-94 (inkdo01)
**          - bug 63904 Only follows opl_translate 1 level to move ON 
**          clause of folded outer join. Should pursue until the next 
**          containing outer join is found.
**	13-sep-1996 (inkdo01)
**	    Make <> (NE) preds look "sort of" sargable, so key structure
**	    can be built (to improve selectivity estimates later on). 
**	10-apr-97 (inkdo01)
**	    Support ANDs under ORSs in post-CNF world.
**	 4-apr-00 (hayke02)
**	    The fix for bug 95161 has been moved from opj_translate()
**	    to here - we now check for an out-of-range outer join id
**	    (ojid >= opl_lv) before checking for translatable outer
**	    joins. This change fixes bug 100871.
**	12-may-00 (inkdo01)
**	    Drop unused OPB_COMPLEX flag.
**	15-sep-00 (inkdo01)
**	    Support opl_translate value of OPL_NOOUTER, since oj's can be 
**	    folded into top level join which may have NOOUTER as joinid.
**	    (bug 87175 and who knows how many others)
**	20-oct-00 (hayke02)
**	    Reinstate use of trans2 to find nested translated OJ's - without
**	    trans2 translated was always being set to either OPL_NOINIT (-2)
**	    or OPL_NOOUTER (-1) rather than >= 0 values, resulting in ojid
**	    and pst_joinid not being set correctly. This would eventually
**	    lead to E_OP0287 errors. This change fixes bug 102969.
**	5-oct-01 (inkdo01)
**	    Minor tweak to above change to clear up a OP0482 issue with OJs.
**	29-may-02 (inkdo01)
**	    Set flag for non-constant funcs with constant/parm-less plists.
**	16-dec-03 (inkdo01)
**	    Add support for iterative IN-list structures.
**	06-Jun-2008 (kiria01) b120476  
**	    Don't progress keybuild on a comparison if AD_NOKEY is set on the
**	    datatype.
**	14-Nov-08 (kibro01) b121225
**	    Added OPL_UNDEROR to determine if during tree traversal we are
**	    underneath an OR statement, whereupon "is not null" must be dealt
**	    with the same as "is null" - i.e. it cannot be determined until
**	    the relevant OJ parts are assembled.  It only degrades the join
**	    in the case where it is not used under an OR.
**	15-march-2009 (dougi) b121798
**	    Check "case" exprs under BOPs for "is null" in OJ queries.
**	02-Dec-2009 (kiria01) b122622
**	    Pre-scan simple INLIST to see if sorted appropriatly to make the
**	    grade for INLISTSORT.
**	    Allow for a range key to be build if count exceeeds threshold
**	    otherwise use existing eq keys.
**	04-Mar-2010 (kiria01) b123370
**	    Correct loop logic with the quick scan for pre-sorted data in the inlist.
**	    The loop previously exited one iteration too soon thereby missing the
**	    last element.
**      02-Apr-2010 (thich01)
**          Add an exception for GEOM family when checking for Peripherals. 
**	24-Jul-2010 (kiria01) b124124
**	    Don't just check for pre-sorted - perform sort, biased to pre-sorted
**	    data.
**	28-Jul-2010 (kiria01) b124138
**	    REPEATABLE queries with parameters in the IN list should block attempts
**	    to pre-sort for ADE_COMPAREN.
**	14-Sep-2010 (thich01)
**	    Change the order of the GEOM family exception to ensure the
**	    conditions are checked correctly.
*/
bool
opb_bfinit(
	OPS_SUBQUERY       *subquery,
	OPB_BOOLFACT       *bfp,
	PST_QNODE          *root,
	bool		   *complex)
{
    bool	call_isconst;	/* make sure isconst is only called once
				** since substitution are being made */
    bool	isnulltest;	/* true if outer join NULL handling 
				** test has been set */
    OPL_IOUTER	maxoj;

    call_isconst = TRUE;
    isnulltest = FALSE;
    maxoj = subquery->ops_oj.opl_lv;
    if (subquery->ops_global->ops_goj.opl_mask & OPL_OJFOUND)
    {	/* if an outer join exists in this subquery then make the following
	** check for outer join ID in all PST_OP_NODEs */
	switch (root->pst_sym.pst_type)
	{
	case PST_BOP:
	case PST_UOP:
	    if (((   root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_npre_instr
		    == 
		    ADE_0CXI_IGNORE
		)
		&&
		( ( subquery->ops_global->ops_cb->ops_server->opg_isnull
		    == root->pst_sym.pst_value.pst_s_op.pst_opno
		  ) ||
		  ((subquery->ops_global->ops_cb->ops_server->opg_isnotnull
		    == root->pst_sym.pst_value.pst_s_op.pst_opno)
		   &&
		   (subquery->ops_oj.opl_smask & OPL_UNDEROR)
		  )
		
		)
		&&
		!(subquery->ops_oj.opl_smask & OPL_ISNULL)
		)
		    ||
		( root->pst_left && root->pst_left->
					pst_sym.pst_type == PST_CASEOP &&
			opb_isnullck(subquery, root->pst_left->pst_left)) ||
		( root->pst_right && root->pst_right->
					pst_sym.pst_type == PST_CASEOP &&
			opb_isnullck(subquery, root->pst_right->pst_left)))
	    {
		subquery->ops_oj.opl_smask |= OPL_ISNULL; /* mark this
					** subtree as referencing a NULL
					** handling function, which cannot be
					** evaluated lower in the tree until
					** all outer join attributes have
					** the respective special eqc available
					*/
		isnulltest = TRUE;
	    }
	    /* fall thru */
	case PST_AND:
	case PST_OR:
	case PST_AOP:
	case PST_COP:
	{
	    OPL_IOUTER	    ojid;
	    if (!subquery->ops_oj.opl_base)
	    {
		root->pst_sym.pst_value.pst_s_op.pst_joinid = 
		ojid = OPL_NOOUTER;	/* this could occur if all the
					** outer joins have degenerated
					** into regular joins */
	    }
	    else if (root->pst_sym.pst_value.pst_s_op.pst_joinid >= 0)
	    {
		OPL_IOUTER	translated, trans2;

		if (root->pst_sym.pst_value.pst_s_op.pst_flags & PST_OJJOIN)
		    bfp->opb_mask |= OPB_OJJOIN; /* mark boolean factor as
					** being part of an equi join which
					** should not participate in histogramming
					** but only in relation placement of
					** outer joins */
		ojid = root->pst_sym.pst_value.pst_s_op.pst_joinid;
		if (ojid < maxoj)
		{
		    for (trans2 = translated = subquery->ops_oj.opl_base->
			opl_ojt[ojid]->opl_translate; trans2 > OPL_NOOUTER;
			translated = trans2, trans2 = subquery->ops_oj.
			opl_base->opl_ojt[trans2]->opl_translate) ;
					/* find top of folded join 
					** hierarchy */

		    if (translated != OPL_NOINIT)
		    {
			ojid = (trans2 == OPL_NOOUTER) ? OPL_NOOUTER : 
			    translated;	/* the outer join can be translated
					** in order to eliminate redundant joins 
					** - the substitution is done here in
					** order to eliminate an extra pass of
					** the query tree */
			root->pst_sym.pst_value.pst_s_op.pst_joinid = ojid;
		    }
		}
		else
		    ojid = translated = OPL_NOINIT;
	    }
	    else
		ojid = OPL_NOOUTER;
	    if (bfp->opb_ojid == OPL_NOINIT)
	    {
		bfp->opb_ojid = ojid;
		if (ojid >= 0)
		{
		    BTset((i4)bfp->opb_bfindex, (char *)subquery->ops_oj.
			opl_base->opl_ojt[ojid]->opl_bfmap);
		    subquery->ops_oj.opl_base->opl_ojt[ojid]->opl_mask |= OPL_BFEXISTS;
					/* mark outer join with having at least one
					** boolean factor to be evaluated at this
					** outer join node */
		}
	    }
	    else if ( ojid != bfp->opb_ojid)
		opx_error(E_OP039F_BOOLFACT); /* check to make sure that
					** all parts of this boolean factor
					** have the same join id */
	}
	default:
	    ;
	}
    }

    switch (root->pst_sym.pst_type)
    {

    case PST_OR:
    {
	bool                  leftsarg;     /* TRUE if left side is sargable */
        bool                  rightsarg;    /* TRUE if right side is sargable */
	bool		      or_complex = *complex;
	bool		      prev_underor = FALSE;

	if (or_complex) subquery->ops_bfs.opb_ncheck = FALSE;
			/* OR under AND under OR disables null check */

	if (subquery->ops_oj.opl_smask & OPL_UNDEROR)
	    prev_underor = TRUE;
	subquery->ops_oj.opl_smask |= OPL_UNDEROR;

	leftsarg = opb_bfinit(subquery, bfp, root->pst_left, &or_complex);
	if (or_complex) leftsarg = FALSE;
	if (leftsarg && bfp->opb_mask & (OPB_SPATF | OPB_SPATJ))
	{		/* no OR optimization of spatials */
	    bfp->opb_mask &= ~(OPB_SPATF | OPB_SPATJ);
	    leftsarg = FALSE;
	}
	if (subquery->ops_bfs.opb_ncheck)
	    opb_nulljoin(subquery, bfp, (bfp->opb_orcount == 0)); /* check for
					    ** non-nullable equivalence clses */
	or_complex = *complex;
	rightsarg = opb_bfinit(subquery, bfp, root->pst_right, &or_complex);
	if (or_complex) rightsarg = FALSE;
	if (rightsarg && bfp->opb_mask & (OPB_SPATF | OPB_SPATJ))
	{		/* no OR optimization of spatials */
	    bfp->opb_mask &= ~(OPB_SPATF | OPB_SPATJ);
	    rightsarg = FALSE;
	}
	if (subquery->ops_bfs.opb_ncheck)
	    opb_nulljoin(subquery, bfp, FALSE); /* check for non-nullable 
					    ** equivalence classes */
        bfp->opb_orcount++;		    /* keep track of or's for histogram
                                            ** processing */
	if (isnulltest)
	    subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
	if (!prev_underor)
	    subquery->ops_oj.opl_smask &= (~OPL_UNDEROR);

        return(leftsarg && rightsarg);
    }
    case PST_AND:
    {	/* ANDs can appear under ORs in the post CNF world of Ingres. Must
	** still recursively analyze, but sargability is now gone. */
	*complex = TRUE;		    /* nested AND ==> complex disj. */
	opb_bfinit(subquery, bfp, root->pst_right, complex);
	if (subquery->ops_bfs.opb_ncheck)
	    opb_nulljoin(subquery, bfp, (bfp->opb_orcount == 0)); /* check for
					    ** non-nullable equivalence clses */
	opb_bfinit(subquery, bfp, root->pst_left, complex);
	if (subquery->ops_bfs.opb_ncheck)
	    opb_nulljoin(subquery, bfp, (bfp->opb_orcount == 0)); /* check for
					    ** non-nullable equivalence clses */
	return(FALSE);			    /* ANDs under ORs are always 
					    ** unsargable */
    }
    case PST_UOP:
    {	/* check for special case "IS NULL" and "IS NOT NULL" operators which
        ** can be used for keying */
	PST_QNODE		*var1;	    /* ptr to PST_VAR node associated
                                            ** with IS NULL */
	ADI_OP_ID              opno1;	    /* operator ID of PST_UOP node */
	bool		       isnull;	    /* TRUE if operator IS NULL */
	opno1 = root->pst_sym.pst_value.pst_s_op.pst_opno; /* operator id of
					    ** the node */
	isnull =(opno1 == subquery->ops_global->ops_cb->ops_server->opg_isnull);
	if (isnull)
	    subquery->ops_bfs.opb_ncheck = FALSE; /* do not matter which eqcls
					    ** is referenced, this boolean factor
					    ** cannot guarantee NULLs to be
					    ** eliminated */
	if ((
		(   isnull
		||
		    (opno1
		    == 
		    subquery->ops_global->ops_cb->ops_server->opg_isnotnull
		    )
		)
	    )
	    &&
	    ((var1 = root->pst_left)->pst_sym.pst_type == PST_VAR)
	    &&
	    (	var1->pst_sym.pst_value.pst_s_var.pst_vno
		!= 
		subquery->ops_joinop.opj_select) /* do not create keys
				    ** for subselect range variable */
	    ) 
	{	/* IS NULL operator found which can be used for keying */
            bool        tempconst;
            opb_isconst(subquery, bfp, &root->pst_left, &tempconst, FALSE);
                                        /* for outer joins make sure opb_isconst
                                        ** gets called for all var nodes since
                                        ** it sets the iftrue semantics */
	    if (!(*complex)) opb_bfkey( subquery, bfp, var1, root, 
			(PST_QNODE *)NULL); /* 
					** create the key and
					** histogram information since a
                                        ** limiting predicate was found
                                        */
	    if (isnulltest)
		subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
	    return(TRUE);
	}
	break;
    }
    case PST_BOP:
    {	
	/* Check if this boolean factor is of the correct form i.e.
        **
        **              (a op c)     where a is an attribute
        **                                 c is a constant
        **                                 op is a =,>,<,>=,<=
        **
        ** - if it is then create a key info element for it
        ** - otherwise just map the left and right part of the tree
        */
	ADI_OP_ID              opno;	/* operator ID of PST_BOP node */
	PST_QNODE              *var;    /* will contain ptr to PST_VAR query
                                        ** tree node if test is successful
                                        */
	PST_QNODE              *constant;  /* will contain ptr to PST_CONST
                                        ** query tree node if test is successful
                                        */
	bool		       leftconst; /* TRUE if the left operand contains
					** only constant nodes
					*/
	bool		       rightconst; /* TRUE if the right operand contains
					** only constant nodes
					*/
	ADI_FI_ID	       complement; /* complement function instance
                                        ** id */
        PST_QNODE              *rightvar;
        PST_QNODE              *leftvar;
        bool                    varbopconst_or_constbopvar = FALSE;
	i4			dtbits;

	/* First check for constant funcs that aren't really constant. */
	if (root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags 
		& ADI_F32_NOTVIRGIN)
	{
	    bfp->opb_mask |= OPB_NOTCONST;
	    break;
	}

	/* An assumption that is used here: (as documented in the ADF) is
	** that if the complement function instance exists then it is one
	** of the comparison operators.  If the function instance does not
	** exist then it would contain an ADI_NOFI id.
	*/
        complement = root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_cmplmnt; /*
					** get the complement function instance
                                        ** id */
	opno = root->pst_sym.pst_value.pst_s_op.pst_opno; /* operator id of
					** the node
					*/
	
	if (opno != subquery->ops_global->ops_cb->ops_server->opg_eq)
	    bfp->opb_mask |= OPB_NOHASH; /* this boolean factor cannot be used
					** for hashing since only equalities
					** are allowed */
	/* map the tree but save information on the existence of the references
	** to var nodes
	*/
        leftvar = root->pst_left;
        rightvar = root->pst_right;     /* save left and right vars since
                                        ** they may be changed by outer join
                                        ** manipulation in opb_isconst,
                                        ** but it still should
                                        ** be used for histogram calculations */
        rightconst = TRUE;
        constant = root->pst_right;
        opb_isconst(subquery, bfp, &root->pst_right, &rightconst, FALSE);
        leftconst = TRUE;
        opb_isconst(subquery, bfp, &root->pst_left, &leftconst, FALSE);
        call_isconst = FALSE;
        if (leftconst)
              constant = root->pst_left;
        if (
               ((var=leftvar)->pst_sym.pst_type == PST_VAR && rightconst)
               ||
               ((var=rightvar)->pst_sym.pst_type == PST_VAR && leftconst)
             )
            varbopconst_or_constbopvar = TRUE;
	if ((*complex)) break;		/* nested ANDs go no further */

	if ( (complement != ADI_NOFI)  /* first check if this
					** is one of the comparison operators
					** i.e. LT, GT, GE, LE, EQ, NE
					*/
             &&
             (varbopconst_or_constbopvar)
             &&
             !((varbopconst_or_constbopvar) &&
               (opno == subquery->ops_global->ops_cb->ops_server->opg_ne) &&
               (constant->pst_sym.pst_value.pst_s_cnst.pst_pmspec != PST_PMNOTUSED)
              )
             &&
             (opno != subquery->ops_global->ops_cb->ops_server->opg_notlike)
                                        /* do not consider the NOT LIKE
                                        ** operator */
             &&
             (  var->pst_sym.pst_value.pst_s_var.pst_vno
                !=
                subquery->ops_joinop.opj_select
             )                          /* do not create keys
                                        ** for subselect range variable */
	     &&
	     !(	/* And only progress key build if key supported */
	    	adi_dtinfo(subquery->ops_global->ops_adfcb,
			var->pst_sym.pst_dataval.db_datatype, &dtbits) ||
		(dtbits & AD_NOKEY)
	      )
           )
	{
	    /* If the constant is on the left side and we must reverse the 
            ** operator.  The node should be in the form (a op c)
	    ** where "a" is an attribute and "c" is a constant
	    */
	    if (leftconst)
	    {	
		constant = root->pst_left;	/* update ptr to constant on left side*/

                if (opno != subquery->ops_global->ops_cb->ops_server->opg_eq &&
                    opno != subquery->ops_global->ops_cb->ops_server->opg_ne)
                {   /* obtain the operator id of the inverse function */
                    PST_QNODE          *temp_qnodep; /* used to switch nodes */

                    temp_qnodep = root->pst_right;
                    root->pst_right = root->pst_left;
                    root->pst_left = temp_qnodep;
                    switch (root->pst_sym.pst_value.pst_s_op.pst_opno)
                    {
                    case ADI_LT_OP:
                        opno = ADI_GT_OP;
                        break;
                    case ADI_LE_OP:
                        opno = ADI_GE_OP;
                        break;
                    case ADI_GT_OP:
                        opno = ADI_LT_OP;
                        break;
                    case ADI_GE_OP:
                        opno = ADI_LE_OP;
                        break;
                    default:
                        opx_error(E_OP079A_OPID);
                    }
                    root->pst_sym.pst_value.pst_s_op.pst_opno = opno;
                    opa_resolve(subquery, root);
		}
	    }


	    /* If this is an unsorted compacted IN-list, make a scan
	    ** and see if it is can be made sorted & appropriate for
	    ** the SORT compares. For this the datatypes must match
	    ** the LHS and the values must be present.
	    ** If sucessful we'll set the bit for sorted.
	    ** We do this if NE as well as EQ - it might not help the
	    ** key build but will help execution. */
	    if ((root->pst_sym.pst_value.pst_s_op.pst_flags &
			(PST_INLIST|PST_INLISTSORT)) == PST_INLIST &&
		(root->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_eq ||
		root->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_ne))
	    {
		PST_QNODE **p = &root->pst_right, *q;
		DB_DATA_VALUE *ld = &root->pst_left->pst_sym.pst_dataval;
		PST_QNODE *prior = NULL;
		i4 cnt = 0;

		while (*p)
		{
		    DB_DATA_VALUE *pd = &(*p)->pst_sym.pst_dataval;
		    q = (*p)->pst_left;
		    if (!pd->db_data ||
			(*p)->pst_sym.pst_type != PST_CONST ||
			(*p)->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_RQPARAMNO)
			/* Give up - nodata to work with or a REPEAT parameter */
			goto notsorted;

		    if (pd->db_datatype < 0 && pd->db_length >= 0)
		    {
			if (((u_char*)pd->db_data)[pd->db_length-1] & ADF_NVL_BIT)
			{
			    /* A NULL present means an unknown result - drop all other
			    ** elements from the list & leave degraded */
			    root->pst_right = *p;
			    (*p)->pst_left = NULL;
			    break;
			}
			/* Otherwise un-NULL it. */
			pd->db_datatype = -pd->db_datatype;
			pd->db_length--;
		    }
		    if (pd->db_datatype != abs(ld->db_datatype) ||
			pd->db_datatype == DB_DEC_TYPE &&
				pd->db_prec != ld->db_prec)
			/* Mismatch - can't make sorted */
			goto notsorted;

		    if (prior) /* Prior data item if any */
		    {
			i4 cmp;
			if (adc_compare(subquery->ops_global->ops_adfcb,
						&prior->pst_sym.pst_dataval, pd, &cmp))
			    goto notsorted; /* Error in compare - give up */
			if (cmp > 0)
			{
			    /* Out of order - specifically this must be
			    ** before the point we are currently at but
			    ** as we cannot back track, we need to find
			    ** insertion point from the root :-( */
			    register PST_QNODE **p1 = &root->pst_right;
			    PST_QNODE *hold = *p;
			    /* Unlink from current position */
			    *p = q;
			    /* Search from the root like before except we can
			    ** dispense with most of the checks as they have all
			    ** passed already. Also we know we need to insert before
			    ** 'prior' */
			    while (*p1 != prior)
			    {
				if (adc_compare(subquery->ops_global->ops_adfcb,
						&(*p1)->pst_sym.pst_dataval, pd, &cmp))
				    goto notsorted; /* Error in compare - give up */
				if (!cmp)
				    /* Simply forget this duplicate & resume outer loop */
				    break;
				if (cmp > 0)
				{
				    hold->pst_left = *p1;
				    *p1 = hold;
				    break;
				}
				p1 = &(*p1)->pst_left;
			    }
			    if (*p1 == prior)
			    {
				hold->pst_left = prior;
				*p1 = hold;
				if (++cnt >= MAXI2-2)
				    /* Too big for ADE_COMPAREN - very unlikely as eSQL
				    ** has similar limit. In fact parser now should not
				    ** exceed this as better to do several mass compares
				    ** than to chain compares */
				    goto notsorted;
			    }
			    continue;
			}
			if (!cmp)
			{
			    /* Drop duplicate */
			    if (root->pst_right->pst_left)
				*p = q;
			    continue;
			}
		    }
		    if (++cnt >= MAXI2-2)
			/* Too big for ADE_COMPAREN - See comment above.  */
			goto notsorted;

		    prior = *p;
		    p = &(*p)->pst_left;
		}
		/* We fell out clean so can treat as a compatible sorted list */
		root->pst_sym.pst_value.pst_s_op.pst_flags |= PST_INLISTSORT;

notsorted:	/* Reset constant in case list rearranged */
		constant = root->pst_right;
	    }

	    /* Sanity test that we have at least 2 values.
	    ** If not drop this to a simple eq/ne. */
	    if ((root->pst_sym.pst_value.pst_s_op.pst_flags &
			(PST_INLIST|PST_INLISTSORT)) &&
			!constant->pst_left)
	    {
		/* Degraded list make simple eq/ne */
		root->pst_sym.pst_value.pst_s_op.pst_flags &= ~(PST_INLIST|PST_INLISTSORT);
	    }

	    /* If this is a sorted compacted IN-list, if the inlist threshold is
	    ** exceeded, call opb_bfkey once with PST_INLISTSORT set which will trigger
	    ** a lowest & highest bound to be taken to create a range key */
	    if ((root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLISTSORT) &&
		root->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_eq)
	    {
		PST_QNODE *p = constant;
		i4 cnt = 1;
		/* How many and where is the end */
		while (p->pst_left)
		{
		    p = p->pst_left;
		    cnt++;
		}
		/* This threshold would be better computed based on available resources
		** and indexs in the cost optimiser where potentially a better decision
		** could be made. For the moment, the threshold is static and in place
		** to allow inlists of great size to be unhampered with the key overheads
		** that might otherwise fail through resource needs. */
		if (subquery->ops_global->ops_cb->ops_alter.ops_inlist_thresh &&
			cnt > subquery->ops_global->ops_cb->ops_alter.ops_inlist_thresh)
		    /* Let opb_bfkey try to build a range key */
		    opb_bfkey(subquery, bfp, var, root, constant);
		else
		{
		    /* Avoid range key build for small numbers */
		    root->pst_sym.pst_value.pst_s_op.pst_flags &= ~PST_INLISTSORT;
		    for (; constant; constant = constant->pst_left)
			opb_bfkey(subquery, bfp, var, root, constant);
		    root->pst_sym.pst_value.pst_s_op.pst_flags |= PST_INLISTSORT;
		}
	    }
	    /* If this is a compacted IN-list, loop down PST_CONSTs adding 
	    ** them all to the keylist. (but not sorted) */
	    else if ((root->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
		root->pst_sym.pst_value.pst_s_op.pst_opno ==
		    subquery->ops_global->ops_cb->ops_server->opg_eq)
	    {
		for (; constant; constant = constant->pst_left)
		{
		    opb_bfkey(subquery, bfp, var, root, constant);
		}
	    }
	    else
	    {
		/* Create the single key and histogram information since a
		** limiting predicate was found */
		opb_bfkey( subquery, bfp, var, root, constant);
	    }

	    if (isnulltest)
		subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
	    return(TRUE);
	}

        /* Rtree spatial predicate filter logic. This code looks for
        ** spatial predicates in which one parameter/operand is a
        ** column (PST_VAR) and the other is a constant/host variable or
        ** a coercion of a constant/host variable or a column from another
        ** table (indicating a spatial join). Columns must NOT be one of
        ** the "long" types.
        **
        ** Spatial predicates can be coded either in infix notation
        ** (e.g. "... col1 inside expr ...") or using function notation
        ** (e.g. "... inside(col1, expr) = 1 ...". If function notation
        ** is used, this logic requires the "=" comparison to be to a
        ** constant 1 or (in the case of the "inside" function) to a
        ** constant 0. adi_opid calls are made to determine if the spatial
        ** functions are even known in this server (and for comparison
        ** with parse tree op codes).
        **
        ** Qualifying predicates are considered for later use of Rtree
        ** access strategies.
        */
 
        if (!(subquery->ops_global->ops_cb->ops_check
            &&
            opt_strace(subquery->ops_global->ops_cb, OPT_F044_NO_RTREE)))
                                /* No Rtree trace pt isn't on */
        {
            PST_QNODE   *spatf = root->pst_left;
            PST_QNODE   *lvar, *rvar;
            ADI_OP_ID   inside_op, intersects_op, overlaps_op;
            DB_STATUS   status;
            bool        infix;
 
            do	/* something to break from */
            {
                status = adi_opid(subquery->ops_global->ops_adfcb,
                        "inside", &inside_op);
                if (DB_FAILURE_MACRO(status)) break;
                                /* if operators not here, quit */
                status = adi_opid(subquery->ops_global->ops_adfcb,
                        "intersects", &intersects_op);
                if (DB_FAILURE_MACRO(status)) break;
                status = adi_opid(subquery->ops_global->ops_adfcb,
                        "overlaps", &overlaps_op);
                if (DB_FAILURE_MACRO(status)) break;
                infix = (opno == inside_op || opno == intersects_op ||
                        opno == overlaps_op);
                if (infix) spatf = root;
                else {
                    if (opno != subquery->ops_global->ops_cb->ops_server->
                        opg_eq || !(leftconst || rightconst)) break;
                                /* better be valid function-style */
                    if (leftconst) spatf = root->pst_right;
                    if (!(spatf->pst_sym.pst_type == PST_BOP &&
                        (spatf->pst_sym.pst_value.pst_s_op.pst_opno
                                        == inside_op ||
                        spatf->pst_sym.pst_value.pst_s_op.pst_opno
                                        == intersects_op ||
                        spatf->pst_sym.pst_value.pst_s_op.pst_opno
                                        == overlaps_op))) break;
                                /* it's a spatial func under an "=" */
 
                    constant = (leftconst) ? root->pst_left : root->pst_right;
                    if (constant->pst_sym.pst_value.pst_s_cnst.pst_tparmtype
                                        != PST_USER ||
                        constant->pst_sym.pst_dataval.db_datatype
                                        != DB_INT_TYPE ||
                        constant->pst_sym.pst_dataval.db_length != 2 ||
                        *((i2 *)constant->pst_sym.pst_dataval.db_data) != 1)
                                break;  /* "=" compares to 1 */
                }               /* end of "func = 0/1" tests */
 
                constant = (spatf->pst_left->pst_sym.pst_type == PST_VAR) ?
                        spatf->pst_right : spatf->pst_left;
                                /* constant is the non-column function parm */
                lvar = spatf->pst_left;
                rvar = spatf->pst_right;
                if (lvar->pst_sym.pst_type != PST_VAR &&
                    rvar->pst_sym.pst_type != PST_VAR) break;
                                /* one of these buggers better be a col. */
                if (constant->pst_sym.pst_type == PST_CONST ||
                                /* 2nd operand is constant */
                        constant->pst_sym.pst_type == PST_UOP &&
                        constant->pst_left->pst_sym.pst_type == PST_CONST ||
                                /* 2nd operand is coercion of constant */
                        lvar->pst_sym.pst_type == PST_VAR &&
                        rvar->pst_sym.pst_type == PST_VAR &&
                        lvar->pst_sym.pst_value.pst_s_var.pst_vno !=
                         rvar->pst_sym.pst_value.pst_s_var.pst_vno)
                                /* 2nd operand is col from another table */
                {
                    i4          dtmask = 0;
 
                    /* Check that VAR's are NOT long types. 
                       Unless the long type is a member of the GEOM family.*/
                    status = adi_dtinfo(subquery->ops_global->ops_adfcb,
                        lvar->pst_sym.pst_dataval.db_datatype, &dtmask);
                    if (status != E_DB_OK) break;
                    if (dtmask & AD_PERIPHERAL)
                    {
                        DB_DT_ID family = adi_dtfamily_retrieve(lvar->pst_sym.pst_dataval.db_datatype);
                        if(family != DB_GEOM_TYPE)
                           break;
                    } 
                    if (lvar->pst_sym.pst_type == PST_VAR &&
                        rvar->pst_sym.pst_type == PST_VAR)
                    {
                        status = adi_dtinfo(subquery->ops_global->ops_adfcb,
                            rvar->pst_sym.pst_dataval.db_datatype, &dtmask);
                        if (status != E_DB_OK) break;
                        if (dtmask & AD_PERIPHERAL)
                        {
                            DB_DT_ID family = adi_dtfamily_retrieve(rvar->pst_sym.pst_dataval.db_datatype);
                            if(family != DB_GEOM_TYPE)
                               break;      /* check 2nd operand, too */
                        } 
 
                        bfp->opb_mask |= OPB_SPATJ;
                        subquery->ops_bfs.opb_mask |= OPB_GOTSPATJ;
                    }
                    else
                    {
                        bfp->opb_mask |= OPB_SPATF;
                        subquery->ops_bfs.opb_mask |= OPB_GOTSPATF;
                    }
                    return(TRUE);
                }
		/* else non-col must be const or coercion */
	    } while (0);
        }

        if ((root->pst_sym.pst_type == PST_BOP ||root->pst_sym.pst_type == PST_MOP) &&
         root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags
		& ADI_F32768_IFNULL)
        {   /* do not eliminate null join operator if ifnull operator used
            ** over an expression */
            /* FIXME - when UDT support nulls then any user defined function
            ** cannot be trusted either, and reset this flag */
            subquery->ops_bfs.opb_ncheck = FALSE; /* does not matter which eqcls
                                            ** is referenced, this boolean factor
                                            ** cannot guarantee NULLs to be
                                            ** eliminated */
        }
	break;
    }
    case PST_COP:
    {
	/* First check for constant funcs that aren't really constant. */
	if (root->pst_sym.pst_value.pst_s_op.pst_fdesc->adi_fiflags 
		& ADI_F32_NOTVIRGIN)
	 bfp->opb_mask |= OPB_NOTCONST;
	break;
    }
    default:
    	;
    }

    bfp->opb_mask |= OPB_DNOKEY;	/* this disjunct cannot be used for
					** keying so mark the entire boolean
					** factor as unsuitable for keying estimates */
    if (call_isconst)
    {
	/* - update the boolean factor equivalence class map
	*/
	bool		novars;  /* dummy */
	if (root->pst_left)
            opb_isconst( subquery, bfp, &root->pst_left, &novars, TRUE);
        if (root->pst_right)
            opb_isconst( subquery, bfp, &root->pst_right, &novars, TRUE);
    }
    if (isnulltest)
	subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
    return(FALSE);		/* return FALSE indicating a
				** non-sargable predicate was found
				*/
}

/*{
** Name: opb_rtminmax	- process range type boolean factors
**
** Description:
**      Update the OPB_GLOBAL structure for sargable boolean factors with
**      the same equivalance class, type and length
** 
** 	This is the piece of code will process range type boolean factors
** 	-range type boolfacts have at least one relational (<, >, <=, >=)
** 	operator (a relational operator is also
** 	implied by pattern matching chars in a constant or similar stuff
** 	in abstract datatypes) and nothing else except possibly ='s.
**
** 	The idea is to find the most restrictive limits for the search.
** 	For instance, if there are several max value clauses in a 
** 	boolfact, then we need to take the max (least restrictive) 
** 	of these because they are connected by OR's.  If there are 
** 	several such boolfacts then take the min of all the max's 
** 	we just found cause the boolfacts are connected by AND's.
**
** 	Due to the creation process for the boolean factor, there will
** 	be exactly the same set of OPB_BFKEYFINFO headers associated
** 	for an sargable predicate.  Moreover, they will be in the same
** 	order in the "opb_keys" list.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      bfp                             ptr to boolean factor which is 
**                                      sargable, and thus is associated 
**                                      with an equivalence class
**      bfi                             boolean factor number associated
**                                      with bfp
** Outputs:
**      eqclsp->ope_nbf                 boolean factor with "range type"
**                                      min and max will be updated to
**                                      reflect the bfp min max values
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-jun-86 (seputis)
**          initial creation
**	18-jul-89 (seputis)
**	    add union view support
**	26-apr-93 (ed)
**	    if an outer join then do not mark partitions in a union view
**	    as being eliminated by qualifications within an on clause
**	    bug 49821
**	20-jul-93 (ed)
**	    fix uninitialized variable problem which would cause the
**	    optimization of pushing qualifications into a union view
**	    partitions to be skipped occasionally
**	13-sep-1996 (inkdo01)
**	    <> (NE) preds are now technically sargable. Take quick exit
**	    from here.
**	29-oct-97 (inkdo01)
**	    Slight change to set opb_qfalse for predicates which will return
**	    no rows.
**	22-apr-98 (inkdo01)
**	    Even more slight change to detect idiot queries such as 
**	    "where c1 = 25 and c1 is null".
**	10-dec-02 (hayke02)
**	    Modify the above change so that we check for a NULL opb_next.
**	    This will prevent the code from incorrectly detecting and
**	    modifying a query of the form "where (c1 is not null and c1 = 1)
**	    or (c1 is null)". This change fixes bug 109198.
**	28-jan-03 (inkdo01)
**	    Slighter yet change to prevent above change from making OJ
**	    queries look like idiots.
**      22-sep-03 (wanfr01)
**          Bug 110436, INGSRV 2385
**          Do not run this routine if processing an outer join, or you may
**          incorrectly end up with zero rows.
**	12-Feb-09 (kibro01) b121568
**	    Remove bug fix for b110436 since it removes setup allowing keys
**	    from outer join elements to be used.  The original bug fix is no
**	    longer required, since outer join elements are produced as 
**	    separate equivalence classes (see op156 output of the testcase of
**	    bug 110436 and the same with inner join to see the difference).
[@history_line@]...
*/
static VOID
opb_rtminmax(
	OPS_SUBQUERY	   *subquery,
	OPB_BOOLFACT       *bfp,
	OPE_EQCLIST        *eqclsp,
	OPB_IBF            bfi)
{
    bool                firsttime;      /* TRUE if first time an sargable
                                        ** predicate has been found on this
                                        ** equivalence class
                                        */
    OPS_STATE           *global;        /* ptr to global state variable */
    OPB_BFKEYINFO       *masterbfinfo;  /* ptr to OPB_BFKEYINFO element which
                                        ** contains the "valid" OPB_GLOBAL
                                        ** structure */
    OPB_BFKEYINFO       *updatebfinfo;  /* ptr to OPB_BFKEYINFO element to be
                                        ** used to update the global structure
                                        */

    global = subquery->ops_global;      /* get ptr to global state variable */
    updatebfinfo = bfp->opb_keys;	/* this is the list of min max
					** values which will be used
					** to update the master list
					*/

    /* To aid in detecting bad where clauses with "a = 5 and a is null",
    ** a fake value is introduced for "is null" predicates. This is a
    ** very weak attempt to catch such queries, but the complexity of the 
    ** value list structures make it much more difficult to solve in 
    ** the fully correct manner. For example, this will not catch 
    ** "c1 is null and c1 is not null". */

    if (bfp->opb_keys->opb_keylist && 
	bfp->opb_ojattr == NULL &&
	bfp->opb_keys->opb_keylist->opb_null ==
		subquery->ops_global->ops_cb->ops_server->opg_isnull &&
	bfp->opb_keys->opb_keylist->opb_keyvalue == NULL &&
	bfp->opb_keys->opb_keylist->opb_next == NULL)
    switch (bfp->opb_keys->opb_bfdt->db_datatype) {

     case DB_INT_TYPE:
     case -DB_INT_TYPE:
	bfp->opb_keys->opb_keylist->opb_keyvalue = (PTR)&intc0;
	break;
     case DB_FLT_TYPE:
     case -DB_FLT_TYPE:
	bfp->opb_keys->opb_keylist->opb_keyvalue = (PTR)&doublec0;
	break;
     case DB_CHA_TYPE:
     case -DB_CHA_TYPE:
	bfp->opb_keys->opb_keylist->opb_keyvalue = (PTR)&charc0;
	break;
     }

    if (updatebfinfo && updatebfinfo->opb_sargtype == ADC_KNOKEY) return;
					/* quick exit - it's a "<>" */

    if (firsttime = (eqclsp->ope_nbf == OPB_NOBF))
    {
	eqclsp->ope_nbf = bfi;		/* save the reference to the boolean
					** factor since it is the
					** first one found for this
                                        ** equivalence class */
        masterbfinfo = updatebfinfo;    /* assign this boolean factor to be the
                                        ** master copy
                                        */
    }
    else
    {
	masterbfinfo = subquery->ops_bfs.opb_base->
	    opb_boolfact[eqclsp->ope_nbf]->opb_keys;
					/* get ptr to boolean factor
					** containing the "master"
					** list of range type boolean
					** factors */
    }

    /* need to adjust the min max list for each unique pair
    ** of (type, length) in this equivalence class */
    for(;masterbfinfo; masterbfinfo = masterbfinfo->opb_next,
		       updatebfinfo = updatebfinfo->opb_next)
    {
	bool		minvalue;       /* TRUE if updatebfinfo key should be
                                        ** used to update the opb_pmin value
                                        */
        bool            maxvalue;       /*TRUE if updatebfinfo key should be
                                        ** used to update the opb_pmax value
                                        */ 
        OPB_BFGLOBAL    *globalbool;    /* ptr to info global to all sargable
                                        ** boolean factors on this equivalence
                                        ** class, type and length */
	bool		exact;		/* TRUE if ADC_KEXACTKEY type key is found */
	OPG_CB		       *opgcb;	/* server control block */

	opgcb = global->ops_cb->ops_server;
#ifdef    E_OP0386_BFCREATE
	if ( !updatebfinfo
	     ||
	     (updatebfinfo->opb_eqc != masterbfinfo->opb_eqc)
	     ||
	     (  updatebfinfo->opb_bfdt->db_datatype 
		!= 
		masterbfinfo->opb_bfdt->db_datatype
	     )
	     ||
	     (  updatebfinfo->opb_bfdt->db_length
		!= 
		masterbfinfo->opb_bfdt->db_length
	     )
	    )
	    opx_error( E_OP0386_BFCREATE);
		/* consistency check to validate assumption
		** that lists have same (type, length) ordering
		*/
#endif
	if (firsttime)
	{	/* initialize the components of the OPB_GLOBAL structure the
	    ** first time through */

            masterbfinfo->opb_global =
	    globalbool = (OPB_BFGLOBAL *)opu_memory( global,
		(i4) sizeof(OPB_BFGLOBAL));
	    globalbool->opb_pmin = NULL;
	    globalbool->opb_pmax = NULL;
	    globalbool->opb_xpmin = NULL;
	    globalbool->opb_xpmax = NULL;
	    globalbool->opb_single = OPB_NOBF;
	    globalbool->opb_multiple = OPB_NOBF;
	    globalbool->opb_range = OPB_NOBF;
	    globalbool->opb_bfmask = 0;
	}
	else
	    updatebfinfo->opb_global =
	    globalbool = masterbfinfo->opb_global;


	if (bfp->opb_ojid >= 0)
	    globalbool->opb_bfmask |= OPB_OJSARG;	/* this qualification
				    ** is part of an outer join so it cannot
				    ** eliminate the partition */
	maxvalue = minvalue = exact = FALSE;
	switch (updatebfinfo->opb_sargtype)
	{   /* test for this inside for loop since a bfkeyinfo list can have
            ** numerous opb_sargtype values in the same list e.g. range key
            ** pattern matching values which where truncated
            */
	case ADC_KEXACTKEY:
	{   /* do not update the min/max values however need to update
	    ** the opb_single or opb_multiple structures
	    */
	    globalbool->opb_multiple = bfi; /* an exact key was
					** found with one or more values */
	    if (!updatebfinfo->opb_keylist->opb_next)
		globalbool->opb_single = bfi; /* this key has
					** exactly one value */
	    exact = TRUE;
	    break;
	}
	case ADC_KLOWKEY:
	{
	    minvalue = TRUE;		/* update the opb_pmin value */
	    maxvalue = FALSE;
            break;
	}
	case ADC_KHIGHKEY:
	{
	    minvalue = FALSE;
	    maxvalue = TRUE;            /* update the opb_pmax value */
            break;
	}
	case ADC_KRANGEKEY:
	{
	    minvalue = TRUE;            /* update the opb_pmin value */
	    maxvalue = TRUE;            /* and the opb_pmax value */
            break;
	}
	default:
	    {
		continue;
	    }
	}
	{
	    /* traverse the key list and find the min or max values where
	    ** appropriate
	    */
	    OPB_BFVALLIST         *currentvp; /* current key value being
                                        ** analyzed */
	    OPB_BFVALLIST	  *minvp; /* will contain ptr to the min value
					** for the list 
					*/
	    OPB_BFVALLIST	  *maxvp; /* will contain ptr to the max value
					** for the list 
					*/
	    OPB_BFVALLIST	  *xminvp; /* will contain ptr to the min value
					** for the list, including exact equalities
					*/
	    OPB_BFVALLIST	  *xmaxvp; /* will contain ptr to the max value
					** for the list, including exact equalities 
					*/
	    bool		  check_empty; /* TRUE if a new min or max value is
					** found, so that there is a possibility of
					** eliminating a union partition because of
					** an empty set */
	    bool		  comp_flag; /* TRUE - if the opu_compare routine was
					** called */
	    i4			  comp_value; /* result of opu_compare operation */

	    if (!exact)
		globalbool->opb_range = bfi; /* a range type
					** boolean factor was found */

	    minvp = NULL;
	    maxvp = NULL;
	    xminvp = NULL;
	    xmaxvp = NULL;
	    
	    for (currentvp = updatebfinfo->opb_keylist;
		 currentvp;
		 currentvp = currentvp->opb_next)
	    {
		OPB_BFVALLIST	    *lower; /* ptr to lower bound of range */
		OPB_BFVALLIST	    *upper; /* ptr to upper bound of range */

		lower = currentvp;
		if (currentvp->opb_operator == ADC_KRANGEKEY)
		{   /* the lower and upper bound are inserted in order for
		    ** a ADC_KRANGEKEY */
		    currentvp = currentvp->opb_next;
		    upper = currentvp;
		}
		else
		    upper = lower;          /* either a ADC_KHIGHKEY or a
					    ** ADC_KLOWKEY, or ADC_EXACTKEY
					    */

		/* for processing parameter constants which may not have a
		** value, check this case, and use a value if available,
                ** otherwise save only if it is the first one */

		if (minvalue		    /* is a minimum value required */
		    &&
		    !exact
		    &&			    /* only look at range type
					    ** qualifications */
		    (   !minvp		    /* is this the first such value */
			||
			!minvp->opb_keyvalue /* was the first one a parameter
                                            ** constant */
			||
			(   lower->opb_keyvalue /* does compare value exist */
			    &&
			    (opu_compare( global, minvp->opb_keyvalue, 
				   lower->opb_keyvalue, masterbfinfo->opb_bfdt)
				>= 0	    /* or is it smaller
					    ** than the previous such value */
			    )
			)
		     )
		    )
		{
		    minvp = lower;	    /* then save ptr to this new 
					    ** minimum */
		}

		/* in order to eliminate partitions of the union, all elements
		** of the qualification must be analysed, including equality */
		comp_flag = FALSE;
		if ((exact || minvalue)	    /* is a minimum value required */
		    &&
		    (   !xminvp		    /* is this the first such value */
			||
			!xminvp->opb_keyvalue /* was the first one a parameter
                                            ** constant */
			||
			(   lower->opb_keyvalue /* does compare value exist */
			    &&
			    ( (comp_flag = TRUE)
			      &&
			      ( (comp_value = opu_compare( global, xminvp->opb_keyvalue, 
				   lower->opb_keyvalue, masterbfinfo->opb_bfdt)
                                )
				>= 0	    /* or is it smaller
					    ** than the previous such value */
			      )
			    )
			)
		     )
		    )
		{
		    if (comp_flag && !comp_value)
		    {	/* at this point, both qualifications have equivalent
			** keys, but now prefer <= or =, over simply < */
			if ((lower->opb_opno == opgcb->opg_ge)
			    ||
			    (lower->opb_opno == opgcb->opg_eq)
			    )
			    xminvp = lower;
		    }
		    else
			xminvp = lower;
		}
		if (maxvalue		    /* is a maximum value required */
		    &&
		    !exact
		    &&
		    (   !maxvp		    /* is this the first such value */
			||
			!maxvp->opb_keyvalue /* was the first one a parameter
                                            ** constant */
			||
			(   upper->opb_keyvalue /* does compare value exist */
			    &&
			    (opu_compare( global, maxvp->opb_keyvalue, 
				upper->opb_keyvalue, masterbfinfo->opb_bfdt)  
				< 0	    /* or is it greater
					    ** than the previous such value */
			    )
			)
                     )
		    )
		    maxvp = upper;      /* then save ptr to this new 
					** maximum */
		comp_flag = FALSE;
		if ((exact || maxvalue) /* is a maximum value required */
		    &&
		    (   !xmaxvp		/* is this the first such value */
			||
			!xmaxvp->opb_keyvalue /* was the first one a parameter
                                        ** constant */
			||
			(   upper->opb_keyvalue /* does compare value exist */
			    &&
			    ( (comp_flag = TRUE)
			      &&
			      ( (comp_value = opu_compare( global, xmaxvp->opb_keyvalue, 
				   upper->opb_keyvalue, masterbfinfo->opb_bfdt)
                                )
				<= 0	    /* or is it greater
					    ** than the previous such value */
			      )
			    )
			)
		     )
		    )
		{
		    if (comp_flag && !comp_value)
		    {	/* at this point, both qualifications have equivalent
			** keys, but now prefer <= or =, over simply < */
			if ((upper->opb_opno == opgcb->opg_le)
			    ||
			    (upper->opb_opno == opgcb->opg_eq)
			    )
			    xmaxvp = upper;
		    }
		    else
			xmaxvp = upper;
		}
	    }

	    /* find the "maximum" of all the previous minimums */
	    if (minvalue		/* is a minimum value update needed 
					*/
		&&
		!exact
		&& 
		(   !globalbool->opb_pmin /* if so then is this
					** the first value found */
		    ||
		    !globalbool->opb_pmin->opb_keyvalue /* was this a parameter
					** constant */
		    ||
		    (	minvp->opb_keyvalue
			&&
			(opu_compare( global, minvp->opb_keyvalue, 
				    globalbool->opb_pmin->opb_keyvalue,
				    masterbfinfo->opb_bfdt) 
			    > 0
			)		/* OR is it
					** greater than the previous min
					** value found */
		    )
		)
	       )
		globalbool->opb_pmin = minvp; /* update the
					** minimum of all the maximums */
	    comp_flag = FALSE;
	    check_empty = FALSE;
	    if ((exact || minvalue)	    /* is a minimum value required */
		&&
		(   !globalbool->opb_xpmin		    /* is this the first such value */
		    ||
		    !globalbool->opb_xpmin->opb_keyvalue /* was the first one a parameter
					** constant */
		    ||
		    (   xminvp->opb_keyvalue /* does compare value exist */
			&&
			( (comp_flag = TRUE)
			  &&
			  ( (comp_value = opu_compare( global, xminvp->opb_keyvalue, 
				globalbool->opb_xpmin->opb_keyvalue, masterbfinfo->opb_bfdt)
			    )
			    >= 0	    /* or is it smaller
					** than the previous such value */
			  )
			)
		    )
		 )
		)
	    {
		if (!comp_flag 
		    || 
		    comp_value
		    ||
		    /* at this point, both qualifications have equivalent
		    ** keys, but now prefer > over >= or =, since it is the
		    ** higher minimum */
		    (xminvp->opb_opno == opgcb->opg_gt)
		    )
		{
		    globalbool->opb_xpmin = xminvp;
		    check_empty = TRUE;
		}
	    }

	    /* find the "minimum" of all the previous maximums */
	    if (maxvalue		    /* is a maximum value update needed 
					*/
		&&
		!exact
		&& 
		(   !globalbool->opb_pmax /* if so then is this
					** the first value found */
		    ||
		    !globalbool->opb_pmax->opb_keyvalue /* was this a parameter
					** constant */
		    ||
		    (	maxvp->opb_keyvalue
			&&
			(opu_compare( global, maxvp->opb_keyvalue, 
				globalbool->opb_pmax->opb_keyvalue,
				masterbfinfo->opb_bfdt) 
			     < 0
			)		/* OR is it
					** smaller than the previous max
					** value found */
		    )
		 )
	        )
		globalbool->opb_pmax = maxvp; /* update the
					** minimum of all the maximums */
	    comp_flag = FALSE;
	    if ((exact || maxvalue)    /* is a maximum value required */
		&&
		(   !globalbool->opb_xpmax /* is this the first such value */
		    ||
		    !globalbool->opb_xpmax->opb_keyvalue /* was the first one a parameter
					** constant */
		    ||
		    (   xmaxvp->opb_keyvalue /* does compare value exist */
			&&
			( (comp_flag = TRUE)
			  &&
			  ( (comp_value = opu_compare( global, xmaxvp->opb_keyvalue, 
				globalbool->opb_xpmax->opb_keyvalue, masterbfinfo->opb_bfdt)
			    )
			    <= 0	    /* or is it greater
					** than the previous such value */
			  )
			)
		    )
		 )
		)
	    {
		if (!comp_flag 
		    || 
		    comp_value
		    ||
		    /* at this point, both qualifications have equivalent
		    ** keys, but now prefer < over <= or = \*/
		    (xmaxvp->opb_opno == opgcb->opg_lt)
		    )
		{
		    globalbool->opb_xpmax = xmaxvp;
		    check_empty = TRUE;
		}
	    }
	    if (check_empty
		&&
		globalbool->opb_xpmax
		&&
		globalbool->opb_xpmin
		&&
		globalbool->opb_xpmax->opb_keyvalue
		&&
		globalbool->opb_xpmin->opb_keyvalue
		&&
		!(globalbool->opb_bfmask & OPB_OJSARG) /* outer joins cannot
					    ** eliminate partitions, FIXME, need
					    ** to keep outer joins conjuncts separate
					    ** or process non-outer join conjuncts first
					    ** so that this check can continue to be
					    ** made */
		)
	    {	/* since there was a change, check to see if there is
		** an overlapping range */
		DB_DT_ID	tempdt;
		/* FIXME - normally if opb_keyvalue is 0 then it is a parameter constant
		** but this also occurs for a "IS NULL" boolean, which should be handled
		** better for keying purposes */
		comp_value = opu_compare( global, globalbool->opb_xpmin->opb_keyvalue, 
		       globalbool->opb_xpmax->opb_keyvalue, masterbfinfo->opb_bfdt);
		if ((comp_value > 0) /* min is greater than max so no rows can
				    ** qualify */
		    ||
		    (	(comp_value == 0)
			&&
			(   (globalbool->opb_xpmin->opb_opno == opgcb->opg_gt)
			    ||
			    (globalbool->opb_xpmax->opb_opno == opgcb->opg_lt)
			)
			&&
			!((tempdt = abs(masterbfinfo->opb_bfdt->db_datatype))
				== DB_INT_TYPE || tempdt == DB_CHA_TYPE ||
				tempdt == DB_VCH_TYPE || tempdt == DB_TXT_TYPE 
				|| tempdt == DB_MNY_TYPE)
		    )
		    )
		 {
		    subquery->ops_sunion.opu_mask |= OPU_NOROWS;
		    subquery->ops_bfs.opb_qfalse = TRUE;
		 }
	    }
	}
    }
}

/*{
** Name: opb_cojoin	- create joinop attributes for corelated variables
**
** Description:
**      This routine will create joinop attributes for corelated columns and
**      add these to the appropriate equivalence classes.  This will ensure
**      that the equivalence classes will be brought to the appropriate
**      position in the CO tree.
**
** Inputs:
**      subquery                        subquery to be analyzed
**      virtual                         range var which may contain correlated
**                                      attributes
**
** Outputs:
**      subquery                        joinop attributes may be added for
**                                      each correlated attribute referenced
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-may-87 (seputis)
**          initial creation
**      24-oct-88 (seputis)
**          modified ope_addeqcls interface to remove nulls on join
**      2-may-89 (seputis)
**          fix bug 5635, caused by opv_eqcmp not initialized
[@history_template@]...
*/
static VOID
opb_cojoin(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          virtual)
{   
    OPV_SUBSELECT	*subp;
    OPV_VARS		*varp;
    OPZ_AT		*abase; /* ptr to base of array of pointers to
				** joinop attributes */

    varp = subquery->ops_vars.opv_base->opv_rt[virtual];
    abase = subquery->ops_attrs.opz_base;
    for (subp = varp->opv_subselect; subp; subp = subp->opv_nsubselect)
    {
	OPV_SEPARM	*parmp;	/* ptr to local corelated descriptor */

	for (parmp = subp->opv_parm;
	    parmp;
	    parmp = parmp->opv_senext)
	{
	    OPZ_IATTS	    attno; /* correlated attribute */
	    OPE_IEQCLS      eqcls; /* equivalence class of the
				** corelated attribute */
	    bool	    add_eqcls; /* TRUE implies we need to
				** create and add a new virtual attribute
				** for the subselect join, since it is used
				** for placement of outer equivalence classes
				** used to evaluate the subselect */
            PST_QNODE       *covarp; /* ptr to var node to correlate */
            if (parmp->opv_sevar->pst_sym.pst_type == PST_VAR)
                covarp = parmp->opv_sevar;
            else if ((parmp->opv_sevar->pst_right->pst_sym.pst_type == PST_VAR)
                &&
                (parmp->opv_sevar->pst_left->pst_sym.pst_type == PST_VAR)
                )
                covarp = parmp->opv_sevar->pst_right;
            else
                opx_error(E_OP039D_OJ_COVAR); /* at this point either the iftrue
                                            ** function or a var node is expected */
            attno = covarp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	    eqcls = subquery->ops_attrs.opz_base->
		opz_attnums[attno]->opz_equcls;
	    add_eqcls = TRUE;
	    if (eqcls >= 0)
	    {	/* check the attr map of the equivalence class
		** to determine if it should be added */
		OPZ_IATTS	    next_attr; /* attribute number of the 
					    ** equivalence class being analyzed
					    */
		OPZ_IATTS	    maxattr; /* maximum joinop attribute index
					    ** assigned in subquery
					    */
		OPZ_BMATTS	    *attrmap; /* map of attributes current in the
					    ** equivalence class */
		attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
		maxattr = subquery->ops_attrs.opz_av;
		for (next_attr = -1; 
		     (next_attr = BTnext((i4)next_attr, (char *)attrmap, maxattr)
		     ) >= 0;)
		{
		    if (abase->opz_attnums[next_attr]->opz_varnm == virtual)
		    {
			add_eqcls = FALSE;  /* do not add eqcls since it is already
					    ** required in the virtual table */
			break;
		    }
		}
	    }
	    if (add_eqcls)
	    {	/* add joinop attribute to equivalence class if not already
                ** represented in the virtual table */
		OPZ_IATTS	    newattr; /* joinop attribute
				    ** assigned to virtual table to
				    ** represent the corelated
				    ** attribute */
		bool		    success;

		newattr = opz_addatts(subquery, virtual, 
		    (OPZ_DMFATTR) OPZ_CORRELATED, 
		    &abase->opz_attnums[attno]->opz_dataval);
		success = ope_addeqcls( subquery, eqcls, newattr, TRUE); /* do
				    ** not remove NULLs for this implicit join */
#ifdef    E_OP0385_EQCLS_MERGE
		if (!success)
		    opx_error(E_OP0385_EQCLS_MERGE);
#endif
	    }
	}
    }
}

/*{
** Name: opb_relabel	- relabel all var nodes to reference new range var
**
** Description:
**      Routine is called when the subselects referenced in the query
**      can be partitioned into two sets of subselects, that can be evaluated
**      at different points in the query.  This routine will place all the
**      subselects represented by the equivalence class map and create a
**      new joinop range table var to represent them, as being executed
**      together but separate from the remaining subselects.  This routine
**      is passed the query tree fragment representing the boolean factor
**      which contains subselect var nodes to be relabelled.
**
** Inputs:
**      subquery                        subquery currently being analyzed
**      qnode                           ptr to node in query tree associated
**                                      will the boolean factor
**      eqcmap                          map of equivalence classes representing
**                                      subselects to be executed together
**      eqcbfmap                        map of equivalence classes required to
**                                      evaluate boolean factors containing
**                                      any subselects which must be evaluated
**                                      together
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Relabels parse tree nodes
**
** History:
**	1-may-86 (seputis)
**          initial creation
**      29-mar-90 (seputis)
**          unix porting change so that non-byte aligned machines work
**	18-mar-91 (seputis)
**	    provide unique global range table label name so that
**	    SEjoin can be distinquished in query plan
**      18-sep-92 (ed)
**          bug 44850 - changed global to local table mapping
[@history_line@]...
*/
static VOID
opb_relabel(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnode,
	OPE_BMEQCLS        *eqcmap,
	OPE_BMEQCLS        *eqcbfmap)
{
    OPZ_ATTS            *attr_ptr;
    if (qnode->pst_sym.pst_type == PST_VAR
	&&
	(attr_ptr = subquery->ops_attrs.opz_base->opz_attnums[
	    qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id])
	&&
	attr_ptr->opz_attnm.db_att_id == OPZ_SNUM
	)
    {	/* relabel all subselect var nodes referenced in eqcmap to be
	** associated with a new range variable */
	if (subquery->ops_joinop.opj_nselect == OPV_NOVAR)
	{   /* first node so create new variable number */
	    OPV_SUBSELECT   **oldlist; /* used to traverse the old list of
				    ** subselect nodes */
	    OPV_SUBSELECT   *newlist; /* list of subselects to be associated
				    ** with the new range variable */
	    OPZ_AT          *abase; /* ptr to base of array of pointers to
				    ** joinop attributes */
	    OPE_BMEQCLS     correlated; /* bit map of the correlated
				    ** equivalence classes associated with
				    ** this variable */
	    OPV_VARS	    *masterp;	/* ptr to list of subselects
				    ** which have not been completely processed
				    */
	    abase = subquery->ops_attrs.opz_base;
	    newlist = NULL;
	    masterp = subquery->ops_vars.opv_base->
		opv_rt[subquery->ops_joinop.opj_select];
	    oldlist = &masterp->opv_subselect;
	    while (*oldlist)
	    {
		if (BTtest( (i4)abase->opz_attnums[(*oldlist)->opv_atno]
			    ->opz_equcls, (char *)eqcmap))
		{   /* if this subselect must be executed with the new
		    ** range variable then extract this node */
		    OPV_SUBSELECT	    *tempptr; /* used to remove link
					    ** from list */
		    tempptr = *oldlist;	/* save link */
		    *oldlist = tempptr->opv_nsubselect; /* remove link */
		    masterp->opv_gvar = masterp->opv_subselect->opv_sgvar;
					    /* in order to keep names for
					    ** SEjoin variable unique, update
					    ** label for table name */
		    masterp->opv_grv = subquery->ops_global->ops_rangetab.opv_base
			->opv_grv[masterp->opv_gvar];
		    tempptr->opv_nsubselect = newlist; /*place link in
					    ** new list */
		    newlist = tempptr;
		    tempptr->opv_eqcrequired = eqcbfmap;
		    BTor(   (i4)BITS_IN(*eqcbfmap),
			    (char *)&tempptr->opv_eqcmp,
			    (char *)eqcbfmap); /* include any corelated
					** equivalence classes in those that
					** need to be available */
		    if (subquery->ops_joinop.opj_nselect == OPV_NOVAR)
		    {   /* create new range variable since it is the
			** first time it is has been referenced */
			(VOID) opv_relatts( subquery, tempptr->opv_sgvar,
			    OPE_NOEQCLS, OPV_NOVAR);
			subquery->ops_vars.opv_prv 
			    = subquery->ops_vars.opv_rv;
                        subquery->ops_joinop.opj_nselect = subquery->ops_global
                            ->ops_rangetab.opv_lrvbase[tempptr->opv_sgvar];

			MEcopy( (PTR)&tempptr->opv_eqcmp,
				sizeof(OPE_BMEQCLS),
				(PTR)&correlated);  /* init the corelated map */
		    }
		    else
			BTor((i4)BITS_IN(correlated),
			    (char *)&tempptr->opv_eqcmp,
			    (char *)&correlated);   /* add to the corelated map
                                                    */
		    continue;
		}
		else
		    oldlist = &(*oldlist)->opv_nsubselect;
	    }
	    /* attach new subselect list to new range variable */
	    subquery->ops_vars.opv_base->
		opv_rt[subquery->ops_joinop.opj_nselect]->opv_subselect
		= newlist;
	    opb_cojoin(subquery, subquery->ops_joinop.opj_nselect); /* assign
						** equivalence classes to
						** correlated attributes */
	}
	/* new range variable has already been created so copy it */
	qnode->pst_sym.pst_value.pst_s_var.pst_vno = 
	attr_ptr->opz_varnm = subquery->ops_joinop.opj_nselect;
    }
    else
    {
	if (qnode->pst_right)
	    opb_relabel(subquery, qnode->pst_right, eqcmap, eqcbfmap);
	if (qnode->pst_left)
	    opb_relabel(subquery, qnode->pst_left, eqcmap, eqcbfmap);
    }
}

/*{
** Name: opb_correlated	- init the eqc maps for correlated vars
**
** Description:
**      This routine will init the eqc maps for the correlated vars
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      virtual                         var number of correlated var
**      subp                            non-null list of subselect descriptors
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
**      21-jul-87 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opb_correlated(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          virtual,
	OPV_SUBSELECT	   *subp)
{

    for (; subp; subp = subp->opv_nsubselect)
    {
	OPV_SEPARM	*parmp;	/* ptr to local corelated descriptor */

	for (parmp = subp->opv_parm;
	    parmp;
	    parmp = parmp->opv_senext)
	{
	    ope_aseqcls(subquery, &subp->opv_eqcmp, parmp->opv_sevar);
	}
	if (virtual != subquery->ops_joinop.opj_select)
	{
	    subp->opv_eqcrequired = &subp->opv_eqcmp; /* if this
			    ** is a corelated aggregate and not a
			    ** subselect then the required
			    ** equivalences are exactly the
			    ** correlated equivalence classes */
	    opb_cojoin(subquery, virtual); /* since subselect nodes
			    ** may have the var nodes
			    ** relabelled, do not assign 
			    ** equivalence classes until range 
			    ** variables are assigned */
	}
    }
}

/*{
** Name: opl_bfeqc	- outer join constant equivalence class check
**
** Description:
**      Check if the equivalence class can be made a constant equivalence
**      class.  This can only be done if all variables in the eqc have
**      the same outer join nesting level.  The interesting case is when
**	the qualification is in the where clause and is an IS NULL on the
**      inner attribute.  The IS NULL cannot be used as a constant attribute
**	on a left key join to the inner relation since the left join must
**	execute first before the IS NULL can be applied.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	eqcls				equivalence class being tested for
**					safety of converting to constant
**
** Outputs:
**
**	Returns:
**	    TRUE - if equivalence class can be made constant
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      3-may-94 (ed)
**          initial creation to fix b59597 - only create constant
**	    eqcls if all variables are in the same scope
[@history_template@]...
*/
static bool
opl_bfeqc(
	    OPS_SUBQUERY    *subquery,
	    OPB_BOOLFACT    *bfp)
{
    OPZ_IATTS	    next_attr;	/* attribute number of the 
				** equivalence class being analyzed
				*/
    OPZ_IATTS	    maxattr;	/* maximum joinop attribute index
				** assigned in subquery
				*/
    OPZ_BMATTS	    *attrmap;	/* map of attributes current in the
				** equivalence class */
    OPZ_AT          *abase;	/* ptr to base of array of pointers to
				** joinop attributes */
    OPL_BMOJ	    *ojinner;	/* map of outer joins for which this
				** variable is an inner */
    OPV_IVARS	    maxprimary; /* number of explicitly referenced
				** relations in subquery */
    bool	    primary_found; /* TRUE when the first variable
				** in the eqcls attrmap list is analyzed */
    OPV_RT	    *vbase;
    bool	    isnull;	/* special case IS NULL test for
				** constant requires boolfact outer
				** join ID to be outer to variable */
    OPL_OJT	    *lbase;
    OPL_BMOJ	    *ojmap;	/* for IS NULL case, map of outer joins
				** defined in scope of boolean factor
				** on clause containing IS NULL test */
    OPL_BMOJ	    where_scope; /* used for ojmap if boolean factor is
				** IS NULL is in the where clause */
    OPL_IOUTER	    maxoj;	/* number of outer joins defined */
    OPL_IOUTER	    bfojid;	/* ojid of boolean factor representing
				** constant qualification */
    bfojid = bfp->opb_ojid;
    abase = subquery->ops_attrs.opz_base;
    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist
	[bfp->opb_eqcls]->ope_attrmap;
    maxattr = subquery->ops_attrs.opz_av;
    ojinner = NULL;
    maxprimary = subquery->ops_vars.opv_prv;
    primary_found = FALSE;
    vbase = subquery->ops_vars.opv_base;
    isnull =(bfp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_opno 
	== subquery->ops_global->ops_cb->ops_server->opg_isnull);
    lbase = subquery->ops_oj.opl_base;
    maxoj = subquery->ops_oj.opl_lv;
    if ((bfojid >= 0)
	&&
	(lbase->opl_ojt[bfojid]->opl_type == OPL_FULLJOIN))
	return(FALSE);	/* full join prevents
			** constant equivalence class */
    if (isnull)
    {	/* special case handling for IS NULL test, find the map of
	** outer join IDs which are defined in the scope of the outerjoin
	** ID which contained the IS NULL qualification */
	if (bfojid >= 0)
	{
	    ojmap = lbase->opl_ojt[bfojid]->opl_idmap;
	}
	else
	{
	    MEfill(sizeof(where_scope), (u_char)0, (PTR)&where_scope);
	    BTnot((i4)BITS_IN(where_scope), (char *)&where_scope);
	    ojmap = &where_scope;
	}
    }
    for (next_attr = -1; 
	 (next_attr = BTnext((i4)next_attr, (char *)attrmap, maxattr)
	 ) >= 0;)
    {
	OPV_IVARS	varno;
	varno = abase->opz_attnums[next_attr]->opz_varnm;
	if (varno < maxprimary)
	{
	    if (!primary_found)
	    {
		primary_found = TRUE;
		ojinner = vbase->opv_rt[varno]->opv_ijmap;
	    }
	    else
	    {
		OPL_BMOJ	*newinner;
		newinner = vbase->opv_rt[varno]->opv_ijmap;
		if (newinner == ojinner)
		    continue;	/* if both NULL then OK */
		if (!newinner   /* if either NULL then a mismatch
				** and constant eqcls not
				** guaranteed */
		    || 
		    !ojinner
		    ||
		    MEcmp((PTR)ojinner, (PTR)newinner, sizeof(ojinner))
		    )
		    return(FALSE);
	    }
	    if (isnull)
	    {
		OPL_IOUTER	ojid;
		for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)ojmap, 
		    (i4)maxoj))>=0;)
		{
		    OPL_OUTER	    *ojp;
		    ojp = lbase->opl_ojt[ojid];
		    if ((   (ojp->opl_type == OPL_FULLJOIN)
			    ||
			    (ojp->opl_type == OPL_LEFTJOIN)
			)
			&&
			BTtest((i4)varno, (char *)ojp->opl_ivmap))
			return(FALSE);	/* IS NULL test cannot be
				    ** pushed into nested outer join
				    ** as in the case of a key join */
		}
	    }
	}
    }
    return(TRUE);
}

/*{
** Name: opb_bfsinit	- initialize the boolean factors structure
**
** Description:
**      This routine will traverse the qualification list associated with
**      the subquery and analyzed each conjunct.  A change from the 4.0
**      version of clauseset is that only one traversal of the query tree
**      is made.
**
**      Evaluates constant clauses in the qual and if any of them are false 
**      then set opb_qfalse.  Handles type conversions between constants to
**	be used as keys and the atts that they key.  Handles pattern matching 
**      chars and abstract datatypes (which may also have pattern matching 
**      chars).
**
**      If the conjunct is a boolean factor with only constant nodes then 
**      an attempt is made to evaluate this conjunct.  This may not be possible
**      since some of the constant nodes may only be available at runtime.  If
**      runtime constants are required then the qualification is saved in a
**      list of qualifications which are to be executed at runtime prior to
**      execution of the query.
** 
**      If the boolean factor contains references to variables then lists of
**      (key, histogram) values are built for any boolean factors which contain
**      limiting predicates; i.e. a qualification of the form
**                    (r.a1>=c1) OR (s.b1>=c2) OR... OR <other stuff>
**      will have a list of values built which represent (r.a1>=c1)
**      and another list of values built which represent (s.b1>=c2) etc.
**        
**      If the boolean factor contains only limiting predicates then it 
**      can be used for keyed access and it is marked as such.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**          .ops_root->pst_right        qualification of subquery which will
**                                      be analyzed
**
** Outputs:
**      subquery->ops_bfs               boolean factor structure initialized
**                                      with (key, histogram) lists for all
**                                      limiting predicates in qualification
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-may-86 (seputis)
**          initial creation
**	1-may-89 (seputis)
**          subselects not calculated together after normalization
**      29-mar-90 (seputis)
**          unix porting change so that non-byte aligned machines work
**      14-apr-92 (seputis)
**          fix b43405 - only mark equivalence class as a constant if
**          no OR conjuncts exist
**	21-dec-92 (ed)
**	    fix outer join problem, constant on clause qualifications
**	    cannot be treated as constant where clause qualifications
**      5-jan-93 (ed)
**          fix b48049 - constant equivalence handling in joins
**	11-feb-94 (ed)
**	    bug 59593 - mark subselect variables in outer join scope
**	    were appropriate.
**	19-may-97 (inkdo01)
**	    Added complex parm to opb_bfinit call to support non_CNF 
**	    where clause analysis.
**	11-jul-97 (inkdo01)
**	    Fix to make constant ON factors compile into body of plan, rather
**	    than ahd_constant. Otherwise, ON gets treated like WHERE.
**	1-aug-97 (inkdo01)
**	    Fine tune previous fix for case in which views (with 
**	    restrictions) are outer joined with constant ON clauses.
[@history_line@]...
*/
static
opb_bfsinit(
	OPS_SUBQUERY       *subquery)
{
    PST_QNODE           *and_node;  /* ptr to AND node of current
				    ** qualification being analyzed
				    */
    PST_QNODE           *previous;  /* ptr to previous AND node used
				    ** used to remove conjuncts
				    ** from the list
				    */

    OPB_BOOLFACT        *bfp;	    /* ptr to boolean factor element
				    ** being initialized
				    ** - NULL indicates that the boolean
				    ** factor element needs to be
				    ** allocated
				    */
    OPE_ET              *ebase;     /* ptr to base of array of ptrs to
                                    ** equilvalence class elements */
    OPE_IEQCLS		maxeqcls;   /* max number of equivalence classes */

    maxeqcls = subquery->ops_eclass.ope_ev;
    ebase = subquery->ops_eclass.ope_base;
    bfp = NULL;			    /*boolean factor element not allocated*/
    previous = subquery->ops_root;
    for( and_node = previous->pst_right;
	and_node->pst_sym.pst_type != PST_QLEND;
	and_node = (previous = and_node)->pst_right) 
    {
	bool               sargable; /* TRUE - all predicates in the boolean
                                    ** factor are of the form (A relop const)
                                    ** where relop is >,<,=,>=,<=
                                    */
	bool		   complex = FALSE; /* TRUE - this BF has an AND
				    ** nested under an OR */
	bfp = opb_bfget(subquery, bfp); /* get a new boolean factor element if
                                    ** bfp is NULL, otherwise initialize the
                                    ** existing structure.  This structure will
                                    ** be at the head of the list of all
                                    ** limiting predicate (key, histogram) info
                                    ** associated with the boolean factor
                                    */
	bfp->opb_qnode = and_node->pst_left;    /* save query tree node */

	subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
	sargable = opb_bfinit( subquery, bfp, and_node->pst_left, &complex);
				    /* traverse the query
				    ** tree fragment associated with the
				    ** boolean factor and setup histogram
				    ** information, keyed access information
				    ** and initialize the equivalence
				    ** class map for the boolean factor
				    */
	if (subquery->ops_bfs.opb_ncheck)
	{   /* bit map of equivalence classes which cannot return nulls in the
	    ** final result has been calculated so reset the null-join flag
	    ** of the equivalence class accordingly */
	    OPE_BMEQCLS		*bmnullp;
	    OPE_IEQCLS		eqcls;

	    if (bfp->opb_mask & OPB_ORFOUND)
		bmnullp = subquery->ops_bfs.opb_njoins; /* this is the set
				    ** of equivalence classes which were
				    ** referenced in every predicate of this
				    ** boolean factor */
	    else
		bmnullp = &bfp->opb_eqcmap; /* if there are not any OR's 
				    ** then all the tuples which reference
				    ** equivalences classes which contain 
				    ** NULLs will be eliminated
				    ** by this conjunct */
	    for( eqcls= -1; 
		(eqcls = BTnext( (i4)eqcls, (char *)bmnullp, (i4)maxeqcls)) >= 0;)
		ebase->ope_eqclist[eqcls]->ope_nulljoin = FALSE;
	}
        if (bfp->opb_truequal)
	    continue;               /* if the boolean factor is always true
                                    ** then skip it - FIXME - this is not
                                    ** implemented yet - will always be false
                                    */
	if (bfp->opb_eqcls == OPB_BFNONE && (!subquery->ops_oj.opl_base ||
	    and_node->pst_sym.pst_value.pst_s_op.pst_joinid == OPL_NOOUTER &&
	    (and_node->pst_left->pst_sym.pst_type != PST_BOP ||
	    and_node->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid 
							== OPL_NOOUTER)) &&
	    !(bfp->opb_mask & OPB_NOTCONST))
	{
	    /* The boolean factor does not reference any attributes so 
	    ** only constant nodes are contained in the boolean factor.
	    ** Attempt to evaluate the constant expressions.  If there
	    ** are any parameter constants then this expression must be
	    ** evaluated at runtime.  Also, certain constant expressions
	    ** are time dependent (such as "now" date) so check for these
	    ** and evaluate expression at runtime if found.
	    **
	    ** Constants found in ON clauses are left there, to avoid
	    ** upsetting outer join semantics.
	    */
	    bool		value;	/* TRUE if constant expression will
				    ** always evaluate to TRUE
				    ** FALSE if constant expression will
				    ** always evaluate to false
				    */

	    if (opb_cbf(subquery, bfp, &value))
	    {   /* constant expression has been evaluated successfully 
                ** AND does not contain parameter nodes or "run time"
                ** constants such as "now"
                ** FIXME - opb_cbf always returns FALSE for now
                */
		if (value)
		    /* the query will never return any tuples if the
		    ** expression does not contain any parameter nodes
		    */
		{
		    subquery->ops_bfs.opb_qfalse = TRUE;
		    return;
		}
		else
		    /* ignore this boolean factor since it is always true
		    ** or contains parameter nodes which must be evaluated
		    ** at execution time
		    */
		    continue;		    /* reuse the allocated boolean
                                            ** factor element */
	    }
	    else
	    {   /* expression must be evaluated at runtime so place
		** qualification in the opb_bfconstants list to be processed 
                ** at runtime prior to starting the query
		*/
		previous->pst_right = and_node->pst_right;
		and_node->pst_right = subquery->ops_bfs.opb_bfconstants;
		subquery->ops_bfs.opb_bfconstants = and_node;
		and_node = previous;
		continue;		    /* reuse the allocated boolean
                                            ** factor element */
	    }
	}
        else if (!sargable || bfp->opb_eqcls == OPB_BFMULTI)
	{   /* check for sargability of the boolean factor
            ** - sargable if there is only one equivalence class in the
            ** boolean factor and all predicates are of the form(A relpop const)
            ** - if "sargable" is false then a predicate other than 
            ** (A relop const) was found
            ** - if bfp->opb_eqcls is OPB_BFMULTI then more than one equivalence
            ** class is in the boolean factor
            */
	    /* traverse the bool info structures and update all bool info 
            ** structs appropriately
            */
	    OPB_BFKEYINFO      *keyhdr;		/* ptr to current key header
                                                ** being analyzed */
            for ( keyhdr = bfp->opb_keys; keyhdr; keyhdr = keyhdr->opb_next)
		keyhdr->opb_sargtype = ADC_KNOKEY;
	}
	else if ((bfp->opb_eqcls >= 0) 
	    &&
            bfp->opb_keys)
	{
            /* for sargable predicates the min max for the equivalence class
            ** needs to be updated, as well as the "EQ" operators need to be
            ** sorted */
	    opb_rtminmax(subquery, bfp, ebase->ope_eqclist[bfp->opb_eqcls],
		(OPB_IBF)(subquery->ops_bfs.opb_bv - 1)); /* update the min 
                                            ** max values for the range type 
                                            ** boolean factors */
           if ((bfp->opb_keys->opb_sargtype == ADC_KEXACTKEY)
                &&
                (!bfp->opb_keys->opb_keylist->opb_next)
                &&
                !bfp->opb_orcount
		&&
		(   !subquery->ops_oj.opl_base
		    ||
		    opl_bfeqc(subquery, bfp)
		))			    /* outer join qualifications
					    ** cannot guarantee constant values */
	    {   /* find the boolean factors that can provide values for
		** join nodes to do lookups ... at this point this boolean
		** factor is an equality sarg and it has only one possible
		** value. Therefore, this boolean factor can be used as a
		** join key.
		** - replaces bfkeyeqc.c routine
		** - FIXME - it is not necessarily - next opb_keys may
		** have non-equality operators due to truncation of pattern
		** matching characters !! (rare case)
		*/
		if (!subquery->ops_bfs.opb_bfeqc)
		{   /* allocate memory for bitmap */
		    subquery->ops_bfs.opb_bfeqc = (OPE_BMEQCLS *)opu_memory(
			subquery->ops_global, (i4)(2*sizeof(OPE_BMEQCLS)));
		    MEfill(2*sizeof(OPE_BMEQCLS), (u_char)0, 
			(PTR)subquery->ops_bfs.opb_bfeqc);
		    subquery->ops_bfs.opb_mbfeqc = &subquery->ops_bfs.opb_bfeqc[1];
		    BTnot((i4)BITS_IN(*subquery->ops_bfs.opb_mbfeqc),
			(char *)subquery->ops_bfs.opb_mbfeqc);
		}
		BTset((i4)bfp->opb_eqcls, (char *)subquery->ops_bfs.opb_bfeqc);
		BTclear((i4)bfp->opb_eqcls, (char *)subquery->ops_bfs.opb_mbfeqc);
    
		subquery->ops_eclass.ope_base->ope_eqclist[bfp->opb_eqcls]
		    ->ope_bfindex = subquery->ops_bfs.opb_bv - 1; /* save the 
					** back reference to the 
					** boolean factor which provides the 
					** constant value for the equivalence 
                                        ** class
					*/
	    }
	}
	bfp = NULL;		    /* boolean factor element has
				    ** been used so initialize the variable
				    ** so a new element will be allocated
				    ** on the next pass
				    */
    }
    /* if the end of the for loop is reached then check if the last boolean
    ** factor element was used and deallocate it if necessary
    */
    if (bfp)
	subquery->ops_bfs.opb_bv--;

    if (subquery->ops_bfs.opb_bv
	&&
	subquery->ops_joinop.opj_select != OPV_NOVAR)
    {   /* use boolean factors equivalence class maps to determine which
	** subselects must be calculated together... 
	** - if two subselects are in the same boolean factor then the
	** the attributes must be in same range variable so that
	** boolean factor can be evaluated at the parent of the subselect
	** SEJOIN node
	** - each subselect attribute is in an equivalence class on its own
	** - it is desirable to calculate as many subselects as possible
	** together since the enumeration phase will complete much more
	** quickly if the number of leaf ORIG nodes is smaller
	*/
	OPB_IBF		bfi;		/* index representing a boolean
					** factor */
	OPE_BMEQCLS     unresolved;     /* bit map of subselect equivalence
					** classes which have not been resolved
					*/
	MEcopy( (PTR)subquery->ops_eclass.ope_subselect,
		sizeof(OPE_BMEQCLS),
		(PTR)&unresolved);	    /* start with all subselect equivalence
					** classes and remove them as new
					** range variable nodes get created */
	for (bfi = 0; bfi < subquery->ops_bfs.opb_bv; bfi++
	    )
	{
	    OPE_BMEQCLS	    sub_eqcmap;	/* map of subselect equivalence classes
					** which must be executed together
					** because they are connected by
					** boolean factors */
	    OPB_BOOLFACT    *sbfp;	/* ptr to current boolean factor
					** descriptor being analyzed */
	    OPB_IBF	    bfi2;	/* index representing a boolean
					** factor which contains a subselect
					*/
	    OPE_BMEQCLS     (*alleqcmap); /* bit map of all the equivalence
					** classes required to evaluate all
					** boolean factors which contain the
					** subselects at this range variable */

	    alleqcmap = (OPE_BMEQCLS *)NULL;
	    sbfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi];
	    if (!sbfp->opb_virtual
		||
		!sbfp->opb_virtual->opb_subselect)
		continue;		    /* ignore boolean factors which do
					** not contain subselects */
	    MEcopy( (PTR)&sbfp->opb_eqcmap, sizeof(OPE_BMEQCLS),
		(PTR)&sub_eqcmap);
	    BTand(  (i4)BITS_IN(sub_eqcmap), 
		    (char *) &unresolved,
		    (char *) &sub_eqcmap); /* get map of subselect
					** equivalence classes which
					** still have not been bound to
					** a variable */
	    if (BThigh((char *)&sub_eqcmap, (i4)BITS_IN(sub_eqcmap)) < 0)
		continue;		/* continue if all subselects have
					** been resolved
					** in this boolean factor */
	    alleqcmap = (OPE_BMEQCLS *)opu_memory(subquery->ops_global,
		(i4)sizeof(OPE_BMEQCLS));
	    MEcopy( (PTR)&sbfp->opb_eqcmap, sizeof(OPE_BMEQCLS),
		(PTR)alleqcmap);	/* initialize to equivalence classes
					** from first subselect for range
					** variable */
	    {
		bool		    tryagain; /* set FALSE if no changes made
					** to sub_eqcmap on an entire pass
					** of the boolean factor array */
		tryagain = TRUE;
		while (tryagain)
		{
		    tryagain = FALSE;
		    for (bfi2 = bfi+1; bfi2 < subquery->ops_bfs.opb_bv; bfi2++)
		    {	/* search remainder of boolean factors to determine if any
			** subselects which are to be executed together, force
			** other subselects to be executed together */
			OPE_BMEQCLS     temp_map;/* map of equivalence classes in the
						** boolean factor which represent
						** subselects */
			OPB_BOOLFACT	*sbfp2;	/* ptr to current boolean factor
						** descriptor being analyzed */
			sbfp2 = subquery->ops_bfs.opb_base->opb_boolfact[bfi2];
			if (!sbfp2->opb_virtual
			    ||
			    !sbfp2->opb_virtual->opb_subselect)
			    continue;	    /* ignore boolean factors which do
						** not contain subselects */
			MEcopy( (PTR)&sbfp2->opb_eqcmap, 
				sizeof(OPE_BMEQCLS),
				(PTR)&temp_map);
			BTand(	(i4)BITS_IN(temp_map), 
				(char *) &sub_eqcmap,
				(char *) &temp_map); /* get map of subselect
						** equivalence classes which
						** are in common */
			if (BThigh((char *)&temp_map, (i4)BITS_IN(temp_map)) < 0)
			    continue;		/* continue if no equivalence
						** classes are in common */

			BTor(   (i4)BITS_IN(*alleqcmap),
				(char *)&sbfp2->opb_eqcmap,
				(char *)alleqcmap); /* update the
					    ** set of boolean factor equivalence
					    ** classes */
			MEcopy( (PTR)&sbfp2->opb_eqcmap, 
				sizeof(OPE_BMEQCLS),
				(PTR)&temp_map);
			BTand(	(i4)BITS_IN(temp_map), 
				(char *) &unresolved,
				(char *) &temp_map); /* get map of subselect
						** equivalence classes which
						** are in the new subselect */
			if (!BTsubset(	(char *)&temp_map,
					(char *)&sub_eqcmap,
					(i4)BITS_IN(sub_eqcmap))
			    )
			{
			    if (sbfp->opb_ojid != sbfp2->opb_ojid)
				opx_error(E_OP03A7_OJ_SUBSELECT); /* 
					    ** expecting subselects to
					    ** be outer join compatible */
			    tryagain = TRUE;/* a new subselect has been
					    ** added so the list of boolean
					    ** variables needs to be
					    ** traversed once more */
			    BTor(   (i4)BITS_IN(sub_eqcmap),
				    (char *)&temp_map,
				    (char *)&sub_eqcmap); /* update the
					    ** set of subselect equivalence
					    ** classes */
			}
		    }
		}
	    }
	    if (!MEcmp(	(PTR)&unresolved, 
			(PTR)&sub_eqcmap,
			sizeof(OPE_BMEQCLS))
		)
	    {   /* use the current range variable for the subselect since
		** no more unresolved equivalence classes exist */
		OPV_SUBSELECT	    *subp; /* used to traverse the
					   ** subselect descriptors */
		for (subp = subquery->ops_vars.opv_base->
			opv_rt[subquery->ops_joinop.opj_select]->opv_subselect;
		    subp;
		    subp = subp->opv_nsubselect)
		{
		    subp->opv_eqcrequired = alleqcmap; /* place the bit map 
					** of the equivalence classes of all
					** boolean factors to be evaluated
					** in the subselect descriptors */
		    BTor(   (i4)BITS_IN(*alleqcmap),
			    (char *)&subp->opv_eqcmp,
			    (char *)alleqcmap); /* include any corelated
					** equivalence classes in those that
					** need to be available */
		}
		opb_cojoin(subquery, subquery->ops_joinop.opj_select); /*
					** add joinop attributes for any
					** correlated columns */
		if (subquery->ops_oj.opl_base)
		    opl_subselect(subquery, subquery->ops_joinop.opj_select,
			sbfp->opb_ojid); /* mark this subselect variable as
					** being an inner to the outer join
					** variable */
		break;			/* since the unresolved equivalence
					** classes match those which must be
					** executed together no further
					** changes need to be made */
	    }
	    subquery->ops_joinop.opj_nselect = OPV_NOVAR; /* reset
					** new var until first time it 
					** is needed */
	    for (bfi2 = bfi; bfi2 < subquery->ops_bfs.opb_bv; bfi2++)
	    {   /* relabel query tree node with new range variable number */
		OPB_BOOLFACT	*sbfp3;	/* ptr to current boolean factor
					** descriptor being analyzed */
		sbfp3 = subquery->ops_bfs.opb_base->opb_boolfact[bfi2];
		if (!sbfp3->opb_virtual
		    ||
		    !sbfp3->opb_virtual->opb_subselect)
		    continue;	    /* ignore boolean factors which do
					** not contain subselects */
		if (BTsubset((char *)&sbfp3->opb_eqcmap, (char *)alleqcmap,
		    (i4)BITS_IN(*alleqcmap)))
		{
		    opb_relabel(subquery, sbfp3->opb_qnode, &sub_eqcmap,
			alleqcmap);	    /* found boolean
					** factor containing appropriate
					** subselects so relabel the query
					** tree node */
		}
		    
	    }
	    if (subquery->ops_oj.opl_base)
		opl_subselect(subquery, subquery->ops_joinop.opj_nselect,
		    sbfp->opb_ojid);	/* mark this subselect variable as
					** being an inner to the outer join
					** variable */
	    BTnot((i4)BITS_IN(OPE_BMEQCLS), (char *)&sub_eqcmap);
	    BTand((i4)BITS_IN(OPE_BMEQCLS), (char *)&sub_eqcmap,
		(char *)&unresolved);	/* reset all equivalence classes
					** which have been resolved */
	}	
    }
}

/*{
** Name: opb_create	- create the boolean factors table and init opb_bfs
**
** Description:
**	This routine will initialize the boolean factors structure and must
**	be called prior to any reference to this table.
**
**      This routine will build the keys and the histogram values for all
**      equivalence classes, type and lengths ... this should be delayed
**      until histogram processing is needed so one variable queries will
**      be optimized quickly. FIXME
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**
** Outputs:
**      subquery->opb_base              ptr to ptr of array of boolean factor 
**                                      elements allocated
**      subquery->opb_bv                number of boolean factor elements
**                                      defined
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
opb_create(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;

    MEfill( sizeof(OPB_BFSTATE), (u_char)0, (PTR)&subquery->ops_bfs);
					    /* initialize state variable for
                                            ** boolean factors */

    global = subquery->ops_global;
    if (global->ops_mstate.ops_tbft)
    {
	subquery->ops_bfs.opb_base = global->ops_mstate.ops_tbft; /* use full size 
                                            ** OPZ_BFT structure if available */
    }
    else
    {
	global->ops_mstate.ops_tbft =
	subquery->ops_bfs.opb_base = (OPB_BFT *) opu_memory( 
	    subquery->ops_global, (i4) sizeof(OPB_BFT) ); /* get memory for 
					    ** array of pointers to boolean
					    ** factors
					    */
    }
    if (subquery->ops_joinop.opj_virtual)
    {	/* are there any nested subqueries which require correlated attributes
        ** to be available?  If so then calculate the equivalence classes that
        ** must be available.
        */
	{   /* initialize the opv_eqcmp maps of the subselect descriptors */
	    OPV_IVARS           virtual; /* range variable index used to
                                        ** traverse tables in query */

	    for (virtual = 0; virtual < subquery->ops_vars.opv_rv; virtual++)
	    {
		OPV_SUBSELECT       *subp; /* current subselect being analyzed*/

		if (subp = subquery->ops_vars.opv_base->opv_rt[virtual]->
			opv_subselect)
		    opb_correlated(subquery, virtual, subp); /* initialize
					    ** the correlated eqc maps */
	    }	    
	}
    }
    opb_bfsinit(subquery);                  /* read qualification and create
                                            ** boolean factor elements
                                            */
}

/*{
** Name: opb_mboolfact	- make a boolean factor from 2 expressions
**
** Description:
**      Given 2 expressions, this routine will place an equality node above it
**	and create a boolean factor entry.  Needed to make sure function attributes
**	which do not participate in a join are evaluated as a boolean factor
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      expr1                           first expression of join
**      expr2                           second expression of join
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    entry added to boolean factor array
**
** History:
**	5-sep-90 (seputis)
**	    - fix b32560 - recognize when a function attribute needs to be
**	    evaluated as a boolean factor
**	5-aug-93 (ed)
**	    - allocate an extra location for the boolean factor
**	19-may-97 (inkdo01)
**	    Added complex parm to opb_bfinit call to support non_CNF 
**	    where clause analysis.
[@history_template@]...
*/
VOID
opb_mboolfact(
	OPS_SUBQUERY       *subquery,
	PST_QNODE	   *expr1,
	PST_QNODE          *expr2,
	PST_J_ID           joinid)
{
    OPS_STATE	    *global;
    PST_QNODE	    *and_node;
    PST_QNODE	    *eq_node;
    OPB_BOOLFACT    *bfp;
    OPB_BFT	    *bft;
    i4		    size;
    bool	    complex = FALSE;

    global = subquery->ops_global;
    /* create an equality conjunct representing the boolean factor */
    eq_node = opv_opnode(global, PST_BOP, (ADI_OP_ID)global->ops_cb->ops_server->opg_eq,
        (OPL_IOUTER)joinid);
    eq_node->pst_right = expr1;
    eq_node->pst_left = expr2;
    opa_resolve(subquery, eq_node);

    /* create a conjunct and link into qualification list */
    and_node = opv_opnode(global, PST_AND, (ADI_OP_ID)global->ops_cb->ops_server->opg_eq,
        (OPL_IOUTER)joinid);
    and_node->pst_right = subquery->ops_root->pst_right;
    subquery->ops_root->pst_right = and_node;
    and_node->pst_left = eq_node;
    size = sizeof(bft->opb_boolfact[0]) * (subquery->ops_bfs.opb_bv+1);
    bft = (OPB_BFT *) opu_memory(global, size);
    MEcopy ((PTR)subquery->ops_bfs.opb_base, (sizeof(bft->opb_boolfact[0])
	* subquery->ops_bfs.opb_bv), (PTR)bft);
    subquery->ops_bfs.opb_base = bft;
    bfp = opb_bfget(subquery, (OPB_BOOLFACT *)NULL); /* get a new boolean factor element if
				** bfp is NULL, otherwise initialize the
				** existing structure.  This structure will
				** be at the head of the list of all
				** limiting predicate (key, histogram) info
				** associated with the boolean factor
				*/
    bfp->opb_qnode = and_node->pst_left;    /* save query tree node */

    subquery->ops_oj.opl_smask &= (~OPL_ISNULL);
    (VOID) opb_bfinit( subquery, bfp, and_node->pst_left, &complex);/* 
				** traverse the query
				** tree fragment associated with the
				** boolean factor and setup histogram
				** information, keyed access information
				** and initialize the equivalence
				** class map for the boolean factor
				*/
}
