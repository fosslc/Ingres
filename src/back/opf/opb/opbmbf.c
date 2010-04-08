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
#define             OPB_MBFK             TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 

/**
**
**  Name: OPBMBF.C - find the matching boolean factors for a keylist
**
**  Description:
**      This file contains routines which find matching boolean factors
**      associated with a keylist of joinop attributes
**
**  History:    
**      3-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	26-oct-00 (inkdo01)
**	    Tidied by removing "ifdef 0"ed code and one commented bit. Changed
**	    to permit multi-att, non-hash keys to have multiple values 
**	    (IN clause) on 2nd or lower attribute.
**	25-Apr-2007 (kschendel) b122118
**	    Doug's above change basically no-op'ed the local state
**	    and priority definition stuff, so just remove it all.
**	    A note on the 26-Oct-00 change:  originally mbf would not
**	    recognize "a1 between lo and hi and a2 in (x,y,z)" if a1,a2
**	    formed a multi-column non-hash (range/isam) key.  This turns
**	    out to be a DMF restriction;  it has no ability to remember
**	    multi-eq values for subordinate key attributes, when it's
**	    doing a multi-column range.  (DMF would have to jump to the
**	    allowed a2 values each time a1 changed in its range from
**	    lo to hi, and DMF has no clue about such a thing.)  It still
**	    makes sense to let opb see the possibility, though.  opc
**	    ends up being the place (opc_range) which refuses to generate
**	    an outer range with an inner multi-equality.
[@history_line@]...
**/

/*{
** Name: opb_bestbf    - find best boolean factor for keying
**
** Description:
**      This routine is used to determine whether some boolean factor is 
**      useful in building a key, and selects the "best" boolean factor
**	among those available for the eqclass.  The best is very simply
**	chosen by the scheme: single equality is better than multi-equality
**	is better than ranges.
**
** Inputs:
**      bfi                             boolean factor number which contains
**                                      the OPB_BFGLOBAL structure
**      bfkeyp                          ptr to keyinfo structure which 
**                                      contains the OPB_BFGLOBAL information
**      rangeOk				TRUE if range BF's are OK;  this
**					would be passed as FALSE for hash
**	keydesc				Ptr to attr/bfi struct for a key attr
**
** Outputs:
**	Returns:
**	    TRUE if BF usable for keying was found, else FALSE
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates the BF index in the key attrs list, if usable.
**
** History:
**	4-may-86 (seputis)
**          initial creation from checktype in checktype.c
[@history_line@]...
*/
static bool
opb_bestbf(
	OPB_IBF            bfi,
	OPB_BFKEYINFO      *bfkeyp,
	bool		   rangeOk,
	OPB_ATBFLST        *keydesc)
{
    bool	usable;		/* TRUE if BF usable for keying */

    usable = TRUE;		/* Assume BF is usable */
    /* look at cases in priority order */
    if ( (bfkeyp->opb_global->opb_single != OPB_NOBF)) /* was there a single
                                    ** value sargable boolean factor found? */
    {
        keydesc->opb_gbf = bfkeyp->opb_global->opb_multiple; /* save index of
                                            ** boolean factor which has single
                                            ** key */
    }
    else if (
	     (bfkeyp->opb_global->opb_multiple != OPB_NOBF) /* was there a multi
                                            ** value sargable boolean factor
                                            ** found */
            )
    {
        keydesc->opb_gbf = bfkeyp->opb_global->opb_multiple; /* save index of
                                            ** boolean factor which has multiple
                                            ** keys */
    }
    else if ( rangeOk &&
             (bfkeyp->opb_global->opb_range != OPB_NOBF) /* was there a range 
                                            ** type boolean factor found */
            )
    {
        keydesc->opb_gbf = bfi;		    /* save the boolean factor index 
                                            ** which contains OPB_BFGLOBAL
                                            ** structure */
    }
    else 
    {
	usable = FALSE;		/* no useful boolean factor available */
    }

    return (usable);

}

/*{
** Name: opb_mbf	- find matching boolean factors for keylist
**
** Description:
**	See if there are the right matching boolean factors for
**	the given attributes so that they can be used to key the
**	relation.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      storage                         storage structure associated with
**                                      keylist
**      mbf                             matching boolean factor list
**                                      - contains keylist on which the
**                                      relation is ordered, but the matching
**                                      boolean factors have not been found
**
** Outputs:
**      mbf                             ptr to structure which will contain
**                                      the matching boolean factors for the
**                                      attributes in the dmfkeylist
**	Returns:
**	    TRUE if a limiting boolean factor has been found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	4-may-86 (seputis)
**
**	25-apr-94 (rganski)
**	    b59982 - added histdt parameter to call to opb_bfkget(). See
**	    comment in opb_bfkget() for more details.
**	26-oct-00 (inkdo01)
**	    Dropped use of OPB_SSINGLEALL since its sole purpose appeared
**	    to be to prevent IN-lists on lower order columns of multi-att,
**	    non-hash key structures. Just didn't make sense.
**	22-nov-01 (inkdo01)
**	    Check for "tid is null" and don't return key structure (these 
**	    may be outer join tricks and are useless for key access).
{@history_line@}...
*/
bool
opb_mbf(
	OPS_SUBQUERY       *subquery,
	OPO_STORAGE	   storage,
	OPB_MBF            *mbf)
{
    i4                  count;	/* count of number of active elements in the
                                ** the keylist
                                */

    mbf->opb_maorder = NULL;
    if ( (count = mbf->opb_count) <= 0)
        return(FALSE);		/* keylist does not exist so this relation
                                ** does not have a useful ordering for this
                                ** query
                                */
    if (storage == DB_HEAP_STORE)
        return(FALSE);		/* heap is not useful for limiting predicates*/
    
    {
	bool		rangeOk;
	i4                    keyi;	/* offset in opb_keyorder array of
                                        ** of attribute being analyzed
                                        */
	OPZ_AT                 *abase;  /* base of joinop attributes array */
        OPB_BFT                *bfbase; /* base of boolean factor array */
	OPE_ET                 *ebase;  /* base of equivalence class array */

	abase = subquery->ops_attrs.opz_base; 
        bfbase = subquery->ops_bfs.opb_base;
        ebase = subquery->ops_eclass.ope_base;
	rangeOk = (storage != DB_HASH_STORE);
	for (keyi = 0; keyi < count; keyi++)
	{
	    OPB_ATBFLST		*keydesc;/* ptr to key attribute being 
                                        ** analyzed */
	    OPZ_ATTS		*ap;	/* ptr to joinop attribute associated
                                        ** with key element
                                        */
	    OPE_IEQCLS		eqcls;	/* equivalence class of joinop
                                        ** attribute */
	    bool		usable;	/* class of best limiting boolean 
                                        ** factor found so far */

	    usable = FALSE;		/* Assume no limiting predicates */
	    keydesc = &mbf->opb_kbase->opb_keyorder[keyi];
	    ap = abase->opz_attnums[keydesc->opb_attno];
	    if ( (eqcls = ap->opz_equcls) >= 0) 
	    {	/* if equivalence class exists then check if a list of limiting
		** predicates is associated with the equivalence class
		*/
		OPB_IBF            bfi; /* boolean factor number which contains
                                        ** OPB_BFGLOBAL information */

		bfi = ebase->ope_eqclist[eqcls]->ope_nbf; /* get boolean factor
                                        ** which contains summary of information
                                        ** for this equivalence class */
                if (bfi != OPB_NOBF)	/* check if any boolean factors
                                        ** exist for this equivalence class */
		{
		    OPB_BOOLFACT    *bfp;/* ptr to boolean factor which
                                        ** contains the OPB_BFGLOBAL info
                                        ** for this equivalence class 
                                        */
		    OPB_BFKEYINFO   *bfinfop; /* ptr to bfkeyinfo for this
					** joinop attribute (type,length)
					*/
		    bfp = bfbase->opb_boolfact[bfi];

		    /* Check for "tid is null" - a predicate that's useless
		    ** for keying. */
		    if (count == 1 && 
			ebase->ope_eqclist[eqcls]->ope_eqctype == OPE_TID &&
			bfp->opb_qnode &&
			bfp->opb_qnode->pst_sym.pst_value.pst_s_op.pst_opno ==
			    subquery->ops_global->ops_cb->ops_server->opg_isnull)
		    {
			mbf->opb_bfcount = 0;
			return(FALSE);	/* show no key from TID */
		    }

		    bfinfop = opb_bfkget( subquery, bfp, eqcls, 
			&ap->opz_dataval, &ap->opz_histogram.oph_dataval, TRUE);
					/* get the keyinfo ptr
                                        ** for the type and length
                                        ** of the attribute, which
                                        ** contains the OPB_BFGLOBAL
                                        ** information for the
                                        ** attribute
                                        */
		    usable = opb_bestbf( bfi, bfinfop, rangeOk, keydesc);
		}
	    }
	    if (!usable)
	    {   /* boolean factor for attribute not found so stop with
		** what we have so far (if anything).
		*/
		if (storage == DB_HASH_STORE)
		{
		    mbf->opb_bfcount = 0;	/* boolean facts for
                                                ** all key attributes must exist
						** for hash key to be built
                                                */
		    return(FALSE);              /* key cannot be built */
		}
		mbf->opb_bfcount = keyi;        /* save the number of attributes
                                                ** which have useful keys */
		return( keyi > 0 );		/* partial key can be built */
	    }

	}
    }
    mbf->opb_bfcount = mbf->opb_count; /* all attributes have matching boolean
                                    ** factors */
    return( TRUE );		    /* boolean factors were found for key */
}
