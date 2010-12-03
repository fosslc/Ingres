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
#define        OPN_PRCOST      TRUE
#include    <bt.h>
#include    <me.h>
#include    <opxlint.h>

/**
**
**  Name: OPNPRCOST.C - calculate cost of doing a project restrict
**
**  Description:
**      Routines to calculate the cost of doing project restrict
**
**  History:
**      28-may-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      06-nov-89 (fred)
**          Added support for non-histogrammable datatypes.
**      21-may-93 (ed)
**          - changed opo_var to be a union structure so that
**          the outer join ID can be stored in interior CO nodes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      12-oct-2000 (stial01)
**          Removed commented call to opn_sfdircost
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static i4 opn_vlcnt(
	OPB_BFVALLIST *vp);
static PTR oph_dmin(
	OPS_SUBQUERY *subquery,
	OPZ_ATTS *attrp);
static PTR oph_dmax(
	OPS_SUBQUERY *subquery,
	OPZ_ATTS *attrp);
OPO_BLOCKS opn_readahead(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
void opn_prcost(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPB_SARG *sargtype,
	OPO_BLOCKS *pblk,
	OPH_HISTOGRAM **hpp,
	bool *imtid,
	bool order_only,
	OPO_ISORT *ordering,
	bool *pt_valid,
	OPO_BLOCKS *dio,
	OPO_CPU *cpu);

/*{
** Name: opn_vlcnt	- count number of key values in list
**
** Description:
**      Count the number of key values in the list
**
** Inputs:
**      vp                              ptr to beginning of keylist
**
** Outputs:
**	Returns:
**	    number of keys in keylist
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
static i4
opn_vlcnt(
	OPB_BFVALLIST      *vp)
{
    i4                  count;

    /* count number of keys in list (equal to number of OR's in boolean factor
    ** plus one)
    */
    for (count = 0; vp; count++, vp = vp->opb_next)
	;

    return (count);
}

/*{
** Name: oph_dmin	- get minimum histogram value
**
** Description:
**      routine will return a ptr to minimum histogram value for the
**      attribute
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      attrp                           ptr to joinop attribute element for
**                                      which minimum histogram element is
**                                      required
**
** Outputs:
**	Returns:
**	    PTR to minimum histogram element
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
static PTR
oph_dmin(
	OPS_SUBQUERY       *subquery,
	OPZ_ATTS           *attrp)
{
    DB_STATUS	       status;	    /* ADF return status */
    i4		       dt_bits;

    /* allocate memory for histogram element */
    if (!attrp->opz_histogram.oph_dataval.db_data)
	attrp->opz_histogram.oph_dataval.db_data = 
            opn_memory( subquery->ops_global, 
	    (i4)attrp->opz_histogram.oph_dataval.db_length );
    status = adi_dtinfo(subquery->ops_global->ops_adfcb,
			attrp->opz_dataval.db_datatype,
			&dt_bits);
    if ((status == E_DB_OK) && (dt_bits & AD_NOHISTOGRAM))
    {
	MEcopy(OPH_NH_HMIN, OPH_NH_LENGTH,
	    attrp->opz_histogram.oph_dataval.db_data);
    }
    else
    {
	status = adc_hmin(subquery->ops_global->ops_adfcb,
		     &attrp->opz_dataval,
		     &attrp->opz_histogram.oph_dataval);
    }

# ifdef E_OP0789_ADC_HMIN
    if (status != E_DB_OK)
	opx_verror( status, E_OP0789_ADC_HMIN,
	    subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
# endif
    attrp->opz_histogram.oph_mindomain = 
	attrp->opz_histogram.oph_dataval.db_data; /* save minimum histogram
					** value */
    attrp->opz_histogram.oph_dataval.db_data = NULL; /* reset ptr so it is
					** not used for other purposes */
    return (attrp->opz_histogram.oph_mindomain);
}

/*{
** Name: oph_dmax	- get maximum histogram value
**
** Description:
**      routine will return a ptr to maximum histogram value for the
**      attribute
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      attrp                           ptr to joinop attribute element for
**                                      which maximum histogram element is
**                                      required
**
** Outputs:
**	Returns:
**	    PTR to maximum histogram element
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
static PTR
oph_dmax(
	OPS_SUBQUERY       *subquery,
	OPZ_ATTS           *attrp)
{
    DB_STATUS		status;	    /* ADF return status */
    i4			dt_bits;
    
    /* allocate memory for histogram element */
    if (!attrp->opz_histogram.oph_dataval.db_data)
	attrp->opz_histogram.oph_dataval.db_data 
	    = opn_memory( subquery->ops_global, 
	                  (i4)attrp->opz_histogram.oph_dataval.db_length );

    /* save max ptr */
    status = adi_dtinfo(subquery->ops_global->ops_adfcb,
			attrp->opz_dataval.db_datatype,
			&dt_bits);
    if ((status == E_DB_OK) && (dt_bits & AD_NOHISTOGRAM))
    {
	MEcopy(OPH_NH_HMAX, OPH_NH_LENGTH,
	    attrp->opz_histogram.oph_dataval.db_data);
    }
    else
    {
	status = adc_hmax(subquery->ops_global->ops_adfcb, 
		     &attrp->opz_dataval,
		     &attrp->opz_histogram.oph_dataval);
    }
# ifdef E_OP078A_ADC_HMAX
    if (status != E_DB_OK)
	opx_verror( status, E_OP078A_ADC_HMAX,
	    subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
# endif
    attrp->opz_histogram.oph_maxdomain = 
	attrp->opz_histogram.oph_dataval.db_data;
    attrp->opz_histogram.oph_dataval.db_data = NULL;
    return (attrp->opz_histogram.oph_maxdomain);
}

/*{
** Name: opn_readahead	- make rough estimate of group read ahead cost
**
** Description:
**      Given a storage structure make a rough estimate of the reduction
**	in DIO estimates as a result of group reads.  Quite a bit of
**	DIO cost is a result of the system call overhead, so that if a group
**	of pages is requested, the cost is less than reading each of the
**	pages individually
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             CO orig node of variable to be keyed
**
** Outputs:
**
**	Returns:
**	    readahead factor
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-apr-93 (ed)
**          initial creation for bug 47104 and 47733
**	31-jan-94 (rganski)
**	    Ported from 6.4 to 6.5. Changed cop->opo_var to
**	    cop->opo_union.opo_orig, and changed formula for isam and hash to
**	    use rdr_info->rdr_rel->tbl_opage_count instead of
**	    (totalpages - relprim). 
**	06-mar-96 (nanpr01)
**	    Use appropriate scan block factors.
**	17-jun-96 (inkdo01)
**	    Treat Rtree like Btree in readahead computation.
**	16-nov-98 (inkdo01)
**	    No readahead contribution for Btree primaries which read 
**	    index pages only.
[@history_template@]...
*/
OPO_BLOCKS
opn_readahead(subquery, cop)
OPS_SUBQUERY        *subquery;
OPO_CO		    *cop;
{
    RDR_INFO	    *rdr_info;
    OPO_BLOCKS	    totalpages;
    OPO_BLOCKS	    holdfactor;
    i4	    pg_size;
    i4		    i;
    OPV_IVARS	    varno;

    /* effectiveness of scanning is determined by storage structure,
    ** - adjust the scan factor to reflect the average scan costs */
    /* - calculate average number of effective blocks scanned on
    ** a storage structure */

    /* But first, if this is a Btree primary, and we're only going to 
    ** read the index leaf pages, return holdfactor of 1.0. This is because
    ** we turn off readahead in this circumstance. */

/*  varno = BTnext((i4)-1, (char *)cop->opo_variant.opo_local->opo_bmvars,
		(i4)sizeof(OPV_BMVARS)); */
    varno = cop->opo_union.opo_orig;
    if (varno >= 0 && subquery->ops_vars.opv_base->opv_rt[varno]->opv_mask &
		OPV_BTSSUB) return(1.0);

    for (i = 0, pg_size = DB_MINPAGESIZE; i < DB_NO_OF_POOL; 
	 i++, pg_size *= 2)
      if (pg_size == cop->opo_cost.opo_pagesize)
	break;
    holdfactor = subquery->ops_global->ops_estate.opn_sblock[i];
					/* number of blocks read per
					** DIO during scan */
    switch(cop->opo_storage)
    {
    case DB_HEAP_STORE:
    case DB_SORT_STORE:
	/* no index pages, or randomly fixed pages in scan, so scan
	** cost close to hold file cost */
	break;
    case DB_BTRE_STORE:
    case DB_RTRE_STORE:
	holdfactor = holdfactor/2.0; /* since index pages are dispersed
				    ** among data pages, and DMF stops
				    ** a group scan if it already finds
				    ** the page in memory, chances are that
				    ** index pages will effectively reduce
				    ** the read-ahead advantages */
	break;
    case DB_ISAM_STORE:
    case DB_HASH_STORE:
    {
	/* overflow buckets are scanned one page at a time for hash
	** so scanning costs are "group scans" for main pages and
	** single page scans for non-main pages */
	OPO_BLOCKS		relprim;

	rdr_info = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]
	    ->opv_grv->opv_relation;
	totalpages = rdr_info->rdr_rel->tbl_page_count;  /* total pages in hash
							 */
	relprim = cop->opo_cost.opo_cvar.opo_relprim;  /* data pages in HASH
					    ** OR data + index pages for ISAM*/
#if 0
	if (relprim > 1.0)
	    holdfactor = totalpages / 
		((relprim / holdfactor) + (totalpages - relprim)); 
#endif
	if (relprim > 1.0)
	    holdfactor = totalpages / 
		((relprim / holdfactor) + rdr_info->rdr_rel->tbl_opage_count);
				    /* datapages/holdfactor, gives number of
				    ** I/Os needed to scan the main hash
				    ** pages;  but DMF does single page
				    ** lookups on overflow pages so add
				    ** tbl_opage_count */
				    /* tbl_ipage_count is always 0 for hash
				    ** so to calculate the overflow pages
				    ** need to subtract from total pages */
	break;
    }
    }
    if (holdfactor < 1.0)
	holdfactor = 1.0;
    return(holdfactor);
}

/*{
** Name: opn_prcost	- calculate the cost of doing a project and restrict
**
** Description:
**      Take into consideration the storage structure of the relation 
**      FIXME - only the first attribute of a multi-attribute key is used
**      for consideration of pagestouched, need to look at all histograms
**      and sub-intervals of OR's in order to model pagestouched more
**      accurately.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      cop                             ptr to cost ordering node to evaluate
**      sel                             selectivity - used to more accurately
**			                estimate if a single value spans 
**                                      many primary pages.  "sel" is modified
**                                      in cases where it has other preds 
**                                      factored into it and therefore is 
**                                      still too small.
**      order_only                      TRUE - if only *ordering needs to be
**					returned
**
** Outputs:
**      ordering                        this is the ordering which can be
**					used to describe the sortedness of
**                                      the data after the keying has been
**                                      done
**      sargtype                        return 1 here if equality sarg found
**					for hash or isam rel. return 2 if
**					inequality sarg found for isam.
**					otherwise return 5.
**      pblk                            number of blocks read of relation,
**					exclusive of directory searches
**      hpp                             histogram of attribute being keyed
**                                      is returned - only if an exact key
**                                      is found.
**      pt_valid                        TRUE - if the pblk parameter can
**                                      be used as an accurate estimate of
**                                      the number of pages touched in the
**                                      base relation (used for locking)
**	Returns:
**	    estimated number of pages read of relation, including directory
**          searches
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-may-86 (seputis)
**          initial creation
**	21-nov-86 (seputis)
**          change scan of relation to use
**          page size of group level scan
**	20-jan-89 (seputis)
**          exact multi-attribute key bug fix 4542
**	15-aug-89 (seputis)
**	    added dio and cpu parameters to more accurately allow
**	    cpu cost to be calculated, (partial improvement to
**	    low cost of table scan problem)
**	19-dec-89 (seputis)
**	    - add support for op251 tracing flag
**	16-may-90 (seputis)
**	    - b21483, b21485 calculate costs more accurately as per Derek F. suggestions
**	    on DMF
**	5-jun-90 (seputis)
**	    - b30397 - return *pblk for correct pagestouched for locking estimate
**	12-jun-90 (seputis)
**	    - b30278 - use primary key when all things are equal
**	16-jul-90 (seputis)
**	    - fix sequent floating point violation
**	14-aug-90 (seputis)
**	    - added check for global key since as in the dun and brad 
**	    benchmark, multiple key ranges can overlap causing a
**	    nokey boolean factor.
**	31-oct-90 (seputis)
**	    - fix for b34056, multi-attribute exact key problem
**	5-dec-90 (seputis)
**	    - added code to make better estimates of multi-attribute
**	    project restricts
**	8-apr-91 (seputis)
**	    - added support for OPF range and exact startup flags
**	22-apr-91 (seputis)
**	    - corrected problems with improved keying estimate enhancements
**	    for 6.4
**	11-jan-92 (seputis)
**	    - cleanup of routine, fix bug 40923, remove old calculation
**	    of project-restrict keying costs and assume new histogramming
**	    processing numbers in keysel,... take sel out of parameter
**	    list.. the sel parameter included the entire selectivity
**	    rather than just the keying selectivity which made the
**	    cost estimates too cheap in some cases... more cleanup
**	    of this routine is needed to eliminate some calculations
**	    which are ignored FIXME.
**	21-jan-92 (seputis)
**	    - changed ordering to return full ordering, not just boolean
**	    factors since it can be used in top sort node removal for a
**	    btree, as well as cached keying strategies
**	09-nov-92 (rganski)
**	    Simplified computation of percentage of rows satisfying predicate, 
**	    even though this code is now ifdef'd out. Had it all figured out 
**	    anyway, so if it's ever needed again, it's there. It uses the 
**	    assumption that the histogram contains the key values as boundary 
**	    values (they were inserted by oph_catalog()), so a cell is either 0% 
**	    or 100% in the range. This makes it unnecessary to call opn_diff()
**	    and compute fractions of selectivities.
**	    Changed parameters to remaining call to opn_diff(), 
**	    which is for integer type only.
**	14-dec-92 (rganski)
**	    Added datatype parameter to call to opn_diff().
**      5-jan-93 (ed)
**          - bug 48803 do not return an exact match type for a multi-attribute
**          key unless all elements of the key are exact match
**          - bug 48049 do not return a constant eqcls in an ordering for a
**          project restrict node
**	15-apr-93 (ed)
**	    - bug 47137 - fix float overflow problem, no query plan found
**      31-jan-94 (markm)
**          - bug 58140, removed redundant code for calculating selfactor
**          if params are not available for repeated queries - this is not
**          the case since we use the params which are passed on the first
**          execution.  This code was being executed for database procedures
**          that have novalue params being passed - selectivity was being
**          calculated twice - resulting in a low DIO/CPU estimate.
**	31-jan-94 (rganski)
**	    Ported fix for b47104 and b47733 (key join bug) from 6.4 to 6.5.
**	    This includes moving a block of code to new function
**	    opn_readahead() (above).
**	14-feb-94 (ed)
**	    - bug 58140 - fix uninitialize variable problem which could cause
**	    access violations 
**	25-apr-94 (rganski)
**	    b59982 - added histdt parameter to calls to opb_bfkget(). See
**	    comment in opb_bfkget() for more details.
**	26-mar-96 (inkdo01)
**	    Minor bug fix - incorrectly set RANGEKEY.
**	13-feb-04 (inkdo01)
**	    Changes to support 8 byte TIDs for partitioned tables.
[@history_line@]...
*/
VOID
opn_prcost(
	OPS_SUBQUERY       *subquery,
	OPO_CO             *cop,
	OPB_SARG           *sargtype,
	OPO_BLOCKS	   *pblk,
	OPH_HISTOGRAM      **hpp,
	bool		   *imtid,
	bool               order_only,
	OPO_ISORT          *ordering,
	bool               *pt_valid,
	OPO_BLOCKS	   *dio,
	OPO_CPU		   *cpu)
{
    OPO_DBLOCKS         readall;	/* number of disk blocks in relation
                                        ** = number of disk blocks read if
                                        ** entire relation is scanned */
    OPV_VARS            *rp;            /* joinop range variable element
                                        ** associated with this leaf */
    OPB_BFKEYINFO       *bfkeyp;	/* ptr to keylist header for boolean
                                        ** factor used to build key */
    OPZ_ATTS            *ap;		/* ptr to joinop attribute element
                                        ** upon which the relation is ordered*/
    OPO_BLOCKS		holdfactor;     /* pages per scan I/O */
    OPS_STATE           *global;	/* ptr to global state variable */
    bool		dont_skip;	/* TRUE if sargable boolean which are
					** simple, in that they do not select
					** the min and max of the key */
    OPN_PERCENT		sel;

    global = subquery->ops_global;  /* get ptr to global state variable */

    /* initialize return parameters */
    *hpp = NULL;			/* no histogram found */
    *ordering = OPE_NOEQCLS;		/* no ordering initially */
    rp = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]; /*
					** joinop range variable associated
					** with this leaf */
    holdfactor = opn_readahead(subquery, cop);
    *pblk = cop->opo_cost.opo_reltotb;	/* number of blocks in relation */
    readall = cop->opo_cost.opo_reltotb / holdfactor; /*
					** file will be scanned using the
					** group buffer size, FIXME, lookup
					** this size from DMF sort cost CB */
    if (readall < 1.0)
	readall = 1.0;
    *imtid = FALSE;			/* TRUE if implicit tid used for access 
					*/
    *sargtype = ADC_KNOKEY;		/* return "type" of key found; i.e.
                                        ** no useful key exists */
    *pt_valid = TRUE;                   /* initially all pages can be read */
    if (rp->opv_mbf.opb_bfcount <= 0)/* if no matching boolean factors exist
				    ** which can be used as keys then
				    ** return, since entire relation needs
				    ** to be read */
    {
    	if ((rp->opv_mask & OPV_KESTIMATE)
	    &&
	    (rp->opv_keysel < 1.0)
	    &&
	    !order_only
	    )
	    dont_skip = FALSE;
	else
	{
	    *dio = readall;
	    *cpu = cop->opo_cost.opo_tups; /* all tuples are read */
	    return;
	}
    }
    else
    {
	if (!(rp->opv_mask & OPV_KESTIMATE)
	    &&
	    !order_only)
	    opx_error( E_OP0486_NOKEY);	/* key estimate is missing */
	dont_skip = TRUE;
    }
    sel = rp->opv_keysel;		/* this routine is interested in the
					** keying selectivity rather than
					** the entire selectivity, b40923 */

    if (dont_skip)
    {
        OPB_BOOLFACT        *bp;        /* boolean factor used to build key */


	ap = subquery->ops_attrs.opz_base->
	    opz_attnums[rp->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno]; /*get
                                        ** ptr to attribute upon which the
                                        ** key is ordered */
	bp = subquery->ops_bfs.opb_base->opb_boolfact
	    [rp->opv_mbf.opb_kbase->opb_keyorder[0].opb_gbf]; /* get pointer to 
                                        ** boolean factor used to build the 
                                        ** key */

	if (ap->opz_attnm.db_att_id == DB_IMTID)  /* see if we can access via 
					** TIDs: the matching boolean factor 
                                        ** must be on a TID */
	{   
	    DB_DATA_VALUE    datatype;  /* setup TID datatype */
            datatype.db_datatype = DB_TID8_TYPE;
            datatype.db_prec = 0;
            datatype.db_length = DB_TID8_LENGTH;
	    bfkeyp = opb_bfkget(subquery, bp, rp->opv_primary.opv_tid, 
		&datatype, NULL, TRUE);	/* get ptr to list of TID
                                        ** equality keys */
	    *cpu =
	    *dio = opn_vlcnt(bfkeyp->opb_keylist); /* each equality clause on a
					** tid will probably go to a
					** different page */
					/* total number of pages read */
	    *sargtype	= ADC_KEXACTKEY; /* tid equality key found */
	    *ordering = ap->opz_equcls; /* save single ordering
					** equivalence class */
	    *imtid = TRUE;		/* set TRUE since implicit TID is
					** used instead of keyed attributes */
	    return;
	}

	/* get list of keys to be used */
	bfkeyp = opb_bfkget(subquery, bp, ap->opz_equcls, &ap->opz_dataval,
			    &ap->opz_histogram.oph_dataval, TRUE);
    }

    if (dont_skip)
    switch (bfkeyp->opb_sargtype)
    {
    case ADC_KEXACTKEY: 
    {
	i4                    numkeys; /* number of exact keys in the list */
	OPN_PERCENT            seluniq; /* minimum selectivity possible given
                                        ** number of unique values in relation*/
	OPO_TUPLES	       unique_count; /* number of unique values */

        if (rp->opv_mbf.opb_bfcount != rp->opv_mbf.opb_mcount)
        {
            *sargtype = ADC_KRANGEKEY;  /* do not assume sortedness if all
                                        ** elements of key are not specified
                                        ** - FIXME - if the remainder of the
                                        ** equivalence classes are not
                                        ** involved in equi-joins, i.e. are
                                        ** just target list elements then it
                                        ** is probably alright to call this
                                        ** an exact match lookup */
            break;
        }
        else
            *sargtype   = ADC_KEXACTKEY;
                                                 
	unique_count = 1.0;
	if (rp->opv_mbf.opb_bfcount > 1)
	{   /* find if exact keys are defined on subsequent members of
            ** a multi-attribute ordering */
	    i4		attrcount;
	    for (attrcount = 0; attrcount < rp->opv_mbf.opb_bfcount; attrcount++)
	    {
		OPB_BFKEYINFO       *bfkeyp2;/* ptr to keylist header for boolean
					** factor used to build key */
		OPZ_ATTS            *aptr;/* ptr to joinop attribute element
					** upon which the relation is ordered*/
		
		aptr = subquery->ops_attrs.opz_base->opz_attnums
		    [rp->opv_mbf.opb_kbase->opb_keyorder[attrcount].opb_attno]; /*get
					** ptr to attribute upon which the
					** key is ordered */

		/* get list of keys  to be used */
		bfkeyp2 = opb_bfkget(subquery, subquery->ops_bfs.opb_base->opb_boolfact
		    [rp->opv_mbf.opb_kbase->opb_keyorder[attrcount].opb_gbf], /* get 
					** pointer to boolean factor used to 
					** build the key */
			aptr->opz_equcls, &aptr->opz_dataval,
				     &aptr->opz_histogram.oph_dataval, TRUE);

		if (!order_only)
		{
		    OPH_HISTOGRAM	*histp1;
		    histp1 = *opn_eqtamp(subquery, &rp->opv_trl->opn_histogram,
				    aptr->opz_equcls, TRUE); /* return histogram information
						    ** in this case only */
		    if (unique_count < (cop->opo_cost.opo_tups/histp1->oph_nunique))
			unique_count *= histp1->oph_nunique;
		    else
			unique_count = cop->opo_cost.opo_tups;	/* avoid overflow
					** bug 47137 */
		}
		if (bfkeyp2->opb_sargtype != ADC_KEXACTKEY)
		    break;		/* loop until non-exact keylist found,
                                        ** the values in the lists will be
                                        ** keyed in sorted order */
	    }
            if (attrcount != rp->opv_mbf.opb_bfcount)
            {
                *sargtype = ADC_KRANGEKEY;  /* do not assume sortedness if all
                                        ** elements of key are not specified */
                break;
            }
	    if (attrcount == 1)
		*ordering = ap->opz_equcls; /* save single ordering
					** equivalence class, FIXME - define
                                        ** a multi-attribute ordering 
					** if enough exact keys exist */
	    else if (attrcount ==  rp->opv_mbf.opb_count)
	    {	/* all attribute of key are used so create multi-attribute
                ** ordering based on this */
                if (!rp->opv_mbf.opb_prmaorder)
                    opo_makey(subquery, &rp->opv_mbf, rp->opv_mbf.opb_count,
                        &rp->opv_mbf.opb_prmaorder, FALSE, TRUE); /* get the equivalence
                                            ** class list for the ordering */
                *ordering = opo_cordering(subquery, rp->opv_mbf.opb_prmaorder, 
		    FALSE);
		if( (	rp->opv_grv->opv_relation->rdr_rel->tbl_status_mask
			&
			DMT_UNIQUEKEYS
		    )
		    &&
		    (	rp->opv_grv->opv_relation->rdr_keys->key_count
			==
			rp->opv_mbf.opb_count
		    )
		    &&
		    (cop->opo_cost.opo_tups > 0)
		    )
		{   /* adjust selectivity since all attributes of the
		    ** unique key have been found */
		    unique_count = cop->opo_cost.opo_tups;
		}
	    }
	    else
	    {	/* partial key is used so need to allocate a new list */
		OPO_EQLIST	    *newlist;
                opo_makey(subquery, &rp->opv_mbf, attrcount, &newlist, TRUE, TRUE);
                                        /* get the equivalence
                                        ** class list for the ordering */
		*ordering = opo_cordering(subquery, newlist, FALSE);
		if (unique_count > cop->opo_cost.opo_tups)
		    unique_count = cop->opo_cost.opo_tups;
	    }
	}
	else
	    *ordering = ap->opz_equcls;	/* save single ordering
					** equivalence class, FIXME - define
                                        ** a multi-attribute ordering 
					** if enough exact keys exist */
        if (subquery->ops_bfs.opb_bfeqc
            &&
            (*ordering < subquery->ops_eclass.ope_ev)
            &&
            (*ordering >= 0)
            &&
            BTtest((i4)*ordering, (char *)subquery->ops_bfs.opb_bfeqc)
            )
            *ordering = OPE_NOEQCLS;    /* do not return an ordering based
                                        ** on a constant equivalence class */
	if (order_only)
	{
	    *dio = 0.0;
	    *cpu = 0.0;
	    return;			/* only calculcate the ordering and
					** do not do any further processing*/
	}
	numkeys	    = opn_vlcnt(bfkeyp->opb_keylist); /* each key may select
                                        ** more than one tuple */
	*hpp = *opn_eqtamp(subquery, &rp->opv_trl->opn_histogram,
			ap->opz_equcls, TRUE); /* return histogram information
					** in this case only */
	if ((*hpp)->oph_nunique > unique_count)
	    unique_count = (*hpp)->oph_nunique;
        seluniq     = numkeys / unique_count;
        if ((sel < seluniq) && (seluniq < 1.0))
	    sel = seluniq;              /* adjust selectivity if smaller than
                                        ** the minimum possible */
        {   /* estimate number of blocks */
	    OPO_DBLOCKS        selblocks; /* number of blocks read given 
                                        ** estimated selectivity of relation */
            OPO_DBLOCKS	       blockpertup; /* estimated number of blocks read
                                        ** in order to reach a particular tuple
                                        */
            OPO_DBLOCKS        total;   /* total number of blocks read given
                                        ** number of tuples needed to lookup */

            if (cop->opo_cost.opo_reltotb  < cop->opo_cost.opo_tups)
		selblocks = cop->opo_cost.opo_reltotb * sel;
	    else
		selblocks = cop->opo_cost.opo_tups * sel;
					/* cannot have more DIO than tuples
                                        ** selected since tuples cannot span
                                        ** blocks, so check this for sparse
                                        ** hash tables etc */
	    blockpertup = cop->opo_cost.opo_reltotb / cop->opo_cost.opo_cvar.opo_relprim;
            total = blockpertup * numkeys;
            if (total > selblocks)      /* assume worst case */
		*dio = total;
	    else
		*dio = selblocks;
	    *cpu = *dio;
	}
	if (
#if 0
/* due to new keysel calculation, the estimates are made for multi-attribute keying */
	    (rp->opv_mbf.opb_bfcount > 1) /* if more than attribute exists in
                                        ** the key then pages touched will be
                                        ** too high, so reset the valid flag */
	    ||
#endif
	    (*hpp)->oph_default         /* if default statistics exist on the
                                        ** keyed attribute then there may be
					** an overestimate of pages touched
                                        ** also */
	    )
	    *pt_valid = FALSE;		/* initially all pages can be read */

	*dio += rp->opv_dircost;
					/* for each value in the clause
					** do a directory search and
					** read the primary and
					** overflow pages */
	*pblk = *dio;			/* fix bug 30397, make sure pagestouched
					** is equal dio estimate for locking */
	return;
    }
    default: 
    {
	if (!bfkeyp->opb_global
	    ||
	    (!bfkeyp->opb_global->opb_pmin && !bfkeyp->opb_global->opb_pmax))
	{
	    if ((rp->opv_mask & OPV_KESTIMATE)
		&&
		(rp->opv_keysel < 1.0)
		&&
		!order_only
		)
	    {
		dont_skip = FALSE;
		break;
	    }
	    *cpu = cop->opo_cost.opo_tups;
	    *dio = readall;
	    return;
	}
    }
    case ADC_KHIGHKEY: 
    case ADC_KLOWKEY:
    case ADC_KRANGEKEY:
    {
	*sargtype	= ADC_KRANGEKEY;
	break;				/* continue searching for a 
                                        ** ADC_KEXACTKEY sargtype or earlier
					** range sargtype */
    }
    }

    if (order_only)
    {
	*cpu = cop->opo_cost.opo_tups;
	*dio = readall;
	return;				/* only ordering is required so stop
					** histogram processing */
    }
#if 0
#ifdef    E_OP0486_NOKEY
    if (!bfkeyp->opb_global
	||
	(!bfkeyp->opb_global->opb_pmin && !bfkeyp->opb_global->opb_pmax))
	opx_error( E_OP0486_NOKEY);	    /* no key when one expected */
	    /* FIXME - changed from "return (readall);" to consistency check */
#endif
#endif
    {
	OPH_HISTOGRAM          *histp;	    /* ptr to histogram for storage
					    ** structure attribute */
	OPN_PERCENT            pcnt;	    /* total selectivity of the
					    ** relation after the key is
					    ** applied */
	i4                    numdirsearch; /* number of directory searches
					    ** i.e. to find the key disk
					    ** address
					    ** - equal to number of keys; i.e.
					    ** = 2 if high and low key exist
					    ** = 1 otherwise
					    */
	if (!dont_skip)
	{
	    pcnt = rp->opv_keysel;
	    histp = NULL;
	    numdirsearch = 2;
	}
	else
	{   /* now integrate that part of the histo between minv and maxv */
	    OPN_PERCENT	       keysel;	    /* estimated keying selectivity
						** given histograms of boolean
						** factors */
	    if (rp->opv_mask & OPV_KESTIMATE)
		keysel = rp->opv_keysel;
	    else
		keysel = 1.0;

	    numdirsearch	= 0;
	
	    /* opb_hvalue may not exist for repeat query style constants 
	    ** so the default minimum and maximum values will be needed
            **
            ** remove redundant code for selfactor, which has already
            ** been calculated (58140).
	    */
	    if (bfkeyp->opb_global->opb_pmin
		&&
		(bfkeyp->opb_global->opb_pmin->opb_hvalue))
	    	/* min key exists so use it */
		numdirsearch++;
	
	    if (bfkeyp->opb_global->opb_pmax
		&&
		(bfkeyp->opb_global->opb_pmax->opb_hvalue))
	    	/* maximum key exists so use it */
		numdirsearch++;

	    histp  = *opn_eqtamp(subquery, &rp->opv_trl->opn_histogram, 
		 ap->opz_equcls, TRUE);
	    pcnt = sel;				/* due to new keysel calculations
						** the sel estimate is more accurate
						** which obsoletes this calculation
						*/		
	    if (pcnt > keysel)
		pcnt = keysel;		    /* use selectivity given the
						** histogram key */
	    if (pcnt < sel)
		pcnt = sel;			    /* if default histograms are
						** used then sel might be much
						** higher */
	}

	*pblk	= pcnt * cop->opo_cost.opo_reltotb;
	if (*pblk < 1.0)
	    *pblk = 1.0;		    /* at least one block should be
					    ** read */
	else if (
#if 0
/* due to new keysel calculation, better estimates are made for multi-attribute keying */
 	    (rp->opv_mbf.opb_bfcount > 1) /* if more than attribute 
					    ** exists in the key then pages 
					    ** touched will be too high, 
					    ** so reset the valid flag */
	    ||
#endif
	    (	histp
		&&
		histp->oph_default	    /* if default statistics exist on 
					    ** the keyed attribute then there 
					    ** may be an overestimate of pages
					    ** touched also */
	    ))
	    *pt_valid = FALSE;		    /* tell caller that pblk will
                                            ** not be an accurate value for
                                            ** pages touched */
	{
	    OPO_BLOCKS	       singledio;   /* number of single DIOs prior to
						** doing a block read */
	    if (*pblk < 2.0)
		singledio = *pblk;
	    else
		singledio = 2.0;		    /* DMF does 2 single I/Os prior
						** to acclerating to group block
						** reads */
	    *cpu = *pblk * cop->opo_cost.opo_tups/ cop->opo_cost.opo_reltotb;
	    *dio = rp->opv_dircost * numdirsearch
						/* add in the number of
						** directory searches required */
		     + singledio + (*pblk - singledio)/holdfactor ;/* 2 block reads
						** are counted as a 2 DIOs, but
						** subsequent reads are counted
						** as holdfactor reads since DMF
						** knows it is scanning */
	}
    }
}
