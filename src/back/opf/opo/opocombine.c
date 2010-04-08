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
#define        OPO_COMBINE	TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPOCOMBINE.C - combine two orderings as the result of a join
**
**  Description:
**      This file contains procedures which combines the outer and inner
**      ordering to produce a ordering which best describes the output of
**      a join. 
[@comment_line@]...
**
**          opo_combine - combine inner and outer ordering
[@func_list@]...
**
**
**  History:    
**      14-dec-87 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      5-jan-93 (ed)
**          fix bug 48049, fix constant equivalence class handling in joins
**          - remove several special cases, and simplify multi-attribute
**          join calculations in opo_combine.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
[@history_template@]...
**/

/*}
** Name: OPO_CONSTRUCT - state variable for multi-attribute ordering
**
** Description:
[@comment_line@]...
**
** History:
**      26-aug-88 (seputis)
**          made change for unix linting
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPO_CONSTRUCT
{
    i4              opo_state;          /* state of ordering construction */
#define             OPO_FEQCLS          1
/* used when only one ordering has been placed in the exact ordering list
** - it could mean that an exact multi-attribute ordering is not needed
** but only opo_first needs to be used */
#define             OPO_SEQCLS          2
/* the first 2 orderings have been inserted into the multi-attribute ordering
** so an exact ordering is required */
#define             OPO_MEQCLS          3
/* at least 3 orderings have been inserted into the multi-attribute ordering
*/
    OPO_ISORT       opo_orig;           /* original ordering */
    OPO_ISORT	    opo_first;          /* first sub-ordering found */
    OPO_ISORT	    opo_multi;          /* ordering which is compatible with
                                        ** the multi-attribute ordering being
                                        ** constructed */
    i4		    opo_count;		/* number of elements in exact
					** ordering */
    OPO_EQLIST	    *opo_elist;		/*temporary list of orderings
                                        ** classes, for the OPO_SEXACT
                                        ** ordering being constructed,
                                        ** - this is only used if there is
                                        ** not a compatible ordering in
                                        ** opo_multi */
    OPO_EQLIST      opo_tlist;          /* temporary space for equivalence
                                        ** class list */
    bool	    opo_notusable;      /* TRUE - if remainder of ordering
				        ** from outer cannot be used for
                                        ** a new outer ordering 
                                        ** i.e. a opo_ordeqc of the
                                        ** intermediate result, since
                                        ** it is not found in the result
                                        ** eqcls bitmap */
					/* for a join ordering
                                        ** the inner and outer orderings
                                        ** have become incompatible for
                                        ** a multi-attibute join at this
                                        ** point */
    OPE_BMEQCLS     *opo_eqcmap;	/* map of eqcls which can be involved
                                        ** in the ordering being constructed */
    OPO_EQLIST	    *opo_ordp;          /* ptr to list of orderings */
}   OPO_CONSTRUCT;

/*}
** Name: OPO_MSTATE - merge state of two orderings
**
** Description:
**      This routine will traverse the existing orderings defined and 
**      determine if a new ordering needs to be created or that the
**      requested ordering could possibly be contained in the current list
**
** History:
**      11-dec-87 ( seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPO_MSTATE
{
    OPS_SUBQUERY    *opo_subquery;      /* current subquery being analyzed */
    OPO_ISORT       opo_jorder;         /* join order given the outer and
                                        ** inner orderings */
    OPO_CONSTRUCT   opo_cjoin;	        /* state used to construct a
                                        ** join ordering i.e. a opo_sjeqc
                                        ** value for the intermediate result */
    OPO_CONSTRUCT   opo_corder;         /* state used to construct a
                                        ** new ordering for the intermediate
                                        ** result i.e. opo_ordeqc */
} OPO_MSTATE;

/*{
** Name: opo_finish	- finish off ordering
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
**      6-jun-89 (seputis)
**          fix b6377
**      25-sep-89 (seputis)
**          - fix for b5411, MEfill had a size too small
[@history_template@]...
*/
static OPO_ISORT
opo_finish(
	OPS_SUBQUERY	    *subquery,
	OPO_CONSTRUCT	    *conp)
{
    OPE_IEQCLS	    maxeqcls;
    maxeqcls = subquery->ops_eclass.ope_ev;

    /* create new ordering since current one was not found */
    switch (conp->opo_state)
    {
    case OPO_FEQCLS:
	return(OPE_NOEQCLS);		    /* no common eqcls was found so
					    ** no compatible ordering exists
					    */
    case OPO_SEQCLS:
	return(conp->opo_first);	    /* one one ordering is in the list
                                            ** so a new one does not need to
					    ** be created */
    case OPO_MEQCLS:
    {	/* at least two orderings exist in the list */

	if (conp->opo_multi != OPE_NOEQCLS)
	{
	    OPO_ISORT	    order;
	    OPO_ISORT	    maxorder;
	    OPO_SORT	    *previous;
	    maxorder = subquery->ops_msort.opo_sv;
	    previous = subquery->ops_msort.opo_base->
		    opo_stable[conp->opo_multi];
	    if (previous->opo_eqlist->opo_eqorder[conp->opo_count]
		== OPE_NOEQCLS)
		return(conp->opo_multi + maxeqcls); /* current ordering is 
					** compatible */
	    for (order = conp->opo_multi+1; order < maxorder; order++)
	    {
		OPO_SORT	    *exactp;
		exactp = subquery->ops_msort.opo_base->
		    opo_stable[order];
		if (exactp->opo_stype == OPO_SEXACT)
		{
		    bool	    found;
		    i4		    compare;
		    found = TRUE;
		    for (compare = 0; compare < conp->opo_count;
			compare++)
		    {
			if (exactp->opo_eqlist->opo_eqorder[compare]
			    !=
			    previous->opo_eqlist->opo_eqorder[compare])
			{
			    found = FALSE;
			    break;
			}
		    }
		    if (found
			&& 
			(exactp->opo_eqlist->opo_eqorder[compare] ==OPE_NOEQCLS))
			    return(order + maxeqcls);
		}
	    }
	    {
		i4	list_size;
		/* need to create a new ordering since no orderings match
		** exactly, copy into private memory */
		list_size = sizeof(OPO_ISORT) *
		    (conp->opo_count+1);
		conp->opo_ordp = (OPO_EQLIST *)
		    opn_memory(subquery->ops_global, (i4)list_size);
		MEcopy((PTR)previous->opo_eqlist, 
		    list_size-sizeof(OPO_ISORT),
		    (PTR)conp->opo_ordp);
		conp->opo_ordp->opo_eqorder[conp->opo_count] = OPE_NOEQCLS;
	    }
	}
	else
	{   /* check if current list is in ULM, and place it there if not */
	    if ((conp->opo_count <= DB_MAXKEYS)
		||
		!(conp->opo_count % DB_MAXKEYS))
	    {
		i4	list1_size;
		/* need to create a new ordering since no orderings match
		** exactly, copy into private memory */
		list1_size = sizeof(OPO_ISORT) * (conp->opo_count+1);
		conp->opo_ordp = (OPO_EQLIST *)opn_memory(
		    subquery->ops_global, (i4)list1_size);
		MEcopy((PTR)conp->opo_elist, list1_size,
		    (PTR)conp->opo_ordp);
	    }
	    else
	    {	/* memory was allocated out of the opn_memory stream so
		** use it directly, since up to DB_MAXKEYS would use
		** a stack buffer */
		conp->opo_ordp = conp->opo_elist;
	    }
	    conp->opo_ordp->opo_eqorder[conp->opo_count] = OPE_NOEQCLS;
	}
	break;
    }
    default:
	;				    /* FIXME - consistency check */
    }
    {	/* init the bit map for this exact ordering */
	OPO_ISORT	*torderp;
	bool		first_flag;
	OPO_ISORT	ordering;
	OPO_SORT	*orderp;

	ordering = opo_gnumber(subquery);	/* get the next multi-attribute
					    ** ordering number */
	orderp = (OPO_SORT *)opn_memory(subquery->ops_global, (i4)sizeof(*orderp));
	orderp->opo_stype = OPO_SEXACT;
	orderp->opo_bmeqcls = (OPE_BMEQCLS *)opn_memory(subquery->ops_global, (i4)sizeof(OPE_BMEQCLS));
	orderp->opo_eqlist = conp->opo_ordp;
	subquery->ops_msort.opo_base->opo_stable[ordering] = orderp;
	first_flag = TRUE;
	for (torderp = &conp->opo_ordp->opo_eqorder[0]; (*torderp) != OPE_NOEQCLS; torderp++)
	{
	    if ( (*torderp) < maxeqcls)
	    {
		if (first_flag)
		{
		    MEfill(sizeof(*orderp->opo_bmeqcls), (u_char)0,
			(PTR)orderp->opo_bmeqcls);
		    first_flag = FALSE;
		}
		BTset((i4)*torderp, (char *)orderp->opo_bmeqcls);
	    }
	    else
	    {
		OPO_SORT	    *eorderp;	/* exact order pointer */

		eorderp = subquery->ops_msort.opo_base->
		    opo_stable[(*torderp)-maxeqcls];
		if(first_flag)
		{
		    MEcopy((PTR)eorderp->opo_bmeqcls,sizeof(OPE_BMEQCLS), 
			(PTR)orderp->opo_bmeqcls);
		    first_flag = FALSE;
		}
		else
		    BTor((i4)maxeqcls, (char *)eorderp->opo_bmeqcls,
			(char *)orderp->opo_bmeqcls);
	    }
	}
	return((OPO_ISORT)(ordering+maxeqcls));
    }
}

/*{
** Name: opo_aordering	- add an ordering to the one being constructed
**
** Description:
**      This routine appends a new ordering to the one being constructed.
**      It will attempt to use existing orderings as much as possible, but
**      will create a new ordering entry if necessary
**
** Inputs:
**	mstatep                              ptr to state variable
**      conp                            ptr to the state variable for the
**                                      ordering being constructed
**      ordering                        ordering to append to conp
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
**      10-mar-88 (seputis)
**          initial creation
**	16-oct-92 (ed)
**	    - b47232 - exit loop marking multi-attribute ordering as not
**	    being found, or multi-attribute ordering will not be complete.
[@history_template@]...
*/
static VOID
opo_aordering(
	OPS_SUBQUERY	   *subquery,
	OPO_CONSTRUCT      *conp,
	OPO_ISORT          ordering)
{
    OPO_ISORT	    maxorder;
    maxorder = subquery->ops_msort.opo_sv;
    switch (conp->opo_state)
    {
    case OPO_FEQCLS:
    {	/* first eqcls found so store this until next call */

	conp->opo_first = ordering;
	conp->opo_state = OPO_SEQCLS;
	break;
    }
    case OPO_SEQCLS:
    {	/* second eqcls ... so that a multi-attribute ordering needs to
	** be constructed */
	OPO_ISORT	    sorder;
	conp->opo_multi = OPE_NOEQCLS; /* this is set to no eqcls unless
                                       ** an existing ordering is found */
	for (sorder = 0; sorder < maxorder; sorder++)
	{   /* search existing list for an ordering candidate */
	    OPO_SORT	    *exactp;
	    exactp = subquery->ops_msort.opo_base->opo_stable[sorder];
	    if ((exactp->opo_stype == OPO_SEXACT)
		&&
		(exactp->opo_eqlist->opo_eqorder[0] == conp->opo_first)
		&&
		(exactp->opo_eqlist->opo_eqorder[1] == ordering)
	       )
	    {	/* found a partial match */
		conp->opo_multi = sorder;    /* existing ordering found */
		break;
	    }
	}
	if (conp->opo_multi == OPE_NOEQCLS)
	{   /* there is no compatible ordering so a new one needs to be
	    ** constructed */
	    conp->opo_elist = &conp->opo_tlist;
	    conp->opo_elist->opo_eqorder[0] = conp->opo_first ;
	    conp->opo_elist->opo_eqorder[1] = ordering;
	}
	conp->opo_count = 2;
	conp->opo_state = OPO_MEQCLS;
	break;
    }
    case OPO_MEQCLS:
    {	/* continue in this state until the construction is complete */
	if (conp->opo_multi != OPE_NOEQCLS)
	{
	    OPO_ISORT	    order;
	    OPO_SORT	    *previous;
	    bool	    found;

	    previous = subquery->ops_msort.opo_base->
		    opo_stable[conp->opo_multi]; /* ptr to current ordering
					    ** to be analyzed */
	    if (previous->opo_eqlist->opo_eqorder[conp->opo_count]
		== ordering)
	    {	/* current ordering is still compatible */
		conp->opo_count++;
		break;
	    }
	    found = FALSE;
	    for (order = conp->opo_multi+1; order < maxorder; order++)
	    {
		OPO_SORT	    *exactp;
		exactp = subquery->ops_msort.opo_base->
		    opo_stable[order];
		if (exactp->opo_stype == OPO_SEXACT)
		{
		    i4		compare;
		    found = TRUE;
		    for (compare = 0; compare < conp->opo_count;
			compare++)
		    {
			if (exactp->opo_eqlist->opo_eqorder[compare]
			    !=
			    previous->opo_eqlist->opo_eqorder[compare])
			{
			    found = FALSE;
			    break;
			}
		    }
		    if (found
			&&
			(exactp->opo_eqlist->opo_eqorder[conp->opo_count]
			    == ordering))
		    {
			conp->opo_count++;
			conp->opo_multi = order;
			break;
		    }
		    else
			found = FALSE;	/* fix bug 47232 - do  not exit
					** assuming matching attribute has 
					** been found */
		}
	    }
	    if (found)
		break;			/* compatible ordering exists */
	    conp->opo_multi = OPE_NOEQCLS; /* ordering not found so
				** need to create a new list */
	    if (conp->opo_count < DB_MAXKEYS)
	    {
		conp->opo_elist = &conp->opo_tlist;
	    }
	    else
	    {   /* allocate memory to store extra large ordering list,
		** make it the nearest multiple of DB_MAXKEYS */
		conp->opo_elist = (OPO_EQLIST *)opn_memory(
		    subquery->ops_global,
		    (i4) (sizeof(conp->opo_tlist.opo_eqorder[0]) *
			 ((i4)(conp->opo_count/DB_MAXKEYS)+1)*DB_MAXKEYS));
	    }
	    MEcopy((PTR)previous->opo_eqlist,
		sizeof(conp->opo_tlist.opo_eqorder[0])
		    * conp->opo_count, (PTR)conp->opo_elist);
	}
	else
	{   /* no compatible ordering exists so add to the current list */
	    if ((conp->opo_count % DB_MAXKEYS)==0)
	    {	/* a boundary is reached to add more memory */
		OPO_EQLIST	*tlist;
		tlist = conp->opo_elist;
		conp->opo_elist = (OPO_EQLIST *)opn_memory(
		    subquery->ops_global,
		    (i4) (sizeof(conp->opo_tlist.opo_eqorder[0]) *
			 ((i4)(conp->opo_count/DB_MAXKEYS)+1)*DB_MAXKEYS));
		MEcopy((PTR)tlist, sizeof(conp->opo_tlist.opo_eqorder[0])
			* conp->opo_count, (PTR)conp->opo_elist);
	    }
	}
	conp->opo_elist->opo_eqorder[conp->opo_count++] = ordering;
	break;
    }
    default:
	;			/* FIXME - consistency check */
    }
}

/*{
** Name: opo_bnewordering	- build new ordering
**
** Description:
**      This routine will build a new ordering for the result
**      of a join, given the eqcls bitmap of attributes available in
**      the result. 
**
** Inputs:
**      mstatep                         ptr to state variable describing
**                                      result of ordering/join calculation
**      ordering                        next ordering which is compatible with
**                                      the inner and outer
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
**      01-mar-88 (seputis)
**          initial creation
**      5-may-89 (seputis)
**          fix bug 5664
**      4-aug-89 (seputis)
**          fix b6570, check for NULL opo_eqcmap
**      27-dec-89 (seputis)
**          fix multi-attribute ordering bug
**      20-jun-90 (seputis)
**          fix b8410 - lose ordering of secondary attributes with constant
**	    equivalence classes
[@history_line@]...
[@history_template@]...
*/
static VOID
opo_bnewordering(
	OPO_MSTATE         *mstatep,
	OPO_CONSTRUCT      *conp,
	OPO_ISORT          ordering)
{
    /* build the joining ordering given the outer and inner orderings
    ** and also the restricted set ... need to be able to ignore
    ** constant equivalence classes, since they would not invalid
    ** an exact ordering produced by a BTREE */
    OPE_IEQCLS          maxeqcls;
    OPE_BMEQCLS		*bfeqc;
    OPS_SUBQUERY	*subquery;
    
    subquery = mstatep->opo_subquery;
    bfeqc = subquery->ops_bfs.opb_bfeqc;
    maxeqcls = subquery->ops_eclass.ope_ev;
    if (ordering < maxeqcls)
    {
	if (bfeqc && BTtest((i4)ordering, (char *)bfeqc))
	{
	    ordering = OPE_NOEQCLS; /* do not place constant
			    ** equivalence classes in ordering */
	}
	else
	    conp->opo_notusable = 
		(ordering != conp->opo_orig) /* check for single joining
					** eqcls case, with no constant
					** equivalence classes, the ordering
					** may be a constant equivalence class
					** which is why it may not be equal
					** to the original */
		&&
		(   !conp->opo_eqcmap 
		    ||
		    !BTtest((i4)ordering, (char *)conp->opo_eqcmap) /*
					** check for multi-attribute case
					** including constant equivalence
					** classes, it must be in the set
					** of visible equivalence classes
					** to be included */
		);
	if (conp->opo_notusable)
	    ordering = OPE_NOEQCLS;
    }
    else
    {
	OPO_SORT	    *orderp;
	OPE_BMEQCLS	    usable_eqcmap;
	OPE_BMEQCLS	    *bmeqclsp;
	orderp = mstatep->opo_subquery->ops_msort.opo_base->
	    opo_stable[ordering-maxeqcls]; /* should be inexact ordering
				    ** at this point - consistency check 
				    ** FIXME */
	if (orderp->opo_stype != OPO_SINEXACT)
	    opx_error(E_OP049C_INEXACT); /* expecting inexact ordering */
	if (bfeqc)
	{
	    MEcopy((PTR)orderp->opo_bmeqcls, sizeof(usable_eqcmap),
		(PTR)&usable_eqcmap);
	    BTand((i4)maxeqcls, (char *)subquery->ops_bfs.opb_mbfeqc, 
		(char *)&usable_eqcmap); /* remove constant equivalence
				    ** classes from ordering */
	    bmeqclsp = &usable_eqcmap;
	    if (BTnext((i4)-1, (char *)&usable_eqcmap,
		(i4)maxeqcls) < 0)
		ordering = OPE_NOEQCLS;
	}
	else
	    bmeqclsp = orderp->opo_bmeqcls;

	if (ordering != OPE_NOEQCLS)
	{
	    conp->opo_notusable = 
		!conp->opo_eqcmap 
		||
		!BTsubset((char *)bmeqclsp, (char *)conp->opo_eqcmap,
		    (i4)maxeqcls);	    /* this also includes the case
					** of constant equivalence classes
					** being defined,... in the case
					** of a opo_eqcmap being NULL, this
					** means the join was a single eqcls
					** which is tested via opo_orig */
	    if (conp->opo_notusable)
	    {   /* at least part of the multi-attribute join ordering is 
		** unusable, so determine if any subset can be used in the
		** ordering */
		if (conp->opo_eqcmap)
		{
		    if (!bfeqc)
			MEcopy((PTR)orderp->opo_bmeqcls, sizeof(usable_eqcmap),
			    (PTR)&usable_eqcmap);
		    BTand((i4)maxeqcls, (char *)conp->opo_eqcmap,
			(char *)&usable_eqcmap);
		    ordering = opo_fordering(mstatep->opo_subquery,
			    &usable_eqcmap);
		}
		else if ((conp->opo_orig != OPE_NOEQCLS)
			&&
			(conp->opo_orig < maxeqcls)
			&&
			BTtest((i4)conp->opo_orig, (char *)bmeqclsp)
			)
		    ordering = conp->opo_orig;
		else
		    ordering = OPE_NOEQCLS;
	    }
	    else if (bfeqc)
		ordering = opo_fordering(mstatep->opo_subquery,
			&usable_eqcmap);
	}
    }
    if (ordering != OPE_NOEQCLS)
	opo_aordering(mstatep->opo_subquery, conp, ordering); /* add a 
					** ordering to the
					** ordering being constructed */

}

/*{
** Name: opo_prefix	- construct an EXACT ordering descriptor
**
** Description:
**      This routine will construct a new ordering descriptor by repeated
**      calls.  It will try to find an element in the ordering descriptor
**      list which is compatible with the ordering found so far.  If this
**      is not possible then a new ordering will be constructed.
**      This routine will also check that the incremental orderings contain
**      only equivalence classes which are allowed in the new ordering.  If
**      an attempt is made to add an ordering which contains an unusable
**      eqcls, then the ordering creation is terminated, and only the
**      compatible portion of the ordering is added.
**
**      An unusable eqcls is defined to be one which occurs in either the
**      inner or outer CO but is not found in the join CO.
**
** Inputs:
**      mstatep                         ptr to current state of ordering
**                                      construction
**      ordering                        new ordering to be added to the
**                                      exact ordering being defined
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    TRUE - if an incompatible eqcls is found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-dec-87 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opo_prefix(
	OPO_MSTATE         *mstatep,
	OPO_ISORT          ordering)
{
    if (mstatep->opo_corder.opo_notusable && mstatep->opo_cjoin.opo_notusable)
	return(TRUE);
    if (ordering == OPE_NOEQCLS)
	opx_error(E_OP0493_MULTIATTR_SORT);
    if (!mstatep->opo_corder.opo_notusable)
	opo_bnewordering(mstatep, &mstatep->opo_corder, ordering);
    if (!mstatep->opo_cjoin.opo_notusable)
	opo_bnewordering(mstatep, &mstatep->opo_cjoin, ordering);
    return (mstatep->opo_corder.opo_notusable && mstatep->opo_cjoin.opo_notusable);
}

/*{
** Name: opo_merge	- merge slave suborderings
**
** Description:
**      Merge the suborderings of the "slave" ordering, until a mismatch
**      occurs.  The slave and master roles can then be reversed
**
** Inputs:
**      mstatep                         ptr to ordering state variable
**      bmmaster                        merge suborderings from slave while
**                                      eqcls bitmaps are a subset of this
**                                      map
**      bmslave                         cumulative bitmap of slave
**                                      suborderings
**      mslavep                         ptr to current subordering being
**                                      analyzed
**
** Outputs:
**	Returns:
**	    TRUE if merging can continue, FALSE otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jan-88 (seputis)
**          initial creation
**	28-jan-92 (seputis)
**	    fix 41939 - ignore constant equivalence classes when building keys
**	    - fixes top sort node removal problem and keying problem.
**	19-jan-94 (ed)
**	    fixed problem in which result ordering was not being calculated
**	    to take advantage of full outer ordering
[@history_template@]...
*/
static bool
opo_merge(
	OPO_MSTATE         *mstatep,
	OPE_BMEQCLS        *bmmaster,
	OPE_BMEQCLS        *bmslave,
	OPO_ISORT          **slavepp)
{
    OPS_SUBQUERY       *subquery;
    OPO_ISORT           slave;
    OPE_IEQCLS          maxeqcls;

    subquery = mstatep->opo_subquery;
    maxeqcls = subquery->ops_eclass.ope_ev;
    while ((slave = *((*slavepp)++)) != OPE_NOEQCLS)
    {   /* loop while the slave orderings can fit inside the master ordering
        */
	if (slave >= maxeqcls)
	{   /* a multi-attribute ordering was found */
	    OPO_SORT    *sslavep; /* current descriptor of 
			    ** outer being analysed */
	    sslavep = subquery->ops_msort.opo_base->
		opo_stable[slave - maxeqcls];
	    BTor((i4)maxeqcls, (char *)sslavep->opo_bmeqcls,
		(char *)bmslave);   /* merge attributes into set
			    ** of eqcls seen so far */
	    if (!BTsubset((char *)bmslave, (char *)bmmaster, (i4)maxeqcls))
	    {	/* loop terminates since slave will now become master,
                ** check if an ordering set needs to be split */
		OPE_BMEQCLS	    split;
		OPE_IEQCLS	    first_eqcls;
		
		MEcopy((PTR)bmmaster, sizeof(split), (PTR)&split);
		BTand((i4)maxeqcls, (char *)sslavep->opo_bmeqcls,
		    (char *)&split);
		if (subquery->ops_bfs.opb_bfeqc)
		    BTand((i4)maxeqcls, 
			(char *)subquery->ops_bfs.opb_mbfeqc,
			(char *)&split);	/* remove any constant
						** equivalence classes from 
						** this set */
		first_eqcls = BTnext((i4)-1, (char *)&split, (i4)maxeqcls);
						/* since bmmaster should
						** contain opb_bfeqc, there
						** should be at least one
						** non-constant eqcls at this
						** point, which is in the
						** slave but not in the master
						*/
		if (first_eqcls >= 0)
		{   /* at least one part of ordering is compatible , this
		    ** means that the outer ordering needs to be split
		    ** into a compatible and non-compatible portion 
		    ** because the join will occur on the compatible
		    ** portion */
		    OPO_ISORT	    new_ordering;
		    if (BTnext((i4)first_eqcls, (char *)&split, 
			(i4)maxeqcls) >= 0)
		    {   /* There are at least two attributes in this
			** ordering so create or find a multi-attribute
			** ordering */
			new_ordering = opo_fordering(subquery, &split);
			if (opo_prefix(mstatep, new_ordering)) /* 
				    ** copy this ordering and the
				    ** remainder */
			    return(FALSE);
		    }
		    else
		    {   /* split outer ordering in half, first part is
			** compatible with inner ordering and the
			** second part is not */
			if (opo_prefix(mstatep, first_eqcls)) /* 
				    ** copy this ordering and the
				    ** remainder */
			    return(FALSE);
		    }
		}
		break;
	    }
	}
	else 
	{
	    /* a single eqcls was found */
	    if (subquery->ops_bfs.opb_bfeqc
		&&
		BTtest((i4)slave, (char *)subquery->ops_bfs.opb_bfeqc)
		)
		continue;	    /* ignore constant equivalence classes 
				    ** - b41939
				    */
		
	    BTset((i4)slave, (char *)bmslave);
	    if (!BTtest((i4)slave, (char *)bmmaster))
		break;
	}
	/* add elements from outer until a super set of the
	** inner exists */
	if(opo_prefix(mstatep, slave)) /* copy
			    ** this ordering since it is in common*/
	    return(FALSE);  /* since ordering was not completely stored
                            ** successfully, the merge cannot continue */
    }
    if (slave == OPE_NOEQCLS)
	(*slavepp)--;	    /* do not read past OPE_NOEQCLS */

    if (BTsubset((char *)bmmaster, (char *)bmslave, (i4)maxeqcls))
    {
	/* slave can become master so indicate this to parent so
	** roles can be reversed */
	return(TRUE);
    }
    mstatep->opo_cjoin.opo_notusable = TRUE; /* no orderings can
					** be added to the joining eqcls */
    return(FALSE);
}

/*{
** Name: opo_combine	- combine outer and inner orderings
**
** Description:
**      Used in multi-attribute joins... The ordering of the outer is
**      retained, but there may be restrictions imposed by the inner
**      on the result of the join node.  This routine will construct a
**      new ordering descriptor which is compatible with the outer but
**      contains the restrictions imposed by the inner.  Basically this
**      means the set of joining attributes must be ordered first, prior
**      to ordering the remaining set of attributes, which can be used
**      farther up the tree.
**	    This routine should not return constant equivalence classes
**	in any ordering but will be able to deal with any ordering given
**	the existence of constant equivalence classes.  The exception
**	to this is the rare case of a tid join which returns a constant
**	joining tid equivalence class i.e. the tid attribute.
**	    The ordering available after the join has been made is 
**	restricted to the set of attributes in eqcmap, i.e. the parent
**	node does not wish to see any attributes in an ordering which
**	are not available at that node.
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      jordering                       join on eqcls in this set only
**                                      - there are really only 2 cases
**                                      1) an TID join
**                                      2) a multi-attribute join of
**                                      all intersecting equivalence classes
**					- the ordering of this parameter
**					is ignored, only the set of
**					equivalence classes is used
**					to create jorderp
**					- the ordering of the inner is
**					used to enforce key join orderings
**      outer                           ordering from outer CO node
**      inner                           ordering from inner CO node
**      eqcmap                          bitmap of eqcls which must
**                                      be a super set of the new ordering
**                                      - this is the set of equivalence classes
**                                      which are available at the output of the
**                                      join node
**      imtid                           TRUE means implied tid ordering may
**                                      be considered so the inner ordering
**                                      is not important
**
** Outputs:
**      jorderp                         ptr to joining ordering, which is
**                                      based on the outer and inner ordering
**                                      - this ordering does not need to be
**                                      a subset of eqcmap
**                                      - this is the ordering defining the
**                                      all equivalence classes
**                                      which can participate in the join
**                                      since they are in both the inner
**                                      and outer orderings
**	Returns:
**	    OPO_ISORT ordering which is available after the join
**              - ordering which is based on outer, but intersects with
**              inner and is a subset of the eqcmap, this ordering can be
**              used by the parent of the node,... this ordering needs to be
**              a subset of eqcmap
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-dec-87 (seputis)
**          initial creation
**      24-apr-89 (seputis)
**          fix bug 5411 - do not reference eqc in ordering if not available
**	    to parent
**	3-may-90 (seputis)
**	    - bug 21485 - part of fix is all of 3 way join is not being found
**
**	    create table bx1 as select * from iiattribute;\p\g
**	    create table bx2 as select * from bx1;\p\g
**	    create table bx3 as select * from bx1;\p\g
**	    commit\p\g
**	    modify bx1 to btree on attrelid,attrelidx,attname;\p\g
**	    modify bx2 to btree on attrelid,attrelidx,attname;\p\g
**	    modify bx3 to btree on attrelid,attrelidx,attname;\p\g
**	    select bx1.attname,bx2.attrelid
**	    from bx1,bx2
**	    where bx1.attrelid=bx2.attrelid
**	    and bx1.attrelidx=bx2.attrelidx
**	    and bx1.attname=bx2.attname \p\g
**      23-dec-92 (ed)
**          changed to avoid placing constant equivalence classes in output
**	    orderings
**	5-jan-92 (ed)
**	    reworked to remove special cases and fix incorrect handling of
**	    constant equivalence classes
**	2-apr-94 (ed)
**	    b60202 - correct E_OP0497 consistency check
[@history_template@]...
*/
OPO_ISORT
opo_combine(
	OPS_SUBQUERY       *subquery,
	OPO_ISORT          jordering,
	OPO_ISORT          outer,
	OPO_ISORT          inner,
	OPE_BMEQCLS        *eqcmap,
	OPO_ISORT	   *jorderp)
{
    OPO_ISORT           ordering;	/* number used to represent
					** multi-attribute ordering */
    OPE_IEQCLS		maxeqcls;	/* number of equivalence classes
                                        ** defined */
    OPO_MSTATE		mstate;		/* describes state of new ordering
                                        ** being constructed */
    OPE_BMEQCLS		bmouter;	/* map of eqcls seen so far in the
					** outer ordering */
    OPE_BMEQCLS		bminner;        /* map of equivalence classes
					** seen so far in the inner
					** ordering */
    OPO_ISORT		souter;		/* current outer ordering being
                                        ** analyzed */
    OPO_ISORT		*souterp;	/* current outer sort pointer,
                                        ** - actually this is used as
                                        ** a "lookahead pointer" for the
                                        ** next ordering to be analyzed */
    OPO_ISORT		*sinnerp;	/* current inner sort pointer
                                        ** - actually this is used as
                                        ** a "lookahead pointer" for the
                                        ** next ordering to be analyzed */
    OPE_IEQCLS		ieqclslist[2];	/* used to simulate an exact
					** ordering for inner */
    OPE_IEQCLS		oeqclslist[2];	/* used to simulate an exact
					** ordering for outer */
    OPE_BMEQCLS		*bfeqc;		/* constant equivalence class map */

    bfeqc = subquery->ops_bfs.opb_bfeqc;
    maxeqcls = subquery->ops_eclass.ope_ev;
    if ((outer == OPE_NOEQCLS)
	||
	(   bfeqc
	    &&
	    (outer < maxeqcls)
	    &&
	    BTtest((i4)outer, (char *)bfeqc) /* if the outer is a constant
					** equivalence class then this 
					** degenerates to cart prod */
	))
    {
	*jorderp = OPE_NOEQCLS;
	return (OPE_NOEQCLS);
    }

    /* initialize bmouter to be the set of equivalence classes which
    ** are the most significant set of an ordering
    ** - initialize souterp to point to the next available part of
    ** an ordering if it exists */
    if (outer < maxeqcls)
    {
	MEfill(sizeof(bmouter), (u_char)0, (PTR)&bmouter);
	BTset( (i4)outer, (char *)&bmouter);
	souterp = &oeqclslist[0];
	oeqclslist[0] = OPE_NOEQCLS;
    }
    else
    {
	OPO_SORT		*outerp0; /* ptr to multi-attribute
					** ordering descriptor */
	outerp0 = subquery->ops_msort.opo_base->opo_stable[outer-maxeqcls];
	if (outerp0->opo_stype == OPO_SINEXACT)
	{
	    souterp = &oeqclslist[0];
	    oeqclslist[0] = OPE_NOEQCLS;
	    souter = outer;
	}
	else if (outerp0->opo_stype == OPO_SEXACT)
	{
 	    souterp = &outerp0->opo_eqlist->opo_eqorder[0];
	    souter = *(souterp++);
	}
	else
	    opx_error(E_OP0493_MULTIATTR_SORT);
	if (bfeqc)
	{   /* fix b41939 - constant equivalence classes should
	    ** be ignored when checking for join or output orderings */
	    if (BTsubset((char *)outerp0->opo_bmeqcls,
		(char *)bfeqc, (i4)maxeqcls))
	    {
		*jorderp = OPE_NOEQCLS;
		return (OPE_NOEQCLS);
	    }
	}
	if (souter < maxeqcls)	/* init the outer bitmap */
	{
	    if (bfeqc)
		MEcopy((PTR)bfeqc, sizeof(bmouter), (PTR)&bmouter); /* all
						** constant equivalence classes should
						** not affect joins or orderings */
	    else
		MEfill(sizeof(bmouter), (u_char)0, (PTR)&bmouter);
	    BTset( (i4)souter, (char *)&bmouter);
	}
	else 
	{
	    OPO_SORT	*outerp1;

	    outerp1 = subquery->ops_msort.opo_base->
		opo_stable[souter-maxeqcls];
	    if (outerp1->opo_stype != OPO_SINEXACT)
		opx_error(E_OP0493_MULTIATTR_SORT); /* only inexact
				    ** multi-attribute orderings
				    ** expected */
	    MEcopy((PTR)outerp1->opo_bmeqcls, sizeof(bmouter), 
		(PTR)&bmouter);
	    if (bfeqc)
		BTor((i4)maxeqcls, (char *)bfeqc, (char *)&bmouter);
					/* all constant equivalence classes should
					** not affect joins or orderings */
	}
    }

    /* - initialize sinnerp to point to being of list of orderings
    ** available on inner
    ** - initialize opo_veqcmap to be the set of orderings
    ** in common */
    if (inner < maxeqcls)
    {
	sinnerp = &ieqclslist[0];
	ieqclslist[0] = inner;		/* simulate an exact ordering */
	ieqclslist[1] = OPE_NOEQCLS;
    }
    else
    {
	OPO_SORT		*innerp0; /* ptr to multi-attribute
					** ordering descriptor */
	innerp0 = subquery->ops_msort.opo_base->opo_stable[inner-maxeqcls];
	switch (innerp0->opo_stype)
	{
	case OPO_SINEXACT:
	    sinnerp = &ieqclslist[0];
	    ieqclslist[0] = inner;	/* simulate an exact ordering */
	    ieqclslist[1] = OPE_NOEQCLS;
	    break;
	case OPO_SEXACT:
	    sinnerp = &innerp0->opo_eqlist->opo_eqorder[0];
	    break;
	default:
	    opx_error(E_OP0493_MULTIATTR_SORT);
	}
    }
    if (bfeqc)
	MEcopy((PTR)bfeqc, sizeof(bminner), (PTR)&bminner); /* all
					** constant equivalence classes should
					** not affect joins or orderings */
    else
	MEfill(sizeof(bminner), (u_char)0, (PTR)&bminner);
    mstate.opo_corder.opo_state = OPO_FEQCLS;	/* init state to find new ordering
                                        ** i.e. opo_ordeqc */
    mstate.opo_cjoin.opo_state = OPO_FEQCLS;	/* init state to find ordered join
                                        ** i.e. opo_sjeqc */
    mstate.opo_subquery = subquery;
    mstate.opo_corder.opo_notusable = FALSE;
    mstate.opo_cjoin.opo_notusable = FALSE;
    mstate.opo_corder.opo_eqcmap = eqcmap;
    mstate.opo_jorder = OPE_NOEQCLS;    
    mstate.opo_corder.opo_orig = outer;	/* not useful but necessary; inserted
                                        ** for symmetry with opo_cjoin */
    mstate.opo_cjoin.opo_orig = jordering;	/* only consider joining on this
                                        ** ordering, or some compatible subset */
    if (jordering >= maxeqcls)
    {	/* check that the join ordering is an inexact ordering  */
	OPO_SORT		*joinp0; /* ptr to multi-attribute
					** ordering descriptor */
	joinp0 = subquery->ops_msort.opo_base->opo_stable[jordering-maxeqcls];
	switch (joinp0->opo_stype)
	{
	case OPO_SINEXACT:
	{
	    mstate.opo_cjoin.opo_eqcmap = joinp0->opo_bmeqcls;
	    break;
	}
	case OPO_SEXACT:
	{
	    mstate.opo_cjoin.opo_eqcmap = joinp0->opo_bmeqcls;
	    break;
	}
	default:
	    opx_error(E_OP0493_MULTIATTR_SORT);
	}
    }
    else
	mstate.opo_cjoin.opo_eqcmap = NULL;
    do
    {
	if (!opo_merge(&mstate, &bmouter, &bminner, &sinnerp))
	    break;
	if (!opo_merge(&mstate, &bminner, &bmouter, &souterp))
	    break;
    } while ((*souterp != OPE_NOEQCLS) || (*sinnerp != OPE_NOEQCLS) );
				/* change termination condition to ||
				** since 3 way exact match will fail
				** to pick up the last attribute, b21485 */

    {	/* find remaining eqcls for outer ordering */
	OPO_ISORT	new1_ordering;
	mstate.opo_cjoin.opo_notusable = TRUE; /* b60202 no orderings can
					** be added to the joining eqcls */
	BTnot((i4)maxeqcls, (char *)&bminner);
	BTand((i4)maxeqcls, (char *)&bmouter, (char *)&bminner); 
	if (subquery->ops_bfs.opb_bfeqc)
	    BTand((i4)maxeqcls, 
		(char *)subquery->ops_bfs.opb_mbfeqc,
		(char *)&bminner);	/* remove any constant
				    ** equivalence classes from 
				    ** this set */
	new1_ordering = opo_fordering(subquery, &bminner);
	if (new1_ordering != OPE_NOEQCLS)
	    (VOID) opo_prefix(&mstate, new1_ordering); /* add the trailing
					** ordering to the ordeqc */
    }

    if (*souterp != OPE_NOEQCLS)
    {
	OPO_ISORT	souter1;
	souter1 = *(souterp++);
	for (;souter1 != OPE_NOEQCLS; souter1 = *(souterp++))
	{   /* scan thru outer all elements are copied, this is because any
	    ** ordering provided by outer will be retained by the join */
	    if (opo_prefix(&mstate, souter1)) /* copy
				** this ordering and the remainder */
		break;
	}
    }
    ordering = opo_finish(mstate.opo_subquery, &mstate.opo_corder); /* the new ordering 
				** has been construct so find the identifier */
    
    *jorderp = opo_finish(mstate.opo_subquery, &mstate.opo_cjoin); /* the new 
				** ordering 
				** has been construct so find the identifier */
    return(ordering);
}

/*{
** Name: opo_mexact	- convert to exact ordering given parent's orderings
**
** Description:
**      This procedure will convert an ordering to an exact ordering
**      taking into account the needs of the parent CO.  If there
**      are no ordering's required by the parent then a arbitrary
**      exact list will be created.  If the ordering cannot be created
**      then an error will be reported 
**
** Inputs:
**      subquery                        ptr to current subquery state
**      ordering                        ptr to ordering to be made
**                                      into an exact list of equilvance
**                                      classes
**      pordeqc                         exact ordering of parent which 
**                                      must be compatible with output
**                                      ordering
**      psjeqc                          exact ordering of parent join
**                                      which must be compatible with
**                                      output ordering if sjeqc_error
**                                      is FALSE, otherwise try to
**                                      use this ordering if there is
**                                      an option, but do not report
**                                      an error if it is not possible
**      valid_psjeqc                    TRUE - if sjeqc must be compatible
**                                      with the ordering field, report
**                                      inconsistency error if not,
**                                      FALSE - means psjeqc does not need
**                                      to be compatible with pordeqc, but
**                                      try to make it compatible, e.g.
**                                      a key or Tid join may use this
**      valid_pordeqc                   TRUE - if pordeqc must be compatible
**                                      with the ordering field, report
**                                      inconsistency error if not,
**                                      FALSE - means pordeqc does not need
**                                      to be compatible with pordeqc, but
**                                      try to make it compatible, e.g.
**                                      if the "ordering" is from an inner
**                                      then pordeqc would use the outer
**                                      but if the outer ordering is a
**                                      subset of the inner ordering
**                                      then the remainder of the inner
**                                      could provide an ordering
**                                      - if valid_psjeqc and valid_pordeqc
**                                      are both set then pordeqc is to be
**                                      preferred since valid_psjeqc would
**                                      be FALSE if TID joins or KEY joins
**                                      are specified, but valid_pordeqc
**                                      may be used further up the tree.
**	truncate                        TRUE - if the ordering should be
**                                      truncated once psjeqc and pordeqc
**                                      have been specified, this prevents
**                                      long lists in the set qep CO tree
**                                      from being printed
**      sjeqc_error                     TRUE - if error is to be reported
**                                      when psjeqc is incompatible with
**                                      the output ordering pordeqc,
**                                      - psjeqc is to be preferred
**      inner_parent                    TRUE if the parent of the CO node
**                                      has the CO node as the inner...
**                                      in this case the pordeqc does not need
**                                      to be compatible.
**                                      so that the pordeqc ordering does not need
**                                      to be compatible
**
** Outputs:
**      ordering                        ptr to current inexact ordering
**                                      to be updated to an exact ordering
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-mar-88 (seputis)
**          initial creation
**      6-dec-88 (seputis)
**          return all eqcls if truncate is false
**      15-nov-89 (seputis)
**          fix optimizedb query problem, in which unnecessary sort nodes are
**	    generated below a key join, by ensuring that if a multi-attribute
**	    join ordering is provided, assume that this is a keyed lookup and
**	    that the resultant ordeqc needs to be compatible with this.
[@history_template@]...
*/
VOID
opo_mexact(
	OPS_SUBQUERY       *subquery,
	OPO_ISORT          *ordering,
	OPO_ISORT          psjeqc,
	bool		   valid_sjeqc,
	OPO_ISORT          pordeqc,
	bool               valid_ordeqc,
	bool		   truncate)
{
    OPE_IEQCLS	    maxeqcls;		/* maximum number of equivalence classes
                                        ** not including multi-attribute orderings */
    OPO_ISORT	    tempeqorder[2];	/* used to simulate an exact ordering
                                        ** with one inexact element */
    OPO_ISORT       tempordeqc[2];	/* used to simulate an exact ordering
                                        ** with one inexact element */
    OPO_ISORT       tempsjeqc[2];	/* used to simulate an exact ordering
                                        ** with one inexact element */
    OPO_ISORT       *cur_sortp;		/* ptr to current ordering being converted */
    OPO_SORT	    *currentp;		/* ptr to the multi-attribute ordering
                                        ** -needs to be converted to exact ordering
                                        ** without any embedded inexact orderings */
    OPO_ISORT       *ordeqcp;		/* ptr to current ordering of pordeqc being
                                        ** analyzed */
    OPO_ISORT       *sjeqcp;		/* ptr to current ordering of psjeqc being
                                        ** analyzed */
    OPE_BMEQCLS     *const_bm;		/* ptr to bitmap of constant
					** eqcls if it exists otherwise
                                        ** NULL */
    OPO_CONSTRUCT   exact_state;	/* Used to construct exact ordering */

    exact_state.opo_state = OPO_FEQCLS;	/* init state to find ordered join
                                        ** i.e. opo_sjeqc */
    exact_state.opo_notusable = FALSE;
    maxeqcls = subquery->ops_eclass.ope_ev;
    if (*ordering < maxeqcls)
	opx_error(E_OP0497_INCOMPATIBLE); /* only multi-attribute ordering
					** expected */
    currentp = subquery->ops_msort.opo_base->opo_stable[*ordering-maxeqcls];
    if (currentp->opo_stype == OPO_SINEXACT)
    {	/* create an exact ordering list of one element, now only
	** deal with an exact ordering list */
	tempeqorder[0] = *ordering;
	tempeqorder[1] = OPE_NOEQCLS;
	cur_sortp = &tempeqorder[0];
    }
    else
	cur_sortp = &currentp->opo_eqlist->opo_eqorder[0];

    if (pordeqc >= maxeqcls)
    {
	OPO_ISORT	*tempordp;
	tempordp =
	ordeqcp = &subquery->ops_msort.opo_base->opo_stable[pordeqc-maxeqcls]
	    ->opo_eqlist->opo_eqorder[0];
	/* consistency check - since ordering comes from parent, all
	** equivalence classes should not be multi-attribute */
	for (;*tempordp != OPE_NOEQCLS; tempordp++)
	    if (*tempordp >= maxeqcls)
		opx_error(E_OP0497_INCOMPATIBLE); 
    }
    else
    {	/* single or no EQCLS in ordering, as before simulate exact ordering list */
	tempordeqc[0] = pordeqc;
	tempordeqc[1] = OPE_NOEQCLS;
	ordeqcp = &tempordeqc[0];
    }

    if (psjeqc >= maxeqcls)
    {
	OPO_ISORT	*tempsjp;
	tempsjp =
	sjeqcp = &subquery->ops_msort.opo_base->opo_stable[psjeqc-maxeqcls]
	    ->opo_eqlist->opo_eqorder[0];
	/* consistency check - since ordering comes from parent, all
	** equivalence classes should not be multi-attribute */
	for (;*tempsjp != OPE_NOEQCLS; tempsjp++)
	    if (*tempsjp >= maxeqcls)
		opx_error(E_OP0497_INCOMPATIBLE); 
    }
    else
    {	/* single or no EQCLS in ordering */
	tempsjeqc[0] = psjeqc;
	tempsjeqc[1] = OPE_NOEQCLS;
	sjeqcp = &tempsjeqc[0];
    }

    if (subquery->ops_bfs.opb_bfeqc)
	const_bm = subquery->ops_bfs.opb_bfeqc;
    else
	const_bm = NULL;

    for (;(*cur_sortp) != OPE_NOEQCLS ; cur_sortp++)
    {
	if (*cur_sortp < maxeqcls)
	{   /* single attribute found */
	    if (const_bm)
	    {   /* skip all constant eqcls, so that they will be placed
		** at the end of any unordered set */
		if (BTtest((i4)*cur_sortp, (char *)const_bm))
		{
		    opo_aordering(subquery, &exact_state, *cur_sortp);
		    continue;
		}
		while ((*sjeqcp != OPE_NOEQCLS)
		    &&
		    BTtest((i4)*sjeqcp, (char *)const_bm))
		    sjeqcp++;			    /* previous consistency check
						    ** ensures all attributes are
						    ** < maxeqcls */
		while ((*ordeqcp != OPE_NOEQCLS)
		    &&
		    (*ordeqcp < maxeqcls)
		    &&
		    BTtest((i4)*ordeqcp, (char *)const_bm))
		    ordeqcp++;			    /* previous consistency check
						    ** ensures all attributes are
						    ** < maxeqcls */
	    }
	    if (*sjeqcp != OPE_NOEQCLS)
	    {
		if (*sjeqcp == *cur_sortp)
		{   /* found inside bitmap so add this to ordering */
		    if (*ordeqcp != OPE_NOEQCLS) 
		    {
			if (*ordeqcp == *sjeqcp)
			    ordeqcp++;
			else
			{
			    if (valid_ordeqc)
				opx_error(E_OP0497_INCOMPATIBLE); /* should 
					    ** be the same here */
				
			    ordeqcp = &tempordeqc[0]; /* the inner ordering 
						** is not really required by 
						** the parent */
			    tempordeqc[0] = OPE_NOEQCLS;
			}
		    }
		    opo_aordering(subquery, &exact_state, *sjeqcp);
		    sjeqcp++;
		    continue;
		}
		else
		{
		    if (valid_sjeqc)
			opx_error(E_OP0497_INCOMPATIBLE); /* cannot provide 
					    ** join ordering requested by 
					    ** parent */
		    sjeqcp = &tempsjeqc[0]; /* the key join ordering is not
					    ** really required by the parent */
		    tempsjeqc[0] = OPE_NOEQCLS;
		}
	    }
	    if (*ordeqcp != OPE_NOEQCLS)
	    {   /* sjeqc join eqcls is completely specified so continue processing
		** subsequent ordering requirements of the parent */
		if (*ordeqcp == *cur_sortp)
		{   /* found inside bitmap so add this to ordering */
		    opo_aordering(subquery, &exact_state, *ordeqcp);
		    ordeqcp++;
		    continue;
		}
		if (valid_ordeqc)
		    opx_error(E_OP0497_INCOMPATIBLE);	/* if this ordering is based on an
					** parent which requires this ordering
					** from the outer, then it must be
					** available or something went wrong */
	    }
	    if (truncate)
		break;			/* the remainder of the ordering is not
					** required by the parent */
	    opo_aordering(subquery, &exact_state, *cur_sortp); /* just copy since psjeqc, and
					** pordeqc are complete */
	}
	else
	{   /* an inexact multi-attribute ordering found */
	    OPE_BMEQCLS		inexact_map; /* current map of all equivalence
					** classes which have not been processed
					*/
	    bool		const_found;
	    OPE_BMEQCLS         *copy_order;

	    const_found = FALSE;
	    copy_order = NULL;
	    while ((*sjeqcp != OPE_NOEQCLS) || (*ordeqcp != OPE_NOEQCLS))
	    {	/* look at all bits in the unordered set */
		if (!copy_order)
		{
		    MEcopy((PTR)subquery->ops_msort.opo_base->
			opo_stable[*cur_sortp - maxeqcls]->opo_bmeqcls,
			sizeof(inexact_map), (PTR)&inexact_map);
		    copy_order = &inexact_map;
		}
		if (const_bm)
		{   /* skip all constant eqcls, so that they will be placed
                    ** at the end of any unordered set */
		    while ((*sjeqcp != OPE_NOEQCLS)
			&&
			BTtest((i4)*sjeqcp, (char *)const_bm))
			sjeqcp++;
		    while ((*ordeqcp != OPE_NOEQCLS)
			&&
			BTtest((i4)*ordeqcp, (char *)const_bm))
			ordeqcp++;
		}
		if (*sjeqcp != OPE_NOEQCLS)
		{
		    if (BTtest((i4)*sjeqcp, (char *)&inexact_map))
		    {	/* found inside bitmap so add this to ordering */
			if (*ordeqcp != OPE_NOEQCLS) 
			{
			    if (*ordeqcp == *sjeqcp)
				ordeqcp++; /* increment to keep in sync */
			    else
			    {
				if (valid_sjeqc && valid_ordeqc)
				    opx_error(E_OP0497_INCOMPATIBLE); /* should 
						** be the same here */
				else if (valid_ordeqc)
				{
				    /* this is an incompatible eqcls with
				    ** the ordering, but the parent does
                                    ** not need the join ordering on the outer
                                    ** for keying so prefer the ordering
				    ** ordeqc instead */
				    sjeqcp = &tempsjeqc[0];
				    tempsjeqc[0] = OPE_NOEQCLS;
				    continue;
				}
				else
				{
				    /* this is an incompatible eqcls with
				    ** the ordering, but the parent does
                                    ** not need the ordering on the inner
                                    ** for keying so prefer the joining
				    ** eqcls instead */
				    ordeqcp = &tempordeqc[0];
				    tempordeqc[0] = OPE_NOEQCLS;
				}
			    }
			}
			opo_aordering(subquery, &exact_state, *sjeqcp);
			BTclear((i4)*sjeqcp, (char *)&inexact_map);
			sjeqcp++;
			continue;
		    }
		    if (BTnext((i4)-1, (char *)&inexact_map, (i4)BITS_IN(inexact_map))
			< 0)
			break;		/* all bits in ordering have been
					** used */
		    const_found = 
			const_bm
			&&
			BTsubset((char *)&inexact_map, (char *)const_bm, 
			    (i4)BITS_IN(inexact_map));
		    if (const_found)
			break;		/* remainder of bits in ordering
					** are all constant eqcls */
		    if (valid_sjeqc)
			opx_error(E_OP0497_INCOMPATIBLE); /* cannot provide 
					** join ordering requested by parent */
		    sjeqcp = &tempsjeqc[0]; /* the key join ordering is not
					** really required by the parent */
		    tempsjeqc[0] = OPE_NOEQCLS;
		    continue;
		}
		if (*ordeqcp != OPE_NOEQCLS)
		{   /* sjeqc join eqcls is completely specified so continue processing
                    ** subsequent ordering requirements of the parent */
		    if (BTtest((i4)*ordeqcp, (char *)&inexact_map))
		    {	/* found inside bitmap so add this to ordering */
			opo_aordering(subquery, &exact_state, *ordeqcp);
			BTclear((i4)*ordeqcp, (char *)&inexact_map);
			ordeqcp++;
			continue;
		    }
		    if (BTnext((i4)-1, (char *)&inexact_map, (i4)BITS_IN(inexact_map))
			< 0)
			break;		/* all bits in ordering have been used*/
		    const_found = const_bm
			&&
			BTsubset((char *)&inexact_map, (char *)const_bm, 
			    (i4)BITS_IN(inexact_map));
		    if (const_found)
			break;				/* remainder of bits in ordering
							** are all constant eqcls */
		    if (valid_ordeqc)
			opx_error(E_OP0497_INCOMPATIBLE);
		    ordeqcp = &tempordeqc[0];
		    tempordeqc[0] = OPE_NOEQCLS;
		}
	    }
	    if (truncate && !const_found 
		&& 
		(*sjeqcp == OPE_NOEQCLS)
		&&
		(*ordeqcp == OPE_NOEQCLS))
	    {   /* required portions of parent ordering have been
		** calculated so exit */
		*ordering = opo_finish(subquery, &exact_state);
		return;
	    }
	    if (!copy_order)
	    {	/* parent orderings do not matter so just choose an
		** arbitrary order */
		copy_order = subquery->ops_msort.opo_base->
		    opo_stable[*cur_sortp - maxeqcls]->opo_bmeqcls;
		const_found = TRUE;
	    }
	    {   /* the remaining bit map contains only constant eqcls so add them
		** to the end of the ordering */
		OPE_IEQCLS		const_eqcls;
		for(const_eqcls = -1;
		    (const_eqcls = BTnext((i4)const_eqcls, (char *)copy_order,
			(i4)BITS_IN(inexact_map))) >= 0;)
		    opo_aordering(subquery, &exact_state, const_eqcls);
	    }
	}
    }
    *ordering = opo_finish(subquery, &exact_state);
}
