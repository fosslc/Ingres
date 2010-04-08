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
#define        OPN_SM1      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNSM1.C - set maps for project-restrict-reformat for leaf node
**
**  Description:
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
[@history_line@]...
**/

/*{
** Name: opn_sm1	- set maps for project-restrict-reformat for leaf node
**
** Description:
**	for each boolfact - find boolean factors that can be evaluated here
**	    If it's map is subset of this rel's map then set bit in bfm
**
**	for each eqclass in rel - find equivalence classes on which histograms
**              would be useful if they were made available from the subtree.
**	    If the eqclass
**			a) belongs to an SBOP node in a boolfact which is
**				not in map bfm above
**				(so far hists are useful only for SBOP's)
**		    or  b) has more than one member (so could be used for
**				a join)
**			then set bit in eqh
**
**	Called by:	opn_nodecost
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      eqm                             equivalence classes which are
**                                      available from the subtree
**
** Outputs:
**      eqh                             bitmap equivalence classes in eqm for 
**                                      which histograms would be useful 
**      bfm                             boolean factors which can be evaluated
**                                      at this node or below
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-may-86 (seputis)
**          initial creation
**	26-oct-88 (seputis)
**          add corelated parameter
**      29-mar-90 (seputis)
**          fix byte alignment problems
**	8-july-05 (inkdo01)
**	    Add group by eqcs to eqh.
**	31-Aug-2005 (schka24)
**	    Typo in above (right not left) prevented group-by histos from
**	    being reliably fetched.
**	30-jul-08 (hayke02)
**	    Replace test for opb_ojid != OPL_NOOUTER (-1) with >= 0, as we can
**	    also have OPL_NOINIT (-2). This change fixes bug 120708.
[@history_line@]...
*/
VOID
opn_sm1(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS        *eqm,
	OPE_BMEQCLS        *eqh,
	OPB_BMBF           *bfm,
	OPV_BMVARS         *vmap)
{
    OPB_IBF             maxbf;		/* number of boolean factors assigned */
    OPB_BFT             *bfbase;        /* ptr to base of boolean factor array*/
    OPE_IEQCLS          maxeqcls;       /* number of equivalence classes */
    OPE_BMEQCLS		byeqcs;		/* EQC list of by list vars */
    OPZ_AT		*abase = subquery->ops_attrs.opz_base;
    PST_QNODE		*byresp, *byvalp;  /* for analyzing BY list */

    MEfill(sizeof(*bfm), (u_char) 0, (PTR) bfm ); /* initialize bitmap for
                                        ** boolean factors which can be
                                        ** evaluated at this node */
    maxeqcls = subquery->ops_eclass.ope_ev; /* number of equivalence classes */
    maxbf =  subquery->ops_bfs.opb_bv;  /* get maximum boolean factor number
                                        ** allocated */
    bfbase = subquery->ops_bfs.opb_base; /* get base of boolean
                                        ** factor array */
    {   /* find boolean factors which can be applied at this node */
	OPB_IBF             bfno1;	/* boolean factor - index into
                                        ** the boolean factor array currently
                                        ** being analyzed */
        OPL_OJT             *lbase;

        lbase = subquery->ops_oj.opl_base;
        for (bfno1 = maxbf; bfno1--;)
        {
            OPB_BOOLFACT        *bp;
            bp = bfbase->opb_boolfact[bfno1];
	    if (bp->opb_mask & (OPB_OJJOIN|OPB_OJMAPACTIVE))
		continue;		/* equi joins are not interesting at
					** leaves of the join tree 
					** - also skip any boolean factors which
					** require at least one outer join since
					** these cannot be evaluated at the
					** leaf of a qep */
            if (!bp->opb_virtual
                &&
                BTsubset((char *)&bp->opb_eqcmap, (char *) eqm, (i4)maxeqcls))
            {
                if (lbase && (bp->opb_ojid >= 0))
                {   /* check that this boolean factor is properly placed for this
                    ** outer join */
                    OPV_BMVARS      tempmap;
                    OPL_OUTER       *outerp;
                    /* even though the boolean factor can be placed this low in the
                    ** tree, the semantics for outer joins may require it to
                    ** to evaluated at this point since we cannot eliminate tuples
                    ** until an inner is involved in the subtree */
                    outerp = lbase->opl_ojt[bp->opb_ojid];
                    if (outerp->opl_type == OPL_FULLJOIN)
                        continue;       /* any full join boolean factor cannot be evaluated
                                        ** at a leaf node since it cannot eliminate
                                        ** tuples which participate in the outer join */
                    MEcopy ((PTR)outerp->opl_ojtotal, sizeof(tempmap), (PTR)&tempmap);
                    BTand( (i4)subquery->ops_vars.opv_rv, (char *)vmap, (char *)&tempmap);
                    if (BTnext((i4)-1, (char *)&tempmap, (i4)subquery->ops_vars.opv_rv)< 0)
                        continue;               /* since there does not exist
                                                ** the appropriate variables
                                                ** in the subtree, it cannot be evaluated
                                                ** at this point */
                }
		BTset((i4)bfno1, (char *)bfm); /* set bitmap bit representing bool
                                        ** factor if the equivalence classes
                                        ** required to evaluate the boolean
                                        ** factor are available */
	    }
	}
    }
    if ((   subquery->ops_global->ops_estate.opn_rootflg
	    &&
	    (   (!subquery->ops_global->ops_estate.opn_singlevar)
		||
		(subquery->ops_vars.opv_rv == 1) /* no indexes to consider */
	    )
	)
	||
	!eqh				/* return if eqh not required */
       )
	return;				/* no need to set eqh cause we
					** are at the root. we needed
					** to set bfm, however, for the
					** project restrict, prrleaf. */
    MEfill(sizeof(*eqh), (u_char) 0, (PTR) eqh ); /* initialize 
                                        ** bitmap for
					** equivalence classes which require
                                        ** histograms */
    MEfill(sizeof(byeqcs), (u_char) 0, (PTR)&byeqcs);
					/* and those in the BY list */

    /* Build bit map of EQCs of VARs in group by list (if any). */
    for (byresp = subquery->ops_byexpr; byresp && 
				byresp->pst_sym.pst_type == PST_RESDOM; 
		byresp = byresp->pst_left)
    {
	byvalp = byresp->pst_right;
	if (byvalp->pst_sym.pst_type == PST_VAR)
	    BTset((i4)abase->opz_attnums[byvalp->pst_sym.pst_value.
		pst_s_var.pst_atno.db_att_id]->opz_equcls, (char *) &byeqcs);
    }

    /* FIXME - could check the case in which a single var query is specified
    ** so that histograms on the base relation do not need to be calculated
    ** since only secondary indexes will be joined via a TID join */
    {   /* find equivalence classes for which histograms are required */
	OPE_IEQCLS	       eqcls; /* equivalence class being analyzed */
        OPE_ET                 *ebase;  /* ptr to base of array of equivalence
                                        ** class elements */
        OPZ_IATTS              maxattrs;/* maximum joinop attribute allocated */
	
	ebase = subquery->ops_eclass.ope_base; /* base of array of ptrs to
                                        ** equivalence class elements */
        maxattrs = subquery->ops_attrs.opz_av; /* maximum joinop attribute no
                                        ** which was assigned */
	for (eqcls = -1; 
	    (eqcls = BTnext((i4) eqcls, (char *) eqm, (i4) maxeqcls)) >= 0;)
	{   /* for each equivalence class which is available from the subtree */
	    if (BTcount((char *)&ebase->ope_eqclist[eqcls]->ope_attrmap, 
		(i4) maxattrs) <= 1)	/* if more than one element in the
                                        ** equivalence class then it can be
                                        ** used as a join so request histogram
                                        ** by setting appropriate bit in eqh */
	    {   /* otherwise not a joining equivalence class so check if a 
                ** boolean factor can be evaluated at this node */
		OPB_BOOLFACT          *bp; /* boolean factor being analyzed */
		OPB_IBF               bfno; /* boolean factor number - index
                                        ** into the boolean factor array */
		for (bfno = maxbf; bfno--;)
		    if (!BTtest((i4) bfno, (char *) bfm) /* if boolean factor
                                        must be evaluated higher in the tree */
			&&
			BTtest((i4) eqcls, (char *) 
			    &(bp = bfbase->opb_boolfact[bfno])->opb_eqcmap)
                                        /* and the equivalence class is required
                                        ** to evaluate the boolean factor */
			&&
			!(bp->opb_mask & OPB_OJJOIN) /* this is not an equi-join
					** boolean factor */
			&&
			opb_sfind(bp, eqcls)) /* and the equivalence class is
                                        ** sargarable in the boolean factor
                                        */
			break;
		if (bfno < 0 && !BTtest((i4) eqcls, (char *) &byeqcs))
		    continue;          /* if no boolean factor can be applied
				       ** and eqc not in group by list
                                       ** then we do not need the histogram
                                       ** so continue */
	    }
	    BTset((i4) eqcls, (char *) eqh); /* set bit requesting histogram for
                                       ** this equivalence class */
	}
    }
#ifdef    OPT_F029_HISTMAP
        if (
		(subquery->ops_global->ops_cb->ops_check 
		&& 
		opt_strace( subquery->ops_global->ops_cb, OPT_F029_HISTMAP)
                )
	    )
            opt_ehistmap( eqh );	    /* dump information on histgrams
                                            ** required from below
                                            */
#endif
}
