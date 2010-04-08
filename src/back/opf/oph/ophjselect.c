/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <fp.h>
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
#define        OPH_JSELECT      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>
#include    <adulcol.h>

/**
**
**  Name: OPHJSELECT.C - routines to create histograms for join of left and right
**
**  Description:
**      This file contains routines which will be used to create histograms 
**      for the join of left and right subtrees
**
**  History:
**      1-jun-86 (seputis)    
**          initial creation
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-aug-93 (ed)
**	    fix outer join estimate problem in which resetting tuple counts
**	    when hitting "limits" did not reset the null counts for outer joins
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	19-jan-94 (ed)
**	    correct histogram estimate problem for NULLs on an inner attribute
**	15-jan-96 (inkdo01)
**	    Added per-cell repetition factor support (new join cardinality 
**	   formulas).
**	7-mar-96 (inkdo01)
**	    Modified complete flag processing to use predicate selectivities
**	    in face of restrictions on complete tables.
**	29-Mch-96 (prida01)
**	    Remove problem with E_OP0489_NOFUNCHIST bug 74932	    
**      12-Jun-97 - X-Integration of change 426674
**         12-Jul_96 (mckba02)
**             #76191, modified oph_join.  WHen converting a histogram
**             boundary values to DB_CHA_TYPE, ensure that there are nunique
**             and density values for any new char positions created.
**	25-jun-97 (hayke02)
**	    In the function oph_jselect(), we now only return E_OP04AF if
**	    ojhistp is non NULL. E_OP04AF is an outer join error, so we
**	    will only return it if this is an outer join. This change fixes
**	    bug 83309.
**	24-may-99 (sarjo01)
**	    Bug 97032: assure that zero valued left and right per-cell
**	    rep factors are adjusted to 1.0 prior to calculations.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**      19-Apr-02 (bolke01)
**          Bug 107406: The 70401 error is generated when the Right
**	    node histogram is selected and the left node hiostogram is
**	    longer.  Added addition setting of the number of elements in 
**	    the node to ensure correct length.
[@history_line@]...
**/

    /* Static stuff. */
static OPO_TUPLES
oph_join2(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *lhistp,
	OPH_HISTOGRAM      *rhistp,
	OPO_TUPLES         ltuples,
	OPO_TUPLES         rtuples);


/*{
** Name: oph_join	- number of tuples, and histogram from a join
**
** Description:
**	This routine calculates number of tuples in join of two relations.
**      The histogram of the joining attribute is returned if requested.
**
**	Find the number of tuples resulting from the
**	join of two relations, l and r based on the histograms
**	for the joining attributes.
** 
**	Assume that joins can occur only on atts in the same eqclass
**	and hence the same type (but possibly different length).
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      lhistp                          ptr to histogram element for left
**                                      joining attribute
**      rhistp                          ptr to histogram element for right
**                                      joining attribute
**      ltups                           number of tuples in the left
**                                      relation to be joined
**      rtups                           number of tuples in the right
**                                      relations to be joined
**      histflag                        TRUE - if histogram for result
**                                      of join is requested to be returned
**                                      via jhistpp
**
** Outputs:
**      jhistpp                         ptr to location to return ptr
**                                      histogram of join
**	Returns:
**	    estimated number of tuples from a join of the relations
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jun-86 (seputis)
**          initial creation from histj in histj.c
**	24-jul-89 (jrb)
**	    Changed check to also check precision when type is decimal.  It
**	    looks like this routine assumes a lot about the datatypes (esp.
**	    where it's making decisions about from and to types for conversion)
**	    and I don't know how decimal is really supposed to fit here.
**	6-feb-90 (seputis)
**	    - do not compute cell zero of fcnt, since it is not initialized
**      29-may-92 (seputis)
**          - bug 44425 - uninitialized memory, causes a "NaN" to be used
**          causing an infinite (very long) loop in opneprime, even though a check
**          for a max of 50 is made there
**	09-nov-92 (rganski)
**	    Changed parameters in opn_diff() call. The
**	    charlength parameter is set to 0 when computing diff
**	    between cell boundary values, so that diff, when the
**	    attribute is of a character type, is computed as far as
**	    necessary for accuracy. Then overlap is computed to the
**	    same character position. This means that overlap may need
**	    to be computed twice, if ldiff and rdiff require
**	    different lengths. For this reason I replaced the variable overlap
**	    with 2 variables, loverlap and roverlap.
**	    Added variables leftcelltups and rightcelltups, which are the
**	    number of tuples in the current left and right cells. Also added
**	    rcharlength and lcharlength, which are used as the charlength 
**	    parameter.
**	    Moved computation of bd so that it is only done for float types.
**	14-dec-92 (rganski)
**	    Added datatype parameter to calls to opn_diff: these diffs
**	    may be computed in the context of a histogram
**	    whose datatype has been converted to the type of the
**	    other column in the join; in this case the datatype
**	    information in attrp is incorrect.
**	15-feb-93 (ed)
**	    - retrofit 6.4 bug fixes, also prevent double evaluation of
**	    estimates if lhistp==rhistp==newhp
**	11-feb-94 (ed)
**	    - changes from outer join walkthru, fix incorrect calculation
**	    of full join tuple estimate for cell
**	15-jan-96 (inkdo01)
**	    Added per-cell repetition factor support (new join cardinality 
**	   formulas).
**	12-june-97 (inkdo01 from natjo01)
**	    Make sure we switch offf the left histogram selection indicator
**	    if using right histogram (for left/right ojs nested under fulls).
**	12-june-97 (inkdo01 from 12-Ju-96 mckba02)
**        #76191, Character histogram enhancements expect to be able to
**        find a chardensity and charnunique value for every character
**        position in a histogram boundary value of DB_CHA_TYPE.  This
**        was not the case when boundary values were being converted to
**        DB_CHA_TYPE.  Expand the chardensity and charnunique arrays
**        to the new size of the boundary values.  Values in these
**        arrays for the new char positions are intialized to the
**        default values for the array.
**    9-Jun-97 (i4jo01)
**        Make sure we switch off the left histogram selection indicator
**        if using right histogram.
**	16-mar-99 (inkdo01)
**	    Refinements in computation of per-cell rep factors in join hists.
**	29-jun-99 (hayke02)
**	    Implement new dbms parameter opf_stats_nostats_max and trace
**	    points op205, op206. If opf_stats_nostats_max or trace point
**	    op205 is set, we limit the tuple estimates for joins between
**	    table with stats and table without by assuming uniform data
**	    distribution. The max tuple estimate will be:
**	    <rows_in_tab_with_stats> x <rows_in_tab_without_stats>
**	    ------------------------------------------------------
**	               <unique_values_in_tab_with_stats>
**	    Trace point op206 swtiches off both opf_stats_nostats_max and
**	    op205. Existing trace point op201 (use default stats for
**	    joins between table with and without stats) overrides both
**	    opf_stats_nostats_max, op205 and op206. This change fixes bug
**	    97450.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	21-feb-02 (inkdo01)
**	    Changed opu_memory calls to opn_memory (for char stats) to avoid
**	    OP0002 when memory could be from recoverable enumeration stream.
**	22-jul-02 (hayke02)
**	    Added ops_stats_nostats_factor to scale the opf_stats_nostats_max
**	    value (see comments for bug 97450 above for details). This
**	    change fixes bug 108327.
**	9-jan-04 (inkdo01)
**	    Sanity check inserted to assure join of 2 cells doesn't 
**	    contribute more rows than in the whole table.
**	25-oct-05 (inkdo01)
**	    Added xDEBUG to display some useful join stats.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
[@history_line@]...
*/
static OPO_TUPLES
oph_join(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *orig_lhistp,
	OPH_HISTOGRAM      *orig_rhistp,
	OPO_TUPLES         ltups,
	OPO_TUPLES         rtups,
	OPH_HISTOGRAM      **jhistpp,
	bool               histflag,
	OPL_OJHISTOGRAM    *ojhistp)
{
    OPH_CELLS           leftlower;	/* lower boundary of left histogram
                                        ** cell currently being analyzed */
    OPH_CELLS           rightlower;     /* lower boundary of right histogram
                                        ** cell currently being analyzed */
    OPH_CELLS           templower;	/* lower boundary of newly created
                                        ** histogram cell currently being
                                        ** analyzed - this array of cell
                                        ** boundaries is allocated and
                                        ** deallocated within this procedure
                                        ** if it is needed in case of a
                                        ** histogram conversion */
    OPH_HISTOGRAM	*lhistp;
    OPH_HISTOGRAM	*rhistp;
    DB_DATA_VALUE       *datatype;      /* ptr to datatype used for cell by
                                        ** cell histogram processing */
    i4                  newhp_numcells; /* number of cells in the new histogram
                                        ** used only if requested by the user 
                                        ** i.e. if histflag is set */
    OPZ_AT              *abase;         /* ptr to base of array of ptrs to 
                                        ** joinop attributes */
    OPH_HISTOGRAM       *newhp;         /* equal to either lhistp or rhistp
                                        ** depending on whether the left or
                                        ** or right histogram is being used
                                        ** to model interval information for
                                        ** the histogram of the join */
    OPH_HISTOGRAM       *jhistp;        /* ptr to new histogram element if
                                        ** requested by caller */
    OPZ_ATTS            *lattrp;	/* ptr to joinop attribute of the
                                        ** left histogram */
    OPZ_ATTS            *rattrp;        /* ptr to joinop attribute of the
                                        ** right histogram */
    OPO_TUPLES          totaltups;	/* total estimated number of tuples
                                        ** resulting from the join */
    bool		defaults;       /* TRUE - if default histograms were
                                        ** used on the right and left sides */
    bool		use_righthistp;	/* TRUE - if the right histogram should
					** be used as a template for the new
					** histogram, FALSE if the left histogram
					** should be used instead, 
					** -previously PTRs were compared which
					** resulted in double computations */
    bool		fastpath = FALSE;  /* TRUE - if outer join adjustments 
					** to inner join estimates should be made
					** directly to the cells on this pass */
    OPO_TUPLES          rightojnull; /* number of tuples which were kept
                                        ** due to outer join semantics */
    OPO_TUPLES		leftojnull; /* count of number tuples which
                                        ** were included due to outer join
                                        ** semantics */
    bool		null_switch_flag; /* TRUE if outer join left and right
					** operands are switched, requiring that
					** NULL calculations also be switched */
    OPH_COUNTS          saved_chardensity;
    i4                  *saved_charnunique = NULL;
    OPZ_ATTS            *saved_attrp = NULL;

    OPL_OJTYPE          ojtype;
    OPS_STATE		*global;
    OPO_TUPLES		maxonehisttups;
    bool		onehist = FALSE;
    PTR			tbl = NULL;		/* collation table */
#ifdef xDEBUG
    OPO_TUPLES		tottup1 = 0.0;
#endif

    global= subquery->ops_global;
    abase = subquery->ops_attrs.opz_base; /* get ptr to base of array of ptrs to
					** joinop attributes */

    if (global->ops_gmask & OPS_COLLATEDIFF)
        tbl = global->ops_adfcb->adf_collation;

    lhistp = orig_lhistp;
    rhistp = orig_rhistp;
    null_switch_flag = FALSE;
    use_righthistp = FALSE;
    rightojnull = leftojnull = 0.0;
    if (ojhistp)
    {
        ojtype = ojhistp->opl_ojtype;
	fastpath = !(ojhistp->opl_ojmask & OPL_OJONCLAUSE);
    }
    else
        ojtype = OPL_INNERJOIN;
    newhp = NULL;
    if (lhistp->oph_default != rhistp->oph_default)
    {	/* one default histogram and one real histogram is used, so assume
        ** that the distribution of the real histogram is more accurate and
        ** use it exclusively - FIXME use keying information to modify
        ** a default histogram appropriately, i.e. if r.a >"abc" AND r.a <"ccc"
        ** then use the default selectivity, and modify histogram to be
        ** uniformly distributed in the interval defined by the keys, and
        ** ignore the arbitrary bounds defined in oph_histos */
	if (global->ops_cb->ops_check
	    &&
	    opt_strace(global->ops_cb, OPT_F073_NODEFAULT)
	   )
	   defaults = TRUE;
	else
	{
	    if (lhistp->oph_default)
	    {
		lhistp = rhistp;
		maxonehisttups = ltups * rtups / rhistp->oph_nunique;
	    }
	    else
	    {
		rhistp = lhistp;
		maxonehisttups = ltups * rtups / lhistp->oph_nunique;
	    }
	    defaults = FALSE;
	    onehist = TRUE;
	}
    }
    else 
    {
	defaults = lhistp->oph_default;	    /* TRUE if default histograms are
                                            ** used on both side */
	if (ojhistp && !defaults)
	{   /* define interval information based on outer join histogram
	    ** in order to model the result more accurately, otherwise
	    ** parts of the equijoin histogram may not be represented in the
	    ** outer join since the wrong histogram may not span the entire
	    ** domain of outer join values, whereas the right histogram by
	    ** definition will span that domain */
	    switch (ojtype)
	    {
	    case OPL_LEFTJOIN:
		newhp = lhistp;
		break;
	    case OPL_RIGHTJOIN:
		newhp = rhistp;
		break;
	    }
	}
	if (abase->opz_attnums[lhistp->oph_attribute]->opz_histogram.oph_numcells 
	    < 
	    abase->opz_attnums[rhistp->oph_attribute]->opz_histogram.oph_numcells)
	{   /* switch so lhistp has more histo cells than rhistp (for 
	    ** convenience below) 
	    */
	    OPH_HISTOGRAM          *temphistp;  /* used to switch histograms */
	    OPO_TUPLES             temptups;    /* used to switch tuple counts */

	    temphistp = lhistp;		    /*switch left and right histograms*/
	    lhistp = rhistp;
	    rhistp = temphistp;
	    temptups = ltups;		    /*switch left and right tup counts*/
	    ltups = rtups;
	    rtups = temptups;

            if (ojhistp)
            {   /* switch outer join types since histograms have switched */
		null_switch_flag = TRUE;
                switch (ojtype)
                {
                case OPL_LEFTJOIN:
                    ojtype = OPL_RIGHTJOIN;
                    break;
                case OPL_RIGHTJOIN:
                    ojtype = OPL_LEFTJOIN;
                    break;
		case OPL_FULLJOIN:
		case OPL_INNERJOIN:
		    break;
                default:
		    opx_error(E_OP04AC_JOINID);	    /* unexpected join id type */
                    break;
                }
            }
	}
    }
    {
	lattrp = abase->opz_attnums[lhistp->oph_attribute]; /* get ptr to joinop
                                            ** attribute of left histogram */
	rattrp = abase->opz_attnums[rhistp->oph_attribute]; /* get ptr to joinop
                                            ** attribute of right histogram */
	leftlower = lattrp->opz_histogram.oph_histmm; /* get ptr to lower
                                            ** boundary value for first cell of
                                            ** left histogram */
	rightlower = rattrp->opz_histogram.oph_histmm; /* get ptr to lower
                                            ** boundary value for first cell of
                                            ** right histogram */
	templower = NULL;                    /* new histogram may not need to
                                            ** be created so initialize this
                                            ** value to NULL to indicate this*/
	if (	(lattrp->opz_histogram.oph_dataval.db_datatype 
		!=			    /* are left and right types equal */
		rattrp->opz_histogram.oph_dataval.db_datatype)
	    || 
		(lattrp->opz_histogram.oph_dataval.db_length 
		!=			    /*are left and right lengths equal*/
		rattrp->opz_histogram.oph_dataval.db_length)
	    ||
		(abs(lattrp->opz_histogram.oph_dataval.db_datatype)
			== DB_DEC_TYPE  &&
		(lattrp->opz_histogram.oph_dataval.db_prec
		!=		
		rattrp->opz_histogram.oph_dataval.db_prec))
	    )
	{   /* mismatched types, one of the histos will have to be converted */
	    OPZ_ATTS           *fromattrp;  /* ptr to attribute to convert
					    ** histogram type from */
	    OPZ_ATTS           *toattrp;    /* ptr to attribute to convert
                                            ** histogram type to */
	    if (    (lattrp->opz_histogram.oph_dataval.db_datatype ==DB_INT_TYPE
		    &&
		    rattrp->opz_histogram.oph_dataval.db_datatype ==DB_FLT_TYPE)
		||
		    (	(lattrp->opz_histogram.oph_dataval.db_datatype 
			== 
			rattrp->opz_histogram.oph_dataval.db_datatype)
		    && 
			(lattrp->opz_histogram.oph_dataval.db_length 
			< 
			rattrp->opz_histogram.oph_dataval.db_length)
		    ))
	    {	/* convert right to left histogram if
		**  1) left histogram type is integer and right histogram type
                **     is float OR
                **  2) if types are equal , but the left length is shorter
		*/
		fromattrp = lattrp;	    /* convert from left attribute */
		toattrp = rattrp;           /* convert to right attribute */
		if (!newhp)
		    newhp = rhistp;	    /* use the right histogram
					    ** if a new histo is needed */
	    }
	    else
	    {   /* convert left to the right histogram */
		fromattrp = rattrp;	    /* convert from right attribute */
		toattrp = lattrp;	    /* convert to left attribute */
		if (!newhp)
		    newhp = lhistp;	    /* use the left histogram
                                            ** if a new histogram is needed */
	    }
	    /* the following is independent of outer joins since this code
	    ** ensures the correct type is used in cell comparisons whereas
	    ** newhp specifies the result type, which may be different
	    ** from the comparison type, since the original interval information
	    ** for an attribute is not changed and only the array of floating pt
	    ** cell counts are modified */
	    {	/* create a new "cell boundary" array and convert its elements*/
 		OPH_CELLS             tocell;/* lower boundary of current cell
                                            ** being converted to */
		OPH_CELLS             fromcell; /* lower boundary of current 
                                            ** cell being converted from */
		i4                   cell; /* number of cells remaining to be
                                            ** converted */
		OPS_DTLENGTH          fromlength; /* length of cell boundary
                                            ** element to be converted */
		OPS_DTLENGTH          tolength; /* length of cell boundary
                                            ** element to be converted to */
                DB_DATA_VALUE         *fromtype; /* type and length to be
                                            ** converted from */
                DB_DATA_VALUE         *totype; /* type and length to be
                                            ** converted to */
		ADF_CB                *adf_scb; /* adf session control block */
		tocell = templower = oph_cbmemory(subquery, (i4)
		    (fromattrp->opz_histogram.oph_numcells * 
		    toattrp->opz_histogram.oph_dataval.db_length));/* get 
					    ** memory for the new array of cell
					    ** boundaries */
                fromlength = fromattrp->opz_histogram.oph_dataval.db_length; /*
                                            ** old length of cell boundary */
                tolength = toattrp->opz_histogram.oph_dataval.db_length; /*
                                            ** new length of cell boundary */
                fromtype = &fromattrp->opz_histogram.oph_dataval; /* old 
                                            ** datatype of cell boundary */
                totype = &toattrp->opz_histogram.oph_dataval; /* new datatype
                                            ** of cell boundary */
                datatype = totype;          /* save ptr to datatype to be used
                                            ** for histogram processing */
		fromcell = fromattrp->opz_histogram.oph_histmm; /* base of array
                                            ** of histogram boundary elements
                                            ** to convert */
                adf_scb = global->ops_adfcb; /* adf session 
                                            ** control block */
		for (cell = fromattrp->opz_histogram.oph_numcells; cell--;)
		{
		    DB_STATUS	    cvintostatus; /* ADF return status */
		    fromtype->db_data = (PTR) fromcell; /* source value for
                                            ** conversion */
                    totype->db_data = (PTR) tocell; /* destination of conversion
                                            */
		    cvintostatus = adc_cvinto( adf_scb, fromtype, totype); /* 
                                            ** convert old cell boundary element
                                            ** to new cell type */
# ifdef E_OP0788_ADC_CVINTO
		    if (cvintostatus != E_DB_OK)
			opx_verror(cvintostatus, E_OP0788_ADC_CVINTO,
			    adf_scb->adf_errcb.ad_errcode);
# endif
		    fromcell += fromlength; /* increment to next cell */
		    tocell += tolength;     /* increment to next cell */
		}
 
                /*  
                    For DB_CHA_TYPES/DB_BYTE_TYPE, to avoid errors in opn_chardiff we have 
                    to extend the chardensity and charnunique lists for the
                    converted boundary. Changing the length of the boundary
                    adds new character positions to it for which no nunique and
                    density values have been determined. .#76191.
                */
                if((totype->db_datatype == DB_CHA_TYPE ||
		    totype->db_datatype == DB_BYTE_TYPE)
                && (totype->db_length != fromtype->db_length))
                {
                    i4  i;
 
                    /* Keep track of which pointers we have modified so
                       we can fix them up later. */
 
                    saved_attrp = fromattrp;
                    saved_chardensity = fromattrp->opz_histogram.oph_chardensity;
                    saved_charnunique = fromattrp->opz_histogram.oph_charnunique;       
 
                    /* allocate some memory for the new temporary arrays */
 
                    fromattrp->opz_histogram.oph_chardensity = 
                                (OPH_COUNTS) opn_memory(global,
                                (i4) (sizeof(OPN_PERCENT) * totype->db_length));
 
                    fromattrp->opz_histogram.oph_charnunique =
                                (i4 *) opn_memory(global,
                                (i4) (sizeof(i4) * totype->db_length));
                
                    /* copy original values into temporary array */
        
                    for(i=0; i < fromtype->db_length; i++)
                    {
                        fromattrp->opz_histogram.oph_charnunique[i] =
                                saved_charnunique[i];
                        fromattrp->opz_histogram.oph_chardensity[i] =
                                saved_chardensity[i];
                    }
 
                    /* now add the default values to the extra char positions */
 
                    for(i=fromtype->db_length; i < totype->db_length; i++)
                    {
                        fromattrp->opz_histogram.oph_charnunique[i] =
                                DB_DEFAULT_CHARNUNIQUE;
                        fromattrp->opz_histogram.oph_chardensity[i] =
                                DB_DEFAULT_CHARDENSITY;
                    }
                }
 
		fromtype->db_data = NULL;   /* just in case */
                totype->db_data = NULL;     /* just in case */
	    }
	    if (fromattrp == lattrp)
		leftlower = templower;	/* left histogram was converted so save
                                        ** base of new array of cell 
                                        ** boundaries in left */
	    else
		rightlower = templower; /* right histogram was converted so save
                                        ** base of new array of cell
                                        ** boundaries here */
	}
	else
	{
	    if (!newhp)
		newhp = lhistp;		/* the histogram with more cells will 
                                        ** be used */
            datatype = &lattrp->opz_histogram.oph_dataval; /* save ptr to
                                        ** datatype to be used for histogram
                                        ** processing */
	}
    }

    if (histflag)			
    {	/* if the caller wants histo of join result */
	OPH_INTERVAL              *intervalp; /* ptr to histogram info
                                       ** associated with attribute */
	i4			  i;	/* loop counter */
        intervalp = &abase->opz_attnums[newhp->oph_attribute]->opz_histogram;
	jhistp = oph_memory(subquery); /* get memory for new histogram ptr */
        STRUCT_ASSIGN_MACRO((*newhp), (*jhistp)); /* copy information */
	jhistp->oph_next = NULL;	/* just in case */
	newhp_numcells = intervalp->oph_numcells;
	jhistp->oph_fcnt = oph_ccmemory(subquery, intervalp); /* get
					** new memory for the cell count array
                                        */
	jhistp->oph_creptf = oph_ccmemory(subquery, intervalp); /* get same
					** amount of memory for repetition    
					** factor array */
	for (i = 0; i < newhp_numcells; i++) /* init the cells (to be safe) */
	{
	    jhistp->oph_fcnt[i] = 0.0;
	    jhistp->oph_creptf[i] = 0.0;
	}
        jhistp->oph_null = 0.0;         /* no overlap with NULLs */
	if (newhp == orig_lhistp)
	    jhistp->oph_mask |= OPH_OJLEFTHIST; /* flag indicating that
					** the interval information was
					** taken from the left histogram */
	else
	   jhistp->oph_mask &= ~OPH_OJLEFTHIST; /* make sure it's off */
    }
    use_righthistp = (newhp != lhistp);

    {
	i4                    leftcell;/* current cell of left histogram being
                                        ** analyzed */
        i4                     rightcell; /* current cell of right histogram
                                        ** being analyzed */
	i4                    maxleftcells; /* total number of cells in left 
                                        ** histogram to be analyzed */
        i4                     maxrightcells; /* total number of cells in right
                                        ** histogram to be analyzed */
	OPS_DTLENGTH           celllength; /* length of a cell boundary element
                                        ** which should be the same for the
                                        ** left and right histogram */
	OPH_CELLS              rightupper; /* ptr to upper boundary of current
                                        ** cell being analyzed from the 
                                        ** right histogram */
 	OPH_CELLS              leftupper; /* ptr to upper boundary of the
                                        ** current cell being analyzed from the
                                        ** left histogram */
	OPN_DIFF               rdiff;   /* a measure of how different two
                                        ** histogram values are */
	OPO_TUPLES	       rightcelltups; /* number of tuples in current
					** right histogram cell */
	OPO_TUPLES             rightjoin; /* estimated tuples (after join) which
                                        ** would fall in the current right 
                                        ** histogram cell being analyzed */
#if 0
        OPO_TUPLES             trighttups; /* number of tuples in the right part
                                        ** of the histogram operation */
#endif
	OPO_TUPLES	       rightreptf; /* right cell repetition factor */
	OPO_TUPLES	       leftreptf; /* left cell repetition factor */
	OPO_TUPLES	       rcard, lcard; /* right/left join cardinalities */
	OPO_TUPLES	       jnuniq;	/* joined cell unique value counter */
	i4		       rcharlength; /* length to which string diff
					** should be computed, for calls to
					** opn_diff */
    
        maxleftcells = lattrp->opz_histogram.oph_numcells; /* number of cells to
                                        ** analyzed - note that left count
                                        ** should be greater than right */
        maxrightcells = rattrp->opz_histogram.oph_numcells; /* number of cells
                                        ** in right histogram to be analyzed */
        celllength = datatype->db_length; /* get length of cell boundary element
                                        */
        /* trighttups = */ rightjoin = totaltups = 0.;
	rightcell = 1;                  /* start with first cell of right
                                        ** histogram */
        rightupper = rightlower + celllength; /* initialize ptr to upper
                                        ** boundary of first cell */
	rightcelltups = rtups * rhistp->oph_fcnt[rightcell];
	rightreptf = rhistp->oph_creptf[rightcell];
        if (rightreptf < 1.0)
            rightreptf = 1.0;
	jnuniq = 0.0;			/* init join cell unique count */
	rcharlength = 0;		/* if char type, compute diff as far
					** as necessary for accuracy */
	if (rightcelltups == 0)
	    /* Don't bother computing diff if there are no tuples */
	    rdiff = 0;
	else
	    rdiff = opn_diff(rightupper, rightlower, &rcharlength,
			     rightcelltups, rattrp, datatype, tbl);
    
        /* leftlower and rightlower - have been initialized earlier to the
        ** lower bound of the first cell of the left and right histograms
        ** respectively
        */
	for (leftcell = 1; leftcell < maxleftcells; leftcell++)
	{
	    OPN_DIFF           ldiff;   /* a measure of how different the
                                        ** the lower and upper bounds of the
                                        ** the current cell being analyzed
                                        ** of the left histogram */
	    OPO_TUPLES	       leftcelltups; /* number of tuples in current
					** left histogram cell */
	    OPO_TUPLES	       leftjoin; /* estimated tuples (after join) which
                                        ** would fall in the current left
                                        ** histogram cell being analyzed */

	    i4		       lcharlength; /* length to which string diff
					** should be computed, for calls to
					** opn_diff */

            /* tlefttups =  */ leftjoin = 0.0; /* start with 0 tuples */
            leftupper = leftlower + celllength; /* set ptr to upper boundary
                                        ** of current cell being analyzed
                                        ** of left histogram */
	    leftcelltups = ltups * lhistp->oph_fcnt[leftcell];
	    leftreptf = lhistp->oph_creptf[leftcell];
	    if (leftreptf < 1.0)
		leftreptf = 1.0; 
	    if (!use_righthistp) jnuniq = 0.0;
	    lcharlength = 0;

#ifdef xDEBUG
	    if ((leftcell / 10)*10 == leftcell || leftcell == 1)
		TRdisplay("\n %d - ", leftcell);
	    TRdisplay("   %f", totaltups-tottup1);
	    tottup1 = totaltups;
#endif

	    /* get difference of current left cell interval */
	    if (leftcelltups == 0.0)
		/* Don't bother computing diff if there are no tuples */
		ldiff = 0.0;
	    else
		ldiff = opn_diff(leftupper, leftlower, &lcharlength,
				 leftcelltups, lattrp, datatype, tbl);

	    while (rightcell < maxrightcells)
	    {
		OPH_CELLS             minupper; /* ptr to minimum of upper
                                        ** bounds for current left and right
                                        ** histogram cells being analyzed */
		OPH_CELLS             maxlower; /* ptr to maximum of lower
                                        ** bounds for current left and right
                                        ** histogram cells being analyzed */
		OPN_DIFF              loverlap;
		OPN_DIFF              roverlap; /* these are a measure of the
                                        ** overlap between the left and right
                                        ** histogram cells being analyzed
                                        ** - if it is positive then there is
                                        ** some fraction of the cell intervals
                                        ** which overlap
					** There is a different overlap
					** variable for left and right, since
					** charlength may be different */

		minupper = oph_minptr(global, leftupper, rightupper, datatype);
                                        /* return ptr to minimum of left and 
                                        ** right upper bounds */
                maxlower = oph_maxptr(global, leftlower, rightlower, datatype);
                                        /* return ptr to maximum of left and 
                                        ** right lower bounds */
		
		if (ojhistp
		    &&
		    (loverlap = opn_diff(minupper, maxlower, &lcharlength,
					leftcelltups, lattrp, datatype, tbl))
		              > 0.0)
		{
		    /* check for boundary conditions of first or last
		    ** overlapping cell */
		    OPN_DIFF    temp_overlap;
		    OPO_TUPLES	special_temptotal;
		    if ((rightcell == 1)
			&&
			(   (ojtype == OPL_LEFTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(maxlower == rightlower)
			&&
			(ldiff > 0.0)
			)
		    {   /* special case of first overlapping histogram cell
			** in which the left cell may have some part of its
			** tuples not overlapped with the right cell */
			temp_overlap = opn_diff(maxlower, leftlower, &lcharlength,
				leftcelltups, lattrp, datatype, tbl);
			if (temp_overlap > 0.0)
			{   /* get NULL contribution from first overlap cell since
			    ** left cell lower boundary is lower than right cell
			    */
			    special_temptotal = leftcelltups * temp_overlap / ldiff; 
			    leftojnull += special_temptotal;
			    if (fastpath)
			    {
				if (!use_righthistp)
				    jnuniq += special_temptotal / leftreptf;
				leftjoin += special_temptotal;
			    }
			}
		    }
		    else if ((leftcell == 1)
			&&
			(   (ojtype == OPL_RIGHTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(maxlower == leftlower)
			&&
			(rdiff > 0.0)
			)
		    {   /* special case of first overlapping histogram cell
			** in which the right cell may have some part of its
			** tuples not overlapped with the left cell */
			temp_overlap = opn_diff(maxlower, rightlower, &rcharlength,
				rightcelltups, rattrp, datatype, tbl);
			if (temp_overlap > 0.0)
			{   /* get NULL contribution from first overlap cell since
			    ** right cell lower boundary is lower than left cell
			    */
			    special_temptotal = rightcelltups * temp_overlap / rdiff; 
			    rightojnull += special_temptotal;
			    if (fastpath)
			    {
				if (use_righthistp)
				    jnuniq += special_temptotal / rightreptf;
				rightjoin += special_temptotal;
			    }
			}
		    }
		    if ((rightcell == (maxrightcells-1))
			&&
			(   (ojtype == OPL_LEFTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(minupper == rightupper)
			&&
			(ldiff > 0.0)
			)
		    {   /* special case of last overlapping histogram cell
			** in which the left cell may have some part of its
			** tuples not overlapped with the right cell */
			temp_overlap = opn_diff(leftupper, minupper, &lcharlength,
				leftcelltups, lattrp, datatype, tbl);
			if (temp_overlap > 0.0)
			{   /* get NULL contribution from first overlap cell since
			    ** left cell lower boundary is lower than right cell
			    */
			    special_temptotal = leftcelltups * temp_overlap / ldiff;
			    leftojnull += special_temptotal;
			    if (fastpath)
			    {
				if (!use_righthistp)
				    jnuniq += special_temptotal / leftreptf;
				leftjoin += special_temptotal;
			    }
			}
		    }
		    else if ((leftcell == (maxleftcells-1))
			&&
			(   (ojtype == OPL_RIGHTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(minupper == leftupper)
			&&
			(rdiff > 0.0)
			)
		    {   /* special case of last overlapping histogram cell
			** in which the right cell may have some part of its
			** tuples not overlapped with the left cell */
			temp_overlap = opn_diff(rightupper, minupper, &rcharlength,
				rightcelltups, rattrp, datatype, tbl);
			if (temp_overlap > 0.0)
			{   /* get NULL contribution from first overlap cell since
			    ** right cell lower boundary is lower than left cell
			    */
			    special_temptotal = rightcelltups * temp_overlap / rdiff;
			    rightojnull += special_temptotal;
			    if (fastpath)
			    {
				if (use_righthistp)
				    jnuniq += special_temptotal / rightreptf;
				rightjoin += special_temptotal;
			    }
			}
		    }
		}
		if ((leftcelltups != 0.0)
		    &&
		    (rightcelltups != 0.0)
		    &&
		    (	(   ojhistp && (loverlap > 0.0))
			||
			(   !ojhistp 
			    && 
			    (loverlap = opn_diff(minupper, maxlower, 
				&lcharlength, leftcelltups, lattrp, 
				datatype, tbl)) 
			    > 0.0
			)
		    ))
		{   
		    /* There is some overlap between the left and right cells
		    ** and there are tuples in the left and right cells.
		    ** Compute the number of tuples in the join of the
		    ** overlapping regions
		    */
		    OPO_TUPLES        olefttups; /* number of tuples in the
					** overlapping interval which are
                                        ** in the left relation */
		    OPO_TUPLES        orighttups; /* number of tuples in the
                                        ** overlapping interval which are
                                        ** in the right relation */
                    OPN_DIFF          bd; /* temporary for calculation */
                    OPO_TUPLES        otemptotal; /* estimate of number of
                                        ** tuples in a join of this overlapping
                                        ** portion of the interval */

#ifdef xNTR1
		if (tTf(38, 14))
			TRdisplay("histj: leftcell:%d, rightcell:%d, loverlap:%.4f, ldiff:%.2f, rdiff:%.2f\n",
				leftcell, rightcell, loverlap, ldiff, rdiff);
#endif

		    /* Compute number of tuples in overlapping interval from
		    ** the left cell
		    */
		    if (ldiff == 0.0)
			/* Shouldn't happen, but just in case ... */
			olefttups = 0.0;
		    else
			olefttups = leftcelltups * loverlap / ldiff; 
		    if (olefttups > ltups)
			olefttups = ltups;	/* cell can't have more rows
						** than whole table */
		    
		    /* Compute roverlap if necessary 
		    */
		    if (lcharlength != rcharlength)
			roverlap = opn_diff(minupper, maxlower, &rcharlength,
					    rightcelltups, rattrp, datatype, tbl);
		    else
			roverlap = loverlap;		  

		    /* Compute number of tuples in overlapping interval from
		    ** the right cell
		    */
		    if (rdiff == 0.0)
			/* Shouldn't happen, but just in case ... */
			orighttups = 0.0;
		    else
			orighttups = rightcelltups * roverlap / rdiff;
		    if (orighttups > rtups)
			orighttups = rtups;	/* cell can't have more rows
						** than whole table */

		    /* Compute number of tuples in join of overlapping regions
		    */
		    if (global->ops_cb->ops_server->opg_smask & OPF_OLDJCARD
					/* server wide override */
			||
		        global->ops_cb->ops_check
	    		&&
	    		opt_strace(global->ops_cb, OPT_F062_OLDJCARD))
					/* session-specific override */
		    {	/* use old cardinality formulas */
		    	if (roverlap == 0.0)
			    otemptotal = 0.0;
		    	else
			    if (datatype->db_datatype == DB_FLT_TYPE &&
			    	(bd = ldiff * rdiff) < 4.0)
			    {
			    	/* assume at least 2 possible values for each of
			    	** the floats */ 
			    	otemptotal = 
				  olefttups * orighttups * bd / roverlap / 4.0;
			    }
			    else
			    	otemptotal = olefttups * orighttups / roverlap;

		    	{  /* make sure that if there are m and n tups in the 
			   ** cells being joined then at least min(m,n)/5 tups 
			   ** result from the join.  This keeps widely dispersed 
			   ** values from generating almost no joins, when in 
			   ** fact they almost always would generate some */
			    OPO_TUPLES       tempcnt; /* temp for calculation*/
			    if ( olefttups > orighttups )
			    	tempcnt = orighttups / 5.0;
			    else
			    	tempcnt = olefttups / 5.0;
			    if ( otemptotal < tempcnt)
			    	otemptotal = tempcnt;
		        }
		    }
		    else	/* spiffy new cardinality formula */
		    {
			lcard = olefttups * rightreptf;
			rcard = orighttups * leftreptf;
			if (lcard < rcard) 	/* pick the smaller */
			{
			    otemptotal = lcard;
			    jnuniq += olefttups / leftreptf;
				/* unique vals in left overlap */
			}
			else
			{
			    otemptotal = rcard;
			    jnuniq += orighttups / rightreptf;
				/* unique vals in right overlap */
			}
		    }		/* wasn't that easy? */

		    if (ojhistp)
		    {
			/* simple comparisons are used for the estimate, but a
			** more involved ball and cells estimate can be made for
			** the outer join, however doing this for each cell
			** of the histogram would be expensive; i.e. we can consider
			** the number of cells to be the number of tuples in the
			** right or left, and the balls to be the tuples which
			** participated in the inner join */
			switch (ojtype)
			{   /* for outer joins place a lower bound on the cell */
			case OPL_LEFTJOIN:
			    if (olefttups > otemptotal)
			    {
				leftojnull += olefttups - otemptotal; /* accumulate
						** estimated "null" tuples */
				if (fastpath)
				{
				    jnuniq += (olefttups - otemptotal) /
					    leftreptf;
				    otemptotal = olefttups; /* reset new tuple count for
						** this cell due to outer join 
						** semantics */
				}
			    }
			    break;
			case OPL_RIGHTJOIN:
			    if (orighttups > otemptotal)
			    {
				rightojnull += orighttups - otemptotal; /* accumulate
						** estimated "null" tuples */
				if (fastpath)
				{
				    jnuniq += (orighttups - otemptotal) /
					rightreptf;
				    otemptotal = orighttups; /* reset new tuple count for
						** this cell due to outer join 
						** semantics */
				}
			    }
			    break;
			case OPL_FULLJOIN:
			{
			    if (olefttups > otemptotal)
			    {
				if (orighttups > otemptotal)
				{
				    OPO_TUPLES	    rightnull;
				    OPO_TUPLES	    leftnull;

				    leftnull = olefttups - otemptotal;
				    leftojnull += leftnull;
				    rightnull = orighttups - otemptotal;
				    rightojnull += rightnull;
				    if (fastpath)
				    {
					jnuniq += leftnull / leftreptf +
					    rightnull / rightreptf;
					otemptotal += leftnull + rightnull;
				    }
				}
				else
				{
				    leftojnull += olefttups - otemptotal;
				    if (fastpath)
				    {
				    	jnuniq += (olefttups - otemptotal) /
					    leftreptf;
					otemptotal = olefttups;
				    }
				}
			    }
			    else if (orighttups > otemptotal)
			    {
				rightojnull += orighttups - otemptotal;
				if (fastpath)
				{
				    jnuniq += (orighttups - otemptotal) /
					rightreptf;
				    otemptotal = orighttups;
				}
			    }
			    break;
			}
			case OPL_INNERJOIN:
			    break;
			default:
			    opx_error(E_OP04AC_JOINID); /* unexpected join id type */
			    break;
			}
		    }

		    leftjoin += otemptotal; /* add the estimated number of 
					** tuples for this overlapping interval
                                        ** to the total for the counter for
                                        ** left histogram cell */
		    rightjoin += otemptotal; /* likewise for the right cell */
    
#ifdef xNTR1
		    if (tTf(38, 14))
			    TRdisplay("histj: bd:%.2f, olefttups:%.2f, orighttups:%.2f, tmp:%.2f, leftjoin:%.2f\n",
				    bd, olefttups, orighttups, tmp, leftjoin);
#endif
    
		}
		else if (ojhistp && (loverlap > 0.0))
		{   /* there are no inner join tuples, but there may still
		    ** be a possibility for outer join tuples */
		    OPO_TUPLES	    ojtemptotal;
		    if ((   (ojtype == OPL_LEFTJOIN) 
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&  
			(leftcelltups != 0.0)
			&&
			(ldiff > 0))
		    {   /* case of some overlap, with no right tuples
			** so all left tuples are retained due to
			** outer join semantics */
			ojtemptotal = leftcelltups * loverlap / ldiff; 
			if (fastpath)
			{
			    leftjoin += ojtemptotal; /* add the estimated number of 
					** tuples for this overlapping interval
                                        ** to the total for the counter for
                                        ** left histogram cell */
			    rightjoin += ojtemptotal; /* likewise for the right cell */
			    jnuniq += ojtemptotal / ((use_righthistp) ?
				rightreptf : leftreptf);
			}
			leftojnull += ojtemptotal; /* estimate of nulls introduced */
		    }
		    else if ((   (ojtype == OPL_RIGHTJOIN) 
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&  
			(rightcelltups != 0.0)
			&&
			(rdiff > 0))
		    {   /* case of some overlap, with no left tuples
			** so all right tuples are retained due to
			** outer join semantics */
			if (lcharlength != rcharlength)
			    roverlap = opn_diff(minupper, maxlower, &rcharlength,
						rightcelltups, rattrp, datatype, tbl);
			else
			    roverlap = loverlap;
			if ((rdiff > 0.0) && (roverlap > 0.0))
			{
			    ojtemptotal = rightcelltups * roverlap / rdiff; 
			    if (fastpath)
			    {
				leftjoin += ojtemptotal; /* add the estimated number of 
					** tuples for this overlapping interval
                                        ** to the total for the counter for
                                        ** left histogram cell */
				rightjoin += ojtemptotal; /* likewise for the right cell */
			    	jnuniq += ojtemptotal / ((use_righthistp) ?
				    rightreptf : leftreptf);
			    }
			    rightojnull += ojtemptotal; /* estimate of nulls introduced */
			}
		    }
		}
    
		/* build new histo cell if necessary.  new histo is 
		** same type, length and number of cells as rhistp. if we
		** are not done with the  "rightcell" of rhistp, then the 
		** next pass through we will reset the value.  This is the fix
		** for the bug mentioned in the history.  We used to
		** always set the fcnt array as if it was being built
		** isomorphic to the lhistp and this would put bad data in
		** fcnt and ocassionally overrun the space allocated
		** for the array 
                */

                if (use_righthistp && histflag)
		{
	    	    if (jnuniq < 1.0 && jnuniq > 0.0) jnuniq = 1.0;
		    jhistp->oph_creptf[rightcell] = jnuniq;
		    jhistp->oph_fcnt[rightcell] = rightjoin;
		}
    
		if ((minupper == leftupper) /* has left upper been reached */
                    ||		            /*OR no more right histogram cells*/
                    (rightcell == (rattrp->opz_histogram.oph_numcells - 1)))
		{   /* finished accumulating for the current left histogram
                    ** cell so add to total */
		    if (ojhistp 
			&& 
			(   (ojtype == OPL_LEFTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(loverlap <= 0.0)
			&&
			(	((rightcell == 1) /* indicates we have not reached
					** any right cells */
				&&
				(maxlower == rightlower))
				/* this histogram cell occurred before all right
				** histogram cells */
			    ||
				((rightcell == (rattrp->opz_histogram.oph_numcells - 1))
				&&
				(maxlower == leftlower))
				/* we have passed all right histogram cells
				** and are scanning left cells */
			))
		    {   
			/* remaining cells all do not match so include
			** them in outer join semantics */
			leftojnull += leftcelltups;
			if (fastpath)
			{
			    if (!use_righthistp)
				jnuniq += leftcelltups / leftreptf;
			    leftjoin += leftcelltups;
			}
		    }
		    totaltups += leftjoin;
		    /* build new histo cell if necessary.  new
		       histo is same type, length and number of
		       cells as lhistp. only do this when we are
		       finished with cell i of lhistp */
		    if (!use_righthistp && histflag)
		    {
	    		if (jnuniq < 1.0 && jnuniq > 0.0) jnuniq = 1.0;
			jhistp->oph_creptf[leftcell] = jnuniq;
			jhistp->oph_fcnt[leftcell] = leftjoin;
		    }
		    break;		    /* now start accumulating tuples for
                                            ** the next cell of the left
                                            ** histogram */
		}
		{   /* update values for the next cell of the right histogram */
		    if (ojhistp 
			&& 
			(leftcell == 1) /* indicates we have not reached
					** any left cells */
			&&
			(   (ojtype == OPL_RIGHTJOIN)
			    || 
			    (ojtype == OPL_FULLJOIN)
			)
			&&
			(loverlap <= 0.0)
			)
		    {	/* this histogram cell occurred before all left
			** histogram cells */
			rightojnull += rightcelltups;
			if (fastpath)
			    totaltups += rightcelltups;
			if (histflag && fastpath)
			{
			    jhistp->oph_creptf[rightcell] = jnuniq;
			    jhistp->oph_fcnt[rightcell] = rightcelltups;
			}
		    }
		    rightlower = rightupper; /* new lower and upper boundaries*/
		    rightupper += celllength;
		    rightcell++;            /* next cell number */
		    rightjoin = 0.0;	    /* 0 tuples for next cell */
		    if (use_righthistp) jnuniq = 0.0;
		    rightcelltups = rtups * rhistp->oph_fcnt[rightcell];
		    rightreptf = rhistp->oph_creptf[rightcell];
            	    if (rightreptf < 1.0)
                	rightreptf = 1.0;
		    rcharlength = 0;	/* if char type, compute diff as far
					** as necessary for accuracy */
		    if (rightcelltups == 0)
			/* Don't bother computing diff if there are no tuples */
			rdiff = 0;
		    else
			rdiff = opn_diff(rightupper, rightlower, &rcharlength,
					 rightcelltups, rattrp, datatype, tbl);
		}
	    }
            leftlower = leftupper;
	    leftupper += celllength;

            if ((rightcell >= maxrightcells)
                &&
                histflag
                &&
                (!use_righthistp))
		opx_error(E_OP04AE_OBSOLETE);    /* removed code which should be
				** be executed, place consistency check
				** in its place, code related to bug 44425 */
        }
        if (histflag && use_righthistp)
        {
	    if (jnuniq < 1.0 && jnuniq > 0.0) jnuniq = 1.0;
	    jhistp->oph_creptf[rightcell] = jnuniq;
	    jhistp->oph_fcnt[rightcell] = rightjoin; /* save the remainder
                                            ** of the join estimate */
            for (;(++rightcell) < maxrightcells; )
            {   /* fix b44425, uninitialized intermediate histograms */
		if (ojhistp
		    &&
		    (   (ojtype == OPL_RIGHTJOIN)
			|| 
			(ojtype == OPL_FULLJOIN)
		    ))
		{
		    rightcelltups = rtups * rhistp->oph_fcnt[rightcell];
		    rightojnull += rightcelltups;
		    if (fastpath)
		    {
			if ((rightreptf = rhistp->oph_creptf[rightcell]) != 0.0)
			    jhistp->oph_creptf[rightcell] = rightcelltups /
			    	rightreptf;
			else
                        {
			    jhistp->oph_creptf[rightcell] = 0.0;
			    rightreptf = 1.0;
			}
			jhistp->oph_fcnt[rightcell] = rightcelltups;
		    }
		    else
		    {
		 	jhistp->oph_creptf[rightcell] = 0.0;
			jhistp->oph_fcnt[rightcell] = 0.0;
		    }
		}
		else
		{
		    jhistp->oph_creptf[rightcell] = 0.0;
		    jhistp->oph_fcnt[rightcell] = 0.0; /* save the remainder
					    ** of the join estimate */
		}
            }
        }
    }
#	ifdef xNTR1
    if (tTf(22, 2))
	    TRdisplay("histj totaltups:%.2f\n", totaltups);
#	endif

    if (totaltups < 1.0)
	totaltups = 1.0;		    /* at least one tuple from join */

    if (histflag)
    {	/* if the histogram is requested then normalize it and calculate
        ** some other statistics
        */
	i4                    ncell;	    /* current cell being normalized */
	OPO_TUPLES	      totnunique = 0.0;   /* # unique in join result */

	jhistp->oph_unique = lhistp->oph_unique && rhistp->oph_unique; /* if 
					    ** either of the joining atts 
                                            ** are not unique then the
					    ** result att is not unique */
	jhistp->oph_complete = lhistp->oph_complete && rhistp->oph_complete &&
	    lattrp->opz_histogram.oph_domain ==rattrp->opz_histogram.oph_domain;
					    /* if both are complete then
					    ** this result att is complete */
	/* Don't forget - if result is unique, all creptf's & reptf
	** must be 1.0. */
	for (ncell = newhp_numcells; --ncell;)
	{
	    if (jhistp->oph_unique)
		jhistp->oph_creptf[ncell] = 1.0;
	    else
	    {
		totnunique += jhistp->oph_creptf[ncell];
		if (jhistp->oph_creptf[ncell] != 0.0)
		    jhistp->oph_creptf[ncell] = jhistp->oph_fcnt[ncell] /
			jhistp->oph_creptf[ncell];
		else jhistp->oph_creptf[ncell] = 0.0; /* recompute per-cell
					    ** repetition factor */
	    }
	    jhistp->oph_fcnt[ncell] /= totaltups; /* normalize histogram
					    ** - do not change cell 0 */
	}

	if (jhistp->oph_unique)
	{
	    jhistp->oph_nunique = totaltups;
	    jhistp->oph_reptf = 1.0;
	}
	else
	{
	    if (totnunique < 1.0) totnunique = 1.0;
	    else if (totnunique > totaltups) totnunique = totaltups;
	    jhistp->oph_nunique = totnunique;
	    jhistp->oph_reptf = totaltups / totnunique;
	}
	*jhistpp = jhistp;                  /* return histogram */
    }
    else
	*jhistpp = NULL;
    if (templower)
	oph_dcbmemory(subquery, templower); /* if an temporary array was
                                            ** allocated then release it */
    if (saved_attrp)
    {   
        /* fix up attrp so original correct chardensity and charnunique 
           arrays are used */
        saved_attrp->opz_histogram.oph_chardensity = saved_chardensity;
        saved_attrp->opz_histogram.oph_charnunique = saved_charnunique;
    }
 
    if (defaults)
    {	/* if default histograms are used then place a lower bound on the
        ** number of tuples which can be returned */
	OPO_TUPLES	    leftmin;
	OPO_TUPLES	    rightmin;
	leftmin = ltups * OPS_INEXSELECT;	    /* 10% of left tuples */
	rightmin= rtups * OPS_INEXSELECT;	    /* 10% of right tuples */
	if (totaltups < leftmin)
	    totaltups = leftmin;

	if (totaltups < rightmin)
	    totaltups = rightmin;
    }
    if (ojhistp)
    {   /* adjust the NULL count for the left and right portions */
        if (null_switch_flag)
        {   /* switch the null counts due to histogram switch */
            OPO_TUPLES      temp;
            temp = leftojnull;
            leftojnull = rightojnull;
            rightojnull = temp;
        }
        /* assume independent distribution of the nulls, P(a)+P(b)-P(a)*P(b)
	** P(attr1) = null(attr1)/tuples, P(attr2) = null(attr2)/tuples
	** so estimated null count is = tuples * 
	**		    (null(attr1)/tuples + null(attr2)/tuples 
	**		    - null(attr1)/tuples * null(attr2)/tuples)
	**   = null(attr1)+null(attr2) - null(attr1)*null(attr2)/tuples 
	*/
	if ((ojhistp->opl_ojtype == OPL_LEFTJOIN) ||  (ojtype == OPL_FULLJOIN))
            ojhistp->tlnulls += leftojnull - ojhistp->tlnulls * leftojnull /ltups;
	if ((ojhistp->opl_ojtype == OPL_RIGHTJOIN) ||  (ojtype == OPL_FULLJOIN))
            ojhistp->trnulls += rightojnull - ojhistp->trnulls * rightojnull /rtups;
    }
 
    if (onehist
	&&
	(totaltups > maxonehisttups)
	&&
	((subquery->ops_global->ops_cb->ops_server->opg_smask & OPF_STATS_NOSTATS_MAX)
	||
	(subquery->ops_global->ops_cb->ops_check &&
	opt_strace(subquery->ops_global->ops_cb, OPT_F077_STATS_NOSTATS_MAX)))
	&&
	!(subquery->ops_global->ops_cb->ops_check &&
	 opt_strace(subquery->ops_global->ops_cb, OPT_F078_NO_STATS_NOSTATS_MAX)))
	return (maxonehisttups * global->ops_cb->ops_alter.ops_stats_nostats_factor);
    else
	return (totaltups);
}

/*{
** Name: oph_join2	- alternate join estimator
**
** Description:
**	This function uses an alternate technique to estimate the number
**	of rows in a join qualified by a pair of histogrammed columns.
**	This technique is based on Sampling-Based Selectivity Estimation
**	for Joins Using Augmented Frequent Value Statistics by Haas & 
**	Swami (ICDE 1995).
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      lhistp                          ptr to histogram element for left
**                                      joining attribute
**      rhistp                          ptr to histogram element for right
**                                      joining attribute
**      ltuples                         number of tuples in the left
**                                      relation to be joined
**      rtuples                         number of tuples in the right
**                                      relations to be joined
**
** Outputs:
**      none
**	Returns:
**	    number of tuples from a join of the relations using
**	    alternate estimation technique
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-nov-05 (inkdo01)
**	    Written as part of join estimation improvement.
**	9-april-2008 (dougi) bug 120199
**	    Fix potential zero-divide.
*/
static OPO_TUPLES
oph_join2(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      *lhistp,
	OPH_HISTOGRAM      *rhistp,
	OPO_TUPLES         ltuples,
	OPO_TUPLES         rtuples)

{
    OPH_DOMAIN	mu1, q1m, q2m, c1m, c2m, lcount, rcount;
				/* many counts, accumulators, etc. */
    OPH_CELLS	lhistmm, rhistmm, lcell, rcell;
    OPZ_ATTS	*lattrp, *rattrp;
    DB_DATA_VALUE ldv, rdv;

    OPO_TUPLES	jtups;
    i4		lhistlen, rhistlen, d1, d2;
    i4		i, j, cmpres, exact;
    DB_STATUS	status;


    /* Set up and initialize various bits & pieces. */
    lattrp = subquery->ops_attrs.opz_base->
				opz_attnums[lhistp->oph_attribute];
    rattrp = subquery->ops_attrs.opz_base->
				opz_attnums[rhistp->oph_attribute];
    STRUCT_ASSIGN_MACRO(lattrp->opz_histogram.oph_dataval, ldv);
    STRUCT_ASSIGN_MACRO(rattrp->opz_histogram.oph_dataval, rdv);
    lhistlen = ldv.db_length;
    rhistlen = rdv.db_length;
    lhistmm = lattrp->opz_histogram.oph_histmm;
    rhistmm = rattrp->opz_histogram.oph_histmm;

    exact = d1 = d2 = 0;
    q1m = q2m = 0.0;

    /* Now loop over left source, processing only the exact cells. */
    for (i = 1; i < lattrp->opz_histogram.oph_numcells; i++)
     if (lhistp->oph_excells->oph_bool[i])
     {
	/* Exact cell on left - search right histogram for the matching
	** value and accumulate the totals. */
	lcount = lhistp->oph_fcnt[i];
	lcell = lhistmm + (i * lhistlen);
	ldv.db_data = lcell;

	rcell = rhistmm + rhistlen;
	for (j = 1; j < rattrp->opz_histogram.oph_numcells; j++)
	{
	    rdv.db_data = rcell;
	    status = adc_compare(subquery->ops_global->ops_adfcb, 
			&ldv, &rdv, &cmpres);
	    if (status != E_DB_OK)
	    {
	        opx_verror(status, E_OP078C_ADC_COMPARE,
		       subquery->ops_global->ops_adfcb->adf_errcb.ad_errcode);
	    }

	    /* If right cell < left cell, keep going. */
	    if (cmpres > 0)
	    {
		rcell += rhistlen;	/* next right cell */
		continue;
	    }

	    /* Magic formula requires accumulation of sums of the 
	    ** proportions of rows with current value ("count" in 
	    ** the terminology of Ingres histograms) and the sum of 
	    ** the product of proportions. */
	    if (cmpres == 0)
		rcount = rhistp->oph_fcnt[j];
					/* same as fcnt[j] for exact cell */
	    else rcount = rhistp->oph_creptf[j] / rtuples;
					/* proportionate for non-exact 
					** (while technically correct, this
					** formula has troubles with the fact
					** that creptf and rtuples seem to vary
					** at different rates when this is a
					** modified version of the original 
					** histo - as when a restriction has
					** already been applied) */
	    q1m += lcount;
	    q2m += rcount;
	    exact++;			/* increment count of exact cells */
	    break;
	}
    }

    /* Final computations - according to the algorithms, mu1 is the
    ** join selectivity due to joining the infrequent values and c2m
    ** is the selectivity due to joining the frequent values. The 
    ** join cardinality is the product of the selectivity and the 
    ** count of rows in the cross product. 
    **
    ** Note: in the paper, mu2 is computed to be the join selectivity 
    ** of the join of the frequent values. But simple factoring leaves
    ** mu2 equal to c2m, hence mu2 isn't computed here exactly as in
    ** the paper. */
    c1m = (1.0 - q1m) * (1.0 - q2m);
    c2m = q1m * q2m;

    d1 = lhistp->oph_nunique;
    d2 = rhistp->oph_nunique;

    if (((d1 - exact) * (d2 - exact)) <= 0)
	mu1 = 0;			/* virtually no rows from the 
					** non-frequent values */
    else
    {
	mu1 = (d1 < d2) ? d1 : d2;		/* min(d1, d2) */
	mu1 = (mu1 - exact) / ((d1 - exact) * (d2 - exact));
    }

    jtups = rtuples * ltuples * (c1m * mu1 + c2m);
    return(jtups);
}


/*}
** Name: OPN_JTSPECIAL - indicates which subtree contains primary relation
**
** Description:
**	Used to help determine minimum tuple count for a TID join
**
** History:
**      30-may-86 (seputis)
[@history_line@]...
*/
typedef i4          OPN_JTSPECIAL;
#define			OPN_JTLINDEX	   3
/* indicates that the left and right trees contain indexes on
** this tid equcls and that the left subtree is equal to or more
** restrictive so the left subtree should be a lower bound for
** the tuple count */
#define			OPN_JTRINDEX	   3
/* indicates that the left and right trees contain indexes on
** this tid equcls and that the right subtree is equal to or more
** restrictive so the right subtree should be a lower bound for
** the tuple count */
#define                 OPN_JTRIGHTPRIMARY 1
/* indicates that the right subtree is a primary relation */
#define                 OPN_JTLEFTPRIMARY  2
/* indicates that the left subtree is a primary relation */
#define                 OPN_JTNEITHER      0

/*{
** Name: opn_jtspecial	- find special case of primary and index subtree
**
** Description:
**      This subroutine will determine whether a special case exists in
**      which one subtree is a single primary and the other contains only
**      indexes.
**
**	If TRUE then the joining attributes are implied tid - explicit tid
**	We know that the number of tups in the primary
**	will be the number of tups in the join.
**
**	This is because all predicates applied to the indexes
**	must have been applied to the primary, but not all
**	predicates applied to the primary would necessarily have
**	been applied to the indexes.
**
**	The only purpose of the indexes is to restrict the amount
**	of blocks of the primary which must be accessed.  They
**	will have no effect on the number of rows retrieved, in this case.
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
**	30-may-86 (seputis)
**          initial creation
**	26-mar-90 (seputis)
**	    made changes to fix b20632
[@history_line@]...
*/
static OPN_JTSPECIAL
opn_jtspecial(
	OPS_SUBQUERY       *subquery,
	OPN_RLS            *lrlmp,
	OPN_RLS            *rrlmp,
	OPH_HISTOGRAM      *lhistp,
	OPH_HISTOGRAM      *rhistp)
{
    OPZ_AT                 *abase;  /* ptr to base of array of ptrs to
				    ** joinop range table elements */
    OPZ_IATTS              primattr; /* joining attribute of the left or 
				    ** right subtree which is a single 
				    ** primary leaf */
    OPV_BMVARS             *primrelmap;  /* map of joinop range variables
				    ** in subtree which contains a single
				    ** primary relation */
    OPV_BMVARS             *indexrelmap;  /* map of joinop range variables
				    ** in subtree which contains only 
				    ** indexes */
    OPV_IVARS              maxvars; /* number of joinop range variables */
    OPV_IVARS              maxprimary; /* number of primary joinop range
				    ** variables */
    OPN_JTSPECIAL          special; /* local state variable which indicates
				    ** which subtree is a primary */
    OPV_IVARS              primvar; /* joinop range variable number of the
				    ** single primary subtree */
    OPV_RT		   *vbase;  /* ptr to base of array of ptrs to
				    ** joinop range table elements */
    OPV_IVARS		   basevar; /* possible primary base table */
    vbase = subquery->ops_vars.opv_base; /* get base of array of ptrs
				    ** to joinop range variable elements */
    abase = subquery->ops_attrs.opz_base; /* get ptr to base of array of
				    ** ptrs to joinop attributes */
    maxvars = subquery->ops_vars.opv_rv; /* get number of range variables */
    maxprimary = subquery->ops_vars.opv_prv; /* get number of primary 
				    ** range variables */
    primrelmap = &lrlmp->opn_relmap;	/* get var map of left subtree */
    if ((BTcount((char *)primrelmap, (i4)maxvars) > 1) /* is left tree not a 
				    ** leaf */
	||
	((basevar = BTnext((i4)-1, (char *)primrelmap, (i4)maxvars)) >= maxprimary) /* are
				    ** the relations in the left subtree
				    ** ALL indexes */
	||
	(vbase->opv_rt[basevar]->opv_index.opv_poffset != basevar)
				    /* is this an explicitly referenced index */
	)
    {   /* left is not a single primary so switch */
	special = OPN_JTRIGHTPRIMARY;
	indexrelmap = primrelmap;   /*left subtree may contain all indexes*/
	primrelmap = &rrlmp->opn_relmap; /* get map from right subtree which
				    ** may be a primary */
	primattr = rhistp->oph_attribute; /* get joining attribute from 
				    ** right subtree (possibly primary) */
    }
    else
    {   /* left subtree is a single primary */
	special = OPN_JTLEFTPRIMARY;
	indexrelmap = &rrlmp->opn_relmap; /* right subtree may contain
				    ** indexes */
	primattr = lhistp->oph_attribute; /* left subtree is definitely
				    ** a single primary so get the joining
				    ** attribute */
    }

    if ((BTcount((char *)primrelmap, (i4)maxvars) == 1)
	&&
	((primvar = BTnext((i4)-1, (char *)primrelmap, (i4) maxvars)) 
 	    < maxprimary)
	&&
	abase->opz_attnums[primattr]->opz_attnm.db_att_id == DB_IMTID)
    {   /* we have a single primary which is joined on an implied tid 
	** - now see if all relations in index subtree are indexes
	** on "primvar" the single primary subtree
	*/
	OPV_IVARS	   irv;	    /* "index" range variable number */

	for (irv = -1; (irv = BTnext((i4)irv, (char *)indexrelmap, (i4) maxvars)) >= 0;)
	{
	    if (/* (irv < maxprimary)	||  remove this check for explicit referenced indexes */
		(vbase->opv_rt[irv]->opv_index.opv_poffset != primvar))
	    {
		special = OPN_JTNEITHER;
		break;		    /* not an index or does not
				    ** index the primary */
	    }
	}
    }
    else
    {
	special = OPN_JTNEITHER;
    }
    if ((special == OPN_JTNEITHER)
	&&
	(subquery->ops_mask & OPS_CBF))
    {
	OPE_ET		*ebase;
	OPE_IEQCLS	tideqc;
	
	ebase = subquery->ops_eclass.ope_base;
	tideqc = abase->opz_attnums[primattr]->opz_equcls;
	if(ebase->ope_eqclist[tideqc]->ope_eqctype == OPE_TID)
	{   /* check for special case of implicit TID join between two
	    ** indexes and make sure TID join is not restrictive is no extract
	    ** boolean factors are used on each side */
	    OPV_IVARS	    lv;
	    for (lv = -1; (lv = BTnext((i4)lv, (char *)&lrlmp->opn_relmap, (i4) maxvars)) >= 0;)
	    {
		OPV_VARS	*lvarp;
		lvarp = vbase->opv_rt[lv];
		if (lvarp->opv_bfmap
		    &&
		    (vbase->opv_rt[lvarp->opv_index.opv_poffset]->opv_primary.opv_tid == tideqc))
		{   /* boolean factor map found, on variable with correct equivalence class */
		    OPV_IVARS	    rv;
		    for (rv = -1; (rv = BTnext((i4)rv, (char *)&rrlmp->opn_relmap, (i4) maxvars)) >= 0;)
		    {	/* make sure all indexes provide more restrictivity on right side */
			OPV_VARS	*rvarp;
			rvarp = vbase->opv_rt[rv];
			if (rvarp->opv_bfmap
			    &&
			    (vbase->opv_rt[rvarp->opv_index.opv_poffset]->opv_primary.opv_tid == tideqc))
			{
			    OPB_BMBF	    *rmap;
			    OPB_BMBF	    *lmap;
			    OPB_IBF	    maxbf;
			    bool	    rsubset;
			    bool	    lsubset;
			    rmap = rvarp->opv_bfmap;
			    lmap = lvarp->opv_bfmap;
			    maxbf = subquery->ops_bfs.opb_bv;
			    rsubset = BTsubset((char *)rmap, (char *)lmap, (i4)maxbf);
			    lsubset = BTsubset((char *)lmap, (char *)rmap, (i4)maxbf);
			    if (rsubset)
			    {
				if(lsubset)
				    return(OPN_JTRINDEX);  /* causes tuple count to be
						    ** used from right side as a minimum */
				else
				    return(OPN_JTLINDEX);  /* causes tuple count to be
						    ** used from left side as a minimum */
			    }
			    else if(lsubset)
			    {
				return(OPN_JTRINDEX);  /* causes tuple count to be
						    ** used from right side as a minimum */
			    }
			    /* at this point each index contributes restrictivity which
			    ** is different so a TID join will supply extra selectivity */
			}
		    }
		}
	    }
	}
    }

    return (special);
}

/*{
** Name: oph_imaps	- get eqcls maps for primary and secondary
**
** Description:
**      This routine will produce a map of all variables associated 
**      with a TID equivalence class, i.e. equivalence classes 
**	for secondary and primaries
**
** Inputs:
**      subquery                        ptr to subquery being analysed
**      tideqc                          tideqc to associate with range var
**      varmap                          ptr to map of variables to analyze
**
** Outputs:
**      eqcmp                           ptr to map to initialize
**
**	Returns:
**	    TRUE if the eqcmp was initialized
**	Exceptions:
**
** Side Effects:
**
** History:
**      14-dec-89 (seputis)
**          initial creation to fix multi-attibute estimate problem
[@history_template@]...
*/
static bool
oph_imaps(
	OPS_SUBQUERY	*subquery,
	OPE_IEQCLS	tideqc,
	OPE_BMEQCLS	*eqcmp,
	OPV_BMVARS	*varmap)
{
    OPE_BMEQCLS	*eqcmp_ptr;
    OPV_IVARS	indexvar;
    OPV_IVARS	maxvar;
    OPV_RT	*vbase;
    OPE_IEQCLS	maxeqcls;

    vbase = subquery->ops_vars.opv_base;
    maxvar = subquery->ops_vars.opv_rv;
    maxeqcls = subquery->ops_eclass.ope_ev;
    eqcmp_ptr = (OPE_BMEQCLS *)NULL;
    /* for each attribute in the attrmap check if it is in the right or
    ** left sides and create the set of eqcls to eliminate from the join */
    for (indexvar = -1;
	(indexvar = BTnext((i4)indexvar, (char *)varmap, (i4)maxvar)) >= 0;)
    {
	OPV_VARS    *varp;
	OPV_IVARS   basevar;

	varp = vbase->opv_rt[indexvar];
	basevar = varp->opv_index.opv_poffset;
	if (basevar < 0)
	    continue;
	if (vbase->opv_rt[basevar]->opv_primary.opv_tid == tideqc)
	{   /* found a variable which is the primary or the secondary
	    ** for this tid join */
	    if (eqcmp_ptr)
		BTor((i4)maxeqcls, (char *)&varp->opv_maps.opo_eqcmap, (char *)eqcmp_ptr);
	    else
	    {
		eqcmp_ptr = eqcmp;
		MEcopy((PTR)&varp->opv_maps.opo_eqcmap, sizeof(varp->opv_maps.opo_eqcmap), 
		    (PTR)eqcmp_ptr);
	    }
	}
    }
    return(eqcmp_ptr != (OPE_BMEQCLS *)NULL);
}

/*{
** Name: oph_ojsvar	- find histogram for ON clause ref'd single-var func
**
** Description:
**	Using the eqclist of attributes for this equiv class, find 
**	a function attribute. Use the varnm of this attribute to find 
**	an appropriate trl struct. Call opn_eqtamp to find a histogram.
**
**      This routine is called when a function attribute is expected to
**      exist in the equivalence class and have a histogram associated with
**      it. It is needed because the fix to bug 84246 resulted in SVARs
**	sometimes not being attached to the ORIG node (because of outer
**	join null semantics). The reuse of OPN_RLS structures results in
**	the possibility that a needed SVAR histogram may not always be 
**	available. 
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**	rlp				ptr to OPN_RLS to which func att 
**					histogram must be added
**	tuples				count of tuples from func att source
**      eqcls                           equivalence class which may contain
**                                      a function attribute to be evaluated
**
** Outputs:
**	Returns:
**      histpp                          ptr to ptr to histogram element
**                                      which is associated with this
**                                      equivalence class
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-oct-97 (inkdo01)
**	    Semi-cloned from opn_hfunc.
**	17-oct-00 (inkdo01)
**	    A few inits to prevent SEGVs.
**	1-Sep-2005 (schka24)
**	    Clear new histo mem since we don't init everything.
**	12-sep-05 (hayke02)
**	    Ensure we have a non-OPL_NOOUTER opz_ojid when searching for
**	    OPZ_OJSVAR func atts. This change fixes bugs 114912 and 115165.
**	26-apr-06 (dougi)
**	    Properly allocate new oph_fcnt/creptf arrays to avoid double
**	    delete from diff. histograms.
**	21-mar-06 (hayke02)
**	    Initialise histpp to NULL so that we will get E_OP0482, rather
**	    than a SEGV, if we never call opn_eqtamp() to set histpp. This was
**	    seen whilst fixing bug 115757.
*/
static OPH_HISTOGRAM *
oph_ojsvar(
	OPS_SUBQUERY       *subquery,
	OPN_RLS		   *rlp,
	OPE_IEQCLS         eqcls,
	OPO_TUPLES	   tuples)
{
    OPZ_IATTS           attr;		    /* joinop attribute number of 
					    ** current attribute being 
                                            ** analyzed */
    OPZ_BMATTS          (*attrmap);	    /* ptr to bit map of attributes in
                                            ** in equivalence class */
    OPZ_IATTS           maxattr;            /* number of joinop
                                            ** attributes in subquery */
    OPZ_IFATTS		fattr;		    /* index to func attr array */
    OPZ_AT              *abase;             /* ptr to base of array of ptrs
                                            ** to joinop range variables */
    OPZ_FT		*fbase;		    /* ptr to base of array of ptrs
					    ** to function attribute descrs */
    OPH_HISTOGRAM       *newhp; 	    /* new histogram element created
					    ** for this equivalence class */
    OPH_HISTOGRAM	*tmphp = NULL;
    OPH_HISTOGRAM	**histpp = NULL;
    OPZ_ATTS		*attrp;
    i4			i;

    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap;
					    /* map of attributes in equivalence
                                            ** class - used to look for function
                                            ** attributes */
    maxattr = subquery->ops_attrs.opz_av;   /* number of joinop attributes
                                            ** defined in subquery */
    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    fbase = subquery->ops_funcs.opz_fbase;  /* ptr to base of array of ptrs
                                            ** to function attributes */
    for (attr= -1; (attr = BTnext((i4)attr, (PTR)attrmap, (i4)maxattr)) >= 0;)
    {
	/* find function attribute */
	if ((fattr = abase->opz_attnums[attr]->opz_func_att) >= 0 &&
		fbase->opz_fatts[fattr]->opz_mask & OPZ_OJSVAR)
	{   /* Qualifying func attr has been found. Look for corr histogram */
	    OPV_IVARS           varno;      /* range variable associated with
                                            ** function attribute */
	    varno = abase->opz_attnums[attr]->opz_varnm;
	    if (!BTtest((i4)varno, (char *)&rlp->opn_relmap) &&
		fbase->opz_fatts[fattr]->opz_ojid != OPL_NOOUTER)
		continue;
					    /* assure func att is from
					    ** this subtree */
	    histpp = opn_eqtamp(subquery, &subquery->ops_vars.opv_base->
		opv_rt[varno]->opv_trl->opn_histogram, eqcls, FALSE);/* get 
                                            ** histogram associated with
                                            ** the equivalence class of
                                            ** this function attribute */
	    break;			    /* exit loop with(/out) histo. */
	}	
    }
    if (histpp == NULL) opx_error(E_OP0482_HISTOGRAM);
					    /* if no hist, we have an error */
     else tmphp = *histpp;

    newhp = oph_memory(subquery); 	    /* get memory for a new histogram
					    ** element for this equivalence
					    ** class */
    MEfill(sizeof(OPH_HISTOGRAM), 0, (char *)newhp);  /* Clear it all */
    newhp->oph_odefined = FALSE;

    /* initialize remainder of histogram components */
    attrp = abase->opz_attnums[tmphp->oph_attribute];
    newhp->oph_fcnt = oph_ccmemory(subquery, &attrp->opz_histogram);
    newhp->oph_creptf = oph_ccmemory(subquery, &attrp->opz_histogram);
    for (i = 0; i < attrp->opz_histogram.oph_numcells; i++)
    {
	newhp->oph_fcnt[i] = tmphp->oph_fcnt[i];
	newhp->oph_creptf[i] = tmphp->oph_creptf[i];
    }
    newhp->oph_null = tmphp->oph_null; 	    /* get NULL count */
    newhp->oph_default = tmphp->oph_default;
    newhp->oph_attribute = tmphp->oph_attribute;
    newhp->oph_nunique = opn_eprime(tuples, tmphp->oph_reptf, 
	tmphp->oph_nunique, 
	subquery->ops_global->ops_cb->ops_server->opg_lnexp);
    if (newhp->oph_nunique > tuples)
	newhp->oph_nunique = tuples;
    if (newhp->oph_nunique < 1.0)
	newhp->oph_nunique = 1.0;
    newhp->oph_reptf = tuples / newhp->oph_nunique;
    newhp->oph_next = rlp->opn_histogram;   /* link histogram element into 
					    ** histogram list of OPN_RLS */
    rlp->opn_histogram = newhp;

    return(newhp);
}

/*{
** Name: opn_jtuples	- find number of tuples from a join
**
** Description:
**      This routine will find the number of tuples resulting from the join
**      of two relations
**
**	Called by:	opn_h2, opn_sjeqc
**
**	Estimates done as in Selinger's System R paper (i.e., if lc then
**	mintups = rtuples*ltuples/lhistp->nuniq instead of just rtuples).
**
**	Cannot do existence_only check cause do not know which is 
**      inner relation.  However, the cost functions will take this 
**      into consideration so we at least consider this case correctly 
**      at this node even though higher nodes will have an estimate of 
**      the number of blocks that might be too high
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      lrlmp                           histograms from left subtree
**      rrlmp                           histograms from right subtree
**      lhistp                          ptr to histogram for joining equivalence
**                                      class obtained from left tree
**      rhistp                          ptr to histogram for joining equivalence
**                                      class obtained from right tree
**      histflag
**
** Outputs:
**	tidvarnop			variable number of base relation if
**					this is a tid join, otherwise
**					OPV_NOVARS
**      tuples                          estimated number of tuples
**	Returns:
**	    histogram of the joining equivalence class (after join)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-86 (seputis)
**          initial creation
**	15-nov-88 (seputis)
**          remove bad *tuple for multi-attribute join
**	8-feb-89 (seputis)
**          correct problem for tuple counts on subselects
**	26-mar-90 (seputis)
**	    made changes to fix b20632
**	3-may-90 (seputis)
**	    - b21485, b21483 - move min max tuple estimate out of multi-attribute loop
**	19-jul-90 (seputis)
**	    - b31778 - fix estimate of TID join to be a upper limit for tuple count
**	25-jul-90 (seputis)
**	    - make histogram estimates less suscepible to from list ordering
**	27-dec-90 (seputis)
**	    - change default for complete flag
**	13-feb-91 (seputis)
**	    - fix problem with 25-jul-90 fix
**	16-may-91 (seputis)
**	    - fix for b36396 - assume cart-prod tuples if joining on a constant
**	    equilvalence class
**      28-jan-92 (seputis)
**          - fix 41730 - incorrect tid tuple estimate used
**      1-apr-92 (seputis)
**          - fix 40687 - not needed for bug 40687 proper but discovered
**          as part of investigation of this bug
**          - ensure that tid joins are not restrictive
**      18-sep-92 (ed)
**          - fix casting bug
**      5-jan-93 (ed)
**          - fix bug 48582 - isolate overloaded bitmaps which cause a OPH_HISTOGRAM
**          not found error
**	2-feb-93 (ed)
**	    - fix bug 47377 - unique DMF index can be used to limit join estimates
**	12-apr-93 (ed)
**	    - outer join underestimate problem - outer tuple count should be a minimum
**	17-may-93 (ed)
**	    - bug 47377 - check correct variable map for unique check
**	7-mar-96 (inkdo01)
**	    Modified complete flag processing to use predicate selectivities
**	    in face of restrictions on complete tables.
**	17-oct-97 (inkdo01)
**	    Alter search for histos to accomodate ON clause svar func atts.
**	16-mar-99 (inkdo01)
**	    Refinements in computation of per-cell rep factors in join hists.
**	10-nov-00 (inkdo01)
**	    If join is on TIDs of 2 secondaries of same base table, no new 
**	    restriction is applied and result is same as ltuples.
**      22-Mar-00 (wanfr01)
**          Removed debugging statement used to ID a test iidbms
**	18-apr-00 (hayke02)
**	    The fix for bug 102956 has been modified so that ltuples is
**	    is scaled by the join reptition factor - if we are TID joining
**	    on a multi relation join we can have non unique TID values.
**	    this change fixes bug 103858.
**	10-nov-00 (hayke02)
**	    Check for OPV_VCARTPROD - we have a key join cart prod, so we
**	    want a key join row estimate - unfortunately, we have no key
**	    join attribute info. Find the max reptf/min nunique and use
**	    these for the key join estimate. This change fixes bug 101399.
**	12-feb-02 (hayke02)
**	    Modify the above fix for 101399 so that the key join cart prod
**	    row estimates are as large as possible. Bug 101399 is in fact
**	    fixed by the fix for bug 103858. This change fixes bug 107083.
**	 8-oct-04 (hayke02)
**	    Further modification to the fix for 102956 and 103858 - we now
**	    use max((right reptf * ltuples), (left reptf * rtuples)) to set
**	    tuples for 'tidonly' joins. This change fixes problem INGSRV 2941,
**	    bug 112874.
**	5-dec-02 (inkdo01)
**	    Changes for range table expansion.
**	2-june-05 (inkdo01)
**	    Removed the change for bug 101399. It generated incorrect
**	    row estimates for CP joins and resulted in excessive use of
**	    CP joins.
**	10-oct-05 (inkdo01)
**	    Add alternate join cardinality estimation, compute ojhistp->
**	    opl_douters for null inner estimation and do a better job on
**	    SE joins.
**	26-apr-06 (dougi)
**	    Properly allocate new oph_fcnt/creptf arrays to avoid double
**	    delete from diff. histograms.
**      25-Sep-2008 (hanal04) Bug 120942
**          Prevent overflow of the tuple count when dealing with 
**          subsequent joins.
[@history_line@]...
*/
static OPH_HISTOGRAM *
opn_jtuples(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE	   *np,
	OPO_ISORT          jordering,
	OPO_ISORT          jxordering,
	OPN_RLS            *lrlmp,
	OPN_RLS            *rrlmp,
	OPE_BMEQCLS        *jeqh,
	bool               *junique,
	bool               *jcomplete,
	OPO_TUPLES         *tuples,
	OPV_IVARS	   *tidvarnop,
	OPL_OJHISTOGRAM    *ojhistp)
{
    OPO_TUPLES          ltuples;	/* number of tuples in left tree */
    OPO_TUPLES          rtuples;	/* number of tuples in right tree */
    OPO_TUPLES          tups;           /* total estimated tuples for join*/
    OPH_HISTOGRAM       *jhistp;         /* ptr to histogram for joining
                                        ** equivalence class */
    OPE_BMEQCLS         *jbmeqcls;      /* bitmap of equivalence classes
                                        ** in the join */
    OPE_BMEQCLS		jtemp;		/* used to adjust estimate for
					** TID join */
    OPE_IEQCLS          maxeqcls;       /* maximum number of elements
                                        ** in eqcls array */
    OPZ_AT              *abase;		/* get base of array of ptrs to 
                                        ** to joinop attributes */
    OPE_IEQCLS          eqcls;          /* current equivalence class being
                                        ** considered */
    OPO_TUPLES          cartprod;       /* number of tuples in cartesean
                                        ** product */
    bool		first_time;     /* TRUE - if first time thru */
    OPN_JTREE		*lp;
    OPN_JTREE		*rp;
    OPO_TUPLES		tmintups;	/* minimum number of tuples from
                                        ** join */
    OPO_TUPLES		tmaxtups;	/* maximum number of tuples from
                                        ** join */
    OPO_TUPLES		ojmintups;	/* minimum number of tuples given
                                        ** outer join semantics */
    OPL_OJTYPE          ojtype;         /* outer join type if applicable */
    OPO_TUPLES		tidtuples;	/* number of tid tuples from outer */
    OPE_IEQCLS		tideqcls;	/* primary tid eqcls */
    bool		lsinglevar;	/* TRUE if left tree is a single var */
    bool		rsinglevar;	/* TRUE if right tree is a single var */
    OPV_BMVARS          *secondary_map; /* if a TID join found this is the map
                                        ** of relations which contain the
                                        ** secondary indexes which are used
                                        ** in this subtree */
    OPO_TUPLES          baserel_tups;   /* if a TID join found this is the
                                        ** number of tuples in the project
                                        ** restrict */
    bool		tidjoin;	/* TRUE if this is an implicit
					** TID join, in which case the tuple
					** estimate should be equal to the
					** outer */
    OPV_RT              *vbase;
    bool		tidonlyjoin = FALSE;
    bool		SEjoin = FALSE;
    OPH_DOMAIN		outer_jc_nunique = 1.0;
    OPO_TUPLES		outertups;

    jhistp = (OPH_HISTOGRAM*)NULL; 
    ltuples = lrlmp->opn_reltups;	/* number of tuples in left tree */
    rtuples = rrlmp->opn_reltups;	/* number of tuples in right tree */
    cartprod = ltuples * rtuples;	/* worst case is cartesean product*/
    if (cartprod < 1.0)
	cartprod = 1.0;
    if (ojhistp)
        ojtype = ojhistp->opl_ojtype;	/* when not processing ON clause
                                        ** qualifications then special handling
                                        ** is needed to calculation outer join
                                        ** tuple count */
    else
        ojtype = OPL_INNERJOIN;         /* when no outer join, or when determining
                                        ** outer join histograms with ON clause
                                        ** qualifications */

    if (ojtype == OPL_LEFTJOIN || ojtype == OPL_FULLJOIN)
	outertups = lrlmp->opn_reltups;
    else if (ojtype == OPL_RIGHTJOIN)
	outertups = rrlmp->opn_reltups;
    else outertups = 0.0;

    {
	rp = np->opn_child[OPN_RIGHT];	/* get the pointer to the right child */
	lp = np->opn_child[OPN_LEFT];	/* get the pointer to the left child */
	tidjoin = FALSE;
	rsinglevar = rp->opn_nleaves == 1;
	vbase = subquery->ops_vars.opv_base;
	if (rsinglevar)
	{
	    OPV_IVARS	    rvarno;
	    OPV_VARS	    *rvarp;
	    
	    rvarno = rp->opn_prb[0];
	    rvarp = vbase->opv_rt[ rvarno ];
	    if (rvarp->opv_subselect		    /* is this a virtual relation */
		&&
		(
		    (rvarp->opv_grv->opv_created == OPS_SELECT /* check for all
						    ** subselect virtual relations */
		    )
		    ||
		    BTcount((char *)&rvarp->opv_grv->opv_gsubselect->
			opv_subquery->ops_correlated, OPV_MAXVAR) /* if
						    ** not a subselect but is a
						    ** correlated aggregate then
						    ** the cost can also be affected
						    ** by the ordering of the outer */
		)
		)
	    {
		SEjoin = TRUE;
		cartprod = ltuples;		    /* use the tuple count from the
						    ** outer as the max since this
						    ** inner SE join would not increase
						    ** the number of tuples returned */
	    }
	    if ((rvarno < subquery->ops_vars.opv_prv)
		&&
		(   (rvarp->opv_index.opv_poffset == OPV_NOVAR)
		    ||
		    (rvarp->opv_index.opv_poffset == rvarno)
		))
	    {
		tidjoin = TRUE;			    /* possible TID join since this
						    ** is a base relation, which is not
						    ** an explicitly referenced index */
		secondary_map = &lp->opn_rlmap;
		tidtuples = ltuples;
		tideqcls = rvarp->opv_primary.opv_tid;
		baserel_tups = rvarp->opv_prtuples; /* save tuple estimate from project
						    ** restrict node */
		*tidvarnop = rvarno;
	    }
	}
	lsinglevar = lp->opn_nleaves == 1;
	if (lsinglevar)
	{
	    OPV_IVARS	    lvarno;
	    OPV_VARS	    *lvarp;
	    lvarno = lp->opn_prb[0];
	    lvarp = vbase->opv_rt[ lvarno ];
	    if (lvarp->opv_subselect		    /* is this a virtual relation */
		&&
		(
		    (lvarp->opv_grv->opv_created == OPS_SELECT /* check for all
						    ** subselect virtual relations */
		    )
		    ||
		    BTcount((char *)&lvarp->opv_grv->opv_gsubselect->
			opv_subquery->ops_correlated, OPV_MAXVAR) /* if
						    ** not a subselect but is a
						    ** correlated aggregate then
						    ** the cost can also be affected
						    ** by the ordering of the outer */
		)
		)
	    {
		SEjoin = TRUE;
		cartprod = rtuples;		    /* use the tuple count from the
						    ** outer as the max since this
						    ** inner SE join would not increase
						    ** the number of tuples returned */
	    }
	    if ((lvarno < subquery->ops_vars.opv_prv)
		&&
		(   (lvarp->opv_index.opv_poffset == OPV_NOVAR)
		    ||
		    (lvarp->opv_index.opv_poffset == lvarno)
		))
	    {
		tidjoin = TRUE;			    /* possible TID join since this
						    ** is a base relation, which is not
						    ** an explicitly referenced index */
		secondary_map = &rp->opn_rlmap;
		tidtuples = rtuples;
		tideqcls = lvarp->opv_primary.opv_tid;
		baserel_tups = lvarp->opv_prtuples; /* save tuple estimate from project
						    ** restrict node */
		*tidvarnop = lvarno;
	    }
	}
    }

    if (jordering == OPE_NOEQCLS)	/* no join eqclass cause a cartesean
					** product at this node */
    {
	OPS_STATE	*global;
	global = subquery->ops_global;
	*junique = OPH_DUNIQUE;		/* use defaults if cart prod */
	*jcomplete = 
	    (   (global->ops_cb->ops_alter.ops_amask & OPS_ACOMPLETE)
		!= 0
	    )
	    ||
	    (   global->ops_cb->ops_check
		&&
		opt_strace(global->ops_cb, OPT_F050_COMPLETE)
	    )
	    ;			/* TRUE if all values for domain exist
				    ** within attribute */
	*tidvarnop = OPV_NOVAR;
	*tuples = cartprod;		/* number of tuples for cartesian prod*/
	return(jhistp);
    }
    maxeqcls = subquery->ops_eclass.ope_ev;
    abase = subquery->ops_attrs.opz_base; /* get ptr to base of array of
				    ** ptrs to joinop attributes */
    if (jordering < maxeqcls)
    {	/* single attribute in the equivalence class */
	if (jxordering != OPE_NOEQCLS)
	{
	    jbmeqcls = subquery->ops_msort.opo_base->opo_stable[jxordering-
		maxeqcls]->opo_bmeqcls;
	}
	else
	{
	    jbmeqcls = (OPE_BMEQCLS *)NULL;
	    eqcls = jordering;
	}
    }
    else
    {	/* multi-attribute ordering found */
	jbmeqcls = subquery->ops_msort.opo_base->opo_stable[jordering-
	    maxeqcls]->opo_bmeqcls;
	if (subquery->ops_mask & OPS_CTID)
	{   /* check if there is a TID join, in which case do not
	    ** add any more selectivity for attributes which are 
	    ** implicitly equal */
	    /* This code probably does not get executed since jxordering would
	    ** be set instead, perhaps it gets executed when left and right
	    ** trees contain secondaries on the same primary */
	    OPE_IEQCLS	    tideqc;

	    for (tideqc = -1;
		(tideqc = BTnext((i4)tideqc, (char *)jbmeqcls, (i4)maxeqcls))>=0;)
	    {
		OPE_BMEQCLS	ltemp;
                OPE_BMEQCLS     rtemp;
		if (!(subquery->ops_eclass.ope_base->ope_eqclist[tideqc]->ope_mask & OPE_CTID))
		    continue;			/* look for TID equivalence class,
						** which requires special checking */
		if (oph_imaps(subquery, tideqc, &ltemp, &lp->opn_rlmap)
		    &&
		    oph_imaps(subquery, tideqc, &rtemp, &rp->opn_rlmap))
		{
		    /* both left and right maps contain equivalences classes
		    ** associated with variables which reference the TID
		    ** equivalence class */
		    BTand((i4)maxeqcls, (char *)&ltemp, (char *)&rtemp);
		    BTnot((i4)maxeqcls, (char *)&rtemp);
		    BTand((i4)maxeqcls, (char *)jbmeqcls, (char *)&rtemp);
		    BTset((i4)tideqc, (char *)&rtemp);
                    MEcopy((PTR)&rtemp, sizeof(jtemp), (PTR)&jtemp);
                    jbmeqcls = &jtemp;
		    if (BTcount((char *)jbmeqcls, (i4)maxeqcls) == 1 &&
			BTtest((i4)tideqc, (char *)jbmeqcls))
		    {
			/* Joining only on TIDs cannot add any restriction,
			** so return tuple count is simply ltuples. */
			tidonlyjoin = TRUE;
			break;
		    }
		}
	    }
	}
    }
    if (jbmeqcls)
        eqcls = BTnext((i4)-1, (char *)jbmeqcls, (i4)maxeqcls);

    if (!tidjoin 
	|| 
	(tideqcls == OPE_NOEQCLS) 
	|| 
	(   (eqcls != tideqcls)
	    &&
	    (
		!jbmeqcls 
		||
		!BTtest((i4)tideqcls, (char *)jbmeqcls)
	    )
	))
    {
	*tidvarnop = OPV_NOVAR;
	tidjoin = FALSE;	    /* check for implicit TID join being performed */
    }
    *junique = FALSE;
    *jcomplete = FALSE;
    first_time = TRUE;
    tmintups = 1.0;
    tmaxtups = cartprod;
    switch (ojtype)
    {

    case OPL_LEFTJOIN:
	ojmintups = ltuples;
	break;
    case OPL_RIGHTJOIN:
	ojmintups = rtuples;
	break;
    case OPL_FULLJOIN:
	if (rtuples >= ltuples)
	    ojmintups = rtuples;
	else
	    ojmintups = ltuples;
	break;
    default:
	ojmintups = 0.0;
	break;
    }

    do	/* loop for each attribute in the join */
    {	
	OPN_JTSPECIAL          special; /* indicates whether special case exists
                                        ** and if it does then which subtree 
                                        ** has a single primary range var */
	bool                   lcomplete; /* is joining attribute from left
                                        ** subtree complete */
	bool                   rcomplete; /* is joining attribute from right
                                        ** subtree complete */
	OPH_HISTOGRAM          **histpp;
	OPH_HISTOGRAM          *lhistp; /* left histogram ptr */
        OPH_HISTOGRAM          *rhistp; /* right histogram ptr */
	bool		       histflag; /* TRUE if histogram is required */
	OPO_TUPLES	       mintups;	/* minimum number of tuples from
                                        ** join */
	OPO_TUPLES	       maxtups;	/* maximum number of tuples from
                                        ** join */
	OPH_DOMAIN	       jsel;	/* plan B join selectivity */
	OPO_TUPLES	       jtups, jtups2; /* plan B tuple estimates */
	
	mintups = 1.0;
	maxtups = cartprod;
	histpp = opn_eqtamp (subquery, &lrlmp->opn_histogram, eqcls, FALSE); 
	if (histpp == NULL)		/* may not find hist for oj SVARs */
	    lhistp = oph_ojsvar(subquery, lrlmp, eqcls, ltuples);
	 else lhistp = *histpp;
	histpp = opn_eqtamp (subquery, &rrlmp->opn_histogram, eqcls, FALSE);
	if (histpp == NULL) 
	    rhistp = oph_ojsvar(subquery, rrlmp, eqcls, rtuples);
	 else rhistp = *histpp;
        histflag = BTtest((i4)eqcls, (char *)jeqh) /* if TRUE then
                                        ** a histogram is required for the
                                        ** joining equivalence class */
            || (ojhistp && (ojhistp->opl_ojmask & OPL_OJONCLAUSE)); /* if an ON
                                        ** clause exists along with the outer
                                        ** join eqc, then histograms are needed
                                        ** in order to process the outer join
                                        ** calculations */
	if (histflag)
	    BTclear( (i4)eqcls, (char *)jeqh); /* clear bit so that
					** histogram is not also 
					** produced below */
        special = opn_jtspecial(subquery, lrlmp, rrlmp, lhistp, rhistp); /* does
                                        ** special case of single primary 
                                        ** subtree AND index-only subtree exist
                                        */
	if (abase->opz_attnums[lhistp->oph_attribute]->opz_histogram.oph_domain
	    == 
	    abase->opz_attnums[rhistp->oph_attribute]->opz_histogram.oph_domain)
	{
	    /* if same domain or if both -1 then default to the same domain */
	    lcomplete = lhistp->oph_complete;
	    rcomplete = rhistp->oph_complete;
	}
	else
	    /* otherwise joining on atts from different domains so the complete
	    ** flag does not mean anything 
	    */
	    lcomplete = rcomplete = FALSE;
	if (lcomplete)			/* if left subtree is complete then 
					** every tuple from right subtree must
					** join with at least one tuple here
					** (restricted by left st selectivity )
					*/
	    mintups = rtuples * lhistp->oph_reptf * lrlmp->opn_relsel;
	if (rcomplete)			/* same the other way */
	{
	    OPO_TUPLES         temptup;
	    if (mintups < (temptup = ltuples * rhistp->oph_reptf * 
							rrlmp->opn_relsel) )
		mintups = temptup;
	}
	if (lhistp->oph_unique)		/* if all we know is that left subtree's
					** tuples are unique then each
					** tuple from right subtree can join 
                                        ** with at most one here */
	    maxtups = rtuples;
	if (rhistp->oph_unique)		/* same the other way */
	{
	    if (maxtups > ltuples)
		maxtups = ltuples;
	}
	if (lhistp->oph_unique && lcomplete) /* if left subtree joining 
					** attribute is unique and complete
					** then every tuple in right subtree 
                                        ** must join with exactly one tuple
                                        ** here */
	    mintups = maxtups = rtuples * lrlmp->opn_relsel;
	else if (special == OPN_JTRIGHTPRIMARY) 
	    mintups = maxtups = rtuples;
	else if (special == OPN_JTRINDEX)
	    mintups = rtuples;
	else if (special == OPN_JTLINDEX)
	    mintups = ltuples;

	if (rhistp->oph_unique && rcomplete)
					/* same the other way */
	    mintups = maxtups = ltuples * rrlmp->opn_relsel;
	else if (special == OPN_JTLEFTPRIMARY) 
	    mintups = maxtups = ltuples;
    
	if (lcomplete 
	    && 
	    rcomplete 
	    && 
	    !lhistp->oph_unique 
	    && 
	    !rhistp->oph_unique 
	    && 
	    (special == OPN_JTNEITHER))
	    maxtups = mintups;

	*junique |= lhistp->oph_unique && rhistp->oph_unique; /* if
					** either of the joining atts 
					** are not unique then the
					** result att is not unique */
	*jcomplete |= lcomplete && rcomplete;
					/* if both are complete then
					** this result att is complete */

	/* Call special estimator if there are exact cells in the inexact 
	** histogram. */
	if (subquery->ops_global->ops_cb->ops_check &&
    		opt_strace(subquery->ops_global->ops_cb, OPT_F035_NO_HISTJOIN)
		&&
		!(lhistp->oph_unique) && !(rhistp->oph_unique) &&
		!(lhistp->oph_default) && !(rhistp->oph_default) &&
		lhistp->oph_excells && rhistp->oph_excells &&
		lhistp->oph_exactcount > 0.1 &&
		rhistp->oph_exactcount > 0.1 &&
		lhistp->oph_exactcount < 0.95 && 
		rhistp->oph_exactcount < 0.95)
	{
	    jtups = oph_join2(subquery, lhistp, rhistp, ltuples, rtuples);
	    jtups2 = oph_join2(subquery, rhistp, lhistp, rtuples, ltuples);

	    /* Pick the larger estimate. */
	    if (jtups2 > jtups)
		jtups = jtups2;
	}
	else if (!(lhistp->oph_default) && !(rhistp->oph_default))
	{
	    /* Otherwise, use basic alternative join estimator. */
	    jsel = (lhistp->oph_nunique > rhistp->oph_nunique) ? 
		1.0 / lhistp->oph_nunique : 1.0 / rhistp->oph_nunique;
	    jtups = jsel * cartprod;
	}
	else jtups = 0.0;

	/* Accumulate join column cardinality for null row estimate. */
	if ((ojtype == OPL_LEFTJOIN || ojtype == OPL_FULLJOIN)
		&& !(lhistp->oph_default))
	    outer_jc_nunique *= lhistp->oph_nunique;
	else if ((ojtype == OPL_RIGHTJOIN || ojtype == OPL_FULLJOIN)
		&& !(rhistp->oph_default))
	    outer_jc_nunique *= rhistp->oph_nunique;

	{
	    OPH_HISTOGRAM	*temphistp;
	    OPO_TUPLES		temptups;
	    bool		delayed_multiattr;
	    OPO_TUPLES		inittups;

	    if (subquery->ops_bfs.opb_bfeqc
		&&
		BTtest((i4)eqcls, (char *)subquery->ops_bfs.opb_bfeqc))
	    {
		if (histflag)
		    tups = oph_join(subquery, lhistp, rhistp, ltuples, rtuples, 
			&temphistp, histflag, ojhistp); /* still need
					** histogram for subsequent use */
		mintups = maxtups =
		tups = cartprod;	/* if joining on a constant equcls
					** then no restrictivity results
					** from this particular equi-join */
	    }
	    else
		tups = oph_join(subquery, lhistp, rhistp, ltuples, rtuples, 
		    &temphistp, histflag, ojhistp);

            if (jtups > 0.0 &&
		subquery->ops_global->ops_cb->ops_check
    		&&
    		opt_strace(subquery->ops_global->ops_cb, OPT_F035_NO_HISTJOIN))
	    tups = jtups;

	    if (!lsinglevar 
		&& 
		lhistp->oph_odefined 
		&& 
		(tups < lhistp->oph_origtuples))
	    {	/* adjust tuple estimate for multi-attribute join */
		inittups = ltuples;
		temptups = (tups/ltuples) * lhistp->oph_origtuples; /*
					** approximete selectivity if all
					** multi-attribute values were
					** evaluated together */
		if (!rsinglevar && rhistp->oph_odefined)
		{
		    OPO_TUPLES	    temptups1;
		    temptups1 = (tups/rtuples) * rhistp->oph_origtuples; /*
					** approximete selectivity if all
					** multi-attribute values were
					** evaluated together */
		    if (temptups < temptups1)
		    {
			temptups = temptups1;
			inittups = rtuples;
		    }
		}
		tups = temptups;
		delayed_multiattr = TRUE;
		
	    }
	    else if (!rsinglevar 
		&& 
		rhistp->oph_odefined
		&& 
		(tups < rhistp->oph_origtuples))
	    {	/* adjust tuple estimate for multi-attribute join */
		temptups = (tups/rtuples) * rhistp->oph_origtuples; /*
					** approximete selectivity if all
					** multi-attribute values were
					** evaluated together */
		tups = temptups;
		delayed_multiattr = TRUE;
		inittups = rtuples;
	    }
	    else
		delayed_multiattr = FALSE;

	    if (delayed_multiattr)
	    {
		if (first_time)
		{
		    first_time = FALSE;
		    *tuples = inittups;
		    tmintups = inittups;
		    tmaxtups = inittups;
		}
	    }
	    if (tups < 1.0)
		tups = 1.0;
	    if (mintups < tups)
		mintups = tups;
	    if (maxtups > tups)
		maxtups = tups;
	    else if (maxtups < 1.0)
		maxtups = 1.0;
	    if (histflag)
	    {
		temphistp->oph_odefined = FALSE;
		temphistp->oph_next = jhistp;
		jhistp = temphistp;		/* return list of histograms 
						** which are required further
						** up the tree */
	    }
	}
        if ((jxordering != OPE_NOEQCLS)
            &&
            (eqcls == tideqcls)
            &&
            tidjoin)
            ;       /* for tid join case in which other joins are possible
                    ** do not count TID join as restrictive */
	else if (first_time)
	{
	    first_time = FALSE;
	    tmintups = mintups;
	    tmaxtups = maxtups;
	    *tuples = tups;
	}
	else
	{   /* subsequent joins are restrictive, assume independence */
	    OPO_TUPLES	    newcount;
	    newcount = (tups * (*tuples)) / ((*tuples) + tups);
	    if (newcount < 1.0)
		*tuples = 1.0;
	    else
            {
                if(FPffinite(&newcount))
		    *tuples = newcount;
                else
                    *tuples = FMAX;
            }

	    newcount = (mintups * tmintups ) / (mintups + tmintups);
	    if (newcount < 1.0)
		tmintups = 1.0;
	    else
		tmintups = newcount;
	    newcount = (maxtups * tmaxtups ) / (maxtups + tmaxtups);
	    if (newcount < 1.0)
		tmaxtups = 1.0;
	    else
		tmaxtups = newcount;
	}
	
    } while (jbmeqcls	    /* loop for multi-attribute join */
	    &&
	    ((eqcls =BTnext((i4)eqcls, (char *)jbmeqcls, (i4)maxeqcls))>= 0));

    if (tidonlyjoin)
    {
	*tuples = rrlmp->opn_histogram->oph_reptf * ltuples;
	if ((lrlmp->opn_histogram->oph_reptf * rtuples) > *tuples)
	    *tuples = lrlmp->opn_histogram->oph_reptf * rtuples;
	return(jhistp);
    }

    /* Finish estimate of join column cardinality in outer source of OJ. */
    if (ojhistp)
     if (outer_jc_nunique > 1.0)
	ojhistp->opl_douters = opn_urn(outertups, outer_jc_nunique);
     else ojhistp->opl_douters = 0.0;

    {
	/* use enforced DMF uniqueness of indices to place limits on
	** the tuple estimates, especially multi-attribute joins */
	OPO_TUPLES	dmfmaxtups;
	dmfmaxtups = cartprod;
	if ((lrlmp->opn_mask & OPN_UNIQUECHECK) /* is unique check possible */
	    &&
	    opn_uniquecheck(subquery, &lrlmp->opn_relmap, jbmeqcls, eqcls))
	{
	    dmfmaxtups = rtuples;   /* left subtree unique so tuple count
				    ** limited by right side */
	    /* FIXME, probably should reset OPN_UNIQUECHECK at this node
	    ** for all relations */
	    if (ojtype == OPL_FULLJOIN)
		dmfmaxtups = rtuples + ltuples; /* if all tuples mismatch
				    ** then worst case is sum */
	    else if ((ojtype == OPL_LEFTJOIN)
		&&
		(rtuples <= ltuples))
		dmfmaxtups = ltuples; /* min due to outer join */
	    
	}
	if ((ltuples < dmfmaxtups)  /* is other direction worth checking */
	    &&
	    (rrlmp->opn_mask & OPN_UNIQUECHECK) /* is unique check possible */
	    &&
	    opn_uniquecheck(subquery, &rrlmp->opn_relmap, jbmeqcls, eqcls))
	{
	    dmfmaxtups = ltuples;   /* right subtree unique so tuple count
				    ** limited by left side */
	    if (ojtype == OPL_FULLJOIN)
		dmfmaxtups = rtuples + ltuples; /* if all tuples mismatch
				    ** then worst case is sum */
	    else if ((ojtype == OPL_RIGHTJOIN)
		&&
		(ltuples <= rtuples))
		dmfmaxtups = rtuples; /* min due to outer join */
	    
	}
	if (tmintups > dmfmaxtups)
	    tmintups = dmfmaxtups;  /* adjust tuple count on DMF uniqueness */
	if (tmaxtups > dmfmaxtups)
	    tmaxtups = dmfmaxtups;
    }

    if (tidjoin)
    {   /* implicit TID joins should limit the tuple count */
        OPV_IVARS       svarno;
        bool            apply;  /* true if at least one secondary has the
                            ** same boolean factors applied as the base
                            ** relation */

        apply = FALSE;
        baserel_tups *= 1.00001;    /* adjust for insignificant float
                            ** roundoff */
        for (svarno=(-1); (svarno=BTnext((i4)svarno, (char *)secondary_map,
                (i4)subquery->ops_vars.opv_rv))>=0;)
        {
            OPV_VARS        *varp;
            varp = vbase->opv_rt[svarno];
            if ((varp->opv_index.opv_poffset == (*tidvarnop))
                &&
                (varp->opv_prtuples <= baserel_tups))
            {
                /* crude test is to compare the project-restrict tuples
                ** - better would be to get boolean factors which have
                ** be applied to the base relation and determine if
                ** this boolean factors have also been applied to the
                ** secondary index subtree, FIXME */
                apply = TRUE;
                break;
            }
        }
        if (tidtuples < 1.0)
            tidtuples = 1.0;
        /* Tid joins are guaranteed to be many to one, so that no duplicates
        ** will be introduced, however there may be some qualifications
        ** which can apply to the base relation which cannot be applied to
        ** this secondary index,... this tuple estimate should reflect that
        ** amount so that tidtuples is an upper bound for the tuple estimate
        ** and not a lower bound....  FIXME check if all qualifications can apply
        ** to the secondary index and make this a lower bound also
        ** - another case may arise in which a join
        ** is made to a secondary index and this may introduce duplicate tids
        ** FIXME - check for this case and do not make it a lower bound
        */

        if ((jxordering == OPE_NOEQCLS)
            &&
            apply)
        {   /* there are no extra selective joins, so the join should be at
            ** least be many to one */
            /* - the major exception to this case, occurs when qualifications
            ** can be applied to the base relation but not the secondary
            ** index, so that the restrictivity of this qualifications is
            ** contained in tid attribute of the base relation, however
            ** the cost estimate will be lower than expected since the
            ** qualifications which can be applied to the secondary
            ** index only, are also applied to the tid histogram for
            ** the base relation so there is double applications for
            ** these boolean factors,... this is an architectural
            ** problem which needs to be solved by separating the
            ** restrictivity of the ORIG node and the PR node for
            ** a base relation,... other problems also occur in
            ** enumeration due to this architecture flaw, FIXME
            ** - for now make a kludge in which implicit TID
            ** joins are applied only if the secondary index restriction
            ** matches the base relation restriction */
            tmaxtups = tmintups = tidtuples;
        }
        else
        {   /* TID join should provide upper bound of a number of tuples but
            ** other joins may provide more selectivity */
            if (tidtuples < tmaxtups)
                tmaxtups = tidtuples;
            if (tidtuples < tmintups)
                tmintups = tidtuples;
        }
    }
    else
	*tidvarnop = OPV_NOVAR;
    if (tmintups < ojmintups)
	tmintups = ojmintups;			/* minimum estimate due to outer
						** join handling */
    if (tmaxtups < tmintups)
        tmaxtups = tmintups;                    /* probably better to go with
                                                ** larger estimate */
    if (*tuples < tmintups)
        *tuples = tmintups;
    else if (*tuples > tmaxtups)
        *tuples = tmaxtups;

    /* If this is SEjoin, we really have no information to permit a proper
    ** join estimation. Instead, just leave with estimate of nonkey *
    ** cartprod. */
    if (SEjoin)
    {
	*tuples = subquery->ops_global->ops_cb->ops_alter.ops_nonkey * 
								cartprod;
    }

    {
	/* Revise oph_nunique, oph_reptf, oph_creptf[i] values for 
	** computed histograms, based on final tuple estimate. */
	OPH_HISTOGRAM	*thistp;
	OPO_TUPLES	histtups, oldreptf, factor;

	for (thistp = jhistp; thistp; thistp = thistp->oph_next)
	{
	    if (thistp->oph_unique) continue;
					/* these are trivial & correct */
	    histtups = thistp->oph_nunique * thistp->oph_reptf;
					/* re-compute tups from hist */
	    if (*tuples > histtups * .098 && *tuples < histtups * 1.02)
		continue;		/* if hist is close to *tuples
					** don't bother recomputing */

	    /* Adjust nunique with balls & cells, then recompute reptf
	    ** and all the creptf. */
	    oldreptf = thistp->oph_reptf;
	    thistp->oph_nunique = opn_eprime(*tuples, oldreptf,
		thistp->oph_nunique, subquery->ops_global->ops_cb->
					ops_server->opg_lnexp);
	    thistp->oph_reptf = *tuples / thistp->oph_nunique;
	    if (thistp->oph_reptf < 1.0) thistp->oph_reptf = 1.0;

	    if (oldreptf != thistp->oph_reptf && oldreptf != 0.0)
	    {
		i4	numcells = abase->opz_attnums[thistp->oph_attribute]
				->opz_histogram.oph_numcells;
		i4	i;

		factor = thistp->oph_reptf / oldreptf;
		for (i = 0; i < numcells; i++)
		    thistp->oph_creptf[i] *= factor;
	    }
	}
    }	/* end of nunique, reptf, creptf[i] adjustments */

    return (jhistp);
}

/*{
** Name: opn_hfunc	- find histogram of a multi-var function
**
** Description:
**	Using the eqclist of attributes for this equiv
**	class, find a function attribute. Use the 
**	varnm of this attribute to find an appropriate
**	trl struct. Call opn_eqtamp to find a histogram.
**
**	This histogram is imprecise, at best. We want
**	a histo from a function because we know it will
**	have default characteristics.
**
**      This routine is called when a function attribute is expected to
**      exist in the equivalence class and have a histogram associated with
**      it.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      eqcls                           equivalence class which may contain
**                                      a function attribute to be evaluated
**
** Outputs:
**      histpp                          ptr to ptr to histogram element
**                                      which is associated with this
**                                      equivalence class
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
**          initial creation from hist_func
**      29-Mch-96 (prida01)
**	    Give warning about missing histogram.
[@history_line@]...
*/
static VOID
opn_hfunc(
	OPS_SUBQUERY       *subquery,
	OPE_IEQCLS         eqcls,
	OPH_HISTOGRAM      **histpp)
{
    OPZ_IATTS           attr;		    /* joinop attribute number of 
					    ** current attribute being 
                                            ** analyzed */
    OPZ_BMATTS          (*attrmap);	    /* ptr to bit map of attributes in
                                            ** in equivalence class */
    OPZ_IATTS           maxattr;            /* number of joinop
                                            ** attributes in subquery */
    OPZ_AT              *abase;             /* ptr to base of array of ptrs
                                            ** to joinop range variables */

    attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_attrmap; /*
					    ** map of attributes in equivalence
                                            ** class - used to look for function
                                            ** attributes */
    maxattr = subquery->ops_attrs.opz_av;   /* number of joinop attributes
                                            ** defined in subquery */
    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    for (attr= -1; (attr = BTnext((i4)attr, (PTR)attrmap, (i4)maxattr)) >= 0;)
    {
	/* find function attribute */
	if (abase->opz_attnums[attr]->opz_func_att >= 0
	    ||
	    abase->opz_attnums[attr]->opz_mask & OPZ_SPATJ)
	{   /* function attribute has been found so look for histogram for it */
	    OPV_IVARS           varno;      /* range variable associated with
                                            ** function attribute */
	    if (abase->opz_attnums[attr]->opz_mask & OPZ_SPATJ)
		eqcls = abase->opz_attnums[attr]->opz_equcls;
					    /* if this is a wacky spatial join
					    ** eqclass, need to send "real"
					    ** eqclass */
	    varno = abase->opz_attnums[attr]->opz_varnm;
	    *histpp= *opn_eqtamp(subquery, &subquery->ops_vars.opv_base->
		opv_rt[varno]->opv_trl->opn_histogram, eqcls, TRUE);/* get 
                                            ** histogram associated with
                                            ** the equivalence class of
                                            ** this function attribute */
	    return;			    /* return if histogram found */
	}	
    }
/* Remove this as a fatal error and do a TRdisplay instead */
    TRdisplay("Warning E_OP0489_NOFUNCHIST\n");
#undef E_OP0489_NOFUNCHIST    	
#ifdef E_OP0489_NOFUNCHIST
    opx_error(E_OP0489_NOFUNCHIST);
#endif
}

/*{
** Name: oph_jselect	- create histograms for join of left and right subtrees
**
** Description:
**	Called by:	opn_nodecost
**
**	When a hist for an eqclass exists on both sides f4 is chosen over i4.
**      Otherwise, if types are the same choose longer one, but if same 
**      length choose one with more cells.  When setting complete flag, 
**      domains are checked on both sides.
**
**	If a histogram for a equiv class is not found at this level, 
**      it is possible that the equiv class is on a multi-var function.
**	This means that the attribute in question is new.
**	Histos, in joinop, created a default histogram.  Try and use this 
**      histogram if possible. If no histogram is visible, we syserr.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      lsubtp                          ptr to left subtree cost ordering set
**                                      of join
**      leqclp                          ptr to OPN_EQS structure
**                                      which contains lsubtp
**      lrlmp                           ptr to OPN_RLS struct which contains
**                                      leqclp
**      rsubtp                          ptr to right subtree cost ordering set
**                                      of join
**      reqclp                          ptr to OPN_EQS structure
**                                      which contains rsubtp
**      rrlmp                           ptr to OPN_RLS struct which contains
**                                      reqclp
**      jeqc                            joining equivalence class - could be
**                                      OPE_NOEQCLS if there is a cartesean
**                                      product at this node
**      jeqh                            map of equivalence classes which
**                                      require histograms further up the
**                                      tree
**	byeqcmap			map of eqclasses which require
**					histograms, like jeqh, except that
**					these are just BY-list variables.
**					eqc's that are in the BY-list but
**					not in jeqh are handled differently.
**      jordering                       join ordering to be analyzed
**      jxordering                      if jordering is a tid join then this
**                                      ordering may be non-null and represents
**                                      equivalence classes which provide some
**                                      selectivity, i.e. are not in a secondary
**      ojhistp                         NULL if no outer joins to consider
**                                      otherwise contains outer join state
**
** Outputs:
**	trlpp				ptr to ptr to OPN_RLS structure with
**					histos of joined relation
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jun-86 (seputis)
**          initial creation from hist2.c
**	26-mar-90 (seputis)
**	    made changes to fix b20632
**	17-oct-90 (seputis)
**	    fix b33543 initialize oph_default flag
**	28-nov-90 (nancy)
**	    Fixed bug 34622.  If jtups < newhp->oph_nunique then reptf (repetition
**	    factor) is less than 0.  This causes a consistency check (E_OP048b
**	    negative cpu or io cost) later in opn_calcost().  Make 
**	    newhp->oph_nunique = jtups in this case, so reptf will be 1.0.
**      15-dec-91 (seputis)
**          rework fix for MHexp problem to reduce floating point computations
**      7-apr-94 (ed)
**          b60191 - uninitialized variable caused E_OP04B3 consistency check
**	7-mar-96 (inkdo01)
**	    Init newtrl->opn_relsel (introduced for improved complete flag 
**	    processing).
**	29-Mch-96 (prida01)
**	    b74932 - Ignore missing histogram and try next query plan.
**	25-jun-97 (hayke02)
**	    We now only return E_OP04AF if ojhistp is non NULL. E_OP04AF is
**	    an outer join error, so we will only return it if this is an
**	    outer join. This change fixes bug 83309.
**	 5-mar-01 (inkdo01 & hayke02)
**	    Support for changes in RLS/EQS/SUBTREE structures - jselect call
**	    may come with RLS already allocated.
**	    We now scale the newhp opn_creptf values by a factor of newhp
**	    oph_reptf divided by lhistp oph_reptf.
**	    This change fixes bug 103858.
**      19-Apr-02 (bolke01)
**          Bug 107406: The 70401 error is generated when the Right
**	    node histogram is selected and the left node hiostogram is
**	    longer.  Added addition setting of the number of elements in 
**	    the node to ensure correct length.
**	1-Sep-2005 (schka24)
**	    Pass in BY-list eqc map separately.  We want histograms for
**	    these just like eqc's in jeqh, but they don't imply non-joining
**	    predicates, and some error checks are relaxed.
**	    Also, clear newly allocated histo struct, it's not always
**	    all filled in, and we want to keep dirt out of it.
**      18-Nov-2008 (huazh01)
**          Modify the fix to b107406. We now check the boundary of the
**          lhistp->oph_creptf[] when using it to set newhp->oph_creptf[].
**          (bug 120617).
**	3-Jun-2009 (kschendel) b122118
**	    xdebug a not particularly important trdisplay.
[@history_line@]...
*/
VOID
oph_jselect(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *np,
	OPN_RLS		   **trlpp,
	OPN_SUBTREE        *lsubtp,
	OPN_EQS            *leqclp,
	OPN_RLS            *lrlmp,
	OPN_SUBTREE        *rsubtp,
	OPN_EQS            *reqclp,
	OPN_RLS            *rrlmp,
	OPO_ISORT          jordering,
	OPO_ISORT          jxordering,
	OPE_BMEQCLS        *jeqh,
	OPE_BMEQCLS        *byeqcmap,
	OPL_OJHISTOGRAM    *ojhistp)
{
    OPN_RLS             *newtrl;	    /* ptr to new set of histograms for
                                            ** the new relation created by this
                                            ** join */
    OPO_TUPLES          jtups;              /* estimated number of tuples in
                                            ** join */
    OPZ_AT              *abase;             /* get ptr to base of array of ptrs
                                            ** to joinop attributes */
    bool                junique;            /* TRUE - if joining equivalence
                                            ** class will have the "unique"
                                            ** property */
    bool                jcomplete;          /* TRUE - if joining equivalence
                                            ** class will have the "complete"
                                            ** property */
    OPV_IVARS		tidvarno;	    /* if this is an implicit tid join
					    ** then this is the base relation
					    */
    OPE_IEQCLS          maxeqcls;	    /* maximum number of equivalence
                                            ** classes defined  */
    OPE_BMEQCLS		orig_jeqh;	    /* Original jeqh before jtuples */
    OPE_BMEQCLS		histomap;	    /* jeqh OR byeqcmap */

    abase = subquery->ops_attrs.opz_base;   /* get ptr to base of array of
                                            ** ptrs to joinop attributes */
    maxeqcls = subquery->ops_eclass.ope_ev;
    MEcopy(jeqh, sizeof(orig_jeqh), (PTR) &orig_jeqh);
    {
	if (*trlpp == (OPN_RLS *) NULL)	    /* must get new RLS */
	{
	    newtrl = opn_rmemory(subquery); /* get new OPN_RLS header structure
					    ** to store info on new relation */
	     *trlpp = newtrl;
	}
	else newtrl = *trlpp;

	newtrl->opn_relsel = OPN_D100PC;    /* init selectivity to 1.0 */
	newtrl->opn_histogram = opn_jtuples (subquery, np, jordering, jxordering,
	    lrlmp, rrlmp, jeqh, &junique, &jcomplete, &jtups, &tidvarno, ojhistp); 
					    /* if jeqh has 
					    ** bits in jordering then
					    ** opn_jtuples will return a ptr
					    ** to histogram list for joining
					    ** equivalence class.  Otherwise,
					    ** just the number of tuples "jtups"
					    ** in the join is calculated */
	/* Turn off bits in byeqcls that got handled in opn-jtuples. */
	MEcopy((PTR) jeqh, sizeof(histomap), (PTR) &histomap);
	BTnot(maxeqcls, (char *) &histomap);
	BTand(maxeqcls, (char *) &orig_jeqh, (char *) &histomap);
	BTnot(maxeqcls, (char *) &histomap);
	BTand(maxeqcls, (char *) &histomap, (char *) byeqcmap);
    }

    {	/* assign atts (all hists will be unchanged except for 
	** joining equivalence class's histogram) 
	*/
	OPE_IEQCLS             eqcls;	    /* current equivalence class being
                                            ** analyzed */
	MEcopy(jeqh, sizeof(histomap), (PTR) &histomap);
	BTor(maxeqcls, (char *) byeqcmap, (char *) &histomap);
	for (eqcls = -1; 
	    (eqcls = BTnext((i4)eqcls, (PTR) &histomap, (i4)maxeqcls)) >= 0;)
	{
	    OPH_HISTOGRAM         *lhistp;
	    OPH_HISTOGRAM         *rhistp;
	    OPH_HISTOGRAM         **lhistpp;
	    OPH_HISTOGRAM         **rhistpp;
	    OPH_HISTOGRAM         *newhp; /* new histogram element created
					** for this equivalence class */
	    OPZ_ATTS              *lattrp = NULL; /* ptr to attribute of left
						** tree which is in this eqcls
						** (if attr exists) */
	    OPZ_ATTS              *rattrp; /* ptr to attribute of right
					** tree which is in this eqcls
					** (if attr exists) */
	    bool		  is_byeqc;

	    /* If it's not in jeqh, eqc is just a BY-list variable */
	    is_byeqc = ! BTtest(eqcls, (char *) jeqh);
	    newhp = oph_memory(subquery); /* get memory for a new histogram
					** element for this equivalence
					** class */
	    MEfill(sizeof(OPH_HISTOGRAM), 0, newhp);  /* Clear it all */
	    newhp->oph_odefined = FALSE;
	    /* note that the histogram must be available from the left
	    ** or right subtrees since it is in jeqh map, but it is not 
	    ** known which */
	    if (lhistpp = opn_eqtamp(subquery, &lrlmp->opn_histogram,
				    eqcls, FALSE))
	    {   /* histogram for left joining attribute found */
		lhistp = *lhistpp;
		lattrp = abase->opz_attnums[lhistp->oph_attribute]; /* get
					** ptr to joining attribute from
					** left subtree */
		/* FIXME - bug fix from 4.0 made here - jhistp was not being
		** used */
		newhp->oph_unique = lhistp->oph_unique && junique;
		newhp->oph_complete = lhistp->oph_complete && jcomplete;
		if (lhistp->oph_odefined)
		{   /* make multi-attribute keying estimates equal */
		    newhp->oph_origtuples = lhistp->oph_origtuples;
		    newhp->oph_odefined = TRUE;
		}
	    }
	    else
	    {
		lhistp = NULL;
		newhp->oph_unique = FALSE; /* this equivalence class is not
					** available here so set flags so
					** that values are taken from right
					** histogram */
		newhp->oph_complete = FALSE; /* ditto */
	    }

	    if (rhistpp = opn_eqtamp(subquery, &rrlmp->opn_histogram, 
				    eqcls, FALSE))
	    {   /* histogram for right joining attribute found */

		rhistp = *rhistpp;
		rattrp = abase->opz_attnums[rhistp->oph_attribute];
		newhp->oph_unique |= rhistp->oph_unique && junique;
		newhp->oph_complete |= rhistp->oph_complete && jcomplete;

		if (lhistp)
		{   /* this case can occur even though it appears all
		    ** joining equivalence classes should be represented
		    ** in opn_jtuples, since on tid joins, joining
		    ** secondary to primary index attributes do not
		    ** contribute to selectivity (FIXME - this may
		    ** change for left tid joins)
		    ** - can also occur if implicit tids are joined on
		    ** secondary indices, if some attributes of the
		    ** secondary indices are in common and were eliminated
		    ** from the join estimation since they may cause
		    ** double selectivity for any keying boolean factors
		    ** FIXME what about joins of the secondary with other
		    ** relations below this node?
		    */
		    
		    if (rhistp->oph_odefined)
		    {	/* make multi-attribute keying estimates equal */
			if (!newhp->oph_odefined
			  || newhp->oph_origtuples < rhistp->oph_origtuples)
			    newhp->oph_origtuples = rhistp->oph_origtuples;
			newhp->oph_odefined = TRUE;
		    }
		    if ((tidvarno >= 0)
			&&
			(tidvarno < subquery->ops_vars.opv_prv)
			)
		    {			/* since multi-attribute joins are
					** made, only expect secondary index
					** attributes matching to base relation
					** attributes to reach this point 
					** - pick the left histogram associated with
					** secondary index since this
					** could be changed by joins 
					** to other relations, whereas the
					** base relation would contain the
					** original histogram */
			if( BTtest((i4)tidvarno, (char *)&lrlmp->opn_relmap))
			    lhistp = rhistp; /* choose histogram from secondary
					** index side, since it may have
					** been joined with other relations */
		    }
		    else if ((subquery->ops_mask & OPS_CTID)
			&&
			(jordering >= maxeqcls)
			&&
			BTtest((i4)eqcls, (char *)subquery->ops_msort.opo_base->
			    opo_stable[jordering-maxeqcls]->opo_bmeqcls)
			&& !is_byeqc
			)
		    {	/* join of 2 secondary indexes on same base table 
			** has occurred which makes the common non-tid attributes
			** redundant and perhaps overly restrictive since their
			** is probably no extra selectivity at this join node
			** - unless one subtree joins to another relations on
			** one of the common subtrees FIXME */
			if (opt_strace(subquery->ops_global->ops_cb, OPT_F077_STATS_NOSTATS_MAX))
			{
			   if (rrlmp->opn_reltups < lrlmp->opn_reltups)
			   {
#ifdef xDEBUG
			      TRdisplay("jselect:%d%:RIGHTHISTUSED R=%.4f/L=%.4f\n",
					__LINE__,rrlmp->opn_reltups, lrlmp->opn_reltups);
#endif
			      lhistp = rhistp;	/* choose side with
					** greater restrictivity 
					** bolke01 set to '<' from '>'
					*/
			      lattrp = abase->opz_attnums[lhistp->oph_attribute];
		           }
		        }
			else
			{
			   if (rrlmp->opn_reltups > lrlmp->opn_reltups)
			   {
			      lhistp = rhistp;	/* choose side with
					** greater restrictivity */
			      lattrp = abase->opz_attnums[lhistp->oph_attribute];
		           }
		        }

		    }
		    else if (!is_byeqc && ojhistp)
			opx_error(E_OP04AF_JOINEQC);/* - consistency check since expect
					** a tid join here, since otherwise
					** eqcls should be in jordering or
					** jxordering and already be processed
					*/
		    if (lhistp != rhistp)
			newhp->oph_mask |= OPH_OJLEFTHIST;

		}
		else 
		{
		    lhistp = rhistp;
		    if (rhistp->oph_odefined)
		    {
			newhp->oph_origtuples = rhistp->oph_origtuples;
			newhp->oph_odefined = TRUE;
		    }
		}
	    }
	    /* If a histogram wasn't found, it's either a BY-list var
	    ** and we don't have a histogram for some reason;  or it's a
	    ** function attribute.
	    */
	    if (!lhistp && !is_byeqc)
		/*static*/ opn_hfunc(subquery, eqcls, &lhistp);

	    /* If we couldn't find the required histogram then it could
	    ** be the outer join's inner table not  passing
	    ** the histogram up ignore and try other histograms in list
	    */
	    if (!lhistp)
		continue;

	    /* initialize remainder of histogram components */
	    if (!lattrp)
		lattrp = abase->opz_attnums[lhistp->oph_attribute];
	    newhp->oph_fcnt = oph_ccmemory(subquery, &lattrp->opz_histogram);
	    newhp->oph_null = lhistp->oph_null; /* get NULL count */
	    newhp->oph_default = lhistp->oph_default;
	    newhp->oph_attribute = lhistp->oph_attribute;
	    newhp->oph_nunique = opn_eprime(jtups, lhistp->oph_reptf, 
                lhistp->oph_nunique, subquery->ops_global->ops_cb->ops_server->opg_lnexp);
	    if (newhp->oph_nunique > jtups)
		newhp->oph_nunique = jtups;
	    if (newhp->oph_nunique < 1.0)
		newhp->oph_nunique = 1.0;
	    newhp->oph_reptf = jtups / newhp->oph_nunique;
	    newhp->oph_creptf = oph_ccmemory(subquery,
						    &lattrp->opz_histogram);
	    /* bolke01 reduced divides */
	    {
		f4 ftmp_reptf ;
		i4 i, numcells;

		ftmp_reptf = newhp->oph_reptf / lhistp->oph_reptf;
                numcells = abase->opz_attnums[lhistp->oph_attribute]->
			opz_histogram.oph_numcells;

		for (i = 0; i < lattrp->opz_histogram.oph_numcells; i++)
		{
		    if (i < numcells && lhistp->oph_creptf[i])
			newhp->oph_creptf[i] = lhistp->oph_creptf[i] * ftmp_reptf;
		    else
			newhp->oph_creptf[i] =  0.0;
		    newhp->oph_fcnt[i] = lhistp->oph_fcnt[i];
					/* copy count while we're here */
		}
	    }  
	    newhp->oph_next = newtrl->opn_histogram; /* link histogram
					** element into histogram list
					** of new relation */
	    newtrl->opn_histogram = newhp;
	}
    }
    newtrl->opn_reltups = jtups;
    if ((lrlmp->opn_mask & OPN_UNIQUECHECK)
	&&
	(rrlmp->opn_mask & OPN_UNIQUECHECK)
	&&
	!ojhistp)			/* outer joins may introduce
					** non-unique NULLs */
	newtrl->opn_mask |= OPN_UNIQUECHECK;
#   ifdef xNTR1
    if (tTf(22, 0))
	prls (newtrl);
#   endif

}
