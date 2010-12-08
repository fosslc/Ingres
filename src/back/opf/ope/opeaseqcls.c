/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#define             OPE_ASEQCLS         TRUE
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPEASEQCLS.C - assign equivalence classes
**
**  Description:
**      Routine which will assign equivalence classes to all nodes in subtree
**
**  History:
**      19-jun-86 (seputis)    
**          initial creation from aseqclass.c
**	20-mar-90 (linda,bryanp)
**	    Integrated changes from seputis for non-SQL Gateway support.
**	    Routines added are opd_sjtarget() and ope_qual().
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      25-mar-94 (ed)
**          bug 59355 - an outer join can change the result of a resdom
**          expression from non-null to null, this change needs to be propagated
**          for union view target lists to all parent query references in var nodes
**          or else OPC/QEF will not allocate the null byte and access violations
**          may occur
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static void opl_nullchange(
	OPS_SUBQUERY *subquery,
	PST_QNODE *qnodep);
void ope_aseqcls(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqcmap,
	PST_QNODE *root);
static void opd_sjtarget(
	OPS_SUBQUERY *subquery);
void ope_qual(
	OPS_SUBQUERY *subquery);

/*{
** Name: opl_nullchange	- check for nullability change in var node
**
** Description:
**      outer joins can change non-nullable var nodes to null, so 
**      checks need to be made if underlying temporary relations
**	have been affected by this change 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      qnodep                          ptr to var node to be checked
**
** Outputs:
**      qnodep                          var node updated if necessary
**					to be nullable
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
**      25-mar-94 (ed)
**          initial creation
**	29-apr-94 (ed)
**	    b59355 - make sure target list gets the info on a datatype which
**	    gets changed to nullable.
**	11-jun-01 (hayke02)
**	    We now correctly deal with a function attribute resdom (db_att_id
**	    of -1 -> OPZ_FANUM): resdomp = fattrp->opz_function. This change
**	    fixes bug 104920.
**      14-aug-01 (huazh01)
**          Test varp->opv_grv->opv_gquery for null pointer before calling 
**          function opl_findresdom. This fixes bug 105171
[@history_line@]...
*/
static VOID
opl_nullchange(
	OPS_SUBQUERY	    *subquery,
	PST_QNODE	    *qnodep)
{
    OPZ_ATTS	    *attrp;
    OPV_VARS	    *varp;
    PST_QNODE	    *resdomp;

    if (qnodep->pst_sym.pst_dataval.db_datatype < 0)
	return;			/* already nullable */
    varp = subquery->ops_vars.opv_base->opv_rt[qnodep->pst_sym.
        pst_value.pst_s_var.pst_vno];
    if (!varp->opv_grv
	||
	!(varp->opv_grv->opv_gmask & OPV_NULLCHANGE))
	return;			/* underlying resdom has not
				** changed */
    /* traverse the resdom list of the underlying relation to 
    ** determine if this var node needs to be made nullable */
    attrp = subquery->ops_attrs.opz_base->opz_attnums[qnodep->pst_sym.
	pst_value.pst_s_var.pst_atno.db_att_id];
    if ( (attrp->opz_attnm.db_att_id != OPZ_FANUM) &&
	 ( varp->opv_grv->opv_gquery ) )
	resdomp = opl_findresdom( varp->opv_grv->opv_gquery, 
		(OPZ_IATTS)attrp->opz_attnm.db_att_id);
    else
    {
	OPZ_FT		*fbase;

	fbase = subquery->ops_funcs.opz_fbase;
	if (attrp->opz_func_att != OPZ_NOFUNCATT)
	{
	    OPZ_FATTS	    *fattrp;

	    fattrp = fbase->opz_fatts[attrp->opz_func_att];
	    if (fattrp)
		resdomp = fattrp->opz_function;
	    else
		return;
	}
	else
	    return;
    }
	    
    if (resdomp->pst_sym.pst_dataval.db_datatype < 0)
    {	/* change nullability of var node */
	qnodep->pst_sym.pst_dataval.db_datatype =
	attrp->opz_dataval.db_datatype = resdomp->pst_sym.
	    pst_dataval.db_datatype;
	qnodep->pst_sym.pst_dataval.db_length =
	attrp->opz_dataval.db_length = resdomp->pst_sym.
	    pst_dataval.db_length;
	subquery->ops_mask |= OPS_CLEARNULL;
    }

}

/*{
** Name: ope_aseqcls	- assign the equivalence classes in the query tree
**
** Description:
**      This routine will map the equivalence classes in the query tree.
**      If the equivalence class does not exist then it will be created.
**      If the eqcmap ptr is NULL then the equivalence class map will
**      not be created.
**
** Inputs:
**      subquery                        ptr to subquery be analyzed
**      tree                            ptr to root of query tree to map
**
** Outputs:
**      eqcmap                          ptr to map which will be updated with
**                                      equivalence classes found in subtree
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
**	25-apr-86 (seputis)
**          initial creation
**	9-jan-04 (inkdo01)
**	    Ignore PST_CONSTs - they may anchor long, IN-list predicate 
**	    chains and they're not interesting.
[@history_line@]...
*/
VOID
ope_aseqcls(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS	   *eqcmap,
	PST_QNODE          *root)
{
    if (root->pst_sym.pst_type == PST_CONST)
	return;
    if (root->pst_left)
	ope_aseqcls(subquery, eqcmap, root->pst_left);
    if (root->pst_sym.pst_type == PST_RESDOM)
	subquery->ops_mask &= (~OPS_CLEARNULL); /* clear flag before
					** traversing resdom to detect
					** change in type of underlying
					** var node */
    if (root->pst_right)
	ope_aseqcls(subquery, eqcmap, root->pst_right);

    switch (root->pst_sym.pst_type)
    {
    case PST_VAR:
    {
	OPZ_IATTS	       attno;	/* attribute associated with var node*/
	OPE_IEQCLS             eqcls;	/* equivalence class of attribute */

	attno = root->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	eqcls = subquery->ops_attrs.opz_base->opz_attnums[attno]->opz_equcls;
	if (subquery->ops_global->ops_gmask & OPS_NULLCHANGE)
	    opl_nullchange(subquery, root); /* underlying union views and
					** temporary relations can change the
					** nullability of a var node, and
					** subsequent references should be
					** consistent */
	if (eqcls == OPE_NOEQCLS)
	    eqcls = ope_neweqcls(subquery, attno);/* get new equivalence class
					** if it is unassigned
					*/
	if (eqcmap)
	    BTset((i4) eqcls, (char *)eqcmap);
	break;
    }
    case PST_RESDOM:
    {
	if ((subquery->ops_global->ops_gmask & OPS_NULLCHANGE)
	    &&
	    (subquery->ops_mask & OPS_CLEARNULL))
	{
	    subquery->ops_mask &= (~OPS_CLEARNULL); 
	    opl_nulltype (root);	/* convert parse tree node to
					** nullable */
	}
	break;
    }
    default:
	break;
    }
}

/*{
** Name: opd_sjtarget	- semi-join on target relation
**
** Description:
**      Gateways do not allow tids to be used for updates so that
**	the target relation on updates cannot be moved to another 
**      site in a semi-join operation.  The would occur as follows 
**          table A                 table B
**	    site 1		    site 2 
**	    1000000 tuples          1000000 tuples 
**	    index on x		    index on y 
**        
**      a query like "select * from A,B where A.z = B.y and A.x=5" 
**        
**      could be evaluated by indexing x moving the temp to site 2 
**      doing a key join on y, moving the temp back to site 1 and 
**      doing a TID join to update or delete table A.  If TIDs are 
**      not available as in gateways, one alternative is to move 
**      table B to site 1 and process the query which would be 
**      expensive.  The approach used here is to define a new range 
**      variable on the target relation A, and allow site movement 
**      of any intermediate on site 1 which does not contain table A 
**      but could contain a reference to the new range variable on A. 
**      The new range variable is treated like a secondary index 
**      in that it does not need to be part of the query.  The new 
**      range variable needs to be linked back to the original table 
**      on all equivalence classes which are referenced in the original 
**      table. 
**
**	This routine needs to be run prior to ordering detection but
**	so that the range variable can be initialized to detect the
**	index.
**
** Inputs:
**      subquery                        ptr to subquery on which to
**					add the new range variable
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
**      15-jan-89 (seputis)
**          initial creation, part of TID handling problems for distributed
[@history_template@]...
*/
static VOID
opd_sjtarget(
	OPS_SUBQUERY       *subquery)
{
    OPZ_IATTS	    no_attrs;		/* number of attributes to define
					** for the new range variable */
    OPV_IVARS	    target;		/* target variable number */
    OPV_IVARS	    newrange;		/* new range variable */
    OPZ_IATTS	    maxattr;		/* number of joinop attributes
					** defined for this subquery */
    OPZ_AT	    *abase;		/* ptr to base of joinop attributes array */

    target = subquery->ops_localres;
#ifdef E_OP0A85_UPDATE_VAR
    if (target <= 0)
	opx_error(E_OP0A85_UPDATE_VAR);	/* expecting update variable to be
					** defined */
#endif
    abase = subquery->ops_attrs.opz_base;
    maxattr = subquery->ops_attrs.opz_av;
    no_attrs = 0;
    /* only attributes of the target relation which are referenced in the
    ** qualification are of interest, since these will be used to link back
    ** to the original relation, make sure that enough attributes exist
    ** to include this new range variable */
    {
	OPZ_IATTS	atno;		/* joinop attribute being analyzed */
	for (atno = maxattr; --atno >= 0;)
	{   /* look at each attribute to see if an equivalence class has been
	    ** defined for this attribute, which means it is being used in a
	    ** qualification, need to count these since opv_eqcmp is not initialized
	    ** yet */
	    OPZ_ATTS	    *attrp;

	    attrp = abase->opz_attnums[atno];
	    if ((attrp->opz_equcls != OPE_NOEQCLS)
		&&
		(attrp->opz_varnm == target)
		)
		no_attrs++;		/* this attribute belongs to an equivalence
					** class, so it is part of the qualification
					*/
	}
    }
    /* cannot use this statement since opv_eqcmp not initialized yet 
    ** no_attrs = BTcount ((char *)&varp->opv_eqcmp, (i4)subquery->ops_eclass.ope_ev);
    */
    if (((no_attrs + subquery->ops_attrs.opz_av) > OPZ_MAXATT)
	||
	(!no_attrs))
	return;				/* too many attributes or none in qualification
					** so ignore this part of the search space */
    newrange = opv_relatts( subquery, subquery->ops_result, OPE_NOEQCLS, OPV_NOVAR);
					/* define new local range variable for target
					** relation, FIXME - local OPC may have trouble
					** when multiple local variables point to the
					** same global variable */
    {
	OPZ_IATTS	atno1;		/* joinop attribute being analyzed */
	for (atno1 = maxattr; --atno1 >= 0;)
	{   /* look at each attribute to see if an equivalence class has been
	    ** defined for this attribute, which means it is being used in a
	    ** qualification, need to count these since opv_eqcmp is not initialized
	    ** yet */
	    OPZ_ATTS	    *attrp;

	    attrp = abase->opz_attnums[atno1];
	    if ((attrp->opz_equcls != OPE_NOEQCLS)
		&&
		(attrp->opz_varnm == target)
		)
	    {	/* create a joinop attribute and add this to the same equivalence class
		** as the original target relation */
		DB_ATT_ID	attid;
		attid.db_att_id = opz_addatts( subquery, newrange,
		    attrp->opz_attnm.db_att_id, 
		    &attrp->opz_dataval);
		if (!ope_addeqcls(subquery, attrp->opz_equcls, (OPZ_IATTS)attid.db_att_id,
			TRUE))
		    opx_error(E_OP0A86_EQCLASS);
	    }
	}
    }
    subquery->ops_dist.opd_updtsj = newrange;	/* save the new range variable
					** indicator for this special semi-join
					** situation for distributed gateways */
    return;
}

/*{
** Name: ope_qual	- assign equivalence classes to qualification
**
** Description:
**      This routine will assign equivalence classes to the qualification. 
**      It also assumes that the target list does not have any equivalence 
**      classes assigned, so that a special case situation in distributed 
**      can be handled by defining a new range variable 
**
** Inputs:
**      subquery                        ptr to subquery state variable
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
**      15-jan-89 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
ope_qual(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE	    *global;

    global = subquery->ops_global;
    ope_aseqcls(subquery, (OPE_BMEQCLS *) NULL, subquery->ops_root->pst_right);
					/* assign
                                        ** equivalence classes for vars in
                                        ** the qualification which have not
                                        ** been assigned yet
                                        */
    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	&&
	(global->ops_gdist.opd_gmask & OPD_NOUTID) /* no TIDs are available for link
				    ** back on update (gateway restriction)*/
	&&
	(subquery->ops_sqtype == OPS_MAIN) /* need to consider only on the main
				    ** query */
	)
    	/* no TIDs available for link-back on update so define a new
	** range variable on the base relation to allow semi-joins
	** on target relation to be considered */
	/* static */opd_sjtarget(subquery); /* allow semi-joins on target relation 
					** - this must occur after equivalence
					** class have been assigned to the
					** qualification but before keying has
					** been decided for primary relations
					*/
}
