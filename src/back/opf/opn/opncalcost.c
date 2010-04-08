/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = dgi_us5 rs4_us5 sgi_us5 i64_aix
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
#include    <dmccb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <uld.h>
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
#define        OPN_CALCOST      TRUE
#include    <me.h>
#include    <bt.h>
#include    <mh.h>
#include    <opxlint.h>

/**
**
**  Name: OPNCALCOST.C - calculate cost ordering combinations
**
**  Description:
**      Routines which will match inner and outer cost orderings and 
**      calculate a cost ordering which is the result of there join
**
**  History:    
**      1-jun-86 (seputis)    
**          initial creation from calcost in calcost.c
**      15-aug-89 (seputis)
**          added several changes to fix b6538
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      28-jan-92 (seputis)
**          - fix 41730 - fix bad cost estimate problem due to using
**	    histograms for keying based on total selectivity rather than
**	    just the keyed selectivity.
**      5-jan-93 (ed)
**          bug 48049 - avoid placing constant equivalence classes in
**          ordering wherever possible
**          - removed redundant routines and used more general multi-attribute
**          routine opo_combine to determine if there is some sortedness
**          for the joining attributes.
**          - fixed problem with op255 trace point in which timeout was
**          one plan number later than specified.
**          -moved local structure typedefs to beginning of procedure
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      7-sep-93 (ed)
**          added uld.h for uld_prtree
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	06-mar-96 (nanpr01)
**	    - calculate the tpb based on pagesize. For variable pagesize
**	    project.
**	 6-jun-97 (inkdo01 from hayke02)
**	    In the function opn_highdups(), we now do not reduce the DIO
**	    estimates by a factor relative to the total cache of the server.
**	    This change fixes bug 79038.
**      26-aug-98 (hayke02)
**	    Added NO_OPTIM for dgi_us5 to prevent spurious corruptions to
**	    the various values used to calculate cpu in opn_sjcost(). This
**	    prevents inappropriate joining strategies being picked due to cpu
**	    estimates of MAXI4. This change fixes bug 92497.
**	02-May-2000 (ahaal01)
**	    Added NO_OPTIM for AIX (rs4_us5) to prevent E_OP0491 consistency
**	    check error.
**      03-aug-00 (wansh01)
**          Added NO_OPTIM for sgi_us5.
**          This prevents inappropriate joining strategies and causing
**          E_OP0491_NO_QEP error message while using accessdb to select
**          database and extend form.
**      12-oct-2000 (stial01)
**          Removed commented call to opn_sfdircost and changed key[1]perpage
**	    vars to key[1]perdpage to clarify as directory pages
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	12-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent "createdb" from producing
**	    an E_OP0901 error when creating the front-end system catalogs.
**	26-jun-2003 (somsa01)
**	    Removed NO_OPTIM for i64_win with GA version of Platform SDK.
**	30-Aug-2006 (kschendel)
**	    Eliminate redundant opo_topsort bool.
**	11-Mar-2008 (kschendel) b122118
**	    Comment clarifications.  Remove the ojfilter stuff, since it
**	    caused lots of extra meaningless grep hits.
**/

/*}
** Name: OPN_STAT
**
** Description:
**      summary of statistics describing the inner or outer
**
** History:
**      07-feb-88 (seputis)
**          initial creation
**	12-jun-90 (seputis)
**	    added support for the SORTMAX server startup parameter
[@history_template@]...
*/
typedef struct _OPN_STAT
{
    OPO_TUPLES		tuples;		/* number of tuples in relation */
    OPN_RLS		*rlsp;		/* OPN_RLS descriptor for inner/outer */
    OPN_EQS		*eqsp;          /* OPN_EQS descriptor */
    OPN_SUBTREE         *sbtp;          /* CO list for intermediate */
    bool		sort;		/* TRUE - if node is a sort node */
    bool                duplicates;     /* TRUE - if default should be
                                        ** overidden and duplicates should
                                        ** be removed */
    OPN_SDESC		stats;		/* list of stats describing all unique
					** sets of orderings */
    bool		allowed;	/* TRUE - if sort nodes are allowed
					** since tuple counts are within the
					** limits defined by SORTMAX */
} OPN_STAT;

/*}
** Name: OPN_SJCB - simple join control block
**
** Description:
**      This is the control block which contains the information used to
**      calculate a simple join cost
**
** History:
**      24-jan-88 (seputis)
**          initial creation due to length of parameter list
**	1-nov-89 (seputis)
**	    - fix b8521, added "keying" substructure to sjstate so that pagetouched
**	    estimate is done on page count prior to project restrict
**	20-jun-90 (seputis)
**	    - added cpfound to allow cart-prods to be used if multi-attribute
**	    keying is enhanced
**      31-jan-94 (markm)
**          - removed all references to cpfound (cartprod/best plan indication
**          is now checked through ops_mask OPS_CPFOUND).
**	25-apr-01 (hayke02 & inkdo01)
**	    Added boolean eonly_highdups to indicate that we should still call
**	    opn_higdups() for an existence_only (CO) K join because the keyjoin
**	    eqcls are a subset of the jordering eqcls - the key structure
**	    does not cover all the join attributes, and so extra DIO will
**	    be required to search for the non-key attribute join values.
**	    This change fixes bug 104552.
[@history_template@]...
*/
typedef struct _OPN_SJCB
{
    OPS_SUBQUERY	*subquery;
    OPN_STAT		keying;         /* statistics describing inner with keying */
    OPN_STAT		inner;          /* statistics describing inner */
    OPN_STAT		outer;          /* statistics describing outer */
    OPN_STAT            join;		/* statistics describing join result */
    OPO_ISORT		keyjoin;	/* joining set of equivalence classes */
    OPO_ISORT		jordering;	/* joining set of equivalence classes */
    OPO_ISORT           new_jordering;  /* if an ordering exists on the joining
                                        ** equivalence classes in which a
                                        ** full sort merge can be performed
                                        ** without sort nodes */
    OPO_ISORT           new_ordeqc;     /* new ordering of outer after join */
    OPL_IOUTER		ojid;		/* outer join id */
    bool		existence_only; /* TRUE - if join can quit on
                                        ** first match */
    bool		kjeqcls_subset;	/* TRUE - if keyjoin is a subset of
					** jordering */
    OPN_PERCENT		exist_factor;	/* percentage of tuples read before
					** existence_only effect takes place */
    bool		exist_bool;	/* TRUE - if existence_check will break
					** on first join match since no boolean
					** factors exist at this node */
    bool		notimptid;	/* TRUE - if an implicit TID is NOT
                                        ** involved in the join */
    OPO_BLOCKS		blocks;		/* total blocks for this node */
    OPO_BLOCKS		dio;		/* total disk I/O for this node */
    OPO_CPU		cpu;		/* total cpu I/O for this node */
    OPO_BLOCKS		pagestouched;	/* total pages touched for this node */
    OPO_TUPLES          tuplesperblock;	/* estimate of the number of tuples
                                        ** per disk block of the join
                                        ** result relation */
    i4		pagesize;       /* page size of the result relation */
    OPO_BLOCKS		jcache;         /* number of blocks used in the 
					** DMF cache required for this join */
    OPN_SDESC		*opn_cdesc;	/* ptr to next unallocated descriptor */
    OPO_BLOCKS		maxpagestouched; /* estimate of the maximum number of pages
					** touched */
    OPN_JTREE		*np;		/* ptr to current join descriptor */
    OPL_OJTYPE          opn_ojtype;     /* type of outer join to be evaluated at
                                        ** this node */
    OPO_TUPLES		key_nunique;	/* used if keying is a possibility,
					** - a worst case estimate of the number
					** of unique values of the portion of the
					** key specified 
					** - cannot use the estimate of oph_jselect
					** since it includes all boolean factors,
					** and other problems exist if calculating this
					** number in oph_jselect, since histograms
					** are only calculated once per set of relations
					** and keying on this relation may not
					** be an option in the original set
					** - b41730 */
    OPO_TUPLES		key_reptf;	/* used if keying is a possibility
					** - a lower bound estimate of repitition
					** factor */
    bool                partial_sort;   /* TRUE  if outer ordering provides
                                        ** a useful partial or full sort
                                        ** for the ordering */
    OPL_BMOJ		filter;		/* set of outer joins which have filter
					** requirements satisfied by children */
}   OPN_SJCB;

/*{
** Name: opn_dremoved	- check if duplicates are removed in this subtree
**
** Description:
**      Check if all duplicates are removed in this subtree, by a sort
**      node being placed above all the relations which require duplicates 
**      to be removed.  Part of no tid available solution.
**
** Inputs:
**      subquery                        ptr to subquery state variable
**      cop                             subtree to consider
**
** Outputs:
**      duprel                          set to TRUE if a relation which
**                                      requires duplicates removed is
**                                      found
**	Returns:
**	    TRUE if sort node has removed all duplicates
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
static bool
opn_dremoved(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	bool               *duprel)
{
    if (cop->opo_sjpr == DB_ORIG)
    {
	*duprel = BTtest((i4)cop->opo_union.opo_orig, (char *)subquery->ops_remove_dups);
	return(FALSE);
    }
    else
    {
	bool                left;	    /* TRUE if outer removes duplicates */
	bool                right;          /* TRUE if inner removes duplicates */
	bool                leftduprel;     /* TRUE if outer contains any relations
                                            ** which require duplicates to be removed
                                            */
	bool                rightduprel;    /* TRUE if inner contains any relations
                                            ** which require duplicates to be removed
                                            */

	if (cop->opo_outer)
	    left = opn_dremoved(subquery, cop->opo_outer, &leftduprel);
	else
	{
	    left = FALSE;
	    leftduprel = FALSE;
	}
	if (cop->opo_inner)
	    right = opn_dremoved(subquery, cop->opo_inner, &rightduprel);
	else
	{
	    right = FALSE;
	    rightduprel = FALSE;
	}
	if (leftduprel && rightduprel)
	{
	    *duprel = TRUE; 
	    return(FALSE);		    /* if relations with dups to be
                                            ** removed are in the left and right
					    ** branches then a sort node is needed
                                            ** higher up the tree */
	}
	*duprel = leftduprel || rightduprel; /* at least one duplicate relation in
                                            ** subtree */
	if (cop->opo_osort)
	{
	    cop->opo_odups = OPO_DREMOVE;
	    left |= leftduprel;
	}
	if (cop->opo_isort)
	{
	    cop->opo_idups = OPO_DREMOVE;
	    right |= rightduprel;
	}
        return (left			    /* left side already removes duplicates */
		||
		right                       /* right side already removes duplicates */
	       );
    }
}

/*{
** Name: OPN_SJ1	- estimate simple join cpu and dio cost
**
** Description:
**      This is a utility routine used by opn_sjcost below; assumes
**      that the outer is sorted or at least onunique should reflect
**      the sortedness of the outer.
**
** Inputs:
**      existence_only                  TRUE - if only existence of match
**                                      required
**      otuples                         count of some subset of tuples in 
**                                      outer relation
**                                      (not necessarily total count)
**      onunique                        number of unique values in the set of
**                                      tuples represented by otuples (of the
**                                      joining attribute)
**      jnunique                        number of unique values in the join
**                                      equivalence class 
**      iblocks                         a count of "disk read or write" units
**                                      not necessarily equal to one disk block
**                                      required for each outer tuple to read
**                                      in order to check all possible inner
**                                      tuples involved in the join
**      itpb                            a count of the number of tuples per
**                                      "disk unit"
**
** Outputs:
**      cpu                             estimate of CPU for simple join
**      dio                             estimate of "disk units" read or written
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-jun-86 (seputis)
**          initial creation from sj1
**      6-aug-02 (wanfr01)
**          Bug 108406, INGSRV 1855
**          Do not add negative cost onunique < jnunique
**          or else you may get a negative total cost.
[@history_line@]...
*/
static VOID
opn_sj1(
	bool               existence_only,
	OPO_TUPLES         otuples,
	OPH_DOMAIN         onunique,
	OPH_DOMAIN         jnunique,
	OPO_BLOCKS         iblocks,
	OPO_TUPLES         itpb,
	OPO_BLOCKS         *dio,
	OPO_CPU            *cpu)
{
    OPO_BLOCKS		temp_dio;
    /* (otuples * (jnunique / onunique)) is the number of
    ** tuples from the outer which participate in the join i.e.
    ** finds an inner tuple which matches;  take this number
    ** and multiple it by the number of blocks (i.e. overflow
    ** chain blocks) iblocks required to be scanned; if existence
    ** only is required then 1/2 of iblocks typically will be scanned
    ** - for each tuple which does not participate in the join
    ** i.e. (onunique - jnunique), then the entire iblock is scanned
    ** so existence only does not apply
    */
    if (iblocks < 1.0)
	iblocks = 1.0;	    /* at least 1 DIO is needed */
    if (existence_only)
	temp_dio = (otuples * jnunique * iblocks / (2.0 * onunique) )
	    + (onunique - jnunique) * iblocks;
    else
	temp_dio = (otuples * jnunique * iblocks / onunique)
	    + (onunique - jnunique) * iblocks;
    if (onunique < jnunique)
	    temp_dio -= (onunique - jnunique) * iblocks;
    *cpu += temp_dio * itpb;
    *dio += temp_dio;
}

/*{
** Name: opn_acap	- calculate a ball and cell problem
**
** Description:
**	Sprinkle the opn_eprime(r,m,n) balls on a circle divided into n cells.
**	the average distance between cells with a ball is n/opn_eprime(r,m,n).
**	If we randomly mark a cell as the beginning/ending of the relation
**	then the number of cells between this mark and a cell with a ball is
**	n/(opn_eprime(r,m,n)*2). therefore the number of cells needed to be
**	scanned is n - n/(opn_eprime(r,m,n)*2).
**
** Inputs:
**      r                               number of balls
**      m                               number of balls in n cells
**      n                               number of cells
**
** Outputs:
**	Returns:
**	    number of cells needed to be scanned
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-jun-86 (seputis)
**          initial creation from acap in acap.c
**	15-dec-91 (seputis)
**	    rework fix for MHexp problem to reduce floating point computations
[@history_line@]...
*/
static OPO_BLOCKS
opn_acap(
	OPO_TUPLES         r,
	OPO_TUPLES         m,
	OPO_BLOCKS         n,
	f8		   underflow)
{
    return (n - (n / (opn_eprime ((OPO_TUPLES)r, 
	(OPO_TUPLES)m, (OPH_DOMAIN)n, underflow) * 2.0)));
					/* gross estimate of "n"
					** - expected length of initial
					** run of non hit buckets when
					** r balls are selected from n
					** cells holding m balls each
					** without replacement
                                        */
}

/*{
** Name: opn_runs	- combinatorial problem
**
** Description:
**	Expected number of runs in a random ordering of m*t balls
**	where there are m balls each of t types
**
** Inputs:
**      m                               number of balls
**      t                               number of types
**
** Outputs:
**	Returns:
**	    expected number of runs
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-jun-86 (seputis)
**          initial creation from runs in runs.c
[@history_line@]...
*/
static OPO_BLOCKS
opn_runs(
	OPO_TUPLES         m,
	OPO_BLOCKS         t)
{
    if (m < 1.1)
	return (t);
    if (t < 1.1)
	return (1.0);
    if (2 * m < t)
	return (t * m);
    return (t * m - m + t / 2.0);		/* see comments in opn_sjcost */
}

/*{
** Name: opn_stats	- calculate stats given the histogram list
**
** Description:
**      This routine will calculate multi-attribute statistics given
**      a histogram list. 
[@comment_line@]...
**
** Inputs:
**      sjp                             ptr to calcost state descriptor
**      jordering                       set of eqcls which will have statistics
**					calculated as a group
**      rlp                             ptr to temp relation descriptor
**
** Outputs:
**      statp                           statistics on jordering
**      iattrp                           ptr to attribute, if jordering contains
**					exactly one equivalence class, otherwise
**					this is set to OPZ_NOATTR;
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      8-aug-89 (seputis)
**          initial creation
**      8-aug-90 (seputis)
**          fix b32350 - partial fix, which will not work in all cases
**	    of floating point exceptions
**      5-may-92 (seputis)
**          - b36075 - fix vldb qep problem, make sure number of unique values does
**	    not exceed tuple count estimate.
**	12-oct-00 (inkdo01)
**	    Skip composite histograms.
[@history_template@]...
*/
static VOID
opn_stats(
	OPN_SJCB	    *sjp,
	OPO_ISORT	    jordering,
	OPN_RLS		    *rlp,
	OPN_SDESC	    *statp,
	OPZ_IATTS	    *iattrp)
{
    statp->mas_next = (OPN_SDESC *)NULL;
    statp->mas_sorti = jordering;
    if (jordering == OPE_NOEQCLS)
    {	/* this routine expects to find an attribute */
#ifdef    E_OP048A_NOCARTPROD
	/* validity check for cartesian product */
	opx_error(E_OP048A_NOCARTPROD);
#endif
    }
    else 
    {
	OPE_IEQCLS	maxeqcls;
	OPH_HISTOGRAM	*histp;		/* histogram for the joining
					** equivalence class */
	OPS_SUBQUERY	*subquery;

	subquery = sjp->subquery;
	maxeqcls = subquery->ops_eclass.ope_ev;
	if (jordering < maxeqcls)
	{
	    histp = *opn_eqtamp(subquery, &rlp->opn_histogram, jordering, TRUE);
						/* find histogram of the 
						** relation - it must exist */
	    *iattrp = histp->oph_attribute;

	    statp->mas_nunique = histp->oph_nunique;
	    statp->mas_unique = histp->oph_unique;
	    statp->mas_reptf = histp->oph_reptf;
	    if (rlp->opn_reltups < statp->mas_nunique)
	    {
		statp->mas_nunique = rlp->opn_reltups;
		statp->mas_reptf = 1.0;
	    }
	}
	else
	{   /* multi-attribute joining equivalence class */
	    OPO_SORT	    *jsortp;
	    OPZ_AT	    *abase;	    /* ptr to array of ptrs to joinop
                                            ** attributes */
	    statp->mas_unique = FALSE;
	    statp->mas_nunique = 1;
	    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
	    *iattrp = OPZ_NOATTR;
	    jsortp = subquery->ops_msort.opo_base->
			opo_stable[jordering - maxeqcls];
	    for (histp = rlp->opn_histogram; histp; histp = histp->oph_next)
	    {   /* look at each of the histograms and determine the
		** multi-attribute info summary */
		OPE_IEQCLS		eqcls;
		if (histp->oph_mask & OPH_COMPOSITE) continue;
		eqcls = abase->opz_attnums[histp->oph_attribute]->opz_equcls;
		if (BTtest((i4)eqcls, (char *)jsortp->opo_bmeqcls))
		{   /* eqcls is a joining attribute so update stats */
		    if (histp->oph_unique)
			/* if one attribute in join is unique then
			** all are unique */
		    {
			statp->mas_unique = TRUE; /* FIXME - check if 
					    ** underlying multi-attribute
					    ** storage structure e.g.
					    ** unique BTREE implies
					    ** uniqueness */
			break;
		    }
		    statp->mas_nunique *= histp->oph_nunique; /* assume
					    ** best case in which all
					    ** attributes of join provide
					    ** extra uniqueness */
		    if (rlp->opn_reltups < statp->mas_nunique)
		    {
			statp->mas_nunique = rlp->opn_reltups;
			break;
		    }
		}
	    }
	    if (statp->mas_unique)
		statp->mas_nunique = rlp->opn_reltups;
	    if ( statp->mas_nunique < 1.0)
		statp->mas_nunique = 1.0;   /* number of unique values in
					    ** joining equivalence class from
					    ** relation - at least 1 */
	    statp->mas_reptf = rlp->opn_reltups / statp->mas_nunique; /*
					    ** calculate new repitition factor
					    ** for the multi-attribute join */
	    if (statp->mas_reptf < 1.0)
		statp->mas_reptf = 1.0;
	}
    }
}

/*{
** Name: opn_jstats	- calculates stats for a join result
**
** Description:
**      This routine will use the descriptors from the outer and inner
**	relations, to calculate info about the join result. 
**
** Inputs:
**      sjp                             ptr to state variable
**	jordering			ordering which is used to
**					produce the join
**	ostatp				ptr to outer stat descriptor
**					for jordering
**	istatp				ptr to inner stat descriptor
**					for jordering
**
**
** Outputs:
**      jstatp                          statistics for join result
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-aug-89 (seputis)
**          initial creation to correct b6538
[@history_template@]...
*/
static VOID
opn_jstats(
	OPN_SJCB	    *sjp,
	OPO_ISORT	    jordering,
	OPN_SDESC	    *jstatp,
	OPN_SDESC	    *ostatp,
	OPN_SDESC	    *istatp)
{
	
    /* calculate minimal stats for the join result which is required for cost calculation */
    jstatp->mas_sorti = jordering;

    jstatp->mas_nunique = sjp->join.tuples /
	(ostatp->mas_reptf * istatp->mas_reptf); /* number of unique
					** values in join result */
    if (jstatp->mas_nunique > ostatp->mas_nunique)
	jstatp->mas_nunique = ostatp->mas_nunique; /* restrict to be less than then
					** number of unique values from
					** outer relation */
    if (jstatp->mas_nunique > istatp->mas_nunique)
	jstatp->mas_nunique = istatp->mas_nunique; /* restrict 
					** to be less than then
					** number of unique values from
					** inner relation */
    if (jstatp->mas_nunique < 1.0)
	jstatp->mas_nunique = 1.0;	/* need to have at least one value
					** in active domain */
}

/*{
** Name: opn_astatp	- return a newly allocate stats descriptor
**
** Description:
**      This routine will allocate a stats descriptor from the available 
**      list or from memory. 
**
** Inputs:
**      sjp                             join state descriptor
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
**      11-aug-89 (seputis)
**          initial creation for b6538
[@history_template@]...
*/
static OPN_SDESC *
opn_astatp(
	OPN_SJCB	    *sjp)
{
    OPN_SDESC	    *retval;

    if (sjp->opn_cdesc)
    {
	retval = sjp->opn_cdesc;
	sjp->opn_cdesc= retval->mas_masterp; /* traverse the
					** master list contains unallocated
					** descriptors */
    }
    else
    {
	retval = (OPN_SDESC *)opn_memory(sjp->subquery->ops_global, (i4)sizeof(*retval));
	retval->mas_masterp = sjp->subquery->ops_global->ops_estate.opn_sfree;
	sjp->subquery->ops_global->ops_estate.opn_sfree = retval;
    }
    return(retval);
}

/*{
** Name: opn_fstatp	- find stats on subset of joining attributes
**
** Description:
**      This routine will look at the list of calculated stats for the joining
**	attributes and return if found, otherwise it will calculate them and store
**	and then return them. 
**
** Inputs:
**      sjp                             join state variable
**      ordering                        ordering containing attributes which participate
**					in the join
**      statp                           ptr to header descriptor for stats list
**
** Outputs:
**
**	Returns:
**	    ptr to stats which apply to the "ordering"
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-aug-89 (seputis)
**          initial creation for b6538
[@history_template@]...
*/
static OPN_SDESC *
opn_fstatp(
	OPN_SJCB	    *sjp,
	OPO_ISORT	    ordering,
	OPN_STAT	    *statp,
	OPN_SDESC	    *ostatp,
	OPN_SDESC	    *istatp)
{
    OPN_SDESC		*tstatp;
    OPE_IEQCLS		maxeqcls;
    OPS_SUBQUERY	*subquery;
    OPO_ST		*obase;		/* base of multi-attribute
					** ordering descriptors */
    subquery = sjp->subquery;
    maxeqcls = subquery->ops_eclass.ope_ev; /* set maximum number of
					    ** equivalence classes */
    obase = subquery->ops_msort.opo_base; /* ptr to sort descriptors */

    for (tstatp = &statp->stats; tstatp; tstatp = tstatp->mas_next)
    {
	if (ordering >= maxeqcls) 
	{   /* multi-attribute equivalence class */
	    if (tstatp->mas_sorti >= maxeqcls)
	    {	/* the stats only depend on equal sets
		** but not necessarily in the same order */
		OPO_SORT	*orderp1;
		OPO_SORT	*orderp2;
		orderp1 = obase->opo_stable[ordering - maxeqcls];
		orderp2 = obase->opo_stable[tstatp->mas_sorti - maxeqcls];
					/* FIXME - it would be nice to have
					** a BT routine for equality */
		if (BTsubset ((char *)orderp1->opo_bmeqcls, 
			(char *)orderp2->opo_bmeqcls, (i4)maxeqcls)
		    &&
		    BTsubset ((char *)orderp2->opo_bmeqcls, 
			(char *)orderp1->opo_bmeqcls, (i4)maxeqcls))
		    return(tstatp);	/* found one in which the bitmaps
					** are equal */
	    }
	}
	else if (ordering == tstatp->mas_sorti)
	    return(tstatp);		/* found single attribute ordering
					** stats */
    }
    {
	OPZ_IATTS	attr;
	/* cannot find stats for this set of equivalence classes so create a new one */
	tstatp = opn_astatp(sjp);	/* allocate a free stat pointer */
	if (ostatp)
	    opn_jstats(sjp, ordering, tstatp, ostatp, istatp); /* this is a JOIN stat
					** calculation because the outer and inner were
					** supplied */
	else
	    opn_stats(sjp, ordering, statp->rlsp, tstatp, &attr); /* initialize descriptor
					** for this set of equivalence classes */
	tstatp->mas_next = statp->stats.mas_next;	/* place into list */
	statp->stats.mas_next = tstatp;
    }
    return(tstatp);
}

/*{
** Name: opn_highdups	- make keying estimates given large repetition factor
**
** Description:
**      The routine makes keying cost adjustments given a large repetition
**      factor
**
** Inputs:
**      sjp                             simple join descriptor pointer
**      blocks_per_tuple                number of blocks scanned on average for
**					each tuple probe into the inner
**	estimate_blockstouched		estimate of added blocks touched due to
**					scan of data pages.
**
** Outputs:
**      dio                             number of dio's given that one block per
**					probe is read (i.e. low repetition
**					factor)
**      blockstouched                   ptr to estimate of blocks touched, used
**      				for table locking estimates
**
**	Returns:
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-feb-93 (ed)
**          initial creation for 47733
**	03-may-93 (rganski)
**	    Added cpup param and computation of CPU.
**	31-jan-94 (rganski)
**	    Ported from 6.4 to 6.5. Changed icop->opo_var to
**	    icop->opo_union.opo_orig.
**	 8-apr-97 (hayke02)
**	    We now do not reduce the DIO estimates by a factor relative to
**	    the total cache of the server. Assuming that no caching can take
**	    place if the number of pages exceeds the per session cache limit
**	    is pessimistic. However, the estimate for blocks_per_tuple is a
**	    minimum value, and therefore very optimistic. This change fixes
**	    bug 79038.
**	 8-apr-05 (hayke02)
**	   Use the inner's page size to determine the amount of available cache
**	   (maxcache).
[@history_template@]...
*/
static VOID
opn_highdups(
     OPN_SJCB	*sjp,
     OPO_CO	*ocop,
     OPO_CO	*icop,
     OPO_TUPLES	probes,
     OPO_BLOCKS	blocks_per_tuple,
     OPO_BLOCKS	*blockstouchedp,
     OPO_BLOCKS	*diop,
     OPO_BLOCKS	*datablockstouchedp,
     OPO_CPU	*cpup)
{   /* check if key is highly duplicated which would require several
    ** data pages to be scanned, FIXME check for btree, isam perhaps
    ** already has overflow data pages defined for duplicate keys 
    ** - FIXME check for handling of highly duplicated partial keys
    ** also, b41730 
    ** - do not use istatp->mas_reptf instead of sjp->key_reptf since
    ** istatp has histograms calculated for both keying and non-keying
    ** boolean factors and the selectivity would be abnormally low
    ** if sufficient non-keying boolean factors existed at this join
    ** node */
    OPO_COST	high_duplicates;
    OPS_STATE	*global;
    OPO_BLOCKS	readahead;
    OPO_BLOCKS	maxcache;

    global = sjp->subquery->ops_global;

    /* Add CPU for each key comparison */
    *cpup += probes * sjp->key_reptf;

    {	/* calculate readahead factor */
	OPE_IEQCLS  maxeqcls; /* max equivalence class */
	i4	key_count;  /* number of attributes in key join */

	maxeqcls = sjp->subquery->ops_eclass.ope_ev;
	if (sjp->keyjoin < maxeqcls)
	    key_count = 1;  /* single equivalence class */
	else
	    key_count = BTcount((char *)sjp->subquery->ops_msort.opo_base->
		opo_stable[sjp->keyjoin - maxeqcls]->opo_bmeqcls,
		(i4)maxeqcls); /* number of attributes
			    ** in multi-attribute ordering */
	/* DMF will use group scans if a partial key is specified, so use
	** the read ahead reduction factor to reduce cost of key joins
	** in cases where group scans are used */
	if (sjp->subquery->ops_vars.opv_base->opv_rt[icop->opo_union.opo_orig]
	    ->opv_grv->opv_relation->rdr_keys->key_count > key_count)
	    readahead = opn_readahead(sjp->subquery, icop);
	else
	    readahead = 1.0;
    }
    high_duplicates = probes /* number of unique 
			    ** values participating in the join */
	* blocks_per_tuple; /* number of 
			    ** blocks scanned 
			    ** per unique value */
    if ((*datablockstouchedp) < high_duplicates)
	(*datablockstouchedp) = high_duplicates;
    (*blockstouchedp) += (*datablockstouchedp); /* add estimates for data pages 
			    ** for blocks touched used for locking
			    ** estimates */
    if (blocks_per_tuple <= readahead)
	return;		    /* since group reads will fetch all blocks
			    ** in one DIO there will be not much difference
			    ** in terms of cost for the high duplicate factor */

    /* adjust dio estimate if high duplication exists
    ** so that pages scanned cannot fit into cache */
    (*diop) *= (blocks_per_tuple/readahead); /* since *diop is the number
			    ** of DIOs assuming only one probe
			    ** per unique outer tuple, this needs to
			    ** be adjusted for highly duplicated
			    ** keys */

    if (icop->opo_cost.opo_pagesize == P2K)
	maxcache = global->ops_estate.opn_maxcache[0];
    else if (icop->opo_cost.opo_pagesize == P4K)
	maxcache = global->ops_estate.opn_maxcache[1];
    else if (icop->opo_cost.opo_pagesize == P8K)
	maxcache = global->ops_estate.opn_maxcache[2];
    else if (icop->opo_cost.opo_pagesize == P16K)
	maxcache = global->ops_estate.opn_maxcache[3];
    else
	maxcache = global->ops_estate.opn_maxcache[4];
    if (blocks_per_tuple > maxcache)
    {
	high_duplicates = (blocks_per_tuple/readahead) * ocop->opo_cost.opo_tups; /* worst case number of tuples */
	if (high_duplicates > (*diop) )
	    (*diop)  = high_duplicates;
    }
}

/*{
** Name: opn_sortcpu	- check if cpu cost for sort should be used
**
** Description:
**      The routine determines if the CPU cost for an internal sort
**	node should be added to the cost.  If the tuple count is small
**	then it may be desirable to bais towards a sort node in case
**	the tuple count was vastly underestimated. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      tuples                          number of tuples in the sort
**      jordering                       join ordering
**      cop                             ptr to child subtree be sorted
**
** Outputs:
**
**	Returns:
**	    TRUE if CPU cost should be added
**	Exceptions:
**
** Side Effects:
**
** History:
**      19-jan-94 (ed)
**          added check to make sure a sort is not performed on top
**	    of already sorted data
[@history_template@]...
*/
bool
opn_sortcpu(
    OPS_SUBQUERY	*subquery, 
    OPO_TUPLES		tuples, 
    OPE_IEQCLS		jordering, 
    OPO_CO		*cop)
{
    bool	cheap_osort;	/* TRUE if CPU cost should be
				** calculated */
    if ((tuples > 3.0)		/* magic number */
	    &&
	 (jordering != OPE_NOEQCLS)
	)
	return(TRUE);
    else
    {
	/* check if outer is sorted on appropriate eqcls and make
	** sort node slightly more expensive in this case,
	** otherwise make sorting cheaper if outer tuple count
	** is small, since sort may be very useful if tuple count
	** is grossly underestimated, and it is not that expensive
	** if it actually is small */

	if ((jordering < 0) || (cop->opo_ordeqc < 0))
	    return(FALSE);
	else 
	{   /* if there is already some ordering which can be
	    ** taken advantage of then do not make sort nodes
	    ** cheaper since sorting already sorted data is
	    ** probably not good, unless it eliminates duplicates
	    */
	    OPE_IEQCLS	    compatible_join;
	    OPE_IEQCLS	    compatible_order;
	    compatible_order = opo_combine(
		subquery,
		jordering,  /* join on the eqcls in this set */
		cop->opo_ordeqc, /* is there some common ordering? */
		jordering, /* check if outer ordering
			** is compatible with joining equivalence classes */
		&cop->opo_maps->opo_eqcmap, 
		&compatible_join); /* if orderings are compatible
			** then at least one equivalence class will
			** be int this set */
	    return((compatible_join != OPE_NOEQCLS)
		&& (compatible_order != OPE_NOEQCLS));
	}
    }
}

/*{
** Name: opn_holdfactor - find out the holdfactor given a pagesize. 
**
** Description:
**      This routine finds out the holdfactor given a pagesize.
**
** Inputs:
**	subquery	    - to get access to the ops_global structure
**      page size           - page size will be one of:
**                              2048, 4096, 8192, 16384, 32768, 65536
**
**
** Outputs:
**      Returns:
**          holdfactor.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      06-mar-96 (nanpr01)
**          initial creation.
**	    Alternatively, this routine can be optimized to get to the
**	    index by just shifting
*/
static OPO_BLOCKS 
opn_holdfactor (
   OPS_SUBQUERY *subquery, 
   i4 pagesize )
{
   i4	page_sz;
   i4           i;
 
   for (i = 0, page_sz = DB_MINPAGESIZE; i < DB_NO_OF_POOL; i++, page_sz *= 2)
   {
       if (pagesize == page_sz)
	 break;
   }
   return((OPO_BLOCKS)subquery->ops_global->ops_estate.opn_sblock[i]);
}
 
/*{
** Name: opn_sjcost	- calculate cost for doing a simple join
**
** Description:
**	Calculate cost of doing a simple join in terms of disk io (the 
**	expected number of disk accesses required), and (cpu is the
**	number of tuple compares and moves required)
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      jordering                          equivalence class used to join the
**                                      inner and outer
**      ocop                            cost ordering info from the outer
**                                      relation
**      ohistp                          histogram of joining equivalence class
**                                      from outer relation
**      icop                            cost ordering info from the inner
**                                      relation
**      ihistp                          histogram of joining equivalence class
**                                      of the inner relation
**      tuples                          estimated number of tuples in the
**                                      join of inner and outer
**      otuples                         estimated number of tuples in the
**                                      outer relation
**      ituples                         estimated number of tuples in the
**                                      inner relation
**      iwidth                          number of bytes in a tuple from the
**                                      inner relation
**      existence_only                  TRUE if only the existence of a match
**                                      is required
**
** Outputs:
**      diop                            estimated disk i/o of join - blocks
**                                      read and written
**      cpup                            estimated cpu for join - number of
**                                      tuples compares and moves
**      pagestouched                    estimated pagestouched for join
**	Returns:
**	    cost of simple join
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-jun-86 (seputis)
**          initial creation from sjcost
**	5-oct-88 (seputis)
**          take into account in-memory sorting by QEF
**	26-nov-88 (seputis)
**          fix uninitialized variable problem
**	11-aug-89 (seputis)
**          fix b6538
**	1-nov-89 (seputis)
**	    - fix b8521, added "keying" substructure to sjstate so that pagetouched
**	    estimate is done on page count prior to project restrict
**	9-nov-89 (seputis)
**	    - fixed unix lint problem
**	11-dec-89 (seputis)
**	    - fix TID join problem, in which DB_SORT_STORE was assumed for wrong attribute
**	7-apr-90 (seputis)
**	    - fix bug 21086
**	24-jun-90 (seputis)
**	    - fix b8410 - overestimate of DIO with some multi-attribute keyed joins
**	16-jul-90 (seputis)
**	    - fix sequent floating point violation
**	20-feb-91 (seputis)
**	    - part of fix for b33279 - btree CPU calculation incorrect
**	8-apr-91 (seputis)
**	    - changed hold file calculation as part of support for
**	    QEF hold file handling for 6.4
**	23-sep-91 (seputis)
**	    - add parameter to pass underflow check value for MHpow fix
**	7-dec-93 (ed)
**	    - corrected estimate for keyed unsorted outer in which index_hit
**	    instead of effective_tuples was used causing cost underestimates
**	03-may-93 (ed, rganski)
**	    Added calls to opn_highdups() when keying into a Btree with high
**	    key duplication. This accounts for scanning extra leaf pages.
**	31-jan-94 (rganski)
**	    Ported above change (03-may-93) from 6.4 to 6.5. This is a fix for
**	    b47104 and b47733 (keyjoin bug).
**	22-apr-94 (ed)
**	    b59461 Take into account that for FSM joins, that 6.5 will only place into
**	    a hold file a number of tuples with identical key instead of the
**	    entire inner.
**	8-may-94 (ed)
**	    b58894 - fix inefficient query plans for right and full cart-prods
**	06-mar-96 (nanpr01)
**	    The dependencies on DB_PAGESIZE has been removed for variable page
**	    size project. Rest of the DB_PAGESIZE reference has been ifdef 0
**	    and no more used previous to this change.
**	 1-apr-99 (hayke02)
**	    Use tbl_id.db_tab_index and tbl_storage_type rather than
**	    tbl_dpage_count <= 1 to check for a btree or rtree index. As of
**	    1.x, dmt_show() returns a tbl_dpage_count of 1 for single page
**	    tables, rather than 2 in 6.x. This change fixes bug 93436.
**	10-Mar-2000 (wanfr01)
**	    Bug 100640, INGSRV 1120
**	    Allow opn_highdups() to be called for keyed structures other than 
**	    btrees
**	19-apr-01 (inkdo01)
**	    Recompute cpu/dio for sorts on output of existence only joins.
**	 9-sep-02 (hayke02)
**	    Modify the fix for bug 104552 so that eonly_highdups is renamed 
**	    kjeqcls_subset and is used as before. kjeqcls_subset is now
**	    also used to indicate that ocop->opo_cost.opo_tups rather than
**	    jstatp->mas_nunique should be used for 'probes' in the first two
**	    opn_highdups() calls (when the outer is sorted). This will
**	    result in higher and more accurate DIO and CPU estimates for
**	    multi-attribute joins where one or more of the non-primary keys
**	    are unsorted. This change fixes bug 108522.
**      6-aug-02 (wanfr01)
**          Bug 108406, INGSRV 1855
**          Do not add negative cost onunique < jnunique
**          or else you may get a negative total cost.
**      4-nov-02 (wanfr01)
**          Bug 108406, INGSRV 1855
**          Extension of prior bug fix to bug 108406
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**     18-Feb-03 (hanal04) Bug 109648 INGSRV 2103
**          Extend the above change to prevent further case of -ve
**          cpu cost estimate.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	28-sep-04 (inkdo01)
**	    Increase estimates for key joins with join outer to bias
**	    against their use in the face of potentially inaccurate estimates.
**	20-oct-04 (inkdo01)
**	    Retract above change because of adverse effect on other queries.
[@history_line@]...
*/
static VOID
opn_sjcost(
	OPN_SJCB	*sjp,
	OPO_CO		*ocop,
	OPO_CO		*icop)
{
    OPS_SUBQUERY	*subquery;
    bool                memsort;	    /* TRUE if tuples from inner
                                            ** relation can fit into a
                                            ** sorter's memory buffer */
    OPO_CPU             cpu;                /* amount of cpu used during
                                            ** processing of the join node
                                            ** only */
    OPO_BLOCKS          dio;                /* amount of disk i/o used during
                                            ** processing of this node */
    OPO_BLOCKS          blockstouched;      /* used to calculate
                                            ** number of disk pages accessed
                                            ** during the join */
    OPO_BLOCKS          itotblk;            /* total number of blocks in the
                                            ** inner relation */
    OPO_BLOCKS          holdfactor;         /* number of pages which can be
                                            ** buffered in a temporary memory
                                            ** resident buffer associated with
                                            ** the hold file - this is made
                                            ** equal to one "disk i/o" for
                                            ** cost calculation purposes */
    bool                idiskrel;           /* TRUE if inner relation is
                                            ** disk resident */
    OPS_WIDTH		owidth;		    /* width of outer tuple */
    OPS_WIDTH		iwidth;		    /* width of inner tuple */
    OPO_BLOCKS		cache_usage;        /* number of blocks of the cache
					    ** which have been used */
    OPO_BLOCKS		cache_available;    /* number of blocks still available
					    ** in the cache for use in a TID
					    ** or key join */
    OPS_STATE		*global;
    OPN_SDESC		*istatp;	    /* ptr to inner statistics descriptor */
    OPN_SDESC		*ostatp;	    /* ptr to outer statistics descriptor */
    OPN_SDESC		*jstatp;	    /* ptr to join statistics descriptor */
    OPN_SDESC		*kstatp;	    /* ptr to keying statistics descriptor */
    OPO_ISORT		jordering;	    /* join ordering used for this cost
					    ** calculation */
    bool                rightfull;          /* TRUE if a right or full join is
                                            ** selected */
    bool                keyed_join;         /* TRUE if keyed lookup be made
                                            ** using attributes of the outer
                                            ** relation on an ordering of
                                            ** the inner relation */
    bool                osorted;	    /* TRUE - if outer is sorted either
                                            ** by a sort node OR an ordering
                                            ** on the joining equivalence class?
					    */
    i4		max_pagesize;
    OPO_BLOCKS          iholdfactor,        /* number of pages which can be  */
                        oholdfactor;        /* buffered in a temporary memory
                                            ** resident buffer associated with
                                            ** the hold file - this is made
                                            ** equal to one "disk i/o" for
                                            ** cost calculation purposes */
    OPO_BLOCKS          dio_cost;           /* disk i/o cost checking */
    OPH_DOMAIN          unique_count;

    subquery = sjp->subquery;
    global = subquery->ops_global;
    owidth = sjp->outer.eqsp->opn_relwid;	/* width of outer tuple */
    iwidth = sjp->inner.eqsp->opn_relwid;	/* width of inner tuple */
    max_pagesize = global->ops_cb->ops_server->opg_maxpage; 
    holdfactor = opn_holdfactor(subquery, max_pagesize);
					/* DB_HFBLOCKSIZE / DB_MINPAGESIZE */
    oholdfactor = opn_holdfactor(subquery, ocop->opo_cost.opo_pagesize);
    iholdfactor = opn_holdfactor(subquery, icop->opo_cost.opo_pagesize);
    itotblk = icop->opo_cost.opo_reltotb;
    blockstouched = itotblk;
    dio = 0.0;
    cpu = 0.0;
    cache_usage = 0.0;
    rightfull = (sjp->opn_ojtype == OPL_RIGHTJOIN) /* a right or full join always
                                    ** creates a hold file except for FSM joins
                                    ** since a TID is necessary for processing */
        ||
        (sjp->opn_ojtype == OPL_FULLJOIN);
    /* FIXME - remember to initialize ops_qsmemsize , also sorter and HOLD
    ** file assumptions have changed */
    /*((inner relation tuple width) + (sort record overhead)) * (inner tuples)*/
    memsort = ( ((iwidth + DB_SROVERHEAD) * sjp->inner.tuples)
	        <=
		global->ops_cb->ops_alter.ops_qsmemsize
	      );			    /* TRUE - if inner's tuples fit into
                                            ** sorter's memory buffer */
    idiskrel = (!memsort) || (icop->opo_sjpr == DB_ORIG); /* TRUE 
					** if inner is disk resident */
    jordering = sjp->new_jordering;
    osorted = FALSE;
    if ((jordering != OPE_NOEQCLS) && (jordering != sjp->jordering))
    {	/* only a subset of the joining attributes are participating and the
	** remaining joining attributes will be evaluated like a normal
	** boolean factor after the join is completed, so lookup the stats
	** on the subset */
	istatp = opn_fstatp(sjp, jordering, &sjp->inner, 
	    (OPN_SDESC *)NULL, (OPN_SDESC *)NULL); /* get descriptor
					    ** for inner relation */
	ostatp = opn_fstatp(sjp, jordering, &sjp->outer
	    , (OPN_SDESC *)NULL, (OPN_SDESC *)NULL); /* get descriptor
					    ** for the outer relation */
	jstatp = opn_fstatp(sjp, jordering, &sjp->join, ostatp, istatp); /* get descriptor
					    ** for the joined relation */
    }
    else
    {	/* these stats have been precalculated for the join which uses all
	** the attributes possible */
	istatp = &sjp->inner.stats;
	ostatp = &sjp->outer.stats;
	jstatp = &sjp->join.stats;
    }
    if (sjp->keyjoin >= 0)
	kstatp = &sjp->keying.stats;
    else
	kstatp = istatp;		    /* probably not used but just in
					    ** case */

    keyed_join = FALSE;
        /* FIXME check if all the attribute of the outer are sorted, and all
        ** attributes are involved in the join, (or a cart prod. i.e. no
        ** join is defined) then any sort node of the inner can be added to 
        ** the sortedness of the result, to give more flexibility for a
        ** multi-attribute sort */

    if ((icop->opo_sjpr == DB_ORIG)
	&&
	(subquery->ops_vars.opv_base->opv_rt[icop->opo_union.opo_orig]->
	    opv_subselect		    /* is this a virtual relation */
	)
	&&
	(
	    (subquery->ops_vars.opv_base->opv_rt[icop->opo_union.opo_orig]->
		opv_grv->opv_created == OPS_SELECT /* check for all
					    ** subselect virtual relations */
	    )
	    ||
	    (BTcount((char *)&subquery->ops_vars.opv_base->opv_rt[icop->
		opo_union.opo_orig]->opv_grv->opv_gsubselect->
		opv_subquery->ops_correlated, OPV_MAXVAR) /* if
					    ** not a subselect but is a
					    ** correlated aggregate then
					    ** the cost can also be affected
					    ** by the ordering of the outer */
	    )
	)
	)
    {	/* check for virtual relations which may have subqueries reexecuted
	** depending on the ordering of the outer correlated equivalence
	** classes */
        /* a virtual table has costs associated with each tuple
	** of the inner, if the outer supplies correlated values
	*/
	/* FIXME - need to consider the changing "key" which may require
        ** a rescan of the inner tuples, i.e. r.a = ALL select s.b from s
        ** will require a rescan of the inner for each unique value of the
        ** outer if sorted */
	OPS_SUBQUERY	*virtual;	/* set to subquery only if the
                                        ** inner is a relation which
					** is virtual but not a view */
	virtual = subquery->ops_vars.opv_base->opv_rt[icop->opo_union.opo_orig]->
		opv_grv->opv_gsubselect->opv_subquery;
	if (virtual->ops_bestco)
	{   /* if the child subquery does not exist, it is due to the fact that
            ** it is degenerate, i.e. no range variables, so there is added cost
            ** in evaluated the node.  An example of this is
            **    select r.a from r where 5 > select (max(r.a) from r)
	    ** in which the simple aggregate is calculated, but the subselect
            ** aggregate does not have any variables so "virtual" would be null
            ** here, FIXME, virtual queries without CO trees should be flattened.
            */
	    OPO_TUPLES		tup_factor; /* number of tuples which cause
					    ** reevaluation of the subselect
					    */
	    if (!BTcount((char *)&virtual->ops_correlated, OPV_MAXVAR))
		tup_factor = 1.0;	    /* non corelated so subquery is
					    ** evaluated once */
	    else if (jordering != OPE_NOEQCLS)
	    {
		tup_factor = ostatp->mas_nunique; /* sma_nunique - since the outer is sorted
					    ** few evaluation of the inner are
					    ** required, FIXME - this does 
					    ** not seem like the correct
					    ** check for the outer being sorted */
		if (tup_factor > (ocop->opo_cost.opo_tups/2.0))
		    tup_factor = ocop->opo_cost.opo_tups/2.0; /* hack to bais
					    ** subselect to sort on the correlated
					    ** attributes since subselects are
					    ** expensive operations */
	    }
	    else
		tup_factor = ocop->opo_cost.opo_tups; /* the outer is not sorted
					    ** so subselect needs to be
					    ** evaluated each time */
	    dio += virtual->ops_bestco->opo_cost.opo_dio * tup_factor; 
					    /* for each outer tuple 
					    ** with a unique set of 
					    ** correlated attributes, the
					    ** inner subquery has to be
					    ** evaluated again 
					    ** FIXME - make sure the
					    ** key values are included
					    ** in the outer list to be
					    ** sorted */
	    cpu += virtual->ops_bestco->opo_cost.opo_cpu * tup_factor; 
	    /* FIXME - add subquery caching costs at the top of the plan,
	    ** this would normally be non-zero only for correlated plans
	    ** which do not get put into temporaries or sort nodes */
	}
    }
    else if (jordering == OPE_NOEQCLS)/* check for cartesean product*/
    {   /* cartesian product found */
        if (idiskrel)
	{
	    OPO_BLOCKS		seq1_blocks; /* number of sequential block I/Os
					** for this scan, typically sequential
					** block I/O size is larger than random I/O
					*/
	    seq1_blocks = itotblk / iholdfactor;
	    if (seq1_blocks < 1.0)
		seq1_blocks = 1.0;	/* at least 1 I/O */
	    dio += sjp->outer.tuples * seq1_blocks; /* for join - scan
					** through the entire inner relation
					** once for each tuple in the
					** outer relation, hold files,
					** scans of relations, or sort
					** files all use bigger pages so divide
					** by this larger factor , add cost
					** of creating hold file for inner,
					** but note that it is never created
					** for the outer */
#if 0
hold file calculation is added at end of routine
#endif
	}
	cpu += sjp->outer.tuples * sjp->inner.tuples; /* look at all the inner tuples
					** once for each outer tuples */
	cache_usage = OPN_SCACHE + OPN_SCACHE; /* the first two blocks of any scan would
					** use the page cache prior to accelerating to
					** the group level cache */
    }
    else
    {	/* a regular join, not a cartesean product */
	bool                isorted;	    /* TRUE - if inner is sorted either
                                            ** by a sort node OR an ordering
                                            ** on the joining equivalence class?
                                            */
	OPV_VARS	    *varp;	    /* ptr to variable descriptor for
					    ** ORIG node */
	
	if (icop->opo_sjpr == DB_ORIG)
	{
	    varp = subquery->ops_vars.opv_base->opv_rt[icop->opo_union.opo_orig];
	    keyed_join = opn_ukey(subquery, &ocop->opo_maps->opo_eqcmap, OPE_NOEQCLS, 
		icop->opo_union.opo_orig, FALSE); /* TRUE - if the inner node can be
                                            ** probed i.e. can a keyed lookup
                                            ** be made using available 
                                            ** attributes from the outer node
                                            */
	}
	else
	    keyed_join = FALSE;
        /* FIXME - okay to have ordering like ISAM?? on the joining equivalence
        ** class here ?? maybe if ISAM overflow buckets can be sorted
        ** in mini sorts */
        /* if sjp->new_jordering is defined then an ordering with a join
        ** exists, this necessary implies the outer is ordered on
        ** sjp->new_jordering */
	if (sjp->outer.sort)
	{
	    osorted = TRUE;		    /* cache usage is reset since
					    ** the sort needs all tuples to complete
					    ** so that previous cache usage is not
					    ** applicable */
	    cache_usage = OPN_SCACHE;	    /* initial few reads will use the single
					    ** page cache and subsequent reads will
					    ** use the group page cache */
	}
	else
	{
	    /* make a check to see if the outer storage structure is sorted
	    ** and appropriate for use in this join, i.e. it is not
	    ** ordered on some other attribute */

	    if ((jordering == OPE_NOEQCLS)
		||
		(ocop->opo_storage != DB_SORT_STORE))
		osorted = FALSE;
	    else
                osorted = sjp->partial_sort;
	    cache_usage = ocop->opo_cost.opo_cvar.opo_cache; /* subtract cache usage since
					    ** a sort node has not collected all
					    ** the tuples and a data flow is used
					    ** from the outer */
	}
	if (sjp->inner.sort)
	{
	    isorted = TRUE;
	    cache_usage += OPN_SCACHE;	    /* initial few reads will use the single
					    ** page cache and subsequent reads will
					    ** use the group page cache */
	}
	else
	{
	    isorted =(jordering != OPE_NOEQCLS) 
		    &&
		      (	(icop->opo_sjpr != DB_ORIG)
			||
			(icop->opo_storage == DB_BTRE_STORE)
		      );		    /* if the inner is a DB_ORIG node
					    ** then this is a keyed lookup
					    ** or possibly a physical scan
                                            ** without any hold file, in this
                                            ** case the inner is sorted only in
                                            ** the case of a BTREE */
	    if (icop->opo_sjpr != DB_ORIG)
		cache_usage += icop->opo_cost.opo_cvar.opo_cache; /* subtract cache usage since
					    ** a sort node has not collected all
					    ** the tuples and a data flow is used
					    ** from the inner */
	    else if (!keyed_join)	    /* keyed join cache
					    ** usage will be added later in this procedure
					    */
		cache_usage += OPN_SCACHE;  /* initial few reads will use the single
					    ** page cache and subsequent reads will
					    ** use the group page cache */
	}
	cache_available = global->ops_estate.opn_maxcache[0] - cache_usage;
	if (cache_available < 0.0)
	    cache_available = 0.0;	/* calculate the cache available for probes */
	if (!sjp->notimptid)
	{   /* implied TID is on the inner */
	    OPO_BLOCKS	    probe_io;
	    /*
	    ** Calculate the number of blocks of the inner that will be touched.
	    ** This works whether the outer has duplicates or not.  Let each 
	    ** unique value of the outer be the balls and each block of the 
	    ** inner be the cells.  Then each cell holds itpb balls, so if we 
	    ** select the balls from the cells randomly without replacement
	    ** we have our equation.  If the outer is ordered on the 
	    ** tid's then each of the blocks is read only once and this 
 	    ** is the cost.  
	    */
	    probe_io = opn_eprime (ostatp->mas_nunique /*kma_nunique */, 
                icop->opo_cost.opo_tpb, itotblk,
                global->ops_cb->ops_server->opg_lnexp);
	    if (osorted)
	    {
		dio += probe_io;	/* this TID join assumes the outer
					** is sorted on explicit TID so
					** each page is touched once*/
		cache_usage += 1.0;	/* only 1 page of the cache will be
					** used and it will not be revisited */
	    }
	    else
	    {
		/*
		** If the outer is not sorted the problem has the following 
                ** analogy.  Let c be the number of blocks of the inner 
                ** which are touched, as determined above.  Let each outer 
                ** tuple be a ball.  Let each ball have a type which is 
                ** defined as the block number of the inner of the
		** tuple to which it joins.  So each ball will belong to one 
                ** of c types.  There will be an average of m=otuples/c balls 
                ** of each type.  Suppose the balls are labeled with their 
                ** type and randomlly ordered.  (this corresponds to the outer
                ** being not ordered).  Let a "run" be defined as a sequence 
                ** of one or more balls of the same type.  then for a given
		** run, all balls correspond to the same block of the inner.  
                ** This means that only one inner block need be read per run.
		** So the expected number of runs is also the expected number
                ** of reads from the inner.  Procedure "runs" calculates this 
                ** expected number of runs.  The expected number of runs in a 
                ** random ordering of otuples balls where there are m balls 
                ** each of t types (otuples=m*t) can be calculated as follows:
		** first let all the types except the first be grouped 
                ** together.  Then there are m balls of type 1 and otuples-m 
                ** balls of type 2.  The expected number of runs for this is
		**          (2*m*(otuples-m))/otuples + 1
		**          from pg. 278 of Hogg & Tanis.
		** Dividing by two gives the expected number of runs 
                ** involving type 1 balls.  Therefore the expected number 
                ** of runs involving a given type of ball is
		**          m*(otuples-m)/otuples + 1/2
		** and the expected number of runs is
		**          t*(m*(otuples-m)/otuples + 1/2) 
                **          = t*m - m + t/2.
		**
		** This assumes that each time we go to a new page, it is 
		** not in the page cache.
		*/
		if (probe_io <= cache_available)
		{   /* all data is available in the DMF cache so no extra
		    ** I/O's are needed */
		    dio += probe_io;
		    cache_usage += probe_io;
		}
		else
		{   
		    OPO_BLOCKS	    tempdio;
		    tempdio = opn_runs (sjp->outer.tuples / probe_io, probe_io);
		    if (tempdio <= probe_io)
			dio += probe_io;
		    else
			dio += (tempdio - probe_io) * ( 1.0 - cache_available/probe_io)
			    + probe_io;
		    cache_usage = global->ops_estate.opn_maxcache[0]; /* the
				** entire DMF cache has been used at this
				** point */
		}
	    }
	    cpu += sjp->outer.tuples;
	    blockstouched = probe_io;
	}
	else if (osorted)	/* not an implied tid join and outer sorted */
	{
	    OPO_BLOCKS		    exist_factor;

	    exist_factor = sjp->exist_factor;
	    if (keyed_join)
	    {	/* outer is sorted */
		/*
		** For each unique value in the outer we do a directory search
                ** or hash calculation.  Then for each tuple of the outer 
		** which participates in the join (oreptf*jnunique) we need 
		** to read through the blocks determined by the directory or 
		** hashing (in/ip - total blocks divided by primary blocks).  
		** For each unique value of the outer which does not 
		** participate in the join (ohistp->opo_cost.opo_nunique -
                ** jnunique) we need to read through the in/ip blocks once 
                ** to determine that this value does not indeed participate in
		** the join if we are doing an existence_only search then 
		** we only have to search half way through the linked list 
                ** for those tuples which participate in the join
		*/
		OPO_BLOCKS          iprimblks;	/* number of primary blocks
					** in inner relation */
		OPO_BLOCKS	    iblocks1; /* number of blocks scanned
					** for each new outer value */
		OPO_BLOCKS	    dircost;	/* number of blocks looked
					** at for directory search */
		OPO_TUPLES	    keyperdpage; /* number of keys that
					** can fit onto one directory page */
		OPO_BLOCKS	    datapages;
		DMT_TBL_ENTRY	    *relationp;
		OPO_TUPLES	    probes;

		relationp =  varp->opv_grv->opv_relation->rdr_rel;
		datapages = relationp->tbl_dpage_count;
		iprimblks = icop->opo_cost.opo_cvar.opo_relprim;
#if 0
**              if (iprimblks > sjp->inner.kma_nunique)
**		    iprimblks = sjp->inner.kma_nunique; /* causes over
**					    ** estimate for sparse tables */
**old calculation*/ /*static*/opn_sj1(sjp->existence_only, sjp->outer.tuples, 
**		    sjp->outer.kma_nunique, sjp->join.kma_nunique,
**		    itotblk / iprimblks, icop->opo_cost.opo_tpb, &dio, &cpu);
#endif
#if 0
/* old calculation */
**		iblocks1 = itotblk / iprimblks;	
					    /* average number of DIO
					    ** needed to read a overflow
					    ** chain */
#endif
		iblocks1 = (iprimblks + relationp->tbl_opage_count)/ iprimblks;	
					    /* average number of DIO
					    ** needed to read a overflow
					    ** chain */

		dircost = varp->opv_dircost;
		keyperdpage = varp->opv_kpb;
		if (icop->opo_storage == DB_BTRE_STORE)
		{   /* since index pages are dense, the data page will not
                    ** be scanned in a secondary index */
		    if (sjp->exist_bool)
			cpu += dircost * sjp->outer.tuples; /* one comparison for each 
					    ** directory page scanned, since join
					    ** will fail or succeed on first tuple */
		    else
		    {
                        unique_count = ostatp->mas_nunique - jstatp->mas_nunique;
			if (unique_count < 0)
			    unique_count = 0;
			cpu += (jstatp->mas_nunique /* the number of tuples
					    ** from the outer which participates
					    ** in the join */
			    * exist_factor /* if existence only then 1/2
						** of chain is read */
			    + unique_count) /* number of tuples
						** which do not participate in the
						** join */
			    * (sjp->outer.tuples / ostatp->mas_nunique ) /* repetition factor
						** of outer */
			    + dircost * sjp->outer.tuples; /* one comparison for each 
						** directory page scanned */
		    }
#if 0
	old calculation
**		    if (sjp->existence_only)
**			cpu += (jstatp->mas_nunique /* the number of tuples
**						** from the outer which participates
**						** in the join */
**			    * exist_factor      /* if existence only then 1/2
**						** of chain is read */
**			    + ostatp->mas_nunique - jstatp->mas_nunique) /* number of tuples
**						** which do not participate in the
**						** join */
**			    * (sjp->outer.tuples / ostatp->mas_nunique ) /* repetition factor
**						** of outer */
**                          * iblocks1 * icop->opo_cost.opo_tpb  /* number of inner
**                                              ** tuples looked at in each
**                                              ** chain */
**			    + dircost * sjp->outer.tuples; /* one comparison for each directory
**						** page scanned */
**		    else
**			cpu += sjp->outer.tuples * dircost; /* one comparison for each directory
**						** page scanned */
#endif
		}
		else
		{
		    /* for ISAM and HASH, the entire page is scanned */
		    if (sjp->existence_only)
		    {
                        unique_count = ostatp->mas_nunique - jstatp->mas_nunique
;
                        if (unique_count < 0)
                            unique_count = 0;

			if (sjp->exist_bool)
			{
			    /* due to existence only with no boolean factors, join
			    ** will exit on first tuple found, so with a high
			    ** repitition on inner, this will be done more quickly,
			    ** - assume original tuple is in the first page and
			    ** reduce the size of the search on the first page
			    ** by the repitition factor */
			    exist_factor = (1.0 - sjp->inner.tuples/ (istatp->mas_nunique *
				 icop->opo_cost.opo_tpb)) * sjp->exist_factor;
			    if (exist_factor < 0.0)
				exist_factor = 0.0;
			}
			cpu += (jstatp->mas_nunique /* the number of tuples
						** from the outer which participates
						** in the join */
			    * exist_factor      /* if existence only then 1/2
						** of chain is read */
			    + unique_count) /* number of tuples
						** which do not participate in the
						** join */
			    * (sjp->outer.tuples / ostatp->mas_nunique ) /* repetition factor
						** of outer */
			    * iblocks1 * icop->opo_cost.opo_tpb  /* number of inner
						** tuples looked at in each
						** chain */
			    + dircost * sjp->outer.tuples; /* one comparison for each directory
						** page scanned */
		    }
		    else
			cpu += sjp->outer.tuples	* (iblocks1 * icop->opo_cost.opo_tpb  /* each
						** outer tuple will be compared
						** to every tuple in the chain */
			    + dircost);	    /* one comparison for each directory
						** page scanned */
		}
		/* take into account the caching of the directory pages
		** since the outer is sorted, there will be many hits
		** on the nodes closest to root of the directory pages
		** - thus calculate number of directory pages touched
		** rather than number looked at */
		if ((keyperdpage > 0.0) && (dircost > 1.0))
		{	
		    OPO_TUPLES  cell_count;	/* number of cells at a
					    ** particular directory level
					    */
		    OPO_TUPLES  new_count;	/* maximum number of tuples
					    ** that can be represented
					    ** at this level
					    */

		    cell_count = keyperdpage;
		    blockstouched = 0.0;
		    while (dircost > 1.0)
		    {   /* ball and cell problem for caching hits 
			** use > 1.0 as opposed to >= 0.0 since 
			** we will assume that the root page of the
			** index is always in memory */
			OPO_BLOCKS	estimated;
			new_count = cell_count * cell_count;
			if (ostatp->mas_nunique < new_count)
			    estimated = opn_eprime ((OPO_TUPLES)
				ostatp->mas_nunique, /* number of
					    ** balls to select */
				(OPO_TUPLES)cell_count,  /* number of balls
					    ** in each cell */
                                (OPH_DOMAIN)cell_count, /* number of cells */
                                global->ops_cb->ops_server->opg_lnexp);
			else
			    estimated = cell_count; /* if there are more unique
					    ** values than tuples at this
					    ** level then all cells will
					    ** be hit at this level */
                        if (estimated > icop->opo_cost.opo_tups)
                            estimated = icop->opo_cost.opo_tups; /* cannot have
                                            ** more index page hits than tuples
                                            ** available, fixes problem with small
                                            ** btrees */
			if (estimated > sjp->maxpagestouched)
			    estimated = sjp->maxpagestouched;
			blockstouched += estimated;
			dircost = dircost - 1.0;
			cell_count = new_count;
			cache_usage += 1.0;	/* assume the index pages will get
					    ** priority, since outer is sorted
					    ** each page will be visited once */
		    }
		}
		else
		    blockstouched = dircost;
		/* blockstouched contains the number of index pages touched in order
		** to perform the lookup */
		if (!sjp->existence_only && sjp->kjeqcls_subset)
		    probes = (OPO_TUPLES)ocop->opo_cost.opo_tups;
		else
		    probes = (OPO_TUPLES)jstatp->mas_nunique;
		if (!(relationp->tbl_id.db_tab_index > 0
		     &&
		     (relationp->tbl_storage_type == DB_BTRE_STORE ||
		      relationp->tbl_storage_type == DB_RTRE_STORE)))
		{
		    /* BTREE secondary indexes do not have any data pages so that
		    ** only the index lookup should contribute to the cost in the
		    ** case of BTREE, other storage structures have data pages */
		    OPO_BLOCKS	    dio1estimated;
		    OPO_BLOCKS	    dio2estimated;

		    /* assume caching occurs since outer is sorted, the DIO
		    ** will be such that duplicate outer keys will not cause
		    ** extra DIO, but instead will count as 1 DIO for all
		    ** duplicates, this means using sjp->outer.kma_nunique instead
		    ** of sjp->outer.tuples;  note that this is not true for
		    ** CPU since caching will not reduce the comparisons */
		    dio1estimated = ostatp->mas_nunique + /* at least one dio
						** for each unique value in outer */
			(jstatp->mas_nunique * exist_factor +    
						/* for each
						** unique join value, DIO is
						** needed for the overflow
						** chain, if existence only
						** then fraction of chain is read
						** on average */
			ostatp->mas_nunique - jstatp->mas_nunique) /* for each
						** unique value which does not
						** participate in join, an
						** entire overflow chain is read
						** - this assumes domains are
						** complete */
			    * (iblocks1 - 1.0);
		    dio2estimated = opn_eprime(ostatp->mas_nunique, 
			      kstatp->mas_nunique / datapages,
                              datapages,
                              global->ops_cb->ops_server->opg_lnexp)
			* iblocks1;		/* FIXME - changed
						** jstatp to ostatp since it is probably
						** a better measure of the number of unique
						** values for probing (b8521), since jstatp
						** contains statistics after all selectivity
						** is applied, what is needed is getting the
						** info from oph_jselect prior to applying
						** the selectivity so that jstatp->mas_nunique
						** can contain the unique values as a result of
						** the join, and not after restrictions */
						/* relprim includes data and index pages but
						** not overflow pages */
                    if (dio1estimated > icop->opo_cost.opo_tups)
                        dio1estimated = icop->opo_cost.opo_tups; /* cannot have more
                                                ** DIOs than tuples */
		    if (dio1estimated > dio2estimated)
			dio1estimated = dio2estimated;
		    if (dio1estimated > sjp->maxpagestouched)
			dio1estimated = sjp->maxpagestouched; /* clustering of multiattribute
						** makes estimate lower */
                    if ((!sjp->existence_only || sjp->kjeqcls_subset)
                        &&
			(icop->opo_storage == DB_BTRE_STORE)
			&&
                        (sjp->key_reptf > icop->opo_cost.opo_tpb))
			/* adjust estimates due to high duplication factor */
			opn_highdups(sjp, ocop, icop, probes,
			    sjp->key_reptf/icop->opo_cost.opo_tpb, 
			    &blockstouched, &dio, &dio1estimated, &cpu);
		    else
			blockstouched += dio1estimated; /* add estimates for data pages */
		}
		else if ((!sjp->existence_only || sjp->kjeqcls_subset)
			&&
			(sjp->key_reptf > icop->opo_cost.opo_tpb))

		{   /* take into account highly duplicated BTREE secondary index
		    ** which has no data pages */
		    OPO_BLOCKS	    dummy;
		    dummy = blockstouched;
		    opn_highdups(sjp, ocop, icop, probes,
			sjp->key_reptf/icop->opo_cost.opo_tpb, 
			&blockstouched, &dio, &dummy, &cpu);/* adjust
					    ** estimates due to high duplication
					    ** factor */

		}

		dio += blockstouched;

		cache_usage += 1.0;	    /* one block of the cache will be used for
					    ** the data page, but the
					    ** same block will not be revisited */
	    }
	    else if (isorted)
	    {	/* the inner and outer are sorted for a full sort merge */
                rightfull = FALSE;          /* no difference between a right
                                            ** and left join or full join when
                                            ** processing */
		if (memsort)
		    ;			    /* no extra disk I/O for inner if
                                            ** it can be buffered */
		else
		{
		    if (sjp->inner.sort)
			dio += itotblk / iholdfactor ;
					    /* hold file on inner is read 
					    ** at least only once */
		    if (!istatp->mas_unique 
			&& 
			!ostatp->mas_unique
			&&
			!sjp->exist_bool)
		    {	/* Calculate cost of any rescans of hold file
			** - if the inner is unique then no rescans are required
			** assuming QEF buffers the last tuple read, and detects
                        ** no reseek is required - FIXME check this 
                        ** QEF assertion
                        ** - if the outer is unique then no rescan will be made
			** - if existence only check, based on the key with
			** no other boolean factors involved at this node, then
			** any inner tuples which matches an outer, will not pass the
			** first tuple in the run, so assuming DMF caching
			** no extra DIO is needed
			*/

			dio_cost = sjp->exist_factor * 
			    jstatp->mas_nunique * (ostatp->mas_reptf - 1.0) * /* number of tuples
					    ** in the outer which will cause
					    ** a rescan */
			   (istatp->mas_reptf * iwidth / (holdfactor * 
			      max_pagesize) );
					    /* a rescan will
					    ** cause a disk I/O if the TID seek 
					    ** in the hold file is past
					    ** DB_HFBLOCKSIZE boundary,... this
					    ** will occur on average every
                                            ** (DB_HFBLOCKSIZE /(ireptf*iwidth))
					    ** tuples */
			if (dio_cost > 0.0)
			    dio += dio_cost;
		    }
		}
		cpu += jstatp->mas_nunique * istatp->mas_reptf * ostatp->mas_reptf +
						/* COMPARISON cost - for each 
						** join value in the result
						** required one compare of an
						** outer and inner tuple */
		    (istatp->mas_nunique - jstatp->mas_nunique) * istatp->mas_reptf + /*
						** 1 COMPARISON for
						** each inner tuple which did
						** not participate in the join
						*/
		    (ostatp->mas_nunique - jstatp->mas_nunique) * ostatp->mas_reptf; /* 
						** 1 COMPARISON for 
						** each outer tuple which did
						** not participate in the join
						*/
	    }
	    else
	    {
		/*
		** The inner is neither isam nor hash nor sorted so for each 
                ** tuple of the outer which participates in the join we need 
                ** to read all blocks of the inner.  See previous comment 
                ** about existence_only searches.
		*/
		if (idiskrel)
		{
		    /*static*/opn_sj1(sjp->existence_only, 
			sjp->outer.tuples + 1.0,	    /* since this is a dangerous
						    ** access method, add 1
						    ** so that at least 2 tuples
						    ** are considered on outer
						    */
			ostatp->mas_nunique, 
			jstatp->mas_nunique, itotblk/* + 1.0 */,
			icop->opo_cost.opo_tpb, 
			&dio, &cpu);
		}
		/*
		** In the case where the inner is not disk resident, the first
		** pass reads from below and then writes the tuples to a hold 
		** file and subsequent passes read from the hold file.
		** See previous comment about existence_only searches.
		*/
		else
		{
		    /*static*/opn_sj1(sjp->existence_only, sjp->outer.tuples, 
			ostatp->mas_nunique, 
			jstatp->mas_nunique,
			itotblk / iholdfactor,
			icop->opo_cost.opo_tpb * iholdfactor, &dio, &cpu);
		    dio += itotblk /* + DB_HFOVERHEAD added at end of routine */ ;
		}
	    }
	}
	else	/* outer is not sorted */
	{
	    OPO_TUPLES         adfactor;	/* average length of run of
                                                ** contiguous tuples with
                                                ** the same value */
	    OPO_TUPLES         sortfactor;      /* average length of a run
                                                ** of rows in outer relation
                                                ** which are in order on the
                                                ** joining equivalence class
                                                */
            if ((ocop->opo_ordeqc == jordering)
                ||
                !keyed_join)                    /* if not a key join then
                                                ** jordering will only be set
                                                ** if the orderings are compatible
                                                ** in some subset of ocop->opo_ordeqc
                                                */
	    {
		/* if outer is sorted then csf will be large */
		sortfactor  = ocop->opo_cost.opo_sortfact;
		adfactor = ocop->opo_cost.opo_adfactor;
	    }
	    else
	    {   /* use defaults */
		sortfactor  = OPO_DSORTFACT;
		adfactor = OPO_DADFACTOR;
	    }
	    if (sortfactor > ocop->opo_cost.opo_tups)
		sortfactor = ocop->opo_cost.opo_tups;
	    if (adfactor > ocop->opo_cost.opo_tups)
		adfactor = ocop->opo_cost.opo_tups;
	    if (keyed_join)
	    {
		/*
		** The outer is not sorted, however it is possible that some 
                ** duplicate values are adjacent (this can happen when the 
                ** outer is hashed or isam for instance).  A set of adjacent 
                ** duplicate values defines a run.  For each run 
                ** (otuples/adfactor), we perform a directory search or 
                ** hash computation to find the blocks which must be checked.
                ** "jnunique/ohistp->opo_cost.opo_nunique" is the proportion 
                ** of unique values from the outer which participate in the 
                ** join.  So "jnunique/ohistp->opo_cost.opo_nunique * otuples
                ** /adfactor" is the number of runs which will be on values 
                ** in the join.  For each of the adfactor tuples in these we 
                ** read all of the blocks in the set of blocks of the inner to
		** be checked (in/ip).  For the rest of the runs we read this 
                ** set of blocks through only once (to realize that there are 
                ** no matches).  Total is:
		**     otuples/adfactor * dirsearchcost + 
		**     jnunique/ohistp->opo_cost.opo_nunique * 
                **       otuples/adfactor * in/ip * adfactor + 
                **     (1 - jnunique/ohistp->opo_cost.opo_nunique) * ot/
                **       adfactor * in/ip.
		** see comment above about existence_only searches
		*/
		OPO_BLOCKS	    nocache;    /* number of I/O's if no cache
						** is available */
	    	OPO_BLOCKS	    data1pages;
		OPO_BLOCKS          iprimblks1;	/* number of primary blocks
					** in inner relation */
		OPO_BLOCKS	    iblocks2; /* number of blocks scanned
					** for each new outer value */
		OPO_BLOCKS	    datapages;
		DMT_TBL_ENTRY	    *relation1p;

		relation1p =  varp->opv_grv->opv_relation->rdr_rel;
		data1pages = relation1p->tbl_dpage_count;
		iprimblks1 = icop->opo_cost.opo_cvar.opo_relprim;
		iblocks2 = (iprimblks1 + relation1p->tbl_opage_count)/ iprimblks1;	
					    /* average number of DIO
					    ** needed to read a overflow
					    ** chain */

		nocache = 0.0;
		iprimblks1 = icop->opo_cost.opo_cvar.opo_relprim;
#if 0
**              if (primblks > sjp->inner.kma_nunique)
**		    primblks = sjp->inner.kma_nunique; /* over estimate for
**					    ** sparse tables */
**		/*static*/opn_sj1(sjp->existence_only, adfactor, 1.0, 
**		    jstatp->mas_nunique / ostatp->mas_nunique,
**		    (itotblk /*+ 1.*/) / primblks * sjp->outer.tuples / adfactor, 
**		    icop->opo_cost.opo_tpb, &nocache, &cpu);
#endif
		/*static*/opn_sj1(sjp->existence_only, adfactor, 1.0, 
		    jstatp->mas_nunique / ostatp->mas_nunique,
		    iblocks2 * sjp->outer.tuples / adfactor, 
		    icop->opo_cost.opo_tpb, &nocache, &cpu);
		if (icop->opo_storage == DB_BTRE_STORE)
		    cpu = nocache;	    /* since BTREE data pages are not
					    ** scanned, the CPU used should be
					    ** the same as the DIO used */
		{   
		    OPO_BLOCKS		dircost1;
		    OPO_TUPLES		key1perdpage;
		    OPO_TUPLES		effective_tuples;

		    effective_tuples = sjp->outer.tuples / adfactor;
		    dircost1 = varp->opv_dircost;
		    key1perdpage = varp->opv_kpb;
		    if ((key1perdpage > 0.0) && (dircost1 > 1.0))
		    {	
			OPO_TUPLES  cell1_count;/* number of cells at a
						** particular directory level
						*/
			OPO_TUPLES  new1_count;	/* maximum number of tuples
						** that can be represented
						** at this level
						*/
			cell1_count = key1perdpage;
			blockstouched = 0.0;
			while (dircost1 > 1.0)
			{   /* ball and cell problem for caching hits , only
			    ** look at dircost1 > 1.0 as opposed to >0.0
                            ** since we assume the root page of the index
			    ** is always cached */
			    OPO_BLOCKS	 index_hit; /* number of pages at this
						** level of the index which will
						** be hit */
			    OPO_BLOCKS	dio3estimate;
			    new1_count = cell1_count * cell1_count;
			    if (ostatp->mas_nunique < new1_count)
				index_hit = opn_eprime ((OPO_TUPLES)
				    ostatp->mas_nunique, /* number of
						** balls to select */
				    (OPO_TUPLES)cell1_count,  /* number of balls
						** in each cell */
                                    (OPH_DOMAIN)cell1_count, /* number of cells */
                                    global->ops_cb->ops_server->opg_lnexp);
			    else
				index_hit = cell1_count; /* if there are more unique
						** values than tuples at this
						** level then all cells will
						** be hit at this level */
                            if (index_hit > icop->opo_cost.opo_tups)
                                index_hit = icop->opo_cost.opo_tups; /* cannot have
                                                ** more index page hits than tuples
                                                ** available, fixes problem with small
                                                ** btrees */
			    blockstouched += index_hit;
			    dircost1 = dircost1 - 1.0;
			    cell1_count = new1_count;
			    if (index_hit <= cache_available)
			    {	/* all pages are in the cache so no extra dio */
				cache_available -= index_hit;
				cache_usage += index_hit;
				dio3estimate = index_hit;
			    }
			    else
			    {	/* not all pages are in the cache so assume only
				** a fraction of pages hit */
				if (cache_available <= 0.0)
				    dio3estimate = effective_tuples;
				else
				    dio3estimate = effective_tuples * ( 1.0 - cache_available/index_hit);
				cache_available = 0.0;
				cache_usage = global->ops_estate.opn_maxcache[0];
			    }
			    if (dio3estimate > sjp->maxpagestouched)
				dio3estimate = sjp->maxpagestouched;
			    dio += dio3estimate;
			}
		    }
		    /* dio += effective_tuples * dircost1; */
		    else
			blockstouched = dircost1;
		}
		if (!(relation1p->tbl_id.db_tab_index > 0
		     &&
		     (relation1p->tbl_storage_type == DB_BTRE_STORE ||
		      relation1p->tbl_storage_type == DB_RTRE_STORE)))
		{
		    /* for BTREE secondary indexes there are no datapages */

		    OPO_BLOCKS	    dio4estimate;
		    dio4estimate = opn_eprime(ostatp->mas_nunique, 
			    kstatp->mas_nunique / data1pages,
                            data1pages,
                            global->ops_cb->ops_server->opg_lnexp)
			* iblocks2;
			/* itotblk / icop->opo_cost.opo_cvar.opo_relprim replaced by iblocks2*/ 
						/* FIXME - changed
						** jstatp to ostatp since it is probably
						** a better measure of the number of unique
						** values for probing (b8521), since jstatp
						** contains statistics after all selectivity
						** is applied, what is needed is getting the
						** info from oph_jselect prior to applying
						** the selectivity so that jstatp->mas_nunique
						** can contain the unique values as a result of
						** the join, and not after restrictions */
                    if (dio4estimate > icop->opo_cost.opo_tups)
                        dio4estimate = icop->opo_cost.opo_tups; /* cannot have more
                                                ** DIOs than tuples */
		    if (dio4estimate > sjp->maxpagestouched)
		    {
			if (nocache > dio4estimate)
			    nocache *= sjp->maxpagestouched/dio4estimate; /* since there
						** is some added clustering due to the
						** multiattribute values, adjust the nocache
						** estimate appropriately */
			dio4estimate = sjp->maxpagestouched;
		    }
		    if ((!sjp->existence_only || sjp->kjeqcls_subset)
			&&
			(kstatp->mas_reptf > icop->opo_cost.opo_tpb))
			/* adjust estimates due to high duplication factor */
			opn_highdups(sjp, ocop, icop, ocop->opo_cost.opo_tups,
			    kstatp->mas_reptf/icop->opo_cost.opo_tpb, 
			    &blockstouched, &nocache, &dio4estimate, &cpu);
		    else
			blockstouched += dio4estimate;
		    if (dio4estimate <= cache_available)
		    {   /* all blocks will be cached so assume no DIO */
			dio += dio4estimate;
			cache_usage += dio4estimate;
			cache_available -= dio4estimate;
		    }
		    else
		    {
			if (cache_available <= 0.0)
			    dio += nocache;
			else
			{
			    OPO_BLOCKS	extra;	/* number of extra I/O's
						    ** due to random accesses to
						    ** data pages, as opposed to
						    ** having everything cached */

			    extra = nocache - dio4estimate;
			    if (extra <= 0.0)
				dio += dio4estimate; /* this should never be
						    ** executed */
			    else
			    {
				dio += dio4estimate + 
				    extra * (1.0 - cache_available/dio4estimate);
						    /* this calculation is probably
						    ** too pessimistic, FIXME */
				cache_usage = global->ops_estate.opn_maxcache[0];
				cache_available = 0.0;
			    }
			}
		    }
		}
		else if ((!sjp->existence_only || sjp->kjeqcls_subset)
			 &&
			 (kstatp->mas_reptf > icop->opo_cost.opo_tpb))
		{   /* adjust for high duplication factor of btree secondary
		    ** index */
		    OPO_BLOCKS	dummy;
		    dummy = blockstouched;
		    opn_highdups(sjp, ocop, icop, ocop->opo_cost.opo_tups,
			kstatp->mas_reptf/icop->opo_cost.opo_tpb, 
			&blockstouched, &dio, &dummy, &cpu); /* adjust
					    ** estimates due to high duplication
					    ** factor */
		}
	    }
	    else if (isorted)
	    {
		if (memsort)
		{
		    cpu += sjp->outer.tuples / sortfactor  * 
			/*static*/ opn_acap(sortfactor , 1.0, sjp->inner.tuples,
			global->ops_cb->ops_server->opg_lnexp);
		}
		else
		{
		    /*
		    ** The outer is not sorted, however there will be runs of 
		    ** sorted tuples, the length of which we expect to 
		    ** average sortfactor.  There will therefore be otuples/
		    ** sortfactor runs.  For a given run we read sequentially 
                    ** through the inner until a value is found which is 
                    ** greater than the last value of the run.  Assuming the
		    ** values in the run are found at random locations in the 
                    ** inner, we expect 
                    **	 opn_acap(sortfactor ,icop->opo_cost.opo_tpb,in) blocks
                    ** of the inner to be read.  Add 1 to reltotb so it is 
                    ** not too cheap (inaccurate evaluation of restrictions 
                    ** causes the need for this).  Divide by holdfactor cause 
                    ** hold files pages are bigger.
		    */
		    OPO_BLOCKS	tempdio1;
		    OPO_BLOCKS  blockperrun;
		    OPO_TUPLES  tupleperblock;

		    tupleperblock = icop->opo_cost.opo_tpb * iholdfactor;/* this
					    ** was changed from just using opo_tpb
					    ** since a block is a hold file block
					    ** in this case */
		    blockperrun = /*static*/opn_acap (sortfactor ,tupleperblock,
			     (itotblk /*+ 1*/) / iholdfactor,
			     global->ops_cb->ops_server->opg_lnexp);
		    if (blockperrun < 1.0)
			blockperrun = 1.0;  /* need at least one DIO per run */
		    tempdio1 = sjp->outer.tuples / sortfactor  * blockperrun;
		    /*
		    ** For each run we will stop about half way through the 
                    ** last page, so subtract "otuples / sortfactor  / 2"
                    ** from the number of pages read; then multiply
		    ** by the number of rows on each page.
		    */
		    cpu += (tempdio1 - sjp->outer.tuples / (sortfactor * 2.0)) *
			    tupleperblock;
		    dio += tempdio1 /* + DB_HFOVERHEAD added at end of routine */ ;
		}
	    }
	    else	/* inner is neither sorted, isam nor hash */
	    {
		/*
		** Otherwise for each tuple of the outer we must read all 
                ** blocks of the inner.
		** see comment above about existence_only searches
		*/
		if (idiskrel)
		{
		    /*static*/opn_sj1(sjp->existence_only, adfactor, 1.0, jstatp->mas_nunique /
			ostatp->mas_nunique, (itotblk /*+ 1*/) * sjp->outer.tuples / adfactor,
			icop->opo_cost.opo_tpb, &dio, &cpu);
		    /* when the outer degenerates to 1 tuple, then a cart prod results which is
		    ** dangerous, especially if the outer tuple estimate is incorrect, it is better
		    ** to bias the query to a full sort merge when the number of tuples in the outer
		    ** is 1.0 or less, e.g. try joining two empty tables together */
		    if ((sjp->outer.tuples <= 1.0)
			&&
			(jordering != OPE_NOEQCLS))
			cpu += icop->opo_cost.opo_tups;	/* this baises to a FSM just barely */
		}
		else
		{
		    /*static*/opn_sj1(sjp->existence_only, adfactor, 1.0, 
			jstatp->mas_nunique / ostatp->mas_nunique, 
			(itotblk / iholdfactor) * sjp->outer.tuples / adfactor, 
			icop->opo_cost.opo_tpb * iholdfactor,
			&dio, &cpu);
		    dio += itotblk /* + DB_HFOVERHEAD added at end of routine */ ;
		}
	    }
	}
    }
#ifdef    E_OP048B_COST
    if (dio < 0.0 || cpu < 0.0)
	    /* sjcost: bad dio:%.2f or bad cpu:%.2f\n */
	opx_error(E_OP048B_COST);
#endif

    dio += ocop->opo_cost.opo_dio + icop->opo_cost.opo_dio; /* add in the 
                                        ** costs to form the
					** outer and inner rels */
    cpu += ocop->opo_cost.opo_cpu + icop->opo_cost.opo_cpu + sjp->outer.tuples;
					/* all strategies require reading
                                        ** the outer relation once */
    /* NOTE - a project restrict, or join node on the outer does not
    ** cause any disk I/O for temps, since a hold file is never
    ** created, but SORTs will always cause a hold file so this
    ** case needs to be considered */
    if (sjp->outer.sort)
    {	/* add in costs of sorting the outer */
	OPO_BLOCKS	exdio;
	OPO_CPU		excpu;
	
	if (ocop->opo_existence)
	{
	    /* Sort estimates in eqsp do NOT assume existence only join and
	    ** can be (typically are) much bigger than they should be. Recompute
	    ** disk/cpu estimates to reflect existence_only result size. */
	    opn_reformat(subquery, sjp->outer.tuples, owidth,
				&exdio, &excpu);
	}
	else
	{
	    exdio = sjp->outer.eqsp->opn_dio;
	    excpu = sjp->outer.eqsp->opn_cpu;
	}

	if ( ((owidth + DB_SROVERHEAD) * sjp->outer.tuples)
	        >
		global->ops_cb->ops_alter.ops_qsmemsize
	      )				/* TRUE - if outer's tuples does not
					** fit into sorter's memory buffer */
	{
	    OPO_BLOCKS		seq2_blocks; /* number of sequential block I/Os
					** for this scan, typically sequential
					** block I/O size is larger than random I/O
					*/
	    seq2_blocks = ocop->opo_cost.opo_reltotb / oholdfactor;
	    if (seq2_blocks < 1.0)
		seq2_blocks = 1.0;	/* at least 1 I/O */
	    dio += exdio		/* DIO for sorting outer */
	        + seq2_blocks;		/* add in
					** the cost of reading the outer
					** sort file which is 
					** placed on disk */
	    cpu += excpu;		/* CPU for sorting outer */
	}
	else if ((icop->opo_sjpr == DB_ORIG)
		||
		opn_sortcpu(subquery, sjp->outer.tuples, jordering, ocop))
	    cpu += excpu;		/* CPU for sorting outer
					** make it cheap if it can be used
					** for the join and there are only
					** a few tuples */
	/* FIXME - if the underlying structure is ISAM then cannot
        ** assume it is sorted, but nearly sorted, so assume
        ** a linear scan for sort cost, since quicksort is not
        ** used but a "snowplow" algorithm is used */
	/* note that cost of reading outer does not exist in the data flow
	** architecture of the DBMS, the outer tuples are read once but
	** this is included in the cost of producing the outer node except
	** for outer sort nodes */
    }
    if (sjp->inner.sort)
    {	/* add in costs of sorting the inner */
	OPO_BLOCKS	exdio;
	OPO_CPU		excpu;
	
	if (icop->opo_existence)
	{
	    /* Sort estimates in eqsp do NOT assume existence only join and
	    ** can be (typically are) much bigger than they should be. Recompute
	    ** disk/cpu estimates to reflect existence_only result size. */
	    opn_reformat(subquery, sjp->inner.tuples, owidth,
				&exdio, &excpu);
	}
	else
	{
	    exdio = sjp->inner.eqsp->opn_dio;
	    excpu = sjp->inner.eqsp->opn_cpu;
	}

	if (!memsort)
	    dio += exdio;
	if (opn_sortcpu(subquery, sjp->inner.tuples, jordering, icop))
	    cpu += (excpu-3.0);	/* CPU for sorting inner
					** make it cheap if it can be used
					** for the join and there are only
					** a few tuples, otherwise a cart
					** prod could be favored, reduce cost
					** by 3 CPU units */
    }
    else if (icop->opo_sjpr != DB_ORIG)
    {
	/* FIXME - on a BTREE sorted inner tuple stream for an FSM, the
	** project restrict is not entirely created prior to the FSM but
	** instead memory is reused as the outer tuple changes, so that
	** disk based hold file may not be needed even though the size of the
	** project restrict is larger than the in memory hold file can support */
	if (!memsort)
	{   /* hold files are always written to disk until version 6.4
	    ** - calculate the cost of a hold file for the inner here, since
	    ** a sub-tree can be used as an inner or outer
	    ** - A hold file is not used if a sort is done since the sort
	    ** file is then used 
	    ** - only a physical join, and key or tid joins do not use hold
	    ** files
	    ** - FSM join with inner not a sort node would use a hold file
	    ** equal to the repetition factor of the joining attributes */
	    /* FIXME - in 6.5 hold files will be buffered in memory so change this */
	    OPO_BLOCKS		seq3_blocks; 

	    seq3_blocks = icop->opo_cost.opo_reltotb;
	    if (seq3_blocks < 1.0)
		seq3_blocks = 1.0;	/* at least 1 I/O */
	    if (!istatp->mas_unique || !osorted)
	    {	/* anytime the tuples for a particular join key, exceeds the
		** available QEF memory for hold files it will spill to disk
		** but if the next join key is not as highly duplicated then
		** it will fit into memory any not require a write to disk */
		OPO_TUPLES  isize;

		isize = istatp->mas_reptf * (iwidth + DB_SROVERHEAD);
		if (osorted && (isize <= global->ops_cb->ops_alter.ops_qsmemsize))
		    seq3_blocks  *= isize/global->ops_cb->ops_alter.ops_qsmemsize;
				    /* use a sliding factor
				    ** to model average consecutive string of join
				    ** keys on the inner, except in the case of
				    ** a PSM join */
		dio += DB_HFOVERHEAD	/* hold file overhead */
		    + seq3_blocks;	/* add in the cost of writing blocks
					** of hold file which is 
					** placed on disk */

	    }
	}
	cpu += icop->opo_cost.opo_tups;	/* CPU for writing inner or moving
					** to in-memory hold file
					*/
    }
    if (rightfull)
    {
        {
            /* create a TID hold file, which should contain a
            ** TID for each tuple from the inner which participated
            ** in the join */
            OPO_CPU             cpu1;
            OPO_BLOCKS          dio1;

            opn_reformat(subquery, sjp->inner.tuples, sizeof(DB_TID), &dio1,
                &cpu1);                 /* determine the cost of sorting the
                                            ** TIDs at this node */
            cpu += cpu1;
            if (
#if 0
                  keyed_join        /* for the key join case, the
                                    ** TIDs may not be sorted */
                  &&
#endif
                (   ((sizeof(DB_TID) + DB_SROVERHEAD) * sjp->inner.tuples)
                    >
                    global->ops_cb->ops_alter.ops_qsmemsize
                ))                          /* TRUE - if TID tuples do not
                                            ** fit into sorter's memory buffer */
            {   /* add DIO cost of sorting TIDs */
                OPO_BLOCKS              tidfilesize;
                OPO_BLOCKS              readtid;

                tidfilesize = sjp->inner.tuples * sizeof(DB_TID)
                    / max_pagesize;  /* add cost to create hold file */
                if (tidfilesize < 1.0)
                    tidfilesize = 1.0;
                readtid = tidfilesize/holdfactor;
                if (readtid < 1.0)
                    readtid = 1.0;
                dio += DB_HFOVERHEAD + tidfilesize + readtid + dio1;
            }
        }
        {   /* an extra scan of the inner is made to compare the TID
            ** for the tuples needed for the right join */
            OPO_BLOCKS          seq4_blocks;

            seq4_blocks = icop->opo_cost.opo_reltotb /iholdfactor;
            if (seq4_blocks < 1.0)
                seq4_blocks = 1.0;      /* at least 1 I/O */
            dio += seq4_blocks; /* add cost of scanning inner */
            cpu += icop->opo_cost.opo_tups;     /* CPU for comparing TIDs */
        }
    }
    sjp->jcache = cache_usage;
    sjp->dio = dio;
    sjp->cpu = cpu;
    sjp->pagestouched = blockstouched;

    /* See if we're to factor down the Kjoins. */
    if (keyed_join)
    {
	i4	first = 0, second = 0;

	if (global->ops_cb->ops_check &&
	    opt_svtrace( global->ops_cb, OPT_F120_SORTCPU, &first, &second) &&
	    first > 1)
	{
	    sjp->dio /= first;
	    sjp->cpu /= first;
	}
    }

    /* The following change makes sense - to bias away from Kjoins
    ** compunding on other joins because of the accumulated estimation
    ** error - but predictably, caused other bad query plans. It is
    ** being disabled, but left here as a reminder to possibly try
    ** something else similar in the future. */
    if (FALSE && keyed_join && (ocop->opo_sjpr == DB_SJ ||
	ocop->opo_outer && ocop->opo_outer->opo_sjpr == DB_SJ))
    {
	/* Bulk up key join estimates because of potential inaccuracy
	** in "join of join" estimates. Currently fixed at 15%, but 
	** could be parameterized later on. */
	sjp->dio *= 1.15;
	sjp->cpu *= 1.15;
	sjp->pagestouched *= 1.15;
    }

}

/*{
** Name: opn_tdeferred	- hueristics to see if temporary is needed
**
** Description:
**      Traverse CO tree to see if temporary is needed to support semantics
**      of DEFERRED update.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to current cop node being analyzed
**      nonindex                        ptr to boolean to indicate a non
**                                      index leaf has been found in the
**                                      subtree
**
** Outputs:
**      tempptr                         ptr to boolean which is TRUE if a
**                                      temporary relation is needed
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-oct-86 (seputis)
**          initial creation
**      16-nov-89 (seputis)
**          modify to check for updateable indexes on secondaries
[@history_line@]...
[@history_template@]...
*/
static bool
opn_tdeferred(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPV_IVARS          basevar,
	bool               *nonindex,
	bool		   *basefound,
	bool               *tempptr)
{
    bool		insubtree;      /* TRUE - if this subtree contains the
                                        ** base relation being considered */
    bool                inner_base;     /* TRUE - if inner subtree is exactly
                                        ** the base relation being considered*/
    bool                inner_duptids;  /* TRUE - if inner subtree contains only
                                        ** indices on the base relation being
                                        ** considered */
    bool                outer_duptids;  /* TRUE - if outer subtree contains only
                                        ** indices on the base relation being
                                        ** considered */
    insubtree = FALSE;
    inner_base = FALSE;
    if (cop->opo_outer)
    {
	(VOID) opn_tdeferred(subquery, cop->opo_outer, basevar, 
		&outer_duptids, &insubtree, tempptr);
	if (cop->opo_osort)
	    *tempptr = FALSE;		    /* a temp relation is no longer
					    ** needed since a sort node will
					    ** be created */	    
    }
    else
	outer_duptids = TRUE;		    /* degenerate case of subtree
                                            ** being NULL, and therefore
                                            ** can be considered to contain
                                            ** only indexes */
    if (cop->opo_inner)
    {
	inner_base = opn_tdeferred(subquery, cop->opo_inner, basevar, 
		&inner_duptids, &insubtree, tempptr);
	if (cop->opo_isort)
	    *tempptr = FALSE;		    /* a temp relation is no longer
					    ** needed since a sort node will
					    ** be created */
    }
    else
	inner_duptids = TRUE;		    /* degenerate case of subtree
                                            ** being NULL, and therefore
                                            ** can be considered to contain
                                            ** only indexes */
    if (inner_base && !outer_duptids	    /* if inner is the base relation
                                            ** then outer must contain only
                                            ** indexes, which would imply
                                            ** the inner is probed only once 
                                            ** per tuple */
       )
        *tempptr = TRUE;		    /* TRUE- means temporary is needed
                                            ** - this hueristics checks if the
                                            ** smallest subtree containing the
                                            ** the base relation, contains only
                                            ** indexes on the base relation then
                                            ** no temporary rel is needed */
    switch (cop->opo_sjpr)
    {

    case DB_ORIG:
    {
	/* leaf node has been found */
	if (*basefound = (basevar == cop->opo_union.opo_orig))
	{
	    *nonindex = TRUE;               /* this branch does not contain
                                            ** only indexes on the base relation
                                            */
	    return (TRUE);                  /* if this is the base relation
                                            ** being considered then indicate
                                            ** this to the parent */
	}
	*nonindex = FALSE;
	break;
    }
    case DB_RSORT:
    case DB_RISAM:
    case DB_RHASH:
    case DB_RBTREE:
    if (insubtree && (cop->opo_sjpr == DB_RSORT))
	*tempptr = FALSE;		    /* this hueristic checks that
                                            ** since there is a temp created
                                            ** above the relation to be
                                            ** updated, there is no need to
                                            ** add another temporary relation*/
	/* fall through */
    case DB_PR:
    case DB_SJ:
	*basefound = insubtree;		    /* TRUE if subtree contains the
                                            ** base relation */
	*nonindex = inner_duptids && outer_duptids; /* TRUE - if subtree
					    ** contains only indexes, on the
                                            ** base relation, so that TIDS
                                            ** are unique per tuple */
	break;
#ifdef E_OP048E_COTYPE
    default:
	opx_error(E_OP048E_COTYPE);
#endif
    }
    return( FALSE );			    /* FALSE - base relation is not at
                                            ** this node */
}

/*{
** Name: opn_deferred	- adjust cost if a sort node is required
**
** Description:
**      This routine will decide whether a sort node will be added to the
**      query plan, in order to solve a problem with deferred updates.
**
**      Replace and Delete Issues for the optimizer in Jupiter
**
**      The special case handling of REPLACE and DELETE query plans will be described
**      in the following section.  Some examples illustrating the problem and
**      the solution will follow.
**
**      DMF (Data Manipulation Facility - the access paths) allows a table
**      to be opened in DEFERRED and DIRECT mode.  The set statements are always opened
**      for DEFERRED and cursors can use either mode.
**
**           In DEFERRED mode, once a tuple is updated it is marked.  This enables DMF
**      to a) detect multiple non-functional updates, and b) to prevent the halloween
**      problem.  An important point for the optimizer is that once an update has been
**      made on a tuple, any further attempt to access that tuple (by that cursor)
**      will cause a DMF error.  (However, other cursors will be able to see the new
**      value of that tuple.)  In other words, the optimizer is required to 
**      choose a query plan in which all retrieves to a tuple must preceed the update 
**      of that tuple by that cursor.  This becomes complicated when multiple range 
**      variables are used and refer to the same relation in the same query.  This 
**      technique has the advantage that simple update queries
**      will execute faster.  It has the disadvantage that it complicates the
**      optimizer.  The optimizer will create a temporary relation of updates in the
**      rare cases where it is necessary to do so.
**
**           In DIRECT mode, DMF does not mark the tuple and as a result,
**      new values are visible immediately to the user.  The problems of multiple 
**      non-functional updates, and the halloween problem exist.  Multiple 
**      non-functional updates are avoided by not allowing
**      multi-variable cursor statements to be parsed.  The Halloween problem
**      is partially solved by the optimizer by avoiding the use of secondary
**      indices on updatable attributes within the query plan.  The problem may still 
**      exist if the user attempts to update the primary index (It is strongly 
**      suggested that DEFERRED mode be used in this case.)
**
**      Examples:
**
**      a) Halloween Problem - "Give everyone who makes more that 20,000 a 10% raise."
**          suppose that an index existed on the salary attribute, the query would be
**
**             range of e is employee
**             replace e ( e.sal = e.sal * 1.1 ) where e.sal > 20000.00
**
**          Suppose an index on salary was used, every time the salary was updated it
**          would be moved ahead of itself in the index and "reappear".  The query
**          would execute forever unless something was done to prevent this.
**          DMF will handle this case in DEFERRED mode by marking each tuple which
**          is updated, and not allowing the tuple to be scanned again.
**          In the DIRECT case, the optimizer will not use salary index, so that the
**          tuple will not reappear.
**
**      b) Halloween Problem with multiple range variables on same relation - 
**          "Give everyone who makes more than his manager and 
**           make more than 20000, a 10% raise."
**
**             range of e is employee
**             range of m is employee
**             replace e (e.sal = e.sal*1.1) where e.sal>20000. AND e.manager=m.empno
**                                         AND e.sal > m.sal
**
**          In the DEFERRED case:
**          In this case, DMF would mark the "e" tuple as being updated, but if the
**          "m" variable attempted to access this tuple once again, the DMF would
**          report an error.  This is because all retrieves on a tuple need to be
**          completed prior to the update, in DEFERRED mode.  The optimizer will 
**          compensate for this by creating a plan to ensure all retrieves will
**          preceed the update.
**
**          In the DIRECT (cursor) case:
**          This multi-variable query will not be allowed for cursors in 
**          DIRECT update mode.
**
**      c) multiple non-functional updates - "Give every manager who makes less than
**          an employee he manages, the salary of that employee"
**
**             replace m (m.sal = e.sal) where e.manager=m.empno AND e.sal > m.sal
**
**          If there was a manager who had 2 employees which made more, then this
**          query would generate an error, since it is not clear which salary should
**          be given to the manager i.e. there is an ambiguous multi to one update.
**
**          In the DEFERRED case: the DMF would mark a tuple as being updated, so a
**          second attempt to update the tuple would cause an error.
**
**          In the DIRECT (cursor) case:
**          This multi-variable query will not be allowed for cursors in 
**          DIRECT update mode.
**
**      d)  The interaction between multiple FOR UPDATE cursors operating on
**          a single table concurrently is a "user beware" situation.  For any
**          given table only one cursor can be opened for DEFERRED and the remainder 
**          must be opened for DIRECT.  Any tuple updated by the DEFERRED cursor is 
**          still visible to all the DIRECT cursors.  Any tuple updated by the DIRECT 
**          cursor could potentially be seen by the DEFERRED cursor once again.
**          The optimizer (or DMF) will not solve any of these interactions.
**
**      OPTIMIZER DETAILED DESIGN:
**
**          In DEFERRED mode (includes set commands):
**          The general solution is to retrieve into a temporary relation, the
**          TIDs to update, along with any values required for the update.  This
**          would ensure that all retrieves would be performed ahead of all updates.
**          The optimizer will detect the following situations, in which the
**          temporary relation will not be required.
**             If the smallest subtree which contains the base relation to be updated
**          contains only indices on that base relation then a temporary relation is
**          not required.  This hueristic ensures that no join exists which produces
**          duplicate TIDs which could possibly cause an update, followed by a possible
**          retrieve on the same TID.
**		- FIXME - in deferred case, the halloween problem still exists for
**	    secondary indexes, so create a temp in cases in which a secondary index
**	    is used which has an updatable column
**             If the path from the target base relation to the root of the query tree
**          contains a SORT node, then a temporary relation is not required.  This
**          hueristic ensures that if duplicate TIDs do exist, then a SORT node would
**          force all retrieves to be made prior to the first update.
**             The hueristics need to be checked for each relation with the same
**          table ID as the relation to be updated
**          
**          In DIRECT mode:
**      	 Any secondary index which contains attributes in the FOR UPDATE list 
**          will not be considered for any query plan.  This implies that the 
**          halloween problem may still occur if the primary index is being updated.
**
** Inputs:
**      subquery                        subquery being analyzed
**      ocop                            ptr to outer cop of plan
**      icop                            ptr to inner cop of plan
**
** Outputs:
**	Returns:
**	    TRUE - if plan is too expensive with new sort node
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      21-nov-86 (seputis)
**          initial creation
**	14-apr-94 (ed)
**	    bug 59679 - a delete query was a self join which caused tuples
**	    to be deleted which selected other tuples in the same relation
**	    causing fewer tuples than expected to be deleted, so if a delete
**	    exists which references the target variable in the query then
**	    place a top sort node as a hold file prior to deleting any
**	    tuples
[@history_template@]...
*/
static bool
opn_deferred(
	OPS_SUBQUERY       *subquery,
	OPO_COST           *jcost,
	OPO_CO             *ocop,
	OPO_CO             *icop,
	OPO_BLOCKS         blocks,
	OPO_TUPLES         tuples,
	OPS_WIDTH          width,
	bool               *sortrequired)
{
    bool                nonindex;       /* not used - only needed during
                                        ** recursive descent of tree */
    bool                basefound;      /* not used - only needed during
                                        ** recursive descent of tree */
    OPO_CO              tcop;           /* setup temporary CO node to made
                                        ** recursive descent easier */
    OPV_IVARS           basevar;        /* current varno of base relation being
                                        ** analyzed */
    OPV_BMVARS          (*updateable);  /* bit map of base relations which may
                                        ** be updated */
    OPV_IVARS           maxvar;         /* maximum number of range variables
                                        ** defined */
    bool                multi_var;      /* TRUE - if multiple range variables
                                        ** on the same table */
    nonindex = FALSE;
    basefound = FALSE;
    tcop.opo_inner = icop;
    tcop.opo_outer = ocop;
    tcop.opo_sjpr = DB_SJ;              /* just as long as it is not a
                                        ** DB_ORIG node */
    updateable = subquery->ops_global->ops_estate.opn_halloween; /* get
                                        ** ptr to bit array of base relations
                                        ** to be updated */
    maxvar = subquery->ops_vars.opv_rv; /* number of range variables defined */
    multi_var = BTcount((char *)updateable, (i4)maxvar) > 1;
    if ((subquery->ops_mode == PSQ_DELETE) /* updateable array will only be
					** set for a delete query if it is
					** a self join, bug 59679 */
	||
	(subquery->ops_vars.opv_prv > 1))
    {	/* FIXME - hack for DMF since it does not allow multiple updates
	** of the same values so a sort node is required in all cases
	** in which more than one range variable is required */
	*sortrequired = TRUE;
	return(FALSE);
    }
    for (basevar = OPV_NOVAR; 
	(basevar = BTnext((i4)basevar, (char *)updateable, (i4)maxvar))>=0;)
    {
	bool                temp;	/* TRUE - if a temporary needs
                                        ** to be created at the top of the
                                        ** tree */
	temp = multi_var;		/* temp needs to be created unless
                                        ** every path to top of query plan 
                                        ** from a updateable relation contains
                                        ** a sort node */
	(VOID)opn_tdeferred(subquery, &tcop, basevar, 
	    &nonindex, &basefound, &temp); /* traverse the CO tree */
        if (temp)
	    break;                      /* exit if temporary is required */
    }
    if (basevar < 0)
    {
        *sortrequired = FALSE;		/* indicates that no sort node is
                                        ** required */
	return(FALSE);                  /* if no temp is needed then the
                                        ** query plan can be used directly */
    }
    {
    /* at this point it has been determined that a temporary relation is needed
    ** so compute the cost of a SORT node and see if this affects the selection
    ** of this query plan */
	OPO_BLOCKS	    dio;
	OPO_CPU             cpu;
        OPO_COST            sortcost;

	opn_reformat( subquery, tuples, width, &dio, &cpu);
	/* FIXME - need to add cost of reading tuples if temporary relation
        ** is large enough to be written to disk */
	sortcost = opn_cost(subquery, dio, cpu);
	if ((*jcost + sortcost) > subquery->ops_cost)
	    return(TRUE);		    /* do not consider if it costs
                                            ** more than the current best plan
                                            */
	*jcost += sortcost;                 /* add sortcost for comparison
                                            ** purposes */
        *sortrequired = TRUE;		    /* indicates a sort node on TIDs
                                            ** is required on top of the query
                                            ** plan */
    }
    return (FALSE);
}

/*{
** Name: opn_modcost	- possibly update tuple estimate for subquery
**	to reflect results of aggregate subquery
**
** Description:
**	Use subquery type to improve result tuple estimates of
**	current subquery result.  The subquery bestco and besthisto
**	are used to update the size estimates in the subquery
**	global range variable (the one numbered by ops_gentry).
**	We do not update the bestco itself, as it should continue
**	to reflect the CO-tree leading up to the aggregation.
**
**	(If aggregation were just another node, this foolishness
**	would perhaps be not needed, but enough whinging.)
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**
** Outputs:
**	Tuple estimates altered in opv_gcost for subquery global range var.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-july-05 (inkdo01)
**	    Written to provide accurate tuple estimates for aggregate q's.
**	31-Aug-2005 (schka24)
**	    Don't apply divisor of 100 to tids or unknowns.
**	    Adjust result block count properly.
**	30-Jun-2006 (kschendel)
**	    Alter call sequence to always get histp from subquery besthisto.
**	    Update opo-parm in subquery global range var instead of updating
**	    the subquery bestco;  the latter updates the INPUT to the agg.
**	21-nov-2006 (dougi)
**	    Set OPS_AGGCOST flag so opn_modcost() can be called before we
**	    free the enumeration memory.
**      6-mar-2008 (bolke01) Bug 120059
**          Under rare conditions queries have been found to fail creating a full
**          eqc structure when the OPS_RFAGG subquery processing is required.
**          This fix protects from a possible derefernce of a null pointer.
**          In the research of the problem it was noticed that the summing of 
**          individual values may go through many iterations and accumulate a
**          an oversized groupresult value.  This has been trimmed at 1e10 which
**          also speeds up query plan generation without affectin ( test queries)
**          QEP. 
**          Note: Using the E_OP04A0_FLOAT_OVERFLOW in this context only to force out 
**          the query does not change the program flow. The opf error handler knows
**          to return control to the caller, and in combination with trace point
**          sc924 (set trace point sc924 263360) SQL can be generated.
**          TRACE POINT OP181 has been used to get a fast QEP generated
**          and output to the errlog, the query and get more than a warning that 
**          the modcost value for groupcount has been capped after a number 
**          of iterations.
**     7-mar-2008 (bolke01) Bug 120059
**          Correction to the output of a message to the II_DBMS_LOG - too many
**          were output when the ghistp is NULL.
**	24-Mar-2009 (kibro01) b121845
**	    Remove one of the messages from the ghistp check since it can quite
**	    legitimately be null.
*/
/* Define a limiting value for the accumulator groupresult */
#define OPF_GROUPRES_LIMIT 1e10
/* Define a limiting value for reporting the ghist NULL */
#define OPF_GROUPRES_ITERATIONS 10
VOID
opn_modcost(OPS_SUBQUERY *subquery)

{
    OPZ_AT	*abase;
    OPE_ET	*ebase;
    OPS_SQTYPE	sqtype;
    PST_QNODE	*byresp, *byvarp;
    OPO_TUPLES	groupresult;
    OPH_HISTOGRAM	*histp;
    OPH_HISTOGRAM	**ghistp;
    OPE_IEQCLS	eqcls;
    OPO_PARM	*gcostp;
    bool	found;
    i4          zcnt = 0;
    if (subquery->ops_gentry == OPV_NOGVAR)
	return;				/* The common case, actually */

    abase = subquery->ops_attrs.opz_base;
    ebase = subquery->ops_eclass.ope_base;
    sqtype = subquery->ops_sqtype;
    groupresult = 1.0;
    found = FALSE;
    gcostp = &subquery->ops_global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry]->opv_gcost;
    histp = NULL;			/* In case no histogram (expr?) */
    if (subquery->ops_besthisto != NULL)
	histp = subquery->ops_besthisto;

    /* Tuple estimate is modified according to subquery type - some
    ** are ignored, but aggregates are altered. */

    switch (sqtype) {

      case OPS_SAGG:
      case OPS_RSAGG:
	/* Simple aggregates have only one result row.  (Indeed, they
	** probably don't get here at all, as they aren't likely to have
	** an ops-gentry result variable.)
	*/
	gcostp->opo_pagesize = DB_MINPAGESIZE;
	gcostp->opo_tups = 1.0;
	gcostp->opo_reltotb = 1.0;
	subquery->ops_mask2 |= OPS_AGGCOST;
	break;

      case OPS_FAGG:
      case OPS_RFAGG:
      case OPS_HFAGG: /* for completeness - they don't appear yet */
      case OPS_PROJECTION: /* ? */
	/* Loop over ops_byexpr accumulating unique value counts. */
	if (subquery->ops_byexpr == (PST_QNODE *) NULL)
	    break;

	/* Init. result costs first. */
	if (subquery->ops_bestco)
	    STRUCT_ASSIGN_MACRO(subquery->ops_bestco->opo_cost, *gcostp);
	else MEfill(sizeof(OPO_PARM), 0, (PTR) gcostp);

	for (zcnt = 0, byresp = subquery->ops_byexpr; byresp && 
	    byresp->pst_sym.pst_type == PST_RESDOM; zcnt++, byresp = byresp->pst_left)
	{
	    byvarp = byresp->pst_right;
	    ghistp = (OPH_HISTOGRAM **) NULL;
	    if (histp && byvarp && byvarp->pst_sym.pst_type == PST_VAR)
	    {
		eqcls = abase->opz_attnums[byvarp->pst_sym.pst_value.
			pst_s_var.pst_atno.db_att_id]->opz_equcls;
		if (ebase->ope_eqclist[eqcls]->ope_eqctype != OPE_TID)
		    ghistp = opn_eqtamp(subquery, &histp, eqcls, FALSE);
	    }
	    if (!ghistp || (groupresult  > OPF_GROUPRES_LIMIT) )
	    {
                /*
		** Potential FIXME: why does ghistp get returned as zeop.
		** The use of the 1e10 trim for the groupresult value has been shown
		** to have no effect on the QEPs created for teh test queries.
		*/ 
		/* Note to above: ghistp can be NULL legitimately simply because
		** the items in question in the group by list of the aggregate query
		** don't have histogram information available in the subquery, so if
		** we find any of the group items like that, simply break out and use
		** the default algorithm (kibro01) b121845.
		*/
		if (!ghistp && zcnt > OPF_GROUPRES_ITERATIONS)
		{
		    TRdisplay ("opn_modcost:In %d iterations GroupResult reached %38.0f with ghist = NULL\n", zcnt, groupresult);
		}
		else
		{
		    /* Ensure we don't use a partial (1 column out of many) result */
		    found = FALSE;
		}
		/* 
		** If the user has specified SET TRACE POINT OP181 then give more information to 
		** the II_DBMS_LOG.  In conjuction with SC924 the current query will also be
		** reported.
		*/
		if (subquery->ops_global->ops_cb->ops_check 
		   && opt_strace( subquery->ops_global->ops_cb,OPT_F053_FASTOPT))
		     opx_rerror(subquery->ops_global->ops_caller_cb, E_OP04A0_FLOAT_OVERFLOW);

		break;
	    }
	    /*
	    ** The size of groupresult can rise sharply when there are many items 
	    ** in the group list. (testcase has 18) and histogram cells exist for 
	    ** these (or most).
	    **
	    **   E.G.  using 1e21 ( in place of the 1e10 that is defined in this.
	    **
	    ** !opn_modcost:0:oph_nunique =          3437.000 >> groupresult =    3437
	    ** !opn_modcost:1:oph_nunique =           100.000 >> groupresult =  343700
	    ** !opn_modcost:2:oph_nunique =           171.000 >> groupresult =  587727e+002
	    ** !opn_modcost:3:oph_nunique =        167714.453 >> groupresult =  985703e+007
	    ** !opn_modcost:4:oph_nunique =        215068.000 >> groupresult =  211993e+013
	    ** !opn_modcost:5:oph_nunique =           173.000 >> groupresult =  366748e+015
	    ** !opn_modcost:6:oph_nunique =        215068.000 >> groupresult =  788758e+020
	    ** !opn_modcost:In 7 iterations GroupResult reached           78875815018848200000000000
            ** 
	    **   E.G.  using 1e10 as is defined in this module.
	    **
	    ** !opn_modcost:0:oph_nunique =          3437.000 >> groupresult =    3437
	    ** !opn_modcost:1:oph_nunique =           100.000 >> groupresult =  343700
	    ** !opn_modcost:2:oph_nunique =           171.000 >> groupresult =  587727e+002
	    ** !opn_modcost:3:oph_nunique =        167714.453 >> groupresult =  985703e+007
	    ** !opn_modcost:In 4 iterations GroupResult reached                          985701733248
	    **
	    ** The QEP generated by both runs was identical. A saving of 3 iterations.
	    */
	    
	    if (ghistp && (groupresult  <= OPF_GROUPRES_LIMIT) && (*ghistp)->oph_prodarea != OPH_UNIFORM)
	    {
		/* FIXME This is simplistic, although reasonably effective.
		** It assumes that bylist columns are independent.
		** Where they aren't, this will over-estimate the result.
		** (better than under-estimating, though.)
		** One case that might be worth checking for is a) some
		** att in the eqc belongs to a table with a unique key,
		** and b) those key column(s) are all in the bylist.
		** Under those conditions this column should not contribute.
		** Consider an orders, customers query:
		** "return order value by customer and date, and I'd like
		**  the customer phone"
		** select o.order_date, c.id, c.phone, sum(o.order_value)
		**     from orders o join customers c on o.cust_no = c.id
		**     group by o.order_date, c.id, c.phone
		** "phone" contributes nothing to the grouping in this case.
		*/
		if (groupresult  < OPF_GROUPRES_LIMIT)
		   groupresult *= (*ghistp)->oph_nunique;
#ifdef xDEBUG
                TRdisplay("opn_modcost:%d:oph_nunique = %17f >> groupresult = %12.0g\n",
		    zcnt, (*ghistp)->oph_nunique, groupresult);
#endif
		found = TRUE;
	    }
	}

	/* If no relevant histograms found, assume 1 group for every 10
	** rows -- as valid a wild guess as anything, and too large is
	** better than too small (especially for hash aggs).
	** Prevent the output estimate being more than 1/2 the input.
	** Then apply the same shrinkage factor to the block count.
	*/
	gcostp->opo_pagesize = DB_MINPAGESIZE;
	if (!found)
	    groupresult = 0.1 * gcostp->opo_tups;
	else if (groupresult > 0.5 * gcostp->opo_tups)
	    groupresult = 0.5 * gcostp->opo_tups;
					/* don't let it be bigger */
	if (groupresult < 1 && gcostp->opo_tups > 0)
	    groupresult = 1.0;
	gcostp->opo_reltotb *= (groupresult / 
				gcostp->opo_tups);
	if (gcostp->opo_reltotb < 1)
	    gcostp->opo_reltotb = 1;
	gcostp->opo_tups = groupresult;
	subquery->ops_mask2 |= OPS_AGGCOST;
	break;

      default:
	/* Ignore the rest (MAIN, UNION, VIEW, ...). */
	break;
    }

}


/*{
** Name: opn_tproc_cptest - check join for tproc-induced need for
**	CPjoin
**
** Description:
**	This function looks for table procedures on the inner side
**	of the join whose parameter dependencies are satisfied by the
**	outer side of the join. If there is such a tproc, this must 
**	be a CPjoin.
**
** Inputs:
**      subquery                Ptr to subquery being analyzed
**	omap			Ptr to bit map of outer vars
**	imap			Ptr to bit map of inner vars
**
** Outputs:
**	Returns:
**	    TRUE - if join is tproc-induced CP
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-2008 (dougi)
**	    Written for table procedures.
*/
static bool
opn_tproc_cptest(
	OPS_SUBQUERY	*subquery,
	OPV_BMVARS	*omap,
	OPV_BMVARS	*imap)

{
    i4		i;
    OPV_VARS	*varp;
    OPV_BMVARS	*parmmap;

    /* Loop over Inner variables looking for table procedure to test. */
    for (i = -1; (i = BTnext(i, (PTR)imap, subquery->ops_vars.opv_rv)) >= 0; )
     if ((varp = subquery->ops_vars.opv_base->opv_rt[i])->opv_mask & 
			OPV_LTPROC &&
	(parmmap = varp->opv_parmmap) != (OPV_BMVARS *) NULL &&
	BTsubset((PTR)parmmap, (PTR)omap, subquery->ops_vars.opv_rv))
	return(TRUE);

    /* Dropped off loop - return FALSE. */
    return(FALSE);

}

/*{
** Name: opn_corput	- put a cost ordering structure into a list
**
** Description:
**	Put a cost-ordering structure into the list if it is
**	the least costly for the ordering.  If there was another
**	of the same type of ordering and it was more costly then
**	delete it.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      jsubtp                          cost ordering set to which new ordering
**                                      will be added (join set)
**      icop                            ptr to inner cost ordering 
**      ocop                            ptr outer cost ordering set
**      dio                             estimated disk i/o for join
**      cpu                             estimated cpu for join
**      jeqcls                          joining equivalence class
**      tuplesperblock                  number of tuples of join per block
**      width                           width of joined tuple
**      nblocks                         number of blocks in join
**      eqcp                            contains bit map of equivalence class 
**                                      and width of tuple in join
**      existence_only                  TRUE if only a match is required for
**                                      join
**      tuples                          number of tuples in join
**      pagestouched                    number of disk pages touched during
**                                      join operation
**	orlp, irlp			ptrs to OPN_RLS structures for join
**					inputs (used for table procedures)
**	rlp				ptr to OPN_RLS structure that anchors
**					histograms possibly used to estimate
**					result rows from agg query
**
** Outputs:
**      jsubtp                          cost ordering set updated
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-jun-86 (seputis)
**          initial creation from corput
**	27-sep-88 (seputis)
**          mark new storage structure as sorted if outer sorted
**	6-nov-90 (seputis)
**	    b34081 - avoid cart prod when a join can be used
**	18-apr-91 (seputis)
**	    - bug 36920 - no plan found if floating point exceptions
**	    occurred
**	18-apr-91 (seputis)
**	    - added trace point op188, to print useful plans as they
**	    occur during enumeration
**	18-apr-91 (seputis)
**	    - fixed problem with cart-prod processing, in that some
**	    possibly good plans may have been skipped
**	26-jun-91 (seputis)
**	    - fix unix casting problem
**      5-nov-91 (rickh)
**          Changed opt_f054_allplans to opt_f069_allplans as part of
**          the merge of the outer join path with 6.4.
**      26-dec-91 (seputis)
**          - fix bug 41526
**          deferred update mode sometimes depends on a sort node
**          being used to prevent the halloween problem (i.e. a tuple
**          being updated causing it to move in an index
**          and reappearing later in the index to be
**          updated in perhaps an infinite loop), in some cases
**          top sort nodes may be removed incorrectly resulting in
**          the halloween problem, this could occur when a multi-table
**          update statement is used either with quel, or the ingres
**          from list extension to the update statement in sql.
**      1-apr-92 (seputis)
**          - bug 42847 - access violation when complex query is optimized
**          and internal memory management rountines are invoked.
**	3-dec-92 (ed)
**	    - adjust timeout of OP255 to occur when reaching a plan number 
**	    rather than after
**	31-jan-94 (rganski)
**	    Ported following changes from 6.4 to 6.5:
**	    03-may-93 (rganski)
**	    Modified setting of sjp->cpfound to include case where cartprod is
**	    true.
**	    1-may-93 (ed)
**	    - slight regression due to 48049 - key joins were not considered
**	    cart prods even though all joining attributes were equated to
**	    constants, this incorrectly gave preference to key joins on
**	    constant equivalence classes
**     31-jan-94 (markm)
**          replaced sjp->cpfound with a new mask for subquery->ops_mask
**          "OPS_CPFOUND" since sjp->cpfound was being re-initialized
**          during query optimization (57875).
**	9-may-96 (inkdo01)
**	    Still refusing legitimate cart prods (because of constant eqclass
**	    stuff). Fix to permit cart prods that are at least 10 times better.
**	19-nov-99 (inkdo01)
**	    Changes to support removal of EXsignal for exit processing.
**	18-apr-01 (inkdo01)
**	    Adjust sort cost when top join is CO.
**	19-sep-02 (inkdo01)
**	    Changes to save best CO fragment for new enumeration heuristic.
**	23-oct-02 (inkdo01)
**	    Externalized location of new enumeration fragment CO.
**	25-apr-02 (hayke02)
**	    Do not mark an outer as sorted unless it is not involved in an
**	    outer join or is in a left join. The outer in a right or full 
**	    join is in fact an inner in the outer join, can contain NULLs from
**	    non-matching values in the outer join, and therefore cannot be
**	    marked as sorted. This change fixes bug 107593.
**	08-jan-03 (hayke02)
**	    Modify the above fix for bug 107593 so that the test for an inner
**	    or left join is only and'ed to the sjp->new_ordeqc and
**	    ocop->opo_storage tests, and not the sjp->outer.sort test. This
**	    prevents nested outer joins returning incorrect results. This
**	    change fixes bug 109370.
**	30-mar-2004 (hayke02)
**	    Call opn_exit() with a FALSE longjump.
**	4-nov-04 (inkdo01)
**	    Tiny change to tidy up opo_maps when sort added to top of plan.
**	5-july-05 (inkdo01)
**	    Add call to opn_modcost() to update tuple (and other?) estimates
**	    according to subquery type (e.g. aggregation sqs reduce result tups).
**	30-Jun-2006 (kschendel)
**	    Remove above, now done after subquery is enumerated.
**	5-Dec-2006 (kschendel) SIR 122512
**	    Store unmodified join ordering for later inspection by joinop
**	    processing, in case this CO tree ends up being The One.
**	    (Hash join might want the unrestricted set of join eqc's.)
**	30-may-2008 (dougi)
**	    New logic to detect tproc-induced cartprods.
**	30-oct-08 (hayke02)
**	    Adjust the ops_cost/jcost comparison by opf_greedy_factor or %
**	    figure in trace point op246. This allows otherwise rejected greedy
**	    enum good plan fragments to be chosen. This change fixes bug
**	    121159.
**	26-feb-10 (smeke01) b123349 b123350
**	    Tidy up trace point op255 and op188 code prior to copying it into
**	    opn_prleaf().
**	01-mar-10 (smeke01) b123333 b123373
**	    Improve op188. Replace call to uld_prtree() with call to
**	    opt_cotree() so that the same formatting code is used in op188 as
**	    in op145 (set qep). Save away the current fragment number and
**	    the best fragment number so far for use in opt_cotree(). Also
**	    set ops_trace.opt_subquery to make sure we are tracing the current
**	    subquery.
*/
static OPN_STATUS
opn_corput(
	OPN_SJCB	    *sjp,
	OPO_CO             *ocop,
	OPO_CO             *icop,
	OPN_RLS		   *orlp,
	OPN_RLS		   *irlp,
	OPN_RLS		   *rlp)
{
    OPS_STATE		*global;
    OPS_SUBQUERY        *subquery;
    OPO_COST            jcost;		    /* cost of this join */
    OPO_STORAGE         newstorage;         /* new storage structure of
                                            ** joined relation */
    OPO_CO              *newcop;            /* new cost ordering structure
                                            ** created for relation */
    OPO_CO		*sortcop;           /* if this is the root then a sort
					    ** node may be required */
    bool		cartprod;	    /* TRUE if node is a cart prod */
    bool		tproc_cartprod;
    OPE_IEQCLS		maxeqcls;
    f4			greedy_factor = 1.0;

    subquery = sjp->subquery;
    global = subquery->ops_global;
    maxeqcls = subquery->ops_eclass.ope_ev;
    tproc_cartprod = FALSE;
    cartprod = (sjp->new_jordering == OPE_NOEQCLS)
	||
	(   subquery->ops_bfs.opb_bfeqc	    /* constant eqcls exist */
	&&
	(   (	(sjp->new_jordering < maxeqcls) /* joining attr is constant*/
		&&
		BTtest((i4)sjp->new_jordering, 
		    (char *)subquery->ops_bfs.opb_bfeqc)
	    )
	    ||
	    (	(sjp->new_jordering >= maxeqcls)
		&&
		BTsubset((char *) subquery->ops_msort.opo_base->
		    opo_stable[sjp->new_jordering - maxeqcls]->opo_bmeqcls,
		    (char *)subquery->ops_bfs.opb_bfeqc, (i4)maxeqcls)
					    /* set of joining attrs are
					    ** constant */
	    )

	));

    /* If there's a TPROC in the subquery, check if we need to make this
    ** a cartprod. */
    if (!cartprod && subquery->ops_mask2 & OPS_TPROC)
    {
	cartprod = tproc_cartprod = opn_tproc_cptest(subquery, 
				&orlp->opn_relmap, &irlp->opn_relmap);
	if (tproc_cartprod)
	{
	    sjp->new_jordering = OPE_NOEQCLS;	/* strip join ordering */
	    sjp->inner.sort = (sjp->inner.duplicates != 0);
	    sjp->outer.sort = (sjp->outer.duplicates != 0);
	}
    }

    jcost = opn_cost(subquery, sjp->dio, sjp->cpu);

    if (subquery->ops_oj.opl_base
	&&
	(icop->opo_sjpr == DB_ORIG)
	&&
	(   (sjp->opn_ojtype == OPL_RIGHTJOIN) 
	    ||
	    (sjp->opn_ojtype == OPL_FULLJOIN)
	)
	&&
	(   !global->ops_cb->ops_check
	    ||
	    !opt_strace( global->ops_cb, OPT_F070_RIGHT)
	))
	return(OPN_SIGOK);    /* FIXME, remove when support added
		    ** since OPC/QEF cannot support right or full key joins 
		    ** eliminate those type of joins at this point */

    if (subquery->ops_mask & OPS_LAENUM)
    {
	i4	first = 0, second = 0;

	greedy_factor = global->ops_cb->ops_alter.ops_greedy_factor;
	if (global->ops_cb->ops_check &&
	    opt_svtrace(global->ops_cb, OPT_F118_GREEDY_FACTOR,
							    &first, &second))
	    greedy_factor = (f4)first/100.0;
    }

    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
    {
    }
    else
    {
	if ((jcost>=greedy_factor*subquery->ops_cost)
	    &&
	    (
		!(global->ops_gmask & OPS_BFPEXCEPTION)
		||
		(global->ops_gmask & OPS_FPEXCEPTION) /* only consider costs
					    ** if the current best plan was created
					    ** without floating point exceptions,
					    ** or if not, then ignore costs if the
					    ** current subtree was created without
					    ** floating point exceptions, i.e. in
					    ** an attempt to find a plan which
					    ** does not get floating point exceptions
					    */
	    )
	    &&
	    (	!global->ops_estate.opn_rootflg
		||
		!(subquery->ops_mask & OPS_CPFOUND)
		||
		(cartprod && !tproc_cartprod)
	    )
            &&
            (   !global->ops_cb->ops_check
                ||
                !opt_strace( global->ops_cb, OPT_F069_ALLPLANS)
            ))
	    return(OPN_SIGOK);		    /* do not consider if it costs
                                            ** more than the current best plan
					    ** - for distributed need to
					    ** consider cost at each site
					    ** - if only cart prod has been found
					    ** then continue until a key join has
					    ** been found */
    }

    sortcop = (OPO_CO *)NULL;

    if ((((sjp->new_ordeqc != OPE_NOEQCLS)
	&&
	(ocop->opo_storage == DB_SORT_STORE))
	&&
	((sjp->ojid == OPL_NOOUTER) || (sjp->opn_ojtype == OPL_LEFTJOIN)))
	||
	sjp->outer.sort)
	newstorage = DB_SORT_STORE;	    /* does an ordering exist on the
					    ** outer */
    else
	newstorage = DB_HEAP_STORE;	    /* no ordering exists */

    newcop = opn_cmemory(subquery);         /* get a CO node to be used
                                            ** for internal calculation, it
                                            ** is not necessarily considered
                                            ** useful at this point */

    if (newcop->opo_outer = ocop)
        ocop->opo_pointercnt++;             /* point to outer relation and
                                            ** increment reference count */
    if (newcop->opo_inner = icop)
        icop->opo_pointercnt++;             /* point to inner relation and
                                            ** increment reference count */
    newcop->opo_storage = newstorage;
    newcop->opo_ordeqc = sjp->new_ordeqc;
    newcop->opo_union.opo_ojid = sjp->ojid; /* no ojid */
    newcop->opo_sjpr = DB_SJ;               /* simple join */
    newcop->opo_sjeqc = sjp->new_jordering; /* joining equivalence class */
    /* Remember unreduced joining eqcls as well in case it's a hash join,
    ** but for key joins make sjeqc and orig_sjeqc the same because the
    ** original sjp->jordering is apparently sometimes meaningless for
    ** key joins and it gets the later opj-maordering code into trouble.
    */
    newcop->opo_orig_sjeqc = sjp->jordering;
    if (icop && icop->opo_sjpr == DB_ORIG)
	newcop->opo_orig_sjeqc = newcop->opo_sjeqc;

    if (global->ops_estate.opn_rootflg &&
	    !(subquery->ops_mask & OPS_LAENUM)) /* if we are at the root */
	newcop->opo_maps = &subquery->ops_eclass.ope_maps; /* return all attributes
					    ** needed by target list */
    else
	newcop->opo_maps = &sjp->join.eqsp->opn_maps;   /* ptr to equivalence class map
                                            ** of attributes in new relation*/
    newcop->opo_cost.opo_dio = sjp->dio;    /* disk i/o needed to create new
                                            ** new relation */
    newcop->opo_cost.opo_cpu = sjp->cpu;    /* cpu needed */
    newcop->opo_cost.opo_tups = sjp->join.tuples; /* number of tuples in new relation
                                            */
    newcop->opo_cost.opo_pagestouched = sjp->pagestouched; /* number of pages
                                            ** touched in new relation */
    newcop->opo_cost.opo_sortfact = OPO_DSORTFACT; /* do not have any idea about
					    ** sortfact and adfactor because 
					    ** we are ordered on new eqclass */
    newcop->opo_cost.opo_adfactor = OPO_DADFACTOR;
    newcop->opo_cost.opo_tpb = sjp->tuplesperblock > 1.0 ? sjp->tuplesperblock : 1.0;

    newcop->opo_cost.opo_pagesize = sjp->pagesize;
    newcop->opo_cost.opo_cvar.opo_cache = sjp->jcache; /* save number of cache pages
					    ** used to evaluate this subtree */
    newcop->opo_cost.opo_reltotb = sjp->blocks;
    newcop->opo_existence = sjp->existence_only;
    newcop->opo_isort = sjp->inner.sort;    /* does the inner have a sort node*/
    newcop->opo_osort = sjp->outer.sort;    /* does the outer have a sort node*/
    newcop->opo_idups = sjp->inner.duplicates != 0;
    newcop->opo_odups = sjp->outer.duplicates != 0;
    /* count another fragment for trace point op188 */
    subquery->ops_currfragment++;
    if (global->ops_estate.opn_rootflg)	    /* if we are at the root
                                            ** then save this new co since it 
                                            ** is the cheapest solution so far
					    */
    {
	/* FIXME - can consider the cost of adding a user defined sort order to the
        ** cost model at this point, in much the same way as the halloween problem
        ** was solved above */
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{
	    /* for simple aggregate the best cost of all the plans is
            ** needed; for other subqueries, the best cost for each
	    ** site is required */
	    OPO_CPU	    sort_cpu;
	    OPO_BLOCKS	    sort_dio;

	    if (subquery->ops_msort.opo_mask & OPO_MTOPSORT)
	    {
		if (sjp->existence_only)
		{
		    /* Recompute sort cost to reflect reduced result
		    ** size of existence only join. */
		    opn_reformat(subquery, sjp->join.tuples,
			sjp->join.eqsp->opn_relwid, &sort_dio, &sort_cpu);
		}
		else
		{
		    /* Use pre-computed sort estimates. */
		    sort_dio = sjp->join.eqsp->opn_dio;
		    sort_cpu = sjp->join.eqsp->opn_cpu;
		}
	    }
	    else
	    {
		sort_cpu = 0.0;
		sort_dio = 0.0;
	    }
	    if (!opd_jcost(subquery, 
		    sjp->join.rlsp,
		    sjp->join.sbtp, 
		    newcop,
		    sjp->join.eqsp->opn_relwid, 
		    sort_cpu,
		    sort_dio)
		)
	    {
		opn_dcmemory(subquery, newcop);
		return(OPN_SIGOK);	    /* return if no site exists
					    ** for which this CO would be
					    ** useful */
	    }
	    else if (opd_bestplan(subquery, newcop,
			((subquery->ops_msort.opo_mask & OPO_MTOPSORT) != 0),
			    sort_cpu, sort_dio, &sortcop))
		return(OPN_SIGOK);
	}
	else
	{   /* local optimization only ... distributed optimization already processed */
	    bool                deferredsort;   /* TRUE - if a temporary relation
						** is required to support 
						** DEFERRED mode */
	    if ( global->ops_estate.opn_halloween /* is special
						** case handling of deferred update
						** needed */
		&&
		subquery->ops_sqtype == OPS_MAIN /* only main query will cause the
						** update to occur */
		)
	    {
		/* in order to implement DEFERRED update, the optimizer may need
		** to create a temporary relation, containing the updates and the
		** values to be updated, FIXME this routine should be rewritten
                ** since DMF assumptions have changed, and top sort nodes are
		** added in enumeration now */
		if ( opn_deferred (subquery, &jcost, ocop, icop, 
			sjp->blocks, sjp->join.tuples, 
			sjp->join.eqsp->opn_relwid, &deferredsort) )
		    return(OPN_SIGOK);		/* return if cost of creating
						** temporary is too much */
	    }
	    else
		deferredsort = FALSE;

	    /* Cartesian product heuristic tries to avoid cartprod on top
	    ** of query plan if there's an alternative. Recent changes which
	    ** make joining eqclasses which are also constant (because of 
	    ** "=" restriction predicate on a join column) make some legitmate
	    ** joins look like cart prods. So now we only avoid the cartprods
	    ** if they aren't at least 10 times better than the alternative. */
            if (cartprod)
            {   if (subquery->ops_bestco && !(subquery->ops_mask & OPS_CPFOUND)
			&& jcost*10 > subquery->ops_cost)
                {  /* avoid using a cart prod on the top node if possible */
                    return(OPN_SIGOK);  /* current best plan is not a cart prod */
                }
            }
            else if (subquery->ops_mask & OPS_CPFOUND &&
			jcost < subquery->ops_cost*10)
                subquery->ops_cost = OPO_LARGEST; /* this ensures that the join
                                        ** overrides the cartesean product
                                        ** if possible */

	    if ((subquery->ops_msort.opo_mask & OPO_MTOPSORT) || deferredsort)
	    {	/* FIXME - can detect user specified orderings here and
		** avoid a sort node */
		OPO_ISORT	sordering;
		bool		sortrequired;

		sortrequired = deferredsort;
		if (newstorage == DB_SORT_STORE)
		    sordering = sjp->new_ordeqc;
		else
		    sordering = OPE_NOEQCLS;
		if (sjp->existence_only)
		{
		    opn_reformat(subquery, sjp->join.tuples, 
			    sjp->join.eqsp->opn_relwid, 
			    &sjp->join.eqsp->opn_dio,
			    &sjp->join.eqsp->opn_cpu);
					/* if existence_only, recompute sort costs */
		}

		if (opn_checksort(subquery, &subquery->ops_bestco, subquery->ops_cost,
			sjp->dio, sjp->cpu, sjp->join.eqsp, &jcost, 
			sordering, &sortrequired, newcop)
		    )
		    return(OPN_SIGOK);	/* return if added cost of sort node, or creating
					** relation is too expensive */
		if (sortrequired)
		{
		    OPO_COMAPS	*oldmaps;

		    if (subquery->ops_bestco)
			oldmaps = subquery->ops_bestco->opo_maps;
		    else oldmaps = (OPO_COMAPS *) NULL;
		    sortcop = opn_oldplan(subquery, &subquery->ops_bestco); 
					/* IMPORTANT - allocate memory here so that
					** we do  not run out of memory prior to
					** calling opn_cmemory for the outer cop, and deleting
					** the previous best plan; if we run out
					** of memory without a previous good plan
					** then the query may not execute */
		    if (oldmaps)
			sortcop->opo_maps = oldmaps;
		    else sortcop->opo_maps = &sjp->join.eqsp->opn_maps;
		}
	    }

            /* mark new current best plan as not being a cart prod */
            if (cartprod)
                subquery->ops_mask |= OPS_CPFOUND;
            else
                subquery->ops_mask &= ~OPS_CPFOUND;

	    opn_dcmemory(subquery, subquery->ops_bestco); /* release the space for
						** previous best case CO tree and
						** replace it with this new one */
	    subquery->ops_bestco = newcop;  /*  new best plan */
	    subquery->ops_cost = jcost;	    /* new best cost estimate */

	    /* If we're in new enumeration processing and there's an available
	    ** CO-fragment, save it. */
	    if (subquery->ops_mask & OPS_LAENUM)
		opn_bestfrag(subquery);
	    global->ops_estate.opn_tidrel = deferredsort; /* this field is not valid for
					    ** distributed, it causes TID sort nodes
					    ** to be created */
            subquery->ops_tcurrent++;       /* increment plan number */
	    subquery->ops_tplan = subquery->ops_tcurrent; /* save the plan ID
					    ** for the current best plan */
	    /* save the best fragment so far for trace point op188 */
	    subquery->ops_bestfragment = subquery->ops_currfragment;  
	    if (global->ops_gmask & OPS_FPEXCEPTION)
		global->ops_gmask |= OPS_BFPEXCEPTION; /* a plan was found which
					    ** did not have a floating point exception
					    ** so skip over subsequent plans with
					    ** floating point exceptions */
	    else
		global->ops_gmask &= (~OPS_BFPEXCEPTION); /* reset exception flag
					    ** if plan was found free of float
					    ** exceptions */
	}
	global->ops_estate.opn_search = FALSE; /* new best CO
					    ** so memory garbage collection
					    ** routines may be useful */
    }
    else
    {	/* this subtree is not at the root */
	/* 
	** If this is the least costly so far then put it in colist,
	** delete another co if there was a more costly one with same 
	** storage structure and ordering equivalence class
	*/
        newcop->opo_maps = &sjp->join.eqsp->opn_maps;   /* ptr to equivalence class map
                                            ** of attributes in new relation*/
	if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	{   
	    if (!opd_jcost(subquery, 
		sjp->join.rlsp,
		sjp->join.sbtp, 
		newcop,
		sjp->join.eqsp->opn_relwid, 
		(OPO_CPU)0.0, (OPO_BLOCKS)0.0))
	    {
		opn_dcmemory(subquery, newcop);
		return(OPN_SIGOK);	    /* return if no site exists
					    ** for which this CO would be
					    ** useful */
	    }
            opn_coinsert (sjp->join.sbtp, newcop); /* insert the cost ordering into
                                        ** the cost ordering set */
	}
	else
	{   /* local optimization has only one cost per CO node instead of
	    ** an array */
	    OPN_SUBTREE            *subtp;	    /* current cost ordering set
						** or "subtree" being analyzed*/
	    {
		for (subtp = sjp->join.sbtp; subtp; subtp = subtp->opn_stnext)   /* for the 
						    ** co's in all the subtrees */
		{
		    OPO_CO             *cop;	    /* current cost ordering subtree
						    ** being analyzed */
		    for (cop = subtp->opn_coforw; 
			 cop != (OPO_CO *) subtp; 
			 cop = cop->opo_coforw)
		    {
			if (cartprod)
			{
			    if(cop->opo_sjeqc != OPE_NOEQCLS)
			    {
				opn_dcmemory(subquery, newcop);
				return(OPN_SIGOK);	/* prefer joins over cart prods */
			    }
			}
			else
			{
			    if (cop->opo_sjeqc == OPE_NOEQCLS)
				cop->opo_deleteflag = TRUE;	/* since a non-cart prod
						    ** is about to be added, any existing
						    ** cart prod should be removed */
			}
			if ((cop->opo_storage == newstorage)
			    && 
			    (cop->opo_ordeqc == sjp->new_ordeqc) /* FIXME - ordering may
						   ** be a subset of new ordering */
			    && 
			    (!cop->opo_deleteflag))
			{   /* found a cost ordering with the same storage structure
			    ** ordered on the same equivalence class
			    */
			    if (jcost >= opn_cost(subquery, cop->opo_cost.opo_dio, 
						  cop->opo_cost.opo_cpu)
			       )
			    {
                                opn_dcmemory(subquery, newcop);
				return(OPN_SIGOK);    /*existing cost ordering is better*/
			    }
			    else
				cop->opo_deleteflag = TRUE; /* new cost ordering is
						    ** better so mark the old one
						    ** as deleteable */
			}
		    }
		}
		opn_coinsert (sjp->join.sbtp, newcop); /* insert the cost ordering into
						** the cost ordering set */
	    }
	}
    }
    if (sortcop)
    {
	opn_createsort(subquery, sortcop, newcop, sjp->join.eqsp);
	subquery->ops_bestco = sortcop;
    }

    /* save histogram for later reference in case it's an agg subquery,
    ** we'll use it to modify the agg output estimates. */
    subquery->ops_besthisto = NULL;
    if (rlp != NULL)
	subquery->ops_besthisto = rlp->opn_histogram;

#   ifdef xNTR1
    if (Jne.Rootflg && tTf(23, 2))
	    pcotree (Jne.Gminco);
#   endif

    if (global->ops_cb->ops_check)
    {
	i4	    first;
	i4	    second;

	/* Check for trace point op188 to see if we are to print this CO tree */
	/* 
	** Trace points op188 and op145 (set qep) both use opt_cotree().
	** op188 expects opt_conode to point to the fragment being traced, and op145
	** expects it to be NULL (so that ops_bestco is used). The NULL/non-NULL
	** value of opt_conode also results in different display behaviour. For these
	** reasons opt_conode is also NULLed here after the call to opt_cotree().
	*/
	if (opt_strace( global->ops_cb, OPT_F060_QEP))
	{
	    global->ops_trace.opt_subquery = subquery;
	    global->ops_trace.opt_conode = newcop;
	    opt_cotree(newcop);
	    global->ops_trace.opt_conode = NULL; 
	}

	/* If we have a usable plan, check for trace point op255 timeout setting */
	if( subquery->ops_bestco &&
	    opt_svtrace( global->ops_cb, OPT_F127_TIMEOUT, &first, &second) &&
	    !(subquery->ops_mask & OPS_LAENUM) &&
	    (first <= subquery->ops_tcurrent) &&
	    /* check if all subqueries should be timed out OR a particular subquery is being searched for */
	    ( (second == 0) || (second == subquery->ops_tsubquery) ) )
	{
	    opx_verror(E_DB_WARN, E_OP0006_TIMEOUT, (OPX_FACILITY)0);
	    opn_exit(subquery, FALSE);
	    /*
	    ** At this point we return the subquery->opn_bestco tree.
	    ** This could also happen in freeco and enumerate 
	    */
	    return(OPN_SIGEXIT);
	}
    }

    /* increment pointer count after possible exits, since this routine
    ** has been called with the pointer counts already incremented to
    ** prevent the memory management routines from accidently deleting
    ** the CO node being used */
    ocop->opo_pointercnt++;		    /* point to outer relation and
                                            ** increment reference count */
    icop->opo_pointercnt++;		    /* point to inner relation and
                                            ** increment reference count */
    return(OPN_SIGOK);
}

/*{
** Name: opn_goodco	- is this CO useful?
**
** Description:
**      This routine will return TRUE if the CO tree is less than the minimum 
**      cost tree found so far and if the tree is not deleteable.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to CO tree to be tested
**
** Outputs:
**	Returns:
**	    TRUE if CO tree cost is less than minimum cost found so far
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jun-86 (seputis)
**          initial creation from goodco in calcost.c
[@history_line@]...
*/
bool
opn_goodco(
	OPS_SUBQUERY	   *subquery,
	OPO_CO             *cop)
{
    return ((
		opn_cost(subquery,cop->opo_cost.opo_dio, cop->opo_cost.opo_cpu) 
		< 
		subquery->ops_cost	/* is cost less than minimum so far */
	    ) 
	    && 
		!cop->opo_deleteflag);  /* and is it not deleteable */
}

/*{
** Name: opn_calcost	- calculate cost ordering combinations
**
** Description:
**	Given a pair of subtrees, each with their CO-tree forest,
**	match up each outer cost-ordering with each inner cost-ordering.
**	For each matchup, calculate a cost-ordering which is the result
**	of their join, then put this cost-ordering in the current
**	cost-ordering set if it is the cheapest cost for this ordering
**	so far.  Cost ordering trees might be:
**	- a simple join (assumption here is that it's a merge join unless
**	  jordering is noeqcls, which is a cart-prod).
**	- a proj-rest
**	- an ORIG which is only used for K/T-joining on the inner.
**	and the result of a non-ORIG might be sorted (DB_SORT_STORE) or
**	not known to be sorted (DB_HEAP_STORE).  DB_RSORT ought not to
**	occur here, as they are top-sorts.  (Intermediate sorts are
**	indicated by the isort/osort flags and/or the storage structure.)
**
**	Only consider sorting the minimum-cost cost-ordering in the
**	inner and outer.  Any inner/outer CO-tree candidate that is more
**	costly than the min-cost candidate will have to stand on its
**	own merits, i.e. without an additional sort.  If we get through
**	the costing loops (inner or outer) with a sort-me candidate,
**	try the same CO-tree candidate without sorting to see if that
**	will do even better.
**
**	Don't consider sorts that are not on the joining equivalence class.
**      This avoids unnecessary sorts that can be expensive if we 
**      underestimate the number of tuples.  This should be reconsidered
**      since possibly useful search space has been eliminated FIXME.
**
**      FIXME - need to consider multi-attribute sort nodes, so that the
**      cost of a join is reduced since the size of the inner run is
**      smaller.  The joinop cost model can be adjusted by choosing
**      a smaller repitition factor as some function of the repitition
**      factors of the attributes in the sort.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      jordering			joining ordering set of eqcls
**      isbtp                           inner cost ordering set
**      osbtp                           outer cost ordering set
**      jsbtp                           empty cost ordering set for this
**                                      join
**      irlp                            inner set of histograms
**      orlp                            outer set of histograms
**      tuples                          estimated tuples produced by join
**      eqsp                            header structure for any existing
**                                      cost orderings for this relation
**      blocks                          estimated blocks in join
**      ieqsp                           inner OPN_EQS structure containing
**                                      isbtp
**      oeqsp                           outer OPN_EQS structure containing
**                                      orlp
**	kjonly				if hints are active, TRUE means 
**					only produce kjoin
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
**	1-jun-86 (seputis)
**          initial creation
**	8-aug-88 (seputis)
**          initialized some cost variables to default values
**	15-sep-88 (seputis)
**          fix ^ logic since ^^ does not exist
**	26-oct-88 (seputis)
**          add rlp parameter to get opn_corelated
**	28-oct-88 (seputis)
**          do not calculate sorts for nodes with no attributes
**          allow implicit tid to be outer in case of inner being a
**		subselect
**	11-aug-89 (seputis)
**	    - fix b6538
**	1-nov-89 (seputis)
**	    - fix b8521, added "keying" substructure to sjstate so that pagetouched
**	    estimate is done on page count prior to project restrict
**	15-nov_89 (seputis)
**	    - fix keying problem which caused optimizedb to intermittently report
**	    a unnecessary sort node error
**	29-mar-90 (seputis)
**	    - fix metaphor performance problem by making sorts of one tuple preferred
**	    if a join can be made
**	16-may-90 (seputis)
**	    - b21483,b21485 some cost model improvements suggested by DMF group
**	24-jun-90 (seputis)
**	    - fix b8410 - overestimate of DIO with some multi-attribute keyed joins
**	20-feb-91 (seputis)
**	    - fix b33279 - use outer tuple count as max for existence check join
**	    - check for 1 datapage for btree secondary indexes
**	8-may-92 (seputis)
**	    - b44212, check for overflow in opo_pointercnt
**	15-apr-93 (ed)
**	    - allow right tid FSM joins, since right tid joins not supported
**      31-jan-94 (markm)
**          - removed all references to cpfound (cartprod/best plan indication
**          is now checked through ops_mask OPS_CPFOUND).
**	1-may-94 (ed)
**	    - b59597 - do not allow Pjoin to be considered since this access
**	    path is removed from 6.5
**	06-mar-95 (nanpr01)
**	    - calculate the tpb based on pagesize. For variable pagesize
**	    project.
**	6-aug-98 (inkdo01)
**	    Changes to assure only key joins are used to implement a spatial
**	    join.
**	16-mar-99 (inkdo01)
**	    Changes to fix join histograms when existence_only join alters
**	    join cardinality estimate.
**	 1-apr-99 (hayke02)
**	    Use tbl_id.db_tab_index and tbl_storage_type rather than
**	    tbl_dpage_count <= 1 to check for a btree or rtree index. As of
**	    1.x, dmt_show() returns a tbl_dpage_count of 1 for single page
**	    tables, rather than 2 in 6.x. This change fixes bug 93436.
**	19-nov-99 (inkdo01)
**	    Changes to support removal of EXsignal for exit processing.
**	25-sep-00 (inkdo01)
**	    One more "return(OPN_SIGOK)" to complete above.
**	12-oct-00 (inkdo01)
**	    Skip composite histograms in loop which uses attr info.
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	 6-sep-04 (hayke02)
**	    Set kjeqcls_subset FALSE if we find a TID join equiv class
**	    (OPE_CTID). This prevents cost over-estimation for 'TID only'
**	    joins between secondary indices on the same relation. This change
**	    fixes problem INGSRV 2941, bug 112874.
**	29-dec-04 (inkdo01)
**	   Avoid use of indexes for mixed collation joins.
**	 5-apr-05 (hayke02)
**	    Back out the fix for 112874, INGSRV 2941. This change fixes bug
**	    114263, problem INGSRV 3241.
**	22-mar-06 (dougi)
**	    Added kjonly parm to force kjoins for optimizer hints project.
[@history_line@]...
*/
OPN_STATUS
opn_calcost(
	OPS_SUBQUERY       *subquery,
	OPO_ISORT	   jordering,
	OPO_ISORT	   keyjoin,
	OPN_SUBTREE        *osbtp,
	OPN_SUBTREE        *isbtp,
	OPN_SUBTREE        *jsbtp,
	OPN_RLS            *orlp,
	OPN_RLS            *irlp,
	OPN_RLS            *rlp,
	OPO_TUPLES	   tuples,
	OPN_EQS            *eqsp,
	OPO_BLOCKS	   blocks,
	OPN_EQS            *oeqsp,
	OPN_EQS            *ieqsp,
	OPN_JTREE	   *np,
	OPL_IOUTER         ojid,
	bool		   kjonly)
{
    OPN_SJCB		sjstate;	    /* statistics used to calculate
					    ** the simple join cost given
					    ** the inner and outer */
    OPE_IEQCLS          maxeqcls;	    /* number of equivalence classes
                                            ** defined in the query */
    OPZ_AT              *abase;		    /* ptr to array of ptrs to joinop
                                            ** attributes */
    OPO_CO              *imincop;           /* ptr to CO tree which is the
                                            ** minimum of all cost orderings
                                            ** in the inner relation */
    OPO_CO              *omincop;           /* ptr to CO tree which is the
                                            ** minimum of all cost orderings
                                            ** in the outer relation */
    OPV_RT                 *vbase;	    /* ptr to base of array of ptrs
                                            ** to joinop variables */
    OPN_RLS		*krlp;		    /* valid only if a key join has
					    ** been specified */
    OPV_VARS		*kvarp;		    /* initialized if keyjoin is 
					    ** not OPE_NOEQCLS */
    OPN_SUBTREE         *aggisbtp;          /* used to traverse inner aggregate
                                            ** list */
    OPN_SUBTREE         *aggosbtp;          /* used to traverse outer aggregate
                                            ** list */
    OPS_STATE           *global;
    OPL_IOUTER		maxoj;
    i4		pagesize = -1;
    i4		maxtupsize = -1;
    bool		spjoin = FALSE;
    bool	mixedcoll = FALSE;

    global = subquery->ops_global;
    maxoj = subquery->ops_oj.opl_lv;
    sjstate.subquery = subquery;
    sjstate.keyjoin = keyjoin;
    sjstate.jordering = jordering;
    sjstate.ojid = ojid;
    sjstate.blocks = blocks;
    sjstate.opn_cdesc = (OPN_SDESC *)global->ops_estate.opn_sfree;
    sjstate.np = np;

    /* Set spatial join flag. */
    if (keyjoin >= 0 && keyjoin < subquery->ops_eclass.ope_ev &&
	subquery->ops_eclass.ope_base->ope_eqclist[keyjoin]->ope_mask & OPE_SPATJ)
     spjoin = TRUE;

    sjstate.join.tuples = tuples;
    if (sjstate.join.tuples < 1.0)
	sjstate.join.tuples = 1.0;	    /* at least one tuple */
    sjstate.join.rlsp = rlp;
    sjstate.join.eqsp = eqsp;
    sjstate.join.sbtp = jsbtp;
    sjstate.join.sort = FALSE;
    sjstate.join.stats.mas_next = (OPN_SDESC *)NULL;
    sjstate.join.stats.mas_sorti = jordering;
    sjstate.join.stats.mas_unique = FALSE;
    sjstate.join.stats.mas_reptf = 1.0;
    sjstate.join.stats.mas_nunique = sjstate.join.tuples;

    sjstate.outer.tuples = orlp->opn_reltups;
    if (sjstate.outer.tuples < 1.0)
	sjstate.outer.tuples = 1.0;	    /* at least one tuple */
    sjstate.outer.rlsp = orlp;
    sjstate.outer.eqsp = oeqsp;
    sjstate.outer.sbtp = osbtp;
    sjstate.outer.sort = FALSE;
    sjstate.outer.stats.mas_next = (OPN_SDESC *)NULL;
    sjstate.outer.stats.mas_sorti = jordering;
    sjstate.outer.stats.mas_unique = FALSE;
    sjstate.outer.stats.mas_reptf = 1.0;
    sjstate.outer.stats.mas_nunique = sjstate.outer.tuples;

    sjstate.inner.tuples = irlp->opn_reltups;
    if (sjstate.inner.tuples < 1.0)
	sjstate.inner.tuples = 1.0;	    /* at least one tuple */
    sjstate.inner.rlsp = irlp;
    sjstate.inner.eqsp = ieqsp;
    sjstate.inner.sbtp = isbtp;
    sjstate.inner.sort = FALSE;
    sjstate.inner.stats.mas_next = (OPN_SDESC *)NULL;
    sjstate.inner.stats.mas_sorti = jordering;
    sjstate.inner.stats.mas_unique = FALSE;
    sjstate.inner.stats.mas_reptf = 1.0;
    sjstate.inner.stats.mas_nunique = sjstate.inner.tuples;

    if (global->ops_cb->ops_alter.ops_sortmax < 0.0)
    {	/* check whether sort buffers are within limits */
	sjstate.inner.allowed = 
	sjstate.outer.allowed = TRUE;
    }
    else
    {
	sjstate.inner.allowed = global->ops_cb->ops_alter.ops_sortmax 
	    > (ieqsp->opn_relwid * irlp->opn_reltups);
	sjstate.outer.allowed = global->ops_cb->ops_alter.ops_sortmax 
	    > (oeqsp->opn_relwid * orlp->opn_reltups);
    }
    vbase = subquery->ops_vars.opv_base;    /* init ptr to base of array of
                                            ** ptrs to joinop variables */
    if (keyjoin >= 0)
    {	/* since the inner has an OPN_RLS structure which describes
	** the result after a project restrict; the original base
	** relation descriptor needs to be used to handle keying */
	OPV_IVARS	key_var;
	DMT_TBL_ENTRY	*dmfptr;	/* ptr to DMF table info */

	key_var = BTnext((i4)-1, (char *)&irlp->opn_relmap, 
	    (i4)BITS_IN(irlp->opn_relmap));
	kvarp = vbase->opv_rt[key_var];
	krlp = kvarp->opv_trl;
	dmfptr = kvarp->opv_grv->opv_relation->rdr_rel;
	if (dmfptr->tbl_id.db_tab_index > 0
	    &&
	    (dmfptr->tbl_storage_type == DB_BTRE_STORE ||
	     dmfptr->tbl_storage_type == DB_RTRE_STORE))
	{   /* calculate fanout since secondary index with BTREE
	    ** or RTREE does not have any data pages, since this
	    ** is used to limit pages touched, the number of leaf
	    ** index pages would be sufficient */
	    sjstate.maxpagestouched = kvarp->opv_leafcount;
	}
	else
	    sjstate.maxpagestouched = dmfptr->tbl_dpage_count;
	sjstate.keying.tuples = krlp->opn_reltups;
	if (sjstate.keying.tuples < 1.0)
	    sjstate.keying.tuples = 1.0;	    /* at least one tuple */
	sjstate.keying.rlsp = krlp;
	sjstate.keying.eqsp = ieqsp;
	sjstate.keying.sbtp = isbtp;
	sjstate.keying.sort = FALSE;
	sjstate.keying.stats.mas_next = (OPN_SDESC *)NULL;
	sjstate.keying.stats.mas_sorti = keyjoin;
	sjstate.keying.stats.mas_unique = TRUE;
	sjstate.keying.stats.mas_reptf = 1.0;
	sjstate.keying.stats.mas_nunique = sjstate.keying.tuples;
    }
    if (subquery->ops_mask & OPS_BYPOSITION)
    {	/* eliminate part of search space in which the relation to be
	** updated requires TID lookups as opposed to position lookups */
	OPV_BMVARS	    temp;

	MEcopy((PTR)&irlp->opn_relmap, sizeof(temp), (PTR)&temp);
	BTand((i4)BITS_IN(temp), (char *)global->ops_gdist.opd_byposition,
	    (char *)&temp);		    /* check if there is any relation
					    ** referenced on the inner which
					    ** is a "by position" relation */
	if (BTnext((i4)OPV_NOVAR, (char *)&temp, (i4)BITS_IN(temp)) >= 0)
	    return(OPN_SIGOK);		    /* since the target relation or the
					    ** equivalent index is
					    ** on the inner, ignore this query
					    ** plan since only POSITION updates
					    ** are allowed */
    }

    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    maxeqcls = subquery->ops_eclass.ope_ev; /* set maximum number of
					    ** equivalence classes */
    if ((jordering < maxeqcls) || (keyjoin == OPE_NOEQCLS))
	sjstate.kjeqcls_subset = FALSE;
    else
    {
	OPE_BMEQCLS	*jeqcmap;
	OPE_IEQCLS	jeqcls;

	jeqcmap = subquery->ops_msort.opo_base->
				opo_stable[jordering - maxeqcls]->opo_bmeqcls;
	if (keyjoin < maxeqcls)
	    sjstate.kjeqcls_subset = TRUE;
	else
	{ OPE_BMEQCLS	    *keqcmap;

	    keqcmap = subquery->ops_msort.opo_base->
				opo_stable[keyjoin - maxeqcls]->opo_bmeqcls;
	    if (BTcount((char *)keqcmap, (i4)maxeqcls) !=
					BTcount((char *)jeqcmap, (i4)maxeqcls)
		&&
	        BTsubset((char *)keqcmap, (char *)jeqcmap, (i4)maxeqcls))
		sjstate.kjeqcls_subset = TRUE;
	    else
		sjstate.kjeqcls_subset = FALSE;
	}
    }

    if (jordering > OPE_NOEQCLS)
     if (jordering < maxeqcls)
    {
	if (subquery->ops_eclass.ope_base->ope_eqclist[jordering]->
			ope_mask & OPE_MIXEDCOLL)
	    mixedcoll = TRUE;
    }
    else
    {
	/* Multi-eqc join - loop over constituent eqcs. */
    }

    sjstate.notimptid = TRUE;			    /* no implicit tid */
    if (subquery->ops_oj.opl_base)
    {
        sjstate.opn_ojtype = opl_ojtype(subquery, ojid,
            &orlp->opn_relmap, &irlp->opn_relmap);
	if ((sjstate.opn_ojtype == OPL_FULLJOIN)
	    &&
	    !(np->opn_jmask & OPN_FULLOJ))
	    sjstate.opn_ojtype = OPL_INNERJOIN; /* this is a TID join
					    ** and no OJ boolean factors
					    ** can evaluted here so use
					    ** normal TID join estimations
					    */
    }
    else
        sjstate.opn_ojtype = OPL_INNERJOIN;
    if (jordering == OPE_NOEQCLS)
    {	/* if no joining equivalence class then there must be a cartesian
	** product */
#if 0
#ifdef    E_OP048A_NOCARTPROD
	/* validity check for cartesian product */
	if (!subquery->ops_vars.opv_cart_prod)
	    opx_error(E_OP048A_NOCARTPROD);
#endif
#endif
    }
    else 
    {
	OPZ_IATTS	    iattr;	    /* inner attribute if an implicit join has
					    ** been found */
	OPZ_IATTS	    oattr;	    /* outer attribute if an implicit join has
					    ** been found */
	OPZ_IATTS	    kattr;	    /* outer attribute if an implicit join has
					    ** been found */
	opn_stats(&sjstate, jordering, irlp, &sjstate.inner.stats, &iattr);
	opn_stats(&sjstate, jordering, orlp, &sjstate.outer.stats, &oattr);
	if (keyjoin >= 0)
	    opn_stats(&sjstate, keyjoin, krlp, &sjstate.keying.stats, &kattr);
	if ((iattr >= 0) && (oattr >= 0))
	{
	    OPO_CO		       *innerc; /* ptr to inner co node */

	    /* FALSE if implicit tid is used for joining to inner relation */
	    sjstate.notimptid = (DB_IMTID !=	
		abase->opz_attnums[iattr]->opz_attnm.db_att_id);

	    /* cannot have a join from an outer implicit tid to an inner explicit
	    ** tid (although implicit-implicit joins are allowed)
	    ** - the exception to this case is when the implicit tid is used
	    ** "explicitly" as in " update tst1 s set col2=120 where exists ( select * 
	    **	    from tst1 t where s.tid!=t.tid and s.col1=t.col1) \p\g " since the
	    ** corelated subselect is forced to be the inner by convention
	    */
	    if ((   abase->opz_attnums[oattr]->opz_attnm.db_att_id
		    == 
		    DB_IMTID 
		)
		&&
		sjstate.notimptid
		&&
		/* check that this is not the case of a corelated subselect in which
		** the implicit tid is being corelated */
		!(  (subquery->ops_joinop.opj_virtual) /* check if a
						    ** subselect is in the query */
		    &&
		    ((innerc = isbtp->opn_coforw) != (OPO_CO *) isbtp)  /* check if outer list is not empty
						    */
		    &&
		    (innerc->opo_sjpr == DB_ORIG)    /* is this an original relation? */
		    &&
		    vbase->opv_rt[innerc->opo_union.opo_orig]->opv_subselect /* not NULL only
						    ** for virtual relations */
		    &&
		    (	(vbase->opv_rt[innerc->opo_union.opo_orig]->opv_grv->opv_created 
			== 
			OPS_SELECT)
			||
			(BTcount((char *)&vbase->opv_rt[innerc->opo_union.opo_orig]->
				opv_grv->opv_gsubselect->opv_subquery->
				ops_correlated, OPV_MAXVAR))
		    )
	       )
	       )

		return(OPN_SIGOK);

	}
	opn_jstats(&sjstate, jordering, &sjstate.join.stats, &sjstate.outer.stats, &sjstate.inner.stats);
    }
    /* FIXME - make sure eqsp is NULL during allocation */
    jsbtp->opn_stnext = eqsp->opn_subtree;  /* if this eqsp was already in
					    ** opn_savework then link up to
					    ** its subtree structs so that
					    ** all their co's can be
					    ** checked */

    sjstate.pagesize = opn_pagesize(global->ops_cb->ops_server,
				    eqsp->opn_relwid);
					    /* get optimum page size */
    sjstate.tuplesperblock = opn_tpblk(global->ops_cb->ops_server,
				       sjstate.pagesize, eqsp->opn_relwid); 
					    /* get estimate of tuples 
                                            ** per block in the joined 
                                            ** relation */
    {
	OPO_COST               mcost;       /* temp for current minimum co
                                            ** cost input parameter*/

	mcost = OPO_LARGEST;		    /* use "largest" cost initially so 
                                            ** opn_mincocost will do better */
	omincop = opo_mincocost (subquery, osbtp, &mcost);/* find minimum co 
                                            ** in the outer set of cost 
                                            ** orderings */
	mcost = OPO_LARGEST;
	imincop = opo_mincocost (subquery, isbtp, &mcost);/* find minimum co 
                                            ** in the inner set of cost 
                                            ** orderings */
    }

    /* *Note* These outermost loops only go through one time as things
    ** stand today, since we never hook multiple subtrees together on
    ** the "aggnext" link.  That was put in for the wacky ojfilter stuff
    ** that was never implemented.
    */
    aggosbtp = osbtp;
    do
    {
    aggisbtp = isbtp;
    do
    {
	OPO_CO                 *outercop;   /* ptr to current cost ordering
                                            ** in outer cost ordering set
                                            ** being analyzed */
	OPO_ISORT		outersort;  /* multi-attribute sort for outer */
	OPO_ISORT		innersort;  /* multi-attribute sort for inner */
	OPO_ST			*obase;      /* base of multi-attribute
					    ** ordering descriptors */
	OPE_BMEQCLS		*const_bm;  /* ptr to bitmap of constant
					    ** equivalence classes if it applies
					    ** in this case */
	bool			first_one;  /* make sure at least one plan gets
					    ** considered */

	first_one = TRUE;
	/* first_one will avoid a "no query plan found" in the case of a
	** constant join, as in the following case 
	** create view v1 as select * from t1 where t1.a1=1\g
	** create view v2 as select * from t2 where t2.a3=1\g
	** select v2.a1 from (v2 left join v1 on v1.a1=v2.a1)\p\g
	*/
	obase = subquery->ops_msort.opo_base; /* ptr to sort descriptors */
	if ((const_bm = subquery->ops_bfs.opb_bfeqc) != 0)
	{
	    bool	    needed;	    /* is there the possibility of a
					    ** useless sort being considered
					    ** if all the equivalence classes
					    ** are constant */
	    OPE_BMEQCLS	    bmeqcls;	    /* bitmap to determine intersection
					    ** with constant equivalence classes */
	    needed = FALSE;
	    if (keyjoin != OPE_NOEQCLS)
	    {
		if (keyjoin < maxeqcls)
		{
		    if (BTtest((i4)keyjoin, (char *)const_bm))
			needed = TRUE;
		}
		else
		{
		    MEcopy ((PTR)const_bm, sizeof(bmeqcls), (PTR)&bmeqcls);
		    BTand((i4)maxeqcls, (char *)obase->opo_stable[keyjoin-maxeqcls]
			->opo_bmeqcls, 	(char *)&bmeqcls);
		    if (BTnext((i4)-1, (char *)&bmeqcls, (i4)maxeqcls) >= 0)
			needed = TRUE;	    /* there is at least one constant
					    ** equivalence class in the joining
					    ** set so consider possibility of
					    ** useless sort */
		}
	    }
	    if (jordering != OPE_NOEQCLS)
	    {
		if (jordering < maxeqcls)
		{
		    if (BTtest((i4)jordering, (char *)const_bm))
			needed = TRUE;
		}
		else
		{
		    MEcopy ((PTR)const_bm, sizeof(bmeqcls), (PTR)&bmeqcls);
		    BTand((i4)maxeqcls, (char *)obase->opo_stable[jordering-maxeqcls]
			->opo_bmeqcls, 	(char *)&bmeqcls);
		    if (BTnext((i4)-1, (char *)&bmeqcls, (i4)maxeqcls) >= 0)
			needed = TRUE;	    /* there is at least one constant
					    ** equivalence class in the joining
					    ** set so consider possibility of
					    ** useless sort */
		}
	    }
	    if (!needed)
		const_bm = (OPE_BMEQCLS *)NULL; /* no possibility of a
					    ** join on a constant equivalence
					    ** class so do not consider this case */
	}
	if (keyjoin != OPE_NOEQCLS)
	{
	    /* b8410 - if the initial few attributes of a multi-attribute
	    ** keyed lookup have few unique values, then the caching effects
	    ** of the keyed lookup would be fairly good, especially if the
	    ** tuples were sorted; calculate a max for data pages touched for a keyed
	    ** lookup at this point, taking into account the non-uniqueness
	    ** of the initial attributes */
	    /* FIXME - better to estimate given original join histogram, but
	    ** this has been deleted from rlp by this point, this calculation
	    ** could be done inside oph_jselect and the selectivities for each
	    ** attribute could be saved, however this probably would also cause
	    ** problems since the outer subtree arrangement could affect duplicates
	    ** and the cost of keying at this node, so more accurate to estimate
	    ** each time at this point */
	    OPO_ISORT		*keyeqcp;   /* ptr to equivalence class of
					    ** keyed lookup being analyzed */
	    OPO_BLOCKS		dpages_touched;	    /* number of data pages available
					    ** to be touched - only valid for
					    ** keyed joins */
	    OPO_ISORT		tempkey[2]; /* used to simulate multi-attribute key */
	    OPE_BMEQCLS		*keqcmap;   /* map of multi-attribute key equivalence
					    ** classes being used */
	    OPH_HISTOGRAM	*orighistp; /* histogram for orig node */
	    OPO_TUPLES		total_unique;	/* total number of domain unique values
					    ** for attribute in orig node */
	    OPZ_IATTS		keyno;	    /* used to index key array */

	    if (keyjoin >= maxeqcls)
	    {
		OPO_SORT	*sortp;
		sortp = obase->opo_stable[keyjoin-maxeqcls];
		keyeqcp = &sortp->opo_eqlist->opo_eqorder[0]; 
		keqcmap = sortp->opo_bmeqcls;
	    }
	    else
	    {	/* simulate multi-attribute list */
		keqcmap = NULL;
		keyeqcp = &tempkey[0];
		tempkey[0]=keyjoin;
		tempkey[1]=OPE_NOEQCLS;
	    }
	    sjstate.key_nunique = 1.0;
	    dpages_touched = sjstate.maxpagestouched;
	    for (;*keyeqcp != OPE_NOEQCLS; keyeqcp++)
	    {
		OPO_TUPLES	unique_values; /* number of unique values
					    ** for keying for this attribute */
		
		orighistp = *opn_eqtamp(subquery, &krlp->opn_histogram, (*keyeqcp), TRUE);/*
						** find histogram of the 
						** relation - it must exist */
		total_unique = orighistp->oph_nunique;
		if (const_bm && BTtest((i4)(*keyeqcp), (char *)const_bm))
		{
		    unique_values = 1.0;    /* attribute is constant so
					    ** caching effects occur */
		}
		else
		{
		    OPH_HISTOGRAM   *ohistp;    /* histogram for outer */
		    ohistp = *opn_eqtamp(subquery, &orlp->opn_histogram, (*keyeqcp), TRUE);/*
					    ** find histogram of the 
					    ** relation - it must exist */
		    unique_values = ohistp->oph_nunique;
		    if (unique_values < 1.0)
			unique_values = 1.0;
		}
		if ((unique_values > total_unique)
		    ||
		    (unique_values > dpages_touched)
		    )
		    break;		    /* break if all the unique
					    ** values are covered since entire
					    ** subset of storage structure will
					    ** be scanned */
		if (total_unique > dpages_touched)
		    total_unique = dpages_touched;
		sjstate.maxpagestouched *= (unique_values/total_unique);
		dpages_touched *= (1.0/total_unique); /* estimate of number of
					** datapages which have the same
					** initial attributes */
	    }
	    for (keyno = 0; keyno < kvarp->opv_mbf.opb_count; keyno++)
	    {	/* calculate lower bound for number of data pages scanned
		** in BTREE in the case of a multi-attribute key with
		** highly duplicated initial attributes b41730 
		** - need to scan both constant equivalence classes
		** and attributes in the joining set, and when keying
		** is improved to include ranges then this needs to be
		** considered also FIXME */
		OPE_IEQCLS	keqcls;
		keqcls = abase->opz_attnums[kvarp->opv_mbf.opb_kbase->
		    opb_keyorder[keyno].opb_attno]->opz_equcls;
		if ((keqcls == keyjoin)
		    ||
		    (	keqcmap
			&&
			BTtest((i4)keqcls, (char *)keqcmap)
		    )
		    ||
		    (	const_bm 
			&& 
			BTtest((i4)keqcls, (char *)const_bm)
		    )
		    )
		{
		    orighistp = *opn_eqtamp(subquery, &krlp->opn_histogram, keqcls, TRUE);/*
						** find histogram of the 
						** relation - it must exist */
		    total_unique = orighistp->oph_nunique;
		    if (sjstate.key_nunique < krlp->opn_reltups)
			sjstate.key_nunique *= total_unique; /* calculate worst case for number
						** of unique values specified in the key */
		}
		else
		    break;
	    }
	    if (sjstate.key_nunique < krlp->opn_reltups)
	    {
		sjstate.key_reptf = krlp->opn_reltups / sjstate.key_nunique;
	    }
	    else
	    {
		sjstate.key_nunique = krlp->opn_reltups;
		sjstate.key_reptf = 1.0;
	    }
	}
        outercop = aggosbtp->opn_coforw;
	if (outercop == (OPO_CO *)osbtp)
	    opx_error(E_OP04A7_CONODE);	    /* should not happen */
	if ((subquery->ops_joinop.opj_virtual) /* check if a
					    ** subselect is in the query */
            &&
            (outercop != (OPO_CO *) aggosbtp)  /* check if outer list is not empty
                                            */
	    &&
            (outercop->opo_sjpr == DB_ORIG) /* is this an original relation? */
	    &&
	    vbase->opv_rt[outercop->opo_union.opo_orig]->opv_subselect /* not NULL only
                                            ** for virtual relations */
	    &&
	    (	(vbase->opv_rt[outercop->opo_union.opo_orig]->opv_grv->opv_created 
		== 
		OPS_SELECT)
		||
		(BTcount((char *)&vbase->opv_rt[outercop->opo_union.opo_orig]->
			opv_grv->opv_gsubselect->opv_subquery->ops_correlated, OPV_MAXVAR))
            )				    /* check if variable represents
                                            ** a OPS_SELECT, or is correlated
                                            ** and requires equivalence classes
                                            ** from the outer - if the
					    ** outer is a subselect then this
                                            ** is an illegal subtree which will
                                            ** be ignored */
	   )
		return(OPN_SIGOK);

	/* if duplicate handling semantics need to be determined
	** at this point, only allow plans in which duplicates have been
	** removed to be considered.  If all relations which have duplicates to
	** be removed are on one side and all relations with duplicates to be
	** kept but no TIDs are available on the other, then duplicates must
	** be removed at child of this node by using a sort */
	sjstate.inner.duplicates  = FALSE;
	sjstate.outer.duplicates = FALSE;
	if(subquery->ops_keep_dups)
	{
	    OPV_BMVARS		bmtemp;	    /* temp bitmap used to determine if this
                                            ** node needs to have duplicates removed
                                            */
            bool		inner_keepdups; /* TRUE if inner has at least one
                                            ** relation in which duplicates need to
                                            ** be kept, but TIDs are not available */
            bool		outer_keepdups; /* TRUE if outer has at least one
                                            ** relation in which duplicates need to
                                            ** be kept, but TIDs are not available */
	    MEcopy((PTR)subquery->ops_keep_dups, sizeof(bmtemp),
		(PTR)&bmtemp);
	    BTand((i4)BITS_IN(bmtemp), (char *)&irlp->opn_relmap, (char *)&bmtemp);
	    inner_keepdups = BTnext((i4)-1, (char *)&bmtemp, (i4)BITS_IN(bmtemp))>=0;

	    MEcopy((PTR)subquery->ops_keep_dups, sizeof(bmtemp),
		(PTR)&bmtemp);
	    BTand((i4)BITS_IN(bmtemp), (char *)&orlp->opn_relmap, (char *)&bmtemp);
	    outer_keepdups = BTnext((i4)-1, (char *)&bmtemp, (i4)BITS_IN(bmtemp))>=0;
            if ((inner_keepdups ^ outer_keepdups) & TRUE)
            {	/* either the inner or outer but not both must contain at least one
                ** relation in which duplicates can be removed, due to index substitution
                ** all relations do not need to be present, due to the search space
                ** restriction in opn_arl, there will be one node in the plan
                ** in which this assertion will be true */
		bool		outer_removedups; /* TRUE if outer has at least one relation
					    ** in which duplicates need to be kept */
		bool		inner_removedups; /* TRUE if inner has at least one relation
					    ** in which duplicates need to be kept */

		MEcopy((PTR)subquery->ops_remove_dups, sizeof(bmtemp),
		    (PTR)&bmtemp);
		BTand((i4)BITS_IN(bmtemp), (char *)&irlp->opn_relmap, (char *)&bmtemp);
		inner_removedups = BTnext((i4)-1, (char *)&bmtemp, (i4)BITS_IN(bmtemp))>=0;

		MEcopy((PTR)subquery->ops_remove_dups, sizeof(bmtemp),
		    (PTR)&bmtemp);
		BTand((i4)BITS_IN(bmtemp), (char *)&orlp->opn_relmap, (char *)&bmtemp);
		outer_removedups = BTnext((i4)-1, (char *)&bmtemp, (i4)BITS_IN(bmtemp))>=0;
		if (((inner_removedups ^ outer_removedups) & TRUE)
		    &&
		    (inner_removedups != inner_keepdups))
		{   /* either the inner has relations with duplicates to be removed and
                    ** the outer has relations with duplicates to be kept, but no TIDs
                    ** or visa versa */
		    sjstate.inner.duplicates = inner_removedups;
		    sjstate.outer.duplicates = outer_removedups;
		}
	    }
	}

	if (subquery->ops_duplicates == OPS_DKEEP)
	{
	    sjstate.existence_only = FALSE; /* only can use existence_only if 
					** duplicates are not important */
	    sjstate.exist_factor = 1.0;	/* existence_only check does not
					** apply */
	    sjstate.exist_bool = FALSE;	/* existence only check does not apply */
	}
	else
	{
	    sjstate.existence_only = BTsubset((char *)&eqsp->opn_maps.opo_eqcmap,
		(char *)&outercop->opo_maps->opo_eqcmap, (i4)maxeqcls);
	    if (sjstate.existence_only)
	    {
		sjstate.exist_factor = 0.5; /* existence_only check applies */
		sjstate.exist_bool = (rlp->opn_boolfact == TRUE); /* if no
					** boolean factors exist then match will
					** succeed or fail on first join attempt */
		if (sjstate.join.tuples > outercop->opo_cost.opo_tups)
		{
		    sjstate.join.tuples = outercop->opo_cost.opo_tups; /* part of bug
					    ** fix for b33279, make sure that if
					    ** existence check is made, that the
					    ** tuple count of the outer is not
					    ** exceeded */
		    sjstate.blocks = outercop->opo_cost.opo_tups / sjstate.tuplesperblock;
		}
	    }
	    else
	    {
		sjstate.exist_factor = 1.0;	/* existence_only check does not
					** apply */
		sjstate.exist_bool = FALSE;	/* existence only check does not apply */
	    }
	}
	    
	outersort = opo_fordering(subquery, &oeqsp->opn_maps.opo_eqcmap); /* create
					** an ordering for all the attributes
					** of the outer, since it can be
					** used for flattening of aggregates
					** and subselects */
	innersort = opo_fordering(subquery, &ieqsp->opn_maps.opo_eqcmap); /* create
					** an ordering for all the attributes
					** of the inner, since it can be
					** used for flattening of aggregates
					** and subselects */
        for (;outercop != (OPO_CO *) aggosbtp;
             outercop = outercop->opo_coforw)
	{   /* look at every cost ordering in the cost ordering set of the 
            ** outer relation */
	    if (outercop->opo_sjpr == DB_ORIG)
		continue;		/* do not look at ORIG nodes for outer
					** but instead use PR nodes for this
					** case */
	    if (!(sjstate.outer.sort = (outercop == omincop))
		&&
		jordering != OPE_NOEQCLS) 
	    {	/* use the minimum cost ordering as a candidate to
		** reformat, OR if a jordering is specified then
		** at least the outercop first eqcls needs to have an ordering which
		** is compatible with the jordering to be considered at all */
		if (outercop->opo_ordeqc == OPE_NOEQCLS)
		    continue;
                sjstate.new_ordeqc = opo_combine(
                    subquery,
                    jordering,  /* join on the eqcls in this set */
                    outercop->opo_ordeqc, jordering, /* check if outer ordering
                            ** is compatible with joining equivalence classes */
                    &eqsp->opn_maps.opo_eqcmap, /* not important for this test */
                    &sjstate.new_jordering); /* if orderings are compatible
                            ** then at least one equivalence class will
                            ** be int this set */
                if (sjstate.new_jordering < 0)
                    continue;
	    }
	    if(	!outercop->opo_deleteflag  /* is it not deleteable */
		&&
		(   opn_cost(subquery, outercop->opo_cost.opo_dio, 
			     outercop->opo_cost.opo_cpu) 
		    < 
		    subquery->ops_cost	/* is cost less than minimum so far */
		)
              )
	    while (TRUE)		    /* execute loop twice for sjstate.outer.sort
					    ** being available */
	    {
		/* if we are dealing with the original relation then we
		** want the unique, complete, nuniq and reptf info for
		** the relation before restriction so that sjcost can
		** do its calculations correctly.  bug 1501 */
		OPO_CO                 *innercop;/* ptr to current cost ordering
                                            ** in inner cost ordering set
                                            ** being analyzed */
		bool			dummy; /* used for duplicate removal
                                            ** verification */    
		OPO_ISORT		outer_ordering; /* this is the ordering to be
					    ** used by the outer */
		if (subquery->ops_mask & OPS_BYPOSITION)
		    sjstate.outer.sort = FALSE;	/* do not consider sort nodes on the
					    ** outer since this will prevent any
					    ** by position updates */
		if (sjstate.outer.sort
		    &&
		    (	(outersort != OPE_NOEQCLS)
			||
			sjstate.outer.duplicates
		    ))
		    outer_ordering = outersort; /* use the ordering for the
					    ** sort node */
		else
		{
		    /* note that the outer ordeqc may not be sorted if the
		    ** ordering is obtained from ISAM but it may be mostly
		    ** sorted as in a partial sort merge, therefore consider
		    ** this ordering as part of the search space, this ordering
		    ** will be guaranteed sorted if "opo_storage == DB_SORT_STORE" */
		    outer_ordering = outercop->opo_ordeqc;
		    sjstate.outer.sort = FALSE;  /* execute while loop once more
					    ** without sort node
					    ** on top of outer minimum */
		    if (sjstate.outer.duplicates && !opn_dremoved(subquery, outercop, &dummy))
			/* need to verify that the outer subtree has duplicates
			** removed in order to be considered */
			break;
		}
		if (!sjstate.outer.allowed
		    &&
		    (	!sjstate.outer.duplicates
			|| 
			opn_dremoved(subquery, outercop, &dummy)
		    ))
		{
		    sjstate.outer.sort = FALSE;	/* do not consider sort node since
					    ** it is outside the server startup limits
					    ** unless it is required for semantics */
		    subquery->ops_mask |= OPS_TMAX;
		}
                for (innercop = aggisbtp->opn_coforw;
                     innercop != (OPO_CO *) aggisbtp;
                     innercop = innercop->opo_coforw)
		{
		    sjstate.inner.sort = FALSE;
		    /* Only look at valid inners.
		    ** Look at ORIG inners (for keying possibilities).  However
		    ** ORIG inners aren't sortable (inner.sort false).
		    ** PR/SJ inners must be not-implicit-tid unless we need
		    ** a right join, and we only look at the least expensive
		    ** PR/SJ inner (first sorting, then not.)
		    ** Notice that imincop (and omincop) will not point
		    ** to an ORIG.
		    **
		    ** (below is an historical comment, is it true?)
		    ** The right-join exception is because implicit-tid
		    ** joins are normally done on the ORIG (T-join) but there
		    ** are no right T-joins, so try it with an SJ.
		    */
		    if 
		    (
			/*static*/opn_goodco (subquery, innercop) 
			&&
			(
			    innercop->opo_sjpr == DB_ORIG 
			    || 
			    (
				(   sjstate.notimptid
				    ||
				    (sjstate.opn_ojtype == OPL_RIGHTJOIN) /*
					    ** since right tid joins are not
					    ** supported, allow this access
					    ** path to allow at least one
					    ** query plan i.e. FSM right join */
				)
				&&
				(sjstate.inner.sort = (innercop == imincop))
			    )
			)
		    )
    
		    while (TRUE)
		    {	/* execute twice for case of sort node on inner */
			OPO_ISORT   inner_ordering; /* ordering to be used on the
					    ** inner */
			bool	    corelated;	    /* TRUE if inner is corelated */
			bool	    skip_sort; /* TRUE if sorted case should be skipped
					    ** at this point since it only deals with
					    ** constant equivalence classes or it is
					    ** a cartesean product */
# ifdef xNTR1
			if (tTf(23, 1))
			    TRdisplay("outercop:%p innercop:%p\n", outercop, innercop);
# endif
			corelated = FALSE;
			if (sjstate.inner.sort
			    &&
			    (	(innersort != OPE_NOEQCLS)
				||
				sjstate.inner.duplicates
			    ))
			    inner_ordering = innersort; /* use the ordering for the
					    ** sort node, do not consider sort if no
					    ** attributes are available unless duplicate
					    ** removal is required for semantic reasons */
			else 
			{
			    /* this is the minimum inner tree being considered
			    ** which is not an ORIG node, so need to check if
			    ** it is sorted, i.e. a project-restrict may be
			    ** ordered on ISAM keys, but is not sorted */
			    if (innercop->opo_storage == DB_SORT_STORE)
				inner_ordering = innercop->opo_ordeqc;
			    else
			    {
				OPV_GSUBSELECT	*corel_sub;
				if ((innercop->opo_sjpr == DB_ORIG)
				    &&
				    (corel_sub = vbase->opv_rt[innercop->opo_union.opo_orig]
					->opv_grv->opv_gsubselect)
				    && 
				    BTcount((char *)&corel_sub->opv_subquery->
					ops_correlated, OPV_MAXVAR))
				{
				    corelated = TRUE;
				    inner_ordering = jordering; /* for a corelated subquery the
						** joining equivalence classes are the
						** corelated equivalence classes */
				}
				else
				    inner_ordering = OPE_NOEQCLS;
			    }
			    sjstate.inner.sort = FALSE; /* execute loop 
					** without sort
					** node on top of minimum */
			    if (sjstate.inner.duplicates && !opn_dremoved(subquery, innercop, &dummy))
			    /* need to verify that the inner subtree has duplicates
			    ** removed in order to be considered */
				break;
			}
			if (!sjstate.inner.allowed)
			{
			    OPV_GRV	*grvp;
			    if (!sjstate.inner.duplicates
				|| 
				opn_dremoved(subquery, innercop, &dummy)
				)
			    {
				sjstate.inner.sort = FALSE; /* do not consider sort node since
						    ** it is outside the server startup limits
						    ** unless it is required for semantics */
				subquery->ops_mask |= OPS_TMAX; /* mark search space as being
						    ** reduced */
			    }
			    if ((innercop->opo_sjpr != DB_ORIG)
				 &&
				 (  (innercop->opo_sjpr != DB_PR)
				    ||
				    (	(grvp = vbase->opv_rt[innercop->opo_outer->opo_union.opo_orig]->opv_grv)
					&&
				        (grvp->opv_created != OPS_VIEW)
					&&
					(grvp->opv_created != OPS_UNION)
				    )
				 )
				)
			    {
				subquery->ops_mask |= OPS_TMAX; /* mark search space
						    ** as being reduced */
				break;		    /* inner will create a hold file temporary
						    ** so avoid this if temp file limits have
						    ** been exceeded, except in case of view
						    ** or union view, which always have a PR
						    ** since keying is not allowed on this
						    ** structure */
			    }
			}

/*FIXME - part of duplicate removal in sort solution, if duplicates
      removed then adjust number of tuples estimated
	**		tuples *= ((outercop->opo_cost.opo_tups *
	**			  innercop->opo_cost.opo_tups)/
	**			 (irlp->opn_reltups * orlp->opn_reltups));
	**				** adjust tuple count by factor
	**				** to take into account removal
	**				** of duplicates by sort node 
*/
			{
			    OPO_ISORT	joinordering;	/* this is the
					    ** multi-attribute ordering which
					    ** will be used, or a subset of this
					    ** ordering will be used for the join */
			    if ((innercop->opo_sjpr == DB_ORIG) && (!corelated))
			    {
				joinordering = keyjoin; /* for a keyed join the
						** outer does not need to be
						** sorted for the key to be
						** applied */
				inner_ordering = keyjoin; /* assume this inner
						** ordering exists */
			    }
			    else if ((global->ops_cb->ops_check
				    &&
				    opt_strace( global->ops_cb, OPT_F049_KEYING)
				    || 
				    kjonly 
				    && 
				    (subquery->ops_mask2 & OPS_HINTPASS))
				    &&
				    (keyjoin != OPE_NOEQCLS)
				    ||
				    spjoin)
				    break;  /* do not look at non-keying plans if
					    ** keying trace point is used or this
					    ** is a spatial join or there's a
					    ** relevant KEYJ hint and we're in
					    ** the hint pass of enumeration */
			    else
				joinordering = jordering;
			    sjstate.new_ordeqc = /* return the ordering of the
					    ** intermediate result */
			      opo_combine(subquery,
				joinordering,  /* join on the eqcls in this set */
				outer_ordering, inner_ordering, /* assume the
					    ** respective outer and inner orderings
					    */
				&eqsp->opn_maps.opo_eqcmap, /* make sure the returned
					    ** new_ordeqc ordering of the 
					    ** intermediate result only uses
					    ** eqcls from this set */
				&sjstate.new_jordering); /* the multi-attribute
					    ** join which can be performed using
					    ** the inner and outer orderings
					    ** - this ordering must be a subset
					    ** of jordering */
                            sjstate.partial_sort = (sjstate.new_jordering >= 0); /*
                                            ** TRUE - if there is some useful sortedness
                                            ** for the outer ordering */
			}
			if ((innercop->opo_sjpr == DB_ORIG) && (!corelated))
			{
			    skip_sort = sjstate.outer.sort
				&&
				!sjstate.outer.duplicates
				&& 
				(sjstate.new_jordering == OPE_NOEQCLS);
					    /* this outer sort is useless for this key join
					    ** so skip this plan, i.e. the sort can only
					    ** be used by the parent, but it will always
					    ** be better to let the parent sort since the
					    ** number of rows and columns will be smaller */
			    sjstate.new_jordering = keyjoin; /* for a keyed join the
					    ** outer does not need to be
					    ** sorted for the key to be
					    ** applied */
                            if (!skip_sort && const_bm && (sjstate.new_jordering != OPE_NOEQCLS))
                            {               /* a constant equivalence class exists
                                            ** so the joining equivalence class
                                            ** could be a complete subset of this
                                            ** so there would be no reason to
                                            ** sort at this point */
                                /* this test only needs to be made for a key join since
                                ** opo_combine will not return constant equivalence classes
                                ** in the join ordering , FIXME it is better to do this
                                ** test before enumeration and set a flag in the var descriptor
                                ** to be tested here */
                                if (sjstate.inner.sort
                                    &&
                                    !sjstate.inner.duplicates) /* a sort may be required
                                                    ** for special duplicate handling */
                                {   /* avoid considering sort node if the join is on
                                    ** constant equivalence classes only */
                                    if (sjstate.new_jordering < maxeqcls)
                                        skip_sort = BTtest((i4)sjstate.new_jordering, (char *)const_bm);
                                    else
                                        skip_sort = BTsubset((char *)subquery->ops_msort.opo_base->opo_stable
                                                [sjstate.new_jordering-maxeqcls]->opo_bmeqcls,
                                                (char *)const_bm, (i4)maxeqcls); /* do not use obase here
                                                    ** since new orderings could have been added */
                                }
                                if (sjstate.outer.sort
                                    &&
                                    !sjstate.outer.duplicates) /* a sort may be required
                                                    ** for special duplicate handling */
                                {   /* avoid considering sort node if the join is on
                                    ** constant equivalence classes only */
                                    if (sjstate.new_jordering < maxeqcls)
                                        skip_sort = BTtest((i4)sjstate.new_jordering, (char *)const_bm);
                                    else
                                        skip_sort = BTsubset((char *)subquery->ops_msort.opo_base->opo_stable
                                                [sjstate.new_jordering-maxeqcls]->opo_bmeqcls,
                                                (char *)const_bm, (i4)maxeqcls);/* do not use obase here
                                                    ** since new orderings could have been added */
                                }
                            }
			}
			else if ((sjstate.new_jordering == OPE_NOEQCLS)
			    &&
			    (	(sjstate.inner.sort && !sjstate.inner.duplicates)
				|| 
				(sjstate.outer.sort && !sjstate.outer.duplicates)
			    ))
			    skip_sort = TRUE; /* do not consider useless sort node when
					** a cartesean product exists, FIXME - it may
					** be cheaper to sort the outer, rather than the
					** result of a cartesean product, to provide data
					** sorted for the parent */
			else
			    skip_sort = FALSE;
			if (!skip_sort
			    &&
			    (	(sjstate.new_jordering != OPE_NOEQCLS) 
				||
				(jordering == OPE_NOEQCLS)
				||
				(
				    first_one
				    &&
				    (
					(innercop->opo_sjpr == DB_ORIG)
					||
					((innercop == imincop) && !sjstate.inner.sort)
				    )
				)
				||
				!sjstate.inner.allowed
				||
				!sjstate.outer.allowed /* allow cart prods if temporaries
						** are to be avoided */
			    ) 
					/* consider min co but without a
					** sort node, if no joining attributes */
			    &&
			    (	/* avoid Pjoin in 6.5 since this is not supported */
				(sjstate.new_jordering != OPE_NOEQCLS) 
				||
				(innercop->opo_sjpr != DB_ORIG)
				||
				(   vbase->opv_rt[innercop->opo_union.opo_orig]
					->opv_grv->opv_created
				    == 
				    OPS_SELECT /* allow SEjoin only */
				)
			    )
			    )	
			{		/* do not create join node if the
					** orderings cannot be used, and
					** a joining eqcls exists */
			    first_one = FALSE;	/* avoid cartesean products
					    ** unless join nodes degenerate
					    ** to constants */
			    opn_sjcost(&sjstate, outercop, innercop); /*
					    ** calculate the cost of the
					    ** join given this outer/inner
					    ** pair */
			    /* increment pointer count to prevent this CO node from
			    ** being deleted, it is probably not needed due to the
			    ** bug fix for ... but it is safer to do this in case
			    ** there are any other holes in the memory management
			    ** routines */
			    outercop->opo_pointercnt++;
			    innercop->opo_pointercnt++;
			    if ((!outercop->opo_pointercnt)
				 ||
				 (!innercop->opo_pointercnt)
				)
				opx_error(E_OP04A8_USAGEOVFL); /* report error
					    ** if usage count overflows */
			    else
			    {
				OPN_STATUS	sigstat;

				sigstat = opn_corput(&sjstate, outercop, 
						innercop, orlp, irlp, rlp); 
					    /* create a new CO node to describe this
					    ** new intermediate,... only process if
					    ** no overflow occurs otherwise memory
					    ** management problems may happen */
				if (sigstat == OPN_SIGEXIT) return(sigstat);
			    }
			    outercop->opo_pointercnt--;
			    innercop->opo_pointercnt--;
			}
			if (sjstate.inner.sort && !mixedcoll)
			    sjstate.inner.sort = FALSE;
			else
			    break;
		    }
		}
		if (sjstate.outer.sort && !mixedcoll)
		    sjstate.outer.sort = FALSE;
		else
		    break;
	    }
	}
	aggisbtp = aggisbtp->opn_aggnext;
    } while (aggisbtp != isbtp);
    aggosbtp = aggosbtp->opn_aggnext;
    } while (aggosbtp != osbtp);

    /* If this is existence_only join, any histograms built from the 
    ** join must be adjusted to reflect modified join cardinality.
    ** This is only done if the new tuple estimate differs from the 
    ** old by at least 10%. */
    if (sjstate.existence_only && sjstate.join.tuples < 0.90 * tuples)
    {
	OPH_HISTOGRAM	*histp;
	OPO_TUPLES	oldreptf, oldnunique;
	OPN_PERCENT	factor;
	i4		i;
	OPZ_ATTS	*attrp;

	/* Loop over histograms contained in join result. Determine 
	** new "number of unique values" estimate for whole histogram,
	** compute modified repetition factor for whole histogram and
	** pro-rate per-cell rep factors by same ratio. */

	for (histp = rlp->opn_histogram; histp; histp = histp->oph_next)
	{
	    if (histp->oph_mask & OPH_COMPOSITE) continue;
	    oldnunique = histp->oph_nunique;
	    oldreptf = histp->oph_reptf;
	    histp->oph_nunique = opn_eprime(sjstate.join.tuples, oldreptf,
		oldnunique, subquery->ops_global->ops_cb->ops_server->
		opg_lnexp);
	    if (histp->oph_nunique < 1.0) histp->oph_nunique = 1.0;

	    histp->oph_reptf = sjstate.join.tuples / histp->oph_nunique;
	    if ((0.95 * oldreptf < histp->oph_reptf || 
		1.05 * oldreptf > histp->oph_reptf) && oldreptf != 0.0)
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums
			[histp->oph_attribute];
		factor = histp->oph_reptf / oldreptf;
		for (i = 1; i < attrp->opz_histogram.oph_numcells; i++)
		    histp->oph_creptf[i] *= factor;
	    }
	}
    }

    return(OPN_SIGOK);		/* replace old SIGNAL based return */

}
