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
#define        OPHINTERPOLATE      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPHINTER.C - linearly interpolate between minv and maxv
**
**  Description:
**      routines to linearly interpolate percentage of cell selected 
**      between minv and maxv
**
**  History:    
**      25-may-86 (seputis)    
**          initial creation
**      11-jan-89 (seputis)    
**          look for exact histograms for CHA and VCH types
**	21-jun-93 (rganski)
**	    b50012: added include of adfops.h, for new parameter. See
**	    description below.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	12-sep-1996 (inkdo01)
**	    Added support for selectivity computations on "<>" (NE) relops.
**	1-may-97 (inkdo01)
**	    Followed through on FIXME to remove float-specific code for 
**	    EXACTKEY match (should simply return 100%).
[@history_line@]...
**/

/*{
** Name: oph_txtcompare	- special case compare for histogram text items
**
** Description:
**      This routine will compare two histogram text fields.  This routine is
**      needed since a histogram text datatype is not equivalent to a text
**      datatype.
**
**      FIXME - this should be an ADF function
**
** Inputs:
**      p1                              ptr to first histogram text element
**      p2                              ptr to second histogram text element
**      datatype                        datatype containing the text length
**                                      field
**
** Outputs:
**	Returns:
**	    value < 0 iff p1 < p2
**          value = 0 iff p1 = p2
**          value > 0 iff p1 > p2
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-may-86 (seputis)
**          initial creation
*/
static i4
oph_txtcompare(p1, p2, datatype)
char               *p1;
char               *p2;
DB_DATA_VALUE      *datatype;
{
    i4                  len;	    /* length of text data type */

    len = datatype->db_length;
    while (len--)
    {
	if (*p1 != *p2)
	    return ((i4) (*p1 - *p2));
	p1++;
	p2++;
    }
    return ((i4) 0);
}

/*{
** Name: oph_interpolate	- linearly interpolate between minv and maxv
**
** Description:
**      Calculates the proportion of items in the cell which satisfy the 
**      predicate "var <operator> const".
**
**      The cell boundaries are minv and maxv, and this routine will interpolate
**      if necessary (i.e. if minv < const <= maxv ).  Otherwise, a percentage
**      of 0% or 100% is returned, depending on whether the predicate selects 
**      the tuples represented by the cell.
**
**      FIXME - this routine should be replaced by an algorithm which does
**      a binary search on the histogram cells with "const"
**
**      FIXME - there should be an ADF routine which handles the unique
**      characteristics of histogram compares - in particular oph_compare
**
**	Called by: oph_sarg
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      minv                            ptr to lower boundary of histogram
**                                      cell
**      maxv                            ptr to upper boundary of histogram
**                                      cell
**      const                           ptr to const of the predicate where
**                                      var <operator> const; where "var"
**                                      is implicitly associated with the
**                                      histogram cell
**      operator                        type of key; i.e. ADC_KHIGHKEY,
**                                      ADC_KLOWKEY, or ADC_KEXACTKEY
**      datatype                        datatype of the histogram element
**      cellcount                       number of tuples selected by this cell
**      reptf                           repitition factor for domain in this
**                                      relation ... (number of tuples)/
**                                      (number of elements in active domain)
**                                      - used with equality so that a better
**                                      estimate on histogram cells, with many
**                                      possible values, is made
**	opno				Operator in original predicate (before
**					normalized by opbcreate)
**
** Outputs:
**	Returns:
**	    OPN_PERCENT - percentage of cell selected after applying this
**          predicate
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-may-86 (seputis)
**          initial creation
**	20-apr-90 (seputis)
**	    removed isinteger test for adjusting the ranges, the logic is now placed
**	    in the boolean factors opbcreate.c where the histogram values for keys
**	    are created, i.e. the histogram values for <= are converted to <
**	    using the oph_hinc increment by smallest value when appropriate
**	    and > are converted to >= also by using the histinc instruction
**          - this is not symmetrical, but is done this way because a histogram
**	    cell is defined to be hist-boundary < cell values <= histboundary
**	    - it makes this routine simpler but it makes the opbcreate routine
**	    more complicated
**	09-nov-92 (rganski)
**	    Simplified further still. Now that histograms are modified before
**	    enumeration to include all predicate keys (by oph_catalog()), this
**	    routine only returns 0% or 100%. This obviates most calls to
**	    opn_diff() and the key comparison for min_compare. We only check
**	    to see if the key is equal to maxv for ADC_KEXACTKEY matches, or
**	    less than maxv for ADC_KLOWKEY (in which case we return 100%) or
**	    ADC_KHIGHKEY (in which case we return 0%).
**	    A little clarification on previous history comment: an expression
**	    like "constant <= column" (ADC_LOWKEY) is converted to
**	    "newconstant < column", and "constant > column" (ADC_HIGHKEY) is
**	    converted to "newconstant >= column", where newconstant immediately
**	    precedes constant in magnitude.
**	    For ADC_KEXACTKEY, assume that a matching cell is a single-value
**	    cell, since oph_catalog() ensures that a single-value cell exists
**	    in the histogram for every applicable equality predicate.
**	    Simplified ADC_KEXACTKEY processing accordingly, and removed call
**	    to opn_diff() for cellwidth.
**	21-jun-93 (rganski)
**	    b50012: added special case handling for case where
**	    constant = minv = maxv. For b50012, this happens when there is a
**	    null string in a type c column. Needed to add opno parameter, since
**	    behavior for this special case is different when operator is <= or
**	    >= rather than < or > : the problem is that the cell splitting does
**	    not work for the cases col1 < '' and col1 >= '', which depends on
**	    decrementing the histogram value: this decrementation does not work
**	    for the null string.
**	    Added xDEBUG consistency check for case where constant falls inside
**	    cell but is not equal to one of the boundary values. This should
**	    not happen because of the histogram cell splitting in
**	    oph_catalog(). This requires adding new parameter to see if
**	    histogram is a default histogram; added histogram pointer param hp,
**	    which also allows removing reptf param, which can be dereferenced
**	    from hp.
[@history_line@]...
*/
OPN_PERCENT
oph_interpolate(
	OPS_SUBQUERY       *subquery,
	PTR                minv,
	PTR                maxv,
	PTR                constant,
	OPB_SARG           operator,
	DB_DATA_VALUE      *datatype,
	OPO_TUPLES	   cellcount,
	ADI_OP_ID	   opno,
	OPH_HISTOGRAM	   *hp)
{
    OPN_PERCENT         ret_val;	/* return value - percent of original
                                        ** histogram cell selected */
    DB_DT_ID            type;           /* ADF data type of the histogram */
    i4			key_compare;	/* negative if key < maxv, 
					** 0 if key == maxv
					*/

    type = datatype->db_datatype;

    /* Compare key to maxv */
    if (type == DB_TXT_TYPE)
    {   /* special case for text histograms */
	key_compare = oph_txtcompare(constant, maxv, datatype);
    }
    else
    {   /* handle other datatypes */
	key_compare = opu_compare(subquery->ops_global, constant, maxv,
				  datatype);  
    }

# ifdef xDEBUG
    if (!hp->oph_default)
    {
	/* Consistency check for case where constant falls in cell but does not
	** equal either boundary value. This should not happen, because of cell
	** splitting in oph_catalog() */
	i4		min_compare;	/* where <0 means minv < const */

	if (type == DB_TXT_TYPE)
	    min_compare = oph_txtcompare(minv, constant, datatype);
	else
	    min_compare = opu_compare(subquery->ops_global, minv, constant,
				      datatype);

	if (min_compare < 0 && key_compare < 0)
	    /* Constant is between boundaries but not equal to either */
	    opx_rerror(subquery->ops_global->ops_caller_cb, E_OP03A5_KEYINCELL);
    }
# endif /* xDEBUG */

    switch (operator)
    {
    case ADC_KEXACTKEY: 
    case ADC_KNOKEY:			/* this a "<>" (NE) relop. It works
					** the same as EXACT, but caller
					** inverts its meaning */
	if (key_compare == 0)
	{   
	    /* key == maxv. Assume that the cell is a single-value cell */
	    
	    /* FIXME - phase 3: The only situation where this does not return
	    ** 100% is when the type is float, and cellcount > reptf; then it
	    ** returns reptf/cellcount, which assumes you can't return more
	    ** than reptf tuples. Change this to return 100% always. */ 
	    /* I agree with the comment. This was the whole point of moving
	    ** interpolation to the splitcell routines. I shall remove the
	    ** offending code.    inkdo01 (1-may-97) */

	    ret_val = OPN_100PC;	/* return 100% if equal */
	}
	else
	    ret_val = (OPN_0PC);
	break;

    case ADC_KLOWKEY: 
	if (key_compare < 0)
	    /* constant < maxv */
	    ret_val = (OPN_100PC);
	else
	{
	    ret_val = (OPN_0PC);
	    /* b50012: if constant starts with 0 byte and is equal to maxv, see
	    ** if it is also equal to minv. If so, return 100% if opno is
	    ** ADI_LE_OP or ADI_GE_OP (see history comment above) */
	    if (*(char *)constant == (char)0 && !key_compare)
		if (!opu_compare(subquery->ops_global, minv, constant, datatype))
		    /* Constant is also equal to minv */
		    if (opno == ADI_LE_OP || opno == ADI_GE_OP)
			ret_val = (OPN_100PC);
	}
	break;

    case ADC_KHIGHKEY: 
	if (key_compare < 0)
	    /* constant < maxv */
	    ret_val = (OPN_0PC);
	else
	{
	    ret_val = (OPN_100PC);
	    /* b50012: if constant starts with 0 byte and is equal to maxv, see
	    ** if it is also equal to minv. If so, return 0% if opno is not
	    ** ADI_LE_OP or ADI_GE_OP (see history comment above) */
	    if (*(char *)constant == (char)0 && !key_compare)
		if (!opu_compare(subquery->ops_global, minv, constant, datatype))
		    /* Constant is also equal to minv */
		    if (opno != ADI_LE_OP && opno != ADI_GE_OP)
			ret_val = (OPN_0PC);
	}
	break;
    }
    return (ret_val);
}
