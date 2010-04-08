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
#define             OPLOJOIN         TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>
#include    <st.h>

/**
**
**  Name: OPLOJOIN.C - initialization routines for outer join support
**
**  Description:
**      Routines used to process outer join structures 
**
{@func_list@}...
**
**
**  History:
**      14-sep-89 (seputis)
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	19-jan-94 (ed)
**	    Correct histogram processing problem with estimates of NULLs
**	    after an outer join
**	    - remove obsolete structures
**	11-feb-94 (ed)
**	    - add comments, remove obsolete opl_hadjust routine
**	    - fix problems found by histogram walkthru
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
**  
**      16-nov-94 (inkdo01)
**          added support for debug trace call to dump oj table info
**	6-june-97 (inkdo01 for hayke02)
**	    In the function opl_target(), we now do not call opl_findresdom()
**	    before calling opl_nulltype(). Rather, we now do the same checking
**	    that is in opl_findresdom(), but with a different error condition.
**	    This change fixes bug 79515.
**	6-june-97 (inkdo01 for hayke02)
**	    In the function opl_target(), a check for the existence of
**	    opv_gquery has been AND'ed to the check for a false found_resdom.
**	    This is a fix up to the last change (fix for bug 79515). This
**	    change fixes bug 82532.
**	6-june-97 (inkdo01 for hayke02)
**	    In the function opl_histogram(), we now only add in the tuples
**	    contributed by NULLs if this is a full outer join. This change
**	    fixes bug 82781.
**      05-may-99 (sarjo01)
**          Bug 96813: in opl_target() test for subquery node type of
**          PSQ_DGTT_AS_SELECT (declare global temporary table as select)
**          which may also be an outer join, to see if inner target resdom
**          need to be made nullable.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	3-Jun-2009 (kschendel) b122118
**	    Minor cleanup: delete dead ojfilter stuff, use BTclearmask.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of st.h to eliminate gcc 4.3 warnings.
[@history_template@]...
**/

/*{
** Name: opl_resolve	- resolve parse tree for IFTRUE function
**
** Description:
**      The routine will traverse a parse tree and re-resolve the 
**      parse tree because the "IFTRUE" function was inserted 
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      qnode                           ptr to parse tree node to
**					resolve
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
**      24-nov-89 (seputis)
**          initial creation
**      19-apr-93 (ed)
**          make sure children are resolved prior to parents
**	21-may-93 (ed)
**	    make routine public instead of static
[@history_template@]...
*/
VOID
opl_resolve(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnode)
{
    if (qnode->pst_left)
	opl_resolve(subquery, qnode->pst_left);
    if (qnode->pst_right)
	opl_resolve(subquery, qnode->pst_right);
    switch (qnode->pst_sym.pst_type)
    {
    case PST_AOP:
    case PST_BOP:
    case PST_UOP:
    case PST_COP:
	opa_resolve(subquery, qnode);
    default:
	break;
    }
}

/*{
** Name: opl_iftrue	- insert an iftrue function at this point
**
** Description:
**      In an outer join case, in which the inner join fails, the
**	attribute is undefined, so the iftrue function will be used
**	to return a NULL in this case
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      rootpp                          ptr to ptr of var node to be made
**					a parameter to the iftrue
**	ojid				ID to place in op node
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
**      22-nov-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opl_iftrue(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **rootpp,
	OPL_IOUTER	   ojid)
{
    PST_QNODE	    *iftruep;
    OPS_STATE	    *global;
    DB_ATT_ID	    attr;
    OPZ_ATTS	    *attrp;
    OPV_IVARS	    varno;

    global = subquery->ops_global;
    iftruep = opv_opnode(global, PST_BOP, 
	global->ops_cb->ops_server->opg_iftrue, ojid);
    iftruep->pst_right = *rootpp;
    varno = (*rootpp)->pst_sym.pst_value.pst_s_var.pst_vno;
    attr.db_att_id = subquery->ops_vars.opv_base->opv_rt
	[varno]->opv_ojattr;
    attrp = subquery->ops_attrs.opz_base->opz_attnums[attr.db_att_id];
    
    iftruep->pst_left = opv_varnode( global, &attrp->opz_dataval, 
		    (OPV_IGVARS)varno, 
		    &attr);
    opa_resolve( subquery, iftruep);
    *rootpp = iftruep;

}

/*{
** Name: opl_findresdom	- find resdom in subquery
**
** Description:
**      traverse the target list and find the resdom corresponding to
**	the attribute number 
**
** Inputs:
**      subquery                        subquery to be searched
**      rsno                            resdom number to be searched
**
** Outputs:
**
**	Returns:
**	    PTR to resdom corresponding to resdom number
**	Exceptions:
**	    aborts query if resdom cannot be found
**
** Side Effects:
**	    none
**
** History:
**      26-mar-94 (ed)
**          initial creation
[@history_template@]...
*/
PST_QNODE *
opl_findresdom(
    OPS_SUBQUERY	*subquery, 
    OPZ_IATTS		rsno)
{
    PST_QNODE	    *resdomp;

    for (resdomp = subquery->ops_root->pst_left;
	resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
	resdomp = resdomp->pst_left)
    {
	if (resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno
	    == 
	    rsno)
	    return(resdomp);
    }
    opx_error(E_OP068F_MISSING_RESDOM);
}

/*{
** Name: opl_nulltype	- convert parse node to nullable
**
** Description:
**      convert the parse tree node to nullable 
**
** Inputs:
**      root                            parse tree node to convert
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
**      26-MAR-94 (ed)
**          initial creation
[@history_template@]...
*/
VOID
opl_nulltype(
	PST_QNODE	*qnodep)
{
    if (qnodep->pst_sym.pst_dataval.db_datatype > 0)
    {
	qnodep->pst_sym.pst_dataval.db_datatype = 
	    -qnodep->pst_sym.pst_dataval.db_datatype;
	qnodep->pst_sym.pst_dataval.db_length += 1;
    }
}

/*{
** Name: opl_target	- process target list for outer joins
**
** Description:
**      This routine will process the target list for outer joins 
**      by assigning equivalence classes and inserting IFTRUE 
**      whenever necessary 
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      qnode                           ptr to parse tree node
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
**      24-nov-89 (seputis)
**          initial creation
**	23-aug-93 (ed)
**	    copy result type for non-printing resdoms in outer joins
**	7-dec-93 (ed)
**	    equivalence classes have all been assigned at this point
**      14-mar-94 (ed)
**          - bug 59355 correct temporary table union view problem 
**	    in which datatypes of resdoms can change due to outer joins
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
**	05-oct-94 (dick)
**	    bug 64116 - BOP not made nullable if IFTRUE inserted in its subtree
**	6-june-97 (inkdo01 for hayke02)
**	    We now do not call opl_findresdom() before calling opl_nulltype().
**	    Rather, we now do the same checking that is in opl_findresdom(),
**	    but with a different error condition (the check for a 'found'
**	    resdom is now after the traversal of all subqueries in a union).
**	    This change fixes bug 79515.
**	6-june-97 (inkdo01 for hayke02)
**	    A check for the existence of opv_gquery has been AND'ed to the
**	    check for a false found_resdom. This is a fix up to the last change
**	    (fix for bug 79515) and allows a 'create table ... as select ...
**	    [left, right, full] join ...' style query to pass. This query has
**	    ops_mode PSQ_RETINTO with ops_gentry >= 0, but has a NULL
**	    opv_gquery (which was causing E_OP068F). This change fixes bug
**	    82532.
**      29-jun-98 (sarjo01) from 15-mar-98 (hayke02) change 435297
**          Modify fix for bug 79515 - we now don't break out of the union
**          subquery 'for' loop if we have found a matching resdom. This
**          prevents unioned selects containing literals returning malformed
**          values. This change fixes bug 85225.
**	 2-mar-04 (hayke02)
**	    Modify the code that sets the nullability of union partitions, when
**	    we have a OPS_RFAGG subselect. This means we have a group'ed by
**	    aggregate outer join partition, and the 'target'/resdom list is the
**	    aggregate and group by resdoms, rather than the select list
**	    resdoms. We cannot guarentee that the by list will be in the same
**	    order as the select list, so we will now set all resdoms nullable
**	    in all subqueries to ensure correct results when nulls are returned
**	    by the outer join partition(s). This change fixes problem INGSRV
**	    2626, bug 111412.
**	4-mar-05 (inkdo01)
**	    Change to fix 114011 in the handling of case expressions involving
**	    result columns from inner side of outer join (can cause ii_iftrue
**	    execution errors, or simply wrong results). This is a bit of a hack
**	    and may require further refinement.
**	4-apr-05 (toumi01)
**	    Back out above fix for 114011 because it causes wrong results from
**	    some left outer join queries; the ii_iftrue is indeed needed.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
static VOID
opl_target(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          **qnodepp)
{
	PST_QNODE	   *qnodep = *qnodepp;

    if (qnodep->pst_sym.pst_type != PST_RESDOM) /* recursive call */
    {
	i4		opl_smask_left;

	if (qnodep->pst_sym.pst_type == PST_VAR)
	{
	    if (subquery->ops_vars.opv_base->opv_rt
		[qnodep->pst_sym.pst_value.pst_s_var.pst_vno]->opv_ojeqc
		!= OPE_NOEQCLS)
	    {
		if (qnodep->pst_sym.pst_dataval.db_datatype > 0)
		    subquery->ops_oj.opl_smask |= OPL_IFTRUE; /* if the 
				** type is not-nullable, then it becomes
				** nullable and the tree needs to be 
				** re-resolved */
		opl_iftrue(subquery, qnodepp, OPL_NOOUTER); /* insert the
				** "IFTRUE" function with the special eqcls */
		ope_aseqcls(subquery,
			    &subquery->ops_eclass.ope_maps.opo_eqcmap,
			    *qnodepp);  /* assign eqcls */
	    }
	    return;
	}
	if (qnodep->pst_left)
	{
	    opl_target(subquery, &qnodep->pst_left);
	}
	if (qnodep->pst_right)
	{
	    opl_smask_left = subquery->ops_oj.opl_smask & OPL_IFTRUE;
	    subquery->ops_oj.opl_smask &= ~OPL_IFTRUE;

	    opl_target(subquery, &qnodep->pst_right);

	    subquery->ops_oj.opl_smask |= opl_smask_left;
	}
	if (subquery->ops_oj.opl_smask & OPL_IFTRUE &&
	    qnodep->pst_sym.pst_dataval.db_datatype > 0)
	    opl_nulltype(qnodep);
	return;
    }

    for (;
	 *qnodepp;
	 qnodepp = &(*qnodepp)->pst_left)
    {	/* use iteration on target list so that stack does not overflow */

	if ((*qnodepp)->pst_right)
	    opl_target(subquery, &(*qnodepp)->pst_right);

	if (((*qnodepp)->pst_sym.pst_type == PST_RESDOM) &&
	    (subquery->ops_oj.opl_smask & OPL_IFTRUE))
	{
	    if ((subquery->ops_mode == PSQ_RETRIEVE) ||
		(subquery->ops_mode == PSQ_RETINTO)  ||
		(subquery->ops_mode == PSQ_DEFCURS)  ||
		(subquery->ops_mode == PSQ_DGTT_AS_SELECT)  ||
		!((*qnodepp)->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)) /*
				** non-printing resdoms may be null */
	    {
		if ((*qnodepp)->pst_sym.pst_dataval.db_datatype > 0)
		{
		    /* cannot copy datatype since DB_TABKEY_TYPE gets converted
		    ** to CHAR in a resdom node */
		    opl_nulltype(*qnodepp);

		    if (subquery->ops_gentry >= 0)
		    {
			/*    BUG 59355
			**
			**    Wrong size output buffer was compiled.
			**
			**    OPC compiled the wrong size output buffer for a UNION of two
			**    outer joins.  The output buffer didn't include space for the
			**    null indicator.  This is because OPF needs to force the 
			**    first target list of a union to be a superset of all the 
			**    subqueries.*
			*/
			/* if this is a view and a target list item can change from non- 
			** null to a null value, then the parent query references to those
			** resdoms need to be changed to nullable also, the same may be 
			** said for function aggregates but in that case the flattening 
			** algorithms fail and a retry without flattening is necessary, 
			** since a basic assumption of flattening is that the domains of
			** the by list is the same as the domain of the link back 
			** expression (including any nulls) */
			OPV_GRV		*grvp;
			OPS_SUBQUERY    *gsubquery;
			OPZ_IATTS	rsno;
			bool		found_resdom = FALSE;
			PST_QNODE	*resdomp;
			grvp = subquery->ops_global->ops_rangetab.opv_base->
			    opv_grv[subquery->ops_gentry];
			grvp->opv_gmask |= OPV_NULLCHANGE;
					/* this indicates that var types have
					** changed so that parent var nodes
					** needs to be changed */
			subquery->ops_global->ops_gmask |= OPS_NULLCHANGE;
			rsno = (OPZ_IATTS) (*qnodepp)->
				pst_sym.pst_value.pst_s_rsdm.pst_rsno;
			if (grvp->opv_gquery &&
				    grvp->opv_gquery->ops_sqtype != OPS_RFAGG &&
				    grvp->opv_gquery->ops_sqtype != OPS_HFAGG)
			{
			    for (gsubquery = grvp->opv_gquery;
				gsubquery; 
				gsubquery = gsubquery->ops_union)
			    {   /* traverse each subquery in the union view and
				** change the resdom number to be nullable */

				for (resdomp = gsubquery->ops_root->pst_left;
				    resdomp && (resdomp->pst_sym.pst_type ==
								PST_RESDOM);
				    resdomp = resdomp->pst_left)
				{
				    if (resdomp->pst_sym.pst_value.
					pst_s_rsdm.pst_rsno == rsno)
				    {
					found_resdom = TRUE;
					opl_nulltype(resdomp);
					break;
				    }
				}
			    }
			    if (!found_resdom && grvp->opv_gquery)
				opx_error(E_OP068F_MISSING_RESDOM);
			}
			else if (grvp->opv_gquery)
			{
			    OPS_SUBQUERY    *usubquery = (OPS_SUBQUERY *)NULL;

			    for (gsubquery = grvp->opv_gquery; gsubquery;
				gsubquery = gsubquery->ops_next)
			    {
				if (gsubquery->ops_sqtype == OPS_UNION)
				{
				    usubquery = gsubquery;
				    break;
				}
			    }

			    for (gsubquery = grvp->opv_gquery;
				gsubquery; gsubquery = gsubquery->ops_next)
			    {
				for (resdomp = gsubquery->ops_root->pst_left;
				    resdomp &&
				    (resdomp->pst_sym.pst_type == PST_RESDOM) &&
				    (resdomp->pst_right->pst_sym.pst_type !=
								    PST_AOP);
				    resdomp = resdomp->pst_left)
				    opl_nulltype(resdomp);

				if (usubquery == NULL)
				    break;
			    }
			}
		    }
		}
#if 0
		/* cannot use this since DB_TABKEY_TYPE gets converted to char and would
		** incorrectly invoke this */
		else if ((*qnodepp)->pst_right->pst_sym.pst_dataval.db_datatype
                    !=
                    (*qnodepp)->pst_sym.pst_dataval.db_datatype)
		    opx_error(9999);	    /* unexpected datatype difference */
#endif
	    }
	    subquery->ops_oj.opl_smask &= (~OPL_IFTRUE); /* reset iftrue flag for
					** next resdom expression */
	}
    }
}

/*{
** Name: opl_ojeqcls	- create a "NULL INDICATOR" equivalence class
**
** Description:
**      This routine creates a boolean attribute which will be TRUE if the
**      values in a row from this table are not NULL. 
[@comment_line@]...
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varno                           variable number for which the 
**					equivalence class needs to be created
**      varp                            ptr to variable descriptor
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
**      1-nov-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opl_ojeqcls(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          varno,
	OPV_VARS           *varp)
{
    OPZ_FATTS           *func_attr; /* ptr to function attribute used
				    ** to contain constant node */
    PST_QNODE		*const_ptr;
    OPV_VARS		*basevarp;
    OPV_IVARS		basevar;

    const_ptr = subquery->ops_global->ops_goj.opl_one;
    func_attr = opz_fmake( subquery, const_ptr, OPZ_MVAR);
    func_attr->opz_mask |= OPZ_OJFA;	/* mark this function attribute as being
				    ** created for purposes of an outer join */
    subquery->ops_funcs.opz_fmask |= OPZ_MAFOUND; /* mark multi-variable
				    ** function attribute, ... normally should
				    ** be marked in opz_fmake, but sometimes
				    ** multi-variable function attributes are
				    ** removed, so that marking is more desirable
				    ** when the multi-variable function attribute
				    ** will definitely be used */
    STRUCT_ASSIGN_MACRO( const_ptr->pst_sym.pst_dataval, 
	func_attr->opz_dataval);
    varp->opv_ojattr = opz_ftoa( subquery, func_attr, varno, const_ptr,
	&func_attr->opz_dataval, OPZ_MVAR); /* mark all special equivalence
					** classes to be multi-variable since
					** they do not appear at an orig node
					** but instead they appear at a join
					** node */
    basevar = varp->opv_index.opv_poffset;
    if (basevar < 0)
	opx_error(E_OP0287_OJVARIABLE);
    basevarp = subquery->ops_vars.opv_base->opv_rt[basevar];
    if ((basevar != varno)
	&&
	(basevarp->opv_ojeqc == OPE_NOEQCLS))
	opl_ojeqcls(subquery, basevar, basevarp);
    if (basevarp->opv_ojeqc >= 0)
    {
	DB_STATUS	success;
	varp->opv_ojeqc = basevarp->opv_ojeqc;
	success = ope_addeqcls(subquery, varp->opv_ojeqc, varp->opv_ojattr, TRUE); 
#ifdef    E_OP0385_EQCLS_MERGE
	if (!success)
	    opx_error(E_OP0385_EQCLS_MERGE);
#endif
    }
    else
	varp->opv_ojeqc = ope_neweqcls(subquery, varp->opv_ojattr); /* assign a
					    ** new equivalence class */
}

/*{
** Name: opl_ojoin	- process outer join IDs for this subquery
**
** Description:
**      This routine will initialize some of the bitmaps of the 
**	outer join descriptor for this subquery.  
**
** Inputs:
**      subquery                        subquery which contains outer
**					join
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
**      14-sep-89 (seputis)
**          initial creation
**      15-feb-93 (ed)
**          fix outer join placement bug 49322
**      15-feb-94 (ed)
**          bug 49814 - E_OP0397 consistency check, opv_ijmap not
**          updated properly
**	22-apr-94 (ed)
**	    b59937 - make sure indices are placed into opl_p1 opl_p2
**	    partitions for relation placement
**      16_nov-94 (inkdo01)
**          fiddled maxojmap computation to fix bug 63997 involving
**          cross products under nested outer joins.
**      6-jul-95 (inkdo01)
**          b69462 - added check for basevar to be in bvmap, before 
**          failing with OP03A1 (?)
**	27-jul-98 (hayke02)
**	    We now make sure that all the func att eqiv classes are added
**	    into opo_eqcmap if we have more than one OJ in this subquery.
**	    This prevents wrong results from nested full joins where the
**	    select clause does not contain any columns from certain join
**	    tables. This change fixes bug 88288.
**   	16-nov-04 (hayke02)
**	    Back out the fix for 63997 and now BTor() outerp0->opl_ojtotal
**	    and outerp1->opl_maxojmap for the first two outer join IDs. This
**	    change fixes bug 109370.
**	30-apr-04 (inkdo01)
**	    Changed DB_TID_LENGTH to DB_TID8_LENGTH for partitioned tables.
**      31-Aug-2005 (wanfr01) Bug 111375 INGSRV2618
**          Query containing many inner joins with many restrictions hangs
**          in the optimizer.
**	 4-oct-2005 (hayke02)
**	    Back out fix for 109370 (reinstate fix for 63997) and instead fix
**	    in opl_ojverify(). This change fixes bug 114807, problem INGSRV
**	    3349.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
VOID
opl_ojoin(
	OPS_SUBQUERY       *subquery)
{
    OPV_RT	    *vbase;
    OPL_OJT	    *lbase;
    OPL_IOUTER	    ojid1;
    OPL_IOUTER	    maxoj;
    OPL_OUTER	    *ojp1;
    OPV_IVARS	    maxpvar;
    OPV_IVARS	    maxvar;
    OPZ_IATTS	    maxattr;
    OPZ_AT	    *abase;

    lbase = subquery->ops_oj.opl_base;
    vbase = subquery->ops_vars.opv_base;
    abase = subquery->ops_attrs.opz_base;
    maxattr = subquery->ops_attrs.opz_av;
    maxoj = subquery->ops_oj.opl_lv;
    maxpvar = subquery->ops_vars.opv_prv;
    maxvar = subquery->ops_vars.opv_rv;
    {	/* traverse the function attribute table and assign
	** equivalence classes */
	OPZ_IFATTS	max_fa;	/* index of last free slot in function
				** attribute attribute array which contains
				** a qualification
				*/
	OPZ_FT		*fbase;	/* base of function attribute array */
	OPZ_AT		*abase;	/* base of joinop attribute array */
	OPZ_IFATTS	index;	/* index into function attribute array*/
	fbase = subquery->ops_funcs.opz_fbase;
	abase = subquery->ops_attrs.opz_base;
	max_fa = subquery->ops_funcs.opz_fv;
	
    	/* analyze all elements which are still qualifications and convert
	** any in which both branches of the qualification are within the
	** given class
	*/

	for ( index = 0; index < max_fa ; index++)
	{
	    OPZ_FATTS            *fatt;	/* ptr to function attribute element */

	    fatt = fbase->opz_fatts[index]; /* get ptr to function attribute
					** descriptor */
	    if (fatt->opz_mask & OPZ_OJFA)
	    {
		OPZ_ATTS	*attrp;
		OPE_IEQCLS	eqcls;

		attrp = abase->opz_attnums[fatt->opz_attno];
		if (attrp->opz_equcls == OPE_NOEQCLS)
		    /* assign an equivalence class to this attribute */
		    eqcls = ope_neweqcls(subquery, fatt->opz_attno);
		else
		    eqcls = attrp->opz_equcls;
		vbase->opv_rt[attrp->opz_varnm]->opv_ojeqc = eqcls;
		if (maxoj > 1)
		    BTset((i4) eqcls,
			(char *)&subquery->ops_eclass.ope_maps.opo_eqcmap);
	    }
	}
    }
    {
	/* traverse indexes and create joinop attributes for the
	** special equivalence class */

	OPV_IVARS   varno1;
	for (varno1 = subquery->ops_vars.opv_rv; --varno1 >= 0;)
	{
	    OPV_VARS    *var1p;
	    OPV_IVARS	basevno;
	    var1p = vbase->opv_rt[varno1];
	    basevno = var1p->opv_index.opv_poffset;
	    if ((basevno >= 0)
		&&
		(basevno != varno1)
		&&
		(vbase->opv_rt[basevno]->opv_ojeqc != OPE_NOEQCLS)
		)
	    {
		/* outer join index has been found */
		opl_ojeqcls(subquery, varno1, var1p);
	    }
	}
    }
    for (ojid1 = maxoj; --ojid1 >= 0;)
    {	/* look at each outer join and include the equivalence
	** classes of any subordinate outer joins into the
	** current set */
	OPV_IVARS	    varno;

	ojp1 = lbase->opl_ojt[ojid1];
	if ((ojp1->opl_type != OPL_LEFTJOIN) && (ojp1->opl_type != OPL_FULLJOIN))
	    continue;
	for (varno = -1; (varno = BTnext( (i4)varno, (char *)ojp1->opl_ivmap, 
	    (i4)maxpvar)) >= 0;)
	{   /* mark all null indicator equivalence classes */
	    OPV_VARS	    *varp;
	    varp = vbase->opv_rt[varno];
	    if (varp->opv_ojeqc != OPE_NOEQCLS)
		BTset( (i4)varp->opv_ojeqc, (char *)ojp1->opl_innereqc);
	}
    }
    {	/* traverse the boolean factors in which an IFTRUE function was
	** inserted which changed the result type of an operation
	** so that the conjunct can be re-resolved
	*/
	
	OPB_IBF             maxbf;          /* number of boolean factors allocated*/
	OPB_BFT             *bfbase;        /* ptr to base of array of ptrs to
					    ** boolean factor elements */
	OPB_IBF		    bf;

	maxbf = subquery->ops_bfs.opb_bv;   /* number of boolean factors allocated*/
	bfbase = subquery->ops_bfs.opb_base;/* ptr to base of array of ptrs to 
					    ** boolean factors */
	for (bf = maxbf; --bf >= 0;)
	{
	    OPB_BOOLFACT	*bfp;
	    bfp = bfbase->opb_boolfact[bf];
	    if (bfp->opb_mask & OPB_RESOLVE)
		opl_resolve(subquery, bfp->opb_qnode);
	}
    }
    for (ojid1 = subquery->ops_oj.opl_lv; --ojid1 >= 0;)
    {	/* set max outer join maps used for relation placement */
	OPL_OUTER	*outerp1;
	OPL_IOUTER	ojid0;

	outerp1 = lbase->opl_ojt[ojid1];
	MEcopy((PTR)outerp1->opl_ojtotal, 
	    sizeof(*(outerp1->opl_maxojmap)),
	    (PTR)outerp1->opl_maxojmap);
	if (outerp1->opl_type != OPL_LEFTJOIN)
	    continue;
	for (ojid0 = maxoj; --ojid0 >= 0;)
	{
	    OPL_OUTER	    *outerp0;
	    if (ojid1 == ojid0)
		continue;
	    outerp0 = lbase->opl_ojt[ojid0];
	    if (outerp0->opl_type != OPL_LEFTJOIN)
		continue;	/* in the case of a full join
				** do not expand the map since
				** optimizations cannot take
				** advantage of this yet */
#if 0
**	    if (BTtest((i4)ojid1, (char *)outerp0->opl_idmap)
**	    	/* found parent of outer join so determine if
**		** inner relations of parent are also inner to
**		** this outer join */
**		&&
**		FALSE &&	/* for bug 63997 - leave maxoj same
**				** as ojtotal, and drop this scary
**				** looking logic which seems to rely
**				** on the OPL_OJFILTER changes  */
**		BTsubset((char *)outerp1->opl_bvmap, 
**		    (char *)outerp0->opl_ovmap, (i4)maxvar))
**		/* outer relations are in outer portion of this
**		** outer join so all inner relations of this parent
**		** are also considered inner to this outer join */
**	    {
**		BTor((i4)maxvar, (char *)outerp0->opl_ojtotal,
**		    (char *)outerp1->opl_maxojmap);
**	    }
#endif
	}
    }
    for (ojid1 = subquery->ops_oj.opl_lv; --ojid1 >= 0;)
    {	/* check for full joins and limit opl_maxojmap to
	** be within scope of full join */
	OPL_OUTER	*outerp3;
	outerp3 = lbase->opl_ojt[ojid1];
	if (outerp3->opl_type == OPL_FULLJOIN)
	    BTor((i4)maxoj, (char *)outerp3->opl_idmap,
		(char *)outerp3->opl_reqinner);	/* force all
				** outer joins within scope
				** of this full join to be
				** executed prior to the full
				** join */
    }
    for (ojid1 = maxoj; --ojid1 >= 0;)
    {	/* set the minimum inner oj map to be those variables
	** which are in the max map and are also referenced
	** in the ON clause */
	bool	    notfound;
	OPZ_IATTS   attr;
	OPL_OUTER   *outerp5;
	OPV_IVARS   index;

	outerp5 = lbase->opl_ojt[ojid1];
	if (outerp5->opl_type == OPL_FULLJOIN)
	{   /* execute FULL JOINS as written by user
	    ** without attempting associativity */
	    MEcopy((PTR)outerp5->opl_maxojmap, 
		sizeof(*outerp5->opl_minojmap),
		(PTR)outerp5->opl_minojmap);
	    for (index = subquery->ops_vars.opv_rv;
		 --index >= subquery->ops_vars.opv_prv;)
	    {	/* adjust the opl_p1 opl_p2 maps for
		** the partitioning of tables of this full joins */
		OPV_IVARS	basevar;
		basevar  = vbase->opv_rt[index]->opv_index.opv_poffset;
		if (BTtest((i4)basevar, (char *)outerp5->opl_p1))
		    BTset((i4)index, (char *)outerp5->opl_p1);
		else if (BTtest((i4)basevar, (char *)outerp5->opl_p2))
		    BTset((i4)index, (char *)outerp5->opl_p2);
                           /* only test vars in opl_bvmap - b69462 */
		else if (BTtest((i4)basevar, (char *)outerp5->opl_bvmap))
		    opx_error(E_OP03A1_OJMAP); /* inconsistent map */
	    }
	    continue;
	}
	notfound =TRUE;
	for (attr = -1; (attr=BTnext((i4)attr, (char *)outerp5->opl_ojattr,
	    (i4)maxattr))>=0 ;)
	{
	    OPV_IVARS	    varnm;
	    varnm = abase->opz_attnums[attr]->opz_varnm;
	    if (BTtest((i4)varnm, (char *)outerp5->opl_maxojmap))
	    {
		BTset((i4)varnm, (char *)outerp5->opl_minojmap);
		notfound = FALSE;
	    }
	}
	/* add implicitly referenced secondary indices */
	for (index = subquery->ops_vars.opv_rv;
	    --index >= subquery->ops_vars.opv_prv;)
	{
	    OPV_IVARS	    primary;
	    primary = vbase->opv_rt[index]->opv_index.opv_poffset;
	    if (BTtest((i4)primary, (char *)outerp5->opl_minojmap))
		BTset((i4)index, (char *)outerp5->opl_minojmap);
	    if (BTtest((i4)primary, (char *)outerp5->opl_maxojmap))
		BTset((i4)index, (char *)outerp5->opl_maxojmap);
	}
	if (notfound)
	    MEcopy((PTR)outerp5->opl_ojtotal, 
		sizeof(*(outerp5->opl_minojmap)),
		(PTR)outerp5->opl_minojmap); /* for rare case in which
				** no inner relations are referenced
				** in the ON clause, copy all variables
				** in inner, so that minojmap is not
				** empty */
	outerp5->opl_rmaxojmap = outerp5->opl_maxojmap;
    }
    opl_target(subquery, &subquery->ops_root->pst_left); /* place IFTRUE
				** functions into the target list if necessary
				** and assign equivalence classes */

    {	/* initialize the opz_outervars map which will help with relation
	** placement for outer joins,... this map is the set of vars
	** which contain attributes in the same equivalence class as the
	** current attribute, but each var in the set is also outer
	** and not inner to the var containing the current attribute */
	OPZ_IATTS	attr;
	OPZ_IATTS	maxattr;
	OPE_ET		*ebase;
	OPZ_ATTS	*attrp;

	ebase = subquery->ops_eclass.ope_base;
	maxattr = subquery->ops_attrs.opz_av;
	for (attr = maxattr; --attr >= 0;)
	{
	    OPV_IVARS	    varno2;
	    OPZ_IATTS	    parentattr;
	    OPZ_BMATTS	    *attrmap;
	    OPL_BMOJ	    *ojmap;
	    OPL_BMOJ	    *ijmap;
            RDR_INFO        *gvar1, *gvar2;
            DB_ATTNUM       att1, p_att;

	    attrp = abase->opz_attnums[attr];
	    varno2 = attrp->opz_varnm;
	    ijmap = vbase->opv_rt[varno2]->opv_ijmap;
	    ojmap = vbase->opv_rt[varno2]->opv_ojmap;
	    attrmap = &ebase->ope_eqclist[attrp->opz_equcls]->ope_attrmap;
            gvar1 = vbase->opv_rt[varno2]->opv_grv->opv_relation;
	    for (parentattr = -1; (parentattr = BTnext((i4)parentattr,
		(char *)attrmap, (i4)maxattr))>=0;)
	    {
		OPV_IVARS	    parentvar;
		OPL_BMOJ	    *parentojmap;
		OPL_BMOJ	    *parentijmap;

		parentvar = abase->opz_attnums[parentattr]->opz_varnm;
		parentojmap = vbase->opv_rt[parentvar]->opv_ojmap;
		parentijmap = vbase->opv_rt[parentvar]->opv_ijmap;
                gvar2 = vbase->opv_rt[abase->opz_attnums[parentattr]->opz_varnm]->opv_grv->opv_relation;
                att1 =  abase->opz_attnums[attr]->opz_attnm.db_att_id;
                p_att = abase->opz_attnums[parentattr]->opz_attnm.db_att_id;
		if ((parentattr == attr)
		    ||
		    (!parentojmap && !parentijmap) /* this is an outer
				    ** relation which always should be
				    ** included */
		    )
		    ;
                else if
                     ((gvar1) && (gvar2) &&
                        (gvar2->rdr_rel->tbl_status_mask & DMT_IDX) &&
                        (gvar2->rdr_rel->tbl_id.db_tab_base ==
                         gvar1->rdr_rel->tbl_id.db_tab_base) &&
                         (att1 >= 0) &&
                         (p_att >= 0) &&
                         (gvar1->rdr_attr[att1]) &&
                         (gvar2->rdr_attr[p_att]) &&
                         !(STxcompare(
((*(gvar1->rdr_attr[att1])).att_name).db_att_name,DB_MAXNAME,
((*(gvar2->rdr_attr[p_att])).att_name).db_att_name,DB_MAXNAME, FALSE, FALSE))
                    );
		else if (ijmap && parentojmap)
		{   /* check if the relation is outer to one joinid which
		    ** this relation is an inner of, in which case the
		    ** special eqcls will be >= that of varno2 */
		    OPL_BMOJ	    tempojmap;
		    MEcopy((PTR)ijmap, sizeof(tempojmap),
			(PTR)&tempojmap);
		    BTand((i4)maxoj, (char *)parentojmap, 
			(char *)&tempojmap);
		    if (BTnext((i4)-1, (char *)&tempojmap,
			(i4)maxoj)<0)
			continue;
		}
		else if (ijmap && parentijmap
		    &&
		    BTsubset((char *)parentijmap, (char *)ijmap, (i4)maxoj))
		    ;
		else
		    continue;
		BTset((i4)parentvar,(char *)&attrp->opz_outervars);
	    }
	}

	for (ojid1 = maxoj; --ojid1 >=0;)
	{   /* for each full join, check that the opz_outervars map of the
	    ** attributes are disjoint for the left and right partitions of
	    ** the full join */
	    OPL_OUTER	    *outerp2;
	    outerp2 = lbase->opl_ojt[ojid1];
	    if (outerp2->opl_type == OPL_FULLJOIN)
	    for (attr = maxattr; --attr >= 0;)
	    {
		OPV_BMVARS	*maskp;
		attrp = abase->opz_attnums[attr];
		if (BTtest((i4)attrp->opz_varnm, (char *)outerp2->opl_p1))
		    maskp = outerp2->opl_p2;
		else if (BTtest((i4)attrp->opz_varnm, (char *)outerp2->opl_p2))
		    maskp = outerp2->opl_p1;
		else
		    continue;
		BTclearmask((i4)maxvar, (char *)maskp, (char *)&attrp->opz_outervars);
				/* remove variables from other partition
				** since if an equi-join is made, both attrs
				** must be in the scope of the full join
				** otherwise a multi-attr equi-join may be
				** split such as in b59935 */
	    }
	}
    }
    if ( (subquery->ops_global->ops_cb->ops_check &&
	 opt_strace(subquery->ops_global->ops_cb, OPT_F029_HISTMAP)))
	    opt_ojstuff(subquery);   /* display oj table after all the
	                           ** all the messing around is done   */
}

/*{
** Name: opl_index	- update outer join info for an index
**
** Description:
**      When an index is added, it should take on the outer join
**	characteristics of the base relation 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      basevar                         variable number of base relation
**      indexvar                        variable number of index
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
**      25-oct-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opl_index(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          basevar,
	OPV_IVARS          indexvar)
{
    OPV_RT	*vbase;
    OPV_VARS	*ivarp;	    /* ptr to index range var descriptor */
    OPV_VARS	*bvarp;	    /* ptr to base range var descriptor */
    OPL_IOUTER	ojid;	    /* outer join id being analyzed */
    OPL_IOUTER	maxoj;	    /* number of outer joins defined */
    OPL_OJT	*lbase;	    /* base of ptrs to outer join descriptors */

    vbase = subquery->ops_vars.opv_base;
    ivarp = vbase->opv_rt[indexvar];
    bvarp = vbase->opv_rt[basevar];
    ivarp->opv_ojmap = bvarp->opv_ojmap;
    ivarp->opv_ijmap = bvarp->opv_ijmap;
    lbase = subquery->ops_oj.opl_base;
    maxoj = subquery->ops_oj.opl_lv;
    if (ivarp->opv_ijmap)
    {	/* opl_ojtotal contains all indexes and base relations which should
	** be inner to an outer join */
	for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)ivarp->opv_ijmap, (i4)maxoj))>=0;)
	{
	    BTset((i4)indexvar, (char *)lbase->opl_ojt[ojid]->opl_ojtotal);
	    BTset((i4)indexvar, (char *)lbase->opl_ojt[ojid]->opl_ojbvmap);
	}
    }
    if (ivarp->opv_ojmap)
    {	/* opl_ojbvmap contains all indexes and base relations of outer join id */
	for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)ivarp->opv_ojmap, (i4)maxoj))>=0;)
	    BTset((i4)indexvar, (char *)lbase->opl_ojt[ojid]->opl_ojbvmap);
    }
}

/*{
** Name: opl_inner	- remove outer join from bitmaps
**
** Description:
**      It has been determined that an outer join or portion of a FULL join 
**      has degenerated into an INNER join, so update the bitmaps to reflect 
**      this new situation. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      ojp                             ptr to outer join descriptor
**      varmap                          ptr to map of outer variables affected
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
**      29-oct-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opl_inner(
	OPS_SUBQUERY       *subquery,
	OPL_OUTER          *ojp,
	OPV_BMVARS         *varmap)
{
    OPV_IVARS	    varno;
    OPV_VARS	    *varp;
    OPV_IVARS	    maxvars;	/* number of range variables defined in
				** the query */
    OPV_RT	    *vbase;	/* base of ptrs to range variable descriptors */

    vbase = subquery->ops_vars.opv_base;
    maxvars = subquery->ops_vars.opv_prv;
    for (varno = -1; (varno = BTnext((i4)varno, (char *)varmap, (i4)maxvars))>=0;)
    {	/* reset the joinids for all variables in the outer map */
	varp = vbase->opv_rt[varno];
	if (varp->opv_ojmap)
	{   /* convert this OUTER JOIN variable to an INNER JOIN */
	    BTclear((i4)ojp->opl_id, (char *)varp->opv_ojmap);
	    if (!varp->opv_ijmap)
	    {
		varp->opv_ijmap = (OPL_BMOJ *)opu_memory(subquery->ops_global,
		    (i4)sizeof(*varp->opv_ijmap));
		MEfill(sizeof(*varp->opv_ijmap), (u_char)0,
		    (PTR)varp->opv_ijmap);
	    }
	    BTset((i4)ojp->opl_id, (char *)varp->opv_ijmap);
	}
    }
    BTor((i4)BITS_IN(*varmap), (char *)varmap, (char *)ojp->opl_ivmap);
    BTor((i4)BITS_IN(*varmap), (char *)varmap, (char *)ojp->opl_ojtotal);
    if (!MEcmp((PTR)varmap, (PTR)ojp->opl_ovmap, sizeof(*varmap)))
    {	/* all the variables in the outer join are eliminated */
	MEfill(sizeof(*ojp->opl_ovmap), (u_char)0, (PTR)ojp->opl_ovmap);
    }
    else if(BTsubset((char *)varmap, (char *)ojp->opl_ovmap, (i4)BITS_IN(*varmap)))
    {
	BTclearmask( (i4)BITS_IN(*varmap), (char *)varmap, (char *)ojp->opl_ovmap);
    }
    else
	opx_error( E_OP03A1_OJMAP);	/* outer join maps are inconsistent */
}

/*{
** Name: opl_fjinner	- remove inner variables from FULL JOIN
**
** Description:
**      A full join has degenerated, so remove the set of variables which are
**	still both in the inner and outer maps, from the inner map.  opl_inner
**	will move variables in the outer map to be placed into the inner map.
**	The routine is part of the process of converting a FULL JOIN into a LEFT JOIN 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      ojp                             ptr to outer join descriptor
**      varmap                          ptr to map of inner variables affected
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
**      29-oct-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opl_fjinner(
	OPS_SUBQUERY       *subquery,
	OPL_OUTER          *ojp,
	OPV_BMVARS         *varmap)
{
    OPV_IVARS	    varno;
    OPV_VARS	    *varp;
    OPV_IVARS	    maxvars;	/* number of range variables defined in
				** the query */
    OPV_RT	    *vbase;	/* base of ptrs to range variable descriptors */

    vbase = subquery->ops_vars.opv_base;
    maxvars = subquery->ops_vars.opv_prv;
    for (varno = -1; (varno = BTnext((i4)varno, (char *)varmap, (i4)maxvars))>=0;)
    {	/* reset the joinids for all variables in the outer map */
	varp = vbase->opv_rt[varno];
	if (varp->opv_ojmap 
	    && 
	    varp->opv_ijmap
	    &&
	    BTtest((i4)ojp->opl_id, (char *)varp->opv_ojmap)
	    &&
	    BTtest((i4)ojp->opl_id, (char *)varp->opv_ijmap)
	    )
	{   /* remove inner variable reference and leave the outer remaining */
	    BTclear((i4)ojp->opl_id, (char *)varp->opv_ijmap);
	}
	if (BTtest((i4)varno, (char *)ojp->opl_ovmap))
	{   /* only clear if variable is not in outer map, i.e. it is
	    ** not been previously reduced to a left join, since then the
	    ** variable will not be in either the inner or outer maps */
	    BTclear((i4)varno, (char *)ojp->opl_ivmap);
	    BTclear((i4)varno, (char *)ojp->opl_ojtotal);
	}
    }
}

/*{
** Name: opl_reverse	- change scope of inner join
**
** Description:
**      An inner join cannot be flattened into a full join but the 
**      scope of the inner join can be changed so that it becomes 
**      higher in scope than all of its' OJ parents which are children 
**      of this outer join 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      fulljoinid                      full join id which is the parent of ojid
**      ojid                            inner join to be changed in scope
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
**      19-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opl_reverse(
	OPS_SUBQUERY	    *subquery,
	OPL_IOUTER	    fj_ojid,
	OPL_IOUTER	    inner_ojid)
{
    OPL_OJT	    *lbase;	/* base of ptrs to outer join descriptors */
    OPL_IOUTER	    maxoj;	/* number of outer joins defined */
    OPL_OUTER	    *ijp;	/* inner join pointer */
    OPL_OUTER	    *fjp;	/* full join pointer */
    OPV_BMVARS	    varmap;	/* map of variables which are in the
				** same scope of the outer join as the
				** inner_ojid */
    bool	    changes;	/* TRUE if another outer join ID was
				** found which is on the same side of
				** the full join as the inner join */
    OPL_OUTER	    *parent;	/* descriptor of parent which should
				** be made of child of the inner join */
    OPL_IOUTER	    ojid;	/* outer join id being analyzed */
    OPV_IVARS	    maxvar;

    maxvar = subquery->ops_vars.opv_rv;
    lbase = subquery->ops_oj.opl_base;
    maxoj = subquery->ops_oj.opl_lv;
    fjp = lbase->opl_ojt[fj_ojid];
    ijp = lbase->opl_ojt[inner_ojid];
    MEcopy((PTR)ijp->opl_bvmap, sizeof(varmap), (PTR)&varmap);
    do
    {	/* the full join divides the relations into 2 sets; this loop
	** finds the set of relations which contain the inner join */
	changes = FALSE;
	for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)fjp->opl_idmap, (i4)maxoj))>=0;)
	{
	    OPV_BMVARS	    parent_map;
	    parent = lbase->opl_ojt[ojid];
	    if (BTsubset((char *)parent->opl_bvmap, (char *)&varmap, (i4)BITS_IN(*parent->opl_bvmap)))
		continue;	/* this joinid was already considered */
	    MEcopy((PTR)parent->opl_bvmap, sizeof(parent_map), (PTR)&parent_map);
	    BTand((i4)BITS_IN(varmap), (char *)&varmap, (char *)&parent_map);
	    if (BTnext((i4)-1, (char *)&parent_map, (i4)maxvar) >= 0)
	    {	/* intersection exists so add variable to common map */ 
		BTor((i4)BITS_IN(varmap), (char *)parent->opl_bvmap, (char *)&varmap);
		changes = TRUE;
	    }
	}
    } while (changes);

    for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)fjp->opl_idmap, (i4)maxoj))>=0;)
    {	/* visit all joinid's which have variables in the common map
	** and place them within the scope of the inner join */
	parent = lbase->opl_ojt[ojid];
	if ((ojid != inner_ojid)
	    &&
	    BTsubset((char *)parent->opl_bvmap, (char *)&varmap, (i4)BITS_IN(varmap))
	    )
	{
	    BTor((i4)BITS_IN(ijp->opl_ivmap), (char *)parent->opl_bvmap, (char *)ijp->opl_ivmap);
	    BTor((i4)BITS_IN(ijp->opl_ivmap), (char *)parent->opl_bvmap, (char *)ijp->opl_ojbvmap);
	    BTor((i4)BITS_IN(ijp->opl_ivmap), (char *)parent->opl_bvmap, (char *)ijp->opl_bvmap);
	    BTor((i4)BITS_IN(ijp->opl_ivmap), (char *)parent->opl_bvmap, (char *)ijp->opl_ojtotal);
	    BTset ((i4)ojid, (char *)ijp->opl_idmap);
	    BTclear ((i4)inner_ojid, (char *)parent->opl_idmap);
	}
    }
    {
	OPV_IGVARS	    varno;
	for (varno = -1; (varno = BTnext((i4)varno, (char *)&varmap, (i4)BITS_IN(varmap))) >= 0;)
	{
	    BTset ((i4)inner_ojid, (char *)subquery->ops_vars.opv_base->opv_rt[varno]->opv_ijmap);
				    /* make all variables on this side of the full
				    ** join, inner to the inner join */
	}
    }
}

/*{
** Name: opl_ojmaps	- initialize some outer join maps
**
** Description:
**      There is a hierarchy to outerjoins which are not easily
**	determined by the existing maps in the parser range table
**	this routine will create maps which indicate which outer
**	join IDs are children of other outer join IDs etc. i.e.
**	describe the outer join hierarchy.
**	    This routine will also perform optimizations to determine
**	whether an outer join degenerates into a normal join.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
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
**      28-oct-89 (ed)
**	    initial creation
**      15-feb-94 (ed)
**          - bug 59598 - correct mechanism in which boolean factors are
**          evaluated at several join nodes
**      11-apr-94 (ed)
**          - b59937 - mark inner join ojids as existing in the subquery
**	10-aug-06 (hayke02)
**	    Use BITS_IN rather than sizeof (bits rather than bytes) for the
**	    BTor() size argument. This change fixes bug 116405.
**      08-apr-2009 (huazh01)
**          comment out the code that sets 'opv_svojid' and checks
**          the E_OP03A6 error. After being allocated and initialized on
**          opv_relatts(), 'opv_svojid' is actually not being used by OPF. 
**          All references to 'opv_svojid' outside opl_ojmaps() have 
**          already been commented out. (bug 120101)
**	23-Jun-2009 (kibro01) b121964
**	    When a full outer join comprises two sides, both sides of which
**	    contain a subquery of their own, the opl_p2 value includes the
**	    bitmask of the lhs's subquery, which means that taking the first
**	    element of that bitmap and using that to determine if the rhs is
**	    complete fails, with an E_OP0399 error.  Instead during the main
**	    loop say that if the lhs var is a subset of the current join set
**	    the relevant bits in opl_p1 as before, but then if it isn't a
**	    subset set those bits in a separate tmp_other mask, which will be
**	    used both to determine the first element of the rhs, and to
**	    check for completeness of the rhs.  If there are no other subsets
**	    (i.e. other subqueries), use the original opl_p2 calculation.
**	30-Jun-2009 (kibro01) b121964
**	    Additional check found running qp642, where the first OJ can
**	    map 1 2 3 4 5 6, the second 1 2 3, third 4 5 6 and the fourth
**	    7 8 9.  We need to combine the first three into the lhs bitmap
**	    in the check for completeness, so even though we find the first
**	    (bigger) query first, we need to subsume them, which means checking
**	    for a subset in either direction, and only on a second loop going
**	    through checking that those elements not hoovered up during the
**	    first combiner loop get added to the rhs.
[@history_template@]...
*/
VOID
opl_ojmaps(
	OPS_SUBQUERY       *subquery)
{
    OPV_RT	    *vbase;	/* base of ptrs to range variable descriptors */
    OPL_OJT	    *lbase;	/* base of ptrs to outer join descriptors */
    OPL_IOUTER	    ojid;	/* an outer join ID being analyzed */
    OPL_IOUTER	    ojid1;	/* an outer join ID to be analyzed */
    OPL_IOUTER	    ojid2;	/* an outer join ID to be analyzed */
    OPL_OUTER	    *ojp;	/* ptr to outer join descriptor being
				** analyzed */
    OPL_OUTER	    *ojp1;	/* ptr to outer join descriptor being
				** analyzed */
    OPL_OUTER	    *ojp2;	/* ptr to outer join descriptor being
				** analyzed */
    OPV_IVARS	    varno;
    OPV_VARS	    *varp;	/* ptr to current range variable descriptor
				** being analyzed */
    OPL_IOUTER	    maxoj;	/* number of outer joins defined */
    OPV_IVARS	    maxvars;	/* number of range variables defined in
				** the query */

    lbase = subquery->ops_oj.opl_base;
    vbase = subquery->ops_vars.opv_base;
    maxoj = subquery->ops_oj.opl_lv;
    maxvars = subquery->ops_vars.opv_prv;

    for (varno = maxvars; --varno>=0;)
    {	/* traverse all variables and all the outer join IDs and
	** create a map of variables for the left and right sides for 
	** each outer join */
	varp = vbase->opv_rt[varno];
	if (varp->opv_ojmap)
	    for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)varp->opv_ojmap, 
		(i4)maxoj)) >= 0;)
		/* update the outer bit maps */
		BTset((i4)varno, (char *)lbase->opl_ojt[ojid]->opl_ovmap);
	if (varp->opv_ijmap)
	    for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)varp->opv_ijmap, 
		(i4)maxoj)) >= 0;)
		/* update the inner bit maps */
		BTset((i4)varno, (char *)lbase->opl_ojt[ojid]->opl_ivmap);
    }
    for (ojid = maxoj; --ojid >= 0;)
    {	/* make another pass of the outer joins, to update all the
	** variables referenced within the scope of the outer join
	** update the total var map for both the inner and outer */
	ojp = lbase->opl_ojt[ojid];
	MEcopy((PTR)ojp->opl_ivmap, sizeof(*ojp->opl_ivmap),
	    (PTR)ojp->opl_bvmap);
	BTor((i4)maxvars, (char *)ojp->opl_ovmap, (char *)ojp->opl_bvmap);
	BTor((i4)maxvars, (char *)ojp->opl_bvmap, (char *)ojp->opl_ojbvmap);
    }
    for (ojid1 = maxoj; --ojid1 >= 0;)
    {   /* update the outer join ID maps for each outer join descriptor
	** to determine whether an outer join is subordinate to another */
	for (ojid2 = ojid1; --ojid2 >= 0;)
	{
	    ojp1 = lbase->opl_ojt[ojid1];
	    ojp2 = lbase->opl_ojt[ojid2];
	    if (BTsubset((char *)ojp1->opl_bvmap, (char *)ojp2->opl_bvmap,
		(i4)maxvars))
	    {
		BTset((i4)ojid1, (char *)ojp2->opl_idmap);
	    }
	    else if (BTsubset((char *)ojp2->opl_bvmap, (char *)ojp1->opl_bvmap,
		(i4)maxvars))
		BTset((i4)ojid2, (char *)ojp1->opl_idmap);
	}
    }
    if ( (subquery->ops_global->ops_cb->ops_check &&
	 opt_strace(subquery->ops_global->ops_cb, OPT_F029_HISTMAP)))
	    opt_ojstuff(subquery);   /* display oj table            */
    for (ojid1 = maxoj+1; --ojid1 >= 0;)
    {	/* check for degenerate joins by looking at the variables
	** which were referenced in the on clauses */
	OPL_BMOJ	*idmap;
	OPL_BMOJ	ojmap;
	OPV_BMVARS	*where_clause;
	OPV_BMVARS	*pivarmap;   /* map of relations which are
				** considered inner to this join ID */
	OPV_BMVARS	*povarmap;   /* map of relations which are
				** considered outer to this join ID */

	if (ojid1 == maxoj)
	{   /* make a special case for the "main query" which contains
	    ** a where clause which is global in scope to all outer 
	    ** joins */
	    MEfill(sizeof(ojmap), (u_char)0, (PTR)&ojmap);
	    for (ojid2 = maxoj; --ojid2>= 0;)
		BTset((i4)ojid2, (char *)&ojmap);
	    where_clause = subquery->ops_oj.opl_where;
	    idmap = &ojmap;
	    povarmap = (OPV_BMVARS *)NULL;
	}
	else
	{
	    ojp1 =  lbase->opl_ojt[ojid1];
	    where_clause = ojp1->opl_onclause;
	    idmap = ojp1->opl_idmap;
	    povarmap = ojp1->opl_ovmap;
	    pivarmap = ojp1->opl_ivmap;
	}
	for (ojid2 = -1; (ojid2 = BTnext((i4)ojid2, (char *)idmap,
	    (i4)maxoj))>= 0;)
	{   /* looking at join IDs which are within the scope of ojid1, this will
	    ** allow determination of whether a variable referenced outside
	    ** of scope causes the child join to degenerate */
	    OPV_BMVARS	    degenerate_map; /* map of variables which were referenced
				** outside the scope of ojid2 */
	    ojp2 = lbase->opl_ojt[ojid2];
	    if (povarmap)
	    {
		/* for nested ON clause,... include variables which are not in the outer
		** but located in the inner variable map for the strength reduction case,
		** note that this would eliminate any strength reduction for outer joins 
		** nested in a FULL join on clause, since the inner map equals the outer
		** map */
		MEcopy((PTR)povarmap, sizeof(degenerate_map), (PTR)&degenerate_map);
		BTnot((i4)BITS_IN(degenerate_map), (char *)&degenerate_map);
		BTand((i4)BITS_IN(degenerate_map), (char *)pivarmap,
		    (char *)&degenerate_map); /* consider only variables which are
					    ** are inner to the parent outer join
					    ** since these will be "NULL" and eliminate
					    ** tuples, whereas if it was "outer" then
					    ** the tuple would remain */
		BTand((i4)BITS_IN(degenerate_map), (char *)where_clause,
		    (char *)&degenerate_map); 
	    }
	    else
		MEcopy((PTR)where_clause, sizeof(degenerate_map), (PTR)&degenerate_map);
	    BTand((i4)BITS_IN(degenerate_map), (char *)ojp2->opl_ivmap, 
		(char *)&degenerate_map);  /* this is the set of variables referenced
				** outside the scope of ojp2 */
	    if (BTnext((i4)-1, (char *)&degenerate_map, (i4)maxvars)>= 0)
	    {	/* degenerate join has been found so remove variables from the
		** outer map and place into the inner map, effectively making it
		** an inner join */
		if (!BTsubset((char *)&degenerate_map, (char *)ojp2->opl_ovmap,
		    (i4)BITS_IN(degenerate_map)))
		{   /* this is the case of a left join which has degenerated
		    ** since a variable was referenced in the inner which
		    ** is also referenced outside the scope of the ON clause
		    ** so any outer join tuple will fail, make this an INNER
		    ** join */
		    opl_inner(subquery, ojp2, ojp2->opl_ovmap);	/* convert all references to
				    ** ojp2 to be an inner join */
		}
		else
		{   /* this is a FULL JOIN since the variable referenced is in
		    ** both the inner and outer, so check if this degenerates
		    ** to a left join, or an inner join */
		    OPV_BMVARS	    processed;	    /* map of variables which
					** have already been processed */
		    i4		    oj_count;	    /* since there is an outer
					** join hierarchy, there
					** should only be 2 cases
					** to be processed */
		    MEfill(sizeof(processed), (u_char)0, (PTR)&processed);
		    oj_count = 0;
		    for (varno = -1; (varno = BTnext((i4)varno, (char *)&degenerate_map, 
			(i4)maxvars))>=0;)
		    {
			/* for each variable determine the variables which are connected
			** to this by looking at the set of relations in the which are
			** allocated in the same scope of another outer join ID
			** FIXME - consider all variables from the same multi variable view
			** to be connected also */
			OPV_BMVARS	connected;  /* set of variables which are
					    ** connected */
			OPL_IOUTER	ojid3;
			if (BTtest((i4)varno, (char *)&processed))
			    continue;	/* this variable has already been looked at */
			/* look at all the join IDs which are subordinate to this
			** and determine the set of variables which are connected
			** to this variable */
			MEfill(sizeof(connected), (u_char)0, (PTR)&connected);
			BTset((i4)varno, (char *)&connected);
			for (ojid3 = -1; (ojid3 = BTnext((i4)ojid3, (char *)ojp2->opl_idmap,
			    (i4)maxoj))>= 0;)
			{
			    OPL_OUTER	    *ojp3;
			    ojp3 = lbase->opl_ojt[ojid3];
			    if (BTtest((i4)varno, (char *)ojp3->opl_bvmap))
				BTor((i4)BITS_IN(connected), (char *)ojp3->opl_bvmap, (char *)&connected);
			}
			BTor((i4)BITS_IN(processed), (char *)&connected, (char *)&processed);
			/* the connected map contains the variables which if they are on the
			** inner would degenerate this half of the FULL join and make it a
			** LEFT join */
			if ((oj_count == 1)
			    &&
			    MEcmp((PTR)&processed, (PTR)ojp2->opl_bvmap, sizeof(processed))
			    )
			    opx_error(E_OP03A0_OJBITMAP);   /* sum of maps must equal parent
						** at this point */
			opl_fjinner(subquery, ojp2, &connected); /* change variables which are inner
						** and outer, so that they are just outer */
			BTnot((i4)BITS_IN(connected), (char *)&connected);
			BTand((i4)BITS_IN(connected), (char *)ojp2->opl_bvmap, (char *)&connected);
			opl_inner(subquery, ojp2, &connected);	/* convert those variables which
						** are in connected to be considered to be
						** "inner variables only" */
			oj_count++;
			if (oj_count > 2)
			    opx_error(E_OP03A0_OJBITMAP);   /* invalid outer join bitmaps */
			
		    }
		}
	    }
	}
    }
    /* all degenerate joins have been identified and marked as inner joins
    ** so now some analysis is made to remove all inner joins and fold them
    ** into the parent */
    {
	bool	    inner_count;

	inner_count = 0;
	for (ojid1 = maxoj; --ojid1>=0;)
	{
	    ojp1 = lbase->opl_ojt[ojid1];
	    if (BTnext((i4)-1, (char *)ojp1->opl_ovmap, (i4)BITS_IN(OPV_BMVARS))<0)
	    {	/* no relations left which are outer */
		inner_count ++;
		ojp1->opl_type = OPL_INNERJOIN;	
	    }
	    else if (!MEcmp((PTR)ojp1->opl_ovmap, (PTR)ojp1->opl_ivmap, 
			sizeof(*ojp1->opl_ivmap)))
	    {
		OPV_IVARS	varno1;
		OPV_BMVARS	tmp_other;

		ojp1->opl_type = OPL_FULLJOIN;
		/* initialize the opl_p1 and opl_p2 maps which describe the
		** two sets of variables partitioned by this outer join */
		ojp1->opl_p1 = (OPV_BMVARS *)opu_memory(subquery->ops_global, sizeof(*ojp1->opl_p1));
		ojp1->opl_p2 = (OPV_BMVARS *)opu_memory(subquery->ops_global, sizeof(*ojp1->opl_p2));
		varno1 = BTnext((i4)-1, (char *)ojp1->opl_bvmap, (i4)maxvars);
		if (varno1 < 0)
		    opx_error(E_OP0399_FJ_VARSETS);
		MEfill (sizeof(*ojp1->opl_p1), (u_char)0, (PTR)ojp1->opl_p1);
		MEfill (sizeof(*ojp1->opl_p2), (u_char)0, (PTR)ojp1->opl_p2);
		MEfill (sizeof(tmp_other), (u_char)0, (PTR)&tmp_other);
		BTset ((i4)varno1, (char *)ojp1->opl_p1);
		for (ojid2 = -1; (ojid2 = BTnext((i4)ojid2, (char *)ojp1->opl_idmap, (i4)maxoj))>= 0;)
		{
		    if (BTsubset((char *)ojp1->opl_p1, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (i4)maxvars)
			||
		        BTsubset((char *)lbase->opl_ojt[ojid2]->opl_bvmap, (char *)ojp1->opl_p1, (i4)maxvars))
			BTor((i4)maxvars, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (char *)ojp1->opl_p1);
		}
		for (ojid2 = -1; (ojid2 = BTnext((i4)ojid2, (char *)ojp1->opl_idmap, (i4)maxoj))>= 0;)
		{
		    if (!BTsubset((char *)ojp1->opl_p1, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (i4)maxvars)
			&&
		        !BTsubset((char *)lbase->opl_ojt[ojid2]->opl_bvmap, (char *)ojp1->opl_p1, (i4)maxvars))
			BTor((i4)maxvars, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (char *)&tmp_other);
		}
		MEcopy((PTR)ojp1->opl_p1, sizeof(*ojp1->opl_p2), (PTR)ojp1->opl_p2);
		BTnot((i4)maxvars, (char *)ojp1->opl_p2);
		BTand((i4)maxvars, (char *)ojp1->opl_bvmap, (char *)ojp1->opl_p2);
		if (BTcount((char*)&tmp_other,(i4)sizeof(tmp_other)) == 0)
		    MEcopy((PTR)ojp1->opl_p2, sizeof(*ojp1->opl_p2), (PTR)&tmp_other);
		varno1 = BTnext((i4)-1, (char *)&tmp_other, (i4)maxvars);
		if (varno1 < 0)
		    opx_error(E_OP0399_FJ_VARSETS);	/* there should be at least two disjoint sets */
		{
		    OPV_BMVARS	    tempmap;
		    MEfill(sizeof(tempmap), (u_char)0, (PTR)&tempmap);
		    BTset((i4)varno1, (char *)&tempmap);
		    for (ojid2 = -1; (ojid2 = BTnext((i4)ojid2, (char *)ojp1->opl_idmap, (i4)maxoj))>= 0;)
		    {
			if (BTsubset((char *)&tempmap, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (i4)maxvars))
			    BTor((i4)maxvars, (char *)lbase->opl_ojt[ojid2]->opl_bvmap, (char *)&tempmap);
		    }
		    if (MEcmp((PTR)&tempmap, (PTR)&tmp_other, sizeof(tempmap)))
			opx_error(E_OP0399_FJ_VARSETS);	/* there should be no left over variables */
		}
	    }
	    else
		ojp1->opl_type = OPL_LEFTJOIN;	
	}
	if (inner_count == maxoj)
	{   /* all the joins are now inner joins so eliminate all join ids */
	    subquery->ops_oj.opl_base = (OPL_OJT *)NULL;
	    subquery->ops_oj.opl_lv = 0;
	}
	else for (ojid1 = maxoj; --ojid1>=0;)
	{
	    ojp1 = lbase->opl_ojt[ojid1];
	    if (ojp1->opl_type == OPL_INNERJOIN)
	    {   /* an inner join has been found since there is no outer variable */
		OPL_IOUTER	    current_parent;
		current_parent = OPL_NOOUTER; /* represent the main query
					** since there really is not an outer
					** join descriptor for this */
		for (ojid2 = maxoj; --ojid2>= 0;)
		{   /* look at each other outer join and determine what to relabel
		    ** the nodes to be, find the parent which is closest
		    ** in scope to the outer join which is being removed, but is
		    ** not an INNER join itself */
		    if (ojid1 == ojid2)
			continue;
		    ojp2 = lbase->opl_ojt[ojid2];
		    if (ojp2->opl_type == OPL_INNERJOIN)
			continue;	/* only interested in non-inner joins, flatten
				    ** all inner joins into the appropriate parent */
		    if (BTtest((i4)ojid1, (char *)ojp2->opl_idmap) /* found a parent of
					** the inner join to replace */
			&&
			BTsubset((char *)ojp1->opl_bvmap, (char *)ojp2->opl_ivmap,
			    (i4)BITS_IN(*ojp1->opl_ivmap)) /* need to move
					** qualification into inner joinid */
			&&
			(
			    (current_parent == OPL_NOOUTER) /* if there is no other
					** parent */
			    ||
			    !BTtest((i4)current_parent, (char *)ojp2->opl_idmap)
				    	    /* current parent is not closer in
					    ** scope */
			 )
			)
			 current_parent = ojid2;  /* assign a new parent */
		}
		if ((current_parent != OPL_NOOUTER)
		    &&
		    (lbase->opl_ojt[current_parent]->opl_type == OPL_FULLJOIN)
		    )
		    /* the parent for which this is an inner, is a full join
		    ** which cannot have the qualification placed into the on
		    ** clause, in this case the inner join must be preserved
		    ** but can be made to include all the other joinid's below
		    ** the full join, but still a parent to this inner join */
		    opl_reverse(subquery, current_parent, ojid1);
		else
		{
#if 0
		    ojp1->opl_type = OPL_INNERJOIN;
#endif
/* this should allow queries like
** select * from   r_oj left join
**                      ( s_oj left join t_oj on s_oj.c=t_oj.c )
**              on r_oj.a=s_oj.a and r_oj.b=t_oj.b;
** to be executed with more flexibility
*/
		    ojp1->opl_type = OPL_FOLDEDJOIN;
		    ojp1->opl_translate = current_parent;
		}
	    }
	}
    }

    {	/* for each variable which is the inner of a LEFT JOIN, or participates in
	** a FULL join, an equivalence class is created which will be used as a
	** NULL indicator */
	for (varno = maxvars; --varno >= 0;)
	{
	    varp = vbase->opv_rt[varno];
	    if (varp->opv_ijmap)
	    {	/* an inner join map exists, so check for a LEFT JOIN or FULL JOIN */
		for (ojid = -1; (ojid = BTnext((i4)ojid, (char *)varp->opv_ijmap, (i4)maxoj))>=0;)
		{
		    OPL_OJTYPE	    ojtype;
		    OPL_OUTER	    *ojoinp;

		    ojoinp = lbase->opl_ojt[ojid];
		    ojtype = ojoinp->opl_type;
		    if ((ojtype == OPL_FULLJOIN)
					/* an equivalence class will always be needed for
					** all variables in a FULL JOIN */
			||
			(   (ojtype == OPL_LEFTJOIN)
			    &&
			    ojoinp->opl_ivmap
			    &&
			    BTtest((i4)varno, (char *)ojoinp->opl_ivmap)
			    		/* an equivalence class is only needed for variables
					** which are inner of the outer join */
			)
			)
		    {   /* create an equivalence class for this variable */
			opl_ojeqcls(subquery, varno, varp);
			break;
		    }
		}
	    }
#if 0
	    if (varp->opv_ojeqc >= 0)
	    {	/* initialize the opl_iftrue map to be the set of equivalence classes
		** which does not require an iftrue function if the variable referenced
		** with a particular join id */
		for (ojid1 = maxoj; --ojid1 >= 0;)
		{
		    OPL_OUTER	    *ojoinp1;
		    OPL_BMOJ	    *ojbmp;
		    OPL_OJTYPE	    ojtype1;
		    bool	    ojvar_found;

		    ojoinp1 = lbase->opl_ojt[ojid1];
		    ojtype1 = ojoinp1->opl_type;
		    if ( (  (ojtype1 != OPL_LEFTJOIN) 
			    && 
			    (ojtype1 != OPL_FULLJOIN) 
			    && 
			    (ojtype1 != OPL_INNERJOIN) /* inner joins would only exist in
							** the case of a full join with a view
							** but the opl_iftrue function needs to
							** be set, so that the iftrue function
							** is not placed on boolean factors of
							** the inner join */
			 )
			||
			!BTtest((i4)varno, (char *)ojoinp1->opl_bvmap)
			)
			continue;
		    ojbmp = ojoinp1->opl_idmap;
		    ojvar_found = TRUE;
		    for (ojid2 = -1;(ojid2 = BTnext((i4)ojid2, (char *)ojbmp, (i4)maxoj))>=0;)
		    {	/* traverse all the joinids' which are considered to be within
			** the scope of the ojoinp1 outer join, and determine if
			** any of these would require the iftrue function to be
			** used */
			OPL_OJTYPE	    ojtype2;
			OPL_OUTER	    *ojoinp2;

			ojoinp2 = lbase->opl_ojt[ojid2];
			ojtype2 = ojoinp2->opl_type;
			if ((ojtype2 != OPL_LEFTJOIN) && (ojtype2 != OPL_FULLJOIN))
			    continue;
			if (BTtest((i4)varno, (char *)ojoinp2->opl_ivmap))
			{
			    ojvar_found = FALSE;    /* indicates that a null value
						** introduced by a nested outer join
						** can exist */
			    break;
			}
		    }
		    if (ojvar_found)
			BTset ((i4)varp->opv_ojeqc, (char *)ojoinp1->opl_iftrue);
		}
	    }
#endif
	}
    }
    {
	OPL_IOUTER	ojid4;
	
	for (ojid4 = maxoj; ojid4>=0; ojid4--)
	{   
	    /* check that no correlated aggregates exist, which were flattened
	    ** and have a possible NULL generated due to outer join semantics,
	    ** flattening will not work in this case since it depends on all
	    ** values being represented in the link-back operation */
	    OPL_BMOJ	    allids;
	    OPV_BMVARS	    *agscope;
	    OPL_BMOJ	    *ojmap4;    /* map of outer joins contained in scope
				    ** of this current outer join being analyzed */
	    if (ojid4 == maxoj)
	    {	/* special case main query */
		agscope = subquery->ops_oj.opl_agscope;
		if (!agscope)
		    continue;
		ojmap4 = &allids;
		MEfill(sizeof(allids), (u_char)0, (char *)&allids);
		BTnot((i4)maxoj, (char *)&allids);
	    }
	    else
	    {
		OPL_OUTER	    *ojid4p;

		ojid4p = lbase->opl_ojt[ojid4];
		/* mark any single variable inner joins created for full join 
		** support */
		if (ojid4p->opl_type == OPL_INNERJOIN)
		{   /* if only one explicitly referenced inner relation exists
		    ** then this inner join was created to support the parent
		    ** full join, this requires special case handling at
		    ** orig nodes */
		    OPV_IVARS	    ijvarno;
		    OPV_IVARS	    ijvarno1;
		    subquery->ops_mask |= OPS_IJCHECK; /* since inner joins, invoke
						       ** special boolean handling */
		    ijvarno = BTnext((i4)-1, (char *)ojid4p->opl_ivmap, (i4)maxvars);
		    ijvarno1 = BTnext((i4)ijvarno, (char *)ojid4p->opl_ivmap, (i4)maxvars);
		    if ((ijvarno >= 0)
			&&
			(   (ijvarno1 < 0) 
			    || 
			    (ijvarno1 >= subquery->ops_vars.opv_prv)
			))
		    {
			ojid4p->opl_mask |= OPL_SINGLEVAR;
/*
			if (vbase->opv_rt[ijvarno]->opv_svojid != OPL_NOOUTER)
			    opx_error(E_OP03A6_OUTER_JOIN);
			vbase->opv_rt[ijvarno]->opv_svojid = ojid4;
*/
		    }
		}		
		agscope = ojid4p->opl_agscope;
		if (!agscope)
		    continue;
		ojmap4 = ojid4p->opl_idmap;
	    }
	    {
		OPL_IOUTER	ojid5;
		for (ojid5= -1; (ojid5=BTnext((i4)ojid5, (char *)ojmap4, (i4)maxoj)) >=0;)
		{
		    OPL_OUTER	    *ojid5p;
		    OPL_OJTYPE	    ojtype5;
		    ojid5p = lbase->opl_ojt[ojid5];
		    ojtype5 = ojid5p->opl_type;
		    if ((ojtype5 == OPL_LEFTJOIN) || (ojtype5 == OPL_FULLJOIN))
		    {	/* an inner to a left or full join could cause a NULL */
			OPV_BMVARS	temp;
			MEcopy((PTR)ojid5p->opl_ojtotal, sizeof(temp),
			    (PTR)&temp);
			BTand((i4)maxvars, (char *)agscope, (char *)&temp);
			if (BTnext((i4)-1, (char *)&temp, (i4)maxvars) >= 0)
			{
			    opx_vrecover( E_DB_ERROR, E_OP000A_RETRY, 0); /* 
						** report an error to SCF to
						** cause a retry of the query
						** without flattening */
       
			}
		    }
		}
	    }
	}
    }
#if 0
    {	/* make another traversal of the qualification and add
	** references to the null indicator variable where it is
	** referenced outside the scope of the joinid which set it,
	** this will create an "AND" below a "OR" so it needs to
	** be done before normalization */
	(VOID)opl_ojnull(subquery, subquery->ops_root->pst_right);
    }
#endif
    if ( (subquery->ops_global->ops_cb->ops_check &&
	 opt_strace(subquery->ops_global->ops_cb, OPT_F029_HISTMAP)))
	    opt_ojstuff(subquery);   /* display oj table            */
}

/*{
** Name: opl_fojeqc	- find outer join equivalence classes
**
** Description:
**      There are special cases to check when an equality which can cause 
**      an equivalence class, is referenced outside the scope of an outer join
**	of which it is an inner.  This routine will detect these special cases
**	and cause the boolean factors to be saved so that the proper NULL evaluation can
**	occur. 
**	Examples of what this routine is trying to catch as follows
**    drop t1\p\g
**    create table t1(a1 integer, a2 integer, a3 integer)\p\g
**    insert into t1(a1,a2,a3) values (1,1,1)\p\g
**    drop t2\p\g
**    create table t2(a1 integer, a2 integer, a3 integer)\p\g
**    insert into t2(a1,a2,a3) values (1,0,1)\p\g
**    insert into t2(a1,a2,a3) values (1,0,1)\p\g
**    drop t3\p\g
**    create table t3(a1 integer, a2 integer, a3 integer)\p\g
**    insert into t3(a1,a2,a3) values (1,1,1)\p\g
**
**	on the following query opl_fojeqc should return FALSE for t1.a2=t2.a2
**    select t1.a1, t2.a1, t3.a1 from (t1 left join t2 on t1.a1=t2.a1) 
**	    left join t3 on t1.a2=t2.a2 and t1.a2=t3.a2\p\g
**
**	on the following query *savequal should be TRUE since t2.a2 is on the inner
**    select t1.a1, t2.a1, t3.a1 from (t1 left join t2 on t1.a2=t2.a2) left join t3 on t2.a2=t3.a2\p\g
**      FIXME - the above case should not occur since strength reduction should eliminate
**	the left join between t1 and t2
**
**   In another simliar example,
**	select t1.a1, t2.a1, t3.a1 from (t1 left join t2 on t1.a1=t2.a1)
**          left join t3 on t2.a3=t3.a3 and t1.a2=t3.a2\p\g
**	Even if strength reduction occurs, care must be made to ensure all ON clause
**	qualifications are evaluated before the outer join is evaluated even those
**	which are contained in equivalence classes.  This is why a map of equivalence
**	classes which originated from the an equi-join in which one relation was
**	on the inner set of relations and one relation was in the outer set of relations
**	is created for relation placement, to ensure all joining equivalence classes
**	exist at the outer join node.  An alternative would be a more complex use
**	of iftrue(TID, <special eqc product>) for each reference to an inner attribute
**	in the target list, and a sort to remove duplicates.  This would allow a
**	tree shape of (t1 join t3) join t2 in the above example, whereas the simplier
**	eqc map placement would disallow this tree shape.
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      lvarmap                         ptr to map of variables on the left 
**					side of the equality
**      rvarmap                         ptr to map of variables on the right
**					side of the equality
**	ojid				outer join ID of equality
**
** Outputs:
**
**	Returns:
**	    TRUE - if equivalence class should be generated as normal
**	    FALSE - if this qualification cannot be used to generate an
**	    equivalence class and should be used as an ordinary boolean factor
**	Exceptions:
**
** Side Effects:
**
** History:
**      31-jan-90 (seputis)
**          initial creation
[@history_template@]...
*/
bool
opl_fojeqc(
	OPS_SUBQUERY       *subquery,
	OPV_GBMVARS        *lvarmap,
	OPV_GBMVARS        *rvarmap,
	OPL_IOUTER	   ojid)
	/* bool		   *savequal; */
{
    OPL_OJT	    *lbase;	/* base of ptrs to outer join descriptors */
    OPL_OUTER	    *ojp;	/* ptr to outer join descriptor being
				** analyzed */
    OPV_IVARS	    maxvars;	/* number of range variables defined in
				** the query */
    OPV_GBMVARS	    bvarmap;	/* map of variable in the conjunct */
    OPL_BMOJ	    *idmap;
    OPL_IOUTER	    maxoj;	/* number of outer joins defined */

/*    *savequal = FALSE; */
    maxvars = subquery->ops_vars.opv_prv;
    lbase = subquery->ops_oj.opl_base;
    ojp = lbase->opl_ojt[ojid];
    maxoj = subquery->ops_oj.opl_lv;
    idmap = ojp->opl_idmap; /* map of outer join IDs
				** contained in the scope of this outer
				** join ID */
    MEcopy((PTR)lvarmap, sizeof(*lvarmap), (PTR)&bvarmap);
    BTor((i4)(BITS_IN(bvarmap)),(char *)rvarmap, (char *)&bvarmap);
    switch (ojp->opl_type)
    {

    case OPL_FULLJOIN:
    {	/* for a full join the equivalence class can only be evaluated
	** at the full join node, so check that the varmaps are subsets
	** of the partitions created by the full join */
	if ((	!BTsubset((char *)lvarmap, (char *)ojp->opl_p1, (i4)maxvars)
		||
		!BTsubset((char *)rvarmap, (char *)ojp->opl_p2, (i4)maxvars)
	    )
	    &&
	    (	!BTsubset((char *)rvarmap, (char *)ojp->opl_p1, (i4)maxvars)
		||
		!BTsubset((char *)lvarmap, (char *)ojp->opl_p2, (i4)maxvars)
	    ))
	    return(FALSE);
	break;
    }
    case OPL_LEFTJOIN:
    {
	OPV_GBMVARS	ojvmap;	/* map of variables in child outer joins */
	bool		first_time;
	OPL_IOUTER	ojid2;	/* an outer join ID to be analyzed */
	bool		normal;	/* set TRUE if normal equivalence class
				    ** handling can be made */
	normal = FALSE;
	first_time = TRUE;
	for (ojid2 = -1; (ojid2 = BTnext((i4)ojid2, (char *)idmap,
	    (i4)maxoj))>= 0;)
	{
	    OPL_OUTER	    *childp;

	    childp = lbase->opl_ojt[ojid2];
	    if (first_time)
	    {   /* place this check inside loop, loop will rarely get executed
		** even once */
		if (BTsubset((char *)&bvarmap, (char *)ojp->opl_ovmap,
		    (i4)maxvars))
		    first_time = FALSE;
		else
		    normal = TRUE;	/* since one of the relations is on the
				    ** inner, the equivalence class will be
				    ** evaluated at this node */
		MEcopy((PTR)childp->opl_bvmap, sizeof(ojvmap),
		    (PTR)&ojvmap);
	    }
	    else
		BTor((i4)maxvars, (char *)childp->opl_bvmap,
		    (char *)&ojvmap);
	    if (!normal
		&&
		BTsubset((char *)&bvarmap, (char *)&ojvmap, (i4)maxvars))
		return(FALSE);  /* all variables are referenced in child
				** outer joins so that the conjunct will
				** be referenced lower if equivalence classes
				** are used and possibly eliminate outer tuples
				** which should not happen */
	}
    }
    case OPL_FOLDEDJOIN:
    case OPL_INNERJOIN:
	return(TRUE);
    default:
	opx_error(E_OP039A_OJ_TYPE);
    }
#if 0
/* strength reduction should take care of this */
    {
        OPL_IOUTER	    ojid3;	/* an outer join ID to be analyzed */
	for (ojid3 = -1; (ojid3 = BTnext((i4)ojid3, (char *)idmap,
	    (i4)maxoj))>= 0;)
	{   /* set savequal if NULL tests need to be made */
	    OPL_OUTER	*child2p;

	    child2p = lbase->opl_ojt[ojid3];
	    if ((child2p->opl_type == OPL_LEFTJOIN) || (child2p->opl_type == OPL_FULLJOIN))
	    {   /* check only for non-inner joins which can create the special
		** situation requiring a special equivalence class check */
		OPV_GBMVARS		child_inner;
		MEcopy((PTR)&bvarmap, sizeof(child_inner), (PTR)&child_inner);
		BTand((i4)maxvars, (char *)child2p->opl_ivmap, (char *)&child_inner);
		if (BTnext((i4)-1, (char *)&child_inner, (i4)maxvars)>= 0)
		    *savequal=TRUE;    /* a special equivalence class check is required
				** so keep this qualification around to eliminate NULL
				** values */
	    }
	}
    }
#endif
    return(TRUE);		/* all checks passed so deal with this
				** conjunct as a normal equality */
}

/*{
** Name: opl_ojtype	- determine outer join type
**
** Description:
**      given the left and right variable maps at a join node,
**	this routine calculate the type of join 
**
** Inputs:
**      suqbuery                        ptr to subquery being analyze
**      ojid                            joinid of this node
**      leftmap                         left varmap
**      rightmap                        right varmap
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
**      25-feb-90 (seputis)
**          initial creation
[@history_template@]...
*/
OPL_OJTYPE
opl_ojtype(
	OPS_SUBQUERY       *subquery,
	OPL_IOUTER         ojid,
	OPV_BMVARS         *lvarmap,
	OPV_BMVARS	   *rvarmap)
{
    OPL_OUTER	    *ojp;
    if (!subquery->ops_oj.opl_base || (ojid == OPL_NOOUTER))
	return(OPL_INNERJOIN);
    ojp = subquery->ops_oj.opl_base->opl_ojt[ojid];
    switch (ojp->opl_type)
    {
    case OPL_LEFTJOIN:
    {
	if (BTsubset((char *)rvarmap, (char *)ojp->opl_maxojmap, (i4)BITS_IN(*ojp->opl_maxojmap)))
	    return(OPL_LEFTJOIN);
	else if (BTsubset((char *)lvarmap, (char *)ojp->opl_maxojmap, (i4)BITS_IN(*ojp->opl_maxojmap)))
	    return(OPL_RIGHTJOIN);
	else 
	    opx_error(E_OP039B_OJ_LEFTRIGHT);
    }
    case OPL_FULLJOIN:
	return(OPL_FULLJOIN);
    default:
	return(OPL_INNERJOIN);
    }
}

/*{
** Name: opl_histogram	- adjust the histogram for the outer join
**
** Description:
**	    -is called by opn_nodecost ... e.g. using the following query
**	select r.a from (r left join s on r.a=s.a and (r.b>5 or s.c=6))
**		where  s IS NULL or r.b < 10
**	   - this routine attempts to use existing routines oph_jselect and
**	oph_relselect in various combinations in order to obtain the estimate
**	for the outer join.  It is only called when an other join needs to be
**	evaluated at this join node.  There are 2 basic modes of operation 
**	for opl_histogram.  
**	1) The first "fast_path" mode is used only when equi-joins 
**	exist in the on clause.  In this case the outer join cell 
**	by cell estimates are made directly and only oph_jselect is applied.  
**	If  (r left join s on r.a=s.a) was the outer join then the fast path 
**	would be used but since (r.b>5 or s.c=6) exists in the ON clause, 
**	the fast path cannot be used.
**	2) The second mode is used when non equi-join qualifications exist in the
**	ON clause.  In this case the inner join estimate is made by calling
**	oph_jselect(to evaluate r.a=s.a) followed by oph_relselect (to evaluate
**	(r.b>5 or s.c=6)).  The result of each outer equi-join histogram (e.g. histogram
**	on r.a) is then reexamined before and after to obtain NULL counts
**	and modify the histogram.  
**	In both modes, the null counts of non-equijoin histograms are then
**	modified given the null count estimates from the outer join.
**	After returning to opn_nodecost, oph_relselect is called again to 
**	apply the qualifications from non-ON clause qualifications 
**	(e.g. s IS NULL or r.b < 10) to be applied at the join node.
**
**
**
**	Known problems with outer join histogram processing
**
**	1) Since a histogram is kept on an equivalence class, only the
**	histogram of a coalesced value is calculated and used in all
**	situations if either the inner or outer attribute is referenced.
**	E.g. in the above query, the histogram of r.a and s.a would be 
**	different but since only one histogram is maintained, the outer
**	attribute r.a is used.  This can lead to incorrect estimates in
**	cases where large NULL counts are expected and the s.a attribute
**	is used outside the scope of the outer join e.g. s IS NULL or r.b < 10.
**	2)The current implementation of histogram processing requires either
**	the left histogram or right histogram cell intervals to be used
**	for producing a join histogram result.  In the case of left joins
**	we use the left cell intervals, and visa versa for right joins.
**	In the case of full joins we need to choose one or the other.  This
**	is a problem since if the "losing" histogram contains a range which
**	is not represented by the "winning" histogram intervals then the
**	full join cannot be represented.  This can be corrected if the
**	cell splitting for join histogram project is implemented because
**	then the full domain of an attribute can be represented.
**	e.g. if select * from (r full join s on r.a=s.a)... if r.a
**	contains only integers <= 100 and s.a contains integers >= 100
**	then the intersection is 100, but the full join histogram of
**	the coalesced value cannot be described totally by either
**	the cell intervals of the histogram for r.a, or s.a.
**	3) Normally a qualification is placed as low as possible in
**	the join tree to restrict tuples.  However, there are some
**	cases in which a boolean factor needs to be evaluated at 
**	two or more points between the leaf and root of a join tree.
**	The current histogram processing only evaluates the qualification
**	at the lowest legal point in the join tree.
**	e.g. select r.a from (r left join s on r.a=s.a) 
**		 where r.a = 5 or s.a > 7
**	It would be desirable to evaluate the where clause at the project-
**	restrict of r , as well as s, but also it is necessary to evaluate
**	it again after the outer join is computed.  So in this case the
**	where clause is evaluated at all nodes of the query plan.  The
**	histogram proceeds as though the boolean factor was evaluated only
**	in the project restrict.
**
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	np				ptr to join tree node 
**	lsubtp				ptr to left subtree descriptor
**	leqclp				ptr to left equivalence class descriptor
**	lrlmp				ptr to left relation descriptor
**	rsubtp				ptr to right subtree descriptor
**	reqclp				ptr to right equivalence class descriptor
**	rrlmp				ptr to right relation descriptor
**	jordering			equi-joins available at this node
**	jeqh				ptr to map of equivalence classes for
**					which histograms are required for this
**					join node and above
**	bfm				ptr to map of  boolean factors 
**					evaluated at this node
** Outputs:
**
**	Returns:
**	    TRUE if outer join has been processed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-feb-90 (seputis)
**          initial creation
**	6-june-97 (inkdo01 for hayke02)
**	    Only add in the tuples contributed by NULLs if this is a full
**	    outer join. This change fixes bug 82781.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Changes to support new RLS/EQS/SUBTREE layout (new parm). This
**	    change fixes bug 103858.
**	1-Sep-2005 (schka24)
**	    Pass by-list eqc map through to jselect.
**	25-oct-05 (inkdo01)
**	    Improve null inner count estimate.
**	11-oct-2007 (dougi)
**	    Neglected to init opl_douters for above change - gets periodic OP0082.
*/
bool
opl_histogram(
	OPS_SUBQUERY        *subquery,
	OPN_JTREE	    *np,
	OPN_SUBTREE	    *lsubtp,
	OPN_EQS		    *leqclp,
	OPN_RLS		    *lrlmp,
	OPN_SUBTREE	    *rsubtp,
	OPN_EQS		    *reqclp,
	OPN_RLS		    *rrlmp,
	OPO_ISORT	    jordering,
	OPO_ISORT	    jxordering,
	OPE_BMEQCLS	    *jeqh,
	OPE_BMEQCLS	    *byeqcmap,
	OPB_BMBF	    *bfm,
	OPE_BMEQCLS	    *eqh,
	OPN_RLS 	    **rlmp_resultpp,
	OPN_JEQCOUNT        jxcnt)
{
    OPL_OJHISTOGRAM     ojhist;
    OPN_RLS		*jrlmp = (OPN_RLS *) NULL;
    OPE_IEQCLS		maxeqcls;
    bool		at_root;
    OPB_BMBF            ojbfm;	    /* subset of bfm with ON clause
                                    ** boolean factors removed */
    OPE_BMEQCLS		*join_map;  /* if jordering is multi-attribute then
				    ** this is a map of the equivalence
				    ** classes */
    OPZ_AT		*abase;	/* base of joinop attribute array */
    OPB_IBF		maxbf;
    OPN_PERCENT		sel = 1.0;

    at_root = (subquery->ops_global->ops_estate.opn_rootflg
        &&
        (   !subquery->ops_global->ops_estate.opn_singlevar
            ||
            (subquery->ops_vars.opv_rv == 1)
        ));			    /* TRUE - if histograms are not needed, just
				    ** tuple count, FIXME check for knowledge management */

    ojhist.opl_ojmask = 0;
    ojhist.opl_douters = 0.0;
    ojhist.opl_outerp = subquery->ops_oj.opl_base->opl_ojt[np->opn_ojid];
    ojhist.opl_ojtype = opl_ojtype(subquery, np->opn_ojid, &lrlmp->opn_relmap, &rrlmp->opn_relmap);
    if ((ojhist.opl_ojtype == OPL_INNERJOIN)
	||
	(   (ojhist.opl_ojtype == OPL_FULLJOIN)
	    &&
	    !(np->opn_jmask & OPN_FULLOJ)
	    /* case of TID join occurring after FULL JOIN which should
	    ** be treated will normal TID processing estimates,...
	    ** also oj splitting across join nodes cannot occur
	    ** for full joins so only one node can process boolean
	    ** factors even after filtering is implemented */
	))
	return(FALSE);
    ojhist.opl_ojid = np->opn_ojid;
    abase = subquery->ops_attrs.opz_base;
    maxeqcls = subquery->ops_eclass.ope_ev;
    maxbf = subquery->ops_bfs.opb_bv;

    if (ojhist.opl_outerp->opl_mask & OPL_BFEXISTS)
    {	/* a non-equi join boolean factor exists so determine if it is
	** evaluated at this node, in which case, special ON clause 
	** processing is needed to obtain better NULL estimates,
	** otherwise just use the fast path equi-join estimates for 
	** the outer join */
        OPB_IBF             bfi;    /* current boolean factor being
                                    ** analyzed */
	bool		    found_ojbf; /* TRUE if outer join boolean 
				    ** factor found */
	OPB_BFT             *bfbase;/* ptr to base of array of ptrs to
				    ** boolean factor elements */

	bfbase = subquery->ops_bfs.opb_base;/* ptr to base of array of ptrs to 
				    ** boolean factors */
	found_ojbf = FALSE;
        MEfill(sizeof(ojbfm), (u_char)0, (PTR)&ojbfm);
        for (bfi = -1; (bfi = BTnext((i4)bfi, (char *)bfm, (i4)maxbf)) >= 0;)
        {   /* evaluate the selectivity for each boolean factor */
            if (bfbase->opb_boolfact[bfi]->opb_ojid == ojhist.opl_ojid)
	    {
		found_ojbf = TRUE;
                BTset((i4)bfi, (char *)&ojbfm); /* set ON clause boolean
				    ** factors evaluated at this node */
	    }
        }
	if (found_ojbf)
	    /* more expensive ON clause processing is required */
	    ojhist.opl_ojmask |= OPL_OJONCLAUSE; /* estimate NULLs introduced by
				    ** equi-joins but do not adjust histograms
				    ** and instead leave them as inner join
				    ** histograms, to be adjusted later */
    }

    ojhist.tlnulls = 0.0;	    /* init estimate of NULLs due to outer join
				    ** semantics for left relation */
    ojhist.trnulls = 0.0;	    /* init estimate of NULLs due to outer join
				    ** semantics for right relation */
    oph_jselect (subquery, np, &jrlmp, lsubtp, leqclp, lrlmp, rsubtp, 
	reqclp, rrlmp, jordering, jxordering, jeqh, byeqcmap, &ojhist);
				    /* obtain the histograms after applying
				    ** outer join semantics for equi-joins 
				    ** - if the OPL_OJONCLAUSE mask is set
				    ** since histograms will already have
				    ** NULL adjusts, otherwise histograms
				    ** will represent inner join distribution
				    ** and only tlnulls and trnulls will contain
				    ** estimates of NULL tuple counts
				    */
    if (jordering >= maxeqcls)
	join_map = subquery->ops_msort.opo_base->opo_stable
	    [jordering-maxeqcls]->opo_bmeqcls;
    else
	join_map = (OPE_BMEQCLS *)NULL;

    if (ojhist.opl_ojmask & OPL_OJONCLAUSE)
    {	/* since boolean factors exist, calculate those histograms
	** required by the parent, as well as required by boolean
	** factors selected by oph_relselect */
	OPE_BMEQCLS		eqcmap;	    /* map of equivalence classes
				    ** which are required to compute
				    ** the outer join  */
	MEcopy((PTR)jeqh, sizeof(eqcmap), (PTR)&eqcmap);
	if (join_map)
	    BTor((i4)maxeqcls, (char *)join_map, (char *)&eqcmap);
	else if (jordering >= 0)
	    BTset((i4)jordering, (char *)&eqcmap);
	/* calculate the ON clause histograms, i.e. calculate all
	** outer joins as inner joins and only ON clause qualifications
	** for the specific outer join ID assigned to this node, and
	** avoid applying boolean factors from the where clause or
	** other outer join ON clauses */
	{
	    bool		selapplied;

	    oph_relselect (subquery, &jrlmp, jrlmp, &ojbfm, &eqcmap, 
		&selapplied, &sel, jordering, np->opn_nleaves, jxcnt, OPV_NOVAR);
				    /* return jrlmp if at root or
				    ** selectivity of 1.  in this
				    ** case *rlmp->opn_ereltups is not
				    ** correct if selectivity was
				    ** not 1. therefore in
				    ** nodecommon calculate correct
				    ** size of new rel, put in
				    ** "blocks" 
				    */
	    if (!selapplied)
	    {	/* make sure tuple estimate does not go below left or right
		** join outer table */
		jrlmp->opn_reltups *= sel; /* if we are at the root then
					** trl->opn_reltups is that of
					** original relation so
					** multiply by selectivity
					** before calculating number of
					** blocks in new relation */
		if ((ojhist.opl_ojtype == OPL_LEFTJOIN)
		    ||
		    (ojhist.opl_ojtype == OPL_FULLJOIN))
		    if (jrlmp->opn_reltups < lrlmp->opn_reltups)
			jrlmp->opn_reltups = lrlmp->opn_reltups;
		if ((ojhist.opl_ojtype == OPL_RIGHTJOIN)
		    ||
		    (ojhist.opl_ojtype == OPL_FULLJOIN))
		    if (jrlmp->opn_reltups < rrlmp->opn_reltups)
			jrlmp->opn_reltups = rrlmp->opn_reltups;
		
	    }
	    if (jrlmp->opn_reltups < 1.0)
		jrlmp->opn_reltups = 1.0; /* minimum of one tuple */
	}
	BTxor((i4)maxbf, (char *)&ojbfm,(char *)bfm); /* reset ON clause 
				    ** boolean factors for this joinid,
				    ** so that upon returning only where
				    ** clause boolean factors will be
				    ** applied */

	{   /* combine the appropriate outer histogram prior to the join, to the
	    ** inner join histogram and adjust cells counts if outer join histogram
	    ** has been decreased; keep track of adjustments in order to process
	    ** NULL adjustments for inner histograms,... the oph_jselect processing
	    ** had produced an estimate for nulls produced in the equi-join, but
	    ** since boolean factors have been applied, this following step will
	    ** determine if any added NULLs have been introduced by outer joins */
	    OPH_HISTOGRAM	    *on_histp; /* ptr to histogram produced in
					** ON clause evaluation */
	    OPO_TUPLES		    on_tuples; /* number of tuples estimated
					** due to ON clause restrictions */
	    on_tuples = jrlmp->opn_reltups;
	    for (on_histp = jrlmp->opn_histogram; on_histp; 
		on_histp = on_histp->oph_next)
	    {
		OPE_IEQCLS	eqcls;
		OPZ_ATTS	*attrp;
		OPO_TUPLES	total_tups; /* estimate of total tuples
				    ** for outer relation */
		i4		ncell; /* current cell being analyzed */
		OPO_TUPLES	orig_tuples; /* original number of tuples
				    ** prior to join node for this attribute */
		OPO_TUPLES	on_count; /* tuple count for cell in histogram
				    ** in which ON clause was applied */
		OPO_TUPLES	orig_count; /* tuple count for cell in histogram
				    ** before outer join was applied */
		OPN_RLS	        *child_rlmp; /* ptr to descriptor containing
				    ** histogram used to create this
				    ** element */
		OPO_TUPLES	null_count; /* total estimated null based
					    ** on reduced tuple count on a cell by
					    ** cell basis */
		OPO_TUPLES	*maxnullp;  /* ptr to number of tuples
				    ** contributed by outer join */
		OPH_HISTOGRAM	*orig_histp; /* histogram from child */

		attrp = abase->opz_attnums[on_histp->oph_attribute];
		eqcls = attrp->opz_equcls;

		if ((join_map && BTtest((i4)eqcls, (char *)join_map))
		    ||
		    (eqcls == jordering))
		{   /* this equivalence class is a joining equivalence class 
		    ** so need to determine which histogram was chosen from
		    ** the left or right subtree in order to do the cell by
		    ** cell match */
		    OPN_RLS	    *other_rlmp;
		    OPH_HISTOGRAM   *other_histp; /* consistency check */
		    if (on_histp->oph_mask & OPH_OJLEFTHIST)
		    {	/* setting of maxnullp may seem redundant to
			** below, but is necessary for full joins here */
			child_rlmp = lrlmp; /* tuples preserved on right
				    ** side of join */
			other_rlmp = rrlmp;
			maxnullp = &ojhist.trnulls; /* null count for 
				    ** right side */
		    }
		    else
		    {
			child_rlmp = rrlmp; /* tuples preserved on right
				    ** side of join */
			other_rlmp = lrlmp;
			maxnullp = &ojhist.tlnulls; /* null count for 
				    ** left side */
		    }
		    other_histp = *opn_eqtamp (subquery, &other_rlmp->opn_histogram, 
			eqcls, TRUE);
		    if (on_histp->oph_fcnt == other_histp->oph_fcnt)
			opx_error(E_OP04B3_HISTOUTERJOIN);    /* check that 
					    ** correct histogram was
					    ** obtained, and that no modification
					    ** of underlying active histograms in 
					    ** subtrees are possible */

		    orig_tuples = child_rlmp->opn_reltups;
		    /* note that for a FULL JOIN tuples are lost from one
		    ** side, this is an arch limitation which could be
		    ** fixed by implementing cell splitting for histograms */
		    /* - distribution can be obtained from opposite side
		    ** if default histograms are used, but tuple count
		    ** should be determined by the side which preserves
		    ** tuples, ... for FULL joins since both sides preserve
		    ** tuples, the tuple count can be chosen from the
		    ** side which has the correct interval info */
		    if (ojhist.opl_ojtype == OPL_RIGHTJOIN)
		    {
			orig_tuples = rrlmp->opn_reltups;
                        maxnullp = &ojhist.tlnulls; /* null count for
                                    ** left side */
		    }
		    else if (ojhist.opl_ojtype == OPL_LEFTJOIN)
		    {
			orig_tuples = lrlmp->opn_reltups;
                        maxnullp = &ojhist.trnulls; /* null count for
                                    ** left side */
		    }
		}
		else 
		{   /* look thru each histogram which did not participate in
		    ** an equi-join, and determine if the attribute was an outer
		    ** attribute, i.e. preserved tuples which did not match.
		    ** If this was the case and the original tuple count for the
		    ** cell is larger than the new tuple count for the cell after
		    ** the ON clause had been applied, then some NULLs should be
		    ** added for the inner relation tuples
		    ** - full joins may cause histograms to be associated with inner
		    ** or outer */

		    if (BTtest((i4)eqcls, (char *)&leqclp->opn_maps.opo_eqcmap))
		    {   /* equivalence class comes from left side only */
			if (ojhist.opl_ojtype == OPL_RIGHTJOIN)
			    /* this is a right join so NULLs will be placed
			    ** into this attribute if the inner join fails */
			    continue;	    /* look only at attributes which
					    ** can preserve tuples */
			/* for FULL or LEFT join, need to preserve original
			** tuple count per cell */
			child_rlmp = lrlmp;
			maxnullp = &ojhist.tlnulls;
		    }
		    else if (BTtest((i4)eqcls, (char *)&reqclp->opn_maps.opo_eqcmap))
		    {   /* equivalence class comes from right side only */
			if (ojhist.opl_ojtype == OPL_LEFTJOIN)
			    /* this is a left join so NULLs will be placed
			    ** into this attribute if the inner join fails */
			    continue;	    /* look only at attributes which
					    ** can preserve tuples */
			/* for FULL or RIGHT join, need to preserve original
			** tuple count per cell */
			child_rlmp = rrlmp;
			maxnullp = &ojhist.trnulls;
		    }
		    else
		    {
			/* this would be the case of a multi-var function attribute which
			** by definition has variables defined on both sides, and is
			** created at this point, so that NULLs would be introduced
			** - need to look at attributes which can preserve tuples */
			continue;	    /* look only at attributes which
					    ** can preserve tuples */
		    }
		    orig_tuples = child_rlmp->opn_reltups;
		}
		/* compare all cells and make a determination of the effect of the ON
		** clause on this histogram, ... choose the worse case NULL estimate
		** since it is probably caused by a boolean factor applied only to
		** this attribute */
		total_tups = null_count = 0.0;
		orig_histp = *opn_eqtamp (subquery, &child_rlmp->opn_histogram, 
		    eqcls, TRUE);
		if (orig_histp->oph_default)
		    continue;		    /* outer join estimates with default
					    ** histograms are inaccurate and
					    ** should not affect those made
					    ** by real histograms */

		if (on_histp->oph_fcnt == orig_histp->oph_fcnt)
		    continue;		    /* if the distribution for this
					    ** attribute was not affected by
					    ** the boolean factors then there
					    ** is no need to check for nulls */
		if (on_histp->oph_attribute != orig_histp->oph_attribute)
		    opx_error(E_OP04B3_HISTOUTERJOIN); /* expecting attribute and 
					    ** thereby interval information
					    ** to match, so that following
					    ** loop is possible */
		/* at this point the cell count should have been generated
		** at this node and therefore can be modified if necessary,
		** - above checks will prevent histograms belonging to 
		** subtree nodes from being modified */
		for (ncell = attrp->opz_histogram.oph_numcells; --ncell;)
		{
		    on_count = on_histp->oph_fcnt[ncell] * on_tuples;
		    orig_count = orig_histp->oph_fcnt[ncell] * orig_tuples;
		    if (orig_count > on_count)
			null_count += (orig_count - on_count); /* add to estimate
					    ** of mismatched tuples i.e. non-inner
					    ** join tuples for this attribute */
		    else
			orig_count = on_count;
		    on_histp->oph_fcnt[ncell] = orig_count; /* since this attribute
					    ** is outer it should not decrease
					    ** in tuple count, however it can increase
					    ** in tuple count */
		    total_tups += orig_count;
		}
		/* make a calculation for existing NULLs in the attribute */
		on_count = on_histp->oph_null * on_tuples;
		orig_count = orig_histp->oph_null * orig_tuples;
		if (orig_count > on_count)
		{
		    null_count += (orig_count - on_count); /* add to estimate
					** of NULLs for this relation */
		    on_count = orig_count;
		}
		{
		    /* normalize the histogram */
		    total_tups += on_count; /* add null count */
		    if (total_tups < 1.0)
			total_tups = 1.0;
		    for (ncell = attrp->opz_histogram.oph_numcells; --ncell;)
			on_histp->oph_fcnt[ncell] /= total_tups;
		    on_histp->oph_null = on_count/total_tups;
		}
		if (*maxnullp < null_count)
		    *maxnullp = null_count;  /* check if new NULL count is larger
					** in which case use it */
	    }
	}
	{   /* do not apply following to fast path since tuple counts have
	    ** already been estimated using outer join semantics */

	    OPO_TUPLES	totaltups;
	    OPO_TUPLES	cartprod;

	    if (ojhist.opl_ojtype == OPL_FULLJOIN)
		totaltups = jrlmp->opn_reltups + ojhist.trnulls + ojhist.tlnulls;
						/* tuple output is the total tuples given
						** the ON clause restrictions, plus the
						** tuple contributed by the NULLs */
	    else
		totaltups = jrlmp->opn_reltups;
	    cartprod = lrlmp->opn_reltups * rrlmp->opn_reltups;
	    if (totaltups > cartprod)
		totaltups = cartprod;	    /* consistency check - not expected to
						** execute */
	    if (totaltups < 1.0)
		totaltups = 1.0;
	    jrlmp->opn_reltups = totaltups;
	}
    }

    /* traverse the histograms and adjust the NULL estimates for those histograms
    ** on the 'inner' */
    {
	/* can avoid this code at root if no where clause boolean factors exist */
	OPH_HISTOGRAM	*histp;
	OPO_TUPLES	nullcount, matchcount, outercount;

	/* Use "urn" heuristic to estimate null inner rows in OJ. First
	** use urn model to estimate the number of distinct values of the
	** outer join columns have matches in the inners, and by subtraction,
	** the outers that have no match in the inners. Multiplying the
	** proportion of unmatched values to overall values by the number
	** of outer rows yields the estimated number of null inner rows. */

	if (ojhist.opl_ojtype == OPL_LEFTJOIN ||
			ojhist.opl_ojtype == OPL_FULLJOIN)
	    outercount = lrlmp->opn_reltups;
	else if (ojhist.opl_ojtype == OPL_RIGHTJOIN)
	    outercount = rrlmp->opn_reltups;
	else outercount = 0.0;
	outercount *= sel;

	if (ojhist.opl_douters > 0)
	{
	    matchcount = opn_urn(jrlmp->opn_reltups, ojhist.opl_douters);
	    nullcount = outercount * (ojhist.opl_douters - matchcount)
				/ ojhist.opl_douters;
	}
	else nullcount = 0.0;

	if (nullcount < 0.0)
	    nullcount = 0.0;
	else if ((ojhist.opl_ojtype == OPL_LEFTJOIN || 
		ojhist.opl_ojtype == OPL_FULLJOIN) && nullcount > ojhist.trnulls)
	    ojhist.trnulls = nullcount;
	else if ((ojhist.opl_ojtype == OPL_RIGHTJOIN || 
		ojhist.opl_ojtype == OPL_FULLJOIN) && nullcount > ojhist.tlnulls)
	    ojhist.tlnulls = nullcount;

	for (histp = jrlmp->opn_histogram; histp; histp = histp->oph_next)
	{
	    OPE_IEQCLS	    eqcls;
	    OPZ_ATTS	    *attrp;

	    attrp = abase->opz_attnums[histp->oph_attribute];
	    eqcls = attrp->opz_equcls;
	    if ((join_map && BTtest((i4)eqcls, (char *)join_map))
		||
		(eqcls == jordering))
	    {   
		/* the join equivalence class does not have NULL adjustments 
		** - for full joins, wait until cell splitting enhancements
		** for more accurate histogram
		*/
#if 0
		if (BTtest((i4)eqcls, (char *)jeqh))
		{	/* delete histogram if it is not needed */
		}
#endif
	    }
	    else 
	    {
		OPO_TUPLES	    newfactor;
		i4		    ncell;
		OPN_RLS	    *copy_rlmp;

		if (BTtest((i4)eqcls, (char *)&leqclp->opn_maps.opo_eqcmap))
		{
		    /* equivalence class comes from left side only */
		    if (ojhist.trnulls > nullcount)
			nullcount = ojhist.trnulls;
		    copy_rlmp = lrlmp;
		}
		else if (BTtest((i4)eqcls, (char *)&reqclp->opn_maps.opo_eqcmap))
		{
		    /* equivalence class comes from right side only */
		    if (ojhist.tlnulls > nullcount)
			nullcount = ojhist.tlnulls;
		    copy_rlmp = rrlmp;
		}
		else
		    continue;		/* possible function attribute */
		/* traverse the cells and adjust the percentage for the NULL count */
		if (nullcount > 0.0)
		{	/* create copy of histograms since this oph_fcnt has multiple
		    ** pointers which depend on the distribution not changing,
		    ** since the distribution is changing due to NULL handling
		    ** a copy of the counts is necessary */
		    OPH_COUNTS	new_fcnt;
		    OPH_HISTOGRAM   *copy_histp;

		    copy_histp = *opn_eqtamp (subquery, &copy_rlmp->opn_histogram,
			eqcls, TRUE);
		    if (copy_histp->oph_fcnt == histp->oph_fcnt)
			new_fcnt = oph_ccmemory(subquery, &attrp->opz_histogram);
		    else
			new_fcnt = histp->oph_fcnt; /* use current oph_fcnt
					    ** since it was copied earlier for
					    ** some other reason */
		    if (jrlmp->opn_reltups < nullcount)
			nullcount = jrlmp->opn_reltups;
		    newfactor = (jrlmp->opn_reltups - nullcount) / jrlmp->opn_reltups;
		    for (ncell = attrp->opz_histogram.oph_numcells; --ncell;)
			new_fcnt[ncell] = histp->oph_fcnt[ncell] * newfactor;
		    histp->oph_null = ((histp->oph_null * newfactor) 
			+ nullcount) / jrlmp->opn_reltups;
		    histp->oph_fcnt = new_fcnt;
		}
	    }
	}
    }
    *rlmp_resultpp = jrlmp;

    return(TRUE);
}

/*{
** Name: opl_relabel	- relabel special equivalence classes
**
** Description:
**      When secondary indexes are used, the special equivalence 
**      classes for a secondary is different than the primary
**	so that the boolean factor needs to be adjusted. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qnode                           ptr to query tree node to be converted
**      primaryeqc                      special eqcls of primary relation
**      secondaryeqc                    special eqcls of secondary which is available
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
**      29-oct-90 (seputis)
**          initial creation to support tid joins
[@history_template@]...
*/
VOID
opl_relabel(
	OPS_SUBQUERY       *subquery,
	PST_QNODE          *qnode,
	OPV_IVARS	   primary,
	OPE_IEQCLS         primaryeqc,
	OPV_IVARS	   indexvno)
{
    for (;qnode;qnode = qnode->pst_left)
    {
	if (qnode->pst_sym.pst_type == PST_VAR)
	{
	    OPZ_ATTS	    *attrp;
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[qnode->pst_sym.pst_value.
		pst_s_var.pst_atno.db_att_id];
	    if ((   attrp->opz_equcls
		    ==
		    primaryeqc
		)
		&&
		(   primary
		    ==
		    qnode->pst_sym.pst_value.pst_s_var.pst_vno
		)
		)
	    {
		/* found an instance of the special eqcls being used so replace
		** it with the reference to the eqc for the index */
		qnode->pst_sym.pst_value.pst_s_var.pst_vno = indexvno;
		qnode->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
		    subquery->ops_vars.opv_base->opv_rt[indexvno]->opv_ojattr;
	    }
	}
	else if (qnode->pst_right)
	    opl_relabel(subquery, qnode->pst_right, primary, primaryeqc, indexvno);
    }
}
#if 0

/*{
** Name: opl_seqc - special equivalence class test
**
** Description:
**      This routine will create a check for the special equivalence class 
**      which will eliminate  tuples if the equivalence class is zero, i.e. 
**      an earlier outer join failed.  This is used to allow equi-joins to
**	be processed on nested outer joins, which would normally require the
**	iftrue function to be used to check for nulls, but this would preclude
**      using the equi-join for keying so that this check will eliminate
**	those tuples but still allow keying 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varno                           variable number which contains
**					the special equivalence class to
**					be checked
**      ojid                            outer join ID which should be assigned
**					to the special equivalence class
**					check
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
**      17-mar-92 (seputis)
**          initial creation
**      15-apr-94 (ed)
**          b59935 - extra duplicate boolean factor added unnecessarily
**	15-apr-94 (ed)
**	    b59588 - if def routine 
[@history_template@]...
*/
VOID
opl_seqc(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          varno,
	OPL_IOUTER         ojid)
{
    PST_QNODE	    *and_nodep;
    DB_ATT_ID	    ojattr;
    OPS_STATE	    *global;
    /* create a conjunct which tests the special equivalence class
    ** for one, i.e. nulls will cause ON clause to fail */
    global = subquery->ops_global;
    ojattr.db_att_id = subquery->ops_vars.opv_base->opv_rt[varno]->opv_ojattr;
    for (and_nodep =  subquery->ops_seqctest;
	 and_nodep; and_nodep = and_nodep->pst_right)
	if ((and_nodep->pst_left->pst_left->pst_sym.pst_value.pst_s_var.
	    pst_atno.db_att_id == ojattr.db_att_id)
            &&
           (and_nodep->pst_left->pst_sym.pst_value.pst_s_op.pst_joinid == ojid))
           return;            /* test is already in the list */
    and_nodep = opv_opnode(global, PST_AND, (ADI_OP_ID)0, ojid);
    and_nodep->pst_left = opv_opnode(global, PST_BOP, 
	(ADI_OP_ID)global->ops_cb->ops_server->opg_eq, ojid);
    if (ojattr.db_att_id < 0)
	opx_error(E_OP0498_NO_EQCLS_FOUND); /* expecting special equivalence class
				** to be defined */
    and_nodep->pst_left->pst_left = opv_varnode(global, 
	&subquery->ops_attrs.opz_base->opz_attnums[ojattr.db_att_id]->opz_dataval, 
	(OPV_IGVARS)varno, &ojattr);
    and_nodep->pst_left->pst_right = global->ops_goj.opl_one;
    opa_resolve(subquery, and_nodep->pst_left);
    if (!subquery->ops_seqctest)
	subquery->ops_feqctest = and_nodep;  /* save end of list
				    ** to link into qualification */
    and_nodep->pst_right = subquery->ops_seqctest;
    subquery->ops_seqctest = and_nodep;
}
#endif

/*{
** Name: opl_innervar	- place subselect variable in outer join scope
**
** Description:
**      When a view is flatten there may be a subselect introduced 
**      into the scope of an outer join even though outer joins cannot 
**      directly reference subselect in the ON clause.  This routine
**      will make it appear that this newly created SEjoin variable 
**      is in the correct outer join scope 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varno                           SEjoin variable created
**      ojid                            outer join id of boolean factor
**					referencing varno
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates outer join descriptor with references to varno
**
** History:
**      11-feb-94 (ed)
**          initial creation
[@history_template@]...
*/
static VOID
opl_innervar(
    OPS_SUBQUERY	*subquery,
    OPV_IVARS		varno,
    OPL_IOUTER		ojid)
{
    OPL_OUTER	*outerp;
    OPV_VARS	*varp;
    varp = subquery->ops_vars.opv_base->opv_rt[varno];
    outerp = subquery->ops_oj.opl_base->opl_ojt[ojid];
    BTset((i4)varno, (char *)outerp->opl_ojtotal);
    BTset((i4)varno, (char *)outerp->opl_ojbvmap);
    BTset((i4)varno, (char *)outerp->opl_ivmap);
    BTset((i4)varno, (char *)outerp->opl_bvmap);
    BTset((i4)ojid, (char *)varp->opv_ijmap);
}

/*{
** Name: opl_subselect	- place subselect variable in outer join scope
**
** Description:
**      When a view is flatten there may be a subselect introduced 
**      into the scope of an outer join even though outer joins cannot 
**      directly reference subselect in the ON clause.  This routine
**      will make it appear that this newly created SEjoin variable 
**      is in the correct outer join scope 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      varno                           SEjoin variable created
**      ojid                            outer join id of boolean factor
**					referencing varno
**
** Outputs:
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates outer join descriptor with references to varno
**
** History:
**      11-feb-94 (ed)
**          initial creation
[@history_template@]...
*/
VOID
opl_subselect(
    OPS_SUBQUERY	*subquery,
    OPV_IVARS		varno,
    OPL_IOUTER		ojid)
{
    if (ojid >= 0)
    {	/* mark variable as inner in outer join scope */
	OPL_OUTER	*outerp;
	OPV_VARS	*varp;
	OPL_IOUTER	ojid1;
	varp = subquery->ops_vars.opv_base->opv_rt[varno];
	varp->opv_ijmap = (OPL_BMOJ *)opu_memory(subquery->ops_global,
                    (i4)sizeof(*varp->opv_ijmap));
	MEfill(sizeof(*varp->opv_ijmap), (u_char)0,
	    (PTR)varp->opv_ijmap);
	outerp = subquery->ops_oj.opl_base->opl_ojt[ojid];
	for (ojid1 = subquery->ops_oj.opl_lv; --ojid1 >=0;)
	{   /* update outer join maps for nested outer joins */
	    if ((ojid != ojid1)
		&&
		BTsubset((char *)outerp->opl_ivmap,
		    (char *)subquery->ops_oj.opl_base->opl_ojt[ojid1]->
			opl_ivmap, (i4)subquery->ops_vars.opv_rv)
		)
		opl_innervar(subquery, varno, ojid1);
	}
	opl_innervar(subquery, varno, ojid); /* update ojid last since
				** it is needed to identify nested
				** outer joins */
    }
}

/*{
** Name: opl_ijcheck - update innerjoin boolean factor placement maps
**
** Description:
**	An inner join is retained in order to group relations for placement
**	beneath a full join.  This routine updates the boolean factor placement
**	maps since inner joins are not explicitly executed in the co tree
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      linnermap			ptr to ojmap of ojids which have been
**                                      partially or totally evaluated in the
**                                      left subtree
**      rinnermap			ptr to ojmap of ojids which have been
**                                      partially or totally evaluated in the
**                                      right subtree
**      ojinnermap			ptr to ojmap of ojids which have been
**                                      partially or totally evaluated
**      oevalmap                        ptr to ojids which have been totally
**                                      evaluated in the outer co tree
**      ievalmap                        ptr to ojids which have been totally
**                                      evaluated in the inner co tree
**      varmap                          varmap of relations in this subtree
**                                      
**
** Outputs:
**
**	Returns:
**      ojinnermap			ptr to ojmap of ojids which have been
**                                      partially or totally evaluated
**      oevalmap                        ptr to ojids which have been totally
**                                      evaluated in the outer co tree
**      ievalmap                        ptr to ojids which have been totally
**                                      evaluated in the inner co tree
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      11-apr-94 (ed)
**          initial creation - b59937 - incorrect boolean factor placement which
**          inner join ojid is used to group relations
[@history_template@]...
*/
VOID
opl_ijcheck(
    OPS_SUBQUERY	*subquery,
    OPL_BMOJ            *linnermap,
    OPL_BMOJ            *rinnermap,
    OPL_BMOJ            *ojinnermap,
    OPL_BMOJ            *oevalmap,
    OPL_BMOJ            *ievalmap,
    OPV_BMVARS          *varmap)
 {   /* if an inner join ojid exists then it has not been folded
     ** into the parent due to semantic reasons of grouping relations
     ** together for full joins, since the boolean factor placement
     ** for outerjoins depends on ojevalmap and ojinnermap, it is necessary
     ** to add ojids for inner joins b59937 */
     OPL_IOUTER	    	ojid;
     OPV_IVARS	      	maxvar;

     maxvar = subquery->ops_vars.opv_rv;
     for (ojid = subquery->ops_oj.opl_lv; --ojid >=0;)
     {
         OPL_OUTER	    *ojp1;
         ojp1 = subquery->ops_oj.opl_base->opl_ojt[ojid];
         if (ojp1->opl_type == OPL_INNERJOIN)
         {
	     if (BTsubset((char *)varmap, (char *)ojp1->opl_maxojmap, (i4)maxvar))
		 BTset((i4)ojid, (char *)ojinnermap); /*
				    ** only inner relations exist so inner
				    ** join has not been completely evaluated
				    ** yet */
	     else if (BTtest((i4)ojid, (char *)linnermap))
		 BTset((i4)ojid, (char *)oevalmap); /*
				    ** an outer var was introduced
				    ** at this node but an immediate
				    ** child was an inner node so the
				    ** inner join has been completely
				    ** evaluated */
	     else if (BTtest((i4)ojid, (char *)rinnermap))
		 BTset((i4)ojid, (char *)ievalmap); /*
				    ** an outer var was introduced
				    ** at this node but an immediate
				    ** child was an inner node so the
				    ** inner join has been completely
				    ** evaluated */
         }
    }
}

/*{
** Name: opl_sjij - setup boolean factor maps for single variable
**
** Description:
**	An inner join is retained in order to group relations for placement
**	beneath a full join.  This routine updates the boolean factor placement
**	maps since inner joins are not explicitly executed in the co tree
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	varno				variable being analyzed
**                                      
**
** Outputs:
**
**	Returns:
**      ojinnermap			ptr to ojmap of ojids which have been
**                                      partially or totally evaluated
**      ojmap                           if not NULL then a map of ojids which have
**                                      been evaluated in some parent node
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      11-apr-94 (ed)
**          initial creation - b59937 - incorrect boolean factor placement which
**          inner join ojid is used to group relations
[@history_template@]...
*/
VOID
opl_sjij(
      OPS_SUBQUERY	     *subquery,
      OPV_IVARS		     varno,
      OPL_BMOJ		     *innermap,
      OPL_BMOJ		     *ojmap)
{
    OPL_IOUTER	    ojid;

    for (ojid = subquery->ops_oj.opl_lv; --ojid >=0;)
    {
	OPL_OUTER	    *ojp;
	OPL_OJTYPE	    ojtype;
	ojp = subquery->ops_oj.opl_base->opl_ojt[ojid];
	ojtype = ojp->opl_type;
	if ((	(ojtype == OPL_LEFTJOIN)
		||
		(ojtype == OPL_FULLJOIN)
		||
		(ojtype == OPL_INNERJOIN)
	    )
	    &&
	    BTtest((i4)varno, (char *)ojp->opl_minojmap))
	{   /* active outer join descriptor found in which a required
	    ** inner relation exists */
	    if (!ojmap || !BTtest((i4)ojid, (char *)ojmap))
	    {
		if (ojtype == OPL_INNERJOIN)
		    BTset((i4)ojid, (char *)innermap);
#if 0
/* probably not needed since above BTset will achieve the same result */
/* consistency check for outer joins which have not been evaluated */
		if (ojp->opl_mask & OPL_SINGLEVAR)
		{   /* special case of full join with view may have outer
		    ** join defined at single leaf in order to partition
		    ** a full join into left and right relation set, since
		    ** var maps for a full join are identical 
		    ** e.g. select * from (r full join (s inner join t))
		    ** would have r,s,t in both the inner and outer varmap
		    ** for the full join, so an inner join is defined on
		    ** r, and another inner join is defined on {s,t} */
		    if (subquery->ops_vars.opv_base->opv_rt[varno]->opv_svojid
			== ojid)
		    {
			BTset((i4)ojid, (char *)&nodep->opn_ojevalmap);
			BTset((i4)ojid, (char *)ojmap);
			nodep->opn_ojid = ojid;
		    }
		    else
			opx_error(E_OP0287_OJVARIABLE); /* expected 
					** single var inner
					** join variable to be defined */
		}
#endif
#if 0
		else
		    opx_error(9999);	/* expecting outer join to
					** be evaluated in parent node
					** - there are some cart prod cases
					** which this is not necessary
					** - also there are some inner joins
					** in which the join id may disappear
					*/
#endif
	    }
	}
    }
}
