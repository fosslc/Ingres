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
#include    <ex.h>
#define             OPX_EXROUTINES      TRUE
#define             OPN_PROCESS         TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNPROCESS.C - process a join tree structure
**
**  Description:
**      This file contains all routines which attach relations to the join
**      tree and perform hueristics prior to cost enumeration 
**
**  History:    
**      13-may-86 (seputis)    
**          initial creation from process.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      26-mar-94 (ed)
**          bug 49814 E_OP0397 consistency check
**    11-dec-2002 (devjo01)
**        Remove undeclared variable 'firstcomb' which was introduced
**        at rev 20, most of which was quickly backed out by subsequent
**        changes.
**    09-Oct-2003 (hanje04)
**	  Backed out X-integ of devjo01 'firstcomb' change from 2.6 as its
**	  not relavent to main.
[@history_line@]...
**/

/*{
** Name: opn_cjtree	- clear mask field in join tree
**
** Description:
**      Init mask field, used to track status of tree search 
**
** Inputs:
**      jtree                           ptr to current tree node
**					being analyzed 
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
**      11-may-91 (seputis)
**          initial creation to skip tree shapes more quickly, b37233
**	14-jun-91 (seputis)
**	    check for leaf nodes, since the child nodes are not
**	    initialized for these
**      26-mar-94 (ed)
**          bug 49814 E_OP0397 consistency check
**	30-oct-02 (inkdo01)
**	    Externalize for use by new enumeration logic.
[@history_template@]...
*/
VOID
opn_cjtree( 
	OPS_SUBQUERY	    *subquery,
	OPN_JTREE	    *jtree)
{
    jtree->opn_jmask = 0;
    for (; jtree->opn_nleaves > 1; jtree = jtree->opn_child[OPN_LEFT])
    {
	jtree->opn_jmask = 0;
	if (subquery->ops_oj.opl_smask & OPL_OJCLEAR)
	{
	    jtree->opn_jmask &= (~OPN_FULLOJ);
	    jtree->opn_ojid = OPL_NOOUTER;
	}
	opn_cjtree(subquery, jtree->opn_child[OPN_RIGHT]);
    }
}

/*{
** Name: opn_process	- process a join tree structure
**
** Description:
**	For each combination of relations and indexes, (the do
**	loop) assign them in all possible ways to the leaves of the
**	tree (procedure opn_arl does this).  Check to see that
**	certain heuristics are met and if they are then evaluate the
**	cost of this tree.
**
**	Examine possibility of using the secondary index instead of base 
**      relation(s).  Instead of always including base relations,
**      permute through all relations.  "opn_gnperm" gets the next permutations
**      where the relations used in the permutation are sufficient to the query.
**
** Inputs:
**      subquery                        ptr to current subquery being analyzed
**      .ops_global                     ptr to global state variable
**          .ops_estate                 current state of enumeration phase
**              ->opn_sroot             root of structurally unique join tree
**      numleaves                       number of leaves in the join tree
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
**	13-may-86 (seputis)
**          initial creation from process
**      29-mar-90 (seputis)
**          fix metaphor performance problem - be smarter about search space
**	    fix byte alignment problems
**	16-may-90 (seputis)
**	    - b21582 fix
**	20-jul-90 (seputis)
**	    - b30812 - init the index subsititution map
**      10-Aug-90 (GordonW)
**          The last change caused a SEGV by not initializing "nextvar".
**	11-may-91 (seputis)
**	    fix for b37233, init status fields in opn_jmask
**      1-apr-92 (seputis)
**          fix for b40687 - prune unbalanced tree shapes more quickly
**          when secondary indexes are involved.
**      5-jul-95 (inkdo01)
**          (Temporarily) disables last of 3 opn_cjtree calls. opn_arl already
**          clears those flags, and only for parts of tree being reanalyzed by
**          opn_jintersect.
**	09-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
**	30-apr-96 (inkdo01)
**	    Add Rtree tid join heuristic to assure only 1 Rtree secondary per
**	    primary is in permutation.
**	5-sep-97 (inkdo01)
**	    Fine tune Rtree check. Forgot to see if it even was an Rtree ix.
**	3-sep-98 (inkdo01)
**	    Another adjustment to rtree code to assure we only cost valid
**	    rtree join orders.
**	18-nov-99 (inkdo01)
**	    Replaced EXsignal by return of signal-specific value to avoid
**	    the overhead of kernel longjmp calls.
**	18-sep-01 (hayke02)
**	    Return OPN_SIGEXIT if either opn_timeout() returns TRUE or
**	    opn_arl() returns FALSE and sigstat is OPN_SIGEXIT - this ensures
**	    that we timeout correctly when looking at a large number of
**	    index combinations. This change fixes bug 105801.
**	21-may-02 (inkdo01)
**	    Added heuristic for big range tables to place secondaries right in
**	    front of corr. base tables in 1st permutation.
**	27-aug-02 (inkdo01)
**	    Move opn_gnperm call to loop head (pre-test) so index heuristics can 
**	    be applied to first combination, too (avoiding unnecessary calls to 
**	    opn_arl).
**	29-nov-02 (inkdo01)
**	    Removed heuristic of 21-may. It dropped valuable permutations of
**	    leaf node assignments. This fixes 109197.
**    11-dec-2002 (devjo01)
**        Remove undeclared variable 'firstcomb' which was introduced
**        at rev 20, most of which was quickly backed out by subsequent
**        changes.
**    09-Oct-2003 (hanje04)
**	  Backed out X-integ of devjo01 'firstcomb' change from 2.6 as its
**	  not relavent to main.
**	30-mar-2004 (hayke02)
**	   Call opn_exit() with a FALSE longjump.
**	23-mar-06 (dougi)
**	    Add checks for ORDER hint.
*/
OPN_STATUS
opn_process(
	OPS_SUBQUERY	   *subquery,
	i4                numleaves)
{
    OPV_BMVARS          rmap;               /* map of base relations needed
                                            ** lower in the join tree */
    OPV_BMVARS          pr_n_included;	    /* map of base relations dropped
                                            ** from join tree */
    OPN_STLEAVES        permutation;	    /* contains the current permutation
                                            ** of joinop range variables
					    ** which will be attached to the 
					    ** leaves structurally unique tree
					    */
    OPV_IVARS           maxvars;            /* number of joinop range variables
                                            ** defined in subquery
                                            */
    OPN_JTREE           *root;              /* root of the structurally unique
                                            ** join tree */
    OPN_PARTSZ          partsz;	    	    /* This array defines the number
                                            ** of elements in a partition.  For
                                            ** binary trees this array has
                                            ** two elements, the first of which
                                            ** defines the size of the first 
                                            ** partition etc.; i.e. the number 
                                            ** relations in the partition */
    OPS_STATE		*global;
    OPV_IVARS		maxprimary = subquery->ops_vars.opv_prv;
    OPN_STATUS		sigstat = OPN_SIGOK;
    bool		firstcomb = TRUE;
    bool		primonly = (maxprimary == numleaves);

    global = subquery->ops_global;
    if ((global->ops_gmask & OPS_SKBALANCED) && (global->ops_gmask & OPS_BALANCED))
    {
	global->ops_gmask &= (~OPS_SKBALANCED);
	return(OPN_SIGOK);		    /* on the second pass, only consider
					    ** balanced trees, which is done by
					    ** returning from the first call to opn_process
					    */
    }
    MEfill(sizeof(pr_n_included), (u_char)0, (char *)&pr_n_included);

    if (global->ops_gmask & OPS_GUESS)
    {
	OPV_IVARS	    *varp;	    /* ptr into array of joinop
					    ** range variables */
	OPV_IVARS           varnum;         /* joinop range variable number */

	maxvars = subquery->ops_vars.opv_rv;    /* number of joinop range variables
						** defined */
	varp = permutation;
	for (varnum = 0; varnum < maxvars; *varp++ = varnum++); /* setup array
					    ** which is used for deciding 
					    ** which indexes to use */
    }
    else
    {	/* use initial set of relations determined by hueristics
	** to be a good approximation of the best set of secondaries
	** to be used */
	OPV_IVARS	nextvar;
	OPV_IVARS	varno;
	maxvars = numleaves;
	nextvar = 0;
	MEcopy((PTR)global->ops_estate.ops_arlguess,
	    sizeof(permutation), (PTR)&permutation[0]);
	for (varno = 0; varno < maxprimary; varno++)
	{
	    if (permutation[nextvar] == varno)
		nextvar++;
	    else
		BTset ((i4)varno, (char *)&pr_n_included); /* mark relations
					    ** which are index substitions */
	}
    }
    /* Set the size of the partitions for secondary index use.
    ** Permute through all relations.
    */
    partsz[0] = numleaves;		    /* number of leaves in the tree */
    partsz[1] = maxvars - numleaves;        /* number of remaining joinop
					    ** variables which cannot be 
					    ** to leaves in the tree 
					    */
    root = global->ops_estate.opn_sroot;    /* get root of the join
                                            ** tree */

    while (
	opn_gnperm(subquery, permutation, maxvars, partsz, &pr_n_included, firstcomb))
    {	/* go through this loop once for every combination of index relations */
	OPV_IVARS              *firsttime;  /* NULL if not the first time
                                            ** that opn_arl is called for this
                                            ** particular permutation */
	/* if (!firstcomb && primonly)
	    break;			    /* once through the combination loop
					    ** for primary only */
	firstcomb = FALSE;		    /* next gnperm call will produce 
					    ** real combination */

	global->ops_gmask &= (~OPS_JSKIPTREE); /* reset skip plan flag for this
					    ** set of relations, which is set
					    ** if current tree shape is not
					    ** appropriate */
        if (subquery->ops_mask & OPS_CPLACEMENT)
        {
            /* check for proper tree shape given 2 or more tid joins */
            OPV_BMVARS      tjmap;          /* map of indexes to be used in
                                            ** TID joins */
            OPV_BMVARS      rtjmap;         /* map of Rtree indexes to be used 
                                            ** in TID joins */
            OPV_IVARS       varno1;
            OPV_IVARS       tidcount;       /* number of tid joins required */
	    OPV_RT	    *vbase = subquery->ops_vars.opv_base;
	    bool	    rtreefail = FALSE;

            opn_cjtree(subquery, root);
            tidcount = 0;
            MEfill(sizeof(tjmap), 0, (PTR)&tjmap);
            MEfill(sizeof(rtjmap), 0, (PTR)&rtjmap);
            for (varno1 = 0; varno1 < numleaves; varno1++)
            {   /* create map of indexes which are to be used in TID joins */
                OPV_IVARS       index;
                OPV_IVARS       primary;
                index = permutation[varno1];
                primary = vbase->opv_rt[index]->opv_index.opv_poffset;
                if ((primary != index)
                    &&
                    (primary >= 0))
		{
		    if (vbase->opv_rt[index]->opv_mask & OPV_SPATF)
		     if (BTtest((i4)primary, (char *)&pr_n_included) ||
			BTtest((i4)primary, (char *)&rtjmap)) rtreefail = TRUE;
					    /* 2 Rtree ixes on same primary
					    ** in same permutation - NOT 
					    ** ALLOWED, NOR is Rtree without
					    ** corresponding primary */
		     else BTset((i4)primary, (char *)&rtjmap);
                    if (!BTtest((i4)primary, (char *)&pr_n_included))
		    {
                	BTset((i4)primary, (char *)&tjmap);
                	tidcount++;
		    }
                }
            }
            if (rtreefail || 
		tidcount >= 2 &&
                !opn_tjconnected(subquery, /* nodep, */ permutation, numleaves,
                    &tjmap, root))
            {
                if (opn_timeout(subquery))  /* check for timeout conditions */
		{
                    opn_exit(subquery, FALSE);
		    return(OPN_SIGEXIT);
		}
                continue;                   /* skip this assignment since tree
                                            ** shape is inappropriate */
            }
        }

	firsttime = permutation;
        if (subquery->ops_oj.opl_base)
	{
	    OPL_IOUTER	    oj;		/* traverse oj descriptors */

	    /* calculate a bitmap of available range variables */

	    /* reset outer join values in join tree */
	    subquery->ops_oj.opl_smask |= OPL_OJCLEAR;
	    opn_cjtree(subquery, root);
	    subquery->ops_oj.opl_smask &= (~OPL_OJCLEAR);

	    for (oj = subquery->ops_oj.opl_lv; --oj >= 0;)
	    {
		subquery->ops_oj.opl_base->opl_ojt[oj]->opl_mask &= (~OPL_FULLOJFOUND);
		subquery->ops_oj.opl_base->opl_ojt[oj]->opl_nodep = NULL;
	    }
	}

	while (opn_arl (subquery, root, firsttime, &sigstat)) /* recursive routine to
					    ** attach relations to leaves */
	{
	    OPL_BMOJ	    tempmap;	    /* used to help build map of
					    ** outer joins which have been
					    ** entirely evaluated at a node */
	    OPL_BMOJ	    *ojmap;

	    if (sigstat == OPN_SIGEXIT) return(sigstat);

	    if (subquery->ops_oj.opl_lv > 0)
	    {
		ojmap = &tempmap;
		MEfill(sizeof(tempmap), (u_char)0, (char *)ojmap);
	    }
	    else
		ojmap = NULL;

	    firsttime = NULL;		    /* indicates that this is not
                                            ** the first call to opn_arl
                                            ** with this permutation */
	    
	    /* If ORDER hint is in effect and this is hint pass, call
	    ** special function to validate this permutation. */
	    if (subquery->ops_mask2 & OPS_HINTPASS &&
		subquery->ops_mask2 & OPS_ORDHINT &&
		!opn_order_hint(subquery, root))
		continue;		    /* fails ORDER test - try next */

	    (VOID) opn_ro4(subquery, root, &rmap);  /* Set rmap to those base 
					    ** relations that were needed lower
					    ** in the join tree 
                                            ** - FIXME - this test is useless
                                            ** since an index is not considered
                                            ** only for its' ordering, there
                                            ** must be a boolean factor, or
                                            ** the index must replace the base
                                            ** relation entirely, in which case
                                            ** the test is ignored anyways */
	    if (
		(   /* No primary relations were needed further up the tree or
		    ** those that were needed have been dropped from this cost
                    ** tree, because indexes have been used to replace them
		    */
		    !BTcount((char *)&rmap, (i4) maxvars)
		    ||
		    BTsubset((char *)&rmap, (char *)&pr_n_included, (i4) maxvars)
		)
		&& 
#if 0
/* placed equivalent check inside opn_jintersect to eliminate this case earlier */
                /* 4.0 imap check replaced by check inside opn_gnperm */
		opn_ro3 (subquery, root) /* Pass the relorder3 requirements */
		&&
#endif
		opn_jmaps(subquery,root,ojmap)	/* set the opn_eqm, opn_rlasg 
						** and opn_rlmap maps 
                                                ** - check for correct placement
                                                ** of subselects */
		)
	    {

		    global->ops_gmask |= OPS_PLANFOUND;	/* mark that at least one
						** plan is found at this level
						** since an unbalanced tree may never
						** produce a plan if 2 tables with
						** and index each exists, since it
						** would not be joinable */
		    sigstat = opn_ceval(subquery);	/* evaluate cost of this
                                                ** join tree */
		    if (sigstat == OPN_SIGEXIT) return(sigstat);
	    }
            /*  Following call disabled because it effectively clears join/ojoin
            **  flags from ENTIRE join tree, even though subsequent opn_arl call
            **  may only reassign nodes in lower part of tree. This has the effect
	    **  of potentially losing vital outer join information. The flags 
	    **  re-initted by this call are also reset node by node in opn_arl,
	    **  so this call apears superfluous in any event. (bug 69462) */
/*          if (subquery->ops_mask & OPS_CPLACEMENT)
		opn_cjtree(subquery, root);    */
	}
	if (sigstat == OPN_SIGEXIT) return(sigstat);
    } 					    /* loop as long as there are more 
					    ** combinations */ 

    if (!(global->ops_gmask & OPS_BALANCED) && (global->ops_gmask & OPS_PLANFOUND))
	return(OPN_SIGSEARCHSPACE);	    /* New mechanism for "signalling"
					    ** searchspace condition. */
	/* EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)E_OP0401_SEARCHSPACE);/* signal an 
                                            ** optimizer long jump so that the
                                            ** exception handler can look at the
					    ** next unbalanced tree with an extra
					    ** leaf */
    return(OPN_SIGOK);			    /* otherwise, normal return */
}
