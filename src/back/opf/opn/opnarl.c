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
#define        OPN_ARL      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPNARL.C - routines to attach relations to leaves of a join tree
**
**  Description:
**      Routines to attach relations to the leaves of a structurally unique
**      tree.
**
**  History:
**      8-jun-86 (seputis)
**          initial creation from arl.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	26-mar-94 (ed)
**	    bug 49814 E_OP0397 consistency check
**	20-may-2001 (somsa01)
**	    Changed 'nat' to 'i4' due to cross-integration.
**	3-Jun-2009 (kschendel) b122118
**	    Cleanup: remove unused CCARTPROD flag, dead ojfilter code.
[@history_line@]...
**/

/*{
** Name: opn_cpartition	- generate next partition of set of relations
**
** Description:
**	Two types of partitionings occur, first we try to find a new sub-
**	partitioning within any partition where there are two or more children
**	(which therefore have identical structures).
**	This is a type 1 partition, (1 is the fourth parameter of call to
**	partition).  If there are none of these then we do new partitioning of
**	all subpartitions (type 0 partition).
**	Type 1 partitions are performed on opn_prb so that opn_pra
**	will not get messed up for the next type 0 partition call. after
**	a type 0 partition call, opn_pra is copied to opn_prb.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      nodep                           ptr to node which has a set of relations
**                                      upon which the next partition is
**                                      requested
**
** Outputs:
**	Returns:
**	    TRUE - if next valid partitioning is found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-jun-86 (seputis)
**          initial creation from cpartition in cparttion.c
[@history_line@]...
*/
static bool
opn_cpartition(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep)
{
    OPN_CHILD           numchild;		/* number of children at this
                                                ** node */
    numchild = nodep->opn_nchild;

    /* Quick heuristic for "leaf node join" - don't need opn_partition
    ** for these. */
    if (numchild == 2 && nodep->opn_nleaves == 2 && nodep->opn_pra[0] <
	nodep->opn_pra[1])
    {
	OPV_IVARS	tempvar;
	tempvar = nodep->opn_pra[0];
	nodep->opn_pra[0] = nodep->opn_prb[0] = nodep->opn_pra[1];
	nodep->opn_pra[1] = nodep->opn_prb[1] = tempvar;
	return(TRUE);
    }

    if (numchild != nodep->opn_npart)
    {	/* If the number of partitions is not the same as the number of
	** children then there is more than one child per partition.
	** Therefore, type 1 partitions need to be made.
	*/
	OPN_LEAVES	       *partsz;		/* ptr to number of elements
                                                ** in respective partition */
	OPN_CHILD	       *numsubtrees;    /* - number of subtrees in this
                                                ** particular partition
                                                ** - either 0, 1 or 2 for binary
                                                ** trees */
	partsz = nodep->opn_psz + nodep->opn_npart;
	for (numsubtrees = nodep->opn_cpeqc + nodep->opn_npart;
	     --numsubtrees >= nodep->opn_cpeqc;)
						/* for each partition */
	{
	    --partsz;				/* keep partsz pointing to the
						** corresponding element in
						** opn_psz */
	    numchild -= *numsubtrees;		/* make numchild be index into
						** opn_csz for the first child
						** in this partition */
	    if (*numsubtrees > 1)		/* if there is more than one
						** child in this equivalence
						** class (otherwise there is
						** only one partition and hence
						** no more partitionings) */
	    {
		OPN_ISTLEAVES		proffset; /* offset into the opn_prb
						** and opn_pra arrays for the
                                                ** beginning of the relations
                                                ** belonging to this partition
                                                */
		proffset = nodep->opn_pi[numchild]; /* offsets into partition
                                                ** arrays opn_prb and opn_pra */
		if (opn_partition(subquery, nodep->opn_prb + proffset, *partsz,
			nodep->opn_csz + numchild, *numsubtrees, TRUE))
		    return (TRUE);		/* if there is another
						** partitioning for this
						** equivalence class... */
		else
		    MEcopy ((PTR)(nodep->opn_pra + proffset),
			(*partsz)*sizeof(nodep->opn_pra[0]),
			(PTR)(nodep->opn_prb + proffset) );
	    }					/* otherwise, reset opn_prb to
						** original values */
	}
    }

    /* find type 1 partitioning */
    if (opn_partition (subquery, nodep->opn_pra, nodep->opn_nleaves,
	nodep->opn_psz, nodep->opn_npart, FALSE))
						/* if another partitioning for
						** the whole opn_pra array */
    {
	MEcopy ((PTR)nodep->opn_pra,
	    nodep->opn_nleaves * sizeof(nodep->opn_pra[0]),
	    (PTR)nodep->opn_prb);
						/* set up opn_prb for use in
						** later cpartition calls */
	return (TRUE);
    }
    else
	return (FALSE);				/* no more partitionings */
}

/*{
** Name: opn_rconnected	- reduce search space if relations are not connected
**
** Description:
**      This routine is called recursively when all the variables connected
**      to the selected var can be added to the connected map if they are in the
**	same partition
**
** Inputs:
**      var                             variable to add to the connected
**                                      map of relations
**      maxvar                          number of variables list
**
** Outputs:
**      connect                         ptr to map of all relations which
**                                      are currently connected
**                                      updated with all new variables that
**                                      "var" is connected to
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-jul-90 (seputis)
**          initial creation for b31778
[@history_line@]...
*/
static VOID
opn_rconnected(
	OPS_SUBQUERY	   *subquery,
	OPV_MVARS	   *connect,
	OPV_IVARS          var,
	OPV_IVARS	   *partition,
	OPV_IVARS	   maxvar)
{
    OPV_IVARS           *joinvar;    /* variable which will be tested against
                                    ** the current var for joinability
                                    */
    OPV_VARS            *var_ptr;   /* ptr to current var which will be used
                                    ** to attempt to include other vars in
                                    ** the connected set
                                    */
    OPV_IVARS		varcount;

    joinvar = partition;
    varcount = maxvar;
    var_ptr = subquery->ops_vars.opv_base->opv_rt[var];
    for ( ; varcount--; joinvar++)
    {
	if (*joinvar != var)	    /* skip current variable */
	{
	    if (var_ptr->opv_joinable.opv_bool[*joinvar]
		&&
		(!connect->opv_bool[*joinvar]) )
	    {   /* a new variable has been found which is joinable so use
		** recursion to find all variables which can be joined to it
		*/
		connect->opv_bool[*joinvar] = TRUE;
		opn_rconnected(subquery, connect, *joinvar, partition, maxvar);
	    }
	}
    }
}

/*{
** Name: opn_cartprod	- does the storage structure require a cart prod
**
** Description:
**      In order to check the case of several small tables being keyed into
**	a large table, this routine will check the joinability of a particular
**	set of relations to the parts of a multi-attribute key.  This may mean
**	that the small tables may need to be cart-prod together in order to use
**	the key of the large table.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      partition                       array of variables in the partition
**      maxvar                          number of variables in partition
**      keyedvar                        variable with multi-attibute key
**					to be checked against partition
**
** Outputs:
**      nonjoinable                     TRUE if this set of relations is not
**					joinable
**
**	Returns:
**	    TRUE - if partition is interesting due to keying possibility
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-dec-90 (seputis)
**          initial creation - as part of investigation of metaphor problems
**      18-mar-91 (seputis)
**          add trace point to avoid cartprod search space for
**	    keying purposes
**	13-nov-00 (hayke02)
**	    Keyvar marked as taking part in a key cart prod (OPV_VCARTPROD).
**	    This flag is then used in opn_jtuples() to ensure that reasonable
**	    join estimates are made. This change fixes bug 101399.
**	2-june-05 (inkdo01)
**	    Remove above change - flag was being set inconsistently and its
**	    later use led to excessive CP joins (with bad row counts).
[@history_line@]...
[@history_template@]...
*/
bool
opn_cartprod(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          *partition,
	OPV_IVARS          maxvar,
	OPV_IVARS	   keyedvar,
	bool		   *notjoinable)
{   /* keying can be used in this case, so that it may be
    ** more effectively to cart-prod some small relations
    ** in order to key into a large relation,
    ** - also do not look at storage structure if a TID
    ** join will be made */
    OPV_MVARS              connect; /* map of variables which
				** are connected to one another
				*/
    OPV_VARS	*varp;
    i4		key_count;	/* used to analyze each attribute
				    ** of a multi-attribute key */
    OPZ_IATTS	maxattr;	/* number of attributes in subquery */
    OPZ_AT	*abase;		/* base of array of joinop attributes */
    bool	first_time;
    varp = subquery->ops_vars.opv_base->opv_rt[keyedvar];
    *notjoinable = FALSE;
    abase = subquery->ops_attrs.opz_base;
    maxattr = subquery->ops_attrs.opz_av;
    MEfill( subquery->ops_vars.opv_rv*sizeof(connect.opv_bool[0]),
	(u_char) 0, (PTR) &connect ); /* each boolean in the partition
				** must be set for the partition to be
				** connected */
    first_time = FALSE;
    for (key_count = 0;
	key_count < varp->opv_mbf.opb_count;
	key_count++)
    {
	OPZ_BMATTS   *attrmap;	/* map of attributes contained
				** in this equivalence class */
	bool	    found;
	OPE_IEQCLS  eqcls;	/* equivalence class of an
				** attribute of the multi-attribute key */
	OPZ_IATTS   attr;	/* used to traverse the attributes
				** in the eqcls equilvalence class */
	OPV_IVARS   keyvar;	/* will contain a variable which is
				** in the partition and contains an
				** attribute which can be used for keying */

	found = FALSE;
	eqcls = abase->opz_attnums
	      [varp->opv_mbf.opb_kbase->opb_keyorder[key_count].opb_attno]->opz_equcls;
	attrmap = &subquery->ops_eclass.ope_base->ope_eqclist[eqcls]
	    ->ope_attrmap;
	for (attr = -1; !found && ((attr = BTnext((i4)attr, (char *)attrmap, (i4)maxattr))>=0);)
	{   /* search the attributes of the equivalence class
	    ** and determine if any variables in the partition
	    ** can supply attributes */
	    keyvar = abase->opz_attnums[attr]->opz_varnm;
	    if (keyvar >= 0)
	    {
		OPV_IVARS	*part1;
		OPV_IVARS	maxvar1;
		part1 = partition;
		for (maxvar1=maxvar; --maxvar1 >= 0; part1++)
		{
		    if (*part1 == keyvar)
		    {
			found = TRUE;
			break;
		    }
		}
	    }
	}
	if (found)
	{   /* variable found which can provide tuples for keying into
	    ** the keyedvar relation */
	    OPV_IVARS	    *part2;
	    OPV_IVARS	    maxvar2;
	    maxvar2 = maxvar;
	    if (!connect.opv_bool[keyvar])
	    {	/* this relation was not connected to the previous set
		** so a cartesean product will exist, but the key can be
		** used */
		if (first_time)
		{   /* set when a cartesean product is required in order
		    ** to fully use the key */
		    varp->opv_mask |= OPV_CARTPROD;
		}
		else
		    first_time = TRUE;
		connect.opv_bool[keyvar] = TRUE;	/* relation is connected to itself */
		opn_rconnected(subquery, &connect, keyvar, partition, maxvar); /* init the
					    ** connect array */
		for (part2 = partition; maxvar2--; part2++)
		    if (!connect.opv_bool[*part2])
			break;
		if (maxvar2 < 0)
		{
		    if ((varp->opv_grv->opv_relation->rdr_rel->tbl_storage_type != DB_HASH_STORE)
			||
			(key_count == (varp->opv_mbf.opb_count-1))
			)
			return(TRUE);	/* all relations are connected
					** - also make sure that if hash
					** is selected, then all attributes
					** are available */
		}
		else
		    *notjoinable = TRUE;
	    }
	}
	else
	{
	    /* FIXME check if the equivalence class is constant */
	    if (!subquery->ops_bfs.opb_bfeqc
		||
		!BTtest((i4)eqcls, (char *)subquery->ops_bfs.opb_bfeqc)
	       )
	    break;
	}
    }
    return(FALSE);	/* if the relations are not joinable from a previous
			** check, then they will continue not to be
			** joinable */
}

/*{
** Name: opn_connected	- test for connected set of relations
**
** Description:
**      In order to avoid recursing unnecessarily in opn_arl, this routine will test if
**	a partition is joinable before continuing
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      partition                       list of variables in partition
**      maxvar                          number of variables in partition > 1
**	keyedvar			relation which can be used for
**					multi-attribute keying
**					which may cause a cart prod
**
** Outputs:
**
**	Returns:
**	    bool - TRUE if partition is connected
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-jul-90  (seputis)
**          initial creation for b31778
**      20-dec-90 (seputis)
**          - added support for metaphor problem, use cart prod's if
**	    it would help multi-attribute keying
**	14-feb-91 (seputis)
**	    - fixed problem with aug-6 cart-prod hueristic
**	7-apr-94 (ed)
**	    - b59930 - allow cart prods if outer join semantics
**	    require this relation nesting
**	27-jul-98 (hayke02)
**	    Only test for cart prods (for multi-attribute keying) if we
**	    haven't made an 'index guess' in opn_tree() (OPS_GUESS set in
**	    ops_gmask). This prevents massive overestimation due to a
**	    combination of the cart prod heuristic passing and incomplete
**	    enumeration due to the 'index guess'. This change fixes bug
**	    88068.
[@history_template@]...
*/
static bool
opn_connected(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          *partition,
	OPV_IVARS          maxvar,
	OPV_IVARS	   keyedvar,
	bool		   tidjoin,
	OPV_BMVARS	   *ojvarmap)
{
    OPV_MVARS              connect; /* map of variables which
				** are connected to one another
				*/

    if ((keyedvar != OPV_NOVAR)
	&&
	!tidjoin)
    {
	OPV_VARS	*varp;
	varp = subquery->ops_vars.opv_base->opv_rt[keyedvar];
	if ((varp->opv_mask & OPV_CARTPROD)
	    &&
	    (	!subquery->ops_global->ops_cb->ops_check
		||
		!opt_strace( subquery->ops_global->ops_cb, OPT_F059_CARTPROD)/*
					    ** TRUE - if no cart products
					    ** should be used even if it helps
					    ** keying */
	    )
	    &&
	    (subquery->ops_global->ops_gmask & OPS_GUESS))

	{
	    bool	joinable;
	    if (opn_cartprod(subquery, partition, maxvar, keyedvar, &joinable))
		return(TRUE);
	    if (joinable && !ojvarmap)
		return(FALSE);  /* if the relations are not joinable from a previous
				** check, then they will continue not to be
				** joinable
				** - if outer joins exist make sure partition check
				** is executed below for nested cart prod outer joins
				*/
	}
	else if (subquery->ops_vars.opv_cart_prod
		&&
		(varp->opv_grv->opv_created == OPS_SELECT)
		)
	    return(TRUE);	/* given a subselect, treat it as a cartesean
				** product since, connectivity between correlated
				** attributes is not a valid joining technique */
    }

    if ((maxvar == 2) && !ojvarmap)
    {	/* test easy common case of 2 relations in the partition */
	return (subquery->ops_vars.opv_base->opv_rt[*partition]->opv_joinable.opv_bool[*(partition+1)]);
    }
    MEfill( subquery->ops_vars.opv_rv*sizeof(connect.opv_bool[0]),
	(u_char) 0, (PTR) &connect );
    connect.opv_bool[*partition] = TRUE;	/* relation is connected to itself */
    opn_rconnected(subquery, &connect, *partition, partition, maxvar); /* init the
				** connect array */
    for (partition++; --maxvar; partition++)
    {
	if (!connect.opv_bool[*partition])
	{
	    if(	ojvarmap
		&&
		BTtest((i4)*partition, (char *)ojvarmap))
	    {	/* a cart prod exists in the inner relations of the outer join
		** which must be allowed lower in the tree for correct
		** semantics so consider this relation joinable to anything */
	    }
	    else
		return(FALSE);	/* at least one relation was not connected */
	}
    }
    return(TRUE);		/* all relations are connected */
}

/*{
** Name: opn_tjrconnected	- are all TID joins connected
**
** Description:
**      In order to find whether a couple of TID joins can be placed in the
**	subtree of the given shape, a check is made to determine if the
**	TID joins are connected, by modifying the test so that base relations
**	are not considered connected to their respective indexes.  This will
**	determine the number of TID joins which require a separate subtree
**	to be available.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      29-apr-91 (seputis)
**          initial creation for b37233
**	1-apr-92 (seputis)
**	    - modified for bug 40687 - reduce CPU time required to
**	    discover a balanced tree is needed instead of an unbalanced
**	    tree when 10 relations which 10 indexes exist
[@history_template@]...
*/
static VOID
opn_tjrconnected(
	OPS_SUBQUERY	   *subquery,
	OPV_MVARS	   *connect,
	OPV_IVARS	   var,
	OPV_IVARS	   *partition,
	OPV_IVARS	   maxvar,
	OPV_BMVARS	   *tidjoins)
{
    OPV_IVARS           *joinvar;    /* variable which will be tested against
                                    ** the current var for joinability
                                    */
    OPV_VARS            *var_ptr;   /* ptr to current var which will be used
                                    ** to attempt to include other vars in
                                    ** the connected set
                                    */
    OPV_IVARS		varcount;
    OPV_IVARS		primary;

    connect->opv_bool[var] = TRUE;
    joinvar = partition;
    varcount = maxvar;
    var_ptr = subquery->ops_vars.opv_base->opv_rt[var];
    primary = var_ptr->opv_index.opv_poffset;
    if (primary >= 0)
	BTclear((i4)primary, (char *)tidjoins); /* FIXME - if several indexes
				    ** are used on the same relation, verify that there
				    ** is a subtree which can handle all of the indexes */
    for ( ; varcount--; joinvar++)
    {
	if ((*joinvar != var)	    /* skip current variable */
	    &&
	    (!connect->opv_bool[*joinvar])) /* do not look at variable twice, or stack
				    ** overflow will occur */
	{
	    if (var_ptr->opv_joinable.opv_bool[*joinvar]
		&&
		(   !BTtest((i4)(*joinvar), (char *)tidjoins) /* do not assume connectedness
				    ** thru base relation, but only thru
				    ** index, to determine if special tree
				    ** shape is required */
		)
		)
	    {   /* a new variable has been found which is joinable so use
		** recursion to find all variables which can be joined to it
		*/
		opn_tjrconnected(subquery, connect, *joinvar, partition, maxvar, tidjoins);
	    }
	}
    }
}

/*{
** Name: opn_bfork	- find minimum number of forks
**
** Description:
**      Test for best possible placement of indexes and base relations
**      so that the minimum number of forks are required in the subtree
**
** Inputs:
**      subquery                        ptr to subquery being analyzedd
**      partition                       list of variables to be placed
**      maxvar                          number of variables in partition
**
** Outputs:
**      tidmap                          map of base relations in partition
**
**	Returns:
**	    number of nodes which must have exactly two leaf nodes to
**	    accomodate a TID join
**	
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-jun-91 (seputis)
**          initial creation for bug 37233
**      25-aug-2006 (huazh01)
**          after the opn_tjrconnected() call, remove *current from 
**          rel_remaining if leftmostvar is not the same as *current.
**          This prevents an infinite loop if opn_bforks() is called
**          from opn_newenum() path. 
**          This fixes b116345.
**	26-feb-07 (hayke02)
**	    Change varcount test from '> 0' to '>= 0', as 0 is a valid
**	    varcount value. varcount will be 0 if we break from the preceding
**	    FOR loop at the last var. This prevents an infinite loop if we
**	    break with a varcount of 0. This change fixes bug 117273.
[@history_template@]...
*/
static i4
opn_bforks(
	OPS_SUBQUERY	   *subquery,
	OPV_IVARS	   *partition,
	OPV_IVARS	   maxvar,
	OPV_BMVARS	   *tidmap)
{
    OPV_BMVARS		rel_remaining;	/* bitmap of relations remaining
				** to be analyzed, i.e. those relations
				** which were not found in the first pass
				** of a new relation, this is needed since
				** depending on which relation is chosen to
				** be first, may reduce the number of forks
				*/
    i4			best_forks; /* the smallest number of forks required
				** for this set of relations and indexes */
    OPV_MVARS       connect;    /* map of variables which
				** are connected to one another
				*/
    bool	    found;	/* TRUE - if this is a relation
				** which has not been processed, and could
				** have special placement characteristics */
    OPV_RT	    *vbase;

    vbase = subquery->ops_vars.opv_base;
    MEcopy((PTR)tidmap, sizeof(rel_remaining), (PTR)&rel_remaining);
    best_forks = 0;
    do
    {
	OPV_IVARS	vars_left;
	OPV_IVARS	*current;
	OPV_VARS	*varp;

	MEfill(sizeof(connect), 0, (char *)&connect);
	vars_left = maxvar;
	found = FALSE;
	for (current = partition; vars_left--; current++)
	{
	    if (!connect.opv_bool[*current])
	    {   /* check if this is an index on one of the appropriate base relations
		** and check connectedness again */
		varp = subquery->ops_vars.opv_base->opv_rt[*current];
		if ((varp->opv_index.opv_poffset >= 0)
		    &&
		    (varp->opv_index.opv_poffset != *current)
		    &&
		    BTtest((i4)varp->opv_index.opv_poffset, (char *)&rel_remaining)
		    )
		{
		    found = TRUE;
		    best_forks++;   /* a fork is required to support the TID join just
				    ** found */
		    break;	    /* found secondary index and base relation pair
				    ** which would need at least one pair of leaf
				    ** nodes */
		}
	    }
	}
	if (found)
	{
	    OPV_MVARS       backlink; /* map of variables which
				** have been scanned in a backward search
				** in order to determine
				** an index to which nothing except the
				** base relation can be joined ... this will
				** allow a forward search to determine how
				** long a join chain can be made
				*/
	    OPV_BMVARS	    temp_tidmap; /*used during the backlink check */
	    OPV_IVARS	    leftmostvar;
	    OPV_BMVARS	    joinedvars;

	    MEcopy((PTR)tidmap, sizeof(temp_tidmap), (PTR)&temp_tidmap);
	    MEfill(sizeof(backlink), (u_char)0, (PTR)&backlink);
	    MEfill(sizeof(joinedvars), (u_char)0, (PTR)&joinedvars);
	    leftmostvar = *current;
	    for(;;)
	    {	/* search for starting point, i.e. a secondary index which
		** would be the left most relation of a highly unbalanced tree
		** and would include as many other secondary indexes in the same
		** unbalanced tree,... this is done by determining backlinks
		** until the left most secondary index is found */
		OPV_IVARS       *joinvar; /* variable which will be tested against
					** the current var for joinability */
		OPV_IVARS	varcount;
		OPV_IVARS	primary;

		joinvar = partition;
		primary = varp->opv_index.opv_poffset;
		backlink.opv_bool[leftmostvar] = TRUE;
		if (primary >= 0)
		{
		    backlink.opv_bool[primary] = TRUE;
		    BTclear((i4)primary, (char *)&temp_tidmap); /* FIXME - if several indexes
						** are used on the same relation, verify that there
						** is a subtree which can handle all of the indexes */
		}
		/* on first pass look for another tid join relation, which can join directly
		** to this index */
		for ( varcount = maxvar; varcount--; joinvar++)
		{
		    if (!backlink.opv_bool[*joinvar] /* do not look at variable twice, or stack
						** overflow will occur */
			&&
			varp->opv_joinable.opv_bool[*joinvar] /* is this variable connected
						** to the current variable */
			&&
			BTtest((i4)(*joinvar), (char *)&temp_tidmap) /* check for the
						** tid join variables before trying to
						** connect to other variables */
			)
			break;
		}
		if (varcount >= 0)
		{   /* found primary which requires tid join but can join directly
		    ** to the underlying index, ... find the underlying secondary
		    ** index for this relation and make this the new starting point */
		    leftmostvar = *joinvar;	/* this is the primary */
		    joinvar = partition;
		    for (varcount = maxvar; varcount--; joinvar++)
		    {
			varp = vbase->opv_rt[*joinvar];
			if ((varp->opv_index.opv_poffset == leftmostvar)
			    &&
			    (leftmostvar != (*joinvar)))
			    break;		/* found secondary index associated with
						** this variable */
		    }
		    if (varcount < 0)
			opx_error(E_OP04A6_SECINDEX); 	/* expected to find secondary index */
		    leftmostvar = *joinvar;	/* new index variable to use */
		    if (BTcount((char *)&temp_tidmap, (i4)subquery->ops_vars.opv_rv)
			<=1)
			return(best_forks);	/* this was the last tid join so
						** exit with the number of forks required */
		    MEfill((i4)sizeof(joinedvars), (u_char)0, (PTR)&joinedvars);
		    continue;			/* start again with new secondary index
						** variable which will provide a better
						** starting point */
		}
		/* consider any other relation which may serve as a bridge
		** to a primary which has a secondary in this partition */
		joinvar = partition;
		for ( varcount = maxvar; varcount--; joinvar++)
		{
		    if (!backlink.opv_bool[*joinvar] /* do not look at variable twice, or stack
						** overflow will occur */
			&&
			varp->opv_joinable.opv_bool[*joinvar] /* is this variable connected
						** to the current variable */
			)
			BTset((i4)(*joinvar), (char *)&joinedvars);
		}
		{
		    OPV_IVARS	    varno1;
		    varno1 = BTnext((i4)-1, (char *)&joinedvars, (i4)subquery->ops_vars.opv_rv);
		    if (varno1 < 0)
			break;			/* no more connected variables to analyze */
		    varp = vbase->opv_rt[varno1];
		    backlink.opv_bool[varno1] = TRUE;
		    BTclear((i4)varno1, (char *)&joinedvars);
		}
	    }
	    opn_tjrconnected(subquery, &connect, leftmostvar,
		partition, maxvar, &rel_remaining); /* find additional connectedness to
				    ** the secondary index which would allow joins to
				    ** the secondary without accessing the base relation */

            /* b116345 */
            if (leftmostvar != *current &&
                subquery->ops_vars.opv_base->opv_rt[*current]->opv_index.opv_poffset >= 0)
                BTclear(subquery->ops_vars.opv_base->opv_rt[*current]->opv_index.opv_poffset,
                        (char*)&rel_remaining);

	}
    } while (found);
    return(best_forks);
}

/*{
** Name: opn_tjconnected	- are all TID joins connected
**
** Description:
**      In order to find whether a couple of TID joins can be placed in the
**	subtree of the given shape, a check is made to determine if the
**	TID joins are connected, by modifying the test so that base relations
**	are not considered connected to their respective indexes.  This will
**	determine the number of TID joins which require a separate subtree
**	to be available.
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**      29-apr-91 (seputis)
**          initial creation for b37233
[@history_template@]...
*/
bool
opn_tjconnected(
	OPS_SUBQUERY	   *subquery,
	/* OPN_JTREE	   *nodep, */
	OPV_IVARS	   *partition,
	OPV_IVARS	   maxvar,
	OPV_BMVARS	   *tidmap,
	OPN_JTREE	   *childp)
{
    i4			forks;	/* number subtrees required which have
				** no children of exactly one leaf */
    forks = opn_bforks(subquery, partition, maxvar, tidmap);
    if (forks<=1)
	return(TRUE);		/* all relations are connected, and indexes are
				** joined to relations other than base relation,
				** and base relation does not need to be accessed
				** prior to accessing the index */
    {
	OPN_CTDSC	   *tree_structure;
	for(tree_structure = &childp->opn_sbstruct[0];
	    *tree_structure > 0; ++tree_structure)
	{	/* check the tree structure to make sure the number of forks is
	    ** appropriate, this can be done by counting the number of "2"'s in
	    ** the structure definition */
	    if ((*tree_structure) == 2)
		forks--;
	}
    }
    if (forks > 0)
    {
#if 0
	if ((nodep->opn_child[OPN_LEFT]->opn_jmask & OPN_JSUCCESS)
	    ||
	    (nodep->opn_child[OPN_RIGHT]->opn_jmask & OPN_JSUCCESS)
	    )
	    subquery->ops_global->ops_gmask |= OPS_JSKIPTREE; /* since the
				** current tree shape has failed once, there is
				** most likely no point in continuing */
#endif
	return(FALSE);
    }
    if ((childp->opn_jmask & OPN_JFORKS) /* the bitmap of relations which require
				** bushy trees has been set */
	&&
	(childp->opn_jmask & OPN_JREACHED) /* the subtree has been evaluated
				** once before unsuccessfully */
	)
    {	/* forks have already been required for this child, on a previous assignment
	** of relations, check if this assignment of forked relations has already
	** been tried and failed */
	if (!MEcmp((PTR)tidmap, (PTR)&childp->opn_tidplace, sizeof(*tidmap)))
	    return(FALSE);	/* if the same set of relations is going to be
				** tried in this subtree then exit if it failed
				** before */
				/* FIXME - can make this test more accurate if the
				** "distance" between the relations is calculated
				** and try this path if the distance gets smaller
				*/
    }
    return(TRUE);
}

/*{
** Name: opn_ntcheck	- check for allowable cart prod
**
** Description:
**      This routine will check whether reasonable conditions exist in
**	which a cart prod should be allowed to be considered in order
**	to allow duplicate semantics to be supported
**
** Inputs:
**      subquery                        ptr to subquery to be considered
**      rdpartition                     ptr to list of variables which contain
**					remove duplicate variables
**      rdmaxvar                        number of elements in rdpartition
**      kdpartition                     ptr to list of variables which contain
**					remove duplicate variables
**      kdmaxvar                        number of elements in rdpartition
**
** Outputs:
**
**	Returns:
**	    TRUE - if cart prod should be allowed in this case
**	Exceptions:
**
** Side Effects:
**
** History:
**	5-jun-91 (seputis)
**	    initial creation - to support duplicate semantics when relations
**		do not have tids
*/
static bool
opn_ntcheck(subquery, rdpartition, rdmaxvar, kdpartition, kdmaxvar)
OPS_SUBQUERY       *subquery;
OPV_IVARS          *rdpartition;
OPV_IVARS          rdmaxvar;
OPV_IVARS          *kdpartition;
OPV_IVARS          kdmaxvar;
{
    OPV_MVARS	    rdconnect;	/* map of variables which
				** are connected to one another
				*/
    i4		    part_count; /* number of non-connected partitions */
    OPV_IVARS	    nodup_var;

    MEfill( subquery->ops_vars.opv_rv*sizeof(rdconnect.opv_bool[0]),
	(u_char) 0, (PTR) &rdconnect );
    part_count=0;
    for (nodup_var = -1;
	(nodup_var = BTnext((i4)nodup_var, (char *)subquery->ops_remove_dups,
	    (i4)subquery->ops_vars.opv_prv))>=0;
	)
    {
	if (!rdconnect.opv_bool[nodup_var])
	{
	    rdconnect.opv_bool[nodup_var] = TRUE;
	    part_count++;
	    opn_rconnected(subquery, &rdconnect, nodup_var, rdpartition, rdmaxvar); /* init the
				** connect array */
	}
    }
    if (part_count > subquery->ops_pcount)
	return(FALSE);		/* should not have more partitions than
				** necessary, since each partition will cause
				** a cart prod */
    for (rdpartition++; --rdmaxvar; rdpartition++)
	if (!rdconnect.opv_bool[*rdpartition])
	    return(FALSE);	/* at least one relation was not connected,
				** so that an extra partition was added which
				** is not necessary */

    /* check that the relations which are not connected to the keep duplicate
    ** partition, are also not connected to any of the remove duplicate partition
    ** since then this would be a useless cart prod which can be avoided */
    {
	OPV_MVARS	kdconnect;	/* map of variables which
				    ** are connected to one another
				    */
	OPV_IVARS	kdup_count;
	OPV_IVARS	kdvar;
	MEfill( subquery->ops_vars.opv_rv*sizeof(kdconnect.opv_bool[0]),
	    (u_char) 0, (PTR) &kdconnect );
	for (kdup_count = kdmaxvar; --kdup_count >= 0;)
	{
	    /* mark all relations which are connected to a keep duplicate variable */
	    kdvar = kdpartition[kdup_count];
	    if (BTtest((i4)kdvar, (char *)subquery->ops_keep_dups)
		&&
		!kdconnect.opv_bool[kdvar])
	    {
		kdconnect.opv_bool[kdvar] = TRUE;
		opn_rconnected(subquery, &kdconnect, kdvar, kdpartition,
		    kdmaxvar); /* init the connect array */
	    }
	}
	for (kdup_count = kdmaxvar; --kdup_count >= 0;)
	{
	    /* mark all relations which are connected to a keep duplicate variable */
	    kdvar = kdpartition[kdup_count];
	    if (!kdconnect.opv_bool[kdvar])
	    {	/* found variable which is not connected to keep duplicate variables
		*/
		OPV_VARS	*varp;
		varp = subquery->ops_vars.opv_base->opv_rt[kdvar];
		for (nodup_var = subquery->ops_vars.opv_rv; --nodup_var >= 0;)
		{
		    /* if relation can be connected to remove duplicates partition
		    ** then this partition introduces an unnecessary cart
		    ** prod */
		    if (varp->opv_joinable.opv_bool[nodup_var]
			&&
			rdconnect.opv_bool[nodup_var])
			return(FALSE);
		}
	    }
	}
    }

    return(TRUE);		/* the expected number of partitions exist */
}

/*{
** Name: opl_ojverify	- check that outer join can be evaluated
**
** Description:
**      Check that the outer join can be entirely evaluated at this node.
**	Make sure appropriate attributes are available in the outer
**	subtree.
**
** Inputs:
**      subquery                        ptr to subquery to analyze
**      outerp                          ptr to outer join descriptor
**      vmap                            ptr to outer variable map
**
** Outputs:
**
**	Returns:
**	    TRUE - if outer join can be evaluated at this node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-may-93 (ed)
**          initial creation
**	25-jun-97 (inkdo01)
**	    Doesn't handle outervar replaced by a OPV_CINDEX secondary ix.
**	 4-oct-05 (hayke02)
**	    Added outerp. Use opl_onclause and opl_ovmap BTand'ed together
**	    to see if nested outer join query has outer var(s) in the vmap.
**	    Reject plans if this is not the case, as this can result in
**	    query plans that return incorrect results. This change fixes
**	    bugs 109370 and 114807, problems INGSRV 2043 and 3349.
**	23-mar-07 (hayke02)
**	    Modify the fix for bug 114807 so that we now check for an index
**	    substituting the base table entirely (OPV_CINDEX) when testing
**	    for ovar in vmap. This change fixes bug 117973.
**	 5-sep-07 (hayke02)
**	    Modify fix for 114807 and 117973 so that we now check for any
**	    secondary index on ovar, rather than just a OPV_CINDEX index. This
**	    allows query plans to pass which have a secondary index as the
**	    outer var followed by a TID join back to the base table. This
**	    change fixes bug 117305.
**      30-sep-2009 (huazh01)
**          For query of the form: select .. from t0, t1 left join s2 on
**          t1.col1 = s2.col1, t2 where ... s2.col1 is null or s2.col2 = ...
**          reject plans that attempts to solve the query by joining s2 with
**          tables that is not referenced on oj's ON clause. Otherwise, an
**          iftrue() node won't be able to get inserted and leads to wrong
**          result. (b122613)
[@history_template@]...
*/
bool
opl_ojverify(
	OPS_SUBQUERY	    *subquery,
	OPZ_BMATTS	    *attrmap,
	OPV_BMVARS	    *vmap,
	OPL_OUTER	    *outerp)
{
    OPZ_IATTS	    maxattr;
    OPZ_IATTS	    attr;
    OPZ_AT	    *abase;
    OPV_RT	    *vbase;
    OPV_IVARS	    maxvar, indx;
    OPB_IBF         bfno, bfIdx, maxbf;
    OPB_BOOLFACT    *bf;
    PST_QNODE       *nodePtr;
    OPB_BFT         *bfbase;
    OPE_IEQCLS      maxeqcls;
    OPV_IVARS       ovar;
    OPV_BMVARS      outeronmap;

    maxvar = subquery->ops_vars.opv_rv;
    maxattr = subquery->ops_attrs.opz_av;
    abase = subquery->ops_attrs.opz_base;
    vbase = subquery->ops_vars.opv_base;
    maxbf =  subquery->ops_bfs.opb_bv;
    maxeqcls = subquery->ops_eclass.ope_ev;
    bfbase = subquery->ops_bfs.opb_base;

    {
	MEcopy((PTR)outerp->opl_onclause, (u_i2)sizeof(outeronmap),
							(PTR)&outeronmap);
	BTand((i4)BITS_IN(outeronmap), (char *)outerp->opl_ovmap,
							(char *)&outeronmap);
	for (ovar=-1; (ovar=BTnext((i4)ovar, (char *)&outeronmap,
							(i4)maxvar))>=0;)
	{
	    if (!BTtest((i4)ovar, (char *)vmap))
	    {
		OPV_IVARS	i;
		OPV_VARS	*varp;
		bool		ojindex = FALSE;

		for (i = -1; (i = BTnext((i4)i, (char *)vmap,
					(i4)subquery->ops_vars.opv_rv)) >=0;)
		{
		    varp = subquery->ops_vars.opv_base->opv_rt[i];
		    if (ovar == varp->opv_index.opv_poffset)
		    {
			ojindex = TRUE;
			break;
		    }
		}
		if (!ojindex && subquery->ops_oj.opl_lv > 1)
		    return(FALSE);

                if (!ojindex) for (bfno = 0; bfno < maxbf; bfno += 1)
                {
                    bf = bfbase->opb_boolfact[bfno];
                    nodePtr = bf->opb_qnode;

                    if (nodePtr->pst_sym.pst_type == PST_OR &&
                        nodePtr->pst_left                   &&
                        nodePtr->pst_left->pst_sym.pst_type == PST_UOP &&
                        nodePtr->pst_left->pst_sym.pst_value.pst_s_op.pst_opno ==
                        subquery->ops_global->ops_cb->ops_server->opg_isnull)
                    {
                        for (bfIdx = -1;
                             (bfIdx = BTnext((i4)bfIdx, (char *)outerp->opl_bfmap,
                                          (i4)maxbf))>=0;)
                        {
                           if (BTsubset((char*)&(bfbase->opb_boolfact[bfIdx]->opb_eqcmap),
                              (char*)&(bfbase->opb_boolfact[bfno]->opb_eqcmap),
                              (i4)maxeqcls))
                           {
                              return(FALSE);
                           }
                        }
                   }
               }

	    }
	}
    }
    for (attr= -1; (attr = BTnext((i4)attr, (char *)attrmap, (i4)maxattr))>=0;)
    {
	OPV_BMVARS	tempvarmap;
	MEcopy((PTR)&abase->opz_attnums[attr]->opz_outervars,
	    sizeof(tempvarmap), (PTR)&tempvarmap);
	BTand((i4)BITS_IN(tempvarmap), (char *)vmap,
	    (char *)&tempvarmap);
	if (BTnext((i4)-1, (char *)&tempvarmap, (i4)maxvar) < 0)
	{
	    /* required var isn't in available outers. First check for
	    ** available secondary which is replacement for one of the
	    ** required outers. */
	    bool	gotone = FALSE;
	    if (subquery->ops_mask & OPS_CINDEX)
	     for (indx = subquery->ops_vars.opv_prv; !gotone && indx < maxvar;
							indx++)
	      if ((vbase->opv_rt[indx]->opv_mask & OPV_CINDEX) &&
		BTtest((i4)vbase->opv_rt[indx]->opv_index.opv_poffset,
		(PTR)&abase->opz_attnums[attr]->opz_outervars) &&
		BTtest((i4)indx, (PTR)vmap)) gotone = TRUE;
	    if (gotone) continue;	/* found a replacement ix for one
					** of the outervars */
	
	    return(FALSE);  /* attributes not sufficient to
			    ** evaluate outer join in this
			    ** subtree, so either full join is
			    ** needed or this plan needs to be
			    ** skipped */
	}
    }
    return(TRUE);
}

/*{
** Name: opl_ojboolplace	- placement of outer join boolean factors
**
** Description:
**      Determine the number of outer join boolean factors which can be
**      evaluated for this joinid, given the current set of relations
**      available.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      vmap                            set of relations available
**      outerp                          ptr to outer join descriptor
**
** Outputs:
**
**	Returns:
**	    number of boolean factors which can be evaluated
**	Exceptions:
**
** Side Effects:
**
** History:
**      1-jun-93 (ed)
**          initial creation
**   27-jan-95 (inkdo01)
**       qualify existence of opb_ojattr before calling ojverify (fixes b66511)
[@history_template@]...
*/
static OPL_IOUTER
opl_ojboolplace(
	OPS_SUBQUERY	    *subquery,
	OPV_BMVARS	    *vmap,
	OPL_OUTER	    *outerp)
{
    OPB_IBF	    ibf;		/* boolean factor index */
    OPL_IOUTER	    bool_count;
    OPB_IBF	    maxbf;
    OPB_BFT	    *bfbase;


    maxbf = subquery->ops_bfs.opb_bv;
    bfbase = subquery->ops_bfs.opb_base;
    bool_count = 0;
    for( ibf = -1; (ibf = BTnext((i4)ibf, (char *)outerp->opl_bfmap, (i4)maxbf))>=0;)
    {	/* for each outer join boolean factor check to see if
	** it can be evaluated in this set of relations */
        if (bfbase->opb_boolfact[ibf]->opb_mask & OPB_OJATTREXISTS &&
	    opl_ojverify(subquery, bfbase->opb_boolfact[ibf]->opb_ojattr, vmap, outerp))
	    bool_count++;
    }
    return(bool_count);
}

/*{
** Name: opl_cartprodcheck	- init maps for outer join cart prod checks
**
** Description:
**      Outer join relation placement rules conflict with cart prod
**	relation placement rules.  This routine will initialize maps
**	so that outer joins can be executed even if it means introducing
**	a cart prod into the plan.  The varmap being updated is the set of
**	variables which should be considered to be connected.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      outerp                          ptr to outer join descriptor
**
** Outputs:
**      cartprodpp                      ptr to ptr to varmap to update
**      cartprodp                       ptr to varmap to update
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
**      28-feb-94 (ed)
**          incorrect varmap dereference used
[@history_template@]...
*/
static VOID
opl_cartprodcheck(
	OPS_SUBQUERY	    *subquery,
	OPL_OUTER	    *outerp,
	OPV_BMVARS	    **cartprodpp,
	OPV_BMVARS	    *cartprodp)
{
    OPV_IVARS		maxvar;
    OPV_BMVARS		*updatemap;
    maxvar = subquery->ops_vars.opv_rv;
    if (outerp->opl_mask & OPL_BOJCARTPROD)
    	/* make special cart prod check for this outer join on the
	** right connectedness check */
	updatemap = outerp->opl_ojbvmap;
    else if (outerp->opl_mask & OPL_OJCARTPROD)
    	/* make special cart prod check for this outer join on the
	** right connectedness check */
	updatemap = outerp->opl_ojtotal;
    else
	return;
    if (!(*cartprodpp))
    {
	(*cartprodpp) = cartprodp;
	MEcopy((PTR)updatemap, sizeof(*cartprodp),
	    (PTR)cartprodp);
    }
    else
	BTor((i4)maxvar, (char *)updatemap, (char *)cartprodp);
}

/*{
** Name: opn_jintersect	- is one set of relations joinable to another
**
** Description:
**	Simple form of intersect which works on binary trees only and assumes
**	the existence of the "opv_joinable" structure.  We do not need to worry
**	about implicit or explicit attributes because if two relations are
**	joinable on an attribute which is implicit in one of the relations then
**	one of the relations is an INDEX, the other is its primary and they are
**	therefore joinable on another attribute (the indexed attribute) anyhow.
**
**	In case of cartesean products all relation placements are legal.  This
**      should be improved upon since the search space is very large otherwise.
**      It can be done simply by creating "equivalence classes" on the range
**      variables using "opv_joinable" as the operator which determines whether
**	two variable belong to the same equivalence class.  The idea is to
**      move these equivalence classes down the join tree (and not any subset)
**      until a node has only one equivalence class.  This keeps cartesean
**      products at the top of the tree.  FIXME
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      nodep                           ptr to node to test for correct
**                                      placement of the relations
**
** Outputs:
**	Returns:
**	    TRUE if the placement of the relations passes the hueristic
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-jun-86 (seputis)
**          initial creation from intersct2 in intersct2.c
**	12-apr-89 (seputis)
**          add gateway support for index TID joins
**	11-dec-89 (seputis)
**	    added optimization not to look at plans with secondary and
**	    base relation, in which only the secondary would provide
**	    the attributes by substitution
**	29-mar-90 (seputis)
**	    fix for b20632, do not perform useless TID join
**	15-may-90 (seputis)
**	    place check for timeout conditions - b21582 so that polling
**	    routine for unix can exit
**	18-may-90 (seputis)
**	    - b31644 - since a 12 way join with all tables joined to the same
**	    column is like a cart prod, the index placement hueristic is called
**	    too late, so eliminate invalid placements earlier in enumeration
**	20-jul-90 (seputis)
**	    - b31778 test for connected partition
**	6-aug-90 (seputis)
**	    - add hueristic for cart-prod to place all joins below all
**	    cart prods
**	29-apr-91 (seputis)
**	    - bug 37233 - skip over inappropriate tree shapes quickly
**	5-jun-91 (seputis)
**	    - fix STAR access violation, and local problem in which nested
**	    subselect with union views could result in a query plan not
**	    found error
**	7-aug-91 (seputis)
**	    - b39259 fix op0491 problem with fast commit tests,... occurs when an
**	    index is used explicitly, with no boolean factors or useful
**	    sort order existing, i.e. a useless tid join.
**	15-feb-93 (ed)
**	    - fix outer join placement bug 49322
**	09-apr-93 (vijay)
**	    - tidojid should be i2 and not bool.
**	21-may-93 (ed)
**	    - fix outer join problem, use correct base relation for tid join
**	11-feb-94 (ed)
**	    - remove redundant code
**	5-apr-94 (ed)
**	    - b59935 - E_OOP04AF consistency check, occurred due to secondary index
**	    in both left and right partitions
**      9-nov-94 (Dick)
**	    - b63997 - E_OP0491 (no QEP found): do not perform "connected" test
**	    if an inner cartprod join is under an outer join.  Normally, cartprod
**	    would be moved to top, but that is invalid for the outer join.
**	    Refer to new variable inner_cartprod for chg.
**	12-may-97 (hayke02)
**	    The fix for bug 63997 (see above) tries to prevent the "connected"
**	    test from being performed if an inner cartprod join is under an
**	    outer join (inner_cartprod is TRUE). However, because
**	    !inner_cartprod is OR'ed with !opv_cart_prod, we cannot guarantee
**	    that the test is bypassed (ie. if inner_cartprod is TRUE and
**	    opv_cart_prod is FALSE). We now AND !inner_cartprod with
**	    !opv_cart_prod (and the rest). This change fixes bug 81386.
**	30-jul-97 (hayke02)
**	    The fix for bug 81386 (see above) has been modified so that its
**	    effect is limited to OJ queries only. This prevents non OJ queries
**	    having CP nodes placed below join nodes, thus removing possible
**	    query estimation inaccuracies. This change fixes bug 84042.
**	28-jul-98 (hayke02)
**	    Added leftoj_cartprod, to indicate that a left join has been found
**	    and cart-prod placement heuristics should be ignored for this tree.
**	    This change fxies bug 86013.
**	19-nov-99 (inkdo01)
**	    Changes to support removal of EXsignal for exit processing.
**	17-jul-00 (hayke02)
**	    Added nested_inner_oj. Set TRUE if we have nested OJ's and
**	    at least one is an inner OJ which has been marked as a
**	    normal inner join by the change for SIR 94906 - cart-prod
**	    placement heuristics should be ignored for this tree. This
**	    change fixes bug 81386 (again).
**	18-dec-00 (hayke02)
**	    Return FALSE if the outer join TID join cannot be fully
**	    evaluated at this join node. This is a new fix for bug 95948
**	    - the first fix introduced bug 100286. This change therefore
**	    fixes bug 100286.
**	3-nov-00 (inkdo01)
**	    Replace OPN_CSTIMEOUT constant with cbf-based variable.
**	30-aug-01 (hayke02)
**	    If we have a 'not exists'/'not in' subselect trasnformed into
**	    an outer join (ops_smask & OPS_SUBSELOJ) then allow cart
**	    prod nodes below joins. This change fixes bug 105607.
**	18-oct-02 (inkdo01)
**	    Externalize for call from opn_newenum.
**	15-nov-02 (inkdo01)
**	    Alter full join tests for new enumeration heuristic.
**	13-feb-03 (hayke02)
**	    Check for OPV_OJINNERIDX in the outer join base var opv_gmask
**	    and reject this outer TID join plan if set and the outer join
**	    base var is the inner in the outer join. This change fixes bug
**	    109392, side effect of the fix for 107537.
**	    Return FALSE if the outer join TID join cannot be fully
**	    evaluated at this join node. This change fixes bug 100286.
**	 2-sep-03 (hayke02)
**	    Further refinement to the above change so that we allow QEPs
**	    with the TID OJ immediately following the inner index OJ but
**	    rejects QEPs with join node(s) between the TID OJ and the inner
**	    index OJ.
**	    This change fixes bugs 95948/110790 (wrong results from incorrectly
**	    placed TID OJ) and 100286 (poor performance when disallowing QEP
**	    with TID OJ after inner index OJ).
**	16-sep-03 (hayke02)
**	    Additional change to set OPN_OJINNERIDX in nodep->opn_jmask so
**	    that the inner index OJ placement is checked in opn_jmaps()
**	    where opn_child has already been set.
**	30-mar-2004 (hayke02)
**	    Call opn_exit() with a FALSE longjump.
**	27-jun-2004 (hayke02)
**	    Reject plans that have outer join nodes where on clause attributes
**	    are missing from the equivalence classes of the left and right
**	    child nodes, and the node vars are not a subset of, or equal to,
**	    the vars in the on clause. This change fixes problem INGSRV 2881,
**	    bug 112563.
**	12-sep-05 (hayke02)
**	    Remove boolean nested_inner_oj now that the fix for 94906 has
**	    been removed. This change fixes bugs 114912 and 115165.
**	1-dec-05 (inkdo01)
**	    Remove check for OPS_SUBSELOJ - original bug doesn't happen
**	    even without the check and the check permitted bad OJ plans.
**	10-mar-06 (hayke02)
**	    Set leftoj_cartprod TRUE if OPL_OOJCARTPROD is set in opl_mask.
**	    This will then allow cart prod nodes below left join nodes when
**	    the cart prod is the outer of the left join. This change fixes
**	    bug 115599.
**	24-feb-06 (hayke02)
**	   Limit the fix for bug 112563 to non-inner/folded joins. This
**	   change fixes bug 115501.
**	5-may-2008 (dougi)
**	    Support table procedures by rejecting orders with tproc node
**	    left of anything in opv_parmmap (parm exprs must be computable
**	    at "join" node).
**	23-Sep-2008 (kibro01) b120917
**	    Properly group the OPN_LAENUM check in the test for below-top-node
**	    cart-prods.  Otherwise greedy enumeration may find a cartprod it
**	    wants to put in, combine those elements, and then find it impossible
**	    to complete the plan, giving rise to OP04C0 or OP04C1.
**	12-Dec-2008 (kibro01) b119685/118962
**	    When evaluating a full outer join, the sides of it are evaluated
**	    first.  Each side uses a part of the ON clause, but does not have
**	    the complete ON clause, so allow the case where only a subset of
**	    either lhs or rhs is being evaluated at this point.
**	29-Dec-2008 (kibro01) b121442
**	    The opn_eqm values are not set up yet, so use the evarp variable 
**	    array directly to determine whether an outer join ON clause is 
**	    satisfied by the variables associated with this node.  opn_eqm
**	    is set up in opn_jmaps, called only after this function returns
**	    that the fragment is acceptable.
**      16-apr-2010 (huazh01)
**          if using greedy enumeration, reject plan if oj's bothmaps does
**          not cover all oj attribute. (b123082)
[@history_line@]...
*/
bool
opn_jintersect(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE	   *nodep,
	OPN_STATUS	   *sigstat)
{
    OPV_IVARS           *leftlimit;	    /* ptr to element of opn_prb which
                                            ** delimits the relations to be
                                            ** assigned to the left child */
    OPV_IVARS           *rightlimit;	    /* ptr to element of opn_prb which
                                            ** delimits the relations to be
                                            ** assigned to the right child */
    OPV_IVARS           *leftp;             /* ptr to current var from the left
                                            ** child being tested for
                                            ** joinability with a relation on
                                            ** the right side */
    OPV_IVARS           *rightp;	    /* ptr to current var from the right
                                            ** child being tested for
                                            ** joinability with a relation on
                                            ** the left side */
    OPV_RT              *vbase;		    /* ptr to array of ptrs to joinop
                                            ** range variables */
    bool		tidjoin;	    /* TRUE - if a TID join will be made
					    ** at this node */
    OPV_BMVARS		leftmap;
    bool		leftvalid;
    OPV_BMVARS		rightmap;
    bool		rightvalid;
    bool		ntcheck;	    /* TRUE - if cart products must be
					    ** allowed  in order to support
					    ** duplicate semantics */
    OPV_IVARS		*rdvars;	    /* ptr to partition which contains
					    ** remove duplicate variables */
    OPV_IVARS		rdlimit;	    /* number of relations in partition */
    OPV_IVARS		*kdvars;	    /* ptr to partition which contains
					    ** keep duplicate variables */
    OPV_IVARS		kdlimit;	    /* number of relations in partition */
    bool		inner_cartprod;     /* inner join cartprod below outer join */
    bool		fulloj_cartprod;    /* set TRUE if full join and found and
					    ** cart-prod placement hueristics should
					    ** be ignored for thistree */
    bool		leftoj_cartprod;    /* set TRUE if left join found and
					    ** cart-prod placement heuristics
					    ** should be ignored for this tree
					    */
    OPV_IVARS		ojbasevno;	    /* base relation of tid join to be
					    ** performed here */
    OPV_BMVARS		*leftojp;	    /* NULL - indicates no checking
					    ** not NULL - indicates map of outer joins
					    ** which should be considered to be connected
					    ** for purposes of relation placement since
					    ** there is a cart prod in the outer join */
    OPV_BMVARS		*rightojp;	    /* NULL - indicates no checking
					    ** not NULL - indicates map of outer joins
					    ** which should be considered to be connected
					    ** for purposes of relation placement since
					    ** there is a cart prod in the outer join */
    OPV_BMVARS		leftcartprodmap;    /* used by leftojp if necessary */
    OPV_BMVARS		rightcartprodmap;   /* used by rightojp if necessar */
    OPV_IVARS		maxvar;

    *sigstat = OPN_SIGOK;
    if (subquery->ops_global->ops_estate.opn_cscount++ >
		subquery->ops_global->ops_cb->ops_alter.ops_cstimeout)
    {
	subquery->ops_global->ops_estate.opn_cscount = 0;
	*sigstat = opx_cinter(subquery);    /* exit if interrupt or cntrl C
					    ** detected */
	if (*sigstat == OPN_SIGOK && opn_timeout(subquery)) /* check for timeout conditions */
	{
	    opn_exit(subquery, FALSE);
	    *sigstat = OPN_SIGEXIT;
	}
	if (*sigstat == OPN_SIGEXIT) return(FALSE);
    }

    leftlimit = nodep->opn_prb + nodep->opn_csz[OPN_LEFT]; /* get ptr to
					    ** element of array which delimits
					    ** the relations to be assigned to
					    ** the left child */
    rightlimit = leftlimit + nodep->opn_csz[OPN_RIGHT]; /* get ptr to element
					    ** of array which delimits the
					    ** the relations to be assigned
					    ** to the right child */
    vbase = subquery->ops_vars.opv_base;    /* ptr to base of array of ptrs
					    ** to joinop range variable elements
					    */
    maxvar = subquery->ops_vars.opv_rv;
    tidjoin = FALSE;
    leftvalid = FALSE;
    rightvalid = FALSE;

    if ((subquery->ops_mask & OPS_CPLACEMENT) || 
	(subquery->ops_mask2 & OPS_TPROC))
    {	/* check that if an index is placed in one branch, and the base relation
	** is placed in the other, that the base relation branch is a single node
	** or else a TID join cannot be performed */
	OPV_IVARS	*index1p;
	OPV_BMVARS	tjmap;		    /* only initialized if tidcheck is TRUE
					    ** contains map of base relations
					    ** which are involved in a TID join
					    ** in this subtree */
	bool		tidcheck;	    /* TRUE - if more than one TID join
					    ** exists */
	OPV_IVARS	tidrelation;	    /* set to variable number of first
					    ** TID join found */

	tidcheck = FALSE;
	tidrelation = OPV_NOVAR;
	for (index1p = nodep->opn_prb; index1p < rightlimit; index1p++)
	{
	    if (vbase->opv_rt[*index1p]->opv_mask & OPV_CPLACEMENT)
	    {	/* found an index which requires a TID join */
		OPV_IVARS	*lower;	    /* lower bound of partition
		    			    ** which contains index */
		OPV_IVARS	*upper;	    /* upper bound of partition
					    ** which contains the index */
		OPV_IVARS	*blower;    /* lower bound of partition
					    ** which does not contain the index
					    ** but could contain the base table */
		OPV_IVARS	*bupper;    /* upper bound of partition
					    ** which does not contain the index
					    ** but could contain the base table */
		bool		found;	    /* TRUE - if base relation is in the
					    ** same partition as the index */
		OPV_IVARS           basevno; /* base var of index, valid only if
                                            ** tidjoin boolean is true */

		basevno = vbase->opv_rt[*index1p]->opv_index.opv_poffset;
		if (index1p < leftlimit)
		{
		    lower = nodep->opn_prb;
		    upper = leftlimit;
		    blower = leftlimit;
		    bupper = rightlimit;
		}
		else
		{
		    lower = leftlimit;
		    upper = rightlimit;
		    blower = nodep->opn_prb;
		    bupper = leftlimit;
		}
		found = FALSE;
		{
		    OPV_IVARS	    *searchp;
		    for (searchp = lower ;searchp != upper; searchp++)
		    {
			if ((*searchp) == basevno)
			{
			    found = TRUE;
			    break;
			}
		    }
		}
		if (found)
		{   /* found TID join, check that another TID join does
		    ** not exist on this side of the tree, or if it
		    ** does, that a join exists on the indexed column
		    ** which is connected to this TID join */
		    OPV_VARS	    *indexvarp;
		    if (tidrelation == OPV_NOVAR)
			tidrelation = basevno;
		    else if (tidcheck)
			BTset((i4)basevno, (char *)&tjmap); /* map has
					    ** already been initialized
					    */
		    else		
		    {	/* more than one relation with TID joins in this
			** partition so need to check for tree shape */
			tidcheck = TRUE;
			MEfill(sizeof(tjmap), (u_char)0, (PTR)&tjmap);
			BTset((i4)tidrelation, (char *)&tjmap);
			BTset((i4)basevno, (char *)&tjmap);
		    }
		    /* check ro3 semantics, i.e. replacement for opn_ro3 */
		    indexvarp = vbase->opv_rt[*index1p];
		    if ((indexvarp->opv_mbf.opb_bfcount == 0)
			&&
			(!(indexvarp->opv_mask & OPV_SPATF))
			&&
			(   (subquery->ops_global->ops_cb->ops_alter.
				ops_sortmax < 0.0)
			    ||
			    (indexvarp->opv_tcop->opo_storage != DB_SORT_STORE)
			)
                        &&
                        (
                            ((*index1p) >= subquery->ops_vars.opv_prv) /* if an
                                                ** explicit referenced index then
                                                ** avoid this check since otherwise
                                                ** a no query plan found error may
                                                ** result */
                        )
                        )
		    {	/* check that this TID join can involve a relation
			** which will restrict the secondary or else the
			** TID join is useless */
			OPV_IVARS	*index3p;
			for (index3p = lower; index3p != upper; index3p++)
			{
			    OPV_VARS	    *pvarp;
			    pvarp = vbase->opv_rt[*index3p];
			    if ((pvarp->opv_index.opv_poffset != basevno)
				/* relation found which is not an index
				** or the base relation */
				&&
				indexvarp->opv_joinable.opv_bool[*index3p]
						/* index can be used to join
						** to the relation, so scanning
						** the index will not be useless */
				)
				break;
			}
			if (index3p == upper)
			    return(FALSE);	/* skip this part of search
						** space if an index is being
						** scanned */

		    }
		}
		else
		{   /* if base relation is in the other partition
		    ** then check that this is a single leaf partition
		    ** so that a TID join is possible */

		    i4	    leaf_count;
		    leaf_count = 0;
		    for (;blower < bupper; blower++)
		    {
			if ((*blower) == basevno)
			{
			    if ((leaf_count != 0)
				||
				((blower + 1) != bupper)
				)
				return(FALSE); /* this partitioning cannot
					    ** be useful since a TID join
					    ** cannot be made to the base
					    ** relation */
			    else
			    {
				/* check that the equivalence of ro3 is
				** true here, rather than making another
				** traversal of the qep tree */
				tidjoin = TRUE;
				break;
			    }
			}
			leaf_count++;
		    }
		    if (tidjoin)
		    {   /* check that the opn_ro3 test is satisfied, i.e. all
			** indexes have boolean factors associated with them
			** or are joined to another relation to provide
			** restrictivity */
			OPV_IVARS	*index2p;
			bool		nobooleans;

			nobooleans = FALSE;
			for (index2p = lower; index2p != upper; index2p++)
			{
			    OPV_VARS	    *ivarp;
			    ivarp = vbase->opv_rt[*index2p];
			    if (ivarp->opv_index.opv_poffset == basevno)
			    {   /* secondary index found which will be used
				** for TID join */
				if ((ivarp->opv_mbf.opb_bfcount == 0)
				    &&
				    (!(ivarp->opv_mask & OPV_SPATF))
				    &&
				    (   (subquery->ops_global->ops_cb->ops_alter.
					    ops_sortmax < 0.0)
					||
					(ivarp->opv_tcop->opo_storage != DB_SORT_STORE)
				    )
				    &&
				    (	subquery->ops_bestco
					||
					(subquery->ops_vars.opv_prv <= (*index2p))
				    )
				    )
				    nobooleans = TRUE; /* found an index with no
						** boolean factors, so do not use
						** this relation assignment unless
						** another non-index is found, or
						** if sort space is an issue and
						** this relation provides ordering
						** - also allow this case if an
						** explicitly referenced index is
						** found, or a no query plan error
						** may result */
			    }
			    else
				break;		/* non-index found in subtree
						** so ro3 test passes */
			}
			if (nobooleans && (index2p == upper))
			    return(FALSE);	/* skip this part of search
						** space if an index is being
						** scanned */
			ojbasevno = basevno;	/* save base relation for this
						** tid join */
		    }
		}
	    }
	    if (index1p == leftlimit)
	    {
		if (tidcheck)
		{
		    leftvalid = TRUE;
		    MEcopy((PTR)&tjmap, sizeof(leftmap), (PTR)&leftmap);
		    /* check that the appropriate subtree exists in the left side */
                    if (!opn_tjconnected(subquery, /* nodep, */ nodep->opn_prb, nodep->opn_csz[OPN_LEFT], &tjmap,
                        nodep->opn_child[OPN_LEFT])
                        )
			return(FALSE);
		}
		tidcheck = FALSE;
		tidrelation = OPV_NOVAR;

	    }

	    /* Check for table procedures. */
	    if (FALSE && (vbase->opv_rt[*index1p]->opv_mask & OPV_LTPROC) &&
		vbase->opv_rt[*index1p]->opv_parmmap)
	    {
		i4	i;
		OPV_BMVARS *mapptr = vbase->opv_rt[*index1p]->opv_parmmap;
		OPV_IVARS *lower, *upper, *searchp;
		bool	found = TRUE;

		/* Verify that table procedures can be computed here. */
		if (index1p < leftlimit)
		{
		    lower = nodep->opn_prb;
		    upper = leftlimit;
		}
		else
		{
		    lower = leftlimit;
		    upper = rightlimit;
		}

		for (i = -1; (i = BTnext(i, (char *)mapptr, maxvar)) >= 0; )
		{
		    for (searchp = lower, found = FALSE;
				!found && searchp != upper; 
				searchp++)
		     if (i == (*searchp))
			found = TRUE;
		}

		if (!found) 
		    return(FALSE);		/* ref'd VAR not left of TPROC */
	    }

	}
	if (tidcheck)
	{
	    leftvalid = TRUE;
	    MEcopy((PTR)&tjmap, sizeof(rightmap), (PTR)&rightmap);
	    /* check that the appropriate subtree exists in the right side */
            if (!opn_tjconnected(subquery, /* nodep, */ leftlimit, nodep->opn_csz[OPN_RIGHT], &tjmap,
                    nodep->opn_child[OPN_RIGHT]))
		return(FALSE);
	}
    }
    if (subquery->ops_mask & OPS_CINDEX)
    {	/* check that secondary index/base relation not considered when
	** all attributes can be provided by secondary */
	OPV_IVARS           *indexp;	    /* ptr to current var to test for
					    ** index */
	for (indexp = nodep->opn_prb; indexp < rightlimit; indexp++)
	{
	    /* check if there is a relation in the left side which requires
            ** duplicates to be removed */
	    if (vbase->opv_rt[*indexp]->opv_mask & OPV_CINDEX)
	    {	/* found an index which can supply all the attributes
		** required by the query */
		OPV_IVARS	basevar;
		OPV_IVARS   *basep;	    /* ptr to current var to test for
					    ** base */
		basevar = vbase->opv_rt[*indexp]->opv_index.opv_poffset;
		for (basep = nodep->opn_prb; basep < rightlimit; basep++)
		{
		    if ((*basep) == basevar)
			return(FALSE);	    /* this partitioning cannot be useful
					    ** since it incorporates a useless TID
					    ** join, FIXME, can enhance the tests
					    ** to check for sets of indexes */
		}
	    }
	}

    }

    inner_cartprod  = FALSE;
    fulloj_cartprod = FALSE;
    leftoj_cartprod = FALSE;
    leftojp = NULL;
    rightojp = NULL;
    if (subquery->ops_oj.opl_base)
    {	/* check for legal placement of relations for outer join support
	** - if we are to consider a partitioning in which two inner relations
	** are to be placed into separate partitions, then check that all
	** the relations are inner relations, until the "filter code" has been
	** completed in OPC */
	OPV_BMVARS	    ojleftmap;	/* map of variables which are located
					** in the left partition */
	OPV_BMVARS	    ojrightmap;	/* map of variables which are located
					** in the right partition */
	OPV_BMVARS	    bothmaps;	/* map of all variables in this
					** partition */
	OPV_BMVARS	    prim_bothmaps;
	OPL_IOUTER	    ojid;	/* current outer join ID being analyzed */
	OPL_OJT		    *lbase;	/* ptr to base of outer join descriptor
					** table */
	bool		    joinid_found;   /* TRUE if one outer join id found */
	OPE_BMEQCLS	    joinmap;	/* map of equivalence classes used in join
					** at this node */
	OPL_IOUTER	    tidojid;	/* set to most deeply nested outer join id
					** which still contains the base relation
					** of the TID join as an inner */
	OPE_IEQCLS	    maxeqcls;
	OPL_IOUTER	    ojid_at_node;
	bool		    fullojid_found; /* TRUE if full join exists at this
					** node */
	OPV_BMVARS	    *cartprodp;
	OPV_BMVARS	    **cartprodpp;
	OPV_IVARS	    ojvno;
	maxeqcls = subquery->ops_eclass.ope_ev;
	nodep->opn_ojid = OPL_NOOUTER; /* the outer join for this set of relations
					** must be evaluated at this node */
	joinid_found = FALSE;
	MEfill(sizeof(ojleftmap), (u_char)0, (PTR)&ojleftmap);
	for (leftp = nodep->opn_prb; leftp < leftlimit; leftp++)
	    /* get a bitmap of the left partition */
	    BTset((i4)*leftp, (char *)&ojleftmap);
	MEfill(sizeof(ojrightmap), (u_char)0, (PTR)&ojrightmap);
	for (rightp = leftlimit; rightp < rightlimit; rightp++)
	    /* get a bitmap of the right partition */
	    BTset((i4)*rightp, (char *)&ojrightmap);
	MEcopy((PTR)&ojleftmap, sizeof(bothmaps), (PTR)&bothmaps);
	BTor((i4)maxvar, (char *)&ojrightmap, (char *)&bothmaps); /*
					** init var bitmap for this node */
	MEcopy((PTR)&bothmaps, sizeof(bothmaps), (PTR)&prim_bothmaps);
	for (ojvno = -1; (ojvno = BTnext((i4)ojvno, (char *)&bothmaps,
							(i4)maxvar)) >= 0;)
	{
	    OPV_VARS	*ojvarp = vbase->opv_rt[ojvno];

	    if (ojvarp->opv_grv->opv_relation &&
		(ojvarp->opv_grv->opv_relation->rdr_rel->tbl_id.db_tab_index
								    != 0) &&
		!(ojvarp->opv_mask & OPV_EXPLICIT_IDX))
	    {
		BTclear((i4)ojvno, (char *)&prim_bothmaps);
		BTset((i4)ojvarp->opv_index.opv_poffset,(char *)&prim_bothmaps);
	    }
	}
	lbase = subquery->ops_oj.opl_base;
	tidojid = OPL_NOOUTER;
	ojid_at_node = OPL_NOOUTER;
	fullojid_found = FALSE;
	for (ojid = subquery->ops_oj.opl_lv; (--ojid) >= 0;)
	{   /* look at placement of each outer join */
	    OPL_OUTER	    *outerp;	/* current outer join descriptor being
					** analyzed */
	    OPL_OJTYPE	    ojtype;	/* type of outer join node */
	    OPV_BMVARS	    *outerchildp; /* ptr to outer child for this outer
					** join (tuples preserved) */
	    OPV_BMVARS	    *innerchildp; /* ptr to inner child for this outer
					** join (tuples eliminated) */
	    OPV_BMVARS	    *minoj_vmap; /* map of vars which must be in the inner
					** child for this outer join */
	    OPV_BMVARS	    temp_minoj_vmap; /* pointed to by minoj_vmap if
					** a partial outer join is being evaluated */
	    outerp = lbase->opl_ojt[ojid];
	    ojtype = outerp->opl_type;
	    if (ojtype == OPL_FOLDEDJOIN &&
		(outerp->opl_mask & (OPL_OJCARTPROD | OPL_BOJCARTPROD)))
		inner_cartprod = TRUE;
	    if (ojtype == OPL_LEFTJOIN &&
		(outerp->opl_mask & (OPL_OJCARTPROD | OPL_OOJCARTPROD |
							    OPL_BOJCARTPROD)))
		leftoj_cartprod = TRUE;
	    if ((ojtype != OPL_LEFTJOIN) &&
		(ojtype != OPL_FULLJOIN) &&
		(ojtype != OPL_INNERJOIN))
		continue;		/* inner joins may exist due to view handling and full
					** joins */
	    if (outerp->opl_mask & OPL_FULLOJFOUND)
		continue;		/* full joins cannot be split unless code
					** below is changed */
	    MEcopy((PTR)outerp->opl_minojmap, sizeof(temp_minoj_vmap),
		(PTR)&temp_minoj_vmap);
	    BTand((i4)maxvar, (char *)&bothmaps, (char *)&temp_minoj_vmap); /*
					** calculate the set of relations which
					** must be in the inner tree */
	    if (BTnext((i4)-1, (char *)&temp_minoj_vmap, (i4)maxvar) <0)
		continue;		/* no inner relations in this subtree so
					** outer join does not apply */
	    minoj_vmap = &temp_minoj_vmap; /* map of variables, one of which
					** must be included in the inner set of
					** relations, if the outer join is to
					** be evaluated at this node */
	    if ((ojtype == OPL_LEFTJOIN)
		&&
		BTsubset((char *)&bothmaps, (char *)outerp->opl_maxojmap, (i4)maxvar))
	    {	/* left join cannot be evaluated if only inner relations exist so it
		** must have been evaluated in another part of tree */
		if (!outerp->opl_nodep && !subquery->ops_mask & OPS_LAENUM)
					/* outer join must have been placed at parent
					** node */
		    /* if any minoj relations exist then an error has occurred */
		    opx_error(E_OP04B4_PLACEOUTERJOIN);
		continue;		/* only inner relations in this
					** subtree so outer join has already
					** been entirely evaluated, since opl_maxojmap
					** contains all potential inner relations */
	    }
	    if (outerp->opl_type != OPL_INNERJOIN
		&&
		outerp->opl_type != OPL_FOLDEDJOIN
		&&
		!BTsubset((char *)outerp->opl_onclause, (char *)&prim_bothmaps,
								    (i4)maxvar)
/* Don't reject this plan on the grounds of it not including both halves of an
** outer join if everything within it is contained on one side (it doesn't
** matter which) of the join - that simply means it is a series of inner joins
** on one side, which can perfectly well be joined together.
** (kibro01) b119685/118962 
*/
		&&
		((outerp->opl_p1 == NULL) ||
		 (!BTsubset((char *)&prim_bothmaps, (char *)outerp->opl_p1,
						 (i4)maxvar)))
		&&
		((outerp->opl_p2 == NULL) ||
		 (!BTsubset((char *)&prim_bothmaps, (char *)outerp->opl_p2,
						 (i4)maxvar)))
	       )
	    {
		OPZ_IATTS	ojattr;
		OPZ_IATTS	maxattr = subquery->ops_attrs.opz_av;

		/* Use equivalence class list from the attnums array to confirm that
		** this outer join clause is satisfied, since we haven't got opn_eqm
		** set up yet (it is set up in opn_jmaps, called later)
		** (kibro01) b121442
		*/
		
		for (ojattr = -1; (ojattr = BTnext((i4)ojattr,
				(char *)outerp->opl_ojattr, (i4)maxattr))>=0;)
		{
		    bool found_left = FALSE, found_right = FALSE;
		    i4 eq_to_test = 
			(i4)subquery->ops_attrs.opz_base->opz_attnums[ojattr]->opz_equcls;
		    i4 varloop;

		    for (varloop = -1;
			 (varloop=BTnext(varloop,(char*)&ojleftmap,(i4)maxvar))>=0;)
		    {
			if (BTtest(eq_to_test,(PTR)&subquery->ops_vars.opv_base->
				opv_rt[varloop]->opv_maps.opo_eqcmap))
			{
			    found_left=TRUE;
			    break;
			}
		    }

		    for (varloop = -1;
			 (varloop=BTnext(varloop,(char*)&ojrightmap,(i4)maxvar))>=0;)
		    {
			if (BTtest(eq_to_test,(PTR)&subquery->ops_vars.opv_base->
				opv_rt[varloop]->opv_maps.opo_eqcmap))
			{
			    found_right=TRUE;
			    break;
			}
		    }

		    if (!found_left || !found_right)
			return (FALSE);
		}
	    }
	    if (BTsubset((char *)minoj_vmap, (char *)&ojleftmap, (i4)maxvar))
	    {	/* significant inner relations are in the left branch */
		outerchildp = &ojrightmap;
		innerchildp = &ojleftmap;
		cartprodpp =  &leftojp;	/* in case of cart prod conflict with outer joins,
					** allow cart prod to go thru or "no query plan found
					** errors" will occur */
		cartprodp = &leftcartprodmap;
	    }
	    else if (BTsubset((char *)minoj_vmap, (char *)&ojrightmap, (i4)maxvar))
	    {	/* significant inner relations are in the right branch */
		outerchildp = &ojleftmap;
		innerchildp = &ojrightmap;
		cartprodpp =  &rightojp; /* in case of cart prod conflict with outer joins,
					** allow cart prod to go thru or "no query plan found
					** errors" will occur */
		cartprodp = &rightcartprodmap;
	    }
	    else
	    {	/* inner relations not entirely in left or right child so
		** a split occurs at this node which means that
		** the outer join should occur at this node */
		/* cartprodpp, cartprodp, does not need to be set here
		** since inner relations are split and if a cart prod exists
		** it can be skipped since this orientation is different from
		** the original specification of the user, i.e. the user
		** did not split the relations to form two outer joins
		** - even though it may be possible to devise a more complex
		** algorithm to look a more cases, it is too rare to be
		** worthwhile */
		if (ojid_at_node != OPL_NOOUTER)
		{
		    OPL_OUTER	*prevoj = lbase->opl_ojt[ojid_at_node];

		    if (!(subquery->ops_mask & OPS_LAENUM))
			return(FALSE);  /* two outer joins cannot be evaluated
				** at the same node,  FIXME this can be
				** relaxed in some cases e.g. when both
				** ojid have no boolean factors and one
				** set of joining equivalence classes
				** are a subset of the other, then with
				** careful handling of special equivalence
				** classes this can be done, or it can be
				** done in general with added OPC operators
				** which evaluate outer join qualifications
				** separately and reset appropriate special
				** eqcls separately */

		    /* If this is the new enumeration technique, we may simply
		    ** have already found an OJ that passes the tests to date.
		    ** The OJ whose opl_minojmap is the larger should then be
		    ** picked (one should be a proper subset of the other - we
		    ** want the one that most closely matches the current
		    ** node. */
		    if (BTsubset((char *)outerp->opl_minojmap,
			(char *)prevoj->opl_minojmap, maxvar))
		     continue;	/* current is subset, just skip it */
		    /* Else drop through and reset to current OJ. */
		}

		ojid_at_node = ojid; /* found outer join to evaluate at this node
				** but still need to check other outer joins
				** to make sure there are no conflicts, since
				** 2 outer join IDs cannot be evaluated at
				** the same node */
		if (BTsubset((char *)&ojleftmap, (char *)outerp->opl_maxojmap,
			(i4)maxvar))
		{   /* inner relations are on left side so that
		    ** right side must have outer relations */
		    outerchildp = &ojrightmap;
		    innerchildp = &ojleftmap;
		}
		else if (BTsubset((char *)&ojrightmap, (char *)outerp->opl_maxojmap,
		    (i4)maxvar))
		{   /* inner relations are on right side so that
		    ** left side must have outer relations */
		    outerchildp = &ojleftmap;
		    innerchildp = &ojrightmap;
		}
		else
		    return(FALSE); /* both sides have outer relations so that
				** a full join will be needed to preserve
				** necessary tuples,... avoid a full
				** join since it is rare that this
				** would be a reasonable plan */
#ifdef xDEBUG
		if (!outerp->opl_nodep	/* first time outer join evaluated */
		    &&
/*		    (ojtype == OPL_LEFTJOIN) */
		    outerp->opl_ojattr
		    && 
		    !opl_ojverify(subquery, outerp->opl_ojattr, &bothmaps, outerp))
		{
/* 		    if (nodep != subquery->ops_global->ops_estate.opn_sroot)*/
			opx_error(E_OP04B4_PLACEOUTERJOIN); /* expecting all necessary
				    ** equivalence classes available */
/*		    else
			return(FALSE);*/	/* if at root then this initial
				    ** partitioning cannot be used
				    ** since this outer join cannot be evaluated
				    */
		}
#endif
		/* tid joins are allowed at any level since
		** duplicate semantics can be maintained by
		** top sort nodes */
		if (tidjoin)
		{
		    OPV_VARS	*ojbasevarp = vbase->opv_rt[ojbasevno];

		    if (BTtest((i4)ojbasevno, (char *)outerchildp)
			&&
			BTtest((i4)ojbasevno, (char *)outerp->opl_maxojmap))
		    {	/* make TID join inner child if possible in order
			** for the special case of FULL JOINs to work
			** related to bug 59935, in which an incorrect
			** tid join was performed splitting the multi-attr
			** FULL JOIN into a single attr FULL JOIN and an
			** implicit boolean factor at the TID join */
			outerchildp = innerchildp;
		    }
		    /* Disallow plans that would attempt to evaluate an
		    ** OJ across multiple nodes -- has to happen in one place.
		    */
		    if (outerp->opl_ojattr
			&& 
			!opl_ojverify(subquery, outerp->opl_ojattr, outerchildp, outerp))
		    return(FALSE);

		    if ((ojbasevarp->opv_mask & OPV_OJINNERIDX) &&
			BTtest((i4)ojbasevno, (char *)outerp->opl_ivmap))
			tidojid = ojid;
		}
		else switch (ojtype) /* non-tid join case */
		{
		case OPL_INNERJOIN:
		{
		    ojid_at_node = OPL_NOOUTER;
		    break;
		}
		case OPL_FULLJOIN:
		{
		    /* if full join is evaluated at this node then ignore the
		    ** cart prod check, otherwise no query plan found errors occur */
		    fullojid_found = TRUE; /* do not mark inner join ojid
				** since it may conflict with another join
				** e.g. a left join defined in a view, which
				** is then used in a full join, ... the inner
				** join is used to place relations, but the
				** left join will be placed in the same node
				** as the inner join e.g. select * from t
				** full join ljview on t.a1=ljview.a1 */
		    /* b59935 - check for the case of implicit secondary indices
		    ** which are both inner and outer to the full join
		    ** (like all relations in a full join), need to make
		    ** sure that for any IMPLICIT secondary index that the
		    ** other partition does not reference either the primary
		    ** relation or another implicit secondary index */
		    if ((    !BTsubset((char *)&ojleftmap, (char *)outerp->opl_p1, (i4)maxvar)
			     ||
			     !BTsubset((char *)&ojrightmap, (char *)outerp->opl_p2, (i4)maxvar)

			)
			&&
			(    !BTsubset((char *)&ojleftmap, (char *)outerp->opl_p2, (i4)maxvar)
			     ||
			     !BTsubset((char *)&ojrightmap, (char *)outerp->opl_p1, (i4)maxvar)

			))
		     if (subquery->ops_mask & OPS_LAENUM)
		    {
			/* If new enumeration, we compile bottom up. So just
			** skip to next OJ (heuristics in opnnewenum.c will
			** assure that we eventually compile the full join). */
			fullojid_found = FALSE;			/* flip the flag */
			continue;
		    }
		    else return(FALSE); /* partitioning for full join is not
				       ** as expected */
		    fulloj_cartprod |= ((outerp->opl_mask & OPL_BOJCARTPROD)!=0);
				    /* skip cart prod check for
				    ** left branch */
		    break;
		}
	        case OPL_LEFTJOIN:
		{
		    /* Disallow plans that would attempt to evaluate an
		    ** OJ across multiple nodes -- has to happen in one place.
		    */
		    OPV_BMVARS	    tempfiltermap;
		    MEcopy((PTR)outerchildp, sizeof(tempfiltermap),
			(PTR)&tempfiltermap);
		    BTand((i4)maxvar, (char *)outerp->opl_minojmap,
			(char *)&tempfiltermap);
		    if (BTnext((i4)-1, (char *)&tempfiltermap, (i4)maxvar)>=0)
			return(FALSE);	/* outer join is evaluated at 2 nodes
				    ** and not a tid join so not allowed */
		    break;
		}
	        } /* end switch */

		outerchildp = NULL;
	    }
	    if (outerchildp)
	    {
		if (ojtype == OPL_LEFTJOIN)
		{
		    if (BTsubset((char *)innerchildp, (char *)outerp->opl_maxojmap,
			(i4)maxvar))
		    {   /* no outer relations exist so that the outer join
			** needs to be evaluated at this node since the
			** outer branch contains all outer relations */
			if (ojid_at_node != OPL_NOOUTER)
			    return(FALSE);  /* cannot evaluate 2 outer join
				    ** ID's at the same node */

                        /* b123082:
                        ** if using greedy enumeration, reject the plan if 
                        ** 'bothmaps' does not cover all oj attributes. if we
                        ** allow it and when we join this plan with another table 
                        ** or another plan, at that time, lbase->opl_ojt[ojid]'s 
                        ** innerchildp will be current [innerchildp + outerchildp],
                        ** which is 'bothmaps'. We could get E_OP04C0 after 
                        ** opl_ojverify() rejects such combination.
                        */
                        if (subquery->ops_mask & OPS_LAENUM &&
                            !opl_ojverify(subquery, outerp->opl_ojattr, 
                                 &bothmaps, outerp))
                           return (FALSE);
    
			ojid_at_node = ojid;
		    }
		    else
		    {   /* check that the inner child has necessary
			** info to evaluate the outer join since an
			** outer relation is in the split */
			/* Check each attribute referenced in an outer
			** relation and make sure an appropriate attribute
			** in the equivalence class is available from an
			** outer relation */
			if (outerp->opl_nodep)
			{   /* the same set of boolean factors need to be able
			    ** to be evaluated at this node and the child */
			    if (opl_ojboolplace(subquery, innerchildp, outerp)
				!=
				opl_ojboolplace(subquery, &bothmaps, outerp))
				return(FALSE);  /* if the same number of boolean
				    ** factors cannot be evaluated at the
				    ** child node, then a critical outer relation
				    ** is not in the partition */
			}
			/* If there are no bf, then this must be executed as
			** a cartprod. So, if the current partitioning has
			** the proper inner & outer tables, accept it.
			*/
/*			else if ((outerp->opl_mask &
				  (OPL_BOJCARTPROD | OPL_OJCARTPROD))   &&
				  BTsubset((char *)&ojleftmap,
					   (char *)outerp->opl_ovmap,
					   (i4)maxvar)			&&
				  BTsubset((char *)&ojrightmap,
					   (char *)outerp->opl_ivmap,
					   (i4)maxvar))
			;
*/			else if (!opl_ojverify(subquery, outerp->opl_ojattr, innerchildp, outerp))
			    return(FALSE); /* all attribute info not
				    ** available in this partition */
		    }
		}
		if (outerp->opl_mask & (OPL_BOJCARTPROD | OPL_OJCARTPROD))
		    opl_cartprodcheck(subquery, outerp, cartprodpp, cartprodp); /*
				** set up maps, so that cart prods are allowed
				** so that at least one query plan will be found */
	    }
	    if ((ojid_at_node == ojid)
		&&
		outerp->opl_reqinner)
	    {	/* for cases in which the NULL value in the result of an
		** outer join is used in a parent qualification in a
		** "NULL handling expression", e.g. IS NULL, it is necessary
		** to ensure that all appropriate outer joins are evaluated
		** to produce the NULLs prior to evaluating the respective
		** boolean factor for this outer join, FIXME this can be
		** less restrictive if each boolean factor/function attribute
		** is tested
		** - also used to ensure joins are evaluated in the
		** scope of a full join in which the defined them */
		OPL_IOUTER	ojid1;
		OPL_IOUTER	maxoj;
		maxoj = subquery->ops_oj.opl_lv;
		for (ojid1 = -1; (ojid1 = BTnext((i4)ojid1, (char *)
		    outerp->opl_reqinner, (i4)maxoj))>=0;)
		{
		    OPL_OUTER	    *outerp1;
		    outerp1 = lbase->opl_ojt[ojid1];
		    if (outerp1->opl_nodep
			||
			!opl_ojverify(subquery, outerp1->opl_ojattr, &bothmaps, outerp1))
			return(FALSE);	/* return if outer join cannot be evaluated
					** entirely within context of this outer join */
		}
	    }
	}
	if ((tidojid != OPL_NOOUTER) && (tidojid == ojid_at_node))
	    nodep->opn_jmask |= OPN_OJINNERIDX;
	nodep->opn_ojid = ojid_at_node;
	if (ojid_at_node >= 0)
	{
	    lbase->opl_ojt[ojid_at_node]->opl_nodep = nodep;
	    if (fullojid_found)
	    {	/* mark this full join node to avoid computations in the subtree */
		nodep->opn_jmask |= OPN_FULLOJ;	/* for a full or
				    ** inner join, mark node in which non-tid join
				    ** occurs,... used to prevent multiple
				    ** full join nodes
				    ** for the same join id, except tid joins */
		lbase->opl_ojt[ojid_at_node]->opl_mask |= OPL_FULLOJFOUND; /*
				    ** mark full join node as being assigned */
	    }
	}
    }

    ntcheck = FALSE;
    if (subquery->ops_mask & OPS_NOTIDCHECK)
    {	/* TIDs cannot be used to keep these duplicates so make sure that
        ** set of tables with duplicates to be removed i.e. ops_remove_dups
        ** are not "split" into 2 partitions which contain any non-TID tables
        ** since a sort node to remove duplicates is required
        ** at this node or above
        ** - this will remove large portions of the search space but a
        ** check will still be needed to ensure all duplicates are removed
        ** before a join is made to a table which does not have TIDs
        ** and needs duplicates kept */
	bool	    left_rdups;	    /* TRUE if left side has at least one
				    ** remove duplicates relations */
	bool	    right_rdups;    /* TRUE if right side has at least one
				    ** remove duplicates relations */
	bool	    left_kdups;	    /* TRUE if left side has at least one
				    ** keep duplicates relations */
	bool	    right_kdups;    /* TRUE if right side has at least one
				    ** keep duplicates relations */
	left_rdups = right_rdups = left_kdups = right_kdups = FALSE;

	for (leftp = nodep->opn_prb; leftp < leftlimit; leftp++)
	{
	    if (!left_rdups
		&&
		BTtest((i4)*leftp, (char *)subquery->ops_remove_dups))
		left_rdups = TRUE;
	    else if (!left_kdups
		&&
		BTtest((i4)*leftp, (char *)subquery->ops_keep_dups))
		left_kdups = TRUE;
	}
	for (rightp = leftlimit; rightp < rightlimit; rightp++)
	{
	    if (!right_rdups
		&&
		BTtest((i4)*rightp, (char *)subquery->ops_remove_dups))
		right_rdups = TRUE;
	    else if (!right_kdups
		&&
		BTtest((i4)*rightp, (char *)subquery->ops_keep_dups))
		right_kdups = TRUE;
	}
	if (left_rdups && right_rdups && (left_kdups || right_kdups))
	    return(FALSE);	    /* if duplicates need to be removed on
				    ** both sides by a sort node, then
				    ** a relation with no tids, and requiring
				    ** duplicates to be kept cannot exist
				    ** in this partition */
	if (subquery->ops_mask & OPS_NTCARTPROD)
	{   /* check for special case of allowing cart-prod in order to
	    ** allow duplicate semantics to be enforced */
	    if (left_rdups && right_kdups && !left_kdups)
	    {
		ntcheck = TRUE;
		rdvars = nodep->opn_prb;
		rdlimit = nodep->opn_csz[OPN_LEFT];
		kdvars = leftlimit;
		kdlimit = nodep->opn_csz[OPN_RIGHT];
	    }
	    else if (right_rdups && left_kdups && !right_kdups)
	    {
		ntcheck = TRUE;
		rdvars = leftlimit;
		rdlimit = nodep->opn_csz[OPN_RIGHT];
		kdvars = nodep->opn_prb;
		kdlimit = nodep->opn_csz[OPN_LEFT];
	    }
	}
    }
    if (((subquery->ops_gateway.opg_smask & OPG_INDEX) != 0) /* gateway
				    ** mask indicates whether
				    ** indexes need to be checked */
	&&
	(subquery->ops_global->ops_cb->ops_server->opg_smask & OPF_INDEXSUB)
				    /* and the gateway relation has index
				    ** constraints on it */
	)
    {
	/* check that a partition contains both the index and the underlying
	** base table for a gateway, if only one exists then this must be
	** a leaf or... this must be a binary tree containing both tables */
	for (leftp = nodep->opn_prb; leftp < leftlimit; leftp++)
	{
	    /* check if this is a gateway relation */
	    OPV_VARS		*lvarp;
	    OPV_IVARS		base_rel;

	    lvarp = subquery->ops_vars.opv_base->opv_rt[*leftp];
	    base_rel = lvarp->opv_index.opv_poffset;
	    if (lvarp->opv_grv->opv_relation    /* if this is an RDF relation */
		&&
		(lvarp->opv_grv->opv_relation->rdr_rel->tbl_status_mask & DMT_GATEWAY) /* is this
					    ** a gateway table */
		&&
		(base_rel >= 0)		    /* just in case */
		)
	    {	/* check that this gateway table is in the same partition as the index
		** if the index is referenced */
		for (rightp = leftlimit; rightp < rightlimit; rightp++)
		{   /* now check if there is also one in the right side */
		    if (base_rel
			==
			subquery->ops_vars.opv_base->opv_rt[*rightp]->opv_index.opv_poffset
			)
		    {	/* if the base relation is on one side and the index is on the
			** other then only in a binary tree is this a valid situation */
			if (nodep->opn_nleaves != 2)
			    return(FALSE);
		    }
		}
	    }
	}
    }
    if ((
	 (!subquery->ops_vars.opv_cart_prod && !subquery->ops_oj.opl_base)
					/* no cartprod and not an oj */
	||
	!leftoj_cartprod
	&&
	!fulloj_cartprod 			    /* no split full oj		*/
	&&
	!inner_cartprod    	    	       /* no inner cartprod below outer */
	&&
	(!subquery->ops_global->ops_cb->ops_check   /* trace point not set      */
	 ||
	  !opt_strace( subquery->ops_global->ops_cb, OPT_F052_CARTPROD)
	 )
	)
	&&
	!(subquery->ops_mask & OPS_LAENUM))
				    /* if cartesean product then allow
                                            ** any placement of relations */
/* cart prod are now restricted so that joinable subsets of relations are
** placed as low in the tree as possible; avoid this only if a trace flag
** is set, so that the user wants to consider cart prods below joins... */
/* ... or if greedy enumeration (kibro01) b120917 */

    {
	/* check that the left and right partitions are connected, since on large
	** joins, significant time can be spent in relation attachment, b31778 */
	bool	    left_connected;
	bool	    right_connected;
	OPV_IVARS   left_var;
	OPV_IVARS   right_var;

	if (nodep->opn_csz[OPN_LEFT] == 1)
	    left_var = *(nodep->opn_prb);
	else
	    left_var = OPV_NOVAR;
	if (nodep->opn_csz[OPN_RIGHT] == 1)
	    right_var = *leftlimit;
	else
	    right_var = OPV_NOVAR;
	left_connected = (left_var != OPV_NOVAR)
	    ||
	    opn_connected(subquery, nodep->opn_prb, nodep->opn_csz[OPN_LEFT], right_var,
		tidjoin, leftojp);
	right_connected = (right_var != OPV_NOVAR)
	    ||
	    opn_connected(subquery, leftlimit, nodep->opn_csz[OPN_RIGHT], left_var,
		tidjoin, rightojp);

	/* check the proposed placement of relations to make sure that the left
	** and right side of this subtree can be joined, or that all cart products
	** are placed above joins
	*/
	for (leftp = nodep->opn_prb; leftp < leftlimit; leftp++)
	{

	    OPV_MVARS	    *leftjoinable;  /* array of booleans indicating
						** which variables this var is
						** joinable on */
	    leftjoinable = &vbase->opv_rt[*leftp]->opv_joinable; /* select
						** joinable array associated with
						** this var in the left subtree */
	    for (rightp = leftlimit; rightp < rightlimit; rightp++)
	    {
		if (leftjoinable->opv_bool[*rightp]) /* TRUE if left var joinable to
						** var from right subtree */
		{
		    if ((left_connected && right_connected)
			||
			(   ntcheck
			    &&
			    opn_ntcheck(subquery, rdvars, rdlimit, kdvars, kdlimit)
					    /* duplicate semantics require
					    ** cartesean product to be allowed */
			))
		    {
			leftp = leftlimit;  /* set condition to exit loop */
			break;
		    }
		    else
			return (FALSE);
					    /* for cart-prod, if the left and
					    ** right partitions are joinable
					    ** then both partitions should be
					    ** connected otherwise a cart prod
					    ** will be placed below a join */
		}
	    }
	}
					    /* mark node as having a successful
					    ** partition */
					    /* since the left and right partitions
					    ** are not joinable, this cart prod will
					    ** be above other joins */
    }
    nodep->opn_jmask |= OPN_JSUCCESS;	    /* mark node as having
					    ** a successful partition */
    if (leftvalid)
    {
	MEcopy((PTR)&leftmap, sizeof(leftmap), (PTR)&nodep->opn_child[OPN_LEFT]->opn_tidplace);
	nodep->opn_child[OPN_LEFT]->opn_jmask |= OPN_JFORKS;
    }
    if (rightvalid)
    {
	MEcopy((PTR)&rightmap, sizeof(rightmap), (PTR)&nodep->opn_child[OPN_RIGHT]->opn_tidplace);
	nodep->opn_child[OPN_RIGHT]->opn_jmask |= OPN_JFORKS;
    }

    return(TRUE);
}

/*{
** Name: opn_arl	- attach relations to leaves
**
** Description:
**      This routine will accept a structurally unique tree and attach
**	relations to the leaves of the tree.  There are hueristics that
**      are checked in this routine.
**
**      FIXME - it would be faster to use the joinable array to determine
**      whether a set of relations is connected prior to passing this set
**      to the child to do all the enumerations.
** Inputs:
**      subquery                        ptr to subquery being analyzed
**                                      used for trace flags
**      nodep                           ptr to node of join tree which
**                                      will be assigned a set of vars
**      varset                          set of vars to be assigned to this
**                                      node
**
** Outputs:
**	Returns:
**	    TRUE if the relations were successfully attached to the leaves
**          and the hueristics are passed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-jun-86 (seputis)
**          initial creation from arl
**      16-dec-98 (wanfr01)
**          initialize nodep->opn_ojid to OPL_NOOUTER to indicate this
** 	    is the first time to process this tree
**	19-nov-99 (inkdo01)
**	    Changes to remove EXsignal from exit processing.
**	9-Dec-2004 (schka24)
**	    Rearrange goofy looking for-loop to avoid Sun WS C code gen bug
**	    (WS 6 Update 2 C 5.3 2001/05/15).  Oddly enough, the bug only
**	    happens when NOT compiling optimized.  The error was that the
**	    loop index was post-decremented once when entering the loop, but
**	    was then pre-decremented during the loop;  so if you started with
**	    a loop counter of 1, it skipped zero and spun.
**	28-Dec-2004 (schka24)
**	    Fix boo-boo in above.
[@history_line@]...
*/
bool
opn_arl(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *nodep,
	OPN_STLEAVES	   varset,
	OPN_STATUS	   *sigstat)
{
    OPN_CHILD		 currentchild;	    /* index of current child
                                            ** of this node being analyzed
                                            ** (0 or 1 for binary tree)
                                            */
    OPL_OJT		 *lbase;	    /* base of OJ descriptors */
    *sigstat = OPN_SIGOK;
    lbase = subquery->ops_oj.opl_base;
    if (varset)
    {	/* if we are to generate the first partitioning (instead of the
	** next partitioning)
	*/

	{   /* copy relation numbers from parent into opn_pra and opn_prb
	    ** of this node ; i.e. assign this set of vars to this join node
            ** - if this is a leaf then only one var will be in the set
	    */
	    OPV_IVARS          *pra_ptr;	/* used to initialize opn_pra */
	    OPV_IVARS          *prb_ptr;	/* used to initialize opn_prb */
	    OPN_LEAVES         varcount;        /* number of elements to init */

	    pra_ptr = nodep->opn_pra;
	    prb_ptr = nodep->opn_prb;
	    varcount = nodep->opn_nleaves;
	    while (--varcount >= 0)
	    {
		*pra_ptr++ = *prb_ptr++ = *varset++;
	    }

	}
	if (nodep->opn_nleaves == 1)		/* if this is a leaf */
	    return (TRUE);			/* otherwise we have to worry
						** about subtrees */

	nodep->opn_ojid = OPL_NOOUTER;          /* This tree is being processed
                                                   for the first time */
	nodep->opn_jmask |= OPN_JREACHED;
	if ( !opn_jintersect (subquery, nodep, sigstat))
	    goto middle;
	currentchild = -1;			/* from now on "currentchild"
						** is used as an
						** index into the opn_child and
						** opn_csz arrays */
	goto forward;
    }
    else
    {
	if (nodep->opn_nleaves == 1)		/* if this is a leaf */
	    return (FALSE);			/* then return 0 since there
						** cannot be more than one
						** partitioning for a leaf (it
						** only has one relation) */
	currentchild = nodep->opn_nchild;
	while (--currentchild >= 0)
	{
	    /* From rightmost child to leftmost */
	    if (opn_arl (subquery, nodep->opn_child[currentchild],
		(OPV_IVARS *)NULL, sigstat))	/* if there is
						** a valid (next) partitioning
						** for this child
						*/
		goto forward;			/* then see if there are valid
						** (first) partitionings for
						** the children to the right of
						** here */
	}
    }
middle:
    if (*sigstat == OPN_SIGEXIT) return(FALSE);
    if (lbase && (nodep->opn_ojid >= 0))
    {   /* reinitialize outer join values for this node
	** since variables have been rearranged */
	if (nodep->opn_ojid > subquery->ops_oj.opl_lv)
	    opx_error(E_OP04B4_PLACEOUTERJOIN);	/* out of range outer join
					    ** ID */
	lbase->opl_ojt[nodep->opn_ojid]->opl_nodep = NULL;
	lbase->opl_ojt[nodep->opn_ojid]->opl_mask &= (~OPL_FULLOJFOUND);
	nodep->opn_jmask &= (~OPN_FULLOJ);
	nodep->opn_ojid = OPL_NOOUTER;
    }
    currentchild = -1;
    if ( !opn_cpartition (subquery, nodep)) /* there are no more
						** possible partitionings in the
						** children for the current
						** partitioning at this node so
						** get a new partitioning for
						** this node */
	return (FALSE);				/* if no more return 0 */
    if ( !opn_jintersect(subquery, nodep, sigstat))
    {
#if 0  /* jskiptree never set */
**	if (subquery->ops_global->ops_gmask & OPS_JSKIPTREE)
**	    return(FALSE);			** skip remaining attachments
**						** since high probability that
**						** tree shape is incorrect **
#endif
	goto middle;
    }
forward:
    if (*sigstat == OPN_SIGEXIT) return(FALSE);
    while (++currentchild < nodep->opn_nchild)
	if (!opn_arl (	subquery,
			nodep->opn_child[currentchild],
			nodep->opn_prb + nodep->opn_pi[currentchild], sigstat))
						/* if there is not a (first)
						** partitioning for this child
						*/
	    goto middle;			/* then get a new partitioning
						** for this node */
    return (TRUE);
}
