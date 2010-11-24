/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <me.h>
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
#include    <opckey.h>
#include    <opcd.h>
#define        OPC_KEY		TRUE
#include    <bt.h>
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCKEY.C - Routines to build OPC_KEY structs.
**
**  Description:
**      This file contains the routines needed to build up the OPC_KEY structs 
**      that OPC needs to optimally key into relations. These routines are only 
**      used by the orig node building routines (opcorig.c). As such, this file 
**      only builds keys for orig nodes. Keys for join nodes are built in  
**      opcjcommon.c (opc_kjkeyinfo()). 
**
**	Externally visable routines:
**          opc_srange() - Set up a single range for keying with repeat query
**				parameters.
**          opc_meqrange() - Set up multiple equality ranges for keying with
**				repeat query parameters.
**          opc_range() - Set up for full keying with multiple ranges
**
**	Internal only routines:
**          opc_kfuse() - Combine (fuse?) overlapping ranges (OPC_KOR structs).
**          opc_orcopy() - Make a copy of a range (an OPC_KOR struct).
**          opc_remkor() - Remove a range (OPC_KOR struct) from its queue of
**				ranges.
**          opc_rcompare() - Compare two ranges and return which is higher.
**          opc_complex_range() - Detect and simplify overly complex ranges.
**
**
**  History:    
**      31-aug-86 (eric)
**          created
**      15-aug-89 (eric)
**          Changed the way that repeat query parameters/dbp vars/expressions
**          are used in keying.
**	15-jun-90 (stec)
**	    Fixed bug 30416 in opc_srange().
**	26-nov-90 (stec)
**	    Modified opc_kfuse().
**	29-nov-90 (stec)
**	    Modified opc_srange to fix bug 33994.
**	28-dec-90 (stec)
**	    Modified opc_kfuse().
**      17-aug-91 (eric)
**          Fixed bugs 37437 and 36518 in opc_range() and opc_srange() 
**          respectively. See the routines for further details.
**      12-nov-91 (anitap)
**          Changed the casting of opu_Gsmemory_get() in opc_range() as
**          su3 compiler was complaining.
**      11-dec-91 (anitap)
**          Fixed bug 41358 in opc_range(). See the routine for further
**          details.
**      01-jan-92 (anitap)
**          Fixed bug 41989 in opc_srange(). See the routine for further
**          details.
**      01-jan-92 (anitap)
**          Fixed bug 42033 in opc_range(). See the routine for further
**          details.
**	09-mar-92 (anitap)
**          Fixed bug 35506 in opc_kfuse(). See the routine for further details.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-nov-93 (anitap)
**	    Fix for bug 55408/55404 in opc_srange(). Though opc_srange was 
**	    building a range key, it was not setting opc_inequality to TRUE 
**	    which caused QEF to think that it was dealing with an equality key.	
**       7-oct-94 (inkdo01)
**          Fix for bug 65267 which fixes the fix for 55408/55404. The old 
**          "==" instead of "=" in an assignment trick.
**      15-nov-2000 (devjo01)
**          Back out anitap's fix for 35506, it was insufficient,
**          and fix for 103164 made it redundant.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_srange(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
void opc_meqrange(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
void opc_range(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
static void opc_kfuse(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or,
	i4 *lo_keyno,
	i4 *hi_keyno);
static bool opc_ne_bop(
	PST_QNODE *nodep);
static void opc_orcopy(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *src,
	OPC_KOR **pdest,
	i4 *lo_keyno,
	i4 *hi_keyno);
static void opc_remkor(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *dnf_or);
static i4 opc_rcompare(
	OPS_STATE *global,
	OPC_QUAL *cqual,
	OPC_KOR *ldnf_or,
	i4 lside,
	OPC_KOR *rdnf_or,
	i4 rside);
static void opc_complex_range(
	OPS_STATE *global,
	OPC_QUAL *cqual);

/*
** Constants:
*/
#define	    OPC_RMAX_RANGES	50

/*{
** Name: OPC_SRANGE	- Set up a single range for keying
**
** Description:
**      Unlike opc_range(), which will figure out keying in its full glory, 
**      opc_srange will set up keying using a single range. In other words, 
**      there will be a single upper and lower bound. Although it will work
**      in any situation, it's currently used when there are repeat query 
**      parameters present in the query tree fragment that is being used. 
**        
**      The pseudo code for this is as follows: 
**
**      opc_srange()
**      { 
**	    for (each conjunct in the keying qualification) 
**	    {
**		make sure that this routine can use this conjunct. Conjuncts
**		that cannot be used include ones that refer to more than
**		one key attribute and conjuncts that mix low range and high
**		range comparisons.
**
**		For (each comparison in the conjunct) 
**		{ 
**		    if (this comparison doesn't use a repeat query parameter)
**		    {
**			if (the range has no lower bound or
**				the comparisons lower bound is lower 
**				than the range's lower bound 
**			    ) 
**			{ 
**			    use the comparison's lower bound instead of 
**				the range's current lower bound;
**			} 
**
**			if (the range has no upper bound or
**				the comparisons upper bound is higher than 
**				the range's upper bound 
**			    ) 
**			{ 
**			    use the comparison's upper bound instead of 
**				the range's current lower bound;
**			} 
**		    } 
**		    else this comparison uses a repeat query parameter or a
**			    DBP local var
**		    { 
**			** Fix for bug 30416 - do not miss higher(lower)
**			** bounds equal to higher(lower) bounds of keyed
**			** domain.
**			if (this comparison has a high key value)
**			{
**			    use it in the upper range; 
**			} 
**			if (this comparison has an low key value)
**			{
**			    use it in the lower range; 
**			} 
**		    } 
**		} 
**
**		For (each element in the conjunct's range)
**		{
**		    if (the master range has no lower range or
**			    if the conjunct's lower bound is higher
**			    than that for the master range
**			)
**		    {
**			use the conjunct's lower bound in the master range;
**		    }
**
**		    if (the master range has no upper range or
**			    if the conjunct's upper bound is lower
**			    than that for the master range
**			)
**		    {
**			use the conjunct's upper bound in the master range;
**		    }
**		}
**	    } 
**      } 
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC. This is used
**	mostly as a parameter to all OPC routines.
**  cqual -
**	This is the struct that holds all keying info. It is expected that
**	all fields, except for opc_dnf_qual, are correctly filled in. This
**	is the primary source of information for the translation from a
**	conjunctive (cnf) to a disjunctive (dnf) tree.
**  dnf_or -
**	This is a DNF clause that has been allocated and initialized with
**	default values (ie. values that, if left alone, would result in a
**	full scan of the relation).
**
** Outputs:
**  cqual->opc_dnf_qual -
**	This will point to dnf_or, which will be the only dnf clause.
**  dnf_or -
**	The values in this will be, potentially, completely reset to indicate
**	what single range to use to search the relation in question.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except those that opx_error and opx_verror raise.
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      31-aug-87 (eric)
**          written
**	15-jun-90 (stec)
**	    Fixed bug 30416. Section of the code initializing bitmaps for
**	    higher on lower bounds of the range in case of repeat query parms/
**	    DBP local vars has been modified. Th problem was that in the case
**	    of r.a = :rqparm1 and r.a > :rqparm2, upper and lower range bounds
**	    were computed as follows:
**	    -	equality comparison would have the lower and upper bounds
**		set to :rqparm1
**	    -	inequality comparison would have the lower bound set to
**		:rqparm2 and upper bound to indicate the end of the key
**		attribute domain.
**	    Bitmaps of repeat query parms/DBP lvars used to determine the lower
**	    (upper) range boundary did not reflect the fact that one of the
**	    comparisons specified the upper bound of the key domain. Therefore
**	    bitmaps in cases of mixed equality and inequality comparisons
**	    (see example above) as well as other queries would be wrong.
**	    The problem would generally occur when query specified more than
**	    one range, and their lower(upper) bounds would represent the lower
**	    (upper) bound of the keyed domain and other values at the same time.
**	    Generated bitmaps were wrong because a wrong range was specified. 
**	    This resulted in wrong query results, since the range specified was
**	    smaller than it should have been.
**	    The fix was to mark the fact that a comparison had upper(lower)
**	    range bound equal to upper(lower) bound of the key domain and reset
**	    the bitmap ptr to NULL to indicate that the effective upper(lower)
**	    range bound is to be equal to that of the key domain. The same
**	    problem exists in the section of the code dealing with non-repeat
**	    query comparisons and an identical solution has been adopted.
**	    When fixing the problem one needs to keep in mind the fact that
**	    the key can consist of a number of attributes and range boundaries
**	    for each attribute are computed separately. This is the reason for
**	    having arrays of upper(lower) bound seen indicators.
**	29-nov-90 (stec)
**	    Code determining key type has been changed to fix bug 33994.
**      18-aug-91 (eric)
**          Fixed bug 36518, which was caused by Alex's fix for bug 30416
**          (mentioned above). The fix is to correctly calculate the lower
**          and upper ranges for keying. In bug 30416, opc_srange was
**          calculating too small a range (which excluded data). The fix
**          for this expanded the ranges too much. The correct way to
**          calculate the lower bound is to do a min of the mins within 
**          a conjunct and do a max of the mins between conjuncts. The
**          upper bound works likewise (max of maxs w/in conjuncts and
**          min of maxs between them).
**      1-jan-92 (anitap)
**         Fixed bug 41989, where repeated delete failed. While determining
**         the ranges for keying, the upper bound was being compared like a
**         lower bound.
**	20-JUL-92 (rickh)
**	   Add some pointer casts in mefill calls.
**	15-nov-93 (anitap)
**	    Fix for bug 55408/55404. Though opc_srange was building a range 
**	    key, it was not setting opc_inequality to TRUE which caused QEF to
**	    think that the it was dealing with an equality key.	
**	23-Mar-09 (kibro01) b121835
**	    Allow srange to run after meqrange has produced a partial series
**	    of actions, so a combination of exact OR clauses can then have
**	    the inexact/range conditions added to them.  Note that 
**	    opc_inequality has changed from an effective boolean to a number
**	    detailing the key sequence number where inequality begins.
**      26-Mar-09 (kibro01) b121835
**          Only combine opc_meqrange and opc_srange if there is more than one
**	    OR condition, so there can be some benefit.
[@history_template@]...
*/
VOID
opc_srange(
		OPS_STATE   *global,
		OPC_QUAL    *cqual,
		OPC_KOR	    *dnf_or)
{
    OPC_QAND	    *cnf_and;
    OPC_QCMP	    *cnf_comp;

	/* Ws_dnf_and_list is a workspace key description. It is a pointer
	** to an array of descriptions, one for each key attribute.
	** Ws_dnf_and is a pointer to one of the elements in the 
	** ws_dnf_and_list. Res_dnf_and is similiar to ws_dnf_and
	** except that it points into dnf_or->opc_ands, which is 
	** the result key description.
	*/
    OPC_KAND	    *ws_dnf_and_list;
    OPC_KAND	    *ws_dnf_and;
    OPC_KAND	    *res_dnf_and;
    OPC_KOR         *orig_opc_dnf_qual;

    OPC_KCONST	    *ckconst;
    OPC_QCONST	    *cqconst;

	/* Minmax_qconst represents the min or max (depending on whether
	** opc_khi or opc_klo points to it) possible key value.
	** Minmax_kparm is the same thing except that it is for dbp and
	** repeat query parameter key values. In other words, minmax_kparm
	** represents the begining or end of the relation for the key att.
	*/
    OPC_QCONST	    minmax_qconst;
    PTR		    minmax_kparm;

    i4		    keyno;
    i4		    compno;
    i4		    conjunct_useful;
    i4		    key_type;
    i4		    parmno;
    i4		    szkparm_byte =
			    (cqual->opc_qsinfo.opc_hiparm / BITSPERBYTE) + 1;

	/* Ws_kphi and ws_kplo are pointers to arrays of pointers to
	** bit maps. The array of pointers is large enough to hold all
	** of the pointers to bit maps that will be needed by ws_dmf_ands.
	*/
    PTR		    *ws_kphi;
    PTR		    *ws_kplo;
    i4		    and_size;

    orig_opc_dnf_qual = cqual->opc_dnf_qual;

    /* Begin by initializing the key to a full scan */
    cqual->opc_dnf_qual = dnf_or;
    dnf_or->opc_ornext = dnf_or->opc_orprev = NULL;

    /* allocate and initialize the workspace disjunctive clause */
    and_size = sizeof (OPC_KAND) * cqual->opc_qsinfo.opc_nkeys;
    ws_dnf_and_list = (OPC_KAND *) opu_Gsmemory_get(global, and_size);
    MEfill(and_size, (u_char)0, (PTR) ws_dnf_and_list);
    for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	ws_dnf_and_list[keyno].opc_krange_type = ADE_1RANGE_DONTCARE;
    }

    /* Describe the min/max qconst. By convention, a NULL point for the data
    ** indicates a min or max data value depending on the context.
    */
    minmax_qconst.opc_tconst_type = PST_USER;
    minmax_qconst.opc_keydata.opc_data.db_data = NULL;
    minmax_qconst.opc_const_no = -1;
    minmax_qconst.opc_qnext = NULL;
    minmax_qconst.opc_qlang = DB_SQL;
    minmax_qconst.opc_qop = NULL;
    minmax_qconst.opc_dshlocation = -1;
    minmax_qconst.opc_key_type = ADC_KNOKEY;

    /* initialize the minmax_kparm, ws_kphi and ws_kplo fields (all used
    ** to find the min of mins and max of maxs).
    */
    ws_kphi = (PTR *) opu_Gsmemory_get(global, 
				sizeof (PTR) * cqual->opc_qsinfo.opc_nkeys);
    MEfill(sizeof (PTR) * cqual->opc_qsinfo.opc_nkeys, (char) 0, (PTR) ws_kphi);
    ws_kplo = (PTR *) opu_Gsmemory_get(global, 
				sizeof (PTR) * cqual->opc_qsinfo.opc_nkeys);
    MEfill(sizeof (PTR) * cqual->opc_qsinfo.opc_nkeys, (char) 0, (PTR) ws_kplo);
    minmax_kparm = opu_Gsmemory_get(global, szkparm_byte);
    MEfill(szkparm_byte, 0, minmax_kparm);

    /* For each conjunct in the keying qualification */
    for (cnf_and = cqual->opc_cnf_qual; 
	    cnf_and != NULL;
	    cnf_and = cnf_and->opc_anext
	)
    {
	/* first, lets check to see if this is a conjunct that we can use */
	conjunct_useful = TRUE;
	keyno = -1;  /* set keyno to an illegal value to insure initialization */
	for (compno = 0; compno < cnf_and->opc_ncomps; compno += 1)
	{
	    cnf_comp = &cnf_and->opc_comps[compno];

	    /* find the constant info for this comparison */
	    if (cnf_comp->opc_lo_const != NULL)
	    {
		cqconst = cnf_comp->opc_lo_const;
	    }
	    else if (cnf_comp->opc_hi_const != NULL)
	    {
		cqconst = cnf_comp->opc_hi_const;
	    }
	    else
	    {
		continue;
	    }

	    /* if keyno and key_type are uninitialized for this conjunct
	    ** then initialize them.
	    */
	    if (keyno == -1)
	    {
		keyno = cnf_comp->opc_keyno;
		key_type = cqconst->opc_key_type;
		continue;
	    }
	    else if (cnf_comp->opc_keyno != keyno)
	    {
		/* if this conjunct is on multiple keys then go on */
		conjunct_useful = FALSE;
		break;
	    }
    	} /* for each comparison */

	/* if this conjunct cannot be used for our style of keying, then
	** continue with the next conjunct
	*/
	if (conjunct_useful == FALSE)
	{
	    continue;
	}

	/* Find the min of the mins and the max of the maxs
	** For each key comparison that is ored
	** together within the current conjunct.
	*/
	for (compno = 0; compno < cnf_and->opc_ncomps; compno += 1)
	{
	    cnf_comp = &cnf_and->opc_comps[compno];
	    ws_dnf_and = &ws_dnf_and_list[cnf_comp->opc_keyno];

	    /*
	    ** The body of the IF statement below
	    ** has been modified to fix bug 30416
	    */

	    /* If this isn't a repeat query parameter */
	    if ((cnf_comp->opc_lo_const != NULL && 
		 cnf_comp->opc_lo_const->opc_tconst_type == PST_USER
		)
		||
		(cnf_comp->opc_hi_const != NULL &&
		cnf_comp->opc_hi_const->opc_tconst_type == PST_USER
		)
	       )
	    {
		/* Find the minimum of minimums */
		if (cnf_comp->opc_lo_const != NULL)
		{
		    /* if the key att being built (ws_dnf_and) has no lower
		    ** bound, or if the lower bound of the comparison is
		    ** lower than for the key att.
		    */
		    if (ws_dnf_and->opc_klo == NULL ||
			(ws_dnf_and->opc_klo != &minmax_qconst &&
			    cnf_comp->opc_lo_const->opc_const_no <
					ws_dnf_and->opc_klo->opc_const_no
			)
		       )
		    {
			/* Then use the new constant as 
			** the lower bound for our range.
			*/
			ws_dnf_and->opc_klo = cnf_comp->opc_lo_const;
		    }
		}
		else
		{
		    /* the current comparison's lower bound is the
		    ** beginning of the relation (min value). Therefore,
		    ** set the key value to the min value since we are
		    ** are looking for the min of the mins for lower
		    ** bounds in this conjunct (and the beginning of the
		    ** relation is lower than anything else).
		    */
		    ws_dnf_and->opc_klo = &minmax_qconst;
		}

		/* Find the maximum of maximums */
		if (cnf_comp->opc_hi_const != NULL)
		{
		    /* if the key att being built (ws_dnf_and) has no higher
		    ** bound, or if the higher bound of the comparison is
		    ** higher than for the key att.
		    */
		    if (ws_dnf_and->opc_khi == NULL ||
			(ws_dnf_and->opc_khi != &minmax_qconst &&
			    cnf_comp->opc_hi_const->opc_const_no > 
					ws_dnf_and->opc_khi->opc_const_no
			)
		       )
		    {
			/* Then use the new constant as 
			** the upper bound for our range.
			*/
			ws_dnf_and->opc_khi = cnf_comp->opc_hi_const;
		    }
		}
		else
		{
		    /* As with the lower bound version of this case, 
		    ** the current comparison's upper bound is the
		    ** end of the relation (max value). Therefore,
		    ** set the key value to the max value since we are
		    ** are looking for the max of the maxs for upper
		    ** bounds in this conjunct (and the end of the
		    ** relation is higher than anything else).
		    */
		    ws_dnf_and->opc_khi = &minmax_qconst;
		}
	    }
	    else /* This is a repeat query parameter */
	    {
		ckconst = &cqual->opc_kconsts[cnf_comp->opc_keyno];

		/* If this comparison has an upper bound */
		if (cnf_comp->opc_hi_const != NULL)
		{
		    /* if the key is not already as wide as it can be */
		    if (ws_dnf_and->opc_kphi != minmax_kparm)
		    {
			if (ws_dnf_and->opc_kphi == NULL)
			{
			    /* There's no high range parm bit map to use so
			    ** find one (allocate it or use one that was already
			    ** allocated).
			    */
			    if (ws_kphi[cnf_comp->opc_keyno] == NULL)
			    {
				ws_kphi[cnf_comp->opc_keyno] =
				    ws_dnf_and->opc_kphi =
					opu_Gsmemory_get(global, szkparm_byte);
			    }
			    else
			    {
				ws_dnf_and->opc_kphi = 
					    ws_kphi[cnf_comp->opc_keyno];
			    }
			    MEfill(szkparm_byte, 0, ws_dnf_and->opc_kphi);
			}

			/* set bit for repeat query parameter/DBP lvar */
			BTset(cnf_comp->opc_hi_const->opc_const_no, 
				    ws_dnf_and->opc_kphi);
		    }
		}
		else
		{
		    /*
		    ** When this statement is executed, then one of
		    ** the comparisons has the upper range bound specified
		    ** as upper bound of the domain of the key attribute.
		    ** For a conjunct all comparisons are ORed, therefore
		    ** the range to be searched is widened (only 1 range
		    ** to be generated for keying). So we want to reset
		    ** the upper bound info to indicate that the upper bound
		    ** of the range is going to be equal to the upper bound
		    ** of the domain.
		    */
		    ws_dnf_and->opc_kphi = minmax_kparm;
		}

		/* If this comparison has a lower bound */
		if (cnf_comp->opc_lo_const != NULL)
		{
		    /* if the key is not already as wide as it can be */
		    if (ws_dnf_and->opc_kplo != minmax_kparm)
		    {
			if (ws_dnf_and->opc_kplo == NULL)
			{
			    /* There's no low range parm bit map to use so
			    ** find one (allocate it or use one that was already
			    ** allocated).
			    */
			    if (ws_kplo[cnf_comp->opc_keyno] == NULL)
			    {
				ws_kplo[cnf_comp->opc_keyno] =
				    ws_dnf_and->opc_kplo =
					opu_Gsmemory_get(global, szkparm_byte);
			    }
			    else
			    {
				ws_dnf_and->opc_kplo = 
					    ws_kplo[cnf_comp->opc_keyno];
			    }
			    MEfill(szkparm_byte, 0, ws_dnf_and->opc_kplo);
			}

			/* set bit for repeat query parameter/DBP lvar */
			BTset(cnf_comp->opc_lo_const->opc_const_no, 
			    ws_dnf_and->opc_kplo);
		    }
		}
		else
		{
		    /*
		    ** When this statement is executed, then one of
		    ** the comparisons has the lower range bound specified
		    ** as lower bound of the domain of the key attribute.
		    ** For a conjunct all comparisons are ORed, therefore
		    ** the range to be searched is widened (only 1 range
		    ** to be generated for keying). So we want to reset
		    ** the lower bound info to indicate that the lower bound
		    ** of the range is going to be equal to the lower bound
		    ** of the domain.
		    */
		    ws_dnf_and->opc_kplo = minmax_kparm;
		}
	    } /* else repeat query parm */
	} /* for each key comparison */

	/* At this point, ws_dnf_ands holds the key range that the current
	** conjunct describes. We now need to merge this into the result
	** key range. Because conjuncts are ANDed together, we merge them
	** by taking the max of the lower bounds and the min of the upper
	** bound. If you think that this is wrong, think about it some more
	** (because it's right).
	*/
	for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
	{
	    ws_dnf_and = &ws_dnf_and_list[keyno];
	    res_dnf_and = &dnf_or->opc_ands[keyno];

	    /* If must be a high bound for this key att that is not
	    ** the end of the relation.
	    */
	    if (ws_dnf_and->opc_khi != &minmax_qconst &&
		    ws_dnf_and->opc_kphi != minmax_kparm &&
		    (ws_dnf_and->opc_khi != NULL ||
			ws_dnf_and->opc_kphi != NULL
		    )
	       )
	    {
		/* If a high bound key value is in the workspace */
		if (ws_dnf_and->opc_khi != NULL)
		{
		    /* if the result key has no high bound key value */
		    if (res_dnf_and->opc_khi == NULL)
		    {
			/* then use the one from the workspace */
			res_dnf_and->opc_khi = ws_dnf_and->opc_khi;
		    }
		    else if (res_dnf_and->opc_khi->opc_const_no >
				    ws_dnf_and->opc_khi->opc_const_no &&
				    ws_dnf_and->opc_kphi == NULL
			    )
		    {
			/* else if the high bound in the workspace is lower
			** than the high bound in the result then use the
			** workspaces high bound (ie. do a min of the maxs).
			** Note that we can only do this if there workspace
			** doesn't have any dbp or repeat param high bound
			** values. This is because we can only do a min of
			** maxs only when we completely do a max of maxs
			** within a conjunct. Because repeat query parameters
			** can't be compared, we can't do the max of maxs
			** therefore we can't do the min of max either.
			** In this case, we leave the range a little wider
			** than it should be, but that's better than
			** missing rows that should be returned.
			*/
			res_dnf_and->opc_khi = ws_dnf_and->opc_khi;
		    }
		}

		/* If a high bound key param values is in the workspace */
		if (ws_dnf_and->opc_kphi != NULL)
		{
		    /* If the result key struct doesn't have any parms yet
		    ** then allocate it. Otherwise, just OR the workspace
		    ** param bits into the result.
		    */
		    if (res_dnf_and->opc_kphi == NULL)
		    {
			res_dnf_and->opc_kphi =
					opu_Gsmemory_get(global, szkparm_byte);
			MEcopy(ws_dnf_and->opc_kphi, szkparm_byte,
						res_dnf_and->opc_kphi);
		    }
		    else
		    {
			BTor(BITS_IN(szkparm_byte), ws_dnf_and->opc_kphi,
						    res_dnf_and->opc_kphi);
		    }
		}
	    }

	    /* Clear out the work space for the next clause to
	    ** use.
	    */
	    ws_dnf_and->opc_khi = NULL;
	    ws_dnf_and->opc_kphi = NULL;

	    /* Do the same thing for the lower. */
	    if (ws_dnf_and->opc_klo != &minmax_qconst &&
		    ws_dnf_and->opc_kplo != minmax_kparm &&
		    (ws_dnf_and->opc_klo != NULL ||
			ws_dnf_and->opc_kplo != NULL
		    )
	       )
	    {
		/* If a low bound key value is in the workspace */
		if (ws_dnf_and->opc_klo != NULL)
		{
		    /* if the result key has no low bound key value */
		    if (res_dnf_and->opc_klo == NULL)
		    {
			/* then use the one from the workspace */
			res_dnf_and->opc_klo = ws_dnf_and->opc_klo;
		    }
		    else if (res_dnf_and->opc_klo->opc_const_no <
				    ws_dnf_and->opc_klo->opc_const_no &&
				    ws_dnf_and->opc_kplo == NULL
			    )
		    {
			/* else if the low bound in the workspace is lower
			** than the low bound in the result then use the
			** workspaces low bound (ie. do a min of the maxs).
			** Note that we can only do this if there workspace
			** doesn't have any dbp or repeat param low bound
			** values. This is because we can only do a min of
			** maxs only when we completely do a max of maxs
			** within a conjunct. Because repeat query parameters
			** can't be compared, we can't do the max of maxs
			** therefore we can't do the min of max either.
			** In this case, we leave the range a little wider
			** than it should be, but that's better than
			** missing rows that should be returned.
			*/
			res_dnf_and->opc_klo = ws_dnf_and->opc_klo;
		    }
		}

		/* If a low bound key param values is in the workspace */
		if (ws_dnf_and->opc_kplo != NULL)
		{
		    /* If the result key struct doesn't have any parms yet
		    ** then allocate it. Otherwise, just OR the workspace
		    ** param bits into the result.
		    */
		    if (res_dnf_and->opc_kplo == NULL)
		    {
			res_dnf_and->opc_kplo =
					opu_Gsmemory_get(global, szkparm_byte);
			MEcopy(ws_dnf_and->opc_kplo, szkparm_byte,
						res_dnf_and->opc_kplo);
		    }
		    else
		    {
			BTor(BITS_IN(szkparm_byte), ws_dnf_and->opc_kplo,
						    res_dnf_and->opc_kplo);
		    }

		}
	    }

	    /* Clear out the low range work space for the next clause to
	    ** use.
	    */
	    ws_dnf_and->opc_klo = NULL;
	    ws_dnf_and->opc_kplo = NULL;
	}
    } /* for each conjunct */

    /* For each key attribute, lets figure out whether we have described
    ** an exact or range key. If we are using a hash relation and this 
    ** is a range then the key is not useful.
    */
    for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	res_dnf_and = &dnf_or->opc_ands[keyno];

	/* Assume an exact key unless we see otherwise */
	res_dnf_and->opc_krange_type = ADE_3RANGE_NO;

	/* if the non-repeat query parameter/DBP local var low key value is 
	** different from the high key value then this describes a range
	*/
	if (res_dnf_and->opc_klo != res_dnf_and->opc_khi)
	{
	    res_dnf_and->opc_krange_type = ADE_2RANGE_YES;
	}

	/* if no hi or low keys are given then this describes a range */
	if (res_dnf_and->opc_khi == NULL && res_dnf_and->opc_klo == NULL &&
		res_dnf_and->opc_kphi == NULL && res_dnf_and->opc_kplo == NULL
	    )
	{
	    res_dnf_and->opc_krange_type = ADE_2RANGE_YES;
	}

	/* if repeat query parameters or DBP local vars are specified
	** for this key 
	*/
	if (res_dnf_and->opc_kphi != NULL || res_dnf_and->opc_kplo != NULL)
	{
	    /*
	    ** If the low and high key values use different repeat
	    ** query parameters/DBP local vars or there are more than one 
	    ** values then this is a range.
	    ** Last 2 disjuncts were added to fix bug 33994.
	    */
	    if (res_dnf_and->opc_kphi == NULL || 
		res_dnf_and->opc_kplo == NULL ||
		MEcmp(res_dnf_and->opc_kphi, res_dnf_and->opc_kplo,
							szkparm_byte) != 0 ||
		BTcount(res_dnf_and->opc_kphi, 
					cqual->opc_qsinfo.opc_hiparm) != 1 ||
		res_dnf_and->opc_khi != NULL || res_dnf_and->opc_klo != NULL
	       )
	    {
		res_dnf_and->opc_krange_type = ADE_2RANGE_YES;
	    }
	    else
	    {
		/* in this case, there can only be one repeat query
		** parameter/DBP local var that is common for both the
		** low and high key values for this disjunct. So let's
		** find the OPC_QCONST struct for it.
		*/
		parmno = BTnext(-1, res_dnf_and->opc_kphi, 
						cqual->opc_qsinfo.opc_hiparm);
		if (global->ops_procedure->pst_isdbp == TRUE)
		{
		    cqconst = cqual->opc_kconsts[keyno].opc_dbpvars[parmno];
		}
		else
		{
		    cqconst = cqual->opc_kconsts[keyno].opc_qparms[parmno];
		}

		if (cqconst->opc_key_type != ADC_KEXACTKEY &&
			cqconst->opc_key_type != ADC_KNOMATCH
		    )
		{
		    res_dnf_and->opc_krange_type = ADE_2RANGE_YES;
		}
	    }
	}

        if(res_dnf_and->opc_krange_type == ADE_2RANGE_YES &&
	   cqual->opc_qsinfo.opc_inequality == 0)
	{
	  cqual->opc_qsinfo.opc_inequality = 1;
	}


	if (cqual->opc_qsinfo.opc_storage_type == DMT_HASH_TYPE &&
		res_dnf_and->opc_krange_type != ADE_3RANGE_NO
	    )
	{
	    /* We've found a key attribute that either didn't have a 
	    ** restriction, had more than one restriction, and/or wasn't
	    ** an equality restriction. This means that we can't use any
	    ** of the restrictions, so lets tell the caller to do a full
	    ** scan of the file.
	    */
	    cqual->opc_dnf_qual = NULL;
	    return;
	}
    } /* for each key attribute */

    /* If we're combining the opc_meqrange with the opc_srange builds of keys
    ** we need to do so now.  Only do this if there are multiple ORs.
    */
    if (cqual->opc_qsinfo.opc_inequality > 1 &&
	cqual->opc_dnf_qual &&
	orig_opc_dnf_qual &&
	orig_opc_dnf_qual->opc_ornext)
    {
	OPC_KOR *srange_kor = cqual->opc_dnf_qual;
	OPC_KOR *this_kor;
	cqual->opc_dnf_qual = orig_opc_dnf_qual;
	
	for (this_kor = cqual->opc_dnf_qual; this_kor;
		this_kor = this_kor->opc_ornext)
	{
	    for (keyno = cqual->opc_qsinfo.opc_inequality - 1;
		 keyno < cqual->opc_qsinfo.opc_nkeys;
		 keyno++)
	    {
		this_kor->opc_ands[keyno] = 
			srange_kor->opc_ands[keyno];
	    }
	}
    }
}

/*{
** Name: OPC_MEQRANGE	- Set up multiple equality ranges for keying with
**			    repeat query parameters.
**
** Description:
**      Unlike opc_range(), which will figure out keying in its full
**      generality, opc_meqrange will set up keying using multiple equality 
**      ranges. To do this, we make the requirement that the key qualification
**      use only the first key attribute. This restriction is needed 
**      because of the existence of repeat query parameters, which means 
**      that we cannot have multiple conjuncts in the key qualification
**      To understand the reasons behind the incompatiblity of ANDs, ORs, and 
**      repeat query parameters, please read the comments for opc_range(). 
**
**      The pseudo code for this is as follows: 
**
**	opc_meqrange()
**	{
**	    if (this key qual contains an inequality)
**	    {
**		return a NULL to the caller, indicating no ranges;
**	    }
**
**	    Find a conjunct for each key att that refers only to that
**		key att;
**
**	    For (each key atts conjunct)
**	    {
**		If (this key atts conjuct brings the total number of
**			ranges above OPC_RMAX_RANGE
**		    )
**		{
**		    Null out this key atts conjunct and all sucedding
**			key att conjuncts;
**		}
**	    }
**
**	    if (the first key att has no conjunct
**			or
**		    this is a hash relation and not all keys are given
**		)
**	    {
**		return a NULL to the caller, indicating no ranges;
**	    }
**
**	    for (ever)
**	    {
**		Remove the last comparison for the current key att from
**		    dnf_or;
**
**		move on to the next comparison in the current key atts
**		    conjunct;
**
**		if (there are no more comparisons in the current conjunct)
**		{
**		    continue with the previous key atts conjunct;
**		}
**
**		save the old range for the current keyno;
**
**		Add this comparison to dnf_or;
**
**		if (we are dealing with the last key att)
**		{
**		    make a copy of dnf_or and add it to the dnf qual list;
**		}
**		else
**		{
**		    continue with the next conjunct for the next key att;
**		}
**	    }
**
**	    return the kor list;
**	}
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC. This is used
**	mostly as a parameter to all OPC routines.
**  cqual -
**	This is the struct that holds all keying info. It is expected that
**	all fields, except for opc_dnf_qual, are correctly filled in. This
**	is the primary source of information for the translation from a
**	conjunctive (cnf) to a disjunctive (dnf) tree.
**
** Outputs:
**  cqual->opc_dnf_qual -
**	This will point to one or more OPC_KOR structs.
**	The values in each OPC_KOR struct will be set to indicate an exact
**	match range to search. If no exact match ranges are found then this
**	will be set to NULL.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none, except for those raised by opx_error and opx_verror.
**
** Side Effects:
**	    none
**
** History:
**      1-aug-87 (eric)
**          written
**      1-may-89 (eric)
**          Fixed bug 5424 bug setting the opc_krange_type to ADE_3RANGE_NO.
**          This insures that the keys that this routine builds are always
**          used for exact matches.
**	31-aug-00 (inkdo01)
**	    Remove constant OPC_RMAX_RANGES in favour of CBF constant.
**	23-Mar-09 (kibro01) b121835
**	    Allow srange to run after meqrange has produced a partial series
**	    of actions, so a combination of exact OR clauses can then have
**	    the inexact/range conditions added to them.  Note that 
**	    opc_inequality has changed from an effective boolean to a number
**	    detailing the key sequence number where inequality begins.
**      26-Mar-09 (kibro01) b121835
**          Only combine opc_meqrange and opc_srange if there is more than one
**	    OR condition, so there can be some benefit.
**	18-Jun-09 (kibro01) b1222207
**	    Wrong number of results returned when the OR clause is the final
**	    segment of a complete key.
**      09-nov-2009 (huazh01)
**          modify/restrict the fix to b121835. Do not build keys for 
**          "<", "<=", ">", ">=" and other operators that will make 
**          adc_keybld() builds a non ADC_KEXACTKEY key. opc_meqrange() 
**          set opc_krange_type to ADE_3RANGE_NO, which conflicts with the 
**          non ADC_KEXACTKEY adc_tykey set by adc_keybld(). 
**          (b122625)
**      17-Nov-2009 (hanal04) Bug 122831
**          Extend fix for Bug 122625 to scan the conjunct node and all
**          nodes under the conjunct node for a non-exact BOP.
[@history_template@]...
*/
VOID
opc_meqrange(
		OPS_STATE   *global,
		OPC_QUAL    *cqual,
		OPC_KOR	    *dnf_or)
{
    OPC_QAND	    *cnf_and; 
    OPC_QCMP	    *cnf_comp;
    OPC_QAND	    **cnf_key_list;
    i4		    keyno;
    i4		    temp_keyno;
    OPC_KOR	    *new_dnf_or;
    OPC_KOR	    *next_dnf_or;
    OPC_KAND	    *dnf_and;
    OPC_KAND	    *next_dnf_and;
    OPC_KAND	    *built_dnf_and;
    i4		    compno;
    u_i4	    max_keys;
    i4		    szkparm_byte =
			    (cqual->opc_qsinfo.opc_hiparm / BITSPERBYTE) + 1;
    i4		    lo_keyno,hi_keyno;
    i4		    rmax_ranges = global->ops_cb->ops_alter.ops_rangep_max;
    i4		    high_keyno = cqual->opc_qsinfo.opc_nkeys + 1;

    cqual->opc_dnf_qual = NULL;

    /*	    if (there isn't a key qual or
    **		    this key qual starts with an inequality
    **		)
    */
    if (cqual->opc_cnf_qual == NULL ||
	cqual->opc_qsinfo.opc_inequality == 1 )
    {
	return;
    }
    if (cqual->opc_qsinfo.opc_inequality > 0)
    {
	high_keyno = cqual->opc_qsinfo.opc_inequality;
    }

    /* Allocate and initialize cnf_key_list. The array of OPC_QAND structs,
    ** one for each key attribute.
    */
    cnf_key_list = (OPC_QAND **) opu_Gsmemory_get(global, 
			sizeof(cnf_key_list) * cqual->opc_qsinfo.opc_nkeys);
    MEfill(sizeof(cnf_key_list) * cqual->opc_qsinfo.opc_nkeys,
	(u_char)0, (PTR)cnf_key_list);
    for (cnf_and = cqual->opc_cnf_qual; 
	    cnf_and != NULL; 
	    cnf_and = cnf_and->opc_anext
	)
    {
	/* find out if this conjunct refers to only one key attribute.
	** If it refers to more than one, then go on to the next conjunct
	*/
	keyno = cnf_and->opc_comps[0].opc_keyno;
	for (compno = 1; compno < cnf_and->opc_ncomps; compno += 1)
	{
	    if (cnf_and->opc_comps[compno].opc_keyno != keyno)
	    {
		break;
	    }
	}
	if (compno < cnf_and->opc_ncomps)
	{
	    continue;
	}
	
	/* If we haven't found a conjunct for this key att yet, or
	** the new one is less complicated than the last one, then
	** use it.
	*/
	if (cnf_key_list[keyno] == NULL ||
		cnf_and->opc_ncomps < cnf_key_list[keyno]->opc_ncomps
	    )
	{
            /* b122625, 122831 */
            if (opc_ne_bop(cnf_and->opc_conjunct) == TRUE)
            {
                continue;
            }
	    cnf_key_list[keyno] = cnf_and;
	}
    }

    max_keys = 1;
    for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	if (cnf_key_list[keyno] == NULL)
	{
	    keyno += 1;
	    for ( ; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
	    {
		cnf_key_list[keyno] = NULL;
	    }
	}
	else
	{
	    max_keys = max_keys * cnf_key_list[keyno]->opc_ncomps;
	}
    }
    if (max_keys == 1 && cqual->opc_qsinfo.opc_inequality > 1 )
    {
	/* If there is only a single clause, it should be handled by srange */
	return;
    }

    /* if (the first key att has no conjunct or this will create more
    **		than rangep_max CBF constant (if non-zero)  or (this is a 
    **		hash relation and not all keys are given)
    **	    )
    */
    if (cnf_key_list[0] == NULL || rmax_ranges > 0 && max_keys >= (u_i4)rmax_ranges ||
	    (	cqual->opc_qsinfo.opc_storage_type == DMT_HASH_TYPE && 
		cnf_key_list[cqual->opc_qsinfo.opc_nkeys - 1] == NULL
	    )
	)
    {
	return;
    }

    keyno = 0;
    cnf_and = cnf_key_list[0];
    for (;;)	/* loop for recursion simulation */
    {
	if (cnf_and->opc_curcomp >= 0)
	{
	    /* Set up some variables to keep line length down. */
	    cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];
	    dnf_and = &dnf_or->opc_ands[keyno];

	    /* restore the old range for the current keyno; */
	    if ((cnf_comp->opc_lo_const &&
	         cnf_comp->opc_lo_const->opc_tconst_type == PST_USER) ||
	        (cnf_comp->opc_hi_const &&
	         cnf_comp->opc_hi_const->opc_tconst_type == PST_USER))
	    {
		dnf_and->opc_khi = cnf_and->opc_khi_old;
		dnf_and->opc_klo = cnf_and->opc_klo_old;
	    }
	    else
	    {
		if ((cnf_and->opc_khi_old || cnf_and->opc_klo_old)
		    && keyno < high_keyno)
		{
		    if (cnf_and->opc_khi_old)
		        BTclear(cnf_and->opc_khi_old->opc_const_no,
							    dnf_and->opc_kphi);
		    if (cnf_and->opc_klo_old)
		        BTclear(cnf_and->opc_klo_old->opc_const_no,
							    dnf_and->opc_kplo);
		}
	    }
	}

	cnf_and->opc_curcomp += 1;

	/* If we are finished with the current clause then lets continue with
	** the previous clause.
	*/
	if (cnf_and->opc_curcomp == cnf_and->opc_ncomps)
	{
	    cnf_and->opc_curcomp = -1;
	    cnf_and->opc_nnochange = 0;

	    /* if there aren't any more clauses then we've finished our
	    ** job.
	    */
	    if (keyno == 0)
	    {
		break;
	    }
	    else
	    {
		keyno -= 1;
		cnf_and = cnf_key_list[keyno];
		continue;
	    }
	}

	/* Set up some variables to keep line length down, and to mark
	** what we're doing.
	*/
	cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];	
	dnf_and = &dnf_or->opc_ands[cnf_comp->opc_keyno];

	/* save the old range for the current keyno; */
	if ((cnf_comp->opc_lo_const &&
	     cnf_comp->opc_lo_const->opc_tconst_type == PST_USER) ||
	    (cnf_comp->opc_hi_const &&
	     cnf_comp->opc_hi_const->opc_tconst_type == PST_USER))
	{
	    cnf_and->opc_khi_old = dnf_and->opc_khi;
	    cnf_and->opc_klo_old = dnf_and->opc_klo;
	}
	else
	{
	    if (dnf_and->opc_kphi == NULL || cnf_comp->opc_hi_const == NULL ||
		    BTtest(cnf_comp->opc_hi_const->opc_const_no, 
						    dnf_and->opc_kphi) == FALSE
		)
	    {
		cnf_and->opc_khi_old = cnf_comp->opc_hi_const;
		cnf_and->opc_klo_old = cnf_comp->opc_lo_const;
	    }
	    else
	    {
		cnf_and->opc_khi_old = NULL;
		cnf_and->opc_klo_old = NULL;
	    }
	}

	if ((cnf_comp->opc_lo_const &&
	     cnf_comp->opc_lo_const->opc_tconst_type == PST_USER) ||
	    (cnf_comp->opc_hi_const &&
	     cnf_comp->opc_hi_const->opc_tconst_type == PST_USER))
	{
	    dnf_and->opc_klo = cnf_comp->opc_lo_const;
	    dnf_and->opc_khi = cnf_comp->opc_hi_const;
	}
	else
	{
	    /* allocate the hi and lo bit maps for parameter keying.
	    */
	    if (dnf_and->opc_kphi == NULL)
	    {
		dnf_and->opc_kphi = opu_Gsmemory_get(global, szkparm_byte);
		dnf_and->opc_kplo = opu_Gsmemory_get(global, szkparm_byte);

		MEfill(szkparm_byte, 0, dnf_and->opc_kphi);
		MEfill(szkparm_byte, 0, dnf_and->opc_kplo);
	    }
	    
	    if (cnf_comp->opc_hi_const)
	        BTset(cnf_comp->opc_hi_const->opc_const_no, dnf_and->opc_kphi);
	    if (cnf_comp->opc_hi_const)
	        BTset(cnf_comp->opc_hi_const->opc_const_no, dnf_and->opc_kplo);
	}

	/* Mark this range typs as an equality match only, which will cause
	** this query plan to the recomputed if an inequality is required.
	*/
	dnf_and->opc_krange_type = ADE_3RANGE_NO;

	/* If there aren't any more clauses */
	if (keyno + 1 < cqual->opc_qsinfo.opc_nkeys && 
		cnf_key_list[keyno + 1] != NULL
	    )
	{
	    /* lets continue with the next clause */
	    keyno += 1;
	    cnf_and = cnf_key_list[keyno];
	}
	else
	{
	    /* Look for a range is the same as the one that we just built.
	    ** So, for each range that we currently know about.
	    */
	    for (next_dnf_or = cqual->opc_dnf_qual;
		    next_dnf_or != NULL;
		    next_dnf_or = next_dnf_or->opc_ornext
		)
	    {
		/* for each key in both ranges */
		for (temp_keyno = 0; 
			temp_keyno < cqual->opc_qsinfo.opc_nkeys; 
			temp_keyno += 1
		    )
		{
		    if ((temp_keyno + 1) >= high_keyno)
			continue;

		    next_dnf_and = &next_dnf_or->opc_ands[temp_keyno];
		    built_dnf_and = &dnf_or->opc_ands[temp_keyno];

		    /* if the hi or lo is different then go on to the
		    ** next range.
		    */
		    if (next_dnf_and->opc_khi != built_dnf_and->opc_khi ||
			    next_dnf_and->opc_klo != built_dnf_and->opc_klo
			)
		    {
			break;
		    }

		    /* if the param hi is different then go on to the next
		    ** range.
		    */
		    if (next_dnf_and->opc_kphi == NULL &&
			    built_dnf_and->opc_kphi != NULL &&
			    BTcount(built_dnf_and->opc_kphi, 
					cqual->opc_qsinfo.opc_hiparm) > 0
			)
		    {
			break;
		    }

		    if (next_dnf_and->opc_kphi != NULL &&
			    (built_dnf_and->opc_kphi == NULL ||
				MEcmp(built_dnf_and->opc_kphi, 
						next_dnf_and->opc_kphi,
							    szkparm_byte) != 0
			    )
			)
		    {
			break;
		    }

		    /* if the param lo is different then go on to the next
		    ** range.
		    */
		    if (next_dnf_and->opc_kplo == NULL &&
			    built_dnf_and->opc_kplo != NULL &&
			    BTcount(built_dnf_and->opc_kplo, 
					cqual->opc_qsinfo.opc_hiparm) > 0
			)
		    {
			break;
		    }

		    if (next_dnf_and->opc_kplo != NULL &&
			    (built_dnf_and->opc_kplo == NULL ||
				MEcmp(built_dnf_and->opc_kplo, 
						next_dnf_and->opc_kplo,
							    szkparm_byte) != 0
			    )
			)
		    {
			break;
		    }
		}

		/* the two ranges just compared must be the same, so don't 
		** add the one just built to the list of ranges.
		*/
		if (temp_keyno == cqual->opc_qsinfo.opc_nkeys)
		{
		    break;
		}
	    }

	    /* if we didn't find a range that was the same as the one
	    ** that we just built, then copy it and add it to our ranges.
	    */
	    if (next_dnf_or == NULL)
	    {
		opc_orcopy(global, cqual, dnf_or, &new_dnf_or, 
							&lo_keyno, &hi_keyno);

		if (cqual->opc_dnf_qual == NULL)
		{
		    new_dnf_or->opc_orprev = new_dnf_or->opc_ornext = NULL;
		    cqual->opc_dnf_qual = cqual->opc_end_dnf = new_dnf_or;
		}
		else
		{
		    new_dnf_or->opc_ornext = cqual->opc_dnf_qual;
		    cqual->opc_dnf_qual->opc_orprev = new_dnf_or;
		    cqual->opc_dnf_qual = new_dnf_or;
		}
	    }
	}
    }
}

/*{
** Name: OPC_RANGE  	- Allocate and initialize keying ranges
**
** Description:
**      This routine converts a conjunctive normal form (cnf) qualification 
**      into a disjunctive (dnf) qual in a form suitable for keying. Once 
**      in disjunctive form, the ranges to search a relation become obvious. 
**      One of the problems that must be over come is eliminating the 
**      explosively redundant terms that can occur during most cnf to dnf 
**      translations. Another problem is doing this conversion in polynomial 
**      time. Actually, these two problems are related, since if you can solve 
**      one, the other will probably go away.  
**        
**      Before we go on, I'd like to point out that this isn't a symmetric 
**      conversion. In other words, if you have a CNF qual and use this 
**      routine to convert it into a DNF qual and then have it converted 
**      back into a CNF qual, the resulting CNF qual may be less restrictive 
**      than the original CNF qual. This is because of the unique requirements 
**      for keying into relations. For example, if a second key att is given 
**      a value in a DNF clause but not the first key att, then the clause 
**      will be dropped because it isn't useful for keying.  
**        
**      With all that said, lets go on to discuss how the conversion happens.
**	The first thing to do is to detect qualifications that are too
**	complex for this routine to handle. Quals that are too complex
**	are simplified until they can be handled (see opc_complex_range()
**	for more details). The effect of this simplification is to
**	gradually enlarge the keying range that would be generated.
**
**	The next step is to eliminate redundant comparisons within the
**	conjuncts of the CNF form of the qualification. For example, in
**	the conjunct (x > 7 or x > 10), the (x > 10) is redundant and
**	can be eliminated. This can speed up the conversion process from
**	CNF to DMF.
**
**      The next step it to be able to build disjunctive clauses from a CNF 
**      qual. This is done by taking one comparison from each CNF clause 
**      and anding them together to form your DNF clause. The next step is 
**      to take the newly formed DNF clause and merge it together with 
**      the other DNF clauses that have been formed. The purpose of the merging 
**      is to insure that no two DNF clauses overlap an a range of values.  
**      For example, the two DNF clauses (x > 5 and x < 10) or  
**	(x > 7 and x < 15) would need to be merged together into
**      (x > 5 and < 15). On the other hand, (x > 5 and x < 10) or 
**      (x > 20 and x < 25) would stand without merging. Also note that
**      (x > 5 and x <= 10) or (x >= 10 and x < 15) should be merged into 
**      (x > 5 and x < 15). Once we have merged the DNF clause into the
**	DNF qual, we continue the process with the next DNF clause.
**        
**      The $64 question is: which should be the next DNF clause? We could 
**      Generate all possible DNF clauses, but that would place our algorithm 
**      into the NP class. We need some way of limiting the search space of 
**      DNF clauses. This is done by two exclusion rules. First, as we are 
**      building up a DNF clause by adding comparision from the CNF qual, we 
**      ask ourselves if we have a DNF clause that couldn't ever refer to 
**      data. For example (x > 5 and x < 4) will never refer to data. The 
**      moment that we find that we have such a DNF clause, we drop it on the 
**      spot and go on the the next. 
**        
**      The second exclusion rule eliminates duplicate DNF clauses. I can't 
**      think of a good way to explain it, so I'll give an example and I think 
**      that it will be clear. Given the CNF qual: 
**		(x > 5) and 
**		(x > 4 or x > 3) and 
**		(x < 10)
**      The first DNF clause would be generated using x > 5, > 4, and < 10, 
**      however we would note, when processing the 2nd CNF clause that the 
**      x > 4 did no useful work. The second iteration would start out with 
**      x > 5, and go on the x > 3 and notice that this was the second time 
**      that the 2nd clause wasn't adding any useful information. Because 
**      of this, no DNF clause will be generated and we will continue with the 
**      next iteration, which will find that there are no more DNF clauses. 
**        
**      These two rules are so effective that nearly the only DNF clauses
**	that get by the rules are essential to the keying of the relation. In 
**	other words, because of these two rules DNF clause merging almost never 
**      needs to happen unless the user specified their query in a redundant 
**      fashion. 
**        
**      As a final note, I'd like to point out that I've gone to considerable 
**      work to simplify this code. However, the problem that this routine is 
**	solving is an inherently complicated one. CARE SHOULD BE TAKEN BEFORE 
**	ANY CHANGES ARE MADE TO THIS ROUTINE. Please make sure that you 
**	understand it completely, since it can be deceptivly complicated. 
[@comment_line@]...
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC. This is used
**	mostly as a parameter to all OPC routines.
**  cqual -
**	This is the struct that holds all keying info. It is expected that
**	all fields, except for opc_dnf_qual, are correctly filled in. This
**	is the primary source of information for the translation from a
**	conjunctive (cnf) to a disjunctive (dnf) tree.
**  dnf_or -
**	This is a DNF clause that has been allocated and initialized with
**	default values (ie. values that, if left alone, would result in a
**	full scan of the relation). It is used as a scratch area for
**	computation.
**
** Outputs:
**  cqual->opc_dnf_qual -
**	This will point to one or more OPC_KOR structs.
**	The values in each OPC_KOR struct will be set to indicate an exact
**	match or inequality range to search.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none except for those raised by opx_error and opx_verror.
**
** Side Effects:
**	    none
**
** History:
**      1-sept-86 (eric)
**          written
**      1-aug-87 (eric)
**          rewitten to be non-recursive and, hence, faster, and to fix some
**          bugs.
**      18-aug-91 (eric)
**          Fixed bug 37437, which appeared to be an infinite loop. In fact,
**          it was a complex qualification which was causing opc_range to
**          take an incredibly long time to complete. The two exclusion
**          rules were not sufficient to handle a qual of such complexity.
**          To fix this, two things were done: We eliminate redundant
**          comparisons within a conjunct of the CNF qual. This reduces the
**          size of the task at hand somewhat. Second, we detect quals
**          that are too complex for us to handle and simplify them until
**          we can handle them (see opc_complex_range() for more details).
**	14-nov-91 (anitap)
**	    Changed the cast of opu_Gsmemory_get() which su3 compiler was 
**	    complaining about.
**	11-dec-91 (anitap)
**          Fixed bug 41358, which was regression from 6.4 Beta. The fix
**          is to correctly assign the pointers for opc_hi_const and
**          opc_lo_const for the merged qualification. The low and high
**          constant numbers were being assigned to the merged
**          qualification instead of the pointers.
**	01-jan-92 (anitap)
**          Fixed bug 42033. The fix is to check if opc_complex_range() has
**          eliminated all the conjuncts. If so, then we return to the calling
**          routine opc_key() so that there is no access violation in
**          opc_range().
[@history_template@]...
*/
VOID
opc_range(
		OPS_STATE   *global,
		OPC_QUAL    *cqual,
		OPC_KOR	    *dnf_or)
{
    OPC_KAND	    *dnf_and;
    OPC_QCMP	    *cnf_comp;
    OPC_QAND	    *cnf_and;
    OPC_QCMP	    **keyno_cnf_comp;
    OPC_QCMP	    *merged_cnf_comp;
    i4		    sz_keyno_cnf_comp;
    i4		    keyno;
    i4		    compno;
    i4		    lo_keyno, hi_keyno;

    /* Detect and simplify overly complex ranges */
    opc_complex_range(global, cqual);

    /* If there are no conjuncts, return to opc_key() */
    if (cqual->opc_cnf_qual == NULL)
    {
        return;
    }

    /* Eliminate redundant comparisons with a conjunct of the CNF qual */
    sz_keyno_cnf_comp = sizeof (OPC_QCMP *) * cqual->opc_qsinfo.opc_nkeys;
    keyno_cnf_comp = (OPC_QCMP **) opu_Gsmemory_get(global, sz_keyno_cnf_comp);

    for (cnf_and = cqual->opc_cnf_qual; 
	    cnf_and != NULL;
	    cnf_and = cnf_and->opc_anext
	)
    {
	MEfill(sz_keyno_cnf_comp, (u_char)0, (PTR) keyno_cnf_comp);
	for (compno = 0; compno < cnf_and->opc_ncomps; compno += 1)
	{
	    cnf_comp = &cnf_and->opc_comps[compno];
	    keyno = cnf_comp->opc_keyno;

	    if (keyno_cnf_comp[keyno] == NULL)
	    {
		keyno_cnf_comp[keyno] = cnf_comp;
	    }
	    else
	    {
		merged_cnf_comp = keyno_cnf_comp[keyno];

		if ((cnf_comp->opc_lo_const == NULL ||
			merged_cnf_comp->opc_hi_const == NULL ||
			cnf_comp->opc_lo_const->opc_const_no <=
			    merged_cnf_comp->opc_hi_const->opc_const_no
		     ) &&
		     (cnf_comp->opc_hi_const == NULL ||
			merged_cnf_comp->opc_lo_const == NULL ||
			cnf_comp->opc_hi_const->opc_const_no >=
			    merged_cnf_comp->opc_lo_const->opc_const_no
		     )
		    )
		{
		    cnf_comp->opc_keyno = -1;

		    if (cnf_comp->opc_lo_const == NULL ||
			 (merged_cnf_comp->opc_lo_const != NULL &&
			    cnf_comp->opc_lo_const->opc_const_no <
				merged_cnf_comp->opc_lo_const->opc_const_no
			 )
			)
		    {
			merged_cnf_comp->opc_lo_const =
				    cnf_comp->opc_lo_const;
		    }

		    if (cnf_comp->opc_hi_const == NULL ||
			 (merged_cnf_comp->opc_hi_const != NULL &&
			    cnf_comp->opc_hi_const->opc_const_no >
				merged_cnf_comp->opc_hi_const->opc_const_no 
			 )
			)
		    {
			merged_cnf_comp->opc_hi_const =
				    cnf_comp->opc_hi_const;
		    }
		}
	    }
	}
    }

    cnf_and = cqual->opc_cnf_qual;

    for (;;)
    {
	/* Lets go on to the next comparison in the current clause, but first
	** we need to remove the current comparison from dnf_or.
	*/
	if (cnf_and->opc_curcomp >= 0)
	{
	    /* Set up some variables to keep line length down. */
	    cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];
	    dnf_and = &dnf_or->opc_ands[cnf_comp->opc_keyno];

	    /* restore the old range for the current keyno; */
	    dnf_and->opc_khi = cnf_and->opc_khi_old;
	    dnf_and->opc_klo = cnf_and->opc_klo_old;
	}

	/* Now lets go on the next comparison */
	do 
	{
	    cnf_and->opc_curcomp += 1;
	} while (cnf_and->opc_curcomp < cnf_and->opc_ncomps &&
		    cnf_and->opc_comps[cnf_and->opc_curcomp].opc_keyno == -1
		);
		    

	/* If we are finished with the current clause then lets continue with
	** the previous clause.
	*/
	if (cnf_and->opc_curcomp == cnf_and->opc_ncomps)
	{
	    cnf_and->opc_curcomp = -1;
	    cnf_and->opc_nnochange = 0;
	    cnf_and = cnf_and->opc_aprev;

	    /* if there aren't any more clauses then we've finished our
	    ** job.
	    */
	    if (cnf_and == NULL)
	    {
		break;
	    }

	    continue;
	}

	/* Set up some variables to keep line length down, and to mark
	** what we're doing.
	*/
	cnf_comp = &cnf_and->opc_comps[cnf_and->opc_curcomp];	
	dnf_and = &dnf_or->opc_ands[cnf_comp->opc_keyno];

	/* save the old range for the current keyno; */
	cnf_and->opc_khi_old = dnf_and->opc_khi;
	cnf_and->opc_klo_old = dnf_and->opc_klo;

	/* Add this operation the dnf_or; */
	if (dnf_and->opc_klo == NULL ||
		(cnf_comp->opc_lo_const != NULL &&
		    cnf_comp->opc_lo_const->opc_const_no > 
					dnf_and->opc_klo->opc_const_no
		)
	    )
	{
	    dnf_and->opc_klo = cnf_comp->opc_lo_const;
	}

	if (dnf_and->opc_khi == NULL ||
		(cnf_comp->opc_hi_const != NULL &&
		    cnf_comp->opc_hi_const->opc_const_no <
					dnf_and->opc_khi->opc_const_no
		)
	    )
	{
	    dnf_and->opc_khi = cnf_comp->opc_hi_const;
	}

	/* If the current range has been restricted to the point that
	** it can no longer refer to data, then lets go on with the
	** next comparison.
	*/
	if (dnf_and->opc_khi != NULL && dnf_and->opc_klo != NULL &&
		dnf_and->opc_khi->opc_const_no < 
					    dnf_and->opc_klo->opc_const_no
	    )
	{
	    continue;
	}

	/* If the current comparison didn't change the current range
	** any, and we have let the range through unchanged once already
	** then lets go on to the next comparison.
	*/
	if (cnf_and->opc_khi_old == dnf_and->opc_khi &&
		cnf_and->opc_klo_old == dnf_and->opc_klo
	    )
	{
	    cnf_and->opc_nnochange += 1;
	    if (cnf_and->opc_nnochange > 1)
	    {
		continue;
	    }
	}

	/* If there aren't any more clauses */
	if (cnf_and->opc_anext == NULL)
	{
	    /* fuse dnf_or into the dnf qual; */
	    opc_kfuse(global, cqual, dnf_or, &lo_keyno, &hi_keyno);

	}
	else
	{
	    /* Otherwise, lets continue with the next clause */
	    cnf_and = cnf_and->opc_anext;
	}
    }
}
/*{
** Name: OPC_KFUSE	- Combine (fuse?) overlapping OPC_KOR structs.
**
** Description:
**      This routine takes a DNF clause and merges it into a DNF qualification. 
**      The purpose of the merging is to insure that no two (or more) clauses 
**      in the qual overlap in the value ranges that they specify. 
**      For example, the two DNF clauses (x > 5 and x < 10) or  
**	(x > 7 and x < 15) would need to be merged together into
**      (x > 5 and < 15). On the other hand, (x > 5 and x < 10) or 
**      (x > 20 and x < 25) would stand without merging. Also note that
**      (x > 5 and x <= 10) or (x >= 10 and x < 15) should be merged into 
**      (x > 5 and x < 15). Once we have merged the DNF clause into the
**	Any clause that doesn't overlap will be placed into the qual without
**	change. Any clause the does overlap will be massaged until the
**	overlap doesn't exist. This massaging will be done by combining
**	clauses and/or weakening the qualification. Weakening the qual
**	is OK since that means we will possibly look at more rather than
**	less tuples.
**
** Inputs:
**  global -
**	This is the query wide state variable for OPF/OPC. This is used
**	mostly as a parameter to all OPC routines.
**  cqual -
**	This is the struct that holds all keying info. It is expected that
**	all fields, including the opc_dnf_qual, are correctly filled in. This
**	is the primary source of information for the translation from a
**	conjunctive (cnf) to a disjunctive (dnf) tree.
**  dnf_or -
**	This is the disjunctive clause that is to be merged into the dnf qual.
**  lo_keyno -
**  hi_keyno -
**	Pointers to uninitialized nats
**
** Outputs:
**  cqual->opc_dnf_qual
**	Dnf_or will be merged into this qual.
**  lo_keyno - 
**	The highest key number with a low key value specified in 'dnf_or'.
**	Will be -1 if no key info is given for the low key.
**  hi_keyno - 
**	Like 'lo_keyno' except for the high key.
**
**	Returns:
**	    none
**	Exceptions:
**	    none, except for those raised by opx_error and opx_verror.
**
** Side Effects:
**	    none
**
** History:
**      20-aug-86 (ERIC)
**          written
**	26-nov-90 (stec)
**	    Added initialization of print buffer for trace code.
**	28-dec-90 (stec)
**	    Changed code to reflect fact that print buffer is in OPC_STATE
**	    struct.
**      09-mar-92 (anitap)
**          Changed code to reflect the fact that it should be a FULL SCAN when
**          all the keys are not specified in a qual for a HASH key. Fix for
**          bug 35506.
**      15-nov-2000 (devjo01)
**          Back out anitap's fix for 35506, it was insufficient,
**          and fix for 103164 made it redundant.
[@history_template@]...
*/
static VOID
opc_kfuse(
	OPS_STATE   *global,
	OPC_QUAL    *cqual,
	OPC_KOR	    *dnf_or,
	i4	    *lo_keyno,
	i4	    *hi_keyno )
{
    OPC_KOR	*pdnf_or;	    /* OPC_KOR to be put into the list */
    OPC_KOR	*sdnf_or;	    /* The next sorted OPC_KOR */
    OPC_KOR	*new_sdnf_or;
    OPC_KAND	*pkand;
    OPC_KAND	*skand;
    i4		kandi;
    i4		lcomp;
    i4		ucomp;
    i4		rcomp;

#ifdef OPT_F026_FULL_OPKEY
	if (opt_strace(global->ops_cb, OPT_F026_FULL_OPKEY) == TRUE)
	{
	    char	temp[OPT_PBLEN + 1];
	    bool	init = 0;

	    if (global->ops_cstate.opc_prbuf == NULL)
	    {
		global->ops_cstate.opc_prbuf = temp;
		init++;
	    }

	    opt_opkor(global, cqual, dnf_or);
	    
	    if (init)
	    {
		global->ops_cstate.opc_prbuf = NULL;
	    }
	}
#endif

    *lo_keyno = *hi_keyno = -1;

    /* First off, lets see if there is any point of considering this
    ** kor. If the max (less_equal) is less that the min (greater_equal)
    ** then there isn't any point in considering this range since it 
    ** wouldn't return any tuples anyway.
    */
    if (opc_rcompare(global, cqual, dnf_or, OPC_UPPER_BOUND,
						dnf_or, OPC_LOWER_BOUND) < 0
	)
    {
	return;
    }

    if (cqual->opc_dnf_qual == NULL)
    {
	/* make a copy of dnf_or and point cqual->opc_dnf_qual at it; */
	opc_orcopy(global, cqual, dnf_or, &pdnf_or, lo_keyno, hi_keyno);
	pdnf_or->opc_orprev = pdnf_or->opc_ornext = NULL;
	cqual->opc_dnf_qual = cqual->opc_end_dnf = pdnf_or;
	return;
    }

    /* for (each kor on the cqual->opc_dnf_qual list 
    ** (which happens to be sorted))
    */
    if (cqual->opc_qsinfo.opc_nparms >= 1)
    {
	pdnf_or = dnf_or;
	sdnf_or = cqual->opc_dnf_qual;

	/* combine sdnf_or and pdnf_or; */
	lcomp = opc_rcompare(global, cqual, sdnf_or,
				    OPC_LOWER_BOUND, pdnf_or, OPC_LOWER_BOUND);
	ucomp = opc_rcompare(global, cqual, sdnf_or,
				    OPC_UPPER_BOUND, pdnf_or, OPC_UPPER_BOUND);

	/* for (each kand) */
	for (kandi = 0; kandi < cqual->opc_qsinfo.opc_nkeys; kandi += 1)
	{
	    if (lcomp > 0)
	    {
		sdnf_or->opc_ands[kandi].opc_klo = pdnf_or->opc_ands[kandi].opc_klo;
	    }

	    if (ucomp < 0)
	    {
		sdnf_or->opc_ands[kandi].opc_khi = pdnf_or->opc_ands[kandi].opc_khi;
	    }
	}
    }
    else
    {

	pdnf_or = dnf_or;
	sdnf_or = cqual->opc_dnf_qual;
	while (sdnf_or != NULL)
	{
	    /* If the lower of put is <= the upper of sort */
	    rcomp = opc_rcompare(global, cqual, pdnf_or, OPC_LOWER_BOUND,
						    sdnf_or, OPC_UPPER_BOUND);
	    if (rcomp <= 0)
	    {
		/* if (the upper of put is < lower of sort) */
		ucomp = opc_rcompare(global, cqual, pdnf_or, OPC_UPPER_BOUND,
							sdnf_or, OPC_LOWER_BOUND);
		if (ucomp < 0)
		{
		    /* copy put if needed and put before sort; */
		    if (pdnf_or == dnf_or)
		    {
			/* make a copy of dnf_or and point pdnf_or at it; */
			opc_orcopy(global, cqual, dnf_or, 
					    &pdnf_or, lo_keyno, hi_keyno);
		    }

		    /* Now lets put it on the cqual->opc_dnf_qual list */
		    pdnf_or->opc_ornext = sdnf_or;
		    pdnf_or->opc_orprev = sdnf_or->opc_orprev;
		    sdnf_or->opc_orprev = pdnf_or;
		    if (pdnf_or->opc_orprev != NULL)
		    {
			pdnf_or->opc_orprev->opc_ornext = pdnf_or;
		    }

		    if (cqual->opc_dnf_qual == NULL ||
			    cqual->opc_dnf_qual == sdnf_or
			)
		    {
			cqual->opc_dnf_qual = pdnf_or;
		    }
		    pdnf_or = NULL;
		    break;
		}
		else
		{
		   /* combine the put and sort ranges into 
		   ** the sort range.
		   */
		   for (kandi = 0; 
			    kandi < cqual->opc_qsinfo.opc_nkeys; 
			    kandi += 1
			)
		   {
		       skand = &sdnf_or->opc_ands[kandi];
		       pkand = &pdnf_or->opc_ands[kandi];
		       if (pkand->opc_khi == NULL ||
				(skand->opc_khi != NULL &&
				    pkand->opc_khi->opc_const_no > 
					skand->opc_khi->opc_const_no
				)
			  )
		       {
			  skand->opc_khi = pkand->opc_khi;
		       }

		       if (pkand->opc_klo == NULL ||
				(skand->opc_klo != NULL &&
				    pkand->opc_klo->opc_const_no <
					skand->opc_klo->opc_const_no
				)
			  )
		       {
			  skand->opc_klo = pkand->opc_klo;
		       }
		    }

		    /* Take sort off of its queue, point put at it, and set up
		    ** for the next round. 
		    */
		    new_sdnf_or = sdnf_or->opc_ornext;
		    pdnf_or = sdnf_or;
		    opc_remkor(global, cqual, sdnf_or);
		    sdnf_or = new_sdnf_or;
		    continue;
		}
	    }

	    sdnf_or = sdnf_or->opc_ornext;
	}

	/* If put hasn't been placed anywhere else, then we need to put it
	** at the end of the list.
	*/
	if (pdnf_or != NULL)
	{
	    if (pdnf_or == dnf_or)
	    {
		/* make a copy of dnf_or and point pdnf_or at it; */
		opc_orcopy(global, cqual, dnf_or, &pdnf_or, lo_keyno, hi_keyno);
	    }

	    /* Now lets put it on the cqual->opc_dnf_qual list */
	    if (cqual->opc_dnf_qual == NULL)
	    {
		pdnf_or->opc_orprev = pdnf_or->opc_ornext = NULL;
		cqual->opc_dnf_qual = cqual->opc_end_dnf = pdnf_or;
	    }
	    else
	    {
		pdnf_or->opc_ornext = NULL;
		pdnf_or->opc_orprev = cqual->opc_end_dnf;
		cqual->opc_end_dnf->opc_ornext = pdnf_or;
		cqual->opc_end_dnf = pdnf_or;
	    }
	}
    }
}

/*{
** Name: OPC_NE_BOP	- Search the sub-tree for a non-exact BOP.
**
** Description:
**      This search the tree from the supplied node down, looking for
**      a non-exact BOP.
**
** Inputs:
**      nodep		- PST_QNODE pointer.
**
** Outputs:
**	Returns:
**	    TRUE	if non-exact BOP found under nodep.
**          FALSE	if a non-exact BOP is not found under nodep.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**      17-Nov-2009 (hanal04) Bug 122831
**          Written
*/
static bool
opc_ne_bop(
	PST_QNODE	*nodep)
{
    for(;nodep; nodep = nodep->pst_right)
    {   /* iterate down right side and recurse down left */
        switch (nodep->pst_sym.pst_type)
        {
            case PST_BOP:
                switch (nodep->pst_sym.pst_value.pst_s_op.pst_opno)
                {
                    case ADI_LIKE_OP:
                    case ADI_GT_OP:
                    case ADI_GE_OP:
                    case ADI_LT_OP:
                    case ADI_LE_OP:
                        return(TRUE); /* Non exact BOP found */
    
                    default:
                        break;
                }

            default:
                break;
        }

        if (nodep->pst_left && opc_ne_bop(nodep->pst_left))
            return(TRUE); /* Non-exact BOP found in left sub-tree */
    }
    return(FALSE);
}

/*{
** Name: OPC_ORCOPY	- Make a copy of an OPC_KOR struct
**
** Description:
**      This makes a copy of an OPC_KOR struct and return 
**      [@comment_text@] 
[@comment_line@]...
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
**      30-OCT-86 (eric)
**          written
[@history_template@]...
*/
static VOID
opc_orcopy(
	OPS_STATE   *global,
	OPC_QUAL    *cqual,
	OPC_KOR	    *src,
	OPC_KOR	    **pdest,
	i4	    *lo_keyno,
	i4	    *hi_keyno )
{
    i4		and_size;
    OPC_KOR	*dest;
    i4		keyno;
    i4		szkparm_byte = 
			    (cqual->opc_qsinfo.opc_hiparm / BITSPERBYTE) + 1;

    /* make a copy of kor and point pkor at it; */
    and_size = sizeof (OPC_KAND) * cqual->opc_qsinfo.opc_nkeys;

    dest = (OPC_KOR *) opu_Gsmemory_get(global, sizeof (OPC_KOR));
    STRUCT_ASSIGN_MACRO(*src, *dest);

    dest->opc_ands = (OPC_KAND *) opu_Gsmemory_get(global, and_size);
    MEcopy((PTR)src->opc_ands, and_size, (PTR)dest->opc_ands);

    /* set opc_kphi correctly and clear out any useless 
    ** upper bound keying info. 
    */
    for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	/* if there are repeat query parameter or DBP local vars in this
	** key range then copy them
	*/
	if (src->opc_ands[keyno].opc_kphi != NULL && 
		BTcount(src->opc_ands[keyno].opc_kphi, 
			    cqual->opc_qsinfo.opc_hiparm) != 0
	    )
	{
	    dest->opc_ands[keyno].opc_kphi = 
				    opu_Gsmemory_get(global, szkparm_byte);
	    MEcopy(src->opc_ands[keyno].opc_kphi, szkparm_byte,
					dest->opc_ands[keyno].opc_kphi);
	}
	else
	{
	    dest->opc_ands[keyno].opc_kphi = NULL;
	}

	if (dest->opc_ands[keyno].opc_khi == NULL &&
		dest->opc_ands[keyno].opc_kphi == NULL
	    )
	{
	    break;
	}
    }
    if (cqual->opc_qsinfo.opc_storage_type == DMT_HASH_TYPE && 
	    keyno < cqual->opc_qsinfo.opc_nkeys
	)
    {
	/* if this is a hash and not all the key values were given, then
	** force all the the key values to be cleared out.
	*/
	keyno = 0;
    }

    /* set hi_keyno to the highest key number with data */
    *hi_keyno = keyno - 1;

    /* Null out the key atts that have useless data */
    for ( ; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	dest->opc_ands[keyno].opc_khi = NULL;
	dest->opc_ands[keyno].opc_kphi = NULL;
    }

    /* clear out any useless lower bound keying info. */
    for (keyno = 0; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	/* if there are repeat query parameter or DBP local vars in this
	** key range then copy them
	*/
	if (src->opc_ands[keyno].opc_kplo != NULL && 
		BTcount(src->opc_ands[keyno].opc_kplo, 
			    cqual->opc_qsinfo.opc_hiparm) != 0
	    )
	{
	    dest->opc_ands[keyno].opc_kplo = 
					opu_Gsmemory_get(global, szkparm_byte);
	    MEcopy(src->opc_ands[keyno].opc_kplo, szkparm_byte,
					    dest->opc_ands[keyno].opc_kplo);
	}
	else
	{
	    dest->opc_ands[keyno].opc_kplo = NULL;
	}

	if (dest->opc_ands[keyno].opc_klo == NULL &&
		dest->opc_ands[keyno].opc_kplo == NULL
	    )
	{
	    break;
	}
    }
    if (cqual->opc_qsinfo.opc_storage_type == DMT_HASH_TYPE && 
	    keyno < cqual->opc_qsinfo.opc_nkeys
	)
    {
	/* if this is a hash and not all the key values were given, then
	** force all the the key values to be cleared out.
	*/
	keyno = 0;
    }

    /* set hi_keyno to the highest key number with data */
    *lo_keyno = keyno - 1;

    /* Null out the key atts that have useless data */
    for ( ; keyno < cqual->opc_qsinfo.opc_nkeys; keyno += 1)
    {
	dest->opc_ands[keyno].opc_klo = NULL;
	dest->opc_ands[keyno].opc_kplo = NULL;
    }

    *pdest = dest;
}

/*{
** Name: OPC_REMKOR	- Remove a OPC_KOR struct from its queue
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
**      19-sept-86 (eric)
**          written
[@history_template@]...
*/
static VOID
opc_remkor(
	OPS_STATE   *global,
	OPC_QUAL   *cqual,
	OPC_KOR	    *dnf_or )
{

    /* if dnf_or is the first element on the cqual->opc_dnf_qual list then point
    ** it to the new beginning.
    */
    if (cqual->opc_dnf_qual == dnf_or)
    {
	cqual->opc_dnf_qual = dnf_or->opc_ornext;
    }
    if (cqual->opc_end_dnf == dnf_or)
    {
	cqual->opc_end_dnf = dnf_or->opc_orprev;
    }

    /* Finally, lets take dnf_or off the queue */
    if (dnf_or->opc_ornext != NULL)
    {
	dnf_or->opc_ornext->opc_orprev = dnf_or->opc_orprev;
    }
    if (dnf_or->opc_orprev != NULL)
    {
	dnf_or->opc_orprev->opc_ornext = dnf_or->opc_ornext;
    }
    dnf_or->opc_ornext = dnf_or->opc_orprev = NULL;
}

/*{
** Name: OPC_RCOMPARE	- Compare two ranges and return which is higher.
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
**      21-aug-86 (eric)
**          written
[@history_template@]...
*/
static i4
opc_rcompare(
	OPS_STATE   *global,
	OPC_QUAL    *cqual,
	OPC_KOR	    *ldnf_or,
	i4	    lside,
	OPC_KOR	    *rdnf_or,
	i4	    rside )
{
    i4		ret;
    i4		kandi;
    OPC_QCONST	*lkdata;
    OPC_QCONST	*rkdata;
    i4		lknull = FALSE;
    i4		rknull = FALSE;

    for (kandi = 0; kandi < cqual->opc_qsinfo.opc_nkeys; kandi += 1)
    {
	/* First, get the data for the left side for this key number */
	if (lside == OPC_UPPER_BOUND)
	{
	    lkdata = ldnf_or->opc_ands[kandi].opc_khi;
	}
	else
	{
	    lkdata = ldnf_or->opc_ands[kandi].opc_klo;
	}
	if (lkdata == NULL)
	{
	    lknull = TRUE;
	}
	else if (lknull == TRUE)
	{
	    lkdata = NULL;
	}

	/* Now, get the data for the right side for this key number */
	if (rside == OPC_UPPER_BOUND)
	{
	    rkdata = rdnf_or->opc_ands[kandi].opc_khi;
	}
	else
	{
	    rkdata = rdnf_or->opc_ands[kandi].opc_klo;
	}
	if (rkdata == NULL)
	{
	    rknull = TRUE;
	}
	else if (rknull == TRUE)
	{
	    rkdata = NULL;
	}

	/* Finally, let's compare the two pieces of data */
	if (lkdata == NULL)
	{
	    if (rkdata == NULL)
	    {
		/* Both sides are NULL, so lets give up and return, since
		** looking at more key atts won't change this.
		*/
		ret = rside - lside;
		break;
	    }
	    else
	    {
		ret = -lside;
	    }
	}
	else if (rkdata == NULL)
	{
	    ret = rside;
	}
	else
	{
	    if (lkdata->opc_const_no == rkdata->opc_const_no)
	    {
		ret = 0;
	    }
	    else if (lkdata->opc_const_no > rkdata->opc_const_no)
	    {
		ret = 1;
	    }
	    else
	    {
		ret = -1;
	    }
	}

	/* If one side was bigger than the other then there isn't any need
	** to compare further.
	*/
	if (ret != 0)
	{
	    break;
	}
    }

    return(ret);
}

/*{
** Name: OPC_COMPLEX_RANGE  - Detect and simplify overly complex keying ranges
**
** Description:
**      This routine estimates the complexity of building a key from  
**      the given qualification. If it is determined that the complexity 
**      is too high, then the qualification will be simplified until 
**      is within the abilities of opc_range(). 
**       
**      The two interesting questions here are: how do we estimate the 
**      key building complexity, and how do we simplify the qualification? 
**	
**	Estimating the key building complexity, as with most estimations
**	is a hard thing. A bad estimate with either allow a qualification
**	that is too hard for opc_range, or will simplfy one that opc_range
**	could handle. The things that make it harder for opc_range are:
**
**	    - A large number of comparisons ORed together
**
**	    - A large number of key attributes
**
**	There are other factors that make it hard for opc_range(), but these
**	are the major ones. It should be obvious why more comparisons
**	make it hard for opc_range. Given the same number of comparisons, more
**	key atts makes it hard for opc_range because they reduce the
**	effectiveness of the elimination rules used by opc_range.
**	Consequently, we estimate the complexity as follows:
**
**	        (#_of_comparisons that are ORed together)
**	    --------------------------------------------------
**	    (the number of keys that are specified in the qual)
**
**	This number is raised to the Nth power, where N is the number
**	of keys in the qual. Empirically, quals that get a complexity
**	of 300000 or greater are too hard for opc_range. As opc_range
**	gets smarter, this number should be raised.
**
**	Several other estimation methods were considered (about 6 others)
**	but this one was judged to be the best.
**
**	The qualifications are simplified by reducing the number of key
**	atts that we attempt to key into. We do this by eliminating
**	all conjuncts that refer to the max key att.
**
** Inputs:
**	global -
**	    The query-wide state variable
**	cqual -
**	    The keying qualification to the analysed and simplified.
**
** Outputs:
**	cqual -
**	    The CNF qual may have conjuncts removed from it to make it
**	    simple enough for opc_range() to be able to handle it.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-aug-91 (eric)
**          written
**	24-Apr-2007 (kschendel) b122118
**	    CPU's are a little faster now, 16 years later.  Another zero
**	    on the complexity limit seems perfectly reasonable (with a
**	    few empirical tests confirming same).
[@history_template@]...
*/
static VOID
opc_complex_range(
	OPS_STATE   *global,
	OPC_QUAL    *cqual )
{
    f8		complexity;	/* estimated complexity of the qual */
    i4		total_ncomps;	/* Total number of comparisons in the qual */
    i4		nusefulkeys;	/* Number of keys that could possibly be used */
    OPC_QAND	*qand;
    OPC_QCMP	*comp;
    OPC_KCONST	*kconst;
    i4		keyno;
    i4		compno;
    i4		nkeys;	    /* number of key atts that we will allow in qual */

    nkeys = cqual->opc_qsinfo.opc_nkeys;
    for (;;)
    {
	/* compute the complexity of calculating the key range */

	if (nkeys <= 1)
	{
	    break;
	}

	/* count the number of useful keys and values */
	nusefulkeys = 0;
	for (keyno = 0; 
		keyno < nkeys;
		keyno += 1
	    )
	{
	    kconst = &cqual->opc_kconsts[keyno];
	    if (kconst->opc_qconsts != NULL ||
		    kconst->opc_qparms != NULL ||
		    kconst->opc_dbpvars != NULL ||
		    kconst->opc_nuops > 0
		)
	    {
		nusefulkeys += 1;
	    }
	}

	/* first, compute the number of possible comparison combinations */
	total_ncomps = 0;
	for (qand = cqual->opc_cnf_qual; qand != NULL; qand = qand->opc_anext)
	{
	    total_ncomps += qand->opc_ncomps;
	}

	complexity = MHpow((f8) ((f8)total_ncomps / (f8)nusefulkeys),
			    (f8) nusefulkeys);

	if (complexity < 300000)
	{
	    break;
	}
	else
	{
	    /* reduce the complexity of the key qual */
	    if (nusefulkeys < nkeys)
	    {
		for (keyno = 0; 
			keyno < nkeys;
			keyno += 1
		    )
		{
		    kconst = &cqual->opc_kconsts[keyno];
		    if (kconst->opc_qconsts == NULL &&
			    kconst->opc_qparms == NULL &&
			    kconst->opc_dbpvars == NULL &&
			    kconst->opc_nuops == 0
			)
		    {
			break;
		    }
		}
		nkeys = keyno;
	    }
	    else
	    {
		nkeys -= 1;
	    }

	    /* now get rid of the conjucts that reference key atts >=
	    ** to nkeys. This will simplify the key qual and help to
	    ** let us pass the complexity requirement.
	    */
	    for (qand = cqual->opc_cnf_qual; 
		    qand != NULL; 
		    qand = qand->opc_anext
		)
	    {
		/* find the highest keyno for this conjunct */
		keyno = -1;
		for (compno = 0; compno < qand->opc_ncomps; compno += 1)
		{
		    comp = &qand->opc_comps[compno];

		    if (comp->opc_keyno > keyno)
		    {
			keyno = comp->opc_keyno;
		    }
		}

		if (keyno >= nkeys)
		{
		    /* eliminate this conjunct */
		    if (qand->opc_aprev == NULL)
		    {
			cqual->opc_cnf_qual = qand->opc_anext;
		    }
		    else
		    {
			qand->opc_aprev->opc_anext = qand->opc_anext;
		    }

		    if (qand->opc_anext != NULL)
		    {
			qand->opc_anext->opc_aprev = qand->opc_aprev;
		    }
		}
	    }
	}
    }
}
