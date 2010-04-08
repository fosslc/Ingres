/*
**Copyright (c) 2004 Ingres Corporation
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
#define        OPN_RDIFF      TRUE
#include    <mh.h>
#include    <opxlint.h>
#include    <adulcol.h>

/* static function prototypes */

static OPN_DIFF	opn_chardiff(
		    char		*high,
		    char		*low,
		    i4			*forcemax,
		    i4			nunique,
		    OPN_PERCENT		density,
		    PTR			tbl);

static OPN_DIFF	opn_stringdiff(
		    char		*high,
		    char		*low,
		    i4			*charlength,
		    OPN_PERCENT		cellcount,
		    OPZ_ATTS		*attrp,
		    DB_DATA_VALUE	*datatype,
		    PTR			tbl);

/**
**
**  Name: OPNDIFF.C - return measure of difference between two histogram values
**
**  Description:
**      Contains routines to calculate measure of how different two numbers
**      are.
**
**  History:    
**      25-may-86 (seputis)    
**          initial creation
**	06-apr-92 (rganski)
**	    Fixed ansi compiler warning: trigraph sequence (double question 
**          mark) in comment.
**	09-nov-92 (rganski)
**	    Changed parameters to opn_diff(), and added new static functions
**	    opn_stringdiff() and opn_chardiff(), which implement string diff
**	    functionality for long strings, for Character Histogram
**	    Enhancements project. Removed #define of OPN_B11, which is no
**	    longer needed.
**	28-jun-93 (rganski)
**	    Added include of mh.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	18-Jun-1999 (schte01)
**       Added casts and casted assigns to avoid unaligned access in axp_osf.   
**      18-sep-2009 (joea)
**          Add case for DB_BOO_TYPE in opn_diff.
**/

/*{
** Name: opn_initcollate - Initialize collation sequence flags
**
** Description:
**
**      Called from: ops_qinit()
**
** Inputs:
**      global		OPS_STATE ptr	
**
** Outputs:
**          None.
**
** Side Effects:
**          none
**
** History:
**      01-Jan-00 (sarjo01)
**          Initial creation.
[@history_line@]...
*/
VOID
opn_initcollate(
	OPS_STATE *global)
{
    ADULTABLE	*tbl = (ADULTABLE *)(global->ops_adfcb->adf_collation);
    /*
    ** Test the collation table to see if it's 'simple'
    ** i.e., if it consists only of one-to-one mappings
    */ 
    if (tbl != NULL && tbl->nstate == 0)
    {
	i4  tblsize = sizeof(tbl->firstchar) / sizeof(i2);
	i4  i;

	for (i = 0; i < tblsize; i++)
	{
	    if (tbl->firstchar[i] % COL_MULTI != 0)
		break;
	}
	if (i == tblsize)
	    global->ops_gmask |= OPS_COLLATEDIFF;
    }
}
/*{
** Name: opn_chardiff - compute difference between two characters
**
** Description:
**	This function returns an integer number representing the
**	difference between two characters, known as the "character diff",
**	which is a measure of the number of unique characters between
**	the two characters. It computes the diff in the context of a
**	particular character position of particular column, using character
**	set information from the column's histogram, namely the number of
**	unique characters in the character position these characters came
**	from, and the character set "density" for the character position.The
**	character set density is a measure of how complete the character set
**	in the character position is, stored as a floating point number
**	between 0 and 1: a density of 1.0 indicates a complete character set,
**	given the number of unique characters; lower densities indicate
**	sparser character sets. These statistics (nunique and density) are
**	compiled by optimizedb and stored in the histogram for the column, or,
**	if there is no histogram, default statistics are used.
**
**	You can see that the character diff is more than just subtracting one
**	character from another.	See the Character Histogram Enhancements spec
**	for details on the algorithm (see algorithm for CharDiff).
**
**	Called from: opn_stringdiff()
**	
** Inputs:
**	high			First character
**	low			Second character
**	forcemax		If 0, compute char diff. If -1, set char diff
**				to 1 - nunique. If 1, set to nunique - 1.
**	nunique			Number of unique values in this character
**				position 
**	density			Character set density for this character
**				position
**
** Outputs:
**	forcemax		Set to -1 if computed diff was less than 
**				1 - nunique. Set to 1 if diff was greater than
**				nunique - 1. Otherwise leave at 0.
**	Returns:
**	    OPN_DIFF measure of the character diff.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    none
**
** History:
**	09-nov-92 (rganski)
**          Initial creation.
**	26-apr-93 (rganski)
**	    Changed return type to OPN_DIFF, which gives more accuracy.
**	    If the two characters are different and diff is less than nunique -
**	    1 in magnitude, disallows return values between -1.0 and 1.0 (i.e.
**	    fractions): there must be a difference of at least one, or the
**	    chardiff can cause the magnitude of the string diff to decrease.
**	27-jul-98 (hayke02)
**	    Round up/down chardiff when comparing with nunique. This
**	    prevents forcemax being erroneously set when chardiff contains
**	    rubbish from the f4/f8 calculation (e.g. -25.00000108778477
**	    instead of -25.0). This change fixes bug 91163.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	28-may-03 (inkdo01)
**	    Fold highchar/lowchar below 0x80 on assumption that only 
**	    alternate collations have chars above 0x80. This keeps the lid
**	    on diff values and addresses bug 109507.
[@history_line@]...
*/
static OPN_DIFF
opn_chardiff(
	char		*high,
	char            *low,
	i4		*forcemax,
	i4		nunique,
	OPN_PERCENT	density,
	PTR		tbl)
{
    OPN_DIFF	chardiff;	/* return value */
    
    if (*forcemax == 0)
    {
	i4	highchar;	/* High char converted to i4  */
	i4	lowchar;	/* Low char converted to i4  */

	/* convert high and low to unsigned nats FIXME - unsigned char ? */
        if (tbl == NULL)
        {
	    highchar = (i4)*high;
	    if (highchar < 0)
	        highchar = highchar + 256;
	    if (highchar > 128)
		highchar -= 128;
	    lowchar = (i4)*low;
	    if (lowchar < 0)
	        lowchar = lowchar + 256;
	    if (lowchar > 128)
		lowchar -= 128;
	}
	else
	{
            ADULcstate          acs;
            adulstrinit((ADULTABLE *)tbl, high, &acs);
            highchar = adultrans(&acs) / COL_MULTI;
            adulstrinit((ADULTABLE *)tbl, low, &acs);
            lowchar = adultrans(&acs) / COL_MULTI;
	}
	chardiff = (OPN_DIFF)(highchar - lowchar) * density;

	/* Forcemax is used to constrain diff to largest diff less than
	** computed diff, in cases where the character diff is greater than
	** possible. */
	if ((i4)(chardiff + 0.5) > nunique - 1)
	{
	    *forcemax = 1;
	    chardiff = nunique - 1;
	}
	else if ((i4)(chardiff -0.5) < 1 - nunique)
	{
	    *forcemax = -1;
	    chardiff = 1 - nunique;
	}
	else if (highchar != lowchar)
	{
	    /* Characters are different, so there must be a difference of at
	    ** least 1.0 in magnitude */
	    if (chardiff < 0.0)
	    {
		if (chardiff > -1.0)
		    chardiff = -1.0;
	    }
	    else
	    {
		if (chardiff < 1.0)
		    chardiff = 1.0;
	    }
	}
    }
    else if (*forcemax == 1)
	chardiff = nunique - 1;
    else
	chardiff = 1 - nunique;

    return(chardiff);
}

/*{
** Name: opn_stringdiff	- compute difference between two strings
**
** Description:
**	This function returns a floating point number representing the
**	"difference" between two strings, known as the "string diff", which is
**	a measure of the number of possible unique strings between the two
**	strings. It computes the diff in the context of a particular column,
**	using character set information from the column's histogram, namely
**	the number of unique characters per character position and the
**	character set "density" per character position.
**
**	See the Character Histogram Enhancements spec for details on the
**	algorithm and how these statistics are used.
**
**	Called by: opn_diff()
**
** Inputs:
**	high			    First string
**	low			    Second string
**	charlength		    If 0 or less, compute diff as far as
**				    necessary for computing accurate
**				    selectivities; otherwise, compute diff
**				    to this length.
**	celltups		    Number of tuples in the histogram cell. This
**				    is only used if charlength is 0 or less. We
**				    assume that the strings are the boundary
**				    values of a histogram cell with this many
**				    (celltups) tuples, and we use celltups to
**				    determine when we have gone far enough in our
**				    string diff computation.
**      attrp			    Ptr to joinop attribute which is context
**				    for this diff. 
**	   .opz_histogram	    Interval, datatype, and character set
**				    information associated with this attribute.
**		.oph_dataval	    Data type information for histogram and
**				    high and low parameters.
**	   	.oph_charnunique    Number of unique values per character
**				    position for this attribute
**	   	.oph_chardensity    Character set density per character
**				    position for this attribute.
**	datatype
**	    .db_length		    Length of high and low parameters.
**
** Outputs:
**	charlength		Length to which string diff computation went.
**				If this was greater than 0 on input, it
**				will be unchanged.
**	Returns:
**	    OPN_DIFF (f8) measure of how different the parameter are.
**	Exceptions:
**	    there may be some floating point exceptions in this routine
**          FIXME - need to do something so cannot get underflow when
**          subtracting two floats??
**
** Side Effects:
**	    none
**
** History:
**	09-nov-92 (rganski)
**          Initial creation.
**	14-dec-92 (rganski)
**	    Phase 3 of Character Histogram Enhancements project.
**	    See spec for full discussion of the changes.
**	    Bumped up multiplier in stopping condition from 100 to
**	    200.
**	    Added datatype parameter: diffs computed for join
**	    cardinalities may be done in the context of a histogram
**	    whose datatype has been converted to the type of the
**	    other column in the join; in this case the datatype
**	    information in attrp is incorrect.
**	18-jan-93 (rganski)
**	    Moved prevdiff local to outermost level and initialized to 0.
**	    Otherwise there is a remote possibility it will be used before
**	    initialization.
**	26-apr-93 (rganski)
**	    Changed return type of opn_chardiff() to OPN_DIFF, for more
**	    accuracy.
**	    Changed stopping condition: if previous diff was 0, continue
**	    (otherwise returns a diff of 0).
**	28-jun-93 (rganski)
**	    Added typecast for parameter to MHceil.
**	20-sep-93 (rganski)
**	    Added check of new oph_version field of OPH_INTERVAL. Adjustment of
**	    diff for 6.4 and previous histograms only done if there is no
**	    version.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	28-may-03 (inkdo01)
**	    When checking difference accuracy, compare to "abs(diff)". It is
**	    just the magnitude that matters and alternate collations can 
**	    generate negative diffs that then produce very large numbers,
**	    sometimes resulting in bug 109507.
**	10-feb-04 (hayke02)
**	    Modify the above change so that abs(diff) is replaced with diff ||
**	    (diff * -1.0). abs() expects an integer, but diff is an f8, and so
**	    abs(diff) will return incorrect results when diff > max(i4). This
**	    change fixes problem INGSRV 2702, bug 111759.
**	13-oct-05 (inkdo01)
**	    Add heuristic to return diff of 1 when it appears low is a marker
**	    for high.
**	25-nov-05 (inkdo01)
**	    Minor fix to above to increment pos before assignment to 
**	    *charlength, making up for quick exit of big loop.
**	17-Apr-07 (kibro01) b118015
**	    Ensure that if we're using collation sequences and we get into a
**	    forcemax situation (an odd character like an accent in the string
**	    causes this), we keep the diff value growing so we can break out
**	    of the loop at a reasonable sdifflength value, thus avoiding
**	    numeric overflows elsewhere (oph_splitcell, for example).
[@history_line@]...
*/
static OPN_DIFF
opn_stringdiff(
	char               *high,
	char               *low,
	i4		   *charlength,
	OPO_TUPLES	   celltups,
	OPZ_ATTS	   *attrp,
	DB_DATA_VALUE	   *datatype,
	PTR		   tbl)
{
    OPN_DIFF            diff;		/* variable used to accumulate 
					** differences between the two 
                                        ** strings */
    OPS_DTLENGTH        length, len1;   /* Lengths of strings to diff */
    i4			pos;		/* current character position */
    i4			*nunique;	/* number of unique characters in
					** current character position
					*/
    OPN_PERCENT		*density;	/* character set density for current
					** character position
					*/
    i4			forcemax;	/* Set to 1 iff all character
					** diffs henceforth should be forced
					** to the maximum, set to -1 iff they
					** should be forced to the minimum.
					** This is set when the absolute value
					** of a character diff exceeds the
					** number of unique characters in the
					** position - 1.
					*/
    OPN_DIFF		prevdiff;	/* previous value of diff */
    bool		no_version;	/* Is histogram from before 6.5?
					*/
    bool		allblanks;
	    

    /* Initialization */
    
    diff	= 0.0;
    length	= len1 = datatype->db_length;
    nunique	= attrp->opz_histogram.oph_charnunique;
    density	= attrp->opz_histogram.oph_chardensity;
    forcemax	= 0;
    prevdiff	= 0.0;
    no_version = attrp->opz_histogram.oph_version.db_stat_version[0] == ' ';
    
    if ((*charlength > 0) && (*charlength < length))
	length = *charlength;
    
    /* take ceiling of celltups */
    if (celltups > 0)
	celltups = (OPO_TUPLES)MHceil((f8)celltups);

    /* Compute diff */
    
    for (pos = 0; pos < length; pos++)
    {
	OPN_DIFF	chardiff;	/* Difference between current character
					** of high and low. */

	/* compute difference between current characters */
	chardiff = opn_chardiff(high, low, &forcemax, *nunique, *density, tbl);

	/* This is necessary for pre-6.5 statistics: without it, the total
	** string diff will never become greater than 1 */
	if (no_version && diff == 1 && chardiff == -10)
	    chardiff = -9;
	
	/* If we're asking how far we need to go to get required accuracy,
	** ensure we make diff increase with each character (if there is a
	** single way-out-there character, it sets forcemax to 1 and from there
	** on returns (1-*nunique), and adding 1-(*nunique) to diff repeatedly
	** keeps diff at 1, because (1 x *nunique) + 1 - (*nunique) = 1).
	** This is only a problem when a special collation sequence is in use
	** (tbl!=NULL) since otherwise the characters are "folded" back into
	** a 1-128 range.
	** The problem is fairly specific to when the initial characters in the 
	** histogram cells are a single character apart - otherwise the value 
	** would grow regardless.
	** If we don't allow the diff to increase the forcemax makes the final
	** diff equal 1.0 forever, so you end up with a numeric overflow in
	** oph_splitcell when it tries to use the entire length to determine
	** the number of strings between the values (kibro01) b118015
	*/
	if ( (chardiff != 0.0) &&
	     (diff == (diff * (*nunique) + chardiff)) && /* would stick on 1 */
	     (*charlength <= 0) &&		/* we're asking for sdifflen */
	     (tbl != NULL) )			/* we're using collation */
	{
	    if (diff < 0.0)
		chardiff = 1 - *nunique;	/* make chardiff negative */
	    else
		chardiff = *nunique - 1;	/* make chardiff positive */
	}

	/* compute string diff up to current character */
	diff = diff * (*nunique) + chardiff;

	if (*charlength <= 0)
	{
	    /* have we achieved enough accuracy? ("stopping
	    ** condition") 
	    */
	    if ((diff > ((*nunique) - 1) * 200.0 * celltups)
		||
		((diff * -1.0) > ((*nunique) - 1) * 200.0 * celltups))
	    {
		if (pos == 0)
		    break;
		else if (prevdiff != 0.0)
		{
		    diff = prevdiff;
		    break;
		}
	    }

	    /* Another stop heuristic. For string columns, oph_join()
	    ** may pass histogram values generated for different 
	    ** lengths. "Marker" cells (to mark exact cells) return
	    ** an incorrect diff in these cases - it should be 1.0, but
	    ** may be very large, thus producing bad estimates. We stick
	    ** a little loop here to determine such situations and force an 
	    ** early exit from the diff loop. */
	    if (diff == 1.0)
	    {
		i4	pos1;

		for (pos1 = 1, allblanks = TRUE; 
			pos1 < length - pos && allblanks; pos1++)
		 if (low[pos1] != 0x20 || high[pos1] != 0x20)
		    allblanks = FALSE;

		/* If all the rest are blanks, quit now with diff of 1.0. */
		if (allblanks)
		{
		    pos++;		/* make up for "break" loop exit */
		    break;
		}
	    }

	    prevdiff = diff;
	}

	/* go to next character position */
	high++;
	low++;
	nunique++;
	density++;
    }

    /* A final loop to detect exact cell markers. If shorter length was
    ** used and found diff of 0, keep looping to see if lower has a 
    ** 0x1f and high is all blanks. */
    if (diff == 0.0 && length < len1)
    {
	for (allblanks = TRUE; pos < len1 - 1 && allblanks;
				pos++, low++, high++)
	 if (*high != 0x20 || *low != 0x20)
	    allblanks = FALSE;

	if (allblanks && *low == 0x1f && *high == 0x20)
	    diff = 1.0;
    }

    if (*charlength <= 0)
	*charlength = pos;
    
    return(diff);

}
/*{
** Name: opn_diff	- difference of "high - low" for arbitrary types
**
** Description:
**	This function returns a floating point number representing how 
**	different the two parameter values are. The values may be of 
**      any type, ie. integer, floating point, text, char, etc. 
**      If "diff(a, b, datatype) relop 0" is true then "a relop b"
**      is true; i.e. if diff(a, b, datatype) < 0 then a < b.  Furthermore, 
**      if "diff(a, b, datatype) relop diff(a, c, datatype)" is true 
**      then "b relop c" is true. This is very important to understand, and to 
**	maintain in this function.
**
**	For character types, calls opn_stringdiff(), which computes the number
**	of possible strings between high and low, according to the
**	Character Histogram Enhancements spec.
**
** Inputs:
**	high			    First value
**	low			    Second value
**	charlength		    For character types only:
**				    if 0 or less, compute diff as far as
**				    necessary for computing accurate
**				    selectivities; otherwise, compute diff
**				    to this length.
**	celltups		    For character types only:
**				    Number of tuples in the histogram cell. This
**				    is only used if charlength is 0 or less. We
**				    assume that the strings are the boundary
**				    values of a histogram cell with this many
**				    (celltups) tuples, and we use celltups to
**				    determine when we have gone far enough in our
**				    string diff computation.
**      attrp			    Ptr to joinop attribute which is context
**				    for this diff. 
**	   .opz_histogram	    Interval, datatype, and character set
**				    information associated with this attribute.
**		.oph_dataval	    Data type information for histogram and
**				    high and low parameters.
**	   	.oph_charnunique
**	   	.oph_chardensity    Character set statistics for this
**				    attribute, if character type
**
** Outputs:
**	charlength		For character types only: length to which
**				string diff computation went.
**				If this was greater than 0 on input, it
**				will be unchanged.
**	Returns:
**	    OPN_DIFF (f8) measure of how different the parameter are.
**	Exceptions:
**	    there may be some floating point exceptions in this routine
**          FIXME - need to do something so cannot get underflow when
**          subtracting two floats??
**
** Side Effects:
**	    none
**
** History:
**	25-may-86 (seputis)
**          initial creation
**      14-jan-87 (seputis)
**          all string histogram datatypes are now mapped into DB_CHA_TYPE
**      27-may-89 (seputis)
**	    use unsigned arithmetic for comparing character strings - CLSI bug
**	03-feb-92 (rganski)
**	    When computing diff for DB_CHA_TYPE, set char difference to
**	    -9 instead of -10 if diff so far is 1. This allows the diff
**	    to increase through multiplication.
**	09-nov-92 (rganski)
**	    Replace code for string diffs (char types) with call to new
**	    function opn_stringdiff(), which implements new string diff
**	    functionality for long strings, for Character Histogram
**	    Enhancements project. Added 3 new parameters: charlength,
**	    celltups, and attrp, which are needed by opn_stringdiff(),
**	    and removed datatype parameter, since this can be gotten from
**	    attrp.
**	14-dec-92 (rganski)
**	    Phase 3 of Character Histogram Enhancements project.
**	    Added datatype parameter: diffs computed for join
**	    cardinalities may be done in the context of a histogram
**	    whose datatype has been converted to the type of the
**	    other column in the join; in this case the datatype
**	    information in attrp is incorrect.
**	26-apr-93 (rganski)
**	    Added charlength to description of output parameters above. No code
**	    changes.
**	18-Jun-1999 (schte01)
**       Added casts and casted assigns to avoid unaligned access in axp_osf.   
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**	18-Jun-1999 (schte01)
**       Added casts and casted assigns to avoid unaligned access in axp_osf.   
**	31-mar-05 (inkdo01)
**	    Some moron (a.k.a. inkdo01) forgot to add the i8 case to opn_diff
**	    when bigint's were implemented.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
[@history_line@]...
*/
OPN_DIFF
opn_diff(
	PTR                high,
	PTR                low,
	i4		   *charlength,
	OPO_TUPLES	   celltups,
	OPZ_ATTS	   *attrp,
	DB_DATA_VALUE	   *datatype,
	PTR		   tbl)
{
    OPN_DIFF            ret_val;	/* variable used to accumulate 
					** differences between the two 
                                        ** parameters */
    OPS_DTLENGTH        length;         /* length of datatype */

    /* get datatype length */
    length = datatype->db_length;

    /* FIXME - need the equivalent of EXmath(EX_ON) here */

    switch (datatype->db_datatype)
    {
    case DB_CHA_TYPE:
    {
	ret_val = opn_stringdiff((char *)high, (char *)low, charlength,
				 celltups, attrp, datatype, tbl);
	break;
    }
    case DB_BYTE_TYPE:
    {
	ret_val = opn_stringdiff((char *)high, (char *)low, charlength,
				 celltups, attrp, datatype, NULL);
	break;
    }
    case DB_BOO_TYPE:
        ret_val = (OPN_DIFF)1;
        break;

    case DB_INT_TYPE:
    {

	switch (length)
	{
	case 1: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) I1_CHECK_MACRO(
	           CHCAST_MACRO(&(((DB_ANYTYPE*)high)->db_i1type))) -
		      (OPN_DIFF) I1_CHECK_MACRO(
			 CHCAST_MACRO(&(((DB_ANYTYPE*)low)->db_i1type)));
#else
	    ret_val = (OPN_DIFF) I1_CHECK_MACRO(((DB_ANYTYPE*)high)->db_i1type) -
		      (OPN_DIFF) I1_CHECK_MACRO(((DB_ANYTYPE*)low)->db_i1type);
#endif
	    break;
	case 2: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) (I2CAST_MACRO(&(((DB_ANYTYPE*)high)->db_i2type)))-
		      (OPN_DIFF) (I2CAST_MACRO(&(((DB_ANYTYPE*)low)->db_i2type)));
#else
	    ret_val = (OPN_DIFF) (((DB_ANYTYPE*)high)->db_i2type) -
		      (OPN_DIFF) (((DB_ANYTYPE*)low)->db_i2type);
#endif
	    break;
	case 4: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) (I4CAST_MACRO(&(((DB_ANYTYPE*)high)->db_i4type)))-
		      (OPN_DIFF) (I4CAST_MACRO(&(((DB_ANYTYPE*)low)->db_i4type)));
#else
	    ret_val = (OPN_DIFF) (((DB_ANYTYPE*)high)->db_i4type) -
		      (OPN_DIFF) (((DB_ANYTYPE*)low)->db_i4type);
#endif
	    break;
	case 8: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) (I8CAST_MACRO(&(((DB_ANYTYPE*)high)->db_i8type)))-
		      (OPN_DIFF) (I8CAST_MACRO(&(((DB_ANYTYPE*)low)->db_i8type)));
#else
	    ret_val = (OPN_DIFF) (((DB_ANYTYPE*)high)->db_i8type) -
		      (OPN_DIFF) (((DB_ANYTYPE*)low)->db_i8type);
#endif
	    break;
	}
	break;
    }
    case DB_FLT_TYPE:
    {
	switch (length)
	{
	case 4: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) (F4CAST_MACRO(&(((DB_ANYTYPE*)high)->db_f4type)))-
		      (OPN_DIFF) (F4CAST_MACRO(&(((DB_ANYTYPE*)low)->db_f4type)));
#else
	    ret_val = (OPN_DIFF) (((DB_ANYTYPE*)high)->db_f4type) -
		      (OPN_DIFF) (((DB_ANYTYPE*)low)->db_f4type);
#endif
	    break;

	case 8: 
#ifdef axp_osf
	    ret_val = (OPN_DIFF) (F8CAST_MACRO(&((DB_ANYTYPE*)high)->db_f8type))-
		      (OPN_DIFF) (F8CAST_MACRO(&((DB_ANYTYPE*)low)->db_f8type));
#else
	    ret_val = (OPN_DIFF) ((DB_ANYTYPE*)high)->db_f8type -
		      (OPN_DIFF) ((DB_ANYTYPE*)low)->db_f8type;
#endif
	    break;
	}
	break;

    }
    default:
	{   /* this check should never be executed */
# ifdef E_OP0484_HISTTYPE
	    opx_error(E_OP0484_HISTTYPE); /* unexpected type */
# endif
	}
    }
    /* FIXME - need the equivalent of "EXmath(EX_OFF)" */

    return(ret_val);
}

