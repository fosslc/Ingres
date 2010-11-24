/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#define        OPN_UKEY      TRUE
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNUKEY.C - can inner node be probed
**
**  Description:
**      routine to determine if enough attributes are available to use the 
**      key of the relation
**
**  History:    
**      31-may-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	10-jun-96 (inkdo01)
**	    Support for Rtree inners of Kjoins.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
bool opn_ukey(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *oeqcmp,
	OPO_ISORT jeqc,
	OPV_IVARS keyedvar,
	bool primkey);

/*{
** Name: opn_ukey	- useable key - can inner node be probed
**
** Description:
**
** 	Returns TRUE if there are enough attributes
**	to use the key of the relation.
**
**	The jeqc must be the first element in the ordering that is not 
**	satisfied by a boolfact.  This is because all in order for this 
**	routine to return true, keys 0..N were present in the join node.
**	In opn_sjeqc, we will select the join eqc after reviewing all 
**      possibilities. One of the joined eqc's will be the first in the 
**      order list. If we only return true for this one, it will be joined.
**	This only matters in the BTREE case.  The prnode, in this case is 
**	also sorted on the first equiv class.
**	If we picked an arbitrary eqc as keyed, we may do a needless sort.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      oeqcmp                          available equivalence class map of 
**                                      outer node
**      jeqc                            primary attribute's eqc of key
**      keyedvar		        joinop range variable number of 
**                                      possibly keyed relation
**      primkey                         TRUE - if jeqc must be first joined 
**                                      element of orderlist
**
** Outputs:
**	Returns:
**	    TRUE - if keyed access can be used for join
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-may-86 (seputis)
**          initial creation
**	10-jun-96 (inkdo01)
**	    Support for Rtree inners of Kjoins.
[@history_line@]...
*/
bool
opn_ukey(
	OPS_SUBQUERY       *subquery,
	OPE_BMEQCLS        *oeqcmp,
	OPO_ISORT	   jeqc,
	OPV_IVARS          keyedvar,
	bool               primkey)
{
    OPO_STORAGE         storage;	/* storage structure of relation */
    OPV_VARS            *varp;		/* ptr to range variable element to
                                        ** be checked */
    i4                  ordercount;     /* number of attributes that the
                                        ** relation is ordered on */
    i4                  orderindex;     /* index into the OPB_MBF structure
                                        ** of the range variable element */
    OPZ_AT              *abase;         /* ptr to base of array of ptrs to
                                        ** joinop attribute elements */
    bool                ret_val;        /* TRUE is we can do keyed lookup */

    /* Relation must be disk resident */
    if (keyedvar < 0)
	return (FALSE);

    ret_val = FALSE;
    varp = subquery->ops_vars.opv_base->opv_rt[keyedvar]; /* get ptr to range
					** variable element */

    storage = varp->opv_tcop->opo_storage; /* get storage structure 
					** of relation */
    abase = subquery->ops_attrs.opz_base; /* ptr to base of array of ptrs to
                                        ** joinop attribute elements */
#if 0
/* FIXME - why is this check here */    
    if ((storage) < 0)
	storage = -storage;
#endif
# ifdef xNTR1
    if (tTf (1, 0))
    {
	TRdisplay("Checking for possible keyed lookup into %s.\n", Jn.Range[keyedvar]->xrelid);
	TRdisplay("We %s looking for primary join key eqc %d.\n", (primkey ? "are" : "are not"), jeqc);
	TRdisplay("The joinop order eqcs are:");
	for (i = 0; (att = order[i]) > -1; i++)
		TRdisplay("%d ", (i4) Jn.Attnums[att]->equcls);
	TRdisplay("\n");
	uflush();
    }
# endif

    /* Each attribute in the ordering must be available from the outer
    ** or from the boolfacts
    */
    ordercount = varp->opv_mbf.opb_count; /* number of attributes that the
					** relation is ordered on */
    for (orderindex = 0; orderindex < ordercount; orderindex++)
    {
	OPZ_IATTS	       attr;	/* current ordering attribute being
                                        ** analyzed */
	OPE_IEQCLS             eqcls;   /* equivalence class of 
                                        ** of ordering attribute */

	attr = varp->opv_mbf.opb_kbase->opb_keyorder[orderindex].opb_attno;
	eqcls = abase->opz_attnums[attr]->opz_equcls;

	if (storage == DB_RTRE_STORE)	/* special Rtree inner logic */
	{
	    OPB_IBF	bfi;
	    OPB_BOOLFACT *bfp;
	    for (bfi = 0; bfi < subquery->ops_bfs.opb_bv; bfi++)
	     if ((bfp = subquery->ops_bfs.opb_base->opb_boolfact[bfi])->
		opb_mask & OPB_SPATJ && BTtest((i4)eqcls, 
		(char *)&bfp->opb_eqcmap))
	    {
		OPE_IEQCLS	eqc1;
		if ((eqc1 = BTnext((i4)-1, (char *)&bfp->opb_eqcmap,
		    (i4)subquery->ops_eclass.ope_ev)) == eqcls) 
		 eqc1 = BTnext((i4)eqc1, (char *)&bfp->opb_eqcmap, 
		    (i4)subquery->ops_eclass.ope_ev);
		if (BTtest((i4)eqc1, (char *)oeqcmp))
		{
		    eqcls = eqc1;
		    break;	/* kinda silly to let this drop into 
				** same test again - any alternative will
				** be even uglier! */
		}
	    }
	}

	if (BTtest((i4)eqcls, (char *)oeqcmp) == TRUE)
	{
	    /* Look no further if the first eqcls in the outer
	    ** that exists in the order for the relation is not the 
	    ** joining eqcls (if primkey)
	    */
	    if (primkey && (jeqc != eqcls))
	    {
#ifdef			xNTR1
		if (tTf(1, 0))
		{
		    TRdisplay("We tried to join on eqcls %d but realized that it\n", eqcls);
		    TRdisplay("was not the the primary attribute in the ordering and\n");
		    TRdisplay("that the primary attribute is available from the outer.\n");
		    uflush();
		}
#endif
		    break;
	    }
	    /* Condition satisfied. If non-hash we are keyed */
	    ret_val = TRUE;
	    primkey = FALSE;
	    if (storage != DB_HASH_STORE)
		break;
	    /* we are hashed and need all attrs in the key */
	    continue;
	}
	if (subquery->ops_bfs.opb_bfeqc
	    &&
	    BTtest((i4)eqcls, (char *)subquery->ops_bfs.opb_bfeqc))
	    continue;			/* check if constant key is
                                        ** available for this attribute*/
	/* eqcls is not available. If we haven't gotten all
	** of the hashed key, we can't use the key.
	*/
	if (storage == DB_HASH_STORE)
	{
#	ifdef		xNTR1
	    if (tTf(1, 0))
	    {
		TRdisplay("Not all keys for hashed lookup were found.\n");
		uflush();
	    }
#	endif
	    ret_val = FALSE;
	}
	break;
    }
# ifdef	xNTR1
    if (tTf(1, 0))
    {
	if (ret_val == TRUE)
		TRdisplay("We can do keyed lookup.\n");
	else
		TRdisplay("We can NOT do keyed lookup.\n");
	uflush();
    }
# endif
    return (ret_val);
}
