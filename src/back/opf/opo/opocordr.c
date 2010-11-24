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
#define        OPO_CORDERING      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPOCORDR.C - create multi-attribute ordering
**
**  Description:
**      Create an ordering given a list of equivalence classes, this 
**      will be used to create an ordering available from a BTREE 
**      in an ORIG node.
**
**
**
**  History:    
**      9-dec-87 (seputis)
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	04-Dec-1995 (murf)
**		Added NO_OPTIM for sui_us5 as a SEGV occurred when the help
**		command was issued within the sql session. The OS is
**		Solaris 2.4 and the compiler is 3.0.1.
**	5-jan-97 (inkdo01)
**	    Fixed 87509 by adding OPB_SUBSEL_CNST flag in opo_pr and testing
**	    for it in opo_kordering.
**	25-feb-97 (rosga02) Removed NO_OPTIM for sui_us5
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPO_ISORT opo_gnumber(
	OPS_SUBQUERY *subquery);
void opo_iobase(
	OPS_SUBQUERY *subquery);
OPO_ISORT opo_cordering(
	OPS_SUBQUERY *subquery,
	OPO_EQLIST *keyp,
	bool copykey);
void opo_makey(
	OPS_SUBQUERY *subquery,
	OPB_MBF *mbfp,
	i4 count,
	OPO_EQLIST **keylistp,
	bool tempmem,
	bool noconst);
void opo_pr(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
void opo_orig(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
void opo_rsort(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
OPO_ISORT opo_kordering(
	OPS_SUBQUERY *subquery,
	OPV_VARS *varp,
	OPE_BMEQCLS *eqcmap);

/*{
** Name: opo_gnumber	- get a multi-attribute ordering number
**
** Description:
**      Get the next available multi-attribute ordering number
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Returns:
**	    OPO_ISORT - empty multi-attribute ordering slot
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-dec-87 (seputis)
**          initial creation
[@history_template@]...
*/
OPO_ISORT
opo_gnumber(
	OPS_SUBQUERY       *subquery)
{
    /* check if ordering slot is available */
    if (!(subquery->ops_msort.opo_sv % OPO_BITMAP))
    {	/* if a boundary has been reached then a new set of slots needs to
        ** be allocated */
	OPO_ST		    *stp;
	stp = (OPO_ST *)opn_memory(subquery->ops_global, 
	    (i4)((subquery->ops_msort.opo_sv + OPO_BITMAP)*sizeof(OPO_SORT)));
	MEcopy((PTR)subquery->ops_msort.opo_base, 
	    subquery->ops_msort.opo_sv * sizeof(stp->opo_stable[0]),
	    (PTR)stp);
	subquery->ops_msort.opo_base = stp; /* FIXME - use old ptr
				    ** for something */
    }
    return (subquery->ops_msort.opo_sv++);
}

/*{
** Name: opo_iobase	- init ordering array of ptrs to descriptors
**
** Description:
**      If the sort ordering array of descriptors is not initialized 
**      then this routine will intialize it.
**
** Inputs:
**      subquery                        ptr to subquery to be analyzed
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-feb-88 (seputis)
**          initial creation
[@history_template@]...
*/
VOID
opo_iobase(
	OPS_SUBQUERY	*subquery)
{
    OPO_SORT          *sortp;
    if (subquery->ops_msort.opo_base)
	return;				/* return if already initialized */
    subquery->ops_msort.opo_sv = 0;
    subquery->ops_msort.opo_maxsv = 0;
    subquery->ops_msort.opo_base = (OPO_ST *)opn_memory(subquery->ops_global, 
	    (i4)(OPO_BITMAP*sizeof(sortp)));
}

/*{
** Name: opo_cordering	- create multi-attribute ordering
**
** Description:
**      Create a multi-attribute ordering for a BTREE orig node.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      keyp                            ptr to array of equivalence classes
**	copykey				TRUE if keyp should be copied
**
** Outputs:
**	Returns:
**	    OPO_ISORT - number of multi-attribute ordering
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-dec-87 (seputis)
**          initial creation
**      5-jan-93 (ed)
**          do not create a multi-attribute ordering for a single
**          equivalence class
**	15-sep-93 (ed)
**	    add copy key parameter
[@history_template@]...
*/
OPO_ISORT
opo_cordering(
	OPS_SUBQUERY       *subquery,
	OPO_EQLIST	   *keyp,
	bool		    copykey)
{
    OPO_ISORT           ordering;	/* number used to represent
					** multi-attribute ordering */
    OPO_SORT		*orderp;        /* ptr to multi-attribute
					** ordering descriptor */
    OPO_ST              *obase;

    if (keyp->opo_eqorder[0] == OPE_NOEQCLS)
	return(OPE_NOEQCLS);		/* check for the null case */
    if (keyp->opo_eqorder[1] == OPE_NOEQCLS)
        return(keyp->opo_eqorder[0]);   /* check for the single attribute case */
    obase = subquery->ops_msort.opo_base;
    if (!obase)
	opo_iobase(subquery);		/* initialize the ordering table
					** base */
    {	/* search existing list for the multi-attribute ordering */
	i4		maxorder;

	maxorder = subquery->ops_msort.opo_sv;
	for( ordering = 0; ordering < maxorder; ordering++)
	{
	    orderp = obase->opo_stable[ordering];
	    if ( orderp->opo_stype == OPO_SEXACT)
	    {
		OPE_IEQCLS	*eqclsp;
		OPE_IEQCLS	*tempeqclsp;
		
		eqclsp = &orderp->opo_eqlist->opo_eqorder[0];
		tempeqclsp = &keyp->opo_eqorder[0];
		while ((*eqclsp) == (*tempeqclsp))
		{
		    eqclsp++;
		    tempeqclsp++;
		    if (((*eqclsp) == OPE_NOEQCLS)
			&&
			((*tempeqclsp) == OPE_NOEQCLS))
			return((OPO_ISORT)(ordering+subquery->ops_eclass.ope_ev));
					    /* multi-attribute ordering found
					    ** in list */
		}
	    }
	}
    }
    ordering = opo_gnumber(subquery);	/* get the next multi-attribute
					** ordering number */
    orderp = (OPO_SORT *)opn_memory(subquery->ops_global, (i4)sizeof(*orderp));
    orderp->opo_stype = OPO_SEXACT;
    orderp->opo_bmeqcls = (OPE_BMEQCLS *)opn_memory(subquery->ops_global, (i4)sizeof(OPE_BMEQCLS));
    MEfill(sizeof(OPE_BMEQCLS), (u_char)0, (PTR)orderp->opo_bmeqcls);
    orderp->opo_eqlist = keyp;
    {
	OPO_ISORT	*isortp;
	OPO_ISORT	maxsize;
	isortp = &keyp->opo_eqorder[0];
	for(maxsize = 1; *isortp != OPE_NOEQCLS ; isortp++)
	{
	    maxsize++;
	    BTset((i4)*isortp, (char *)orderp->opo_bmeqcls);
	}
	if (copykey)
	{   /* stack was used to build ordering so copy it into
	    ** opf memory */
	    maxsize = maxsize * sizeof(OPO_ISORT);
	    keyp = (OPO_EQLIST *)opn_memory(subquery->ops_global,
		(i4)maxsize);
	    MEcopy((PTR)orderp->opo_eqlist, maxsize, (PTR)keyp);
	    orderp->opo_eqlist = keyp;
	}
    }
    subquery->ops_msort.opo_base->opo_stable[ordering] = orderp;
    return((OPO_ISORT)(ordering+subquery->ops_eclass.ope_ev));
}


/*{
** Name: opo_makey	- create multi-attribute keylist
**
** Description:
**      Routine will create a multi-attribute keylist, terminated
**      by an OPE_NOEQLCS marker.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      mbfp                            ptr to boolean factor descriptor
**      count                           number of attribute elements to
**                                      consider
**      tempmem                         use enumeration memory if true
**                                      otherwise non-enumeration memory
**      noconst                         remove constant equivalence classes
**                                      if true
**
** Outputs:
**	Returns:
**	    OPO_ISORT *, ptr to an ordering list
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-88 (seputis)
**	    initial creation
**      5-jan-93 (ed)
**          bug 48049 add parameters to allow creation of equivalence class lists
**          which eliminate constant equivalence classes.
**	31-mar-03 (hayke02)
**	    Break out of the orderattr loop if we find an outer join on clause
**	    constant predicate - we cannot use this, or subsequent, attributes
**	    for ordering. This change fixes bug 109683.
[@history_line@]...
[@history_template@]...
*/
VOID
opo_makey(
	OPS_SUBQUERY       *subquery,
	OPB_MBF            *mbfp,
	i4                count,
	OPO_EQLIST         **keylistp,
	bool               tempmem,
	bool               noconst)
{   /* multi-attribute ordering */
    i4                     orderattr;   /* used to index array of ordering
				    ** attributes */
    OPZ_AT                 *abase;  /* base of array of ptrs to joinop
				    ** attribute elements */
    OPO_EQLIST		   *keylist;
    i4                     target;  /* index into target array */

    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to
					** joinop attribute elements */
    if (tempmem)
        keylist = (OPO_EQLIST *)opn_memory(subquery->ops_global,
            (i4) (sizeof(OPO_ISORT) * (count+1)));
    else
        keylist = (OPO_EQLIST *)opu_memory(subquery->ops_global,
            (i4) (sizeof(OPO_ISORT) * (count+1))); /* for now place this into
                                    ** non-enumeration memory since it will
                                    ** typically get placed into OPV_VARS
                                    ** structure, otherwise dangling ptrs
                                    ** into deleted memory streams may be
                                    ** created */
    *keylistp = keylist;
    target = 0;
    for (orderattr = 0; orderattr < count; orderattr++)
    {
	bool	ojconst = FALSE;	
        keylist->opo_eqorder[target] = abase->opz_attnums[mbfp->opb_kbase->
            opb_keyorder[orderattr].opb_attno]->opz_equcls;
        if (!noconst
            ||
            !subquery->ops_bfs.opb_bfeqc
            ||
            !BTtest((i4)keylist->opo_eqorder[target],
                (char *)subquery->ops_bfs.opb_bfeqc)
            )
            target++;               /* non-constant equivalence found */
	else if (subquery->ops_oj.opl_base)
	{
	    OPB_IBF	bfi;

	    for (bfi = 0; bfi < subquery->ops_bfs.opb_bv; bfi++)
	    {
		OPB_BOOLFACT	*bfp;

		bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi];
		if ((bfp->opb_eqcls == keylist->opo_eqorder[target]) &&
		    (bfp->opb_ojid >= 0))
		{
		    ojconst = TRUE;
		    break;
		}
	    }
	    if (ojconst)
		break;
	}
    }
    keylist->opo_eqorder[target] = OPE_NOEQCLS;   /* terminate eqcls list */
}

/*{
** Name: opo_pr	- find ordering for project-restrict node
**
** Description:
**      Given a project restrict node, find the ordering which can be 
**      used to avoid sort nodes by parents.  If the ordering was
**      DB_BTRE_STORE, then the sortedness of the storage structure
**      can be assumed.  Ignore whether constant equivalence classes
**      are embedded, since this will be taken care of in opo_combine.
**      opo_ordeqc will contain an ordering only if sortedness is
**      used, but in the DB_ORIG node, any ordering which can be used
**      for keying will be stored.  This will work since a DB_ORIG
**      node will never be used as an outer for a join directly (i.e.
**      a project restrict will be on top of it).  If the ordering is
**      an inner then this implies keying will be used (FIXME check
**      for physical scan of inner).
**          Note that for a DB_BTRE_STORE, if a set of explicit TIDs are 
**      found then the attributes are not ordered, so check for this case.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to Project-Restrict node
**
** Outputs:
**      [@param_name@] [@comment_text@]#41$
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
**      10-feb-89 (seputis)
**          mark PR node as sorted only if usable ordering is available
**      24-apr-89 (seputis)
**          place only eqc required by parent into ordering, b5411
**      15-aug-89 (seputis)
**          modified interface to opn_prcost for b6538
**	16-may-90 (seputis)
**	    - fix lint errors 
**	20-jun-90 (seputis)
**	    - add support for top sort node removal
**      15-dec-91 (seputis)
**          - fix b40402 - incorrect answer with corelated group by aggregates
**	28-jan-92 (seputis)
**	    - fix 41939 - top sort node removal optimization when constant
**	    equivalence classes exist
**	28-jan-92 (seputis)
**	    - fix 40923 - remove parameter to opn_prcost
**      29-apr-92 (seputis)
**          - b43969 - check boundary condition to avoid overwriting stack
**      11-may-92 (seputis)
**          - check for out of bounds array access earlier in processing
**	    to prevent read segv on unix.
**          - check for constants in by list and treat aggregate result as
**	    a constant if this is true.
**	4-aug-93 (ed)
**	    - fix bug 51642, 52901, constant eqcls map was not consistently
**	    updated
**	5-jan-97 (inkdo01)
**	    Flag when there's a BF constant eqclass originating in the result
**	    of a single valued subselect.
**	27-mar-04 (inkdo01)
**	    Change to prevent assumption of presortedness on partitioned
**	    structured tables.
[@history_line@]...
[@history_template@]...
*/
VOID
opo_pr(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    OPO_CO		*orig_cop;
    OPV_GRV		*grvp;		/* ptr to global range var descriptor
					** for ORIG node */
    OPV_VARS		*varp;		/* pointer to range variable
					** element associated with leaf */
    orig_cop = cop->opo_outer;		/* must be a DB_ORIG node on the outer */
    varp = subquery->ops_vars.opv_base->opv_rt[orig_cop->opo_union.opo_orig];
    grvp = varp->opv_grv;
    if ((orig_cop->opo_storage == DB_HEAP_STORE)
	&&
	grvp
	&&
	grvp->opv_gquery 
	&& 
	(   (grvp->opv_created == OPS_RFAGG)
	    ||
	    (grvp->opv_created == OPS_FAGG)
	))
    {   /* get the ordering available from the sorted by list of the
	** function aggregate */

	PST_QNODE	    *byexprp;
	i4		    sort_count;
	OPO_EQLIST	    templist;
	OPZ_AT		    *abase;
	OPO_ISORT	    agg_ordering; /* sorted ordering provide by an aggregate */
	OPE_IEQCLS	    agg_eqcls;	/* eqcls of the aggregate result */
	bool		    check_constant; /* for rare special case of correlated group by
				    ** aggregate subselect, the link back attributes are
				    ** not available */

	check_constant= FALSE;
	agg_ordering = OPE_NOEQCLS;
	abase = subquery->ops_attrs.opz_base;
	sort_count = 0;
	if (grvp->opv_gquery->ops_agg.opa_fbylist)
	    byexprp = grvp->opv_gquery->ops_agg.opa_fbylist;
	else
	    byexprp = grvp->opv_gquery->ops_byexpr;
	for (;
	    byexprp && (byexprp->pst_sym.pst_type == PST_RESDOM); 
	    byexprp = byexprp->pst_left)
	{
	    OPZ_IATTS	    attr;
	    OPZ_DMFATTR	    dmfattr;

	    dmfattr = byexprp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
	    attr = opz_findatt(subquery, orig_cop->opo_union.opo_orig, dmfattr);
	    if (attr < 0)
	    {
		sort_count=0;	    /* this part of ordering is not used */
		continue;
	    }
	    if (sort_count >= DB_MAXKEYS)
		break;		    /* do not write over stack, exit if
				    ** more than 30 elements in by list */
	    templist.opo_eqorder[sort_count] = 
		abase->opz_attnums[attr]->opz_equcls;
	    if (subquery->ops_bfs.opb_bfeqc
		&& 
		BTtest ((i4)templist.opo_eqorder[sort_count],
		    (char *)subquery->ops_bfs.opb_bfeqc))
	    {
		check_constant = TRUE;
		sort_count--; /* ignore equivalence classes
			** which are constant since they will
			** always be sorted */
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
	    for (count1 = sort_count-1, count2=0; 
		count1 > count2; count1--, count2++)
	    {
		OPO_ISORT	    save;
		save = templist.opo_eqorder[count1];
		templist.opo_eqorder[count1] = templist.opo_eqorder[count2];
		templist.opo_eqorder[count2] = save;
	    }
	}
	for (byexprp = grvp->opv_gquery->ops_root->pst_left;
	    byexprp && (byexprp->pst_sym.pst_type == PST_RESDOM); 
	    byexprp = byexprp->pst_left)
	{   /* find set of attributes which are aggregate results, since
	    ** they are unique with respect to the by list, they can be
	    ** considered sorted, FIXME, - for now do not consider aggregates
	    ** evaluated together, just get the first AOP value since otherwise
	    ** a "set of eqcls" could be used and the rest of the optimizer should
	    ** be able to handle it, but it is not clear this is true at this time.
	    */
	    if (byexprp->pst_right->pst_sym.pst_type == PST_AOP)
	    {
		OPZ_IATTS	    attr1;
		OPZ_DMFATTR	    dmfattr1;

		dmfattr1 = byexprp->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		attr1 = opz_findatt(subquery, orig_cop->opo_union.opo_orig, dmfattr1);
		if (attr1 >= 0)
		{
		    agg_eqcls = abase->opz_attnums[attr1]->opz_equcls;
		    if (sort_count == 0)
		    {   /* if all by list elements are constants then this
			** implies there is only one partition so that this
			** aggregate result is also single valued */
			if (check_constant)
			{
			    BTset((i4)agg_eqcls, (char *)subquery->ops_bfs.opb_bfeqc);
			    BTclear((i4)agg_eqcls, (char *)subquery->ops_bfs.opb_mbfeqc);
			    subquery->ops_bfs.opb_mask |= OPB_SUBSEL_CNST;
							/* indicate a subselect-
							** generated constant */
			}
		    }
		    else if (!subquery->ops_bfs.opb_bfeqc
			    || 
			    !BTtest ((i4)agg_eqcls,
				(char *)subquery->ops_bfs.opb_bfeqc))
			break;
		}
	    }
	}
	if ((sort_count < DB_MAXKEYS)
	    &&
	    byexprp 
	    && 
	    (byexprp->pst_sym.pst_type == PST_RESDOM))
	    templist.opo_eqorder[sort_count++] = agg_eqcls;
	if (sort_count == 1)
	    agg_ordering = templist.opo_eqorder[0];
	else if (sort_count > 1)
	{
	    /* multi-attribute can provide a useful ordering */
	    i4		    size;
	    OPO_EQLIST	    *sortp1;
	    /* create an ordering for this storage structure */
	    size = sizeof(OPO_ISORT) * (sort_count + 1);
	    sortp1 = (OPO_EQLIST *)opu_memory(subquery->ops_global, (i4)size);
	    MEcopy( (PTR)&templist, size, (PTR)sortp1);
	    sortp1->opo_eqorder[sort_count] = OPE_NOEQCLS;
				    /* terminate list */
	    agg_ordering = opo_cordering(subquery, sortp1, FALSE); /* find
				** the exact ordering associated with this
				** list */
	}
	if (agg_ordering != OPE_NOEQCLS)
	    /* sortedness is provided by the function aggregate */
	    cop->opo_ordeqc = agg_ordering;
	else
	    cop->opo_storage = DB_HEAP_STORE; /* no ordering provided */
    }
    else if ((orig_cop->opo_storage == DB_BTRE_STORE)
	||
	(orig_cop->opo_storage == DB_ISAM_STORE)
        ||
	(orig_cop->opo_storage == DB_HASH_STORE)
        ||
	(cop->opo_storage == DB_SORT_STORE)) /* exact keys can convert
					** ordering into a sorted order */
    {	/* both BTREE and ISAM provide some ordering, with the difference
        ** being that BTREE will be converted into DB_SORT_STORE, since
        ** the ordering can be guaranteed, whereas ISAM will be
        ** considered DB_HEAP_STORE, which can be used for partial
        ** sort merges */
	OPB_SARG            sargtype;   /* not used */
	OPO_BLOCKS	    pblk;       /* not used */
	OPH_HISTOGRAM       *hp;        /* not used */
	bool		    pt_valid;   /* not used */
	OPO_ISORT	    ordering;   /* this is the ordering which can be
					** assumed after keying has been done in
					** the relation, - OPE_NOEQLCS if no
					** ordering can be assumed */
	bool		    imtid;      /* TRUE if implicit tid equality has
                                        ** been found, so ordering will be
					** on TID */
	OPO_BLOCKS	    dio;	/* dummy variable for opn_prcost */
	OPO_CPU		    cpu;	/* dummy variable for opn_prcost */
	
	(VOID) opn_prcost (subquery, orig_cop, &sargtype, &pblk, &hp, &imtid,
	    TRUE, &ordering, &pt_valid, &dio, &cpu); /* only calculate 
					** the ordering which
					** exists after keying */
	cop->opo_ordeqc = ordering;
	if ((	(orig_cop->opo_storage != DB_HASH_STORE) /* hash may only introduce
					** orderings if constant look ups are used
                                        ** otherwise no ordering should be
					** considered */
		&&
		!imtid
					/* exit if implicit TID key since
                                        ** ordering will be on this rather
					** than BTREE key */
	    )
	    &&
	    (	(orig_cop->opo_storage != DB_ISAM_STORE)
		||
		(ordering == OPE_NOEQCLS)
					/* at least part of the key was used
                                        ** in an exact ordering so do not
                                        ** override with the entire key since
                                        ** DB_SORT_STORE would not be TRUE for
					** the entire key */
	    )
	    &&
	    (	!(varp->opv_grv->opv_relation->rdr_parts)))
	{
	    if (varp->opv_mbf.opb_count == 1)
	    {
		cop->opo_ordeqc = subquery->ops_attrs.opz_base->
		    opz_attnums[varp->opv_mbf.opb_kbase->
			opb_keyorder[0].opb_attno]->opz_equcls; /* get the
						** equivalence class associated
						** with the single attribute
						** ordering */
                if (subquery->ops_bfs.opb_bfeqc
                    &&
                    BTtest((i4)cop->opo_ordeqc,
                        (char *)subquery->ops_bfs.opb_bfeqc))
                    cop->opo_ordeqc = OPE_NOEQCLS;
	    }
	    else 
	    {   /* ordering was already created previously */
                if(!varp->opv_mbf.opb_prmaorder)
                {
                    opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
                        &varp->opv_mbf.opb_prmaorder, FALSE, TRUE); /* get the
                                                ** equivalence class list for the
                                                ** ordering */
                }
                cop->opo_ordeqc = opo_cordering(subquery,
                    varp->opv_mbf.opb_prmaorder, FALSE);
	    }
	    if ((orig_cop->opo_storage == DB_BTRE_STORE)
		&&
		(cop->opo_ordeqc != OPE_NOEQCLS))
		cop->opo_storage = DB_SORT_STORE;  /* mark node as sorted on the
						** ordering equivalence class if we
						** can guarantee it */
	}
        if (cop->opo_ordeqc == OPE_NOEQCLS)
            cop->opo_storage = DB_HEAP_STORE;
        else
	{
	    OPE_IEQCLS	    ordeqc;
	    OPE_IEQCLS	    maxeqcls;

	    maxeqcls = subquery->ops_eclass.ope_ev;
	    ordeqc = cop->opo_ordeqc;
	    if (ordeqc < maxeqcls)
	    {
		if (!BTtest((i4)ordeqc, (char *)&cop->opo_maps->opo_eqcmap))
		    cop->opo_ordeqc = OPE_NOEQCLS; /* do not return ordering
						** if it is not available in
						** output of CO node */
	    }
	    else
	    {	/* this is a multi-attribute ordering so check that all
		** attributes are available for the parent */
		OPO_SORT	    *sortp;
		sortp = subquery->ops_msort.opo_base->opo_stable[ordeqc-maxeqcls];
		if (!BTsubset((char *)sortp->opo_bmeqcls, (char *)&cop->opo_maps->opo_eqcmap,
		     (i4)maxeqcls))
		{   /* need to remove equivalence classes not available to
		    ** the parent, but skipping over the constant equivalence
		    ** classes which are not available since these should not
		    ** effect the ordering */
		    OPO_EQLIST		   *keylist;
		    OPE_IEQCLS		   first_eqcls;
		    i4			   att_count;
		    OPE_IEQCLS		   *masterp;
		    OPE_IEQCLS		   *eqclsp;

		    keylist = (OPO_EQLIST *)NULL;
		    att_count = 0;
		    for (masterp = &sortp->opo_eqlist->opo_eqorder[0];
			*masterp != OPE_NOEQCLS; masterp++) 
		    {
			if (subquery->ops_bfs.opb_bfeqc
			    &&
			    BTtest((i4)(*masterp), (char *)subquery->ops_bfs.opb_bfeqc))
			    continue;	    /* if this eqcls is part of a constant
					    ** class then we can ignore it and
					    ** continue , b41939 */
			if (!BTtest((i4)(*masterp), (char *)&cop->opo_maps->opo_eqcmap))
				break;	    /* the remainder of the
					    ** ordering is not useful to the
					    ** parent since this eqcls is not
					    ** returned */
			att_count++;
			if (att_count == 1)
			    first_eqcls = *masterp;	    /* only one eqcls is found */
			else
			{
			    if (att_count == 2)
			    {   /* allocate memory for multi-attribute ordering */
				i4			   maxelement;
				maxelement = BTcount((char *)sortp->opo_bmeqcls, (i4)maxeqcls);
				keylist = (OPO_EQLIST *)opu_memory(subquery->ops_global, 
				    (i4) (sizeof(OPO_ISORT) * (maxelement+1))); /* FIXME - 
								** for now place this into
								** non-enumeration memory, it is safer */
				eqclsp = &keylist->opo_eqorder[0];
				*(eqclsp++)=first_eqcls;
			    }
			    *(eqclsp++) = *masterp; /* this eqcls is useful so
						    ** transfer to the ordering */
			}
		    }
		    if (att_count == 0)
		        /* more than one eqcls in the final ordering */
			cop->opo_ordeqc = OPE_NOEQCLS;
		    else if (att_count == 1)
		    	/* exactly one eqcls left */
			cop->opo_ordeqc = first_eqcls;
		    else
		    {
		    	/* a multi-attribute ordering still exists */
			*(eqclsp++) = OPE_NOEQCLS;
			cop->opo_ordeqc = opo_cordering(subquery, keylist, FALSE);
		    }
		}
	    }
	}
    }
}

/*{
** Name: opo_orig	- create key order
**
** Description:
**      Create an equivalence class descriptor for the keying attributes.
**      This is an ordered list of equivalence classes.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to DB_ORIG node
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
**      23-feb-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
VOID
opo_orig(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
    if
    (
	cop->opo_sjpr == DB_ORIG
	&&
	cop->opo_ordeqc >= 0
    )
    {
	OPV_VARS               *varp;       /* pointer to range variable
					    ** element associated with leaf */

	varp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig];

	if (varp->opv_mbf.opb_count == 1)
	{
	    cop->opo_ordeqc = subquery->ops_attrs.opz_base->
		opz_attnums[varp->opv_mbf.opb_kbase->
		    opb_keyorder[0].opb_attno]->opz_equcls; /* get the
					    ** equivalence class associated
					    ** with the single attribute
					    ** ordering */
	}
	else 
	{   /* ordering was already created previously */
	    if(!varp->opv_mbf.opb_maorder)
	    {
		opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
                    &varp->opv_mbf.opb_maorder, FALSE, FALSE); /* get the equivalence
					    ** class list for the ordering */
	    }
	    cop->opo_ordeqc = opo_cordering(subquery, 
		varp->opv_mbf.opb_maorder, FALSE);
	}
    }
}

/*{
** Name: opo_rsort	- sort attributes in the given order
**
** Description:
**      This is a sort node placed in the query plan.  Currently,
**      it will be used to process only the top sort node, but
**      could latter be used to describe other sorts, used to
**      flatten queries, and evaluate aggregates.  Sort nodes
**      which are used for joins are not specified by this node
**      since the join node itself specifies whether a sort needs
**      to be performed. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to top sort node
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
**      23-feb-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
VOID
opo_rsort(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop)
{
#ifdef OPM_FLATAGGON
    if (subquery->ops_bestco == cop)
#endif
	cop->opo_ordeqc = OPE_NOEQCLS;	    /* FIXME - currently top sort
					    ** nodes are not in the cost
					    ** model, need to fix this */
}

/*{
** Name: opo_kordering	- produce a key ordering
**
** Description:
**      Given the available equivalence classes from the outer determine 
**      a key ordering which can be used.
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      eqcmap                          ptr to available equivalence classes
**                                      from the outer
**      mbfp                            ptr to description of the key
**
** Outputs:
**	Returns:
**	    OPO_ISORT - key ordering of OPE_NOEQCLS if no keying can be done
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-mar-88 (seputis)
**          initial creation
**	5-jan-97 (inkdo01)
**	    Added check for single-valued subselect constant eqclass and
**	    1st col covered only by constant eqclass. Fixes bug 87509.
**	23-dec-04 (inkdo01)
**	    Mixed collation eqc's can't be used for key joins.
**	17-Jun-08 (kibro01) b111578
**	    Extend fix for 87509 to include hash table indices where we
**	    have to be able to guarantee every part of the index
[@history_line@]...
[@history_template@]...
*/
OPO_ISORT
opo_kordering(
	OPS_SUBQUERY       *subquery,
	OPV_VARS           *varp,
	OPE_BMEQCLS        *eqcmap)
{
    i4                  count;	    /* number of attributes from outer which
                                    ** can be used in the key */
    i4                  mincount;   /* minimum number of attributes which
                                    ** need to be found to make keying
                                    ** allowable */
    OPO_ISORT           *sortp;     /* ptr to current equivalence class in
                                    ** key list */
    bool		useless;    /* TRUE if a constant equivalence class
				    ** is found in the keylist but not
				    ** found in the eqcmap */
    switch (varp->opv_tcop->opo_storage)
    {
    case DB_HEAP_STORE:
	return(OPE_NOEQCLS);	    /* heap cannot be used for keying so this
                                    ** will be a physical join, i.e. cartesean
                                    ** product */
    case DB_RTRE_STORE:
    {	/* check first if this is the inner of a possible spatial join */
	/* Note that this doesn't deal with the possibility of several 
	** base tabs which all spatial join to the same inner. In such a 
	** case, there would be several spatial join eqc's associated
	** with the current inner, and this little loop would always
	** pick the first, regardless of cost characteristics. */
	OPE_IEQCLS	eqc;
	for (eqc = -1; (eqc = BTnext((i4)eqc, (char *)eqcmap,
		subquery->ops_eclass.ope_ev)) >= 0;)
	 if (subquery->ops_eclass.ope_base->ope_eqclist[eqc]->ope_mask
		& OPE_SPATJ && BTtest((i4)eqc, (char *)&varp->opv_maps.
		opo_eqcmap))	    /* got a spatial join eqclass in the
				    ** current variable - use this order */
	  return(eqc);
    }	/* if no match, let it drop through (though that oughtn't to happen) */

    case DB_BTRE_STORE:
    case DB_ISAM_STORE:
    case DB_SORT_STORE:
    {	/* any prefix of the available equivalence classes can be used */
	mincount = 1;		    /* at least one equivalence class
                                    ** must match */
	break;
    }
    case DB_HASH_STORE:
    {
	mincount = varp->opv_mbf.opb_mcount; /* all the attributes in the
                                    ** key must be found */
	break;
    }
    default:
	return(OPE_NOEQCLS);        /* FIXME - need consistency check */
    }

    if(!varp->opv_mbf.opb_maorder)
    {
	opo_makey(subquery, &varp->opv_mbf, varp->opv_mbf.opb_count,
            &varp->opv_mbf.opb_maorder, FALSE, FALSE); /* get the equivalence
				    ** class list for the ordering */
    }
    if (varp->opv_mbf.opb_maorder->opo_eqorder[0] == OPE_NOEQCLS)
	return(OPE_NOEQCLS);	    /* if no ordering of equivalence
                                    ** classes exists, then keying cannot be
                                    ** used */

    {
	bool		found;	    /* TRUE if attribute is from outer set of
				    ** equilvance classes */
	bool		key_useful; 

	count = 0;
	useless = FALSE;
	key_useful = FALSE;
	for(sortp = &varp->opv_mbf.opb_maorder->opo_eqorder[0];
	    (*sortp != OPE_NOEQCLS) 
	    && 
	    (   (found = BTtest((i4)*sortp, (char *)eqcmap)) /* check the number of attributes
					** which exist in the key from the outer*/
		||
		(
		    subquery->ops_bfs.opb_bfeqc /* or check if the attribute
					** is a constant value */
		    &&
		    BTtest((i4)*sortp, (char *)subquery->ops_bfs.opb_bfeqc)
		)
	    );
	    sortp++)
	{
	    if (subquery->ops_eclass.ope_base->ope_eqclist[*sortp]->
			ope_mask & OPE_MIXEDCOLL)
		continue;	    /* skip eqc's that are mixed collations */

	    key_useful |= found;
	    if (!found)
	    {
		/* if 1st key is only covered by a constant, AND there's a 
		** subselect constant, we can't guarantee the 1st col is covered
		** (87509)
		** Extend fix for 87509 to include hash table indices where we
		** have to be able to guarantee every part of the index
		** (kibro01) b111578
		*/
		if ((count == 0 || varp->opv_tcop->opo_storage == DB_HASH_STORE)
			 && subquery->ops_bfs.opb_mask & OPB_SUBSEL_CNST)
				return(OPE_NOEQCLS);

		useless = TRUE;	    /* at least one eqcls was not in the bitmap
				    ** which means it was available only because
				    ** it was a constant equivalence class */
	    }
	    count++;		    /* add number of eqcls found */
	}
	if (!key_useful)
	    return( OPE_NOEQCLS);   /* key join is made only of constant equivalence
				    ** classes and none are from the outer */
    }

    if (count >= mincount)
    {
	if (count == 1)
	    return((OPO_ISORT)varp->opv_mbf.opb_maorder->opo_eqorder[0]);/*
				    ** only one eqcls so return it */
	if ((count == varp->opv_mbf.opb_mcount)
	    &&
	    !useless)
	    return(opo_cordering(subquery, varp->opv_mbf.opb_maorder, FALSE)); /*
				    ** all the attributes are used so the
                                    ** existing list is applicable */
	{   /* partial key is used so need to allocate a new list */
	    OPO_EQLIST	    *newlist;
	    opo_makey(subquery, &varp->opv_mbf, count,
                &newlist, TRUE, FALSE);         /* get the equivalence
				    ** class list for the ordering */
	    return(opo_cordering(subquery, newlist, FALSE)); /* FIXME -wasted 
                                    ** memory, see if newlist exists first */
	}

    }
    else
	return(OPE_NOEQCLS);
}
