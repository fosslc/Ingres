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
#define        OPN_IOMAKE      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNIOMAKE.C - make interesting orderings
**
**  Description:
**      routines to calculate interesting orderings
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
** Name: opn_iomake	- make interesting orderings
**
** Description:
**	we need to find all possible interesting orders now because
**	we do not know what eqclass above will be used for joining
**	(and it may vary depending on the tree)
**
**	at a given node, interesting structure/ordering combos are those 
**	that match a possible joining class. if the user had a way of
**	specifying a sort order on output, we could include such a
**	request in our set of interesting structure/orders.
**	we could also consider the default "retrieve into" rel structure
**	which would always key on the first att in the target list.
**
**	we do not care about boolean factors, since any applicable
**	boolean factor is always applied as the tuple is formed.
**	the only way a structure/ordering relates to a boolean factor
**	is in the way the user chooses to store the original relation.
**
**	the possible joining eqclasses are those which contain at least one
**	element within the current subtree and at least one element which is
**	not.  an implicit tid within the current subtree is not counted
**	(reformatting on an implicit tid is meaningless)
** 
** 	set the interesting/ordering map, iomap, based on
**		the rels in relmap and eqclasses in eqmap.
**	an interesting ordering is an eqclass.
**
**   
**	could put in user requested or default orderings here
**
**	could make more restrictive:
**	                   X
**			  /1\
**		   ab->  /   \
**			X     C
**		       /0\
**		      /   \
**		     A     B
**	the joining eqclasses are 0 and 1.  if an index on 0 then 0
**	will be an interesting order at point ab and this seems to be
**	a waste. could fix this,... this would only be true if the relation
**      outside the subtree containing this equivalence class is the base
**      relation.
**
**	Allow tid eqclasses to be interesting orderings in the case where the
**	subtree has only implicit tids but there is more than one relation in 
**      the subtree so the implicit tid will become explicit when it
**	is retrieved.
**
**	Called by:	opn_nodecost
**
** Inputs:
**      global                          ptr to global state variable
**      relmap                          relations which form the leaves of
**                                      the subtree at this node
**      eqmap                           equivalence classes available at
**                                      this node
** Outputs:
**      iomap			        equivalence classes which have the
**                                      interesting ordering property
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
**          initial creation
**      29-mar-90 (seputis)
**          fix byte alignment problems
[@history_line@]...
*/
VOID
opn_iomake(
	OPS_SUBQUERY       *subquery,
	OPV_BMVARS         *relmap,
	OPE_BMEQCLS        *eqmap,
	OPE_BMEQCLS        *iomap)
{
    bool                notprimleaf;	/* TRUE if node is not a primary leaf
                                        ** i.e. it can be an index leaf or 
                                        ** a multi-leaf subtree
                                        */
    OPV_IVARS           maxvars;        /* number of joinop range variables
                                        ** allocated */
    OPV_IVARS           maxprimary;     /* number of primary joinop range
                                        ** variables allocated */
    OPZ_IATTS           maxattr;        /* number of joinop attributes
                                        ** allocated */
    OPE_IEQCLS          maxeqcls;	/* number of equivalence classes
                                        ** allocated */
    OPE_IEQCLS          eqcls;          /* equivalence class being analyzed */
    OPE_ET              *ebase;         /* ptr to base of array of ptrs to
                                        ** equivalence class elements */
    OPZ_AT              *abase;         /* ptr to base of array of ptrs to
                                        ** joinop attribute elements */
    OPZ_IATTS		attr;		/* joinop attribute element within the
                                        ** equivalence class being analyzed */
    bool                leaf_found;     /* is this a leaf */
    OPV_IVARS           leaf_var;       /* if this is a leaf then this is the
                                        ** the joinop var number of the leaf */

    MEfill( sizeof(*iomap), (u_char) 0, (PTR) iomap); /* no interesting 
                                        ** orderings have been found yet */
    maxvars = subquery->ops_vars.opv_rv;/* total range variables allocated */
    maxprimary = subquery->ops_vars.opv_prv; /* total primary range variables*/
    maxeqcls = subquery->ops_eclass.ope_ev; /* total equivalence classes */
    leaf_var = BTnext((i4)-1, (char *)relmap, (i4) maxvars); /* at least one
					** variable must be in this map */
    if (leaf_found = (BTnext((i4)leaf_var, (char *)relmap, (i4) maxvars) < 0))
    {   /* this is a leaf node */
	notprimleaf = (leaf_var >= maxprimary)
		      &&
		      BTtest( (i4)subquery->ops_vars.opv_base->opv_rt
				[leaf_var]->opv_index.opv_poffset,
			    (char *)&subquery->ops_global->ops_estate.opn_sroot->
				opn_rlmap); /* if this node is an index and
                                            ** the index has not substituted
                                            ** the respective base relation */
    }
    else
    {	/* not a leaf node */
	notprimleaf = TRUE;
	leaf_var = OPV_NOVAR;
    }
    maxattr = subquery->ops_attrs.opz_av;/* total joinop attributes allocated*/
    ebase = subquery->ops_eclass.ope_base; /* base of array of ptrs to
					** equivalence class elements */
    abase = subquery->ops_attrs.opz_base; /* base of array of ptrs to 
                                        ** joinop attribute elements */

    for (eqcls = -1; (eqcls = BTnext((i4) eqcls, (char *) eqmap, 
	(i4) maxeqcls)) >= 0;)
    {	/* for each eqclass which has at least one member in this subtree */
	OPE_EQCLIST            *eqclsptr; /* ptr to equivalence class element
                                        ** being analyzed */
	eqclsptr = ebase->ope_eqclist[eqcls];
	if ((eqclsptr->ope_eqctype == OPE_TID) && leaf_found)
					/* if a tid eqclass see if there is an
                                        ** available explicit tid - bug 1677 */
	{   /* this is a leaf node and there is a TID equivalence class */
	    for (attr = -1; (attr = BTnext((i4) attr, 
		(char *) &eqclsptr->ope_attrmap, (i4) maxattr)) >= 0;)
	    {   
		OPZ_ATTS	*attrptr; /* ptr to attribute element within the
					** equivalence class being analyzed */
		attrptr = abase->opz_attnums[attr];
		if ((attrptr->opz_attnm.db_att_id != DB_IMTID) 
		    && 
		    (attrptr->opz_varnm == leaf_var))
		    break;		/* if explicit TID available in
                                        ** this leaf then use it */
	    }
	    if (attr < 0)
		continue;               /* do not request cost orderings
                                        ** if this is an implicit TID */
	}

	for (attr = -1; (attr = BTnext( (i4) attr, 
					(char *)&eqclsptr->ope_attrmap, 
					(i4) maxattr
				      )
			) 
			    >= 0;)
	{   /* if this eqclass also has at least one att outside the subtree 
	    ** then set iom */
            OPV_IVARS   varno;		/* joinop range variable in equivalence
                                        ** class but outside the subtree */
            varno = abase->opz_attnums[attr]->opz_varnm;
	    if (!BTtest( (i4) varno, (char *) relmap) && 
		(   notprimleaf 
		    || 
		    (varno < maxprimary)
		    ||
		    !BTtest( (i4)subquery->ops_vars.opv_base->opv_rt
				[varno]->opv_index.opv_poffset,
			    (char *)&subquery->ops_global->ops_estate.opn_sroot->
				opn_rlmap) /* check if the base relation is
					** contained in the set of relations to
					** be considered, if not then an index
                                        ** substitution has occurred so that
					** it should be considered like a base
					** relation */

		))
		/* also, if this is a base relation leaf, then the att
		** outside this leaf must belong to another base relation 
		** also. the purpose of this is so we do not reformat to an
		** att which will only be used for joining with an index
                ** on this leaf, note the special check for indexes
                ** which have substituted the base relation they represent.
		** if we did the reformat the index would be useless since
		** the restriction would have already been applied. 
		*/
	    {
		BTset((i4) eqcls, (char *) iomap);
		break;
	    }
	}
    }
}
