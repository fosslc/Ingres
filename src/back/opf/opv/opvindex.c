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
#define        OPV_INDEX      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPVINDEX.C - enter useful indexes into  range table
**
**  Description:
**      Routine to entry useful indexes into the range table
**      FIXME - can include a "set of indexes" since they can replace
**      the base relation entirely, currently an index will be included
**      only if it can replace all attributes (or has a boolean factor).
**      - can include an index since it provides an ordering on an equivalence
**      class which could be used to avoid a sort node in some cases.  This
**      will work for BTREEs, but not for ISAM which is not guaranteed to be
**      ordered.
**
**  History:
**      21-jun-86 (seputis)    
**          initial creation from chkindex.c
**	17-apr-89 (jrb)
**	    Properly initialized prec field(s).
**      16-aug-91 (seputis)
**          add CL include files for PC group
**      14-oct-91 (anitap)
**          Fixed bug 38646. See routine opv_uidex() for further
**          details.
**	16-oct-92 (rickh)
**	   Complete fix for bug 44850 by comparing return value of opv_agvr
**	   to the "no global range variable" symbol rather than the
**	   "no local range variable" symbol.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	31-jan-97 (kch)
**	    In the function opv_uindex(), we now only use an index for the
**	    inner of an outer join if index substitution is possible. This
**	    prevents extra rows being returned from an outer join when the
**	    index does not cover all the predicates in the ON clause. This
**	    change fixes bugs 78998, 79614 and 79846.
**	    79614 and 79846.
**      13-Mar-2002 (hanal04) Bug 107316 INGSRV 1722
**          Prevent SIGSEGV in opv_uindex().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static bool opv_rbase(
	OPS_SUBQUERY *subquery,
	i4 basevar,
	RDD_KEY_ARRAY *keys,
	bool *coverqual);
static bool opv_sameidx(
	OPS_SUBQUERY *subquery,
	OPV_IVARS basevar,
	DMT_IDX_ENTRY *indexp);
static void opv_uindex(
	OPS_SUBQUERY *subquery,
	OPV_IVARS basevar,
	DB_OWN_NAME *owner,
	OPV_IINDEX iindex,
	bool *gotsub,
	DMT_IDX_ENTRY *index_ptr,
	RDD_KEY_ARRAY *prim_keys,
	bool explicit_index);
void opv_index(
	OPS_SUBQUERY *subquery);

/*{
** Name: opv_rbase	- check if index could replace base relation
**
** Description:
**      This index will only be useful if it can replace the base relation
**      entirely.  Check each attribute used in the base relation and
**      verify that the index could be used to substitute one of its own
**      attributes for it.  If an attribute exists which is not in the index
**      then return FALSE since the index cannot totally substitute the base
**      relation.
**
**      The index could replace the base relation if a scan of the entire
**      base relation is required, but the scan of the index would suffice.
**      The index would be chosen since the smaller tuple width would mean fewer
**      disk I/Os would be required.
**
** Inputs:
**      subquery                        subquery being analyzed
**      basevar                         base relation to be substituted
**      dmfkeylist                      dmf list of attributes of the
**                                      base relation defining the key for
**                                      the index
**      dmfcount                        number of element in dmfkeylist
**
** Outputs:
**	coverqual			flag indicating whether index covers
**					all base relation cols ref'ed in 
**					where/on clauses
**	Returns:
**	    TRUE if base relation can be substituted by the index
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	6-may-86 (seputis)
**          initial creation from useful_i
**	13-apr-90 (seputis)
**	    b21087 - changed routine to ignore single attribute functions
**	    as part of index substitution, since if the function can be
**	    evaluated using base table attributes, then the index can 
**	    supply these same attributes.
**	2-jul-90 (seputis)
**	    b30785 - ignore TID attributes for checking since it is always
**	    available in an index for substitution
**      7-dec-93 (ed)
**          b56139 - ignore unassigned attributes since this were removed
**	    due to union view projection action
**	13-mar-97 (inkdo01)
**	    Add logic to set flag when index at least covers all cols ref'ed 
**	    in on/where clauses.
**	10-nov-98 (inkdo01)
**	    Check for NULL key array ptr for base table key covering feature
**	 8-jul-99 (hayke02)
**	    Check for cases where we have attributes, but none have a
**	    db_attr_id > 0. We return FALSE if this is the case. This
**	    prevents indices being picked for updates with no restrictions.
**	    This change fixes bug 94551.
[@history_line@]...
*/
static bool
opv_rbase(
	OPS_SUBQUERY       *subquery,
	i4                basevar,
	RDD_KEY_ARRAY      *keys,
	bool		   *coverqual)
{
    OPZ_AT              *abase;	    /* ptr to base of joinop attributes array */
    OPZ_IATTS		attr;	    /* current joinop attribute being analyzed*/
    bool		result = TRUE;
    bool		noattr = TRUE;
    bool		attrid = FALSE;

    if (keys == NULL) return(FALSE);
    *coverqual = TRUE;		    /* default it to TRUE */
    abase = subquery->ops_attrs.opz_base;
    for ( attr = subquery->ops_attrs.opz_av; attr-- ;)
    {
	OPZ_ATTS               *ap; /* ptr to joinop attribute element being
                                    ** analyzed
                                    */
        DB_ATT_ID              dmfattr; /* dmf attribute associated with
                                    ** joinop attribute */

	noattr = FALSE;
	ap = abase->opz_attnums[attr];
	if ((ap->opz_varnm != basevar)
	    ||
	    (ap->opz_equcls == OPE_NOEQCLS) /* this is probably an
				    ** attribute eliminated by union view
				    ** projection optimization */
	    )
	    continue;		    /* look for any attributes which belong
                                    ** to the base relation, that needs
                                    ** to be substituted
                                    */
	else
	    dmfattr.db_att_id = ap->opz_attnm.db_att_id; /* get dmf attribute
                                    ** number associated
                                    ** with joinop attribute element
                                    ** FIXME - function attributes will
                                    ** disallow index substitution when
                                    ** it could be done (i.e. db_att_id==-1)
                                    ** need to make single var function
                                    ** function attributes behave like
                                    ** multi-var in order to subsitute
                                    ** the index
                                    */
	if (dmfattr.db_att_id > 0)  /* do not look at function attributes
				    ** b21087, also ignore TID attributes
				    ** since these are always available in
				    ** an index - b30785 */
	{   /* look at dmf keylist and see if attribute exists in index */
	    i4                 dmfkey; /* offset into dmf key list */

	    attrid = TRUE;
	    for ( dmfkey = keys->key_count; dmfkey--;)
	    {
		if (keys->key_array[dmfkey] == dmfattr.db_att_id)
		    break;	    /* index contains attribute which can
                                    ** be substituted
                                    */
	    }
	    if (dmfkey < 0)
	     if (ap->opz_mask & OPZ_QUALATT)
	     {
		/* Index covers neither base relation, nor even cols in
		** on/where clauses. */
		*coverqual = FALSE;
		return(FALSE);
	     }
	    else result = FALSE;    /* save result while we check for 
				    ** remaining where/on clause refs */
	}
    }
    if (attrid || noattr)
	return (result);	    /* index can replace all attributes in
                                    ** base relation
                                    */
    else
	return (FALSE);
}

/*{
** Name: opv_sameidx	- check for another identical index
**
** Description:
**	This routine searches through indexes already added to the query
**	for one with identical structure (as can happen with poor database
**	design). If one is found, the new candidate index is NOT added as
**	it would needlessly add to the enumeration cost of the query.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      basevar                         offset of joinop range variable being
**                                      analyzed - i.e. base relation on which
**                                      the index is built
**      index_ptr                       ptr to index tuple to be checked
**
** Outputs:
**	Returns:
**	    TRUE			when current index is identical to 
**					one already added to range table
**	    FALSE			otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-aug-04 (inkdo01)
**	    Written.
**	7-apr-05 (inkdo01)
**	    "continue" instead of "break" in column loop results in 
**	    some indexes being needlessly dropped (bug 114227).
**	16-oct-05 (inkdo01)
**	    This has apparently been doing the wrong comparison all along 
**	    (blush, blush). Now fixed to use DMT_IDX_ENTRYs to compare 
**	    apples to apples (& fix bug 115394).
**	13-apr-06 (dougi)
**	    Skip heuristic during hint pass.
[@history_line@]...
*/
static bool
opv_sameidx(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS	   basevar,
	DMT_IDX_ENTRY	   *indexp)

{
    OPV_RT	*vbase = subquery->ops_vars.opv_base;
    OPV_VARS	*ixvarp;
    DMT_IDX_ENTRY *ixp;
    i4		i, j, kcount;

    if (subquery->ops_mask2 & OPS_IXHINTS)    /* don't apply during hint pass */
	return(FALSE);

    /* Loop over range table looking for indexes on current primary
    ** variable. Analyze each to determine if they are structurally
    ** idetical to the proposed index candidate. */

    for (i = subquery->ops_vars.opv_rv-1; i >= subquery->ops_vars.opv_prv;
				i--)
    {
	ixvarp = vbase->opv_rt[i];
	if (ixvarp->opv_index.opv_poffset != basevar)
	    return(FALSE);		/* no more ixes on current prim */

	/* Test for sameness - first the structure, then the indexed
	** columns. */
	if (indexp->idx_storage_type != 
		ixvarp->opv_grv->opv_relation->rdr_rel->tbl_storage_type)
	    continue;

	kcount = ixvarp->opv_grv->opv_relation->rdr_keys->key_count-1;
	if (ixvarp->opv_grv->opv_relation->rdr_rel->tbl_storage_type ==
			DMT_HASH_TYPE)
	    kcount++;			/* HASH doesn't hash on tidp */
	if (indexp->idx_key_count != kcount)
	    continue;

	/* Address the indexes index descriptor (not the table descriptor). */
	ixp = vbase->opv_rt[basevar]->opv_grv->opv_relation->rdr_indx[
			ixvarp->opv_index.opv_iindex];

	for (j = 0; j < indexp->idx_key_count; j++)
	{
	    if (indexp->idx_attr_id[j] != ixp->idx_attr_id[j])
		break;			/* diff cols - exit inner loop */
	}
	if (j < indexp->idx_key_count)
	    continue;			/* found mismatch in attr loop */

	return(TRUE);			/* indexes must be identical */
    }

    return(FALSE);			/* passed the tests */
}

/*{
** Name: opv_uindex	- check if index is useful
**
** Description:
**	This routine determines if this index can potentially be useful
**	for the given query.  If it is, the index is put in joinop local
**      range table and equivalence classes and joinop attributes are
**      assigned for it.
**
**	Check if index "index_ptr" indexes an attribute which belongs
**	to a joining clause or has a matching boolean factor.
**	If it does then insert this index into the local range table and 
**      assign equivalence class and joinop attributes for the tid and indexed
**	attributes of this index .
**
**	Also, include indexes that can replace base relations. This implies
**	a useful index is one whose attributes also match a varnode in the
**	target list. We will assume all indexes as useful if they have
**	attributes in the query.
**
**	Currently only single attribute keys are handled with respect to
**	joining clauses.
**
**	Don't syserr if index can't be opened... just assume index can't be 
**      used.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      basevar                         offset of joinop range variable being
**                                      analyzed - i.e. base relation on which
**                                      the index is built
**      owner                           owner of the table
**      iindex                          index into array of ptrs to index tuples
**                                      of slot containing index_ptr
**      gotsub				TRUE if current base tab already has
**					a substitution secondary index (only
**					need 1 per primary).
**      index_ptr                       ptr to index tuple to be checked
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
**	2-may-86 (seputis)
**          initial creation from usefulindex
**	23-oct-88 (seputis)
**	    change interface to ope_addeqcls
**	18-jul-90 (seputis)
**	    fix b31644 - improve heuristic to eliminate search space with illegal index
**	    placement
**	14-aug-90 (seputis)
**	    fix b32554 - do not create function attribute when index is already
**	    in the equivalence class
**	18-nov-90 (seputis)
**	    fix b34509 - fix wrong answer for function attribute and index
**      14-oct-91 (anitap)
**          fix b38646 - expression to be evalated for the function
**          attribute for secondary index was poining to the root of
**          the tree instead of to the right branch.
**      21-jan-92 (seputis)
**          fix b41994 - use matching datatype for function attribute, needed
**          for bug fix 38646
**      18-sep-92 (ed)
**          bug 44850 - added parameter to opv_agrv to allow multi-to-one mapping
**          so a common aggregate temp can be used
**	16-oct-92 (rickh)
**	   Complete fix for bug 44850 by comparing return value of opv_agvr
**	   to the "no global range variable" symbol rather than the
**	   "no local range variable" symbol.
**      5-jan-93 (ed)
**          b48049 - initialize new opb_prmaorder member
**      7-dec-93 (ed)
**          b56139 - add OPZ_HIST_NEEDED to mark attributes which
**          need histograms,... needed since target list is traversed
**          earlier than before
**      16-feb-95 (inkdo01)
**          b66907 - avoid use of inconsistent indexes
**	31-jan-97 (kch)
**	    We now only use an index for the inner of an outer join if index
**	    substitution is possible (index_sub is TRUE). This prevents extra
**	    rows being returned from an outer join when the index does not
**	    cover all the predicates in the ON clause. This change fixes
**	    bugs 78998, 79614 and 79846.
**	13-mar-97 (inkdo01)
**	    Slight change to above fix to permit use of index on outer inners,
**	    as long as index covers cols ref'ed in where/on clauses.
**	25-mar-97 (inkdo01)
**	    Fix to only take one substitution index with no high order column
**	    restriction per primary rt entry. This reduces the rt for 
**	    enumeration purposes (bug 78136).
**	29-jul-97 (inkdo01)
**	    Further attempt to reduce no. of secondaries by not including
**	    those covered by base table key structure unless ixsub or base
**	    table structure is hash and secondary isn't.
**	24-sep-97 (inkdo01)
**	    Slight fix to 31-jan change. Original used local variable number
**	    when it should have used global variable number.
**	 7-jul-99 (hayke02)
**	    Only permit use of index on outer inners if the subquery contains
**	    two primary relations. This prevents incorrect results where
**	    the outer outer is a multi-relation view, and an index on the inner
**	    covers all cols ref'ed in the on/where clause. The QEP which
**	    returns the incorrect rows shows an inner join after the index
**	    outer join and before the TID outer join back to the base table.
**	    This change fixes bug 95948.
**	11-sep-00 (inkdo01)
**	    More refinement of secondary index selection in outer joins. If
**	    the variable is only "inner" to joins which aren't "outer" to any
**	    other variables, it is an inner join not nested in an outer and
**	    may be accessed by a TID join. New attempt to implement 94906.
**	24-nov-00 (hayke02)
**	    Reject an Rtree index if we have no spatial functions or joins
**	    (OPB_GOTSPATF | OPB_GOTSPATJ) - Rtrees cannot have any other
**	    key attributes other than the single spatial type, and this
**	    index cannot be used for retrieval and/or restriction of the
**	    spatial. This change fixes bug 103158.
**	18-dec-00 (hayke02)
**	    Modify latest attempt to implement 94906 to test for case where
**	    the outer is a multi-relation view (NULL pst_outer_rel.pst_j_mask's)
**	    and move fix for 95948 to opn_jintersect(). We also test for an
**	    OPS_MAIN created range variable - in the case of bug 103509 we have
**	    a var which is the temporary table from an unflattened subselect
**	    (opv_created equal to OPS_SELECT) which caused a SIGSEGV due to
**	    an uninitialized pst_rangetab entry. This change fixes bugs 100286
**	    and 103509.
**	28-sep-01 (inkdo01)
**	    You guessed it! Another attempt to filter out bad secondaries
**	    from outer join Tjoin eligibility. The previous logic didn't 
**	    handle secondaries on vars whose OJ joinid involved a view whose
**	    expansion created other distinct joinids.
**      13-Mar-2002 (hanal04) Bug 107316 INGSRV 1722
**          Adjust the above change to prevent SIGSEGV when ojdesc
**          is NULL.
**	16-apr-02 (hayke02)
**	    Yet more refinement of secondary index selection in outer joins.
**	    We now allow a secondary inner if we have 2 or more relations in
**	    the inner var map (opl_ivmap). This indicates that the index is
**	    being used in a join before it is used in the outer join and not
**	    directly as with bug 95948. This change fixes bug 107537.
**	13-feb-03 (hayke02)
**	    Set OPV_OJINNERIDX in the basevar opv_mask if this var has a
**	    secondary index chosen for use in the inner of an outer join, but
**	    does not cover the on and where clauses, nor is suitable for index
**	    substitution. Used in opn_jintersect() to indicate that any outer
**	    TID join with this var as an inner should be rejected. This change
**	    fixes bug 109392, side effect of fix for bug 107537.
**	    Move fix for 95948 to opn_jintersect(). This change fixes bug
**	    100286.
**	29-jul-03 (hayke02)
**	    For BTnext() call to find outer join IDs, correct the size of
**	    bitmap from 4 (bytes) to 32 (bits). This allows IDs greater than
**	    3 to be extracted. This change fixes bug 110634.
**	    Also allow secondary indices on inners of folded outer joins.
**	 2-sep-03 (hayke02)
**	    Set OPV_OJINNERIDX even if coverqual is TRUE so that QEPs with
**	    the TID OJ not immediately after the inner index OJ can be
**	    rejected in opn_jmaps(). This change fixes bugs 95948/110790 and
**	    100286.
**	30-apr-04 (inkdo01)
**	    Changed DB_TID_LENGTH to DB_TID8_LENGTH for partitioned tables.
**	20-aug-04 (inkdo01)
**	    Drop index candidates that are identical to ixes already in rt.
**	14-Jan-2005 (schka24)
**	    Don't set up the tid eqcls for replacement indexes; we aren't
**	    going to t-join to them.
**	25-feb-05 (inkdo01)
**	    Remove preceding change - it caused a variety of other problems.
**      17-may-2005 (huazh01)
**          Instead of looking up oj information from global opl_gojt[], we
**          now loop through subquery's opl_ojt[], and use the ojdesc stored
**          in such array to check the 'inner var of oj' problem. The global
**          array opl_gojt[] may be re-initialze to OPL_NOOUTER if there 
**          are many oj union-ed together in one user query. 
**          this fixes both b111536, INGSRV2651 and b114417, INGSRV3274.
**	9-june-05 (inkdo01)
**	    Strengthen test to omit secondaries that are covered by primaries.
**	20-june-05 (inkdo01)
**	    Tweaked change from 20-aug-04 to eliminate HASH index test when
**	    opb_mbf() returns FALSE. It might be a useful index for Kjoin, 
**	    then Tjoin.
**	11-aug-06 (hayke02)
**	    pst_j_mask is now an array of i4's rather than a single i4 (due to
**	    128 vars), so use BTcount() to see if it is 'non-zero'.
**	29-may-07 (hayke02)
**	    Remove 'mbfkl.opb_bfcount > 0' test from 20-june-05 change, as
**	    it was setting primcsec incorrectly to FALSE, thus causing indices
**	    covered by the primary key to be chosen. This change fixes bug
**	    118413.
**	 3-dec-07 (hayke02)
**	    Modify the previous change, and the 20-june-05 change, replacing
**	    mbfkl.opb_bfcount with index_ptr->idx_key_count, as
**	    mbfkl.opb_bfcount is not a reliable method of determining the
**	    secondary index keys. This change fixes bug 119546.
**	20-feb-08 (hayke02)
**	    Use basevar and opl_ivmap, rather than opv_gvar and pst_inner_rel,
**	    when looking for indices that are inner in outer joins. This will
**	    prevent applicable indices not being marked as OPV_OJINNERIDX when
**	    unflattened views and temporary tables cause multiple subqueries
**	    and differences between local and global var numbering. This
**	    change fixes bug 119938.
[@history_line@]...
*/
static VOID
opv_uindex(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS	   basevar,
	DB_OWN_NAME        *owner,
	OPV_IINDEX         iindex,
	bool               *gotsub,
	DMT_IDX_ENTRY      *index_ptr,
	RDD_KEY_ARRAY	   *prim_keys,
	bool		   explicit_index)
{
    OPB_MBF             mbfkl;	    /* matching boolean factors and keylist
                                    ** - index is useful if key can be built
                                    ** from limiting predicates
                                    */
    OPB_KA              keyorder;   /* array of descriptors for index key */
    OPO_STORAGE		storage;    /* storage structure of index being
                                    ** analyzed
                                    */
    OPE_IEQCLS		equcls;	    /* equivalence class of primary joinop 
                                    ** attribute that the index is ordered on
                                    */
    RDD_KEY_ARRAY       rdr_keys;   /* needed to call opb_jkeylist */
    OPZ_AT              *abase;	    /* ptr to base of joinop attributes array */
    bool		index_sub;  /* TRUE if index substitution is possible */
    bool		coverqual;  /* TRUE if index at least covers cols ref'd
				    ** in where/on clause */
    bool		primcsec;   /* TRUE primary table structure keys
				    ** cover the secondary keys */
    OPZ_IATTS	       maxattr; /* number of joinop attributes defined*/
    OPV_RT              *vbase;     /* ptr to base of joinop range table */
    OPV_VARS		*basevarp;
    OPE_ET		*ebase;	    /* ptr to base of eqcls descriptor array */
    bool		urtree = FALSE; /* usable Rtree ? */
    i4			idx_key_count;

    vbase = subquery->ops_vars.opv_base;
    basevarp = vbase->opv_rt[basevar];
    ebase = subquery->ops_eclass.ope_base;
    if (index_ptr->idx_status & DMT_I_INCONSISTENT) return;
                                    /* Can't use broken indexes */
    if (explicit_index)
    {	/* check if an explicitly referenced index is already in the plan, in which
	** case, do not look at the same index again */
	OPV_IVARS	evar;
	for (evar = subquery->ops_vars.opv_rv; --evar>=0;)
	{
	    OPV_VARS	    *evarp;
	    evarp = vbase->opv_rt[evar];
	    if (evarp->opv_index.opv_poffset == basevar)
	    {	/* found explicitly referenced index */

		if ((index_ptr->idx_id.db_tab_base == evarp->opv_grv->opv_relation->rdr_rel->tbl_id.db_tab_base)
		    &&
		    (index_ptr->idx_id.db_tab_index == evarp->opv_grv->opv_relation->rdr_rel->tbl_id.db_tab_index))
		    return;	    /* index is explicitly reference so do
				    ** not consider it again */
	    }
	}
    }
    maxattr = subquery->ops_attrs.opz_av; /* number of joinop attributes
				    ** defined
				    */
    equcls = OPE_NOEQCLS;           /* no equivalence class is assigned if
                                    ** the attribute is not found in the
                                    ** query
                                    */
    abase = subquery->ops_attrs.opz_base;
    rdr_keys.key_array = index_ptr->idx_attr_id;
    storage = index_ptr->idx_storage_type;
    mbfkl.opb_kbase = &keyorder;
    mbfkl.opb_maorder = NULL;
    mbfkl.opb_prmaorder = NULL;
    if (storage != DB_RTRE_STORE) rdr_keys.key_count = index_ptr->idx_key_count;
    else rdr_keys.key_count = index_ptr->idx_array_count;
				    /* restrict key definition to those 
				    ** attributes in the key, i.e. ignore the
				    ** deadweight attributes (except for 
				    ** goofy Rtrees which don't have "keys) */
    opb_jkeylist( subquery, basevar, storage, &rdr_keys, &mbfkl);
				    /* obtain a keylist based on the
                                    ** index, but using basevar to find 
                                    ** var nodes which reference attributes 
                                    ** in the index
                                    */
    rdr_keys.key_count = index_ptr->idx_array_count; /* for index subsitition
				    ** tests, consider all attributes in the
				    ** secondary index */

    index_sub = (storage != DB_RTRE_STORE) 
		&& opv_rbase(subquery, basevar, &rdr_keys, &coverqual);
    if (mbfkl.opb_count == 0)
    {	/* the outer most attribute of the index was not referenced so
        ** index will only be useful if it can replace the base relation
        ** entirely
        */
	if (!index_sub)
	    return;		    /* attributes in base relation exist
                                    ** which are not in the index so return
                                    */
    }
    else if (storage == DB_RTRE_STORE)
    {	/* do the Rtree eligibility check */
	OPB_IBF		bfi;
	OPB_BOOLFACT	*bfp;

	if (!(subquery->ops_bfs.opb_mask & (OPB_GOTSPATF | OPB_GOTSPATJ)))
	    return;

	equcls = abase->opz_attnums[mbfkl.opb_kbase->opb_keyorder[0].opb_attno]
		->opz_equcls;	    /* get equivalence class of joinop attribute
                                    ** which is the primary attribute of the key
                                    */
	for (bfi = subquery->ops_bfs.opb_bv; bfi > 0; bfi--)
	{
	    bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi-1];
	    if (bfp->opb_mask & (OPB_SPATF | OPB_SPATJ) && 
		(bfp->opb_eqcls == equcls || bfp->opb_eqcls == OPB_BFMULTI &&
		 BTtest((i4)equcls, (char *)&bfp->opb_eqcmap))) break;
	}
	if (bfi > 0) urtree = TRUE;
	 else return;
    }
    else
    {
	equcls = abase->opz_attnums[mbfkl.opb_kbase->opb_keyorder[0].opb_attno]
		->opz_equcls;	    /* get equivalence class of joinop attribute
                                    ** which is the primary attribute of the key
                                    */
	if (!opb_mbf( subquery, storage, &mbfkl )) /* opb_mbf is TRUE if 
				    ** matching boolean factors
                                    ** can be found to build a key which
                                    ** can be used for the index
                                    */
	{   /* check if a joining clause exists on the index */
	    if ( equcls == OPE_NOEQCLS 
		||		    /* if no equivalence class is
                                    ** assigned then attribute exists; is only
                                    ** in the target list; moreover no joining
                                    ** clause will exist on the attribute
                                    ** if the BTcount is less than one
                                    ** so index will not be useful. 
                                    */
		BTcount( (char *)&subquery->ops_eclass.ope_base->
		    ope_eqclist[equcls]->ope_attrmap, (i4)maxattr ) <= 1
		)
				    /* only one element in equivalence class
                                    ** means no joining clause exists on the
                                    ** primary attribute of the index
                                    */
		    if (!index_sub) /* index still
                                    ** can be useful if base relation can be
                                    ** replaced
                                    */
			return;	    /* attributes in base relation 
                                    ** exist in query
                                    ** which are not in the index so return
                                    ** since index cannot replace relation
                                    ** totally
                                    */
	}
    }

    /*
    ** inner of an outer join - only use index
    ** if index substitution is possible or index
    ** covers cols ref'd by where/on clauses.
    */
    if (!index_sub)
    {
	OPL_OUTER	*ojdesc;
	i4          i; 

	/* To verify this is really a var that's inner to an outer
	** join, loop over the inner OJs and see if any is left, right
	** or full. If so, a TID join with this index isn't possible. */

	for (i = 0; i < subquery->ops_oj.opl_lv; i++)
	{
	    ojdesc = subquery->ops_oj.opl_base->opl_ojt[i];
	    if (!BTtest((i4)basevar, (char *)ojdesc->opl_ivmap))
		continue; 

	    if (!coverqual && (BTcount((char *)ojdesc->opl_ivmap,
		(i4)BITS_IN(*ojdesc->opl_ivmap)) == 1) &&
		(ojdesc->opl_type == OPL_LEFTJOIN ||
		ojdesc->opl_type == OPL_RIGHTJOIN ||
		ojdesc->opl_type == OPL_FULLJOIN))
		return;		/* inner var of an OJ */
	    else if ((ojdesc->opl_type != OPL_INNERJOIN)
		&&
		(ojdesc->opl_type != OPL_FOLDEDJOIN))
		basevarp->opv_mask |= OPV_OJINNERIDX;
	}
    }

    /* Loop over primary and secondary key structures. If primary covers 
    ** secondary, there may be no point in adding it to the range table. */
    primcsec = TRUE;
    if (storage != DB_RTRE_STORE) idx_key_count = index_ptr->idx_key_count;
    else idx_key_count = index_ptr->idx_array_count;
    if (prim_keys && prim_keys->key_count >= idx_key_count)
    {
	i4	i;
	for (i = 0; i < idx_key_count && primcsec; i++)
	 if (prim_keys->key_array[i] != rdr_keys.key_array[i]) primcsec = FALSE;
    }
    else primcsec = FALSE;

    /* If this index is only interesting because it covers all cols ref'd
    ** in query (index_sub) AND it's 1st col isn't covered by the where clause
    ** AND there's already an index_sub index for this primary, discard this
    ** one. It would be unlikely to be better than the one already added, and
    ** index_sub indexes just clog up the enumeration process. The same logic 
    ** applies for non-substitution secondaries which are covered by the
    ** base table key structure. The primary should always be a better 
    ** choice, when the cost of TID joining is added in. */
    if (index_sub && *gotsub && mbfkl.opb_count == 0) return;
    if (!index_sub && primcsec) return;

    if (!index_sub && opv_sameidx(subquery, basevar, index_ptr))
	return;				/* check for identical index already
					** in range table
					*/

    /* this point will be reached only if the index is considered useful 
    ** so add it to the local and global range tables
    */
    {
	OPV_IGVARS	       grv;	/* offset into global range table of
                                        ** entry made for index
                                        */
	OPV_IVARS              lvar;    /* offset into local joinop range table
                                        ** of entry made for index
                                        */
        OPV_VARS               *lvarp;  /* ptr to local joinop range table
                                        ** entry representing index */

	/* - get the RDF table descriptor for the index 
	** - return if it cannot be opened and simply optimize without the index
	*/
	grv = opv_agrv(subquery->ops_global, &index_ptr->idx_name, owner, 
		    &index_ptr->idx_id, OPS_MAIN, FALSE, 
		    &subquery->ops_vars.opv_gbmap, OPV_NOGVAR);
                                        /* get global range table entry for 
                                        ** index - do not abort if problems
                                        ** occur
					*/
        if (grv == OPV_NOGVAR)
	    return ;			/* return if unsuccessful reading in
				        ** index */
	lvar = opv_relatts( subquery, grv, equcls, basevar);/* create a joinop
					** range table entry for the index
					*/
	if (subquery->ops_keep_dups)
	{   /* if TIDs cannot be used to keep duplicates then duplicates
            ** need to be removed for indexes, in case index substitution
            ** is made */
            if (BTtest((i4)basevar, (char *)subquery->ops_remove_dups))
		BTset((i4)lvar, (char *)subquery->ops_remove_dups); /* add
                                        ** index var to set of relations
                                        ** which require duplicates to be
                                        ** removed */
	}
        if (subquery->ops_oj.opl_base)
            opl_index(subquery, basevar, lvar); /* update the outer join
                                        ** info for the index */
	lvarp = subquery->ops_vars.opv_base->opv_rt[lvar]; /* ptr to joinop 
					** range table element assigned to query
					*/
	if (index_sub)
	{
	    lvarp->opv_mask |= OPV_CINDEX;  /* mark index as being able to
					** subsitute base relation */
	    subquery->ops_mask |= OPS_CINDEX;
	    *gotsub = TRUE;		/* for future reference */
	}
	else
	{
	    lvarp->opv_mask |= OPV_CPLACEMENT;
	    subquery->ops_mask |= OPS_CPLACEMENT;
	}
	if (urtree)
	{
	    lvarp->opv_mask |= OPV_SPATF;
	    subquery->ops_mask |= OPS_SPATF;
	}
        lvarp->opv_index.opv_iindex = iindex; /* save the index into the array
                                        ** of ptrs to index tuples (the base of
                                        ** which can be found in the RDF struct
                                        ** associated with the base relation)
                                        */
        STRUCT_ASSIGN_MACRO(mbfkl, lvarp->opv_mbf); /* use the matching boolean
                                        ** factors found during the 
                                        ** determination of the usefulness of
                                        ** of the index
                                        */
	lvarp->opv_mbf.opb_keywidth = lvarp->opv_grv->opv_relation->rdr_rel->
	    tbl_width;			/* width of key in directory is width
					** of entire tuple for an index */
	
	lvarp->opv_mbf.opb_kbase = (OPB_KA *)opu_memory(subquery->ops_global,
		(i4)(sizeof(OPB_ATBFLST) * index_ptr->idx_array_count));
					/* size of memory required to store
					** attributes used in index */
	MEcopy((PTR)mbfkl.opb_kbase,
		sizeof(OPB_ATBFLST) * index_ptr->idx_array_count,
		(PTR)lvarp->opv_mbf.opb_kbase); /* copy the key list of
					** attributes out of stack */
	    
	{   /* add a joinop attribute for each index attribute which is
            ** referenced in the query
            */
	    i4                 key;	/* attribute of key for index */


	    for ( key = 0; key < rdr_keys.key_count; key++)
	    {
		DB_ATT_ID	  indmfattr; /* dmf attribute number */
		OPZ_ATTS          *ap;  /* joinop attribute element of base
					** relation attribute being analyzed
					*/
		OPZ_IATTS	  indexattr;   /* joinop attribute number for index
					** attribute
					*/
		OPZ_IATTS	  baseattr; /* joinop attribute number for base
					** relation attribute corresponding
					** to index attribute
					*/
		bool		  success;

		if (mbfkl.opb_count > key)
		{   /* if the attribute was found then use it */
		    baseattr = mbfkl.opb_kbase->opb_keyorder[key].opb_attno;
		}
		else
		{   /* lookup joinop attribute number which may be in the
                    ** portion of the multi-attribute key which is not used
                    ** for keying
                    */
                    indmfattr.db_att_id = rdr_keys.key_array[key] ; /* get attr
                                        ** number of the base relation which
                                        ** is used as part of the key */
		    baseattr = opz_findatt( subquery, basevar, 
			indmfattr.db_att_id);
		    if ( baseattr == OPZ_NOATTR )
			continue;	/* if joinop attribute could not
					** be found then it is not 
                                        ** referenced in the query... and
                                        ** would not be useful
                                        */
		}
		ap = abase->opz_attnums[baseattr]; /* joinop attribute element
					** of base relation attribute
					** corresponding to index attribute
					*/
                indmfattr.db_att_id = key+1; /* Element 0 in the key array is
                                        ** equivalent to dmfattr number 1 of
                                        ** the index relation */
		indexattr = opz_addatts( subquery, lvar, indmfattr.db_att_id, 
			    &ap->opz_dataval ); /* create joinop attribute */
		if (mbfkl.opb_count > key)
		{   /* if the attribute was found then use it */
		    lvarp->opv_mbf.opb_kbase->opb_keyorder[key].opb_attno 
			= indexattr;	/* save joinop attribute number of the
					** index in the corresponding element of
                                        ** the keylist - keylist currently
                                        ** contains joinop attribute of base
                                        ** relation
                                        */
		}
		if (ap->opz_equcls == OPE_NOEQCLS)
					/* FIXME - this has the side effect
                                        ** of causing histo to read histogram
                                        ** information about this attribute
                                        ** which may not be useful - this also
                                        ** creates a join which may not be
                                        ** be useful - perhaps only attributes
                                        ** in the qualification should cause
                                        ** equivalence classes to be produced?
                                        */
		    (VOID) ope_neweqcls( subquery, baseattr);
					/* joinop attribute may
					** have only been in the target list
					** but not in the qualification
					** - still useful if base relation
					** can be replaced by index
					*/
	        success = ope_addeqcls( subquery, ap->opz_equcls, indexattr, TRUE);/*
					** add the joinop attribute for index 
					** to the equivalence class containing
					** the corresponding base relation
					** attribute, TRUE indicates that NULLs
					** should not be removed based on addition
					** of this attribute, i.e. joining the NULL
					** in the secondary to the base relation
					** should not remove the tuple
					*/
#ifdef    E_OP0385_EQCLS_MERGE
		if (!success)
		    opx_error(E_OP0385_EQCLS_MERGE);
#endif
		{   /* look for case in which another index exists which can
		    ** supply this attribute */
		    OPZ_IATTS		second;
		    OPZ_BMATTS		*attrmap;
		    OPE_EQCLIST		*eqclsp;

		    eqclsp = ebase->ope_eqclist[ap->opz_equcls];
		    eqclsp->ope_mask |= OPE_HIST_NEEDED; /* indicate that histograms are
					    ** needed for this attribute */
		    attrmap = &eqclsp->ope_attrmap;
		    for (second = -1;
			(second = BTnext((i4)second, (char *)attrmap, (i4)maxattr))>=0;)
		    {
			OPV_IVARS	varno;
			OPV_VARS	*varp;
			varno = abase->opz_attnums[second]->opz_varnm;
			if (varno == basevar)
			    continue;	/* OK to reference base variable */
			varp = vbase->opv_rt[varno];
			if ((varp->opv_index.opv_poffset == basevar)
			    &&
			    (basevarp->opv_primary.opv_tid != OPE_NOEQCLS))
			{   /* found a duplicate attribute so special case checking is necessary */
			    subquery->ops_mask |= OPS_CTID;
			    subquery->ops_eclass.ope_base->ope_eqclist[basevarp->opv_primary.opv_tid]
				->ope_mask |= OPE_CTID;
			    break;
			}
		    }
		}
	    }
	}    
	{   /* add information about the explicit and implicit TIDs */
	    DB_DATA_VALUE      tid;     /* data type for tid */
	    OPZ_IATTS	       implicit; /* joinop attribute number for the
                                        ** implicit TID of the base relation
                                        */
            OPZ_IATTS	       explicit; /* joinop attribute number for the
                                        ** explicit TID of the index
                                        */
	    OPZ_ATTS           *imp;    /* ptr to joinop table element for
                                        ** implicit TID of base relation
                                        */
	    DB_ATT_ID		imdmfattr; /* dmf attribute number */
	    bool		success;

	    /* if the implicit TID of relation r has not been created as an 
	    ** attribute then create it, and also create an eqclass for it
            **
	    ** set the idomnmbt field in Jn.Rangev[r] so that the primary rel
	    ** knows the eqclass of its implicit TID (done in addatts)
	    */
	    if (basevarp->opv_grv->opv_relation->rdr_parts)
	    {
		tid.db_datatype = DB_TID8_TYPE;
		tid.db_prec = 0;
		tid.db_length = DB_TID8_LENGTH;
	    }
	    else
	    {
		tid.db_datatype = DB_TID_TYPE;
		tid.db_prec = 0;
		tid.db_length = DB_TID_LENGTH;
	    }
            imdmfattr.db_att_id = DB_IMTID;
	    implicit =opz_addatts( subquery, basevar,imdmfattr.db_att_id, &tid);
					/* save joinop attribute number of the
					** implicit TID.
                                        */
            imp = abase->opz_attnums[implicit];
	    if (imp->opz_equcls == OPE_NOEQCLS)
		(VOID) ope_neweqcls( subquery, implicit); /* create an
                                        ** equivalence class for the implicit
                                        ** attribute
					*/
            imdmfattr.db_att_id = rdr_keys.key_count+1;/* the explicit TID 
                                        ** is the last attribute of the index
                                        */
	    explicit =opz_addatts(subquery, lvar, imdmfattr.db_att_id, &tid);
					/* create joinop attribute number of the
					** explicit TID for the index
                                        */
	    success = ope_addeqcls( subquery, imp->opz_equcls, explicit, TRUE); /* add the
                                        ** joinop attribute of the explicit
                                        ** TID to the same equivalence class
                                        */
	    ebase->ope_eqclist[imp->opz_equcls]->ope_mask |= OPE_HIST_NEEDED; /* mark
					** eqcls as needing histograms */
#ifdef    E_OP0385_EQCLS_MERGE
	    if (!success)
		opx_error(E_OP0385_EQCLS_MERGE);
#endif
	}
	if (subquery->ops_attrs.opz_base)
	{
	    /* add information about function attributes which can be evaluated at
	    ** the base relation but also can be evaluated at this index
	    ** this is so index subsitution can occur */
	
	    OPZ_IFATTS            fattr; /* function attribute currently
					** being analyzed */
	    OPZ_IFATTS            maxfattr; /* maximum function attribute
					** defined */
	    OPZ_FT                *fbase; /* ptr to base of an array
					** of ptrs to function attributes*/
	    OPE_IEQCLS		  maxeqcls;
	    OPE_ET		  *ebase;

	    maxfattr = subquery->ops_funcs.opz_fv; /* maximum number of
					** function attributes defined */
	    fbase = subquery->ops_funcs.opz_fbase; /* ptr to base of array
					** of ptrs to function attribute
					** elements */
	    ebase = subquery->ops_eclass.ope_base;

	    maxeqcls = subquery->ops_eclass.ope_ev;

	    for (fattr = 0; fattr < maxfattr; fattr++)
	    {
		OPZ_FATTS		*fattrp; /* ptr to current function
					** attribute being analyzed */
		OPE_IEQCLS          feqc; /* equivalence class containing
					** the function attribute */
		bool		    index_fattr;    /* TRUE if the index
					** can calculate this function
					** attribute */
		OPZ_IATTS	    maxattrs;

		maxattrs = subquery->ops_attrs.opz_av; /* init at this point
					** since it may change as new attributes
					** get added */
		fattrp = fbase->opz_fatts[fattr]; /* get ptr to current
					** function attr being analyzed*/
		switch (fattrp->opz_type)
		{
		case OPZ_TCONV:
		case OPZ_SVAR:
		case OPZ_MVAR:
		{
		    index_fattr = (abase->opz_attnums[fattrp->opz_attno]->opz_varnm == basevar)
			&&
			!(fattrp->opz_mask & OPZ_OJFA);
		    if (!index_fattr)
			break;		    /* only use function attributes if the
					    ** base relation has it, since numerous
					    ** checks are made to determine whether
					    ** a function attribute is legitimate */
		    
		    for (feqc = -1; (feqc = BTnext((i4)feqc, (char *)&fattrp->opz_eqcm, (i4)maxeqcls))>=0;)
		    {	
			bool		in_base; /* TRUE if equivalance class found in the
					    ** base relation */
			bool		in_index; /* TRUE if the equivalence class is found
					    ** in the function attribute */
			OPZ_IATTS	battr; /* attribute number being analyzed */
			OPZ_BMATTS	*fa_attrmap;

			fa_attrmap = &ebase->ope_eqclist[feqc]->ope_attrmap;
			in_base = FALSE;
			in_index = FALSE;
			for (battr = -1; (battr= BTnext((i4)battr, (char *)fa_attrmap, maxattrs))>=0;)
			{   /* would be more convenient to have eqcls bitmaps opv_eqcmp defined
			    ** to determine intersection but this does not get defined until
			    ** opv_ijnbl */
			    if (abase->opz_attnums[battr]->opz_varnm == basevar)
				in_base = TRUE;
			    else if (abase->opz_attnums[battr]->opz_varnm == lvar)
				in_index = TRUE;
			}
			if (!in_base || !in_index)
			{   /* attribute in the base relation but not in the index */
			    index_fattr = FALSE;    /* index cannot substitute this
						    ** function attribute */
			    break;
			}
		    }
		    break;
		}
		    default:
			index_fattr = FALSE;
		}
		if (index_fattr)
		{   /* check that variable is not already in the equivalence
		    ** class */
		    OPZ_IATTS	    attr1;   /* attribute number of a joinop attr
						** which is already in the equivalence
						** class
						*/
		    OPZ_BMATTS	    *attr1map;
		    attr1map = &ebase->ope_eqclist[abase->opz_attnums
			[fattrp->opz_attno]->opz_equcls]->ope_attrmap;
		    for (attr1 = -1; 
			 (attr1 = BTnext((i4)attr1,
					(char *) attr1map, 
					maxattrs)
			 ) >= 0;)
			/* cannot merge cause then we would have more than one att from a
			** single relation in the same eqclass.  Just keep the eqclasses
			** seperate and keep the clause in the query 
			*/
			if (abase->opz_attnums[attr1]->opz_varnm == lvar)
			{
			    index_fattr = FALSE;
			    break;
			}
		}		
		if (index_fattr)
		{
		    /* create a function attribute for this index */
		    OPZ_FATTS	    *func_attr;	/* ptr to function attribute used
						    ** to contain constant node */
		    PST_QNODE	    *expr;	/* ptr to expression defining the
						** constant node */
		    OPZ_IATTS       fattr1;	/* joinop function attribute */
		    bool	    success1;

		    expr = fattrp->opz_function->pst_right;
		    opv_copytree(subquery->ops_global, &expr);	/* do not have
						** two function attributes pointing
						** to the same tree */
		    func_attr = opz_fmake( subquery, expr, fattrp->opz_type);
                    STRUCT_ASSIGN_MACRO( fattrp->opz_function->pst_sym.pst_dataval,
                        func_attr->opz_dataval);
		    fattr1 = opz_ftoa( subquery, func_attr, lvar, expr,
			&func_attr->opz_dataval, fattrp->opz_type);
		    success1 = ope_addeqcls(subquery, abase->opz_attnums
			[fattrp->opz_attno]->opz_equcls, fattr1, TRUE); 
#ifdef    E_OP0385_EQCLS_MERGE
		    if (!success1)
			opx_error(E_OP0385_EQCLS_MERGE);
#endif
		}
	    }
	}
	if (basevarp->opv_mask & OPV_FIRST)
	{
	    basevarp->opv_mask |= OPV_SECOND;
	    subquery->ops_mask |= OPS_MULTI_INDEX;
	}
	else
	    basevarp->opv_mask |= OPV_FIRST;
    }
}

/*{
** Name: opv_index	- check for useful indexes
**
** Description:
**      Search for all potentially useful indexes
**	- for each index on the specified relation usefulindex is called with
**	the relation and index tuple as parameters. 
**
**      Indexes are modeled as tables in Ingres.  When an index is read, the
**      "inx_attr_id" component of the RDF index tuple is used to join
** 
** Inputs:
**      subquery                        ptr to subquery being analyzed
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
**	2-may-86 (seputis)
**          initial creation chk_index
**	16-nov-89 (seputis)
**	    avoid using secondaries on deferred or direct cursors
**	    on columns to be updated (FIXME - better solution
**	    is to create a temporary for the TIDs) b8780
**      26-mar-90 (seputis)
**          - fix b20632
**	16-may-90 (seputis)
**	    - fix lint error
**	    - rdf interface requires that opv_relation be reset after each call
**	26-feb-91 (seputis)
**	    - add improved diagnostics for b35862
**	31-jul-97 (inkdo01)
**	    Add parm to opv_uindex call for heuristic concerning primary
**	    structure which covers secondaries.
**	7-aug-97 (inkdo01)
**	    Moved init of prim_keys slightly.
**	10-nov-98 (inkdo01)
**	    Add support of OPV_BTSSUB flag, indicating B-tree base table
**	    can be accessed through index leaves and NOT base table pages.
**	2-dec-98 (inkdo01)
**	    Minor change to above feature to assure at least one index page.
**	10-dec-98 (inkdo01)
**	    Yet another minor change to assure BTREE and not gateway table.
**	22-may-02 (inkdo01)
**	    Change var loop to be ascending (in from clause order) rather than
**	    descending (reverse from clause order) on assumption that indexes
**	    on earlier tables in from are more important.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
**	26-jul-04 (hayke02)
**	    Add op211 and opf_old_idxorder to revert back to old reverse var
**	    'for' loop for calling opv_uindex(). This change fixes problem
**	    INGSRV 2919, bug 112737.
**	27-Jul-2004 (schka24)
**	    Fetch all interesting RDF info, not just index info.
**	5-may-2008 (dougi)
**	    Skip table procedures - no indexes there.
*/
VOID
opv_index(
	OPS_SUBQUERY       *subquery)
{
    OPV_IVARS		var = 0;    /* index into joinop range table of 
                                    ** range table variable being analyzed
                                    */
    OPV_RT              *vbase;     /* ptr to base of joinop range table */
    OPV_GRV             *direct_ptr; /* points to global range variable
                                    ** which is target of cursor
                                    ** which is opened for DIRECT update */
    bool		dummy;
    bool		rev_var = FALSE;
    OPV_IVARS		i;

    direct_ptr = NULL;
#if 0
/* use indexes at all times but create temp sort nodes when necessary
** to avoid the halloween problem */
    if ((   (subquery->ops_global->ops_qheader->pst_updtmode == PST_DIRECT)
	    ||
	    (subquery->ops_global->ops_qheader->pst_updtmode == PST_DEFER)
	)
        &&
        (subquery->ops_global->ops_qheader->pst_mode == PSQ_DEFCURS)
       )
    {	/* find pointer to global range variable element which is to be
        ** replaced ... this will be used to determine if there is a
        ** restriction on the indices to be considered for use */
        OPV_IGVARS	    resultvarno; /* global range table number of
					** result relation */

        resultvarno = subquery->ops_global->ops_qheader->pst_restab.pst_resvno;
	if (resultvarno >= 0)
	    direct_ptr = subquery->ops_global->ops_rangetab.opv_base->
		opv_grv[resultvarno];
    }
#endif
    vbase = subquery->ops_vars.opv_base; /* get base of joinop range table */

    /* The range table loop used to be decrementing, but for queries with so
    ** many tables that not all useful indexes can be added, it is likely
    ** that indexes on the earlier tables in the from clause are more 
    ** important. If severe performance problems ensue for existing queries, 
    ** we could change the loop to start as decrementing, then switch to 
    ** incrementing if there isn't enough room in the range table. */
    if (((subquery->ops_global->ops_cb->ops_server->opg_smask &
							    OPF_OLDIDXORDER) 
	||
	(subquery->ops_global->ops_cb->ops_check
	&&
	opt_strace(subquery->ops_global->ops_cb, OPT_F083_OLDIDXORDER))) 
	&&
	subquery->ops_vars.opv_prv > 0)
    {
	rev_var = TRUE;
	var = (subquery->ops_vars.opv_prv - 1);
    }
    if (subquery->ops_vars.opv_prv > 0)
     for (i = 0; i < subquery->ops_vars.opv_prv; i++, rev_var ? var-- : var++)
    {	/* analyze the indexes associated with all the base relations */
	OPV_GRV		       *gvar_ptr; /* ptr to global range table element
                                    ** being analyzed
                                    */
	OPV_VARS               *lvar_ptr; /* ptr to local joinop range table
                                    ** element being analyzed
                                    */

	lvar_ptr = vbase->opv_rt[var]; /*get ptr to joinop range table element
                                    ** being analyzed
                                    */
	gvar_ptr = lvar_ptr->opv_grv;   /* get ptr to corresponding global 
                                    ** range table element
                                    */
	if( !gvar_ptr->opv_relation )/* if the ptr to the RDF element is NULL
                                    ** then this is a temporary table which
                                    ** is the result of aggregate processing and
                                    ** would not have any indexes defined
                                    */
	    continue;

	if (gvar_ptr->opv_gmask & OPV_TPROC)
	    continue;		    /* table procs aren't interesting */

	/* Check if table is Btree and keyed columns cover all cols of table
	** referenced in subquery. This will allow us to avoid reading base
	** table data pages. Note, no point in doing this unless the table 
	** is big enough to contain some index pages. */
	if (gvar_ptr->opv_relation->rdr_rel->tbl_storage_type == DB_BTRE_STORE &&
		!(gvar_ptr->opv_relation->rdr_rel->tbl_status_mask & DMT_GATEWAY) &&
		gvar_ptr->opv_relation->rdr_rel->tbl_id.db_tab_index <= 0 &&
		gvar_ptr->opv_relation->rdr_rel->tbl_ipage_count > 0 &&
		opv_rbase(subquery, var, 
				gvar_ptr->opv_relation->rdr_keys, &dummy) &&
	    BTnext((i4)-1, (char *)&subquery->ops_global->ops_qheader->pst_updmap,
		(i4)BITS_IN(subquery->ops_global->ops_qheader->pst_updmap)) < 0)
	{
	    lvar_ptr->opv_mask |= OPV_BTSSUB;
	    lvar_ptr->opv_tcop->opo_cost.opo_cvar.opo_relprim =
	    lvar_ptr->opv_tcop->opo_cost.opo_reltotb =
		gvar_ptr->opv_relation->rdr_rel->tbl_ipage_count;
	}

	if( !(gvar_ptr->opv_relation->rdr_rel->tbl_status_mask & DMT_IDXD) )
	    continue;		    /* continue if no indexes exist */
	if ((lvar_ptr->opv_mbf.opb_bfcount == 1) /* is there a matching boolean
                                    ** factor associated with the base relation
                                    */
            &&
	    (lvar_ptr->opv_primary.opv_tid ==subquery->ops_attrs.opz_base->
		opz_attnums[lvar_ptr->opv_mbf.opb_kbase->opb_keyorder[0].opb_attno]
		    ->opz_equcls))  /* is the 
                                    ** implicit TID joinop attribute used for 
				    ** the key
                                    */
	    continue;               /* do not use any indexes if a TID lookup
                                    ** on the base relation has already been
                                    ** found
				    ** - FIXME - is this tid attribute ever
				    ** set in the primary storage structure
                                    */
	if( !gvar_ptr->opv_relation->rdr_indx )
	{   /* if index information does not exist then call RDF to get it */
	    RDF_CB             *rdfcb;	/* ptr to RDF control block used to
                                    ** obtain index information
                                    */
	    i4            status; /* RDF return status */

	    rdfcb = &subquery->ops_global->ops_rangetab.opv_rdfcb;
	    STRUCT_ASSIGN_MACRO( subquery->ops_global->ops_qheader->
		pst_rangetab[gvar_ptr->opv_qrt]->pst_rngvar, 
		rdfcb->rdf_rb.rdr_tabid); /* get table id from parser range 
				    ** table */
	    /* Ask for all optimizer-relevant info, because we're going to
	    ** make RDF's answer the official info for this table.  We
	    ** don't want to lose e.g. keying info if RDF decides to give
	    ** us a new private info copy.  If our RDF info block already
	    ** has the requested stuff, asking again is free.
	    */
	    rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION | RDR_ATTRIBUTES
				| RDR_BLD_PHYS | RDR_INDEXES;
	    rdfcb->rdf_info_blk = gvar_ptr->opv_relation;
	    
	    status = rdf_call( RDF_GETDESC, rdfcb);
            gvar_ptr->opv_relation = rdfcb->rdf_info_blk; /* save descriptor
				    ** since RDF may change the underlying
                                    ** info block */
	    if ((status != E_RD0000_OK)
		||
		!gvar_ptr->opv_relation->rdr_indx )
		continue;	    /* if index information not available then
                                    ** continue processing
                                    */
	}
	/* index information is now available so add any useful indexes to
	** to the local range table
	*/
	{
	    OPV_IINDEX         ituple; /* index into the array of index tuples
				    ** associated with global range table
                                    */
            DB_OWN_NAME        *owner; /* ptr to owner name */
	    RDD_KEY_ARRAY      *prim_keys; /* ptr to base table key structure */
	    bool		gotsub = FALSE;	/* TRUE: got a substituting
				    ** secondary for this primary */

	    if (gvar_ptr->opv_relation->rdr_rel->tbl_storage_type != 
								DMT_HASH_TYPE)
		prim_keys = gvar_ptr->opv_relation->rdr_keys;
	     else prim_keys = NULL;
            owner = &gvar_ptr->opv_relation->rdr_rel->tbl_owner;
	    for( ituple = gvar_ptr->opv_relation->rdr_rel->tbl_index_count;
		ituple-- ;)
	        opv_uindex(subquery, var, owner, ituple, &gotsub,
		    gvar_ptr->opv_relation->rdr_indx[ituple], 
		    prim_keys,
		    (lvar_ptr->opv_mask & OPV_EXPLICIT)!=0); /* check if 
                                    ** the index is useful and add it to 
                                    ** the local range table if it is
                                    */
	}
    }
    if (subquery->ops_dist.opd_updtsj != OPV_NOVAR)
    {
	if (subquery->ops_dist.opd_updtsj != (subquery->ops_vars.opv_prv-1))
	    opx_error(E_OP0A87_UPDT_MATCH);
	--subquery->ops_vars.opv_prv;	/* make the new range variable look like
				    ** an index so it does not need to be used
				    ** but is only considered as an alternative
				    ** access path */
    }
}
