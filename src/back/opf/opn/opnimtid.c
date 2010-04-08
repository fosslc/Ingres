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
#define        OPN_IMTIDJOIN      TRUE
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNIMTID.C - check for implied TID join
**
**  Description:
**      Check for implied TID join between two relations
**
**  History:    
**      30-may-86 (seputis)    
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
** Name: opn_impliedtid	- check for implied TID join between two relations
**
** Description:
**      Check if there is an implied TID join between the two subtrees
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      lp                              ptr to left join subtree
**      rp                              ptr to right join subtree
**      eqcls                           joining equivalence class to test
**
** Outputs:
**      lfflag                          TRUE if left subtree "rp" contains
**                                      the implicit TID
**	Returns:
**	    TRUE - if there is an implied TID join between the two relations
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-may-86 (seputis)
**          initial creation
[@history_line@]...
*/
bool
opn_imtidjoin(
	OPS_SUBQUERY       *subquery,
	OPN_JTREE          *lp,
	OPN_JTREE          *rp,
	OPE_IEQCLS         eqcls,
	bool               *lfflag)
{
    OPE_EQCLIST         *eqclsp;	    /* ptr to joining equivalence class
                                            */
    OPZ_BMATTS          *amap;              /* ptr to attribute map for
                                            ** this equivalence class */
    OPZ_IATTS           attr;               /* current attribute of equivalence
                                            ** class being analyzed */
    OPZ_IATTS           maxattr;            /* number of joinop attributes
                                            ** defined */
    OPZ_AT              *abase;             /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    bool                rightleaf;          /* TRUE if right subtree is a leaf
                                            */
    bool                leftleaf;           /* TRUE if left subtree is a leaf
                                            */

    eqclsp = subquery->ops_eclass.ope_base->ope_eqclist[eqcls]; /* get ptr to
                                            ** joining equivalence class */
    if (eqclsp->ope_eqctype != OPE_TID)	    /* is this a TID equivalence class*/
	return (FALSE);

    amap = &eqclsp->ope_attrmap;	    /* get equivalence class attribute
                                            ** map */
    maxattr = subquery->ops_attrs.opz_av;   /* maximum number of joinop
                                            ** attributes defined */
    abase = subquery->ops_attrs.opz_base;   /* ptr to base of array of ptrs
                                            ** to joinop attributes */
    leftleaf = (lp->opn_nleaves == 1);      /* TRUE if the left subtree is a 
                                            ** leaf */
    rightleaf = (rp->opn_nleaves == 1);     /* TRUE if the right subtree is a
                                            ** leaf */
    for (attr = -1; (attr = BTnext((i4)attr, (char *)amap, (i4)maxattr)) >= 0;)
    {
        OPZ_ATTS        *attrp;             /* ptr to joinop attribute element*/

	attrp = abase->opz_attnums[attr];
	/* if on one side the TID is implied, then on the other side
	** the TID must be explicit 
        */
	if ((*lfflag = (   (leftleaf) 
			&& BTtest((i4)attrp->opz_varnm, (char *)&lp->opn_rlmap)
			&& attrp->opz_attnm.db_att_id == DB_IMTID))
		||     (   (rightleaf)
			&& BTtest((i4)attrp->opz_varnm, (char *)&rp->opn_rlmap)
			&& attrp->opz_attnm.db_att_id == DB_IMTID))
	    return (TRUE);
    }
    return (FALSE);
}
