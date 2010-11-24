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
#include    <ex.h>
#include    <tm.h>
#define             OPN_ENUM            TRUE
#define             OPX_TMROUTINES      TRUE
#define             OPX_EXROUTINES      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNENUM.C - main entry point for enumeration
**
**  Description:
**      This file contains the main entry point for the enumeration phase
**      of the optimizer
**      FIXME - check the number of parameter nodes in the query tree header
**      and adjust the timeout factor if a repeat query is being optimized.
**      - need to readjust sort node cost formulas since duplicates are
**      not removed by a sort
**      - can reduce the search space by forcing indexes which restrict
**      less than 1%, or 1 tuple, automatically, and then enumerate on the
**      remainder; i.e. include obvious indexes.
**      - it would be nice to have a DB_SISAM_STORE storage structure which
**      be guaranteed to be sorted if keyed access was used, this would
**      avoid sort nodes in some cases; especially if few overflow chains
**      existed which the optimizer could detect.
**
**  History:    
**      8-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	03-jun-1996 (prida01)
**	    Cross integrate change for bug 52444 from 6.4
**	11-apr-1996 (angusm)
**	    Remove doubtful optimisation in opn_ovqp() which
**	    causes poor use of multiple work locations for sort. (bug 75831)
**      06-mar-96 (nanpr01)
**          Change the DB_PAGESIZE to DB_MINPAGESIZE. 
**      13-dec-1999 (thaju02)
**	    Modified opn_ovqp().
**	    Reapplied 11-apr-1996 (angusm) change for bug 75831; removed 
**	    questionable optimization, which lead to first pass of disk sort
**	    to use fewer work locations than necessary. 
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Deal with the halloween problem for rules triggered by updates.
**      03-Jul-2002 (huazh01)
**          Extend the fix for b100680. 
**          This fixes the halloween problem described in bug 108173, 
**          INGSRV / 1821. 
**       9-Sep-2003 (hanal04) Bug 110853 INGSRV 2504
**          Prevent delete cursors from looping unexpectedly when
**          rules fires inserts back into the table from which we
**          were deleting rows.
**      22-feb-2006 (stial01)
**          Try to fix b115765 for the simple cases where there are no rules,
**          or there are only system generated rules which do not trigger
**          inserts back into the table we are deleting from.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	30-Aug-2006 (kschendel)
**	    Eliminate redundant opo_topsort bool.
**	30-aug-2006 (thaju02)
**	    Check in opn_enum() if we are to use prefetch for 
**	    rule-triggering deletes. (B116355)
**	3-Jun-2009 (kschendel) b122118
**	    Rename rule-halloween to reflect what it actually does.
**	2-Dec-2009 (whiro01) Bug 122890
**	    Make "opn_nonsystem_rule" non-static so it can be called
**	    externally (corollary to Karl's massive Datallegro change).
[@history_line@]...
**/

/*{
** Name: opn_iemaps	- initialize global maps needed for enumeration
**
** Description:
**      Initialize maps needed if enumeration phase is entered, but which
**	are static during the enumeration phase
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
**      12-jan-93 (ed)
**          initial creation for bug 47377 - DMF enforced uniqueness
**	    used for top sort node elimination and join tuple estimates
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
*/
static VOID
opn_iemaps(
	OPS_SUBQUERY       *subquery)
{
    OPV_RT	*vbase;	    /* ptr to base of table descriptors */
    OPV_VARS	*varp;	    /* current table descriptor being analyzed */
    OPE_BMEQCLS	*mbfeqc;    /* mask of all equivalence classes
			    ** which are not constant */
    OPV_IVARS	varno;	    /* table id being analyzed */
    OPE_IEQCLS	maxeqcls;   /* number of equivalence classes in subquery*/
    OPZ_AT	*abase;	    /* ptr to base of attribute descriptors */

    if (subquery->ops_mask & OPS_IEMAPS)
	return;		    /* subquery has already been
			    ** processed */
    subquery->ops_mask |= OPS_IEMAPS;
    abase = subquery->ops_attrs.opz_base;
    maxeqcls = subquery->ops_eclass.ope_ev;
    if (subquery->ops_bfs.opb_bfeqc)
	mbfeqc = subquery->ops_bfs.opb_mbfeqc;
    else
	mbfeqc = NULL;
    vbase = subquery->ops_vars.opv_base;
    for (varno = subquery->ops_vars.opv_rv; --varno >= 0;)
    {
	OPE_BMEQCLS	    eqcmap; /* map of equivalence classes
			    ** which define the unique key */

	varp = vbase->opv_rt[varno];
	if (varp->opv_grv
	    &&
	    (	(varp->opv_grv->opv_created == OPS_FAGG)
		||
		(varp->opv_grv->opv_created == OPS_HFAGG)
		||
		(varp->opv_grv->opv_created == OPS_RFAGG)
	    ))
	{   /* function aggregate tuple can be uniquely identified
	    ** by the by list elements ... find the attribute
	    ** numbers of by list elements and translate these
	    ** to equivalence classes */
	    PST_QNODE	    *qnodep;
	    MEfill(sizeof(eqcmap), (u_char)0, (PTR)&eqcmap);
	    for (qnodep = varp->opv_grv->opv_gquery->ops_byexpr;
		qnodep && (qnodep->pst_sym.pst_type == PST_RESDOM);
		qnodep = qnodep->pst_left)
	    {
		if (qnodep->pst_sym.pst_value.pst_s_rsdm.pst_rsflags&PST_RS_PRINT)
		{
		    OPZ_DMFATTR	    dmfattr;
		    OPZ_IATTS	    attno;
		    attno = opz_findatt(subquery, varno, 
			(OPZ_DMFATTR)qnodep->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
		    if (attno < 0)
			break;	/* aggregate may be part of a view
				** and not have all attributes referenced */
		    BTset((i4)abase->opz_attnums[attno]->opz_equcls,
			(char *)&eqcmap);
		}
	    }
	    if (qnodep && (qnodep->pst_sym.pst_type == PST_RESDOM))
		continue;	/* all attributes of by list not
				** referenced so this key cannot be
				** used */
	}
	else if (varp->opv_grv
	    &&
	    varp->opv_grv->opv_relation
	    &&
	    (varp->opv_grv->opv_relation->rdr_rel->tbl_status_mask
             &
             DMT_UNIQUEKEYS)
	    &&
            (   varp->opv_grv->opv_relation->rdr_keys->key_count
		==
                varp->opv_mbf.opb_count
	    ))
	{   /* a unique key is specified in which all the attributes
	    ** are referenced in the query */
	    OPZ_IATTS	    key;

	    MEfill(sizeof(eqcmap), (u_char)0, (PTR)&eqcmap);
	    for( key=varp->opv_mbf.opb_count; --key >= 0;)
	    {
		OPZ_IATTS	attno;
		attno = varp->opv_mbf.opb_kbase->opb_keyorder[key].opb_attno;
		BTset((i4)abase->opz_attnums[attno]->opz_equcls,
		    (char *)&eqcmap);
	    }
	}
	else if (varp->opv_grv
	    &&
	    (varp->opv_grv->opv_created == OPS_SELECT))
	    {
		varp->opv_mask |= OPV_1TUPLE; /* mark flag indicating that
				** this relation will return exactly one
				** tuple ( in other words no duplicates
				** will be introduced), since it is subselect */
		continue;	
	    }
	else
	    continue;		/* no uniqueness found with this
				** variable */
	vbase->opv_rt[varp->opv_index.opv_poffset]->opv_mask |= OPV_UNIQUE; /*
				** mark primary table as having uniqueness */
	varp->opv_mask |= OPV_UNIQUE; /* mark relation as containing
				    ** a key which can be considered
				    ** unique by DMF */
	varp->opv_trl->opn_mask |= OPN_UNIQUECHECK; /* mark all enumeration
				** single variable descriptors has having
				** uniqueness if possible */

	if (mbfeqc)
	{   /* remove constant equivalence classes from list */
	    BTand((i4)maxeqcls, (char *)mbfeqc, (char *)&eqcmap);
	    if (BTnext((i4)-1, (char *)&eqcmap, (i4)maxeqcls)< 0)
	    {
		varp->opv_mask |= OPV_1TUPLE; /* mark flag indicating that
				** this relation will return exactly one
				** tuple */
		continue;	/* special case checks if all attributes
				** are constant equivalence classes */
	    }
	}
	
	{   /* check previously initialized cases, to avoid allocating
	    ** another equivalence class map */
	    OPV_IVARS	varno1;
	    for (varno1 = varno; ++varno1 < subquery->ops_vars.opv_rv;)
	    {
		OPE_BMEQCLS	*keqcmap;
		OPV_VARS	*varp1;

		varp1 = vbase->opv_rt[varno1];
		keqcmap = varp1->opv_uniquemap;
		if (keqcmap
		    &&
		    !MEcmp((PTR)&eqcmap, (PTR)keqcmap, sizeof(eqcmap))
		    )
		{
		    varp->opv_uniquemap = keqcmap;
		    break;
		}
	    }
	}
	if (!varp->opv_uniquemap)
	{   /* allocate map if previous check failed */
	    varp->opv_uniquemap = (OPE_BMEQCLS *)opu_memory(subquery->ops_global,
		(i4)sizeof(*(varp->opv_uniquemap)));
	    MEcopy((PTR)&eqcmap, sizeof(eqcmap), (PTR)varp->opv_uniquemap);
	}
    }
    for (varno = subquery->ops_vars.opv_rv; --varno>=0;)
    {	/* after completion, look at all variables and mark all secondary indexes
	** has having potential DMF uniqueness */
	varp = vbase->opv_rt[varno];
	if ((varno != varp->opv_index.opv_poffset)
	    &&
	    (vbase->opv_rt[varp->opv_index.opv_poffset]->opv_mask & OPV_UNIQUE))
	{
	    varp->opv_mask |= OPV_UNIQUE;
	    varp->opv_trl->opn_mask |= OPN_UNIQUECHECK;
	}
    }
}

/*{
** Name: opn_uniquecheck	- check for uniqueness of join
**
** Description:
**      Given enforced uniqueness from DMF on indexes, determine 
**      if uniqueness can be assumed given the joining set of equcls 
**      and the set of variables used in the subtree.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      vmap                            ptr to set of variables used to
**					form the join key
**      jeqcmap                         ptr to map of equcls which are in
**					the join set, if NULL then eqcls
**					parameter has meaning
**	eqcls				if jeqcmap is NULL then this is
**					the single attribute to be
**					checked
**
** Outputs:
**
**	Returns:
**	    TRUE - if each tuple is unique which respect to the jeqcmap
**	Exceptions:
**
** Side Effects:
**
** History:
**      24-jan-93 (ed)
**          initial creation for bug 47377 - DMF enforced uniqueness
**	    used for top sort node elimination and join tuple estimates
[@history_template@]...
*/
bool
opn_uniquecheck(
	OPS_SUBQUERY	*subquery,
	OPV_BMVARS	*vmap,
	OPE_BMEQCLS	*jeqcmap,
	OPE_IEQCLS	eqcls)
{
    OPV_IVARS	    varno;	    /* current variable being analyzed */
    OPV_IVARS	    maxvar;	    /* number of variables in subquery */
    OPV_RT	    *vbase;	    /* ptr to base of variable descriptors */
    OPE_IEQCLS	    maxeqcls;	    /* number of equivalence class in subquery */
    OPE_BMEQCLS	    temp_eqcmap;    /* map of one eqcls for convenience */

    if (!jeqcmap)
    {	/* create temp eqcmap, if only one eqcls is provided */
	MEfill(sizeof(temp_eqcmap), (u_char)0, (PTR)&temp_eqcmap);
	BTset((i4)eqcls, (char *)&temp_eqcmap);
	jeqcmap = &temp_eqcmap;
    }
    vbase = subquery->ops_vars.opv_base;
    maxvar = subquery->ops_vars.opv_rv;
    maxeqcls = subquery->ops_eclass.ope_ev;
    for (varno = -1;
	(varno = BTnext((i4)varno, (char *)vmap, (i4)maxvar))>=0;)
    {	/* traverse map and determine if every base relation has
	** a DMF enforced unique key */
	OPV_VARS	*varp;	    /* ptr to variable descriptor */
	OPV_IVARS	primary;    /* base relation associated with
				    ** variable */
	varp = vbase->opv_rt[varno];
	primary = varp->opv_index.opv_poffset;
	if (primary < 0)
	    opx_error(E_OP04AB_NOPRIMARY);
	if (vbase->opv_rt[primary]->opv_mask & OPV_1TUPLE)
	    continue;		/* unique key is equated to constants 
				** so it passes test */
	{
	    OPE_IEQCLS	    tideqc;
	    tideqc = vbase->opv_rt[primary]->opv_primary.opv_tid;
	    if ((tideqc >= 0)
		&&
		BTtest((i4)tideqc, (char *)jeqcmap))
		continue;	/* tids provide guaranteed unique
				** identifiers */
	}
	if (!(vbase->opv_rt[primary]->opv_mask & OPV_UNIQUE))
	{
	    return(FALSE);	/* no unique key exists for this base 
				** relation, so uniqueness of join can
				** not be guaranteed so terminate
				** search */
	}
	if ((varno != primary)
	    &&
	    BTtest((i4)primary, (char *)vmap))
	    continue;		/* if this is a secondary index and
				** the base relation is also in this
				** set then defer check until base
				** relation is found */
	if (varp->opv_uniquemap
	    &&
	    BTsubset((char *)varp->opv_uniquemap,
		(char *)jeqcmap, (i4)maxeqcls))
	    continue;		/* condition has been satisfied
				** by unique key of this relation */
	/* search remaining indexes associated with the primary
	** to identify unique keys */
	{
	    OPV_IVARS	    varno1;
	    for (varno1=maxvar; --varno1>=0;)
	    {
		OPV_VARS	*varp1;
		OPV_IVARS	primary1;
		varp1 = vbase->opv_rt[varno1];
		primary1 = varp1->opv_index.opv_poffset;
		if ((primary == primary1)
		    &&
		    (varno1 != varno)
		    &&
		    varp1->opv_uniquemap
		    &&
		    (	(varno == primary)
			||
			BTsubset((char *)varp1->opv_uniquemap,
			    (char *)&varp->opv_maps.opo_eqcmap, (i4)maxeqcls)
		    )
		    &&
		    BTsubset((char *)varp1->opv_uniquemap,
			(char *)jeqcmap, (i4)maxeqcls)
		    )
		    break;	/* found DMF unique key */
	    }
	    if (varno1 < 0)
	    	/* couldn't find unique key */
		return(FALSE);
	}
    }
    return(TRUE);
}

/*{
** Name: opn_ovqp	- check for a single variable query
**
** Description:
**	The routine name is somewhat of a misnomer.  It used to check
**	for a single-variable subquery that would not need enumeration.
**	However we now always do the regular enumeration stuff even
**	for a single variable, to get cost estimates, QEP's, etc.
**	This routine also does a bunch of setup stuff for top-sort
**	checking and potential elimination.
**
**	The only case where we now return "don't enumerate" is if
**	the subquery involves no range variables at all, ie it's just
**	an expression.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    TRUE - if a zero variable query has been found
**	    FALSE - to go ahead with enumeration
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-jul-86 (seputis)
**          initial creation
**	11-aug-88 (seputis)
**          move opj_cartprod call to opj_joinop 
**	6-nov-88 (seputis)
**          add timeout tracing
**	24-nov-88 (seputis)
**          look at dewitt performance problem
**          with aggregates
**	9-may-89 (seputis)
**          fix b5655, remove reference to rtuser
**	22-aug_89 (seputis)
**	    initialize opn_maxcache prior to calling opn_reformat
**	20-dec-90 (seputis)
**	    added support for top sort node removal
**	12-jan-91 (seputis)
**	    add code to limit size of sort node preallocation by DMF
**	    by defining OPN_DSORTBUF
**	21-jan-91 (jkb) added for seputis
**	    change exit condition for for loop so it works on UNIX
**	18-mar-91 (seputis)
**	    remove top sort node if select distinct is used and duplicates
**	    can be guaranteed to be eliminated
**	4-nov-91 (seputis)
**	    - fix bug 40715, if ORDER BY is on constant equivalence classes
**	    then select DISTINCT will not work.
**	18-dec-91 (seputis)
**	    - fix bug 41526 - duplicates need to be removed even if btree
**	    exists when count(distinct var) is used
**      26-dec-91 (seputis)
**          fix bug 41526 - if expressions exist in multi-variable aggregate
**          queries then an incorrect answer may be given due to incorrect
**          top sort node elimination
**	28-jan-92 (seputis)
**	    fix 41809 - check if btree exists and is possibly useful for
**	    top sort node removal for single variable query
**	16-feb-92 (seputis)
**	    fix 42532 - make sure order by was specified before optimizing
**	    away the order by list
**	14-apr-92 (seputis)
**	    fix b43311 - make sure eqcls does not appear twice in a sort
**	    list or a e_op0497 error may result.
**	6-apr-1992 (bryanp)
**	    Add support for PSQ_DGTT_AS_SELECT for temporary tables.
**      14-sep-92 (ed)
**          fix 44510 - for repeated queries which eventually may go thru
**          knowledge management, use enumeration for estimate always
*	11-apr-1996 (angusm)
**		remove doubtful optimisation, causing first pass of disk sort
**		to use fewer work locations than it needs (bug 75831).
**      06-mar-96 (nanpr01)
**          Change the DB_PAGESIZE to DB_MINPAGESIZE. Also initialize the
**	    scanblock factors. Use the width consistently.
**	18-dec-1997 (nanpr01)
**	    Division by zero in Unicenter query.
**	16-mar-1998 (hayke02)
**	    The previous change added a call to opn_pagesize(). We now only
**	    call opn_pagesize() if twidth is non-zero. This prevents a divide
**	    by zero error in opn_pagesize() (SEGV, E_OP0082). This change
**	    fixes bug 89702.
**	13-dec-1999 (thaju02)
**	    Reapplied 11-apr-1996 (angusm) change for bug 75831; removed 
**	    questionable optimization, which lead to first pass of disk sort
**	    to use fewer work locations than necessary. 
**	1-nov-99 (inkdo01)
**	    Do cost estimate for single variable queries, even without "set qep"
**	    for readahead estimates.
**	11-sep-01 (hayke02)
**	    Modify the new desc top sort removal code so that we now break
**	    from the qsortp loop if we have a desc sort node and OPO_MDESC
**	    is not set. This change fixes bug 105721.
**      14-sep-2001 (thaju02)
**	    Reapplied 11-apr-1996 (angusm) change for bug 75831; removed 
**	    questionable optimization, which lead to first pass of disk sort
**	    to use fewer work locations than necessary. 
**	27-Jun-2006 (kschendel)
**	    Update header comment, replace bizarre if test with what it
**	    really means today.
**	31-Aug-2006 (kschendel)
**	    Watch for HFAGG as well as RFAGG.
**	19-sep-2006 (dougi)
**	    Removed very long standing division by 2 of scanblocks which
**	    didn't appear to be justified.
**	6-feb-2008 (dougi)
**	    Guard against SEGV because of "distinct" on constant view.
**	14-Oct-2010 (kschendel)
**	    Delete weird sort test that looked at default result structure
**	    instead of actual result structure.  I think it was trying to
**	    test for heapsort, but who knows why.  We actually allow
**	    ctas into a heapsort, but it's done with a post-modify, not
**	    a sort on the query.
*/
static bool
opn_ovqp(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE           *global;            /* ptr to global state variable*/
    bool		dupsort;	    /* TRUE if sort required because
					    ** of duplicate handling */
    bool		rdupsort;	    /* TRUE if sort required because
					    ** of duplicate handling other
					    ** than a user defined DISTINCT */
    OPE_BMEQCLS		targetmap;	    /* map of eqcls in target list 
					    ** which appear in resdoms which
					    ** are not part of an expression */
    bool		noexpr;		    /* TRUE if no expression exists 
					    ** in the target list */
    i4			i;

    global = subquery->ops_global;          /* global state variable */

    /* we need a reformat node on top of the tree if this is a user-level
    ** query (not an intermediate query issued by aggregate processing)
    ** and it is a retrieval (not append, delete or replace) and
    ** (either a sort or reformat to (c)heapsort);  a retrieve
    ** unique to the terminal causes the sortflag to be set
    */
    rdupsort = (subquery->ops_duplicates == OPS_DREMOVE);
    if (rdupsort && subquery->ops_vars.opv_rv > 0)
    {
       i = subquery->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_created;
       rdupsort = (subquery->ops_vars.opv_rv != 1)
		||
		(   (i != OPS_FAGG)
		&&
		(i != OPS_HFAGG)
		    &&
		(i != OPS_RFAGG)
		);		/* check for special case of
				** function aggregate which is just
				** one variable, since duplicates do
				** not need to be removed because
				** BY values will enforce uniqueness, note
				** this case will occur with aggregate
				** variable substitution as in
				** retrieve (x=sum(r.a by r.b)) but if
				** user provided a restriction with "unique"
				** then pst_dups which catch that case and
				** duplicates will be removed */
    }

    dupsort = (subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_dups
	    == PST_NODUPS);		/* TRUE - if sort node required
					** on top of query plan to eliminate
                                        ** duplicates */
    if (dupsort || rdupsort)
    {	/* build map of eqcls in target list which are not part of an
	** expression, since sort nodes are required for duplicate removal
	** if uniqueness cannot be guaranteed in resdoms which are not
	** expressions */
	PST_QNODE	*qnodep;

	opn_iemaps(subquery);		/* initialize various maps
					** required during enumeration,
					** - used here to determine if
					** a distinct can be ignored since
					** all variables have unique ids
					** in the target list */
	MEfill(sizeof(targetmap), (u_char)0, (PTR)&targetmap);
	noexpr = TRUE;
	for (qnodep = subquery->ops_root->pst_left; 
	    qnodep 
	    && 
	    (qnodep->pst_sym.pst_type == PST_RESDOM);
	    qnodep = qnodep->pst_left)
	{
	    if ((qnodep->pst_right->pst_sym.pst_type == PST_VAR)
		&&
		(   qnodep->pst_sym.pst_dataval.db_datatype 
		    == 
		    qnodep->pst_right->pst_sym.pst_dataval.db_datatype
		)
		&&
		(   qnodep->pst_sym.pst_dataval.db_length
                    ==
                    qnodep->pst_right->pst_sym.pst_dataval.db_length
                ))
	    {
		OPE_IEQCLS  targeteqcls;
		
		targeteqcls = subquery->ops_attrs.opz_base->opz_attnums[
		    qnodep->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]
		    ->opz_equcls;
		BTset((i4)targeteqcls, (char *)&targetmap);
	    }
	    else if (qnodep->pst_right->pst_sym.pst_type != PST_CONST)
		noexpr = FALSE;
	}
	{
	    OPV_BMVARS	    vmap;
	    MEfill(sizeof(vmap), 0, (PTR)&vmap);
	    BTnot((i4)BITS_IN(vmap), (char *)&vmap);
	    if (opn_uniquecheck(subquery, &vmap, &targetmap, OPE_NOEQCLS))
	    {
		rdupsort = FALSE; /* sorting will not eliminate duplicates
				** since each relations has a DMF enforced
				** unique key */
		dupsort = FALSE;
		noexpr = FALSE;
	    }
	}
	if (dupsort)
	{   /* check the target list of the query to see if only var nodes exist
	    ** otherwise duplicate removal requires evaluating the expressions */
	    if (noexpr)
	    {   /* since duplicates can be eliminated by internal
		** sort nodes, change this to an rdupsort */
		dupsort = FALSE;
		rdupsort = TRUE;
		if (!subquery->ops_agg.opa_tidactive)
		{
		    subquery->ops_agg.opa_tidactive = TRUE;
		    MEfill(sizeof(subquery->ops_agg.opa_keeptids),
			(u_char)0,
			(PTR)&subquery->ops_agg.opa_keeptids);
		}
	    }
	}
    }
    if(	dupsort
	||
	(subquery->ops_sqtype == OPS_PROJECTION)
	||
	(
	    rdupsort
	    &&
	    (subquery->ops_mask & OPS_AGEXPR)   /* cannot depend on
					    ** duplicate removal if an expression
					    ** exists in target list */
	)
	)
    {
	dupsort |= rdupsort;
	subquery->ops_msort.opo_mask |= OPO_MTOPSORT;
    }
    else
    {
	/* check for cases in which top sort node can be removed */
	OPO_EQLIST	    templist;
	i4		    sort_count;
	OPZ_AT		    *abase;
	bool		    possible;	    /* TRUE if it is possible
					    ** to eliminate the sort node */
	bool		    order_by;	    /* TRUE if order by list was
					    ** analyzed */
	order_by = FALSE;
	abase = subquery->ops_attrs.opz_base;
	sort_count = 0;
	if ((subquery->ops_sqtype == OPS_FAGG)
	    ||
	    (subquery->ops_sqtype == OPS_RFAGG)
	    )
	{   /* search the group by list to determine which attributes
	    ** need to be sorted */

	    PST_QNODE	    *byexprp;

	    order_by = TRUE;
	    possible = TRUE;
	    subquery->ops_msort.opo_mask |= OPO_MTOPSORT;
	    for (byexprp = subquery->ops_byexpr; 
		byexprp && (byexprp->pst_sym.pst_type == PST_RESDOM); 
		byexprp = byexprp->pst_left)
	    {
		if(sort_count >= DB_MAXKEYS)  /* cannot handle more 
					** than this, FIXME create a
					** enumeration temporary array to fix this */
		{
		    possible = FALSE;
		    break;
		}
		if (byexprp->pst_right->pst_sym.pst_type == PST_VAR)
		{
		    templist.opo_eqorder[sort_count] = 
			abase->opz_attnums[byexprp->pst_right->pst_sym.
			    pst_value.pst_s_var.pst_atno.db_att_id]->opz_equcls;
		    if ((   subquery->ops_bfs.opb_bfeqc
			    && 
			    BTtest ((i4)templist.opo_eqorder[sort_count],
				(char *)subquery->ops_bfs.opb_bfeqc)
			)
			||
			(   (sort_count > 0)
			    &&
			    (templist.opo_eqorder[sort_count] 
			     ==
			     templist.opo_eqorder[sort_count-1]
			    )	/* ignore equivalence classes which are
				** adjacent */
			))
			sort_count--; /* ignore equivalence classes
				** which are constant since they will
				** always be sorted */
		}
		else
		{
		    possible = FALSE;
		    break;
		}
		sort_count++;
	    }
	    {
		OPO_ISORT	    count1;
		OPO_ISORT	    count2;
		/* the default sort order is inverted from this list in OPC
		** so invert the list to be used for ordering, FIXME there is
		** a build in assumption that OPC/QEF will sort the resdoms
		** in this order */
		for (count1 = sort_count-1, count2=0; count1 > count2; count1--, count2++)
		{
		    OPO_ISORT	    save;
		    save = templist.opo_eqorder[count1];
		    templist.opo_eqorder[count1] = templist.opo_eqorder[count2];
		    templist.opo_eqorder[count2] = save;
		}
		for (count1 = 0; count1 < sort_count; count1++)
		{   /* check for multiple occurances of same eqcls
		    ** which would not be possible to use given current
		    ** OPF model */
		    for (count2 = count1+1; count2 < sort_count; count2++)
		    {
			if (templist.opo_eqorder[count1] == templist.opo_eqorder[count2])
			    sort_count = count2;
		    }
		}
	    }
	}    
	else if (
		(subquery->ops_sqtype == OPS_MAIN) /*
					    ** TRUE - if this is the root of the
					    ** user query (not an aggregate)
					    */
		&&
		(global->ops_qheader->pst_stflag) /* TRUE - if sort 
					    ** list for user query */
	    )
	{
	    PST_QNODE	    *qsortp;
	    bool	    descsort;
	    OPV_IVARS	    firstdvar, nextdvar;
	    descsort = global->ops_qheader->pst_stlist &&
		!(global->ops_qheader->pst_stlist->pst_sym.pst_value.
							pst_s_sort.pst_srasc);
	    if (descsort) subquery->ops_msort.opo_mask |= OPO_MDESC;
	    subquery->ops_msort.opo_mask |= OPO_MTOPSORT;
	    order_by = TRUE;
	    firstdvar = nextdvar = OPV_NOVAR;
	    /* get list of equivalence classes which are the target ordering */
	    for (qsortp = global->ops_qheader->pst_stlist; qsortp; qsortp = qsortp->pst_right)
	    {
		PST_QNODE	    *resdomp;
		bool	    found;
		descsort &= !(qsortp->pst_sym.pst_value.pst_s_sort.pst_srasc);
		if (!(qsortp->pst_sym.pst_value.pst_s_sort.pst_srasc))
		{
		    for (resdomp = subquery->ops_root->pst_left; 
			resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM);
			resdomp = resdomp->pst_left)
		    {
			if ((qsortp->pst_sym.pst_value.pst_s_sort.pst_srvno
			    ==
			    resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
			    &&
			    resdomp->pst_right
			    &&
			    (resdomp->pst_right->pst_sym.pst_type == PST_VAR))
			{
			    if (firstdvar == OPV_NOVAR)
				firstdvar = resdomp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
			    else
				nextdvar = resdomp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
			    break;
			}
		    }
		}
		if ((qsortp->pst_sym.pst_value.pst_s_sort.pst_srasc && 
		    subquery->ops_msort.opo_mask & OPO_MDESC)
		    ||
		    ((!qsortp->pst_sym.pst_value.pst_s_sort.pst_srasc) &&
		    !(subquery->ops_msort.opo_mask & OPO_MDESC))
		    ||		    /* cannot deal with mixed ordering */
		    ((firstdvar != OPV_NOVAR) && (nextdvar != OPV_NOVAR) &&
		    (firstdvar != nextdvar))
		    ||
		    (sort_count >= DB_MAXKEYS)  /* cannot handle more 
					** than this, FIXME create a
					** enumeration temporary array to fix this */
		    )
		    break;
		found = FALSE;
		for (resdomp = subquery->ops_root->pst_left; 
		    resdomp && (resdomp->pst_sym.pst_type == PST_RESDOM); 
		    resdomp = resdomp->pst_left)
		{
		    if (resdomp->pst_sym.pst_value.pst_s_rsdm.pst_rsno
			==
			qsortp->pst_sym.pst_value.pst_s_sort.pst_srvno)
		    {
			/* found the correct target list item to sort on, check
			** if it maps directly to an equivalence class, FIXME -
			** create function attributes which can be sorted for
			** expressions */
			if (resdomp->pst_right->pst_sym.pst_type == PST_VAR)
			{
			    OPE_IEQCLS	    add_eqcls;	/* eqcls which has been added */
			    OPO_ISORT	    tempcount; /* search list of attributes to
							** be sorted */
			    add_eqcls = 
			    templist.opo_eqorder[sort_count] = 
				abase->opz_attnums[resdomp->pst_right->pst_sym.
				    pst_value.pst_s_var.pst_atno.db_att_id]->opz_equcls;
			    for (tempcount = sort_count; tempcount--;)
			    {
				/* search list for identical equivalence class since
				** if same eqc is referenced then the subsequent
				** attributes are insignificant , FIXME, for outer
				** joins a null may be present even if it is the
				** same eqcls, but the tuple should be eliminated
				** anyways since NULL != NULL, check this case for
				** a null join however. */
				if (templist.opo_eqorder[tempcount] == add_eqcls)
				{
				    sort_count--;
				    break;
				}
			    }
			    if ((tempcount < 0)	/* do not decrement twice */
				&&
				subquery->ops_bfs.opb_bfeqc
				&& 
				BTtest ((i4)templist.opo_eqorder[sort_count],
				    (char *)subquery->ops_bfs.opb_bfeqc))
				sort_count--; /* ignore equivalence classes
					** which are constant since they will
					** always be sorted */
			    found = TRUE;
			}
			break;
		    }
		}
		if (!found)
		    break;
		sort_count++;
	    }
	    possible = (qsortp == (PST_QNODE *)NULL); /* if the end of the
					** order by list is reached successfully
					** then the sort node can be removed */
	    if (!sort_count || !descsort) 
			subquery->ops_msort.opo_mask &= ~OPO_MDESC;
					/* show descending sort */
	}
	else
	    possible = rdupsort;
	if (rdupsort)
	{
	    if (possible)
	    {
		OPE_IEQCLS	maxeqcls;
		OPV_IVARS	vno;
		OPV_RT		*vbase;	    /* ptr to base of a array of
					    ** ptrs to range variables */
		vbase = subquery->ops_vars.opv_base;
		maxeqcls = subquery->ops_eclass.ope_ev;
		subquery->ops_msort.opo_mask |= OPO_MRDUPS; /* sort node can be eliminated
					    ** if FSM sort can eliminate duplicates,
					    ** check if this is compatible with other
					    ** conditions which require a top sort node */
		subquery->ops_tonly = (OPE_BMEQCLS *)opu_memory(global, 
			sizeof(*subquery->ops_tonly));
		MEcopy((PTR)&targetmap, sizeof(*subquery->ops_tonly),
		    (PTR)subquery->ops_tonly);
		/* the relations for which duplicates are to be kept provides a
		** a set of attributes which can be checked against internal sort
		** nodes... if a unique internal sort node is a subset of this then
		** no duplicates will be introduced by relations in the subtree
		** of that internal sort node */
		if (subquery->ops_agg.opa_tidactive)
		{
		    for (vno = subquery->ops_vars.opv_prv; --vno >= 0;)
		    {
			OPV_IGVARS		gvar;

			gvar = vbase->opv_rt[vno]->opv_gvar;
			if ((gvar >= 0) 
			    && 
			    BTtest((i4)gvar, (char *)&subquery->ops_agg.opa_keeptids))
			{
			    BTor((i4)maxeqcls, (char *)&vbase->opv_rt[vno]->opv_maps.opo_eqcmap,
				    (char *)subquery->ops_tonly);
			}
		    }
		}
	    }
	    subquery->ops_msort.opo_mask |= OPO_MTOPSORT;
	    dupsort = rdupsort;
	}
	if (possible && order_by)
	{
	    /* sort node can be eliminated if proper ordering exists */
	    if (!sort_count)
	    {
		/* this can happen with constant equivalence classes so
		** convert this to a non-sorted query */
		if (subquery->ops_sqtype == OPS_MAIN)
		{
		    global->ops_qheader->pst_stlist = (PST_QNODE *)NULL;
		    global->ops_qheader->pst_stflag = FALSE;
		}
		if (!rdupsort)
		{   /* do not need sort node if order by without
		    ** a distinct is used */
		    subquery->ops_msort.opo_mask &= (~OPO_MTOPSORT);
		}
	    }
	    else
	    {
		i4		size;
		size = sizeof(OPO_ISORT) * (sort_count + 1);
		subquery->ops_msort.opo_tsortp = (OPO_EQLIST *)opu_memory(global,
					(i4)size);
		MEcopy( (PTR)&templist, size, (PTR)subquery->ops_msort.opo_tsortp);
		subquery->ops_msort.opo_tsortp->opo_eqorder[sort_count] = OPE_NOEQCLS;
					/* terminate list */
		subquery->ops_msort.opo_mask |= OPO_MREMOVE; /* all attributes are
					** present so remove top sort node if ordering
					** exists */
	    }
	}
    }


    for (i=0; i < DB_NO_OF_POOL; i++)
    {
      global->ops_estate.opn_sblock[i] = 
	      global->ops_cb->ops_alter.ops_scanblocks[i];
				    /* removed division by 2 allegedly 
				    ** suggested by Derek F in prehistoric
				    ** times, but which, at least for table
				    ** scans, overestimates DIO by (surprise)
				    ** a factor of 2 */
      if (global->ops_estate.opn_sblock[i] < 1.0)
	  global->ops_estate.opn_sblock[i] = 1.0;
				    /* formerly this was 
				    ** DB_HFBLOCKSIZE/DB_PAGESIZE 
				    ** ... number of blocks 
				    ** per I/O in a hold file or
				    ** sort file scan */
    }
    {
	i4		first;      /* plan to timeout */
	i4		second;     /* subquery plan to timeout */

	if (global->ops_cb->ops_check)
	{
	    if (opt_svtrace( global->ops_cb, OPT_F123_SCAN, &first, &second)
		&&
		first>0)
    		for (i=0; i < DB_NO_OF_POOL; i++)
		  global->ops_estate.opn_sblock[i] = first; 
					/* number of pages per
					** DIO when scanning a relation */
	}
    }

    if ((subquery->ops_sqtype == OPS_MAIN)
	&&
	dupsort
	&&
	(   (global->ops_gdist.opd_gmask & OPD_TOUSER)
	    ||
	    (global->ops_qheader->pst_mode == PSQ_RETINTO)
	    ||
	    (global->ops_qheader->pst_mode == PSQ_DGTT_AS_SELECT)
	    ||
	    (global->ops_qheader->pst_mode == PSQ_APPEND)
	))
	global->ops_gdist.opd_gmask |= OPD_DISTINCT; /* this flag indicates to
					** distributed OPC that a SELECT DISTINCT
					** is required for the main query if
					** a SELECT can be used */

    /* If any vars in the subquery at all, enumerate */
    if (subquery->ops_vars.opv_rv > 0)
        return(FALSE);
    
    if ((global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	&&
	(subquery->ops_dist.opd_cost))
    {
	/* this would only be entered for constant queries as in the case
	** of b21822 */
	subquery->ops_dist.opd_cost->opd_intcosts
	    [global->ops_gdist.opd_coordinator] = 1.0; /* assign an
				    ** arbitrary cost for this constant query
				    ** so that it is executed on the coordinator
				    ** site, all other sites should be infinite */
    }
    subquery->ops_timeout = FALSE;  /* only one plan possible to no
				    ** timeout */
    subquery->ops_cost = 0.0;	    /* initialize cost
				    ** of "best plan found"
				    ** - since OVQP should go fast
				    ** histogram processing and cost
				    ** estimation is bypassed
				    */
    global->ops_estate.opn_tidrel = FALSE;  /* set TRUE if sort node on TID
					    ** required for query plan */
    for (i = 0; i < DB_NO_OF_POOL; i++)
      global->ops_estate.opn_maxcache[i] = 0.0;  
					    /* initialize prior to calling
					    ** opn_createsort */
    subquery->ops_bestco = NULL;	    /* init field to indicate query tree
                                            ** is really an expression to be
                                            ** evaluated */
    return(TRUE);
}

/*{
** Name: opn_halloween	- handle case of halloween problem
**
** Description:
**      Check for halloween problem, and return bitmap if found 
**
** Inputs:
**      subquery                        subquery state for main query
**
** Outputs:
**	Returns:
**	    PTR to halloween problem bitmap, map of range variable to
**	    be updated
**	Exceptions:
**
** Side Effects:
**
** History:
**      16-jan-89 (seputis)
**          initial creation
**	14-apr-94 (ed)
**	    bug 59679 - a delete query was a self join which caused tuples
**	    to be deleted which selected other tuples in the same relation
**	    causing fewer tuples than expected to be deleted, so if a delete
**	    exists which references the target variable in the query then
**	    place a top sort node as a hold file prior to deleting any
**	    tuples
**	03-jun-96 (prida01)
**	    Cross integrate change for bug 52444 from 6.4. Change for 
**	    ambiguous replace errors.
**       9-Sep-2003 (hanal04) Bug 110853 INGSRV 2504
**          A delete query which fires rules which insert back into
**          the same table needs a prefetch despite opv_rv <= 1.
**	25-aug-06 (dougi)
**	    Slight change to permit pre-sort of delete's for HEAP, HASH
**	    because it can speed them up.
**	10-May-07 (kibro01) b118289
**	    Only check storage type when we know this is a delete query,
**	    so we know it exists.
**      01-aug-2008 (huazh01)
**          modify the fix to bug 115765. We now use the 'resultp' to 
**          look up the table storage information. 
**          bug 120680.
*/
static OPV_BMVARS *
opn_halloween(
	OPS_SUBQUERY       *subquery)
{
    OPV_BMVARS		*return_val;	/* bit map of range variable to
					** be updated */
    DB_TAB_ID		resultrel;	/* table ID of the result
					** relation to be updated */
    DMT_TBL_ENTRY	*resultp;	/* ptr to DMF descriptor of the
					** relation to be updated */
    OPV_RT              *vbase;		/* ptr to base of a array of
					** ptrs to range variables */
    OPV_IVARS           varno;		/* range var number of base
					** relation referenced in query */
    OPS_STATE		*global;	/* global state variable */
    i2			storage_type;
    bool		delete_query;   /* TRUE if this is a delete query */
    global = subquery->ops_global;
    return_val = (OPV_BMVARS *) opu_memory( global, sizeof(OPV_BMVARS));
					/* allocate space for bit map
					** of range variables in table
					** to be updated , use non -enum
					** memory */
    MEfill( sizeof(OPV_BMVARS), (u_char)0, (PTR)return_val); /* init bit 
					** vector to indicate no multiple
					** range variables */
    global->ops_estate.opn_sortnode = (PST_QNODE *) opu_memory( global,
	sizeof(PST_QNODE));		/* allocate space out of non-enum
					** memory for possible sort node */
    resultp = global->ops_rangetab.opv_base->
	opv_grv[subquery->ops_global->ops_qheader->pst_restab.pst_resvno]->
	opv_relation->rdr_rel;
    resultrel.db_tab_base = resultp->tbl_id.db_tab_base;
    resultrel.db_tab_index =  resultp->tbl_id.db_tab_index;
    vbase = subquery->ops_vars.opv_base;
    for ( varno = 0; varno < subquery->ops_vars.opv_prv; varno++)
    {   /* for each base relation referenced in query, see if it 
	** references the relation to be updated */
	OPV_GRV		*gvarp;	    /* global range variable element
					** associated with joinop
					** range variable */
	gvarp = vbase->opv_rt[varno]->opv_grv;
	if (gvarp->opv_relation)
	{	/* RDF info pointer exists, so this is not a temp table */
	    if (    (gvarp->opv_relation->rdr_rel->tbl_id.db_tab_base
		    ==
		    resultrel.db_tab_base)
		&&
		    (gvarp->opv_relation->rdr_rel->tbl_id.db_tab_index
		    ==
		    resultrel.db_tab_index)
		)
		BTset( (i4)varno, (char *)return_val);
	}
    }
    delete_query = (subquery->ops_global->ops_qheader->pst_mode == PSQ_DELETE);
    if (delete_query)
    {
	/* Don't check storage type unless this is a delete query.
	** create procedure, for example, has no storage type
	** (kibro01) b118289
	*/
        storage_type = resultp->tbl_storage_type;
    }

    /* Check for simple delete */
    if (delete_query &&
	BTcount((char *)return_val, (i4)subquery->ops_vars.opv_rv)<=1 &&
	storage_type != DB_HASH_STORE && storage_type != DB_HEAP_STORE)
    {
	/* If no rules, no halloween problem */
	if (global->ops_statement->pst_after_stmt == NULL)
	    return (NULL);

	/* Check for rules which may cause halloween problem */
	if (!opn_nonsystem_rule(subquery)) 
	    return (NULL);
    }
    if ( delete_query
	 ||
	(subquery->ops_global->ops_qheader->pst_mode == PSQ_REPLACE))
    {
	subquery->ops_msort.opo_mask |= OPO_MTOPSORT;
	if ((subquery->ops_root->pst_sym.pst_value.pst_s_root.pst_qlang == DB_QUEL) ||
	    (subquery->ops_global->ops_qheader->pst_mask1 & PST_UPD_CONST))

	    subquery->ops_duplicates = OPS_DREMOVE; 
	/* remove duplicates in all
	** cases in which there is more than
	** one primary relation, 
    ** allowed for QUEL, or SQL cross-table update where only 
    ** constant values are used.
    ** FIXME - this is a kludge which is removed for SQL and relaxed 
    ** later for QUEL (for QUEL we can check that the joining attribute
    ** to "foreign tables" is on an attribute with a unique index
    ** - this kludge is needed to  ensure that replaces to the same tid
    ** with the same set of update values, does not cause an error, since
    ** DMF will not allow any more reads when the tuple has been updated
    ** even though it may not change the tuple values */
    }
    return(return_val);
}

/*{
** Name: opd_byposition	- TID handling for gateways
**
** Description:
**      This routine will set up data structures needed to enforce the rule 
**      for gateways which requires by-position updates of any relation.  This is 
**      done by placing the target relation on the outer in all plans, and 
**      making sure that no sort nodes occur between it and the top of the 
**      plan. 
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
**      16-jan-89 (seputis)
**          initial creation
[@history_template@]...
*/
static VOID
opd_byposition(
	OPS_SUBQUERY       *subquery)
{
    OPV_BMVARS	    *target;
    OPV_IVARS	    varno;
    OPV_IVARS	    targetno;

    targetno = subquery->ops_localres;
    if (targetno < 0)
	opx_error(E_OP0A85_UPDATE_VAR);	/* expecting update var to be 
					** defined */
    if (subquery->ops_vars.opv_base->opv_rt[targetno]->opv_grv->opv_relation->rdr_rel
	->tbl_status_mask & DMT_GATEWAY)
    {
	subquery->ops_mask |= OPS_BYPOSITION; /* limit search space
					    ** so that result relation is always
					    ** the outer with no sort nodes */
	target = (OPV_BMVARS *)opu_memory(subquery->ops_global, (i4)sizeof(*target));
	BTset((i4)targetno, (char *)target); /* include target relation */

	for (varno = subquery->ops_vars.opv_rv; --varno >= 0;)
	{	/* traverse range table and set bits for any index on the
	    ** target relation */
	    OPV_VARS	*varp;
	    varp = subquery->ops_vars.opv_base->opv_rt[varno];
	    if (varp->opv_index.opv_poffset == targetno)
		BTset ((i4)varno, (char *)target);	/* set bit since this
					    ** is either the base relation or
					    ** an index on it */
	}
	subquery->ops_global->ops_gdist.opd_byposition = target;
    }
}

/*{
** Name: opn_ienum	- initialization of enumeration variables
**
** Description:
**      This routine is used to initialize the enumeration phase variables
**
** Inputs:
**      global                          ptr to global state variable
**          .ops_estate                 enumeration state variable
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
**	10-may-86 (seputis)
**          initial creation
**	2-oct-88 (seputis)
**          use non-enum memory for opn_halloween
**	2-nov-88 (seputis)
**          Changed CSstatistics interface
**	14-aug-89 (seputis)
**          fix for b6538, opn_sfree
**	14-may-90 (seputis)
**	    b21582 - timeout for unix
**	18-apr-91 (seputis)
**	    b36920 - floating point exception handling
**	12-aug-93 (swm)
**	    Cast first parameter of CSaltr_session() to CS_SID to match
**	    revised CL interface specification.
**	06-mar-96 (nanpr01)
**	    Initialize all the maxcache values.
**	17-nov-00 (inkdo01)
**	    Check for need to use prefetch strategy for rule-triggering 
**	    updates (bug 100680).
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Moved inkdo01's check from opn_ienum() to opn_enum() as we
**          we will not call opn_ienum() if opn_ovqp() returns TRUE.
**	30-aug-2006 (thaju02)
**	    For delete, call opn_halloween() from opn_enum() where check
**	    we will test for prefetch instead of here. (B116355)
**	24-oct-07 (hayke02)
**	    Initialize opd_tcost and opd_tbestco to NULL (as we already do in
**	    opd_recover()) after we opu_allocate() opn_streamid, as these two
**	    temporary areas are allocated from enumeration memory. This
**	    prevents stale pointers being used when opn_streamid is allocated
**	    from a previously deallocated stream. This change fixes bug 119088.
[@history_line@]...
*/
static VOID
opn_ienum(
	OPS_SUBQUERY	    *subquery)
{
    OPS_STATE           *global;	/* ptr to global state variable */
    i4			i;

    global = subquery->ops_global;	/* get ptr to global state variable */

    global->ops_estate.opn_singlevar = (subquery->ops_vars.opv_prv == 1); /* 
			            ** TRUE if only one
                                    ** primary variable in subquery */
    global->ops_estate.opn_cbstreamid = NULL; /* initialize streamid's here
                                    ** in case any errors occur in allocate
                                    ** routines */
    global->ops_estate.opn_jtreeid = NULL; /* ditto */
    global->ops_estate.opn_streamid = opu_allocate(global); /* get a new memory
                                    ** stream for the enumeration phase */
    global->ops_estate.opn_jtreeid = opu_allocate(global); /* get a new memory
                                    ** stream for the OPN_JTREE structures which
                                    ** will describe the structurally unique
                                    ** trees */
    global->ops_estate.opn_sroot = (OPN_JTREE *) NULL;
    global->ops_estate.opn_cbstreamid = opu_allocate(global); /* get a new 
                                    ** memory stream for the temporary 
                                    ** cell boundary array used in enumeration*/
    global->ops_estate.opn_sfree = NULL; /* init the OPN_SDESC free list */
    global->ops_estate.opn_rlsfree = NULL; /* init the RLS free list */
    global->ops_estate.opn_eqsfree = NULL; /* init the EQS free list */
    global->ops_estate.opn_stfree = NULL; /* init the SUBTREE free list */
    global->ops_estate.opn_cofree = NULL; /* init the CO free list */
    global->ops_estate.opn_hfree = NULL; /* init the HISTOGRAM free list */
    global->ops_estate.opn_ccfree = NULL; /* init the cell count free list */
    global->ops_estate.opn_cbptr = NULL; 
    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
	global->ops_gdist.opd_tcost = NULL;
	global->ops_gdist.opd_tbestco = NULL;
    }
    for (i = 0; i < DB_NO_OF_POOL; i++)
      global->ops_estate.opn_maxcache[i] = 0.0; 
					  /* size of DMF cache in blocks */
    global->ops_estate.opn_cbsize = 0;
    global->ops_estate.opn_swcurrent = 0; /* no subtrees have been placed
					** in the saved work area */
    global->ops_estate.opn_search = FALSE; /* TRUE - if a garbage collection
                                        ** of memory has occurred - reset
                                        ** everytime a new best CO tree is found
                                        ** which may obsolete more saved work
                                        ** structures */
    global->ops_estate.opn_tidrel = FALSE; /* set TRUE if sort node on TID
					** required for query plan */
    global->ops_estate.opn_cscount = 0;	/* used to timeout for unix */
    global->ops_estate.opn_halloween = NULL; /* NULL indicates that no
					** check is needed for the halloween
					** problem */
    if (subquery->ops_sqtype == OPS_MAIN)
    {
	switch (global->ops_qheader->pst_mode)
	{
	case PSQ_DEFCURS:
	{
	    if (global->ops_cb->ops_server->opg_qeftype == OPF_GQEF)
	    {
		if ((global->ops_qheader->pst_updtmode != PST_READONLY)
		    ||
		    global->ops_qheader->pst_delete)	 /* check
					** check if there is possibility of update */
		    opd_byposition(subquery); /* initialize by-position variables */
	    }
	    break;
	}
	case PSQ_REPLACE:
	{
	    if ((subquery->ops_vars.opv_prv > 1) /* if only one base relation
					    ** then DEFERRED problem will not
					    ** occur - only main query will 
					    ** cause the update to occur */
		||
		(global->ops_qheader->pst_updtmode != PST_DEFER) /* for DIRECT the halloween
					    ** problem will occur, but DMF
					    ** should handle the DEFER case */
		)
		global->ops_estate.opn_halloween = opn_halloween(subquery);
	    if (global->ops_cb->ops_server->opg_qeftype == OPF_GQEF)
		opd_byposition(subquery); /* initialize by-position variables */
	    break;
	}
	case PSQ_DELETE:
	{
	    if (global->ops_cb->ops_server->opg_qeftype == OPF_GQEF)
		opd_byposition(subquery); /* initialize by-position variables */
	    break;
	}
	}
    } 

    global->ops_estate.opn_sbase = (OPN_ST *) opn_memory( global,
	(i4) sizeof( OPN_ST )); /* get memory for savework */
    MEfill( sizeof(OPN_ST), (u_char)0, 
	(PTR)global->ops_estate.opn_sbase->opn_savework); /*
                                            ** init all ptrs to NULL */
    subquery->ops_cost = OPO_LARGEST;	    /* initialize cost
                                            ** of "best plan found" with
                                            ** max float value so that first
                                            ** valid plan will override this
                                            */
    subquery->ops_bestco = NULL;	    /* no CO tree found so far */
    subquery->ops_lastlaloop = FALSE;	    /* init flag for new enumeration */
    global->ops_gmask &= (~(OPS_FLSPACE|OPS_FLINT)); 
					    /* reset flag which indicates
					    ** that the search space was
					    ** reduced due to floating point
					    ** exceptions */
    global->ops_gmask |= OPS_BFPEXCEPTION;  /* this flag indicates that 
					    ** floating point exceptions
					    ** occurred during evaluation
					    ** of the best plan, it has the
					    ** effect of doing an EXCONTINUE
					    ** if floating point exceptions
					    ** occur, which is desired until
					    ** a plan is found which does not
					    ** have floating point exceptions */
    {
	TIMERSTAT	timer_block;
	STATUS		cs_status;
	i4		turn_on;	    /* CSaltr_session expects ptr to
					    ** i4  */

	turn_on = TRUE;			    /* turn on statistics */
	if (!global->ops_estate.opn_statistics)
	    				    /* statistics have already been
					    ** turned on */
	{
	    if (cs_status = CSaltr_session((CS_SID)0, CS_AS_CPUSTATS, (PTR)&turn_on))
	    {	/* FIXME - define a new CL routine which counts time quantums
		** instead since it will be more efficient, and less error
		** prone to do that */
		if (cs_status == E_CS001F_CPUSTATS_WASSET)
		    global->ops_estate.opn_reset_statistics = FALSE; /* indicates that
					    ** statistics should be reset after
					    ** optimization, since it may be
					    ** expensive to keep accounting
					    ** in QEF */
		else
		    opx_verror(E_DB_ERROR, E_OP0490_CSSTATISTICS, 
			(OPX_FACILITY)cs_status);
	    }
	    else
	    {
		global->ops_estate.opn_reset_statistics = TRUE; /* indicates that
					    ** statistics should be reset */
	    }
	    global->ops_estate.opn_statistics = TRUE;  /* statistics have been turned on
					    */
	}
	if (cs_status = CSstatistics(&timer_block, (i4)0))
	{   /* get cpu time used by session from SCF */
	    opx_verror(E_DB_ERROR, E_OP0490_CSSTATISTICS, 
		(OPX_FACILITY)cs_status);
	}
	global->ops_estate.opn_startime = timer_block.stat_cpu; /* millisecs 
					    ** of CPU time used
                                            ** by the process - used to
                                            ** limit search time
                                            */
    }
    global->ops_estate.opn_slowflg = FALSE; /* have any intermediate results
                                            ** been deleted?
                                            */
    /*global->ops_estate.opn_debug.opn_level = 0;*/ /* indentation level for 
					    ** debugging
					    */

}

/*{
** Name: opn_jhandler	- exception handler for enumeration phase
**
** Description:
**      Exception handler for the enumeration phase of optimization.  This
**      could be a normal event if an EXOPTIMIZER exception is raised.  This
**      will indicate that the best CO found so far should be used. 
**      This routine should deallocated memory resources if an exception
**      occurs within the optimizer which was not an internally generated
**      exception
**
**	Need to find way of recovering from exception in the exception handler
**	- establish another exception handler here to catch exception 
** Inputs:
**      args                            arguments which describe the 
**                                      exception which was generated
**
** Outputs:
**	Returns:
**	    EXRESIGNAL                 - if this was an unexpected exception
**          EXDECLARE                  - if an expected, internally generated
**                                      exception was found
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the calling frame to the point at which
**          the exception handler was declared.
**
** History:
**	7-jun-86 (seputis)
**          initial creation from jhandler in jhandler.c
**	7-nov-88 (seputis)
**          add EX_UNWIND
**	11-mar-92 (rog)
**	    Remove EX_DECLARE and change the EX_UNWIND case to return
**	    EXRESIGNAL instead of EX_OK.
[@history_line@]...
*/
static STATUS
opn_jhandler(
	EX_ARGS            *args)
{
    switch (EXmatch(args->exarg_num, (i4)2, EX_JNP_LJMP, EX_UNWIND))
    {
    case 1:
	return (EXDECLARE);	    /* return to invoking exception point
                                    ** - since this is a normal unwinding
                                    ** of the stack
                                    ** - error status should already have
                                    ** been set in the caller's control block
                                    */
    case 2:			    /* lower level exception handler is clearing
				    ** the stack */
	return (EXRESIGNAL);
    default:
        return (opx_exception(args)); /* handle unexpected exception */
    }
}

/*{
** Name: opn_enum	- main entry point for the optimizer enumeration phase
**
** Description:
**      This routine is the main entry point for the enumeration phase of 
**      processing.  The function of this phase is to evaluate all possible 
**      access paths and choose the best "cost ordering".  This plan will
**      eventually be provided to the query compilation phase of the optimizer
**      which will produce a QEP for QEF.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
** Outputs:
**      subquery.ops_bestco             ptr to best "cost ordering" to
**                                      be used by query compilation
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-may-86 (seputis)
**          initial creation
**	6-nov-88 (seputis)
**          add timeout tracing
**	20-dec-90 (seputis)
**	    added support for floating point exception handling
**	7-may-91 (seputis)
**	    fix b37233 - only obtain histograms if enumeration is
**	    required
**	14-jul-1993 (rog)
**	    Compare return from EXdeclare() against EXDECLARE, not EX_DECLARE.
**	19-nov-99 (inkdo01)
**	    Changes to remove EXsignal from exit processing.
**	27-sep-00 (inkdo01)
**	    Moved op156 call here, to pick up histogram info.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Check for need to use prefetch strategy for rule-triggering
**          updates (bug 100680).
**          Poached from inkdo01's change 448232. Moved inkdo01's check from
**          opn_ienum() to opn_enum() as we will not call opn_ienum() if
**          opn_ovqp() returns TRUE.
**      03-Jul-2002 (huazh01)
**          Extend the fix for b100680 to cover the case where a 
**          PST_IF_TYPE statement is followed by a PST_CP_TYPE statement.
**          This fixes bug 108173 / INGSRV 1821.
**	29-aug-02 (inkdo01)
**	    Added tp op153 to dump local range table only.
**	17-sep-02 (inkdo01)
**	    Add interface to new enumeration technique.
**	1-nov-02 (inkdo01)
**	    Change tracepoint number for new enumeration.
**	13-dec-02 (inkdo01)
**	    Transplanted 03-jul-02 fix into opn_ienum (hopefully without 
**	    changing its impact) udring propagation of 458679.
**	14-jan-03 (inkdo01)
**	    Added config parm to control new enumeration.
**	10-mar-04 (hayke02)
**	    Add back in the EXsignal() exit processing removed by the 19-nov-99
**	    (inkdo01) change. Call opn_exit() with a TRUE longjump so that we
**	    longjump to the memory clear up code as before. This fixes problem
**	    INGSRV 2728, bug 111900.
**	7-may-04 (toumi01)
**	    Declare handler for new and old enumeration function calls.
**	29-oct-04 (inkdo01)
**	    Loop to clear opo_held flag from any available OPO_PERM structures.
**	31-mar-05 (inkdo01)
**	    Fix call to opt_rall() to pick up right subquery addr.
**	3-feb-06 (inkdo01)
**	    Add support of query-level hints for greedy.
**	20-mar-06 (dougi)
**	    Add support for hint-demanded retry.
**	30-aug-06 (thaju02)
**	    Check if we are to use prefetch for rule-triggering deletes. 
**	    (B116355)
**	7-jan-2007 (dougi)
**	    Change to allow op247 to specify range table count (opv_rv) above
**	    which greedy enumeration will be used (regardless of base table 
**	    count).
**	22-Apr-09 (kibro01) b119039
**	    Don't use greedy enumeration if there are >20 tables connected by
**	    the same equivalence class, since that really benefits from old 
**	    enumeration where a timeout is possible.
*/
VOID
opn_enum(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE      *global;       /* ptr to global state variable */
    EX_CONTEXT     excontext;     /* context block for exception handler*/
    OPN_STATUS	   sigstat = OPN_SIGOK;
    PST_STATEMENT  *stmtp;
    i4		   first = -1, second = -1;
    bool	   retry;
    bool	   use_greedy = TRUE;

    global = subquery->ops_global;
    /* See if there are rules requiring prefetch strategy. */
    if( global->ops_qheader->pst_mode == PSQ_DELETE)
    {
        for (stmtp = global->ops_statement->pst_after_stmt; stmtp;
                    stmtp = stmtp->pst_next)
             if ( (stmtp->pst_type == PST_CP_TYPE &&
                  (stmtp->pst_specific.pst_callproc.pst_pmask & PST_DEL_PREFETCH))
                  ||
                  (stmtp->pst_type == PST_IF_TYPE &&
                   stmtp->pst_link &&
                   stmtp->pst_link->pst_type == PST_CP_TYPE &&
                   (stmtp->pst_link->pst_specific.pst_callproc.pst_pmask
                     & PST_DEL_PREFETCH)) )
             {
                global->ops_estate.opn_halloween = opn_halloween(subquery);
                break;
             }
    }

    if( global->ops_qheader->pst_mode == PSQ_REPLACE)
    {
        for (stmtp = global->ops_statement->pst_after_stmt; stmtp;
                    stmtp = stmtp->pst_next)
             if (stmtp->pst_type == PST_CP_TYPE &&
                (stmtp->pst_specific.pst_callproc.pst_pmask & PST_UPD_PREFETCH))
             {
                global->ops_estate.opn_halloween = opn_halloween(subquery);
                break;
             }
    }

    if ( opn_ovqp(subquery) )
	return;                   /* if this is a one variable query then
                                  ** no enumeration is needed so allocate
                                  ** a CO node and return */

    /* If a query includes hints and the first pass through enumeration
    ** (when the hints are applied) failed to find a plan, the hint flag
    ** is extinguished and we branch back here to try it again. */
onceMorewithFeeling:
    retry = FALSE;
    opn_iemaps(subquery);	  /* initialize various maps
				  ** required during enumeration */
    oph_histos(subquery);	    /* read histogram information about
				    ** attributes used in the query
				    ** in a joining clause or in a
				    ** qualification
				    */
#ifdef    OPT_F028_JOINOP
    if (
	    (subquery->ops_global->ops_cb->ops_check 
	    && 
	    opt_strace( subquery->ops_global->ops_cb, OPT_F028_JOINOP)
	    )
	)
	opt_jall( subquery );	    /* - dump all joinop table
					** information
					*/
    else if (
	    (subquery->ops_global->ops_cb->ops_check 
	    && 
	    opt_strace( subquery->ops_global->ops_cb, OPT_F025_RANGETAB)
	    )
	)
    {
	subquery->ops_global->ops_trace.opt_subquery = subquery;
	opt_rall(FALSE);	    	/* load subquery for opt_rall() and
					** dump just range tab info
					*/
    }
#endif

    global->ops_estate.opn_streamid = NULL; /* - indicates that 
                                  ** dynamic memory stream has
                                  ** not been allocated... so that the
                                  ** exception handler does not attempt to
                                  ** deallocate it, incorrectly
                                  */

    {
	OPO_PERM	*permp;

	for (permp = global->ops_estate.opn_fragcolist; 
		permp; permp = permp->opo_next)
	    permp->opo_conode.opo_held = FALSE;
    }

    if ( EXdeclare(opn_jhandler, &excontext) == EXDECLARE )
    {
	/* normal exit point for enumeration
	** - this exception handler was established to return memory
	** to the memory manager
	** - whenever any error occurs during optimization the exception
	** handler will be invoked and this exit point will be used
	** - an exception handler is established here since enumerate never
	** returns but signals an exception
	** - a separate memory stream is assigned to the enumeration phase
	** since CO trees can potentially consume large amounts of memory.
	*/
	(VOID)EXdelete();		/* delete previous exception handler
					** and setup new one to catch errors
					** during deallocation of resources etc.
					** - this will avoid recursive
					** exceptions */
	if ( EXdeclare(opn_jhandler, &excontext) != EXDECLARE )
	{   /* create an exception handler to track the copying of a CO
            ** tree */
	    if (DB_SUCCESS_MACRO(global->ops_cb->ops_retstatus))
	    {
		DB_STATUS	status = E_DB_OK;
		status = opo_copyco(subquery, &subquery->ops_bestco, TRUE);
					/* copy best CO prior to deallocating 
                                        ** memory */
		/* FIXME - do not need to copy if this is the main query, but 
		** need to check if "out of memory" errors have occurred */
		if (status == E_DB_ERROR)
		{
		    subquery->ops_mask2 &= ~(OPS_HINTPASS | OPS_IXHINTS
				| OPS_KJHINTS | OPS_NKJHINTS | OPS_ORDHINT);
					/* turn off hint processing */
		    retry = TRUE;
		}
	    }
	}
	(VOID)EXdelete();		/* delete exception handler for copying
                                        ** the CO tree */
	if ( EXdeclare(opn_jhandler, &excontext) == EXDECLARE )
	{   /* this routine should not be executed unless there are some 
            ** serious problems in the optimizer, this routine will abort
            ** the cleanup routines */
	    (VOID)EXdelete();		/* cancel exception handler since this
                                        ** handler has caused an error*/
	    opx_error (E_OP0083_UNEXPECTED_EXEX); /* report a new error to the
                                        ** next exception handler
                                        ** - an exception occurred within an
					** cleanup routine for exception 
                                        ** handler */
	}
	else
	{   /* deallocate memory resources */

	    /* Before that, see if subquery is aggregate and needs 
	    ** result estimates posted into its ops_gentry (non-Star). */
	    if (!(global->ops_cb->ops_smask & OPS_MDISTRIBUTED))
		opn_modcost(subquery);

	    if (global->ops_estate.opn_streamid)
	    {   /* if memory has been allocated then deallocate it */
		opu_deallocate(global, &global->ops_estate.opn_streamid);

                /* deallocate remainder of streamids which can only be
                ** defined if opn_streamid is defined */
		if (global->ops_estate.opn_jtreeid)
		{   /* if memory has been allocated then deallocate it */
		    opu_deallocate(global, &global->ops_estate.opn_jtreeid);
		}
		if (global->ops_estate.opn_cbstreamid)
		{   /* if memory has been allocated then deallocate it */
		    opu_deallocate(global, &global->ops_estate.opn_cbstreamid);
		}
	    }
	    if (global->ops_gmask & OPS_FLSPACE)
		subquery->ops_mask |= OPS_SFPEXCEPTION;
	    (VOID)EXdelete();		/* cancel exception handler prior
					** to exiting routine */
	    if (!(DB_SUCCESS_MACRO(global->ops_cb->ops_retstatus)))
		EXsignal( (EX)EX_JNP_LJMP, (i4)0); /* signal an optimizer long
					** jump so that next exception
					** handler gets control */

	    if (retry)
		goto onceMorewithFeeling; /* try again with no hints */

	    return;			/* normal exit point */
        }
    }
    if (!global->ops_cb->ops_check 
	||
	!opt_strace( global->ops_cb, OPT_F048_FLOAT))
	EXmath(EX_ON);			/* turn on math exception handling for
					** OPF so that parts of the search space
					** can be skipped when necessary */
    opn_ienum(subquery);		/* initialize enumeration phase */

    if ((((!global->ops_cb->ops_check 
	||
	!opt_svtrace( global->ops_cb, OPT_F119_NEWENUM, &first, &second))
	&&
	!(global->ops_cb->ops_alter.ops_newenum))
	 &&
        !(global->ops_cb->ops_override & PST_HINT_GREEDY))
        ||
        (global->ops_cb->ops_override & PST_HINT_NOGREEDY)
        ||
        global->ops_cb->ops_smask & OPS_MDISTRIBUTED
        ||
        (first <= 0 &&
        (subquery->ops_vars.opv_prv < 4
        ||
        subquery->ops_vars.opv_rv < 10)
        ||
        first > subquery->ops_vars.opv_rv))
    {
        /* Star or no op247 or < 5 base tables or < 10 tables/indexes:
	** do old enumeration. NEW: "n <= opv_rv" where "op247 n",
	** we use greedy enumeration. */
	use_greedy = FALSE;
    }

    if (use_greedy &&
	!(global->ops_cb->ops_override & PST_HINT_GREEDY))
    {
	i4 largest_eqcls = 0;
	i4 eqcls;

        for (eqcls = 0; eqcls < subquery->ops_eclass.ope_ev; eqcls++)
        {
	    if (subquery->ops_eclass.ope_base->ope_eqclist[eqcls])
	    {
	        i4 this_eqcls;
	        this_eqcls = BTcount((PTR)&subquery->ops_eclass.ope_base->
				    ope_eqclist[eqcls]->ope_attrmap,
			    subquery->ops_attrs.opz_av);
	        if (this_eqcls > largest_eqcls)
		    largest_eqcls = this_eqcls;
	    }
        }
	/* If a single equivalence class links >20 things, the attempt to
	** use greedy enumeration will be very slow due to taking all the 
	** three-segment permutations which can be joined.
	** (kibro01) b119039
	*/
	if (largest_eqcls > 20)
		use_greedy = FALSE;
    }

    if (use_greedy)
    {
	sigstat = opn_newenum(subquery, first);  /* do new enumeration */
    }

    /* If we want old-style enumeration, or if greedy already failed... */
    if (!use_greedy || (sigstat == OPN_SIGEXIT))
    {
         sigstat = opn_tree(subquery);
    }

    if (sigstat == OPN_SIGOK) subquery->ops_timeout = FALSE;	
					/* at this point no timeout has occurred
					** so indicate this to the QEP printing
					** routines */
    opn_exit(subquery, TRUE);           /* generate exception so all cleanup
                                        ** is done in one place
                                        ** - this routine will never return
                                        ** and should always signal an 
                                        ** exception */
    (VOID)EXdelete();			/* this statement should not be reached
					*/
}


/*{
** Name: opn_nonsystem_rule	- halloween processing for (delete) rules
**
**	This routine checks for a non-system AFTER rule in the current
**	subquery, and returns TRUE if one is found.
**
**	Originally this was designed to alleviate a performance problem
**	with deletes;  a rule on the DELETE mandated a prefetch, in case
**	the rule DBP accessed the target table in some way.  (Otherwise
**	one gets the "halloween" problem where rows vanish inappropriately.)
**	The prefetch caused DMF to re-find the row to delete, which can
**	be much slower than deleting in place.  Since we know that system
**	generated rules are well behaved, no prefetch is needed for
**	system generated rules.
**
**	The usage will be extended to other situations where system generated
**	rules are safe (e.g. APPEND to LOAD optimization).
**
** Description:
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
** Outputs:
**	Returns:
**	    TRUE			User rules exist, potential problem
**          FALSE			No after-rules or system ones only
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**     22-feb-2006 (stial01)
**          created
**     02-dec-2009 (whiro01)
**	    Made non-static to be called from "opcahd.c"
*/
bool
opn_nonsystem_rule(OPS_SUBQUERY       *subquery)
{
    PST_STATEMENT		*stmtp;
    PST_STATEMENT		*s;
    OPS_STATE		*global;	/* global state variable */

    global = subquery->ops_global;
    for (stmtp = global->ops_statement->pst_after_stmt; 
		stmtp; stmtp = stmtp->pst_next)
    {
	if (stmtp->pst_type == PST_CP_TYPE)
	{
	    if (!(stmtp->pst_specific.pst_callproc.pst_pmask & PST_CPSYS)) 
		return (TRUE); /* Potential halloween problem */
	}
	else if (stmtp->pst_type == PST_IF_TYPE)
	{
	    s = stmtp->pst_specific.pst_if.pst_true;
	    if (s)
	    {
		if (s->pst_type != PST_CP_TYPE)
		    return (TRUE); /* Potential halloween problem */
		else if ( !(s->pst_specific.pst_callproc.pst_pmask & PST_CPSYS))
		    return (TRUE); /* Potential halloween problem */
	    }

	    s = stmtp->pst_specific.pst_if.pst_false;
	    if (s)
	    {
		if (s->pst_type != PST_CP_TYPE)
		    return (TRUE); /* Potential halloween problem */
		else if ( !(s->pst_specific.pst_callproc.pst_pmask & PST_CPSYS))
		    return (TRUE); /* Potential halloween problem */
	    }
	}
	else
	{
	    return (TRUE); /* Potential halloween problem */
	}
    }

    if (!stmtp)
	return (FALSE);
    else
	return (TRUE); /* Potential halloween problem */
}

