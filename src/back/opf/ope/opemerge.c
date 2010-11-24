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
#define             OPE_MERGEATTS       TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPEMERGE.C - merge equivalence classes of joinop attributes
**
**  Description:
**      Routines to merge equivalence classes of two joinop attributes
**
**  History:    
**      19-jun-86 (seputis)    
**          initial creation from mergeatts.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static bool ope_eqcmerge(
	OPS_SUBQUERY *subquery,
	OPE_IEQCLS eqc1,
	OPE_IEQCLS eqc2,
	bool nulljoin,
	bool ignore_overlap);
static bool ope_nexpr(
	OPS_SUBQUERY *subquery,
	PST_QNODE *nodep);
static bool ope_aggregate(
	OPS_SUBQUERY *subquery,
	OPZ_ATTS *attrp,
	OPV_GRV *gvarp);
static bool ope_ojcheck(
	OPS_SUBQUERY *subquery,
	OPZ_ATTS *attrp,
	OPE_IEQCLS eqc,
	OPL_IOUTER ojid);
bool ope_mergeatts(
	OPS_SUBQUERY *subquery,
	OPZ_IATTS firstattr,
	OPZ_IATTS secondattr,
	bool nulljoin,
	OPL_IOUTER ojid);

/*{
** Name: ope_eqcmerge	- merge equivalence classes
**
** Description:
**      merge two sets of equivalence classes 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      eqc1                            equivalence class to be merged
**      eqc2                            equivalence class to be merged
**      nulljoin                        TRUE - if merging should not
**					affect NULL joinability
**
** Outputs:
**	Returns:
**	    TRUE - if equivalence classes successfully merged
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jun-86 (seputis)
**          initial creation from eqcmerge in eqcmerge.c
**	19-jun-86 (seputis)
**          add nulljoin parameter
**      29-mar-90 (seputis)
**          unix porting change so that non-byte aligned machines work
**	15-feb-93 (ed)
**	    fix outer join bug 49322
**	15-jan-94 (ed)
**	    remove outer join initialization of obsolete structures
**	8-sep-05 (inkdo01)
**	    Replace eqc1 bits by eqc2 in eqc1 attrs opz_eqcmap's.
**	24-jul-06 (hayke02)
**	    Add new boolean argument ignore_overlap to indicate if the overlap
**	    testing should be ignored. This change fixes bug 116406.
[@history_line@]...
*/
static bool
ope_eqcmerge(
	OPS_SUBQUERY       *subquery,
	OPE_IEQCLS         eqc1,
	OPE_IEQCLS         eqc2,
	bool		   nulljoin,
	bool		   ignore_overlap)
{
    OPE_EQCLIST         *eqc1p;		    /* ptr to equivalence class element
                                            ** associated with eqc1 */
    OPE_EQCLIST         *eqc2p;             /* ptr to equivalence class element
                                            ** associated with eqc2 */
    OPE_ET              *ebase;             /* ptr to base of array of ptrs
                                            ** to equivalence class elements
                                            */
    OPZ_AT              *abase;             /* ptr to base of array of ptrs
                                            ** to joinop attribute elements
                                            */
    OPZ_IATTS           maxattr;            /* number of joinop attributes
                                            ** defined */

    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    maxattr = subquery->ops_attrs.opz_av;   /* number of attributes defined */
    ebase = subquery->ops_eclass.ope_base;  /* ptr to base of array of ptrs
                                            ** to equivalence class elements */
    eqc1p = ebase->ope_eqclist[eqc1];	    /* pointer to first eqclass */
    eqc2p = ebase->ope_eqclist[eqc2];	    /* pointer to second eqclass */

    {	/* check that equivalence classes do not have overlapping varmaps */
	OPV_BMVARS	       varmap;	    /* bitmap of vars in first
                                            ** equivalence class */
	OPZ_IATTS              attr1;       /* attribute in first equivalence
                                            ** class */
	OPZ_IATTS              attr2;	    /* attribute in second equivalence
                                            ** class */

	MEfill(sizeof(varmap), (u_char)0, (PTR)&varmap);
	for (attr1 = -1; 
	     (attr1 = BTnext((i4)attr1, 
			     (char *)&eqc1p->ope_attrmap, 
                             (i4)maxattr)
             ) >= 0;)
	{
	    OPV_IVARS          varno;	    /* varno associated with joinop
                                            ** attribute */

	    varno = abase->opz_attnums[attr1]->opz_varnm;
#ifdef    E_OP0385_EQCLS_MERGE
	    if (BTtest((i4)varno, (char *)&varmap))
		opx_error(E_OP0385_EQCLS_MERGE);
#endif
	    BTset((i4)varno, (char *)&varmap);
	}
	for (attr2 = -1; 
	     (attr2 = BTnext((i4)attr2, 
			     (char *)&eqc2p->ope_attrmap, 
                             (i4)maxattr)
             )>= 0;)
	    if (BTtest((i4)abase->opz_attnums[attr2]->opz_varnm, (char *)&varmap)
		&&
		(ignore_overlap == FALSE))
		return (FALSE);		/* overlap so cannot merge the 
                                        ** equivalence class maps */
    }

    {	/* no overlap so it is safe to merge the equivalence classes */
	OPZ_IATTS              attr1;	/* current attribute of the first
                                        ** equivalence class being analyzed */
	OPZ_ATTS	      *attr1p;
	
	for (attr1 = -1; 
	     (attr1 = BTnext((i4)attr1, 
			     (char *)&eqc1p->ope_attrmap, 
			     (i4)maxattr)
             ) >= 0;)
	{   /* for each set bit in the first eqclass's map */
	    attr1p = abase->opz_attnums[attr1];
	    BTclear((i4)eqc1, (char *)&attr1p->opz_eqcmap);
	    BTset((i4)eqc2, (char *)&attr1p->opz_eqcmap); /* replace eqc1 with
					** eqc2 in multi-eqc bit map */
	    BTset((i4)attr1, (char *)&eqc2p->ope_attrmap); /* set the corresponding
					** bit in the second eqclass's map */
	    attr1p->opz_equcls = eqc2; /* and reset eqclass 
                                        ** info */
	    /* this could be true if an implicit tid from one relation is being
	    ** joined to an implicit tid from another relation (bug 1521)
	    */
	    if (abase->opz_attnums[attr1]->opz_attnm.db_att_id == DB_IMTID)
		subquery->ops_vars.opv_base->opv_rt[abase->opz_attnums
		    [attr1]->opz_varnm]->opv_primary.opv_tid = eqc2;
	}	
    }

    eqc2p->ope_nulljoin = eqc2p->ope_nulljoin && eqc1p->ope_nulljoin && nulljoin;
    if (eqc1p->ope_eqctype == OPE_TID)
	eqc2p->ope_eqctype = OPE_TID;
    /* FIXME - free space management for eqcls */
    ebase->ope_eqclist[eqc1] = NULL;	    /* mark as unused */
    return (TRUE);
}

/*{
** Name: ope_nexpr	- is NULL possible in this expression
**
** Description:
**      This is more important for distributed DB so that NULL joins
**	are avoided which are expensive 
**
** Inputs:
**      subquery                        subquery descriptor of temporary base 
**					relation
**      nodep                           parse tree node
**
** Outputs:
**	Returns:
**	    TRUE - if a NULL could be returned for this attribute
**	Exceptions:
**
** Side Effects:
**
** History:
**      21-dec-88 (seputis)
**	    initial creation
**      8-jul-90 (seputis)
**          check for undefined equivalence class on target list, this means
**	    the that attribute is not found in the qualification, so that the
**	    null characteristics are defined in the attribute itself
[@history_line@]...
[@history_template@]...
*/
static bool
ope_nexpr(
	OPS_SUBQUERY	    *subquery,
	PST_QNODE	    *nodep)
{
    for (;nodep; nodep = nodep->pst_left)
    {
	if (nodep->pst_sym.pst_type == PST_VAR)
	{
	    OPE_IEQCLS	    equcls;
	    equcls = subquery->	ops_attrs.opz_base->opz_attnums[nodep->pst_sym.pst_value.
		    pst_s_var.pst_atno.db_att_id]->opz_equcls;
	    if (equcls >= 0)
		return(subquery->ops_eclass.ope_base->ope_eqclist[equcls]->ope_nulljoin);
	    else
		return(nodep->pst_sym.pst_dataval.db_datatype < 0);
	}
	else if (nodep->pst_sym.pst_type == PST_CONST)
	{
	    if (nodep->pst_sym.pst_dataval.db_datatype < 0)
		return(TRUE);
	}
	if (nodep->pst_right
	    &&
	    ope_nexpr(subquery, nodep->pst_right))
	    return(TRUE);
    }
    return(FALSE);
}

/*{
** Name: ope_aggregate	- look for aggregate by values
**
** Description:
**      This routine will distinguish between implicit links to aggregate
**      temp by values as opposed to explicit links to aggregate expressions
**
** Inputs:
**      subquery                        ptr to subquery being optimized
**      attrp                           ptr to attribute of possible by-value
**      gvarp                           ptr to global range variable descriptor
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    TRUE - if by value found, FALSE otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-oct-88 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
ope_aggregate(
	OPS_SUBQUERY       *subquery,
	OPZ_ATTS           *attrp,
	OPV_GRV            *gvarp)
{
    OPS_SUBQUERY	*tsubquery; /* ptr to subquery representing the
			    ** temporary */
    PST_QNODE		*bylist;

    tsubquery = gvarp->opv_gquery;
    if (!tsubquery)
	opx_error(E_OP0282_FAGG);	/* expecting subquery here */
    bylist = tsubquery->ops_byexpr;
    if (!bylist)
	opx_error(E_OP0282_FAGG);	/* expecting function agg bylist */
    for(;bylist && (bylist->pst_sym.pst_type == PST_RESDOM);
	bylist = bylist->pst_left)
    {
	if (bylist->pst_sym.pst_value.pst_s_rsdm.pst_rsno
	    ==
	    attrp->opz_attnm.db_att_id)
	{
	    if (bylist->pst_sym.pst_dataval.db_datatype > 0)
		return(FALSE);		/* nulls cannot be returned by this
					** attribute so return FALSE */
	    if (BTtest ((i4)attrp->opz_varnm, 
		(char *)&subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_lvrm)
					/* if the aggregate is referenced in the target
					** list then the NULL join needs to be made
					** even if no tuples will exist for the
					** NULL partition in the temporary relation
					** since the NULL partition will be returned
					** to the target list */
		||
		tsubquery->ops_agg.opa_projection) /* also if a projection is made
					** then the NULL partition will still exist
					** so make this a NULL join */
		return(TRUE);		
	    else
		return(ope_nexpr(tsubquery, bylist->pst_right)); /* traverse
					** the expression for the attribute
					** and return TRUE, if any nullable
					** attributes are used in the
					** expression, OR if it can be
					** guaranteed that no tuples will
					** produce a NULL for this resdom */
	}
    }
    return(FALSE);
}

/*{
** Name: ope_ojcheck	- see if this merge involves OJ that requires
**	multiple EQ classes for same attribute
**
** Description:
**      If this eqc involves a column inner to the outer join and one
**	that is outer to the outer join, and the existing EQclass also 
**	involves an inner and an outer attribute of the OJ, return TRUE
**	so that a new EQC can be constructed, even though all the attrs
**	are otherwise in the same EQC. This allows OJ semantics to override 
**	EQC semantics in queries such as "select * from (j1 a inner join j2 b
**	on a.c1 = b.c1) left join j2 b1 on a.c2 = b1.c2 and b.c2 = b1.c2)"
**	where a.c2 and b.c2 are clearly NOT in the same EQC.
**
** Inputs:
**      subquery                        subquery descriptor of temporary base 
**					relation
**	attrp				ptr to attribute descr for attr not yet
**					in EQC
**	eqc				EQC no of existing EQ class
**	ojid				joinid of join
**
** Outputs:
**	Returns:
**	    TRUE			if the attrs in this EQC include at
**					least 2 outer to the OJ and 1 inner
**					to the OJ
**	Exceptions:
**
** Side Effects:
**
** History:
**      15-aug-05 (inkdo01)
**	    Written to cover goofy OJ implications to equivalence classes.
*/
static bool
ope_ojcheck(
	OPS_SUBQUERY	*subquery,
	OPZ_ATTS	*attrp,
	OPE_IEQCLS	eqc,
	OPL_IOUTER	ojid)

{
    OPZ_IATTS		attno;
    OPZ_ATTS		*attrp1;
    OPL_OUTER		*ojp = subquery->ops_oj.opl_base->opl_ojt[ojid];
    OPE_EQCLIST		*eqcp = subquery->ops_eclass.ope_base->
							ope_eqclist[eqc];
    bool		inner = FALSE, outer = FALSE;


    /* First verify that the new attr is outer to current join. */
    if (!BTtest((i4)attrp->opz_varnm, (char *)ojp->opl_ovmap))
	return(FALSE);


    /* Loop over attrmap checking for attrs that are inner/outer 
    ** to OJ. */
    for (attno = -1; (attno = BTnext((i4)attno, (char *)&eqcp->ope_attrmap,
			subquery->ops_attrs.opz_av)) >= 0; )
    {
	attrp1 = subquery->ops_attrs.opz_base->opz_attnums[attno];
	if (BTtest((i4)attrp1->opz_varnm, (char *)ojp->opl_ivmap))
	    inner = TRUE;
	else if (BTtest((i4)attrp1->opz_varnm, (char *)ojp->opl_ovmap))
	    outer = TRUE;

	if (inner && outer)
	    return(TRUE);
    }

    return(FALSE);

}


/*{
** Name: ope_mergeatts	- merge equivalence classes associated with attributes
**
** Description:
**      This routine will merge the equivalence classes associated with the
**      joinop attributes.  It will create an equivalence class
**      if one does not exist for either attribute.  If the merging of the
**      equivalence classes would cause more than one attribute from the
**      same range variable to be in the same equivalence class, then the 
**      merging is not done and the routine returns false.
**
** Inputs:
**      global                          ptr to global state variable
**      firstattr			first joinop attribute number
**      secondattr			second joinop attribute number
**      nulljoin                        TRUE if merging these attributes
**                                      should not affect the NULL
**                                      joinability of this
**
** Outputs:
**	Returns:
**	    TRUE if the equivalence classes were successfully merged
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-apr-86 (seputis)
**          initial creation from mergeatts
**	15-jan-93 (ed)
**	    b47141 - outer join problem, asking special eqc from wrong relation
**	14-apr-94 (ed)
**	    b59588 - avoid creation of boolean factor but instead keep around
**	    equi-join as boolean factor which later gets iftrue's appended
**	11-may-94 (ed)
**	    - b59588 - insert return status which was incorrectly removed on
**	    fix to bug 59588
**	9-aug-05 (inkdo01)
**	    Add support for ope_ojs OJ bit map.
**	24-aug-05 (inkdo01)
**	    Drop update of ope_ojs & OPE_INNERJ - multi-eqc attr approach 
**	    was used to solve problem.
**	04-nov-05 (toumi01)
**	    Protect against NULL dereference for multi-eqc attr change.
**	24-jul-06 (hayke02)
**	    Call opc_eqcmerge() with new boolean argument ignore_overlap.
**	    Set ignore_overlap TRUE if we find the same outer join ID in
**	    both equiv classes' ope_ojs map. The outer join IDs are set in
**	    ope_ojs in this function after we have merged the attributes into
**	    the equiv classes.
**	    This allows a.c1, b.c1 and c.c1 to reside in the same equiv class
**	    for the query 'select ... from a join b on a.c1 = b.c1 left join c1
**	    on a.c1 = c.c1 and b.c1 = c.c1', despite ope_ojcheck() initially
**	    causing two separate equiv classes containing (b.c1, c.c1) and
**	    (a.c1, c.c1), and a cart prod between a and b. This change fixes
**	    bug 116406.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
bool
ope_mergeatts(
	OPS_SUBQUERY       *subquery,
	OPZ_IATTS	   firstattr,
	OPZ_IATTS	   secondattr,
	bool		   nulljoin,
	OPL_IOUTER	   ojid)
{
    OPE_IEQCLS		eqcls1;	    /* equivalence class associated with first
				    ** attribute
				    */
    OPE_IEQCLS          eqcls2;	    /* equivalence class associated with second
				    ** attribute
				    */
    OPZ_AT		*abase;     /* ptr to base of joinop attributes array */
    bool                retval;	    /* TRUE if both attributes could be placed
				    ** in the same equivalence class
				    */
    OPE_IEQCLS          resulteqc;  /* equivalence class of result if merge
				    ** was successful
				    */
    OPZ_ATTS		*attrp1; /* ptr to attribute descriptor */
    OPZ_ATTS		*attrp2;/* ptr to attribute descriptor */
    
    abase = subquery->ops_attrs.opz_base;
    attrp1 = abase->opz_attnums[firstattr];
    eqcls1 = attrp1->opz_equcls;
    attrp2 = abase->opz_attnums[secondattr];
    eqcls2 = attrp2->opz_equcls;
    if (!nulljoin)
    {	/* if this join is explicit then it may be because aggregate
	** processing placed a join in the qualification, so check if
	** one of the attributes for this join is an aggregate temp
	** and change nulljoin to TRUE if it is one of the by values
        ** however if it is an aggregate expression then leave it alone
	*/
	OPV_RT		*vbase;	    /* ptr to local range table */
	OPS_SQTYPE	sqtype;
	OPV_GRV		*grvp;

	vbase = subquery->ops_vars.opv_base;
	grvp = vbase->opv_rt[attrp1->opz_varnm]->opv_grv;
	sqtype = grvp->opv_created;
	if ((sqtype == OPS_RFAGG)
	    ||
	    (sqtype == OPS_HFAGG)
	    ||
	    (sqtype == OPS_FAGG))
	    nulljoin = ope_aggregate(subquery, attrp1, grvp);
	if (!nulljoin)
	{
	    grvp = vbase->opv_rt[attrp2->opz_varnm]->opv_grv;
	    sqtype = grvp->opv_created;
	    if ((sqtype == OPS_RFAGG)
		||
		(sqtype == OPS_HFAGG)
		||
		(sqtype  == OPS_FAGG))
		nulljoin = ope_aggregate(subquery, attrp2, grvp);
	}
    }

    if (eqcls1 == eqcls2)	    /* if both vars have same eqclass */
    {
	/* neither is in a eqclass yet create a new eqclass ,
	** and put both attributes in the same new eqclass
	*/
	if (eqcls1 == OPE_NOEQCLS)			
	{
	    resulteqc = ope_neweqcls(subquery, firstattr);
	    retval = ope_addeqcls(subquery, resulteqc, secondattr, nulljoin);
	}
	else
	{
	    resulteqc = eqcls1;
	    retval = TRUE;
	}
    }
    else if (eqcls1 == OPE_NOEQCLS)		/* firstattr is not in an eqclass */
    {
	if (ojid != OPL_NOOUTER && subquery->ops_oj.opl_base &&
		ope_ojcheck(subquery, attrp1, eqcls2, ojid))
	{
	    resulteqc = ope_neweqcls(subquery, firstattr);
	    retval = ope_addeqcls(subquery, resulteqc, secondattr, nulljoin);
	}
	else
	{
	    resulteqc = eqcls2;
	    retval = ope_addeqcls(subquery, eqcls2, firstattr, nulljoin);
	}
    }
    else if (eqcls2 == OPE_NOEQCLS)		/* secondattr is not in a eqclass */
    {
	if (ojid != OPL_NOOUTER && subquery->ops_oj.opl_base &&
		ope_ojcheck(subquery, attrp2, eqcls1, ojid))
	{
	    resulteqc = ope_neweqcls(subquery, secondattr);
	    retval = ope_addeqcls(subquery, resulteqc, firstattr, nulljoin);
	}
	else
	{
	    resulteqc = eqcls1;
	    retval = ope_addeqcls(subquery, eqcls1, secondattr, nulljoin);
	}
    }
    else
    {
	bool		ignore_overlap = FALSE;
	OPL_IOUTER	ojid1;

	resulteqc = eqcls2;

	if (ojid != OPL_NOOUTER && subquery->ops_oj.opl_base)
	{
	    for (ojid1 = -1; (ojid1 = BTnext((i4)ojid1,
		(char *)&subquery->ops_eclass.ope_base->ope_eqclist[eqcls1]->
		ope_ojs, (i4)subquery->ops_oj.opl_lv))>=0;)
	    {
		if (BTtest((i4)ojid1, (char *)&subquery->ops_eclass.ope_base->
						ope_eqclist[eqcls2]->ope_ojs))
			ignore_overlap = TRUE;
	    }
	}
	retval = ope_eqcmerge(subquery, eqcls1, eqcls2, nulljoin,
				ignore_overlap);/* merge two
						** equivalence classes into
						** eqcls2
						*/
    }
    if (ojid != OPL_NOOUTER && subquery->ops_oj.opl_base)
	BTset((i4)ojid, (char *)&subquery->ops_eclass.ope_base->
					    ope_eqclist[resulteqc]->ope_ojs);

#if 0
/* bug 59588 - instead of creating this boolean factor for equi-joins it may be
** better to just keep the original equi-join qualification and have special code
** to not consider the boolean factor in part of the cost calculations but just
** have it place outer joins... creating this boolean factor early will prevent
** associativity when it may be desirable, so if it is eventually used again it
** should be created after the query plan has been found */
    if (subquery->ops_oj.opl_base
	&&
	(ojid >= 0)
	&&
	retval)
    {   /* check for eqcls in which one attribute originates from outer
	** set of relations and one attribute originates from inner
	** set of relations */
	OPL_OUTER	*ojp;
	ojp = subquery->ops_oj.opl_base->opl_ojt[ojid];
	/* check for the case of a join in which it is possible to an
	** uninitialized attribute involved due to another outer join,
	** which is indicated by the special eqc... this occurs when
	** one of the attributes is defined as the inner relation of
	** another outer join defined within the scope of this outer join */
	if (subquery->ops_oj.opl_lv > 1)
	{   /* if at least 2 outer joins exist then there is the possibility
	    ** of nesting */
	    OPL_IOUTER	    ojid1;
	    bool	    attr1_found;    /* TRUE - if first attribute has
					    ** already been processed */
	    bool	    attr2_found;    /* TRUE - if second attribute has
					    ** already been processed */

	    attr1_found = attr2_found = FALSE;
	    for (ojid1 = -1; (ojid1=BTnext((i4)ojid1, (char *)ojp->opl_idmap,
		(i4)subquery->ops_oj.opl_lv))>=0;)
	    {
		OPL_OUTER   *ojp1;
		ojp1 = subquery->ops_oj.opl_base->opl_ojt[ojid1];
		if ((ojp1->opl_type != OPL_FULLJOIN)
		    &&
		    (ojp1->opl_type != OPL_LEFTJOIN))
		    continue;		    /* nulls are only available from 
					    ** full joins and left joins, at the
					    ** point right joins are turned into
					    ** left joins and do not exist */
		if (!attr1_found 
		    &&
		    BTtest((i4)attrp1->opz_varnm, (char *)ojp1->opl_ivmap))
		{
		    opl_seqc(subquery, attrp1->opz_varnm, ojid); /* add special eqc 
					    ** test for this object ID to take care
					    ** of joins on NULLs */
		    attr1_found = TRUE;
		}
		else if (!attr2_found
		    &&
		    BTtest((i4)attrp2->opz_varnm, (char *)ojp1->opl_ivmap))
		{
		    opl_seqc(subquery, attrp2->opz_varnm, ojid); /* add special eqc 
					    ** test for this object ID to take care
					    ** of joins on NULLs */
		    attr2_found = TRUE;
		}
	    }
	}
    }    
#endif
    return (retval);
}
