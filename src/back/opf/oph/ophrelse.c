/*
** Copyright (c) 1986, 2001, 2004 Ingres Corporation
**
**
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
#define        OPH_RELSELECT      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>
#include    <adulcol.h>

/**
**
**  Name: OPHRELSE.C - calculate predicate clauses selectivity
**
**  Description:
**      routines which calculate the predicate clauses selectivity
**
**  History:
**      21-may-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**	22-apr-91 (seputis)
**	    Changed project restrict histogram processing to calculate
**	    keying costs, since OPC can do disjunctive keying but OPF
**	    does not have this in the model, ... this is a temporary fix
**	    until disjunctive keying specification is determined in OPF
**	16-aug-91 (seputis)
**	    General cleanup, add include files, remove used variables
**	    to fix lint errors
**      20-nov-91 (seg)
**          Can't dereference or do arithmetic on PTR variables.
**      15-dec-91 (seputis)
**          rework fix for MHexp problem to reduce floating point computations
**	09-nov-92 (rganski)
**	    Changed parameters to opn_diff() call in oph_relselect().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-93 (shailaja)
**	    Change to oph_correlation removed.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	19-jan-94 (ed)
**	    remove obsolete outer join estimation code
**	03-jan-94 (wilri01)
**	    Save oph_prodarea in bf
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	12-sep-1996 (inkdo01)
**	    Added code to accurately compute selectivities for "<>" (ne) 
**	    relops.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	16-Jun-00 (shust01)
**	    Backed out change for bug 97581.  Was causing queries to run
**	    slower when using a case insensitive collation sequence and 
**	    the case of the text in the query did not match the case of 
**	    the text in the table. Bug 101739.
**	    Note: Bug 97581 has been correctly fixed by the change for
**	    bug 99120.
**	20-may-2001 (somsa01)
**	    Changed nat to i4 due to cross-integration.
[@history_line@]...
**/

/*}
** Name: OPH_SELHIST - histogram and selectivity structure
**
** Description:
**      This structure contains a histogram list which describes
**	a set of boolean factors, and an associated selectivity
**	of that set of boolean factors
**
** History:
**      14-apr-91 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPH_SELHIST
{
    OPH_HISTOGRAM   *oph_histp;         /* histogram list */
    OPN_PERCENT     oph_predselect;     /* selectivity of histograms */
} OPH_SELHIST;

/*{
** Name: oph_correlation	- add area based on correlation of data
**
** Description:
**	When boolean factor histogram are processed and OR'ed together
**	there is usually a correlation,  this routine places a bais on
**	the normalization of histograms to be in cells containing single
**	equivalence class selectivity.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      masterhp                        ptr to histogram element which will
**                                      be updated with the result of the
**                                      correlation operation, this must
**                                      not be a default histogram
**	newsel				selectivity to which masterhp will
**					eventually be adjusted too.
**      sehist1p                        single equilvalence class boolean 
**					factor histogram, used to bais
**					this histogram normalization, must
**					be non-NULL
**	sehist2p			could be NULL, otherwise it is
**					a histogram AND'ed with the sehist1p
**
** Outputs:
**      masterhp                        histogram will be updated with the 
**					correlation
**                                      cell by cell of "masterhp" and 
**                                      "sehist1p" and if available
**					"sehist2p", the total selectivity
**					of masterhp->oph_prodarea will be
**					newsel or less
**	Returns:
**	    NONE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-apr-91(seputis)
**          initial creation
**	3-aug-93 (ed)
**	    return type not used so make it void
[@history_line@]...
*/
static VOID
oph_correlation(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *masterhp,
	OPN_PERCENT	   newsel,
	OPH_HISTOGRAM      *sehist1p,
	OPH_HISTOGRAM      *sehist2p)
{
    i4                  cell;	    /* current histogram cell being "ANDed" */
    OPN_PERCENT         area;       /* total selectivity of the histogram 
                                    ** after the AND operation */
    OPH_COUNTS		sehist1fcntp;   /* ptr to master count array */
    OPH_COUNTS		sehist2fcntp;   /* ptr to update count array */
    OPN_PERCENT		factor;	    /* used if one of the histograms is
				    ** a default i.e. not based on real
				    ** statistics */
    OPN_PERCENT		oramount;   /* percentage of cell selected by
				    ** the single eqcls histogram */
    OPN_PERCENT		master_amount; /* percentage of cell selected
				    ** by histogram being adjusted */
    bool		factor_exists;
    OPN_PERCENT		orarea;	    /* amount master histogram selectivity
				    ** must be increased by */
    OPN_PERCENT		searea;	    /* total selectivity of the single
				    ** eqcls boolean factors for this
				    ** eqcls */

    orarea = (newsel - masterhp->oph_prodarea) * 
	subquery->ops_global->ops_cb->ops_alter.ops_nonkey; /* this area
				    ** when or'ed on a cell by cell basis
				    ** will have selectivity less than
				    ** newsel */
    if (orarea <= 0.0)
	return;
    searea = sehist1p->oph_prodarea;
    sehist1fcntp = sehist1p->oph_fcnt;
    if (sehist2p)
    {	
	sehist2fcntp = sehist2p->oph_fcnt;
	searea *= sehist2p->oph_prodarea;
	oramount = sehist1p->oph_null * sehist2p->oph_null;
    }
    else
	oramount = sehist1p->oph_null;
    if (searea > orarea)
    {
	factor_exists = TRUE;
	factor = orarea/searea;
	oramount *=factor;
    }
    else
	factor_exists = FALSE;

    master_amount = masterhp->oph_null;
    area = masterhp->oph_null = 
	oramount + master_amount - oramount * master_amount;
				    /* calculate adjustment to NULLs in 
				    ** the histogram */
    cell = subquery->ops_attrs.opz_base->opz_attnums[masterhp->oph_attribute]->
	opz_histogram.oph_numcells;/* start with highest numbered cell */

    while (cell-- > 1)		    /* for each cell */
    {
	OPN_PERCENT            cellpercent; /* save the percentage of the cell
                                    ** which was selected */
	if (sehist2p)
	    oramount = sehist1fcntp[cell]*sehist2fcntp[cell];
	else
	    oramount = sehist1fcntp[cell];
	if (factor_exists)
	    oramount *= factor;

	master_amount = masterhp->oph_fcnt[cell];
	cellpercent = masterhp->oph_fcnt[cell] =
	    oramount + master_amount - oramount * master_amount;
	area += cellpercent * masterhp->oph_pcnt[cell];	/* add into area 
                                    ** the contribution of this cell by
                                    ** calculating the fraction of the relation
                                    ** which was selected */
				    /* FIXME oph_pcnt is based on a default histogram
				    ** try to fix this */
    }
    masterhp->oph_prodarea = area;
}

/*{
** Name: oph_and	- find AND of two histograms
**
** Description:
**	"AND" histogram of "updatehp" into histogram of "masterhp" and 
**      return the resulting area (normalized) of masterhp 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      masterhp                        ptr to histogram element which will
**                                      be updated with the result of the
**                                      "AND" operation
**	mastersel			current selectivity of the masterhp
**      updatehp                        ptr to histogram element which will
**                                      be used to update "masterhp"
**	updatesel			current selectivity of the updatehp
**
** Outputs:
**      masterhp                        histogram will be updated with the "AND"
**                                      cell by cell of "masterhp" and 
**                                      "updatehp"
**	Returns:
**	    percentage selectivity of the new histogram
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-86 (seputis)
**          initial creation
**	19-sep-88 (seputis)
**          default histograms causing bad estimates for ANDs, use
**          real histogram if possible, and default area percentages
[@history_line@]...
*/
static OPN_PERCENT
oph_and(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *masterhp,
	OPN_PERCENT	   mastersel,
	OPH_HISTOGRAM      *updatehp,
	OPN_PERCENT        updatesel)
{
    i4                  cell;	    /* current histogram cell being "ANDed" */
    OPN_PERCENT         area;       /* total selectivity of the histogram 
                                    ** after the AND operation */
    OPH_COUNTS		hist1fcntp;   /* ptr to master count array */
    OPH_COUNTS		hist2fcntp;   /* ptr to update count array */
    OPH_HISTOGRAM	*hist1p;    /* ptr to non-default histogram */
    OPH_HISTOGRAM	*hist2p;    /* ptr to non-default histogram */
    OPN_PERCENT		factor;	    /* used if one of the histograms is
				    ** a default i.e. not based on real
				    ** statistics */
#ifdef    E_OP0485_HISTAND
    {	/* consistency check - histograms should have the same structure */
	OPZ_ATTS               *masterap; /* ptr to joinop attribute element
                                    ** associated with the "master" histogram
                                    ** ptr */
	OPZ_ATTS               *updateap; /* ptr to joinop attribute element
                                    ** associated with the "update" histogram
                                    ** ptr */
	OPZ_AT                 *abase; /* ptr to base of array of joinop
                                    ** attribute ptrs */

	abase = subquery->ops_attrs.opz_base;
	masterap = abase->opz_attnums[masterhp->oph_attribute];
	updateap = abase->opz_attnums[updatehp->oph_attribute];
	/* JRBCMT -- check for equal prec here too? */
	/* JRBNOINT -- Can't integrate if we check for precision	    */
	if ((masterap->opz_histogram.oph_numcells !=
	     updateap->opz_histogram.oph_numcells)   || /* number of cells 
				    ** in both histograms must be equal */
	    (masterap->opz_dataval.db_datatype != 
	     updateap->opz_dataval.db_datatype)      || /* datatype of both
                                    ** histogram elements must be equal */
	    (masterap->opz_dataval.db_length != 
             updateap->opz_dataval.db_length)) /* length of the data type
                                    ** must be equal */
	    opx_error( E_OP0485_HISTAND); /* report
                                    ** error if this is not true */
    }
#endif

    area = OPN_0PC;		    /* start with 0 percent selectivity */

    /* for default histograms, assume that the distribution of the real
    ** histogram is correct and reduce the real distribution with a
    ** constant factor - FIXME - one can create a more accurate default
    ** histogram e.g. r.a > 10000 can have a one cell with a lower boundary
    ** of 10000 and can be used in histogram processing even though the
    ** histogram was not available */
    if (masterhp->oph_default)
    {
	if (updatehp->oph_default)
	{   /* both are default histograms */
	    return (updatesel * mastersel); /* default histograms so just return
				    ** with default selectivity for AND */
	}
	else
	{   /* use real histogram if possible */
	    masterhp->oph_default = FALSE; /* we now pick up the valid histogram
				    ** for the attribute */
	    hist1p = updatehp;
	    hist2p = NULL;
	    factor = mastersel;
	}
    }
    else
    {
	if (updatehp->oph_default)
	{   /* use master count array for both since real histogram available */
	    hist1p = masterhp;
	    hist2p = NULL;
	    factor = updatesel;
	}
	else
	{   /* use real histogram if possible */
	    hist1p = masterhp;
	    hist2p = updatehp;
	    hist2fcntp = hist2p->oph_fcnt;
	}
    }
    hist1fcntp = hist1p->oph_fcnt;

    cell = subquery->ops_attrs.opz_base->opz_attnums[masterhp->oph_attribute]->
	opz_histogram.oph_numcells;/* start with highest numbered cell */

    while (cell-- > 1)		    /* for each cell */
    {
	OPN_PERCENT            cellpercent; /* save the percentage of the cell
                                    ** which was selected */
	if (hist2p)
	    cellpercent =  masterhp->oph_fcnt[cell] = hist1fcntp[cell] * hist2fcntp[cell]; 
	else
	    cellpercent =  masterhp->oph_fcnt[cell] = hist1fcntp[cell] * factor;
                                    /* in general f1 is product of f1 and f2
                                    ** given the assumption of the
				    ** the "and" of independent events */
	area += cellpercent * masterhp->oph_pcnt[cell];	/* add into area 
                                    ** the contribution of this cell by
                                    ** calculating the fraction of the relation
                                    ** which was selected */
				    /* FIXME oph_pcnt is based on a default histogram
				    ** try to fix this */
    }
    if (hist2p)
	masterhp->oph_null = hist1p->oph_null * hist2p->oph_null;
    else
	masterhp->oph_null = hist1p->oph_null * factor;
    area += masterhp->oph_null * masterhp->oph_pnull; /* adjust area for
				    ** null values */
    return (area);
}

/*{
** Name: ophsarg	- create new histograms
**
** Description:
**	new histograms are created in two steps:
**
**	The first step creates and modifies a histogram. in this step each cell
**	count is a percentage  between 0 and 1. this amount is the percentage
**	of the original number of elements that are still in the cell.
**
**	The second step normalizes the histogram so that it integrates to 1.
**	(so ap->oph_pcnt[i]*tottups is the number of tups in cell i before and
**	ap->oph_fcnt[i]*ap->pcnt[i]*tottups is the number of tups in cell i 
**      after)
**
**	hp->oph_pcnt points to the normalized hist which is the precursor to
**	oph_fcnt.
**
** Inputs:
**      subquery                        subquery being analyzed
**      trl                             contains histogram list (one histogram
**                                      for each "interesting equivalence class"
**                                      in the "temporary relation") and tuple
**                                      count for "temporary relation"
**          .opn_histogram              - one element of this list represents
**                                      the equivalence class eqcls
**              .oph_pcnt               - count array containing percentages
**                                      of relation "trl"
**      bp                              pointer to boolean factor
**                                      to be used for restriction
**					NULL if called for composite histo.
**      eqcls                           equivalence class upon which the
**                                      histogram will be created or selected
**                                      and the boolean factor will be applied
**					OPE_NOEQCLS if called for composite
**					histogram.
**      valuep                          ptr to key list associated with the
**                                      equvalence class i.e. each key in the
**                                      list will effectively represent a
**                                      "limiting predicate" on eqcls within
**                                      the boolean factor
**	comphp				ptr to composite histogram (if that's
**					what is being analyzed) or NULL
**
** Outputs:
**	Returns:
**	    percent of trl "temporary relation" which was selected by this
**          (boolean factor, equivalence class) pair; i.e. selectivity of
**          eqcls predicates in the boolean factor 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-may-86 (seputis)
**          initial creation from histsarg
**	6-dec-89 (seputis)
**	    fix 8965 - uninitialize variable totf
**      12-feb-90 (seputis)
**          look at uniqueness of attribute to place upper bound on estimate
**          to help with metaphor queries
**      2-mar-90 (seputis)
**          adjust uniqueness for character histograms for another case in
**          metaphor
**	8-apr-91 (seputis)
**	    added support for OPF RANGEKEY and EXACTKEY flags 
**	11-oct-91 (seputis)
**	    added trace point to turn off >8 char histogram optimization
**	2-apr-92 (seputis)
**	    - b43171 - make LIKE operator behave like 2 range keys rather
**	    than one when no statistics are available
**	26-apr-93 (rganski)
**	    Character histograms enhancements project: removed >8 char
**	    histogram optimization (character_overflow). The selectivity of the
**	    cell is used, even if the length of the column is greater than the
**	    length of the histogram value, now that we have the -length flag
**	    for optimizedb to increase the value length.
**	21-jun-93 (rganski)
**	    b50012: added opno parameter to call to oph_interpolate() for
**	    special case of null string in type c column. See comments in
**	    ophinter.c for more details.
**	    Also added hp parameter to call to oph_interpolate(); this allows
**	    oph_interpolate to determine if histo is a default histo. This also
**	    allows removal of the reptf param, which can be dereferenced from
**	    hp.
**	19-jan-93 (ed)
**	    bug 54814- correct problem with procedure parameters, create uniform histogram
**	    to represent syntax based selectivity, since otherwise histogram
**	    is set to zero selectivity
**	12-sep-1996 (inkdo01)
**	    Added code to accurately compute selectivities for "<>" (ne) 
**	    relops.
**	 2-aug-1999 (hayke02)
**	    We now switch off collation before the second call to
**	    oph_interpolate() (and restore collation after). This
**	    prevents negative thissel and totf when collation is
**	    present and the where caluse contains a "like '<string>%'"
**	    predicate. This change fixes bug 97581.
**	16-Jun-00 (shust01)
**	    Backed out change for bug 97581.  Was causing queries to run
**	    slower when using a case insensitive collation sequence and 
**	    the case of the text in the query did not match the case of 
**	    the text in the table. Bug 101739.
**	    Note: Bug 97581 has been correctly fixed by the change for
**	    bug 99120.
**	3-oct-00 (inkdo01)
**	    Add support for composite histogram analysis.
**	10-nov-00 (inkdo01)
**	    When LIKE pred (RANGEKEY) has signif. chars after a wildcard,
**	    selectivity must be factored down - new code does this.
**	26-sep-01 (inkdo01)
**	    Re-add declaration of "composite" removed by cross int of 
**	    fix for 101739.
**      12-Mar-2002 (hanal04) Bug  107118 INGSRV 1690
**          Restrict inkdo01's 11-nov-00 change so that we do not
**          attempt to factor down selectivity after a wildcard
**          in a default histogram. Default histograms are padded
**          to the source column width so we were hitting a SIGFPE
**          as likefact became to big to hold in an f4.
**      30-mar-2004 (huazh01/inkdo01)
**          Modify the selectivity computation algorithm so that 
**          OPF uses NULL count for query that contains an 'is null' 
**          predicate in a series of OR-ed restrictions.
**          This fixes INGSRV2719, b111852.
**      19-oct-2007 (kibro01) b119313.
**          When the subquery's parameter is beyond the range of the 
**          histograms, or is in the range of a histogram with 0 rows, 
**          the selectivity becomes 0 and the query fails with  E_OP0491. 
**          Make sure that a minimum value instead of 0 is returned, to 
**          force Ingres to go and look for data in case some exists.
**          
[@history_line@]...
*/
static VOID
oph_sarg(
	OPS_SUBQUERY       *subquery,
	OPN_RLS            *trl,
	OPB_BOOLFACT       *bp,
	OPE_IEQCLS         eqcls,
	OPB_BFVALLIST      *valuep,
	OPH_HISTOGRAM	   *comphp)
{
    i4                  cellcount;  /* number of cells processed so far */
    OPH_HISTOGRAM       *hp;	    /* pointer to "normalized histogram" for
                                    ** eqcls, i.e. percentage of original
                                    ** cell count in respective histogram
                                    ** associated with the temporary relation
                                    ** trl
                                    */
    DB_DATA_VALUE       *datatype;  /* data type of the histogram element
                                    ** i.e. histogram type, and histogram length
                                    */
    PTR                 histval;    /* ptr to the histogram value of the
                                    ** constant found in the limiting predicate
                                    */
    PTR                 histval2;   /* ptr to the histogram value of the
                                    ** upper bound - used only if a range
                                    ** key is provided
                                    */
    i4                  histlen;    /* length of a cell boundary value which
                                    ** is equal to the length of a histogram
                                    ** element i.e. = datatype->db_length
                                    */
    i4                  numbercells;/* number of cells in this histogram */
    OPH_CELLS           bottom;	    /* lower bound for the current cell of
                                    ** the histogram being analyzed
                                    */
    OPO_TUPLES	        tuples;     /* estimated number of tuples in the
                                    ** relation */
    OPB_SARG            operator;   /* classification of key being applied
                                    ** to histogram */
    OPN_DPERCENT	totf;       /* working variable used to accumulate
                                    ** the selectivity of the relation after
                                    ** applying the boolean factor
                                    */
    OPN_DPERCENT	likefact = OPN_100PC;
    OPH_HISTOGRAM       **hpp;      /* ptr to ptr to histogram element */
    OPZ_ATTS            *ap;	    /* attribute pointer contains histogram
                                    ** information */
    OPH_INTERVAL	*intervalp;  /* ptr to histo type info */
    OPN_PERCENT		selectivity; /* selectivity of previous application
				    ** of disjuncts on this equcls */
    bool		composite = (eqcls == OPE_NOEQCLS);

    /* If this is NOT a composite histogram, must locate instance being 
    ** updated by current BF set. */
    if (eqcls != OPE_NOEQCLS)
    {
	if (hpp = opn_eqtamp(subquery, &bp->opb_histogram, eqcls, FALSE) )
            hp = *hpp;			/* histogram already exists */
	else
	{ /* create a normalized histogram for this eqclass initializing it to 
	  ** opb_selectivity */
	    hp = oph_create (subquery, trl->opn_histogram, 0.0, eqcls);
	    hp->oph_next = bp->opb_histogram; /* link new histogram into list 
                                    ** associated with the boolean factor */
	    bp->opb_histogram = hp;
	}
	ap = subquery->ops_attrs.opz_base->opz_attnums[hp->oph_attribute]; /* 
				    ** attribute which contains the interval
				    ** values */
	intervalp = &ap->opz_histogram;
    }
    else 
    {
	hp = comphp;
	intervalp = hp->oph_composite.oph_interval;
    }

    selectivity = hp->oph_prodarea;

    {   /* get relavent histogram information from attribute */

	histlen = intervalp->oph_dataval.db_length; /* length of
                                    ** histogram element and thus also the
                                    ** length of the cell */
        numbercells = intervalp->oph_numcells;
	datatype = &intervalp->oph_dataval;
	bottom = intervalp->oph_histmm;	/* ptr to lower bound of 
                                    ** first cell of the histogram */
    }
    tuples = trl->opn_reltups;	    /* number of tuples in the relation */
    histval = valuep->opb_hvalue;   /* histogram value of the key ... this
                                    ** value will fall into one of the cells
                                    ** of the histogram */

    if ((operator = valuep->opb_operator) == ADC_KRANGEKEY)
    {
	i4	*charn = intervalp->oph_charnunique;

	operator = ADC_KLOWKEY;	    /* histval is min value and histval2 is 
				    ** a max */
	histval2 = valuep->opb_next->opb_hvalue;

	/* Do no perform this loop for default histograms. */
	if (!composite && datatype->db_datatype == DB_CHA_TYPE && 
		histval2 && charn && !(hp->oph_default))
	{
	    /* Check for LIKE patterns containing "_" followed by 
	    ** specific characters. Range estimation ignores restriction
	    ** effect of following chars. These loops factor it back in. 
	    ** NOTE: "%" followed by specific characters are not handled
	    ** since hvalue string ends in all 'ff's. */
	    i4		i;
	    char	*workval = histval2;

	    for (i = 0; i < histlen; i++, workval++)
	     if (*workval == -1) break;
				    /* find first wildcard */
	    for ( ; i < histlen; i++, workval++)
	    {
		if (*workval == -1) continue;
				    /* ignore subsequent wildcards */
		likefact *= (f4)charn[i];
				    /* factor char position if not wildcard */
	    }
	}

    }

    if (valuep->opb_null)
    {	/* IS NULL or IS NOT NULL clause was used so pick up NULL
        ** selectivity */
	if (valuep->opb_null 
	    == 
	    subquery->ops_global->ops_cb->ops_server->opg_isnull
	    )
	{
	    totf = hp->oph_pnull;    /* percentage of column which is NULL */
	    hp->oph_null = 1.0;      /* select all NULLs in cell */
            if (hp->oph_pcnt)
            {
	       for (cellcount = 1; cellcount < numbercells; cellcount++)
               {
                   totf += hp->oph_fcnt[cellcount] * hp->oph_pcnt[cellcount];
               }
            }
	}
	else if (valuep->opb_null 
		== 
		subquery->ops_global->ops_cb->ops_server->opg_isnotnull
		)
	{
	    totf = 1.0 - hp->oph_pnull; /* percentage of column which is 
				    ** NOT NULL*/
	    for (cellcount = 1; cellcount < numbercells; cellcount++)
	    {	/* select all non null cells */
		hp->oph_fcnt[cellcount] = 1.0;
	    }
	}
	else
	    opx_error( E_OP048F_NULL);	/* consistency check - IS NULL or 
				    ** IS NOT NULL phrase expected */
    }
    else 
    {
        totf = hp->oph_null * hp->oph_pnull; 

	if (histval)
	{	/* normal selectivity calculation */
	    OPN_PERCENT	repitition; /* percentage selectivity of one tuple given
				    ** the repitition factor of hp->oph_reptf */

	    for (cellcount = 1; cellcount < numbercells; cellcount++)
	    {	/* for each cell of histogram (except first which is used
		** only to provide a min value for the second cell) 
		*/
		OPO_TUPLES	       maxtups;	/* estimated number of tuples
						** within the current interval
						*/ 
		OPH_CELLS	       top;     /* upper bound for the current
						** interval */
		OPN_DPERCENT	       thissel; /* percent of values in this
						** cell which satisfy limiting
						** predicate */
		
		top = bottom + histlen;		/* top of the current interval
						*/
		maxtups = hp->oph_pcnt[cellcount] * tuples; /* estimate number
							    ** of tuples in
							    ** current interval
							    */ 
		if (maxtups <= 0.0)
		    thissel = OPN_D0PC;
		/*
		** Note, there is a special case check since following
		** calculation does not work when maxtups is 0.0, with
		** ADC_KRANGEKEY cause oph_interpolate always returns 0.0 in
		** this case. This makes it go faster anyhow .
		*/
		else
		{
		    /* get proportion of values, x, between vmin and vmax which
		    ** satisfy "x <operator> histval" */
		    thissel = oph_interpolate(subquery, bottom, top, histval,
					      operator, datatype, maxtups,
					      valuep->opb_opno, hp);
		    /*
		    ** subtract out part that does not satisfy "x <= maxval"
		    **
		    ** NOTE: ADC_KLOWKEY below was ADC_KHIGHKEY which could
		    ** cause problems with char histos as the proportion GT a 
		    ** value plus the proportion LT a value may sum to greater
		    ** than 1
		    */

		    /* if the clause is less than greater than, then the total 
		    ** percentage = percent from bottom of cell to top of value
		    ** in the cell - percent from the bottom of the cell to the
		    ** bottom of the value
		    */
		    switch (valuep->opb_operator)
		    {
		    case ADC_KRANGEKEY:
			thissel -= oph_interpolate(subquery, bottom, top,
				       histval2, (i4) ADC_KLOWKEY, datatype,
				       maxtups, valuep->opb_opno, hp);
			break;
		    case ADC_KEXACTKEY:
			break;
		    case ADC_KNOKEY:
			thissel = 1.0 - thissel;
						/* NE sel is 1.0 - EQ sel */
			break;
		    }
		};

		{   /* add the percentage contribution of this cell to the
		    ** total selectivity in totf */
		    OPN_DPERCENT       oldsel;  /* percent of tuples selected
						** by this cell prior to
						** applying this predicate
						** - i.e. original histogram
						** count for this cell */
		    OPN_DPERCENT       newsel;  /* percentage of tuples
						** selected by this cell after
						** applying this predicate */
		    oldsel = hp->oph_fcnt[cellcount];
		    newsel = hp->oph_fcnt[cellcount] =
			thissel + oldsel - thissel * oldsel; /* "OR" of
						** (assumed) independent events
						*/
		    totf += newsel * hp->oph_pcnt[cellcount]; /* calculate area
						** of modified histo, where
						** (hopefully)
						** OPH_0PC <= totf <= OPH_100PC
						*/
		}
		bottom = top;			/* set bottom to beginning of
						** next cell */
	    }
	}
	else if (!hp->oph_default)
	{   /* histogram value does not exist but statistics do so use
	    ** this information for selectivity ... if the histogram element 
	    ** is not available, e.g. repeat query parameter ,simple aggregate
	    ** subselect result */

	    OPN_PERCENT	    xselect;

	    switch (valuep->opb_operator)
	    {
	    case ADC_KNOMATCH:
		xselect = 0.0;
		break;
	    case ADC_KALLMATCH:
		xselect = 1.0;
		break;
	    case ADC_KRANGEKEY:
		xselect = subquery->ops_global->ops_cb->ops_alter.ops_range
		    * subquery->ops_global->ops_cb->ops_alter.ops_range;
		    /* range key behaves as 2 inequalities */
		break;
	    case ADC_KLOWKEY:
	    case ADC_KHIGHKEY:
				    /* FIXME - slight inconsistency with
				    ** opn_prcost here since, prcost will
				    ** place a maximum selectivity for integer
				    ** datatypes involved in unique
				    ** secondary indices */
		xselect = subquery->ops_global->ops_cb->ops_alter.ops_range;
		break;
	    case ADC_KEXACTKEY:
	        xselect = 1.0/hp->oph_nunique; /* selectivity in this
				    ** case is one of the unique values */
		break;
	    case ADC_KNOKEY:
		xselect = 1.0 - 1.0/hp->oph_nunique; /* selectivity in this
				    ** case is all other unique values! */
		break;
	    default:
		xselect = subquery->ops_global->ops_cb->ops_alter.ops_exact;
	    }
	    for (cellcount = 1; cellcount < numbercells; cellcount++)
	    {	/* for each cell of histogram (except first which is used
		** only to provide a min value for the second cell) 
		*/
		OPH_CELLS	       top1;    /* upper bound for the current interval 
						*/
		top1 = bottom + histlen;	/* top of the current interval */

		{   /* add the percentage contribution of this cell to the total
		    ** selectivity in totf */
		    OPN_DPERCENT       oldsel;  /* percent of tuples selected by this
						** cell prior to applying this predicate
						** - i.e. original histogram count for
						** this cell */
		    OPN_DPERCENT       newsel;  /* percentage of tuples selected by this
						** cell after applying this predicate
						*/
		    oldsel = hp->oph_fcnt[cellcount];
		    newsel = hp->oph_fcnt[cellcount] =
			xselect + oldsel - xselect * oldsel; /* "OR" of (assumed) 
						** independent events */
		    totf += newsel * hp->oph_pcnt[cellcount]; /* calculate area of 
						** modified histo, where (hopefully)
						** OPH_0PC <= totf <= OPH_100PC */
		}
		bottom = top1;			/* set bottom to beginning of next cell
						*/
	    }
	}
	if (hp->oph_default)
	{	/* override the selectivity if a default histogram was used */
	    switch (valuep->opb_operator)
	    {

	    case ADC_KNOMATCH:
		totf = selectivity;
		break;
	    case ADC_KALLMATCH:
		totf = 1.0;
		break;
	    case ADC_KRANGEKEY:
		totf = subquery->ops_global->ops_cb->ops_alter.ops_range
		    *subquery->ops_global->ops_cb->ops_alter.ops_range;
		    /* range key behaves as 2 inequalities */
		totf = totf + selectivity - totf * selectivity;
		break;
	    case ADC_KLOWKEY:
	    case ADC_KHIGHKEY:
				    /* FIXME - slight inconsistency with
				    ** opn_prcost here since, prcost will
				    ** place a maximum selectivity for integer
				    ** datatypes involved in unique
				    ** secondary indices */
		totf =	subquery->ops_global->ops_cb->ops_alter.ops_range 
			+ selectivity -
			subquery->ops_global->ops_cb->ops_alter.ops_range
			* selectivity;   /* use default selectivity
				    ** if histogram do not have statistics */
		break;
	    case ADC_KEXACTKEY:
		if (hp->oph_unique)
		{
		    totf = 1.0 / tuples;    /* given no other information
				    ** use the number of tuples for selectivity */
		    break;	    /* do not override exact match if the
				    ** unique flag was set, since this could
				    ** be set by the existence of unique
				    ** secondary indices */
		}
		totf = subquery->ops_global->ops_cb->ops_alter.ops_exact 
		       + selectivity -
		       subquery->ops_global->ops_cb->ops_alter.ops_exact 
		       * selectivity;
		break;
	    case ADC_KNOKEY:
		if (hp->oph_unique)
		  totf = 1.0 - 1.0/tuples;  /* 1.0 - EXACT sel */
		 else totf = subquery->ops_global->ops_cb->ops_alter.ops_nonkey
		       + selectivity -
		       subquery->ops_global->ops_cb->ops_alter.ops_nonkey
		       * selectivity;
		break;
	    default:
		totf = subquery->ops_global->ops_cb->ops_alter.ops_exact 
		       + selectivity -
		       subquery->ops_global->ops_cb->ops_alter.ops_exact 
		       * selectivity;
	    }
	}
    }
    /* FIXME - there should be an assertion here to ensure "roundoff" did not
    ** to strange things to totf */
    totf /= likefact;			/* apply reduction for like's with
					** significant chars after the
					** wildcard */
    if (totf == OPN_D0PC) 
    {
	totf = OPN_MINPC;
    }

    hp->oph_prodarea = totf;		/* save the new selectivity after 
                                        ** applying the predicate to this
                                        ** histogram */
}

/*{
** Name: opb_dhist	- delete a list of histograms
**
** Description:
**      For memory management, delete a list of histograms no longer needed 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      histpp                          ptr to histogram list to delete
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
**      10-apr-91 (seputis)
**          initial creation to delete a histogram list
**	12-oct-00 (inkdo01)
**	    modify OPH_INTERVAL free logic for composite histograms.
[@history_template@]...
*/
static VOID
oph_dhist(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      **histpp)
{   /* free up all histos which were not needed */
    OPH_HISTOGRAM      *hp2;	/* current histogram being freed */
    OPH_HISTOGRAM      *hp2next;/* save ptr prior to deleting hp2 */
    OPZ_AT	       *abase;	/* ptr to base of array of ptrs to
                                ** joinop attribute elements */
    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to 
				** joinop attribute elements */
    for (hp2 = *histpp; hp2; hp2 = hp2next)
    {
	OPH_INTERVAL	*intervalp;
	hp2next = hp2->oph_next;
	if (hp2->oph_mask & OPH_COMPOSITE) 
			intervalp = hp2->oph_composite.oph_interval;
	else intervalp = &abase->opz_attnums[hp2->oph_attribute]->opz_histogram;
	oph_dccmemory(subquery, intervalp, hp2->oph_fcnt);
	oph_dmemory(subquery, hp2); /* return to memory manager */
    }
    *histpp = NULL;
}

/*{
** Name: oph_cclookup	- check for column comparison & set selectivity
**
** Description:
**	Check if this predicate is a comparison between columns in 
**	the same or different table(s). If the tables are different,
**	it is an inequality join that would otherwise produce poor 
**	result estimates. If the predicate is elib=gible, look for 
**	column comparison stats loaded from catalog and set selectivity 
**	accordingly. This is a (possibly) cute little heuristic that 
**	improves estimates on particular types of queries.
**
** Inputs:
**      subquery			ptr to current subquery
**      bp                              ptr to boolean factor containing 
**					possible column comparison predicate
**
** Outputs:
**      bp                              ptr to boolean factor with updated
**					selectivity estimate
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-jan-06 (dougi)
**	    Written to exploit column comparison statistics.
**	27-jan-06 (dougi)
**	    Sift out temp tables from lookup.
[@history_line@]...
*/
static bool
oph_cclookup(
	OPS_SUBQUERY	   *subquery,
	OPB_BOOLFACT       *bp)
{
    PST_QNODE		*bop = bp->opb_qnode;
    OPZ_AT		*abase;
    OPV_RT		*vbase;
    OPZ_IATTS		attr1, attr2;
    OPV_IVARS		varno1, varno2;
    i4			tabbase1, tabbase2;

    DMT_COLCOMPARE	*rdrcp;
    DB_COLCOMPARE	*colcp;

    i4			i;
    f4			ltc, eqc, gtc;
    bool		sametab;

    /* Check to verify that this is a qualifying column comparison. */
    if (bop->pst_sym.pst_type != PST_BOP ||
	bop->pst_left->pst_sym.pst_type != PST_VAR ||
	bop->pst_right->pst_sym.pst_type != PST_VAR)
	return(FALSE);			/* uninteresting */

    /* It is a comparison between columns in the same or different tables.
    ** Get column numbers in base table(s) and search for stats. */
    attr1 = bop->pst_left->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    attr2 = bop->pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
    abase = subquery->ops_attrs.opz_base;
    attr1 = abase->opz_attnums[attr1]->opz_attnm.db_att_id;
    attr2 = abase->opz_attnums[attr2]->opz_attnm.db_att_id;

    varno1 = bop->pst_left->pst_sym.pst_value.pst_s_var.pst_vno;
    varno2 = bop->pst_right->pst_sym.pst_value.pst_s_var.pst_vno;
    sametab = (varno1 == varno2);
    vbase = subquery->ops_vars.opv_base;
    if (vbase->opv_rt[varno1]->opv_grv->opv_relation == NULL)
	return(FALSE);			/* temp table - not interesting */
    else tabbase1 = vbase->opv_rt[varno1]->opv_grv->opv_relation->rdr_rel->
						tbl_id.db_tab_base;
    if (!sametab)
     if (vbase->opv_rt[varno2]->opv_grv->opv_relation == NULL)
	return(FALSE);			/* temp table - not interesting */
     else tabbase2 = vbase->opv_rt[varno2]->opv_grv->opv_relation->rdr_rel->
						tbl_id.db_tab_base;
    else tabbase2 = 0;

    rdrcp = subquery->ops_vars.opv_base->opv_rt[varno1]->opv_grv->
					opv_relation->rdr_colcompare;

    /* Loop over column stats for 1st table looking for one involving 
    ** exactly the columns in this predicate. */
    if (rdrcp != NULL)
     for (i = 0; i < rdrcp->col_comp_cnt; i++)
      if ((colcp = &rdrcp->col_comp_array[i])->cc_count != 1 ||
	(sametab && colcp->cc_tabid1.db_tab_base != 
			colcp->cc_tabid2.db_tab_base &&
	colcp->cc_tabid2.db_tab_base != 0) ||
	(!sametab && !(colcp->cc_tabid1.db_tab_base == tabbase1 &&
			colcp->cc_tabid2.db_tab_base == tabbase2 ||
	   colcp->cc_tabid1.db_tab_base == tabbase2 &&
			colcp->cc_tabid2.db_tab_base == tabbase1)) ||
	colcp->cc_tabid1.db_tab_index != colcp->cc_tabid2.db_tab_index ||
	(colcp->cc_col1[0] != attr1 && colcp->cc_col2[0] != attr1) ||
	(colcp->cc_col1[0] != attr2 && colcp->cc_col2[0] != attr2))
	continue;			/* no match on this one - keep trying */
      else break;			/* exit loop with 1st match */

    if (rdrcp == NULL || i >= rdrcp->col_comp_cnt)
     if (sametab || tabbase1 == tabbase2)
	return(FALSE);			/* no match - return */
     else
     {
	/* No match, but different tables. Load up and search stats
	** from 2nd table. */
	rdrcp = subquery->ops_vars.opv_base->opv_rt[varno2]->opv_grv->
					opv_relation->rdr_colcompare;

	/* Loop over column stats for 2nd table looking for one involving 
	** exactly the columns in this predicate. */
	if (rdrcp != NULL)
	 for (i = 0; i < rdrcp->col_comp_cnt; i++)
	  if ((colcp = &rdrcp->col_comp_array[i])->cc_count != 1 ||
	    (sametab && colcp->cc_tabid1.db_tab_base != 
			colcp->cc_tabid2.db_tab_base &&
	    colcp->cc_tabid2.db_tab_base != 0) ||
	    (!sametab && !(colcp->cc_tabid1.db_tab_base == tabbase1 &&
			colcp->cc_tabid2.db_tab_base == tabbase2 ||
		colcp->cc_tabid1.db_tab_base == tabbase2 &&
			colcp->cc_tabid2.db_tab_base == tabbase1)) ||
	    colcp->cc_tabid1.db_tab_index != colcp->cc_tabid2.db_tab_index ||
	    (colcp->cc_col1[0] != attr1 && colcp->cc_col2[0] != attr1) ||
	    (colcp->cc_col1[0] != attr2 && colcp->cc_col2[0] != attr2))
	    continue;			/* no match on this one - keep trying */
	  else break;			/* exit loop with 1st match */

     }

    if (rdrcp == NULL || i >= rdrcp->col_comp_cnt)
	return(FALSE);			/* no match in either table - return */

    /* Final step is to determine the resulting selectivity and return. */
    if (colcp->cc_col1[0] == attr1)
    {
	ltc = colcp->cc_ltcount;
	gtc = colcp->cc_gtcount;
    }
    else
    {
	/* Columns are reversed, so reverse the counts, too. */
	ltc = colcp->cc_gtcount;
	gtc = colcp->cc_ltcount;
    }
    eqc = colcp->cc_eqcount;

    switch (bop->pst_sym.pst_value.pst_s_op.pst_opno) {
      case ADI_LT_OP:
	bp->opb_selectivity = ltc;
	break;
	
      case ADI_LE_OP:
	bp->opb_selectivity = ltc + eqc;
	break;
	
      case ADI_EQ_OP:
	bp->opb_selectivity = eqc;
	break;
	
      case ADI_NE_OP:
	bp->opb_selectivity = ltc + gtc;
	break;
	
      case ADI_GE_OP:
	bp->opb_selectivity = gtc + eqc;
	break;
	
      case ADI_GT_OP:
	bp->opb_selectivity = gtc;
	break;
	
      default:
	return(FALSE);			/* oops */
    }

    return(TRUE);
    
}

/*{
** Name: oph_bfcost	- evaluate the clause selectivity
**
** Description:
**	Evaluate the selectivity of a clause given the
**	histograms for the attributes in the clause.
**
** Inputs:
**      global                          ptr to global state variable
**      trl                             ptr to temporary relation struct
**                                      which contains the list of histograms
**                                      for attributes to be used in the
**                                      selectivity
**      bp                              ptr to boolean factor containing keys
**					to be used in histogram processing
**
** Outputs:
**      bp                              ptr to boolean factor used to produce
**                                      next set of histograms
**          .opb_histogram              set of histograms on attributes
**                                      contained in the boolean factor
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-may-86 (seputis)
**          initial creation from clauseval
**	16-nov-88 (seputis)
**          fix performance problem by better estimate of selectivity
**          on exact match even when histogram value is not available
**	8-apr-91 (seputis)
**	    add support for OPF NONKEY flag
**	03-jan-94 (wilri01)
**	    Save oph_prodarea in bf so it will be available to opc_gtqual
**	    to sort predicates to optimize evaluation
**	19-may-97 (inkdo01)
**	    Set OPB_KEXACT flag for keysel logic in oph_relselect.
**	12-jan-06 (inkdo01)
**	    Added call to oph_cclookup to generate selectivity for 
**	    intra-table column comparison predicates.
**	22-may-08 (hayke02)
**	    Do not increment orcount/opb_torcount if OPB_KNOMATCH is set. This
**	    prevents LIKE '<string>' predicates estimating 50% of the rows
**	    instead of 0%, when <string> contains no pattern matching
**	    characters. Previously, the fix for bug 107869 meant that
**	    orcount/opb_torcount was incremeneted from 0 to 1, leading to the
**	    bad (50%) row estimate. This change fixes bug 120294.
[@history_line@]...
*/
static VOID
oph_bfcost(
	OPS_SUBQUERY	   *subquery,
	OPN_RLS            *trl,
	OPB_BOOLFACT       *bp)
{
    OPE_IEQCLS          maxeqcls;	/* number of equivalence classes */
    OPE_IEQCLS          eqcls;          /* current equivalence class being
                                        ** analyzed */
    i4                  orcount;        /* number of OR's found in the
                                        ** the boolean factor */
    bool		colcomp = FALSE;

    maxeqcls = subquery->ops_eclass.ope_ev; /* maximum 
					** number of equivalence classes*/
    if (bp->opb_mask & OPB_KNOMATCH)
	orcount = bp->opb_orcount;
    else
	orcount = bp->opb_orcount+1;	/* get count of number of "OR clauses"
                                        ** in the boolean factor
                                        ** - one more than number of OR nodes*/

    for (eqcls = -1; (eqcls = BTnext((i4)eqcls, (char *)&bp->opb_eqcmap,
	(i4) maxeqcls)) >= 0;)
    {
	OPB_BFKEYINFO          *bfkeyp;	/* current keyinfo ptr (one exists for
                                        ** each eqcls, type and length) being
                                        ** analyzed in the boolean factor */
        OPB_BFVALLIST          *valuep; /* ptr to key (one for each matching
                                        ** predicate in the clause) being
                                        ** analysed */
	bool			allexact = TRUE;

	if ((bfkeyp = opn_bfkhget(subquery, bp, eqcls, trl->opn_histogram, 
            FALSE, (OPH_HISTOGRAM **) NULL)) == NULL)
	{
	    if (oph_cclookup(subquery, bp))
	    {
		bp->opb_mask |= OPB_COLCOMPARE;
		colcomp = TRUE;
		orcount--;
		break;			/* if column compare, leave loop */
	    }
	    continue;			/* if key cannot be found then look
                                        ** at next equivalence class
                                        */
	}
	if (bfkeyp->opb_sargtype == ADC_KALLMATCH)
	{
	    bp->opb_mask |= OPB_ALLMATCH;
	    oph_dhist(subquery, &bp->opb_histogram);
	    return;			/* since no selectivity, remove histograms
					** created so far and exit */
	}
	bfkeyp->opb_used = FALSE;	/* for oph_relselect to indicate that 
                                        ** this key has been used in selectivity
                                        */
	for (valuep = bfkeyp->opb_keylist; valuep; valuep = valuep->opb_next)
	{
	    oph_sarg(subquery, trl, bp, eqcls, valuep, (OPH_HISTOGRAM *) NULL);
	    if (valuep->opb_operator != ADC_KEXACTKEY) allexact = FALSE;
	    if (valuep->opb_operator == ADC_KRANGEKEY)
		valuep = valuep->opb_next;  /* skip over high key of range 
                                        ** routines have already analyzed it*/
	    orcount--;
	}
	if (allexact && bp->opb_eqcls != OPB_BFMULTI) bp->opb_mask |= OPB_KEXACT;
					/* for more accurate keysel compute */
    }
    bp->opb_torcount = orcount;		/* save for later processing of non-sargable
					** keys */
    if (bp->opb_histogram)					
	bp->opb_selectivity = bp->opb_histogram->oph_prodarea;	/* Save for opc to sort	*/
    else if (!colcomp)			/* by after histograms have been freed. rsw 03jan96 */
	bp->opb_selectivity = 1.0;
}

/*{
** Name: oph_compbf_setup - prepare boolean factors which map composite histograms
**
** Description:
**      This routine builds a list of composite histograms which can be used
**	to compute selectivities of more than one BF at a time. This is much
**	more accurate than the simplistic "column value independence" assumption.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      trlp				ptr to the rt entry's TRL structure
**					(anchors its histogram chain)
**	bfmap				ptr to map of BFs applicable at this node
**	varno				range table index of leaf node table
**
** Outputs:
**	harrpp				ptr to array of ptrs to applicable
**					composite histograms
**	hcount				ptr to count of entries in hist array
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
**      28-sep-00 (inkdo01)
**          initial creation
[@history_template@]...
*/
static VOID
oph_compbf_setup(
	OPS_SUBQUERY	*subquery,
	OPN_RLS		*trlp,
	OPB_BMBF	*bfmap,
	OPH_HISTOGRAM	***harrpp,
	OPV_IVARS	varno,
	i4		*hcount)

{
    OPH_HISTOGRAM	*histp;
    OPH_HISTOGRAM	**locarrp;
    OPV_VARS		*varp = subquery->ops_vars.opv_base->opv_rt[varno];
    OPB_BFT		*bfbase = subquery->ops_bfs.opb_base;
    OPZ_AT		*zbase = subquery->ops_attrs.opz_base;
    OPB_BOOLFACT 	*bp;
    OPB_BMBF		locbfmap;
    OPB_IBF		bfi;
    i4			i, maxbfcount;
    bool		keepgoing, hashkey;


    /* First, count up the potentially useful composite histograms and allocate 
    ** the array. */

    for (histp = trlp->opn_histogram; histp; histp = histp->oph_next)
     if (histp->oph_mask & OPH_COMPOSITE &&
	BTsubset((PTR)&histp->oph_composite.oph_bfacts, (PTR)bfmap, 
					(i4)sizeof(OPB_BMBF))) (*hcount)++;

    if (*hcount == 0)
    {
	(*harrpp) = (OPH_HISTOGRAM **) NULL;
	return;
    }

    (*harrpp) = locarrp = (OPH_HISTOGRAM **)opu_memory(subquery->ops_global, 
		(*hcount) * sizeof(OPH_HISTOGRAM *));
    for (i = 0; i < *hcount; i++) locarrp[i] = (OPH_HISTOGRAM *) NULL;

    /* Now loop successively over histograms, adding them to the array in 
    ** descending order of the number of BFs they map. This approximates the
    ** selection of the histogram/BF combinations which should maximize the
    ** accuracy of the selectivity computations. */

    i = 0;
    MEcopy((PTR)bfmap, sizeof(OPB_BMBF), (PTR)&locbfmap);
					/* copy BF map for local manipulation */

    do {
	for (histp = trlp->opn_histogram, maxbfcount = 0, keepgoing = FALSE;
			histp; histp = histp->oph_next)
	/* Look for composite histograms that aren't already in the array,
	** are covered by the remaining BFs and map more BFs than any other
	** in this pass. */
	 if (histp->oph_mask & OPH_COMPOSITE &&
		!(histp->oph_mask & OPH_INCLIST) &&
		BTsubset((PTR)&histp->oph_composite.oph_bfacts, (PTR)&locbfmap,
					(i4)sizeof(OPB_BMBF)) &&
		histp->oph_composite.oph_bfcount > maxbfcount)
	 {
	    locarrp[i] = histp;		/* add to array */
	    maxbfcount = histp->oph_composite.oph_bfcount;
	    keepgoing = TRUE;
	 }

	if (keepgoing)
	{
	    OPB_BMBF	tbfmap;
	    locarrp[i]->oph_mask |= OPH_INCLIST;
					/* flag new entry */
	    MEcopy((PTR)&(locarrp[i]->oph_composite.oph_bfacts), 
		(i4)sizeof(OPB_BMBF), (PTR)&tbfmap);
	    for (bfi = -1; (bfi = BTnext((i4)bfi, (PTR)&tbfmap,
					(u_i2)sizeof(OPB_BMBF))) >= 0; )
		bfbase->opb_boolfact[bfi]->opb_mask |= OPB_BFCOMPUSED;
					/* flag BFs as used in comp hist. */
	    BTnot((i4)sizeof(OPB_BMBF), (PTR)&tbfmap);
	    BTand((i4)sizeof(OPB_BMBF), (PTR)&tbfmap, (PTR)&locbfmap);
					/* remove histos BFs from bfmap */
	    i++;
	}

    } while(keepgoing);

    *hcount = i;

    for (i = 0; i < *hcount; i++) locarrp[i]->oph_mask &= ~OPH_INCLIST;
					/* remove flags - temp only */

    /* Now check for composite histograms whose BFs can be used for 
    ** key structure restriction. Their combined selectivity becomes
    ** opv_keysel for the range variable. */

    if (varp->opv_mbf.opb_count == 0) return;
					/* not a structured var. */

    hashkey = (varp->opv_tcop && varp->opv_tcop->opo_storage == DB_HASH_STORE);
					/* flag indicating hash structure */

    {
	i4		key;
	OPB_KA		*kbase = varp->opv_mbf.opb_kbase;
	OPE_IEQCLS	eqcls1;
	bool		bfnokey, exact, range, found;

	/* Loop over all the composite histograms to determine which
	** can be used for key selectivity computation. The first 
	** histogram, all of whose BFs cover high order key columns,
	** is the key selection histogram. */

	for (i = 0; i < *hcount; i++)
	{
	    histp = locarrp[i];

	    /* Next pair of loops looks at key columns, then loops over 
	    ** BOOLFACTs looking for one with OPB_KESTIMATE which covers the 
	    ** key column with an EXACT match. This gives us key col eqclass
	    ** of LAST key column covered by KESTIMATEable factor. */
	    for (key = 0, bfnokey = FALSE; 
			key < varp->opv_mbf.opb_count && !bfnokey; key++)
	    {
		eqcls1 = zbase->opz_attnums[kbase->opb_keyorder[key].opb_attno]
					->opz_equcls;
		for (exact = FALSE, range = FALSE, bfi = -1; !exact && 
		    (bfi = BTnext((i4)bfi, 
			(char *)&histp->oph_composite.oph_bfacts, 
			(i4)sizeof(OPB_BMBF))) >= 0; )
		{	/* loop over all BFs in current composite histogram */
		    bp = bfbase->opb_boolfact[bfi];
		    if (!(bp->opb_mask & OPB_KESTIMATE))
		    {
			/* If BF isn't KESTIMATE, the histogram isn't 
			** eligible for key selectivity. */
			bfnokey = TRUE;
			break;
		    }
		    if (bp->opb_eqcls == eqcls1)
		     if (bp->opb_mask & OPB_KEXACT)
		      exact = TRUE;	/* got an EXACT on this col - exit */
		     else range = TRUE;
		}
		if (!exact) break;	/* leave key loop when no match */
	    }

	    if (hashkey && !exact) return;
	    if (!bfnokey)
	    {
		histp->oph_mask |= OPH_KESTIMATE;
		return;
	    }
	}

    }
}

/*{
** Name: oph_compand	- find AND of two compatible composite histograms
**
** Description:
**	"AND" composite histogram updatehp into composite histogram
**	masterhp, computing resulting selectivity and storing in masterhp.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      masterhp                        ptr to histogram element which will
**                                      be updated with the result of the
**                                      "AND" operation
**      updatehp                        ptr to histogram element which will
**                                      be used to update "masterhp"
**
** Outputs:
**      masterhp                        histogram will be updated with the "AND"
**                                      cell by cell of "masterhp" and 
**                                      "updatehp"
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-oct-00 (inkdo01)
**          initial creation for composite histogram project.
[@history_line@]...
*/
static VOID
oph_compand(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *masterhp,
	OPH_HISTOGRAM      *updatehp)
{
    i4                 cell;	    /* current histogram cell being "ANDed" */
    OPN_PERCENT         area;       /* total selectivity of the histogram 
                                    ** after the AND operation */
    OPH_COUNTS		hist1fcntp;   /* ptr to master count array */
    OPH_COUNTS		hist2fcntp;   /* ptr to update count array */

    area = OPN_0PC;		    /* start with 0 percent selectivity */

    /* Histograms are guaranteed to be from same set of composite attributes
    ** and to be non-default. We immediately set up for cell by cell ANDing. */

    hist1fcntp = masterhp->oph_fcnt;
    hist2fcntp = updatehp->oph_fcnt;

    cell = masterhp->oph_composite.oph_interval->oph_numcells;

    while (cell-- > 1)		    /* for each cell */
    {
	OPN_PERCENT            cellpercent; /* save the percentage of the cell
                                    ** which was selected */
	cellpercent =  masterhp->oph_fcnt[cell] = hist1fcntp[cell] * hist2fcntp[cell]; 
                                    /* in general f1 is product of f1 and f2
                                    ** given the assumption of the
				    ** the "and" of independent events */
	area += cellpercent * masterhp->oph_pcnt[cell];	/* add into area 
                                    ** the contribution of this cell by
                                    ** calculating the fraction of the relation
                                    ** which was selected */
    }
    
    masterhp->oph_null = masterhp->oph_null * updatehp->oph_null;
    area += masterhp->oph_null * masterhp->oph_pnull; /* adjust area for
				    ** null values */
    masterhp->oph_prodarea = area;
}

/*{
** Name: oph_compcost	- perform cost analysis on BFs mapping to composite 
**			histograms
**
** Description:
**      This routine loops over the composite histograms which are mapped
**	by Boolean factors on this relation, to determine the selectivity
**	of the factors and build an updated histogram for possible use in
**	higher level join nodes. If the OPB_VALLIST entries are to be ANDed,
**	the selectivity is computed in a new histogram and anded to the
**	existing, intermediate results. If they are to be ORed, they are
**	accumulated into the existing result.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      trl				ptr to the rt entry's TRL structure
**					(anchors its histogram chain)
**	charrpp				ptr to array of ptrs to applicable
**					composite histograms
**	chcount				ptr to count of entries in hist array
**
** Outputs:
**	chistp				ptr to chain of non-key composite
**					histograms
**	ckhistp				ptr to chain of key select composite
**					histograms (only 1, I think)
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
**      3-oct-00 (inkdo01)
**          initial creation
[@history_template@]...
*/
static VOID
oph_compcost(
	OPS_SUBQUERY	*subquery,
	OPN_RLS		*trl,
	OPH_HISTOGRAM	**chistp,
	OPH_HISTOGRAM	**ckhistp,
	OPH_HISTOGRAM	**charrpp,
	i4		chcount)

{
    OPH_HISTOGRAM	**sehistpp;
    OPB_BFVALLIST	*valp;
    i4			i;

    /* Loop over array and for each useful compoosite histogram, create
    ** a copy to contain modified counts, then call oph_sarg for each
    ** OPB_BFVALLIST entry to update counts and compute selectivity. */

    for (i = 0; i < chcount; i++)
    {
	OPH_HISTOGRAM	*hp, *comphp, *firsthp;
	bool		first, andval;

	hp = charrpp[i];

	for (valp = hp->oph_composite.oph_vallistp, first = TRUE; valp;
						valp = valp->opb_next)
	{
	    andval = (valp->opb_connector == OPB_BFAND);
	    if (andval || first)
		comphp = oph_create(subquery, hp, 0.0, (OPE_IEQCLS)OPE_NOEQCLS);
	    else comphp = firsthp;	/* select histogram to update */
	    oph_sarg(subquery, trl, (OPB_BOOLFACT *) NULL, 
				(OPE_IEQCLS) OPE_NOEQCLS, valp, comphp);
					/* selectivity & new counts */
	    if (valp->opb_operator == ADC_KRANGEKEY) valp = valp->opb_next;
					/* skip next val (picked up 
					** already by oph_sarg) */
	    /* AND this value comparison into total for this histogram. */
	    if (first)
	    {
		first = FALSE;
		firsthp = comphp;
	    }
	    else if (andval)
	    {
		oph_compand(subquery, firsthp, comphp);
		oph_dhist(subquery, &comphp);
	    }
	}

	/* Add cumulative result for composite histogram to approp. chain. */
	if (hp->oph_mask & OPH_KESTIMATE) sehistpp = ckhistp;
	else sehistpp = chistp;
	firsthp->oph_next = *sehistpp;
	*sehistpp = firsthp;		/* and chain for later combination */
    }

}

/*{
** Name: oph_keyand	- combine single eqcls hist and multi-eqcls hist
**
** Description:
**      This routine obtains a final selectivity by AND'ing all the
**	single eqcls histograms, with the respective multi-eqcls histograms. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      khistp                          ptr to histogram list and selectivity
**					which describe distribution which 
**					can be used in keying
**      nkhistp                         ptr to histogram list and selectivity
**					which describe distribution which 
**					cannot be used in keying
**
** Outputs:
**      nkhistp                         ptr to histogram list and estimated
**					selectivity which is the ANDing of 
**					histograms of khistp and nkhistp
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
**      14-apr-91 (seputis)
**          initial creation
**	4-aug-93 (ed)
**	    - fixed bug 51648
**	    If a combination of multi-eqcls boolean factors and single-eqcls
**	    boolean factors existed on a multi-attribute key for a relation,
**	    then the single eqcls boolean factors would be ignored in the
**	    estimate in some cases 
**	23-oct-01 (inkdo01)
**	    Fix problem with composite histograms.
[@history_template@]...
*/
static VOID
oph_keyand(subquery, khistp, nkhistp)
OPS_SUBQUERY	    *subquery;
OPH_SELHIST	    *khistp;
OPH_SELHIST	    *nkhistp;
{   /* AND the results of the single eqcls histograms and the multi-eqcls
    ** histograms... at this point all the multi-eqcls histograms have
    ** been normalized and have had correlations computed with respect
    ** to one another, i.e. all have the same
    ** prodarea equal to nkhistp->oph_predselect; but the single eqcls
    ** histograms are still unnormalized and have not had correlations
    ** computed i.e. have prodarea greater than khistp->oph_predselect
    ** and need to be combined either assuming independence or via
    ** the hueristic correlation formula a*b/(a+b)...  if there is one
    ** single equivalence class histogram which overlaps with a multi-eqcls
    ** histogram then this "ANDing" will be sufficient to include the
    ** the correlation of the multi-equcls histograms, however if there
    ** is more than one overlap, then the minimum overlap result can be used,
    ** this may miss some correlations but the error introduced at this point
    ** would make more complex computations not very valuable */

    OPH_HISTOGRAM	*shistp;
    OPZ_AT		*abase;
    OPN_PERCENT		new_predselect; /* computation of selectivity
				** given at least one single eqcls histogram
				** is in common with the multi eqcls
				** histogram list */
    OPN_PERCENT		new_prodarea; /* area of ANDed single eqcls and 
				** multi-eqcls histograms, which is 
				** RELATIVELY the most restrictive */
    OPN_PERCENT		old_prodarea; /* area of multi-eqcls histogram 
				** prior to ANDing to give new_prodarea */
    OPN_PERCENT		seqcls_prodarea; /* area of single eqcls histogram
				** which is the most restrictive when combined
				** with the multi eqcls histogram */
    bool		first_eqcls; /* TRUE only if one single eqcls has been
				** processed */
    bool		first_time; /* TRUE if only one single eqcls histogram
				** is in common with the multi eqcls list */

    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to 
				    ** joinop attribute elements */
    shistp = khistp->oph_histp;
    first_time = TRUE;
    first_eqcls = TRUE;
    new_predselect = 1.0;
    while (shistp)
    {
	OPE_IEQCLS	eqcls3;
	OPH_HISTOGRAM	*thistp; /* used to traverse single eqcls histogram list */
	OPH_HISTOGRAM	**thistpp; /* used to traverse multi-eqcls histogram list */

	thistp = shistp;
	shistp = shistp->oph_next;
	/* If not COMPOSITE, search for histo. amongst multi-eqc histos. */
	if (!(thistp->oph_mask & OPH_COMPOSITE) &&
	    (thistpp = opn_eqtamp (subquery, &nkhistp->oph_histp, 
		(eqcls3 = abase->opz_attnums[thistp->oph_attribute]->opz_equcls), 
		FALSE)))
	{   /* if histogram exists */
	    OPN_PERCENT	    temp_old_prodarea;
	    OPN_PERCENT	    temp_seqcls_prodarea;

	    temp_old_prodarea= (*thistpp)->oph_prodarea;
	    temp_seqcls_prodarea = thistp->oph_prodarea;
	    (*thistpp)->oph_prodarea = oph_and (subquery, (*thistpp), 
		nkhistp->oph_predselect, thistp, temp_seqcls_prodarea); 
				/* then AND this one with the first */
	    if (first_time)
	    {	/* found eqcls in common with multi-eqcls histograms */
		new_prodarea = (*thistpp)->oph_prodarea;
		old_prodarea = temp_old_prodarea;
		seqcls_prodarea = temp_seqcls_prodarea;	
		first_time = FALSE;
	    }
	    else if ((temp_old_prodarea <= 0.0) 
		    ||
		    (old_prodarea <= 0.0))
	    {	/* all estimates will go to zero at this point */
                old_prodarea = 0.0;
		new_predselect = 0.0;
	    }
	    else
	    {	/* more than one eqcls in common with multi-eqcls histograms
		** so choose the relatively most restrictive */
		if ((new_prodarea / old_prodarea) >
		    ( (*thistpp)->oph_prodarea / temp_old_prodarea))
		{   /* new minimum found, so switch with the old
		    ** minimum */
		    OPN_PERCENT	    temp;
		    temp = temp_seqcls_prodarea;
		    temp_seqcls_prodarea = seqcls_prodarea;
		    seqcls_prodarea = temp;
		    temp = temp_old_prodarea;
		    temp_old_prodarea = old_prodarea;
		    old_prodarea = temp;
		    new_prodarea = (*thistpp)->oph_prodarea;
		}
		if (first_eqcls)
		{
		    new_predselect = temp_seqcls_prodarea;
		    first_eqcls = FALSE;
		}
		else
		{
		    OPN_PERCENT	    predselect1;
		    predselect1 = new_predselect;
		    new_predselect *= temp_seqcls_prodarea;
		    if (subquery->ops_global->ops_cb->ops_check
			&&
			opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
			&&
			(new_predselect > 0.0)
			)
			new_predselect = new_predselect / (predselect1 
			    + temp_seqcls_prodarea); /* add hueristic correlation
					    ** factor */
		}
	    }
	    thistp->oph_next = NULL;
	    oph_dhist (subquery, &thistp);
	}
	else
	{   /* place histogram into final list */
	    if (first_eqcls)
	    {
		new_predselect = thistp->oph_prodarea;
		first_eqcls = FALSE;
	    }
	    else
	    {
		OPN_PERCENT	predselect2;
		predselect2 = new_predselect;
		new_predselect *= thistp->oph_prodarea;
		if (subquery->ops_global->ops_cb->ops_check
		    &&
		    opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
		    &&
		    (new_predselect > 0.0)
		    )
		    new_predselect = new_predselect / (predselect2 
			+ thistp->oph_prodarea); /* add hueristic correlation
					** factor */
	    }
	    thistp->oph_next = nkhistp->oph_histp;
	    nkhistp->oph_histp = thistp;
	}
    }
    if (first_time)
    {	/* there was not a single eqcls histogram in common with the
	** multi-eqcls histograms */
	new_predselect = khistp->oph_predselect * nkhistp->oph_predselect;
	if (subquery->ops_global->ops_cb->ops_check
	    &&
	    opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
	    &&
	    (new_predselect > 0.0)
	    )
	    new_predselect = new_predselect / (khistp->oph_predselect + nkhistp->oph_predselect);
    }
    else
    {	/* combine the minimum overlap eqcls with the overall estimate */
	if (first_eqcls)
	    new_predselect = new_prodarea;
	else
	{
	    OPN_PERCENT	predselect3;
	    predselect3 = new_predselect;
	    new_predselect *= new_prodarea;
	    if (subquery->ops_global->ops_cb->ops_check
		&&
		opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
		&&
		(new_predselect > 0.0)
		)
		new_predselect = new_predselect / (predselect3 
		    + new_prodarea); /* add hueristic correlation
				    ** factor */
	}
    }

    /* check that selectivity is reduced */
    if (new_predselect > nkhistp->oph_predselect)
	new_predselect = nkhistp->oph_predselect;
    if (new_predselect > khistp->oph_predselect)
	new_predselect = khistp->oph_predselect;

    nkhistp->oph_predselect = new_predselect;
    {
	OPH_HISTOGRAM	*hist4p;
	
	for (hist4p = nkhistp->oph_histp; hist4p; hist4p = hist4p->oph_next)
	    if (new_predselect != hist4p->oph_prodarea)
		oph_normalize(subquery, hist4p, new_predselect); /* normalize 
					** the histograms
					** associated with the boolean factor */
    }
}


/*{
** Name: oph_combine	- combine single eqcls hist and multi-eqcls hist
**
** Description:
**      This routine obtains a final selectivity by AND'ing all the
**	single eqcls histograms, with the respective multi-eqcls histograms. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      mehistp                         ptr to histogram list and selectivity
**					of multi-equivalence class boolean
**					factors which need estimates of
**					correlation due to ORing
**      sehistp                         ptr to histogram list and selectivity
**					of single equivalence class boolean
**					factors
**
** Outputs:
**      mehistp                         ptr to ANDed combination of mehistp
**					and sehistp
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
**      14-apr-91 (seputis)
**          initial creation
**	16-sep-04 (inkdo01)
**	    Copy sehistp in the absence of a mehistp rather than 
**	    merge. This avoids incorrectly setting predselect to 0.
**	7-dec-04 (inkdo01)
**	    Slight correction to the above (which caused really bad
**	    query plans).
[@history_template@]...
*/
static VOID
oph_combine(
	OPS_SUBQUERY	    *subquery,
	OPH_SELHIST	    *mehistp,
	OPH_SELHIST	    *sehistp)
{
    /* combine single eqcls histograms and multi-eqcls histograms */
    /* assume some correlation, since mostly attributes are correlated
    ** but there is no statistics to base this on,... but if we assume
    ** independence then estimates of 2 or more attributes usually 
    ** converge to 0 too quickly */
    /* in order to avoid syntax order dependences, sort in order 
    ** of selectivity */
    OPH_HISTOGRAM	    **thistpp;
    OPH_HISTOGRAM	    *thistp;
    OPN_PERCENT		    spredselect;    /*selectivity of all
					    ** single  equcls
					    ** boolean factors */
    bool		    firsttime;
    OPN_PERCENT		    predselect;

    thistp = NULL;
    spredselect = 1.0;
    predselect = mehistp->oph_predselect;
    firsttime = TRUE;

    if (mehistp->oph_histp == NULL && sehistp->oph_histp != NULL &&
	sehistp->oph_histp->oph_next == NULL)
    {
	/* If there is only one input histogram, copy to mehist rather than
	** merge (in which predselect gets set to 0.0). */
	mehistp->oph_histp = sehistp->oph_histp;
	mehistp->oph_predselect = sehistp->oph_histp->oph_prodarea;
	return;
    }

    do
    {	/* calculate a selectivity of all the single equivalence histograms
	** in sorted order, to avoid syntax dependence */
	OPH_HISTOGRAM	    **minhistpp;
	OPH_HISTOGRAM	    *t1histp;
	minhistpp = &sehistp->oph_histp;
	for (thistpp = &sehistp->oph_histp->oph_next; *thistpp; thistpp = &(*thistpp)->oph_next)
	    if ((*minhistpp)->oph_prodarea > (*thistpp)->oph_prodarea)
		minhistpp = thistpp;
	if (firsttime)
	{
	    firsttime = FALSE;
	    spredselect = (*minhistpp)->oph_prodarea;
	}
	else
	{
	    OPN_PERCENT	tselect;
	    tselect = spredselect * (*minhistpp)->oph_prodarea;
	    if (subquery->ops_global->ops_cb->ops_check
		&&
		opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
		&&
		(tselect > 0.0)
		)
		spredselect = tselect / (spredselect + (*minhistpp)->oph_prodarea);
		/* normally, for independent event, the product would be used but
		** this converges to zero too quickly, and more often than not
		** the data is correlated */
	    else
		spredselect = tselect;
	}
	t1histp = *minhistpp;
	*minhistpp = t1histp->oph_next;
	t1histp->oph_next = thistp;
	thistp = t1histp;
    } while (sehistp->oph_histp);
    sehistp->oph_histp = thistp;
    sehistp->oph_predselect = spredselect;

    if (mehistp->oph_histp)
    {   /* AND the results of the single eqcls histograms and the multi-eqcls
	** histograms, take the average of all the selectivities */
	oph_keyand(subquery, sehistp, mehistp);
    }
    else
    {
	mehistp->oph_histp = sehistp->oph_histp;
	if (subquery->ops_global->ops_cb->ops_check
	    &&
	    opt_strace( subquery->ops_global->ops_cb, OPT_F061_CORRELATION)
	    &&
	    (predselect < 1.0) 
	    && 
	    (predselect > 0.0)
	    )
	    predselect = predselect * spredselect / (predselect + spredselect);
	else
	    predselect *= spredselect;
	mehistp->oph_predselect = predselect;
    }
}

/*{
** Name: oph_relselect	- calculate predicate clause's selectivity
**
** Description:
**	Calculate the selectivity of the  predicate clauses indicated in bfm
**	(boolean factor map) upon relation r.  Setup the resulting histograms
**	for the equivalence classes specified in eqh
**
**	For each invocation of opn_enum this will be called once for each
**	relation and index.
**
**	Tests of sargable clauses will include ADC_KHIGHKEY and ADC_KLOWKEY. 
**      The only clause that is excluded are those that are ADC_KNOKEY
**      (ie. not sargable). This ensures that the selectivity of less than 
**      and greater than clauses are correctly calculated.  Otherwise,
**      the selectivity always remained 1.0 which indicated a full relation
**      scan which caused us to ignore secondary indexes if they existed.
**
**      Since the complete flag can greatly affect the number of tuples
**      being returned from a join (an order of magnitude
**      difference!!) it is reasonable to try to keep this flag set.
**      FIXME - this flag could be remain set if all restrictions
**      in the subtree apply only to that attribute, because then the
**      sub-domain would be complete
**	Called by:	opn_nodecost
**
** Inputs:
**      subquery                        ptr to subquery being analysed
**      trl                             contains list of histograms
**                                      applicable to this relation
**      bfm                             map of boolean factors that can be
**                                      applied at this node
**      eqh                             map of equivalence classes which
**                                      require histograms
**      jeqc                            joining equivalence class or
**                                      OPE_NOEQCLS if leaf node
**      nleaves                         number of leaves in the subtree
**      jxcnt                           number of possible equivalence
**                                      classes that can be used to join
**                                      i.e. number of join predicates
**                                      - used to take into account selectivity
**                                      of these type of boolean factors (which
**                                      were not added to the bool fact array)
**                                      - 0 if leaf node
**      varp				NULL if not a leaf node, otherwise a
**					pointer to the variable descriptor
**					for the leaf
**
** Outputs:
**      selectivity                     percent of temp relation after
**                                      boolean factors applied
**	Returns:
**	    RLS struct which contains histograms after boolean factors are
**          applied
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-may-86 (seputis)
**          initial creation from h1
**	20-sep-88 (seputis)
**          use percent estimates for default histograms
**	16-dec-88 (seputis)
**          fix unix lint problems
**	24-jul-89 (seputis)
**          - make partial fix for b6966, sometimes multi-variable function attribute
**      joins are not included in the jeqc, so that there selectivity needs to be
**      included in jxcnt.  Also, changed multi-attribute join support jxcnt, since
**	the selectivity is included in the oph_jselect calculations.
**	6-dec-90 (seputis)
**	    - make better estimate for PR keying
**	21-feb-91 (seputis)
**	    - added initialization of opn_boolfact to help with better estimates for
**	    existence only checks
**	4-apr-91 (seputis)
**	  for ranges like r.a < 5 and r.a > 10, the optimizer will try to estimate
**	  keying cost more accurately, so "nokey" histograms should be looked at
**	  unless they are non-sargable.
**	24-sep-91 (seputis)
**	    - added parameter to check for underflow in MHpow function
**      20-nov-91 (seg)
**          Can't dereference or do arithmetic on PTR variables.
**	28-apr-92 (seputis)
**	    - bug 43820 - problem with estimation of non-sargable clauses, if no
**	    keying clauses exist at all then no restriction is made at all.
**	09-nov-92 (rganski)
**	    Changed parameters to opn_diff() call.
**	14-dec-92 (rganski)
**	    Added datatype parameter to call to opn_diff().
**	    Moved declaration of attrp out one level so it can be
**	    used for call to opn_diff().
**	15-feb-93 (ed)
**	    fix outer join problem, use local outer join maps instead of global
**	    maps, since global maps are not kept up to date on strength reduction
**	    optimizations
**	21-may-93 (ed)
**	    fix bug 47377 - copy opn_mask to indicate if uniqueness check is
**	    appropriate
**      15-feb-94 (ed)
**          bug 49814 - E_OP0397 consistency check
**	09-jan-96 (kch)
**	    use mehist.oph_predselect * mekhist.oph_predselect to set the
**	    selectivity of all predicates analyzed so far, predselect. This
**	    gives the correct selectivity for the first key. This change
**	    fixes bug 73290.
**	12-jan-96 (inkdo01)
**	    Added support for per-cell repetition factors.
**	7-mar-96 (inkdo01)
**	    Added opn_relsel to OPN_RLS structure, to allow more intelligent 
**	    handling of complete flags.
**	17-mar-97 (inkdo01)
**	    Added support of config parm opf_spatkey to specify default 
**	    selectivity of spatial predicates (for Rtrees).
**	16-may-97 (inkdo01)
**	    Added logic to further refine the set of Boolean factors used
**	    to estimate opv_keysel. 
**	3-nov-99 (inkdo01)
**	    Added OPV_HISTKEYEST flag to show keysel based on real histogram.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	19-feb-01 (hayke02)
**	    The fix for bug 82388 has been modified so that we also switch
**	    off OPB_KESTIMATE if we find a boolean factor (other than a date
**	    range bf - eqcls1) that has an or count. This prevents disk IO
**	    underestimation for queries with or'ed equals predicates and
**	    date range predicates on different attributes. This change fixes
**	    bug 103745.
**	28-sep-00 (inkdo01)
**	    Add code for composite histogram selectivity computation.
**	19-feb-01 (hayke02)
**	    The fix for bug 82388 has been modified so that we also switch
**	    off OPB_KESTIMATE if we find a boolean factor (other than a date
**	    range bf - eqcls1) that has an or count. This prevents disk IO
**	    underestimation for queries with or'ed equals predicates and
**	    date range predicates on different attributes. This change fixes
**	    bug 103745.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Changes to support new RLS/EQS/SUBTREE structures. This change
**	    fixes bug 103858.
**	19-nov-01 (inkdo01)
**	    Added nleaves to opn_deltrl call to allow resolution of linkage
**	    around dropped OPN_RLS.
**	21-feb-02 (inkdo01)
**	    Fine tuning fix for 103858 by opn_deltrl'ing the RLS that 
**	    pre-existed in *trlpp before overlaying with trl.
**	31-oct-03 (hayke02)
**	    Composite histograms means we can have a NULL varp and a non-NULL
**	    mekhist.oph_histp, leading to a SEGV when we try and set
**	    varp->opv_keysel to mekhist.oph_predselect. We now check for a
**	    non-NULL varp before attempting this assignment. This change fixes
**	    bug 111069.
**	 8-apr-05 (hayke02)
**	    Test for OPB_PCLIKE (set for "like '%<string>'" predicates) and set
**	    unknown to 10% (ops_range, OPS_INEXSELECT). This change fixes bug
**	    114263, problem INGSRV 3241.
**	15-feb-08 (hayke02)
**	    Set thistp->oph_next to NULL before the call to oph_dhist(), as
**	    we already do in oph_keyand(). This prevents the same pointer
**	    being placed on the free chains multiple times when the FOR loop
**	    in oph_dhist() and the WHILE loop in this function traverses the
**	    same oph_next linked list. This change fixes bug 119900.
**	10-Mar-2009 (kibro01) b121740
**	    If a histogram links to multiple equivalence classes, it is being
**	    used from multiple segments of the query plan (probably because
**	    of a duplicated correlated subquery split apart due to multiple
**	    aggregates) so don't remove it from the list on the first
**	    occasion or we won't have it available when the second equivalence
**	    class asks for it.
**      10-jul-2009 (huazh01)
**          when copying histogram from leaf, we need to allocate oph_fcnt for
**          the 'found' histogram. (b122296)
**          
[@history_line@]...
*/
VOID
oph_relselect(
	OPS_SUBQUERY	   *subquery,
	OPN_RLS		   **trlpp,
	OPN_RLS            *trl,
	OPB_BMBF           *bfm,
	OPE_BMEQCLS        *eqh,
	bool		   *selapplied,
	OPN_PERCENT        *selectivity,
	OPE_IEQCLS         jeqc,
	OPN_LEAVES         nleaves,
	OPN_JEQCOUNT       jxcnt,
	OPV_IVARS	   varno)
{
    OPN_PERCENT         predselect;	/* selectivity of all predicates
                                        ** analyzed so far */
    OPB_IBF             maxbf;          /* number of boolean factors allocated*/
    OPN_RLS             *newtrl;        /* ptr to new joinop "relation 
                                        ** description" returned to user */
    OPB_BFT             *bfbase;        /* ptr to base of array of ptrs to
                                        ** boolean factor elements */
    OPZ_AT              *abase;		/* ptr to base of array of ptrs to
                                        ** joinop attribute elements */
    OPE_IEQCLS          maxeqcls;	/* maximum number of equivalence
					** classes defined */
    OPE_BMEQCLS		keqcmap;	/* bitmap of equilvalence classes
					** in the key */
    OPB_KA		*kbase;		/* ptr to key definition for varp
					** if it is not null */
    OPZ_AT		*zbase;		/* base of joinop attribute array */
    OPH_HISTOGRAM	*histlistp;	/* list of histograms which contain
					** current information of boolean factors
					** evaluated so far */
    OPH_HISTOGRAM	*comphlistp;	/* list of composite histograms which
					** contain information of BFs evaluated
					** so far. */
    OPV_VARS		*varp;
    bool		hashkey;
    PTR			tbl = NULL;	/* collation table */
    OPH_SELHIST		compsehist;	/* holds single att histos from BFs
					** mapped by comp histos. */
    OPH_HISTOGRAM	**charrpp = NULL;  /* composite histogram ptr array ptr */
    i4			chcount = 0;	/* count of comp histos */

    if (subquery->ops_global->ops_gmask & OPS_COLLATEDIFF)
        tbl = subquery->ops_global->ops_adfcb->adf_collation;

    maxeqcls = subquery->ops_eclass.ope_ev; /* number of equivalence classes
					** which have been defined so far */
    maxbf = subquery->ops_bfs.opb_bv;	/* number of boolean factors allocated*/
    predselect = OPN_D100PC;		/* start with 100 percent selectivity
                                        ** prior to applying boolean factors
                                        */
    bfbase = subquery->ops_bfs.opb_base;/* ptr to base of array of ptrs to 
                                        ** boolean factors */
    if (varno >= 0) varp = subquery->ops_vars.opv_base->opv_rt[varno];
    else varp = NULL;
    *selapplied = TRUE;
    if (*trlpp && nleaves > 1 && *trlpp != trl) /* if a different RLS is already
					** in *trlpp, delete it before overlay */
	opn_deltrl (subquery, *trlpp, jeqc, nleaves); 
    *trlpp = trl;
    if (jxcnt >= OPN_1JEQCLS)
    {	/* at least one joining eqcls */
	if (jeqc >= maxeqcls)
	{   /* this is a multi-attribute join, so determine the
	    ** number of equivalence classes which have already been
	    ** processed in the histogram, so that the remaining equivalence
	    ** classes should have default selectivity */
	    jxcnt -= BTcount((char *)subquery->ops_msort.opo_base->
		opo_stable[jeqc-maxeqcls]->opo_bmeqcls, (i4)maxeqcls);
	}
	else if (jeqc != OPE_NOEQCLS)
	    jxcnt--;
	while (jxcnt-- >= OPN_1JEQCLS)
	{
	    *selapplied = FALSE;
	    predselect *= OPN_D50PC;	/* might still have some function
					** attribute joins which did not
					** participate in the tuple selectivity
					** - give each join predicate 50 percent
                                        ** selectivity */
	}
    }

    if (!BTcount((char *) bfm, (i4) maxbf) && chcount == 0)
    {	/* if no boolean factors apply then return selectivity */
	if (varp)
	    varp->opv_mbf.opb_bfcount = 0; /* some outer join boolean factors
					** may have made this non-zero initially
					*/
	*selectivity = predselect;
	trl->opn_boolfact = FALSE;	/* no boolean factors exist at this
					** node */
	return;				/* set of histograms does not change */
    }

    trl->opn_boolfact = TRUE;		/* at least one boolean factor can
					** be applied at this node */
    bfbase = subquery->ops_bfs.opb_base;/* ptr to base of array of ptrs to 
                                        ** boolean factors */
    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to 
					** joinop attribute elements */
    zbase = subquery->ops_attrs.opz_base;
    if (varp)
    {
	if ((varp->opv_mask & OPV_KESTIMATE)
	    ||
	    (	subquery->ops_global->ops_cb->ops_check
		&&
		opt_strace( subquery->ops_global->ops_cb, OPT_F064_NOKEY)
					/* ignore enhanced keying estimates
					** if pre-6.4 semantics wanted */
	    ))
	    varp = (OPV_VARS *) NULL;	    /* estimates already complete */
	else
	{   /* check for keying equivalence classes, since OPC calculates OR keying,
	    ** assume that any qualification which is sargable and involved keys only
	    ** on the storage structure will eventually be used to produce a restricted 
	    ** lookup, FIXME need to move OPC code and specify the keying in OPF */

	    i4			key;

	    varp->opv_mask |= OPV_KESTIMATE;	/* mark variable has having
						** estimates available */
	    varp->opv_keysel = 1.0;
	    kbase = varp->opv_mbf.opb_kbase;
	    MEfill(sizeof(keqcmap), (u_char)0, (char *)&keqcmap);
	    hashkey = varp->opv_tcop
		&&
		(varp->opv_tcop->opo_storage == DB_HASH_STORE);
	    for (key = 0;
		key < varp->opv_mbf.opb_count;
		key++)
	    {
		OPE_IEQCLS	    eqcls1;
		eqcls1 = zbase->opz_attnums[kbase->opb_keyorder[key].opb_attno]->opz_equcls;
		BTset((i4)eqcls1, (char *)&keqcmap);
	    }
	    if (key <= 0)
		varp = (OPV_VARS *)NULL;	    /* no keying so do not do extra work */
	}
    }

    /* Identify Boolean factors useful in performing keyed access to base table
    ** structure. Several steps are involved in doing this:
    **    1. flag BFs which are restrictive (!ALLMATCH and !DNOKEY) and
    **       which cover a high order subset of the key columns.
    **    2. re-flag only those BFs from step 1 which cover a high order
    **       subset of the key columns, and which are in turn covered
    **       only by those columns. I.e., no col is ref'ed which isn't in
    **       the key column subset, and every column in the key order
    **       subset is covered by one of the BFs.
    **    3. of those BFs, only retain those with EXACT key matches on high
    **       order key columns + the first non-exact match. This stops the
    **       key selectivity computation after the first column with only a
    **       range comparison and fixes a long standing (i.e. forever) problem
    **       in Ingres' costing of access through composite index structures.
    */

    {
	OPB_IBF             bfi;	    /* current boolean factor being 
					    ** analyzed */
	OPE_BMEQCLS	    tempmap;
	bool		    firstkey;	    /* TRUE if most significant
					    ** attribute has a boolean factor
					    ** which is only on a single eqcls */
	OPE_IEQCLS	    keyeqcls;

	if (varp)
	{
	    MEfill(sizeof(tempmap), (u_char)0, (PTR)&tempmap);
	    firstkey = FALSE;
	    keyeqcls = zbase->opz_attnums[kbase->opb_keyorder[0].opb_attno]->opz_equcls;
	}
	for (bfi = -1; (bfi = BTnext((i4)bfi, (char *)bfm, (i4)maxbf)) >= 0;)
	{	/* evaluate the selectivity for each boolean factor */
	    OPB_BOOLFACT           *bp1;    /* ptr to boolean factor being 
                                            ** analyzed */
	    
	    bp1 = bfbase->opb_boolfact[bfi];/* get ptr to next boolean factor */
	    bp1->opb_histogram = NULL;      /* init to empty histogram list */
	    if (varp
		&&
		subquery->ops_oj.opl_base
		&&
		(bp1->opb_ojid >= 0)
		&&
		!BTtest((i4)varno, (char *)subquery->ops_oj.opl_base->opl_ojt
		    [bp1->opb_ojid]->opl_rmaxojmap)  
		)
				/* do not include ON clause qualifications
				** which cannot be used for keying,
				** - these clauses should not have been
				** placed in bitmap, check for this */
		opx_error(E_OP0397_MISPLACED_OJ);
	    oph_bfcost (subquery, trl, bp1); /* evaluate 
					    ** the selectivity of this 
					    ** boolean factor - this routine
                                            ** also create histograms and
                                            ** attach them to bp1, these
                                            ** histograms include newly
                                            ** created opn_fcnt structures */
	    if (bp1->opb_mask & OPB_ALLMATCH)
		bp1->opb_mask |= OPB_BFUSED; /* skip boolean factor if ALLMATCH */
	    else
		bp1->opb_mask &= ~(OPB_BFUSED | OPB_BFCOMPUSED);
	    if (varp)
	    {
		if(!(bp1->opb_mask & (OPB_DNOKEY | OPB_ALLMATCH))
		    &&
		    BTsubset((char *)&bp1->opb_eqcmap, (char *)&keqcmap, (i4)maxeqcls)
		    &&
		    (   !hashkey
			||
			!(bp1->opb_mask & OPB_NOHASH) /* check that no inequalities
							** exist if hashing is proposed */
		    ))
		{
		    BTor((i4)maxeqcls, (char *)&bp1->opb_eqcmap, (char *)&tempmap);
		    bp1->opb_mask |= OPB_KESTIMATE;
		    if (keyeqcls == bp1->opb_eqcls)
		    {
			OPH_HISTOGRAM	*hp;

			firstkey = TRUE;
			if ((hp = bp1->opb_histogram) &&
					!(hp->oph_default))
			 varp->opv_mask |= OPV_HISTKEYEST;
					/* 1st keycol has real histogram */
		    }
		}
		else
		    bp1->opb_mask &= (~OPB_KESTIMATE);
	    }
	}
	if (varp && !firstkey)
	{
	    varp->opv_mbf.opb_bfcount = 0; /* in case outer join ON clauses were
					** eliminated */
	    varp = NULL;		/* since the first attribute cannot be restricted
					** on its' own, then the relation cannot be keyed */
	}
	while (varp && !BTsubset((char *)&keqcmap, (char *)&tempmap, (i4)maxeqcls))
	{   /* remove part of key and boolean factors which cannot be used */
	    i4	    key;
	    if (hashkey)
	    {
		varp->opv_mbf.opb_bfcount = 0; /* in case outer join ON clauses were
					** eliminated */
		hashkey = FALSE;
		varp = NULL;		    /* hash key requires that all values
					    ** are available */
		break;
	    }
	    MEfill(sizeof(keqcmap), (u_char)0, (char *)&keqcmap);
	    for (key = 0;
		key < varp->opv_mbf.opb_count;
		key++)
	    {
		OPE_IEQCLS	    eqcls1;
		eqcls1 = zbase->opz_attnums[kbase->opb_keyorder[key].opb_attno]->opz_equcls;
		if (BTtest((i4)eqcls1, (char *)&tempmap))
		    BTset((i4)eqcls1, (char *)&keqcmap);
		else
		    break;
	    }
	    if (!key)
	    {
		varp = NULL;		    /* first attribute is not in a boolean factor
					    ** so key cannot be used */
		break;
	    }
	    MEfill(sizeof(tempmap), (u_char)0, (PTR)&tempmap);
	    for (bfi = -1; (bfi = BTnext((i4)bfi, (char *)bfm, (i4)maxbf)) >= 0;)
	    {	/* evaluate the selectivity for each boolean factor */
		OPB_BOOLFACT        *bp3;    /* ptr to boolean factor being 
						** analyzed */
		OPE_BMEQCLS	    *keqcp; /* pointer to keqcmap if any eqc
					    ** are available */
		
		bp3 = bfbase->opb_boolfact[bfi];/* get ptr to next boolean factor */
		if (!(bp3->opb_mask & OPB_ALLMATCH)
		    &&
		    (bp3->opb_mask & OPB_KESTIMATE))
		{
		    if (BTsubset((char *)&bp3->opb_eqcmap, (char *)&keqcmap, (i4)maxeqcls))
			BTor((i4)maxeqcls, (char *)&bp3->opb_eqcmap, (char *)&tempmap);
		    else
			bp3->opb_mask &= (~OPB_KESTIMATE);
		}
	    }
	}
	/* The following refinement of the keysel logic could simply replace
	** the preceding logic. However, in the event that the new logic 
	** skips over legitimate cases, both sets of code will be left for 
	** the time being. The new logic is designed to make Ingres more
	** selective about choosing the Boolean factors which will be used
	** to compute key selectivity for composite key structures. This has
	** never been gotten right in the past, leading to artificially low 
	** I/O estimates being made for scans through composite (multi-
	** column) index structures. */

	/* Refinement of keysel logic. OPB_BFMULTI's appear to be useless for 
	** keying purposes, so we'll exclude them altogether. Of the remaining
	** BOOLFACTs, only retain those with OPB_KEXACT and applicable to
	** the high order key columns PLUS a non-OPB_KEXACT applicable to 
	** the next column after a KEXACT column (i.e., can only compute key
	** selectivity from the high order key cols with "=" predicates AND
	** the first column with a range predicate only). */

	/* BTW, if someone decides that BFMULTI's are useful after all, this
	** code will need significant overhaul. */
	{
	    i4		key;
	    OPB_BOOLFACT *bp2;
	    OPE_IEQCLS	eqcls1, eqcls2;
	    bool	exact, range, found, drange;
	    DB_DT_ID	key_dt;

	    /* First pair of loops looks at key columns, then loops over 
	    ** BOOLFACTs looking for one with OPB_KESTIMATE which covers the 
	    ** key column with an EXACT match. This gives us key col eqclass
	    ** of LAST key column covered by KESTIMATEable factor. */
	    eqcls2 = -1;
	    if (varp) for (key = 0; key < varp->opv_mbf.opb_count; key++)
	    {
		eqcls1 = zbase->opz_attnums[kbase->opb_keyorder[key].opb_attno]
					->opz_equcls;
		key_dt = abs(zbase->opz_attnums[kbase->opb_keyorder[key].opb_attno]
					->opz_dataval.db_datatype);
		for (exact = range = drange = FALSE, bfi = -1; !exact && 
		    (bfi = BTnext((i4)bfi, (char *)bfm, (i4)maxbf)) >= 0; )
		{	/* loop over all BFs for each key eqclass */
		    bp2 = bfbase->opb_boolfact[bfi];
		    if (bp2->opb_mask & OPB_KESTIMATE &&
			bp2->opb_eqcls == eqcls1)
		     if (bp2->opb_mask & OPB_KEXACT)
		      exact = TRUE;	/* got an EXACT on this col - exit */
		     else
		     {
			range = TRUE;
			if (key_dt == DB_DTE_TYPE)
			    drange = TRUE;
		     }
		}
		if (range) eqcls2 = eqcls1;
		if (!exact) break;	/* leave key loop when no match */
	    }
	    /* Second pair of loops looks at BOOLFACTs, then loops over 
	    ** "covered" keys looking for the one (if any) referenced by
	    ** this factor. If key not found, KESTIMATE is turned off. */
	    eqcls1 = eqcls2;
	    if (varp && eqcls1 >= 0)
	     for (bfi = -1; (bfi = BTnext((i4)bfi, (char *)bfm, (i4)maxbf))
			>= 0; )
	    {
		bp2 = bfbase->opb_boolfact[bfi];
		found = FALSE;
		if (bp2->opb_mask & OPB_KESTIMATE)
		 if ((bp2->opb_eqcls == OPB_BFMULTI)
		    ||
		    ((bp2->opb_eqcls != eqcls1) && bp2->opb_orcount && drange))
		    bp2->opb_mask &= (~OPB_KESTIMATE);
					/* clean these up first */
		 else for (key = 0; !found && key < varp->opv_mbf.opb_count; 
									key++)
		{
		    eqcls2 = zbase->opz_attnums[kbase->opb_keyorder[key].
			opb_attno]->opz_equcls;
		    if (bp2->opb_eqcls == eqcls2) found = TRUE;
		    if (eqcls1 == eqcls2) break;
					/* last covered key column */
		}
		if (!found) bp2->opb_mask &= (~OPB_KESTIMATE);
	    }
	}

    }

    /* If this is a leaf node and there are composite histograms associated 
    ** with it, check if they can be used to better estimate selectivity. */
    if (varno >= 0)
    {
	if (subquery->ops_vars.opv_base->opv_rt[varno]->opv_mask & OPV_COMPHIST)
		oph_compbf_setup(subquery, trl, bfm, &charrpp, varno, &chcount);
    }

    {
	OPB_IBF         cbf;		/* current boolean factor being
					** analyzed */
	OPH_SELHIST	sehist;		/* histograms for single eqcls boolean 
					** factors which cannot be used for 
					** keyed lookup */
	OPH_SELHIST	sekhist;	/* histograms for single eqcls boolean 
					** factors which can be used for 
					** keyed lookup */
	OPH_SELHIST	mehist;		/* histograms for multi-eqcls boolean 
					** factors which cannot be used for 
					** keyed lookup */
	OPH_SELHIST	mekhist;	/* histograms for multi-eqcls boolean 
					** factors which can be used for 
					** keyed lookup. NOTE: because of the 
					** rules for KESTIMATE selection, this 
					** set must be empty - no multi-eqclass 
					** BFs could be KESTIMATEs. */
	OPH_SELHIST	chist;		/* non-key composite histograms which
					** map BFs */
	OPH_SELHIST	ckhist;		/* composite histograms which map BFs 
					** that can be used for keyed lookup. 
					** I think there can only be one of these
					** per call */

	chist.oph_histp = NULL;
	chist.oph_predselect = OPN_D100PC;
	ckhist.oph_histp = NULL;
	ckhist.oph_predselect = OPN_D100PC;

	/* Call to compute selectivities attributable to BFs which map onto
	** composite histograms. */
	oph_compcost(subquery, trl, &chist.oph_histp, &ckhist.oph_histp, 
							charrpp, chcount);

	/* Combine remaining BFs which refere3nce single eqclasses. */
	sehist.oph_histp = NULL;
	sehist.oph_predselect = OPN_D100PC;
	sekhist.oph_histp = NULL;
	sekhist.oph_predselect = OPN_D100PC;
	compsehist.oph_histp = NULL;
	compsehist.oph_predselect = OPN_D100PC;
	histlistp = NULL;
	for (cbf = -1; (cbf = BTnext((i4)cbf, (char *)bfm, (i4)maxbf)) >= 0;)
	{   /* create histogram lists of all single equilvalence class boolean
	    ** factors, one list for boolean factors which can be used for 
	    ** keying, and another list for equivalence classes which cannot */
	    OPB_BOOLFACT	*sebfp; /* ptr to boolean factor element
				    ** being analyzed */

	    sebfp = bfbase->opb_boolfact[cbf];
	    if (!(sebfp->opb_mask & OPB_BFUSED)
		&&
		(sebfp->opb_eqcls >= 0) 
		&& 
		sebfp->opb_histogram)
	    {   /* this is a boolean factor which is defined on a single equivalence
		** class, and there are advantages to treating this as a special
		** case, namely less computations, and including some corelation
		** between attributes.  Also, these histograms should define the
		** final shape of the result histogram for this equivalence class
		** since OR's of different equivalence classes are assumed independent
		** and would uniformly increase all other histograms, which is not
		** correct */
		OPH_HISTOGRAM	*bf1histp;  /* histogram of boolean factor */
		OPH_HISTOGRAM	**t1histpp; /* ANDed summary of previous histograms */
		OPH_HISTOGRAM	**sehistpp; /* histogram list to be updated */
		if (sebfp->opb_mask & OPB_BFCOMPUSED)
		    sehistpp = &compsehist.oph_histp;
		else if (varp
		    &&
		    (sebfp->opb_mask & OPB_KESTIMATE)
		    )
		    /* chose either the keyed histogram list or the non-keyed
		    ** histogram list */
		    sehistpp = &sekhist.oph_histp;
		else sehistpp = &sehist.oph_histp;
		sebfp->opb_mask |= OPB_BFUSED;
		bf1histp = sebfp->opb_histogram;
		if (bf1histp->oph_next)
		    opx_error(E_OP04A4_ONLY_ONE); /* only expecting exactly one histogram
						** for this boolean factor */
		if (sebfp->opb_torcount > 0)
		{
		    OPN_PERCENT	    unknown;	/* unknown percentage */
		    OPN_PERCENT	    newarea1;  /* new selectivity of boolean factors
				    ** analyzed so far */

		    if (sebfp->opb_mask & (OPB_SPATF | OPB_SPATJ)) unknown = 
			subquery->ops_global->ops_cb->ops_alter.ops_spatkey;
		    else if (sebfp->opb_mask & OPB_PCLIKE) unknown =
			subquery->ops_global->ops_cb->ops_alter.ops_range;
		    else unknown = subquery->ops_global->ops_cb->ops_alter.ops_nonkey;
		    /* for each clause that we do not recognize assign a selectivity of 
		    ** OPN_DUNKNOWN (default for unknown clauses) */
		    newarea1 = bf1histp->oph_prodarea;
		    while (sebfp->opb_torcount--)
			newarea1 = newarea1 + unknown - newarea1 * unknown;
		    oph_normalize(subquery, bf1histp, newarea1); 
		}
		if (t1histpp = opn_eqtamp (subquery, sehistpp, sebfp->opb_eqcls, FALSE))
		{   /* if this is not the first histo */
		    (*t1histpp)->oph_prodarea = 
			oph_and(subquery, *t1histpp, (*t1histpp)->oph_prodarea,
			    bf1histp, bf1histp->oph_prodarea); /* then AND
					** this one with the first */
		    oph_dhist(subquery, &sebfp->opb_histogram);
		}
		else
		{   /* steal histogram from boolean factor if first time */
		    bf1histp->oph_next = *sehistpp;
		    *sehistpp = bf1histp;
		    sebfp->opb_histogram = NULL;
		}
	    }
	}
	
	mehist.oph_histp = NULL;
	mehist.oph_predselect = OPN_D100PC;
	mekhist.oph_histp = NULL;
	mekhist.oph_predselect = OPN_D100PC;
	{   /* process all remaining boolean factors, but keep the distinction between
	    ** those that can be used for keying and those that cannot */

	    for (cbf = -1; (cbf = BTnext((i4)cbf, (char *)bfm, (i4)maxbf)) >= 0;)
	    {				/* for each boolean factor */
		OPB_BOOLFACT          *bfp; /* ptr to boolean factor element
					** being analyzed */
		OPE_IEQCLS	      eqcls2;	/* current eqcls being analzyed */
		OPN_PERCENT	      newarea;  /* new selectivity of boolean factors
					** analyzed so far */
		OPN_PERCENT	      previous_or; /* selectivity of the conjunct
					** being analyzed, assuming independent
					** events */
		OPH_SELHIST	     *mhistp;	/* ptr to histogram list to be
					** used in processing */

		bfp = bfbase->opb_boolfact[cbf];
		if (bfp->opb_mask & OPB_BFUSED)
		    continue;		/* single histogram equivalence classes have
					** already been processed */
		if (varp
		    &&
		    (bfp->opb_mask & OPB_KESTIMATE)
		    )
		    mhistp = &mekhist;
		else
		    mhistp = &mehist;

		previous_or =
		newarea = 0.0;

		for (eqcls2 = -1; (eqcls2 = BTnext((i4)eqcls2, (char *)&bfp->opb_eqcmap,
		    (i4) maxeqcls)) >= 0;)
		{
		    OPH_HISTOGRAM	**findhpp;  /* find histogram in this
						    ** boolean factor if possible */
		    if (findhpp = opn_eqtamp (subquery, &bfp->opb_histogram, 
			eqcls2, FALSE))
		    {
			OPN_PERCENT	temparea;
			OPH_HISTOGRAM	**temphistpp;
			OPN_PERCENT	intersect;  /* estimate intersection of all
						    ** events */
			if (temphistpp = opn_eqtamp (subquery, &mhistp->oph_histp, eqcls2, FALSE))
			{   /* if this is not the first histo */
			    intersect = (*findhpp)->oph_prodarea;
			    temparea = oph_and (subquery, (*temphistpp), mhistp->oph_predselect,
				*findhpp, intersect); /* then AND
						** this one with the first */
			    (*temphistpp)->oph_prodarea = temparea;
			}
			else
			{
			    OPH_HISTOGRAM    *findhp;
			    findhp = *findhpp;   /* else remember this as the first*/
			    *findhpp = findhp->oph_next; /* unlink this histogram */
			    findhp->oph_next = mhistp->oph_histp;
			    mhistp->oph_histp = findhp;
			    intersect = findhp->oph_prodarea;
			    temparea = mhistp->oph_predselect * intersect; /* assume
						    ** independence, if new equivalence
						    ** class is found */
			}
			/* in order to calculate the correlation, using as much of
			** the existing information, histograms are used for
			** AND'ing prior to OR'ing since it takes advantage of
			** the fact we can do good estimates on restrictions of
			** the same equivalence class, but not independent ones.
			** A AND (B OR C) is translated into 
			**   (A AND B) OR (A AND C)
			** looking at the VENN diagram the selectivity can be
			** calculated as
			**   P(A AND B) + P(A AND C) - P(A AND B AND C)
			** Since B and C are on different equivalence classes, we
			** can assume independence, but this leaves a choice as
			** to evaluate P(A AND B AND C) as P(A AND B) * P(C)
			** or as
			** P(A AND C) * P(B)
			** since the P(A AND C) is calculated using histograms
			** on one equivalence class which is more accurate, and
			** the P(B) is independent and known, this is the preferred
			** way of doing this...
			** make this recursive on subsequent OR's */
			newarea = temparea + newarea - temparea * previous_or; /* an
						    ** OR of correlated events and
						    ** independent events */
			previous_or = previous_or + intersect - previous_or * intersect;
						    /* an OR of independent events */
		    }
		}
		if (bfp->opb_torcount > 0)
		{
		    OPN_PERCENT	    unknown;	/* unknown percentage */

		    if (bfp->opb_mask & (OPB_SPATF | OPB_SPATJ)) unknown = 
			subquery->ops_global->ops_cb->ops_alter.ops_spatkey;
		    else if (bfp->opb_mask & OPB_PCLIKE) unknown =
			subquery->ops_global->ops_cb->ops_alter.ops_range;
		    else unknown = subquery->ops_global->ops_cb->ops_alter.ops_nonkey;
		    /* for each clause that we do not recognize assign a selectivity of 
		    ** OPN_DUNKNOWN (default for unknown clauses) */
		    while (--bfp->opb_torcount)
			unknown = unknown + unknown - unknown * unknown;
		    newarea = unknown * mhistp->oph_predselect + newarea - 
			unknown * mhistp->oph_predselect * previous_or; /* an
						** OR of correlated events and
						** independent events */
		}
		else if (bfp->opb_mask & OPB_COLCOMPARE)
		    newarea = bfp->opb_selectivity;

		/* calculate new area(selectivity) of all the histograms */
		if (mhistp->oph_predselect > newarea)
		    mhistp->oph_predselect = newarea;	/* ANDing should always result in a smaller
						** selectivity */
		else
		    newarea = mhistp->oph_predselect;	/* restore old selectivity to the
						** histograms */
		{   /* normalize the histograms to have equal selectivity */
		    OPH_HISTOGRAM	*hist2p;
		    for (hist2p = mhistp->oph_histp; hist2p; hist2p = hist2p->oph_next)
		    {
			if (!hist2p->oph_default)
			{
			    OPH_HISTOGRAM	*chistp;    /* histogram which provides correlation */		    
			    OPH_HISTOGRAM	*ahistp;    /* in case both keying and non-keying
							    ** histograms, use the ANDed result
							    ** to obtain the correlation */
			    OPH_HISTOGRAM	**hist2pp;
			    OPE_IEQCLS	eqcls4;
			    
			    eqcls4 = abase->opz_attnums[hist2p->oph_attribute]->opz_equcls;

			    if (hist2pp = opn_eqtamp (subquery, &sekhist.oph_histp, 
				eqcls4, FALSE))
			    {
				chistp = *hist2pp;
				if (hist2pp = opn_eqtamp (subquery, &sehist.oph_histp, 
				    eqcls4, FALSE))
				    ahistp = *hist2pp;
				else
				    ahistp = NULL;
			    }
			    else
			    {
				if (hist2pp = opn_eqtamp (subquery, &sehist.oph_histp, 
				    eqcls4, FALSE))
				{
				    chistp = *hist2pp;
				    ahistp = NULL;
				}
				else
				    chistp = NULL;
			    }
			    if (chistp
				&&
				(
				    !subquery->ops_global->ops_cb->ops_check
				    ||
				    !opt_strace( subquery->ops_global->ops_cb, 
					    OPT_F063_CORRELATION)
				))
			    {   /* assume some correlation, by baising the adjustment
				** for the new area to the portion of the histogram
				** selected by single eqcls boolean factors */
				oph_correlation(subquery, hist2p, newarea, chistp, ahistp);
			    }
			}
			if (newarea != hist2p->oph_prodarea)
			    oph_normalize(subquery, hist2p, newarea); /* normalize 
						    ** the histograms
						    ** associated with the boolean factor
						    ** - except for the histogram associated
						    ** with eqcls */
		    }
		}
		oph_dhist(subquery, &bfp->opb_histogram);
	    }
	}

	/* Combine results of various classifications of BFs to produce final
	** selectivity and chain of normalized histograms. Next chunk of code 
	** creates new OPN_RLS and anchors list of modified histograms as
	** requested in eqh parameter (eqclasses needed higher in join tree). */

	/* Combine the key structure BFs first, to produce key selectivity. */
	if (ckhist.oph_histp)
	{
	    /* Produce single selectivity for AND of composite histos. */
	    for (histlistp = ckhist.oph_histp; histlistp;
					histlistp = histlistp->oph_next)
		ckhist.oph_predselect *= histlistp->oph_prodarea;
	    oph_combine(subquery, &mekhist, &ckhist);
	}

	if (sekhist.oph_histp) oph_combine(subquery, &mekhist, &sekhist);

	if (varp && mekhist.oph_histp)
	    varp->opv_keysel = mekhist.oph_predselect;
					/* key structure selectivity (if any) */

	/* Now combine with all the rest to produce overall selectivity. */
	if (chist.oph_histp)
	{
	    /* Produce single selectivity for AND of composite histos. */
	    for (histlistp = chist.oph_histp; histlistp;
					histlistp = histlistp->oph_next)
		chist.oph_predselect *= histlistp->oph_prodarea;
	    oph_combine(subquery, &mekhist, &chist);
	}

	if (sehist.oph_histp) oph_combine(subquery, &mekhist, &sehist);

	if (mekhist.oph_histp && mehist.oph_histp) 
				oph_keyand(subquery, &mehist, &mekhist);
	else if (mehist.oph_histp) STRUCT_ASSIGN_MACRO(mehist, mekhist);
	else mekhist.oph_predselect *= mehist.oph_predselect;
					/* might have nonkey selectivity
					** built up in me[k]hist and
					** NULL in oph_histp */

	/* Assign final selectivity and master histogram list. */
	predselect *= mekhist.oph_predselect;
	histlistp = mekhist.oph_histp;
    }


/* new way to fix bug 7003 -
** Even if the selectivity is 1 or greater, generate new histograms so
** that the complete flag is turned off.  This will cause us to use
** histograms to calculate the number of rows in the join instead of
** assuming it to be the number of rows in the larger relation.  This
** was a problem if the outer relation had one row and a restriction that
** that was satisfied.  Then the selectivity was 1 and the complete flag
** stayed on.  This meant that the number of rows estimated in the join
** would be the larger of the sizes of the two relations.  This is okay
** if there is no restriction on the outer relation, but if there is a
** restriction on the outer relation then it would be better if we use
** histograms to estimate the size of the result. 
*/

    if (predselect > OPN_100PC)
	predselect = OPN_100PC;		/* cannot select more than 100% */
    if (subquery->ops_global->ops_estate.opn_rootflg
	&&
	(   !subquery->ops_global->ops_estate.opn_singlevar
	    ||
	    (subquery->ops_vars.opv_rv == 1)
	)
                                        /* fixes bug in which a single primary
                                        ** used with indexes,... the primary
                                        ** is evaluated first without indexes
                                        ** and is thus at the root, so a new
                                        ** trl is not created, which means
                                        ** that the tuple count is not
                                        ** correct for the trl->opn_reltups
                                        ** since it does not have the
                                        ** any restriction applied, this did
                                        ** not cause a problem in 5.0 but
                                        ** now that subselects require an
                                        ** accurate row count at the root
                                        ** the trl structure should be accurate
                                        */
       )
    {   /* do not create a new relation if at the root */
	/* note that eqh is not initialized in this case, so care must
	** be taken to change this if test */
	newtrl = trl;
	*selapplied = FALSE;
    }
    else
    {
	/* Last section of oph_relselect has been extensively changed to
	** accomodate the processing of composite histograms. Instead of
	** locating the histograms covered by the eqh map and modifying
	** counts, repetition factors, etc. in the same loop, one loop
	** builds a list of the composite histograms to be processed, a
	** second loop builds two more lists (one for histograms in the 
	** Boolean factor list, one for those NOT in the BF list) from
	** eqclasses in the eqh map and a final loop to process all the
	** histograms in the 3 lists in successive passes. */

	OPH_HISTOGRAM	       *eqbhp;	    /* eqh hist list copied from
					    ** BF list */
	OPH_HISTOGRAM	       *eqthp;	    /* eqh hist list copied from
					    ** trl */
	OPH_HISTOGRAM	       *hp1; 
	OPH_HISTOGRAM	       **hist1pp;
	OPH_HISTOGRAM	       *found;
  	OPZ_ATTS               *attrp;      /* ptr to attribute associated
                                            ** with the "hp1" histogram */
	OPZ_IATTS	       attno;
	OPE_IEQCLS             eqclshist;   /* equivalence class for which
                                            ** a histogram is required */
	OPH_DOMAIN	       uniq;	    /* estimate of number of unique
                                            ** values */
        i4		       numcells;    /* number of cells in the histogram
                                            */
	i4		       i;

	/* First, build new trl structure and fill in selectivity. */
	if (predselect < OPN_MINPC)
	    predselect = OPN_MINPC;         /* need non-zero selectivity */
    
        newtrl = opn_rmemory( subquery );   /* get memory for new
                                            ** relation structure */
	newtrl->opn_mask = trl->opn_mask;   /* copy uniqueness mask */
	newtrl->opn_relsel = predselect;
	if ((newtrl->opn_reltups = trl->opn_reltups * predselect) < 1.0 )
	    newtrl->opn_reltups = 1.0;      /* estimated number of tuples
                                            ** in new relation must be at least
                                            ** one */

	/* Merge single eqclass histograms from BFs mapped onto composite 
	** histograms with the remaining single eqclass histograms. Then
	** normalize to selectivity computed from composite histograms. */
	hp1 = compsehist.oph_histp;

	while (hp1)
	{
	    OPH_HISTOGRAM	*thistp, **thistpp;

	    thistp = hp1;
	    hp1 = hp1->oph_next;
	    eqclshist = abase->opz_attnums[thistp->oph_attribute]->
						opz_equcls;
	    if ((thistpp = opn_eqtamp(subquery, &histlistp, eqclshist,
					FALSE)))
	    {
		/* Found matching histogram, AND them and drop the first. */
		oph_and(subquery, (*thistpp), predselect, thistp, 
						thistp->oph_prodarea);
		thistp->oph_next = NULL;
		oph_dhist(subquery, &thistp);
	    }
	    else
	    {
		/* No match - copy to histlistp chain. */
		thistp->oph_next = histlistp;
		histlistp = thistp;
		thistpp = &thistp;
	    }

	    oph_normalize(subquery, (*thistpp), predselect);
	}

	/* Build lists of histograms to be modified and added to new trl. */
	eqbhp = eqthp = (OPH_HISTOGRAM *) NULL;

	for (hist1pp = &histlistp; (*hist1pp); 
					hist1pp = &(*hist1pp)->oph_next)
	 if ((*hist1pp)->oph_mask & OPH_COMPOSITE)
	 {
	    /* Only interested in compound histograms for which the first
	    ** two (at least) attrs are in eqh. */
	    for (i = 0, hp1 = *hist1pp; 
		(attno = hp1->oph_composite.oph_attrvec[i]) != OPZ_NOATTR; i++)
	    {
		eqclshist = abase->opz_attnums[attno]->opz_equcls;
		if (!BTtest((i4)eqclshist, (char *)eqh)) break;
		if (i == 0) continue;

		/* We're here if the first two attrs are in eqh. Histogram
		** qualifies and can be added to list. */
		hp1 = *hist1pp;
		*hist1pp = hp1->oph_next;   /* remove from BF hist list */
		hp1->oph_next = eqbhp;
		eqbhp = hp1;		    /* add to eqh hist list */
		break;
	    }
	    if ((*hist1pp) == NULL) break;	/* removing hist from list 
						** could break loop control */
	 }

	/* Append relevant single eqclass histograms from BF list and build 
	** list of relevant single eqclass histograms NOT in BF list. */
    	for (eqclshist = -1; 
	    (eqclshist= BTnext((i4)eqclshist, (char *)eqh, (i4)maxeqcls))>= 0;
	    )
	{
	    if (hist1pp = opn_eqtamp (subquery, &histlistp, eqclshist, FALSE))
	    {
		/* Histogram in BF list. Just move to new list. */
		hp1 = *hist1pp;
		*hist1pp = hp1->oph_next;   /* remove from BF hist list */
		hp1->oph_next = eqbhp;
		eqbhp = hp1;		    /* add to eqh hist list */
	    }
	    else
	    {	/* There was not a boolean factor involved so the histogram
                ** structure will stay unchanged.
		** - if this trl is a non-leaf, steal the nrmlhist struct
		** from there.  this trl will be deleted, along with
		** any remaining hists by "deltrl" below.  This is because
                ** the trl was created by opn_h2 and contains histograms
                ** which are not required above this node, only select 
                ** histograms which are needed above this node.
                ** - if this trl belongs to a leaf, make a copy of its nrmlhist 
                ** struct because it would belong to an "orig" node which should
                ** remain unchanged.

		** NOTE: this logic stays in the eqclshist loop since it is
		** distinct from the composite and single eqclass BF histogram
		** logic.
		*/
		OPH_HISTOGRAM         **temphp; /* histogram to be copied
                                            ** or used directly */
		bool multiple_eqcls;
		temphp = opn_eqtamp_multi (subquery, &trl->opn_histogram,
		    eqclshist, TRUE, &multiple_eqcls);	    /* where TRUE means mustfind */
		attrp = abase->opz_attnums[(*temphp)->oph_attribute]; /* ptr to
                                            ** attribute which contains interval
                                            ** information for histogram */
		numcells = attrp->opz_histogram.oph_numcells;
		if (nleaves > 1 && !multiple_eqcls)
		{
                    /* FIXME - since oph_fcnt is now pointed to by multiple
                    ** histogram structures without a usage count; the
                    ** element cannot be deleted in any garbage collection
                    ** operation; thus enumeration will eventually run
                    ** out of memory - this opn_fcnt structure is also in the
                    ** left or right subtree */
		    found = *temphp;     /* steal histogram */
                    *temphp = found->oph_next; /* remove from list */
		}
		else
		{   /* leaf found so copy histogram since we do not want to
                    ** clobber leaf node copy */
		    /* FIXME - make sure the oph_fcnt cell count memory is not
                    ** clobbered by a deallocation later ?? */
		    found = oph_memory(subquery); /* get histogram from 
                                            ** memory manager */
                    STRUCT_ASSIGN_MACRO(**temphp, *found); /* copy structure*/

                    /* b122296: 
                    ** allocate oph_fcnt memory and copy originals over. 
                    ** otherwise, 'found' and '*temphp' shares the same oph_fcnt
                    ** area and could cause memory corruption after temphp releases
                    ** its oph_fcnt back to the free memory list. 
                    */
                    found->oph_fcnt = oph_ccmemory(subquery, &attrp->opz_histogram);
                    for (i = 0; i < numcells; i++)
                        found->oph_fcnt[i] = (*temphp)->oph_fcnt[i];

		    found->oph_creptf = oph_ccmemory(subquery, 
						&attrp->opz_histogram);
					/* get repetition factor memory (for
					** possible modification) */
		    for (i = 0; i < numcells; i++)
			found->oph_creptf[i] = (*temphp)->oph_creptf[i];
					/* and copy originals over */
		    if (found->oph_odefined)
		    {	/* check that this is a multi-attribute variable */
			OPV_IVARS	varno;
			varno = BTnext((i4)-1, (char *)&trl->opn_relmap, 
			    (i4)subquery->ops_vars.opv_rv);
			if (subquery->ops_vars.opv_base->opv_rt[varno]->opv_mbf.opb_count
			    < 3)
			    found->oph_odefined = FALSE;    /* make sure simple
					    ** index does not have these checks, this
					    ** is only needed with multi-attribute
					    ** indexes which are 3 or more */
		    }
		}

		found->oph_next = eqthp;
		eqthp = found;		    /* add to non-BF eqh hist list */
	    }
	}

	/* This loop does histogram modifications of old logic, while 
	** incorporating differences required for composite histograms.
	** It processes the BF histogram list, then the trl histogram list. */

	found = (eqbhp) ? eqbhp : eqthp;

	while (found)
	{   /* free up all intermediate normalized histos */
	    OPH_HISTOGRAM	*nexthp;
	    OPO_TUPLES		oldreptf; /* previous rep factor */
	    OPN_PERCENT		factor;
	    i4			i;

	    if (eqbhp)
	    {	/* For BF histograms, integrate to 1 and fiddle the unique
		** value estimates. */
		OPH_INTERVAL	  *intervalp;
                PTR               bottom;   /* ptr to value which is the
                                            ** lower bound of the current 
                                            ** histogram cell being analyzed*/
                OPS_DTLENGTH	  cell_length; /* length of histogram element */
                i4               cell;     /* current cell being analyzed */
                DB_DATA_VALUE     *datatype; /* datatype associated with
                                            ** histogram */
                DB_DT_ID          attrtype; /* type of the attribute
                                            ** (not the histogram element) */
		oph_pdf(subquery, found);    /* normalize the hist to one */
		if (found->oph_mask & OPH_COMPOSITE)
		{
		    intervalp = found->oph_composite.oph_interval;
		    attrtype = DB_CHA_TYPE;
		}
		else 
		{
		    attrp = abase->opz_attnums[found->oph_attribute]; /* ptr to
                                            ** attribute which contains interval
                                            ** information for histogram */
		    intervalp = &attrp->opz_histogram;
		    attrtype = attrp->opz_dataval.db_datatype;
		}

		/* get histogram interval information from attribute */
		bottom = intervalp->oph_histmm;
		numcells = intervalp->oph_numcells;
		datatype = &intervalp->oph_dataval;
		cell_length = datatype->db_length;

		uniq = 0.0;		    /* start with no unique values */
                for (cell = 1; cell < numcells;
		    cell++, bottom = (char *) bottom + cell_length)
		{
		    /* 
		    ** "uniq" is used to estimate the number of unique values
		    ** that could possibly have satisfied the restriction
		    ** predicate.  For a given cell, an estimate of the number
		    ** of unique values after restriction is the same as
		    ** before restriction i.e.
                    **       (found->oph_pcnt[cell] * found->oph_nunique)
		    ** except that it must not be greater than 
                    **    a) the number of elements in the cell after 
                    **       restriction i.e.
                    **           (found->oph_fcnt[cell] * newtrl->opn_reltups)
                    **    b) the number of possible unique values in an 
                    **       integer domain cell 
                    **           (opn_diff(bottom+cell_length,...)).
		    */
		    OPH_DOMAIN	    cellunique; /* estimate of number of
					    ** unique values in this cell */
                    OPO_TUPLES      celltuples; /* number of "tuples" which
                                            ** fall into this cell */
                    cellunique = found->oph_pcnt[cell] * found->oph_nunique;
		    celltuples = found->oph_fcnt[cell] * newtrl->opn_reltups;
                    if (cellunique > celltuples)
		        cellunique = celltuples;
		    if (attrtype == DB_INT_TYPE)
		    {	/* if original attribute was an integer then there
                        ** cannot be more unique values than are in the
                        ** interval.
			**
			** NOTE: use of attrp is safe here, since composite
			** histograms are always DB_CHA_TYPE. */
			OPN_DIFF	temp1;
			    
			if ((temp1 = opn_diff((char *)bottom + cell_length,
					bottom, &cell_length,
					celltuples, attrp, 
					&attrp->opz_histogram.oph_dataval, tbl))
				< cellunique)
                            cellunique = temp1;
		    }
		    uniq += cellunique;	    /* add estimate to total for
                                            ** relation */
		}
	    }	/* end of histograms from BFs */
	    else uniq = found->oph_nunique;

	    nexthp = found->oph_next;
	    found->oph_next = newtrl->opn_histogram; /* add hist to the new 
					    ** relation */
	    newtrl->opn_histogram = found;
	    /* found->oph_complete = FALSE;   */
	    /* found->oph_unique stays the same */
	    /* 
	    ** in the following statement notice that eprime(r,m,n) can be
	    ** thought of as the number of non-empty cells when r balls are
	    ** placed randomlly in n cells where each cell can have a max of
	    ** m balls. the n cells are the original number of unique values.
	    ** m is the number of tuples with a given unique value (e.g. the
	    ** original repetition factor). m*n is the original number of 
	    ** tuples and r is the new number of tuples. eprime(r,n,m) gives
	    ** the new number of unique values.
	    */
	    oldreptf = found->oph_reptf;    /* save the original one */
	    found->oph_nunique = opn_eprime(newtrl->opn_reltups, 
		found->oph_reptf, uniq,
		subquery->ops_global->ops_cb->ops_server->opg_lnexp);
	    if (found->oph_nunique < 1.0)
		found->oph_nunique = 1.0;   /* found->oph_reptf and 
                                            ** found->oph_nunique are the values
					    ** from the previous histogram.
					    ** "uniq" takes into consideration
                                            ** any sargable boolean factors 
					    */
	    found->oph_reptf = newtrl->opn_reltups / found->oph_nunique;
            if (found->oph_reptf < 1.0 )
		found->oph_reptf = 1.0;
	    if (oldreptf != found->oph_reptf && oldreptf != 0.0)
	    {
		if (found->oph_mask & OPH_COMPOSITE)
		    numcells = found->oph_composite.oph_interval->oph_numcells;
		else
		    numcells = abase->opz_attnums[found->oph_attribute]->
						opz_histogram.oph_numcells;
		factor = found->oph_reptf / oldreptf;
		for (i = 0; i < numcells; i++)
		    found->oph_creptf[i] *= factor;
			/* reset per-cell rep facts based on change in
			** relation-wide rep fact */
	    }

	    found = nexthp;
	    if (!found)
	     if (eqbhp)
	    {
		eqbhp = (OPH_HISTOGRAM *) NULL;
		found = eqthp;
	    }
	}	/* end of histogram processing loop */
	if (nleaves > 1)
	    opn_deltrl (subquery, trl, jeqc, nleaves);
    }
    oph_dhist (subquery, &histlistp);

    *selectivity = predselect;
    newtrl->opn_relsel = predselect;
    *trlpp = newtrl;
}
