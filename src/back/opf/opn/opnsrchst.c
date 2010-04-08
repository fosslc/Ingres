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
#define        OPN_SRCHST      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNSRCHST.C - search for a previously generated subtree
**
**  Description:
**      routine which will search for a previously generated subtree
**
**  History:    
**      21-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	3-Jun-2009 (kschendel) b122118
**	    Fixes for OJ filter removal.
[@history_line@]...
**/

/*{
** Name: opn_icheck	- check if all indexes
**
** Description:
**      Returns TRUE if the set of relations contains all the indexes
**      being considered in this current partition of relations.
**
**	This routine should never be called during new enumeration, as
**	what it's checking is meaningless in that case.  (srchst will
**	never get here because new enum does not use ops_swcurrent,
**	leaving it as zero always.)
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      rmap                            map of relations being considered
**
** Outputs:
**	Returns:
**	    TRUE - if all indexes in partition are contained in this
**             set of relation
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-oct-87 (seputis)
**          initial creation
[@history_template@]...
*/
static bool
opn_icheck(
	OPS_SUBQUERY       *subquery,
	OPV_BMVARS         *rmap)
{
    /* check if all indexes are contained in this subtree which
    ** means that this case has possibly not been considered */
    OPV_IVARS	numindex;
    OPV_IVARS	index_var;
    numindex = subquery->ops_global->ops_estate.opn_sroot->opn_nleaves - 
	subquery->ops_vars.opv_prv; /* number of indexes being considered */
    for( index_var = subquery->ops_vars.opv_prv -1;
	( index_var  = BTnext((i4)index_var, (char *) rmap, 
			      (i4)subquery->ops_vars.opv_rv)
	) >= 0;)
    {   /* index var found so decrement count */
	if ((--numindex) <= 0)
	{   /* all indexes being considered, so mark this set of relations*/
	    return (TRUE);
	}
    }
    return (FALSE);		/* this subtree has been evaluated
				** before and has been deleted because
				** it is too expensive */
}

/*{
** Name: opn_srchst	- search for a previously generated subtree
**
** Description:
**	Search for the given subtree to see if its cost-ordering
**	was already generated previously.  if it was then return
**	this information
**
**	Called by:	nodecost
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      nleaves                         number of leaves in subtree
**      eqr                             required equivalence classes
**      rmap                            bitmap of relations in subtree
**      rlasg                           relation assignments to this subtree
**      sbstruct                        structure of the subtree (makejtree
**                                      format)
**
** Outputs:
**      subtp                           subtree info
**      eqclp                           equivalence class info
**      rlmp                            relation map and histogram info
**      cjeqc                           joining equivalence class if subtp
**                                      was found
**	Returns:
**	    OPN_SRESULT - indicates which elements were returned
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-may-86 (seputis)
**	    initial creation
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	 5-mar-01 (inkdo01 & hayke02)
**	    Copy histogram chain, tuple estimate from SUBTREE to RLS. Histos
**	    and tups are really a property of a tree shape/leaf node
**	    assignment pair. This change fixes bug 103858.
{@history_line@}...
*/
OPN_SRESULT
opn_srchst(
	OPS_SUBQUERY       *subquery,
	OPN_LEAVES	   nleaves,
	OPE_BMEQCLS        *eqr,
	OPV_BMVARS         *rmap,
	OPN_STLEAVES       rlasg,
	OPN_TDSC           sbstruct,
	OPN_SUBTREE        **subtp,
	OPN_EQS            **eqclp,
	OPN_RLS            **rlmp,
	bool		   *icheck)
{
    OPN_RLS	    *rlp;
    OPS_STATE       *global;

    *icheck = FALSE;			/* indicates that if subtree is not
					** found then it should be evaluated
					** again */
    global = subquery->ops_global;
    rlp = global->ops_estate.opn_sbase->opn_savework[nleaves];
    for (;				/* get
					** beginning of list with "nleaves"
                                        ** number of elements */
	rlp; rlp = rlp->opn_rlnext)	/* for each relation set with "nleaves"
                                        ** number of elements */
    {
	if ((rlp->opn_relmap.opv_bm[0] == rmap->opv_bm[0])
		&&
		!MEcmp ((PTR)&rlp->opn_relmap, (PTR)rmap, sizeof(*rmap))
	    )
	{   /* if found the correct map then search eqs list */
	    OPN_EQS            *eqp;	/* used to search list of subtrees
                                        ** returning different equivalence
                                        ** classes
                                        */
	    *rlmp = rlp;		/* return the OPN_RLS struct found */
	    rlp->opn_delflag = FALSE;	/* so freesp will leave this
					** sub-structure alone */
	    if (nleaves > 1)
	    {
		rlp->opn_histogram = (OPH_HISTOGRAM *) NULL;
		rlp->opn_reltups = -1; /* clear histo chain, reltups */
	    }
	    for (eqp = rlp->opn_eqp; eqp; eqp = eqp->opn_eqnext)
					/* for each eqclass set (this is the 
					** set of eqclasses returned by this 
                                        ** subtree) */
	    {
		if ((eqp->opn_maps.opo_eqcmap.ope_bmeqcls[0] == eqr->ope_bmeqcls[0])
		    &&
		    !MEcmp ((PTR)&eqp->opn_maps.opo_eqcmap, (PTR)eqr, 
			sizeof(*eqr))
		    )
					/* if found the correct eqclass set */
		{
	            OPN_SUBTREE *stp;   /* used to traverse list of subtrees */
		    *eqclp = eqp;       /* return OPN_EQS struct found */
		    for (stp = eqp->opn_subtree; stp; stp = stp->opn_stnext)
					/* for each subtree */
			if (
			    stp->opn_relasgn[0] == rlasg[0]
			    &&
			    stp->opn_structure[0] == sbstruct[0]
			    &&
			    !MEcmp ((PTR)stp->opn_relasgn, 
				    (PTR)rlasg, 
				    (nleaves * sizeof(rlasg[0]))
				   ) 
			    &&
			    !MEcmp ((PTR)stp->opn_structure,
				    (PTR)sbstruct,
				    ((2*nleaves-1)*sizeof(sbstruct[0]))
				   )
                           )
					/* if the same order of relations and 
                                        ** the same structure */
			{
			    *subtp = stp;/* subtree found, so all components
				    ** have been found and caller does
				    ** not need to recalculate costs */
			    if (nleaves > 1)
			    {
				rlp->opn_histogram = stp->opn_histogram;
				rlp->opn_reltups = stp->opn_reltups;
			    }
			    return (OPN_STEQRLS); /*subtree, eqs and rls 
				    ** found*/
			}
                    if ((nleaves > global->ops_estate.opn_swcurrent)
			||
			(!rlp->opn_check)
		       )
			return (OPN_EQRLS); /* OPN_EQS, OPN_RLS struct found */
		    else
		    {
			if (opn_icheck(subquery, rmap))
			    return(OPN_EQRLS);
			else
			{
			    rlp->opn_check = FALSE; /* no need to check this
				    ** set of relations again since it has
				    ** been considered in a tree with fewer
				    ** leaves */
			    return (OPN_DST); /* structure was deleted because
				    ** all CO nodes in it were more expensive
				    ** than the current best CO */
			}
		    }
		}
	    }
	    if ((nleaves > global->ops_estate.opn_swcurrent) 
		||
		(!rlp->opn_check)
	       )
		return (OPN_RLSONLY); /* only OPN_RLS structs found */
	    else
	    {
		if (opn_icheck(subquery, rmap))
		    return(OPN_RLSONLY);
		else
		{
		    rlp->opn_check = FALSE; /* no need to check this
			    ** set of relations again since it has
			    ** been considered in a tree with fewer
			    ** leaves */
		    return (OPN_DST); /* structure was deleted because
			    ** all CO nodes in it were more expensive
			    ** than the current best CO */
		}
	    }
	}
    }
    if (nleaves > global->ops_estate.opn_swcurrent)
    {
	return (OPN_NONE);  /* none of the structs were found */
    }
    else
    {	/* check if set of relations has been evaluated before */
	if (*icheck = opn_icheck(subquery, rmap))
	    return(OPN_NONE);
	else
	    return (OPN_DST);	    /* this subtree has been evaluated
				    ** before and has been deleted because
				    ** it is too expensive */
    }
}
