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
#define        OPN_DELTRL      TRUE
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPNDELTRL.C - delete OPN_RLS struc and its histograms
**
**  Description:
**      delete an OPN_RLS struct and its histograms 
**
**  History:    
**      14-jun-86 (seputis)    
**          initial creation from deltrl.c
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_deltrl(
	OPS_SUBQUERY *subquery,
	OPN_RLS *trl,
	OPE_IEQCLS jeqc,
	OPN_LEAVES nleaves);

/*{
** Name: opn_deltrl	- delete a OPN_RLS struct and its associated histograms
**
** Description:
**      This routine will delete an OPN_RLS structure and its' associated
**      histograms, including the "cell count" array of any histogram on
**      the joining equivalence class.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      trl                             OPN_RLS struct to delete
**      jeqc                            joining equivalence class which
**                                      will have a histogram to delete
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
**	14-jun-86 (seputis)
**          initial creation from deltrl
**	20-dec-90 (seputis)
**	    add support for deleting multi-attribute join histograms
**	19-nov-01 (inkdo01)
**	    Add logic to resolve linkage around OPN_RLS deleted from chain.
[@history_line@]...
*/
VOID
opn_deltrl(
	OPS_SUBQUERY       *subquery,
	OPN_RLS            *trl,
	OPE_IEQCLS         jeqc,
	OPN_LEAVES	   nleaves)
{
    OPZ_AT              *abase;			/* ptr to base of array of ptrs
                                                ** to joinop attributes */
    OPH_HISTOGRAM       *histp;			/* histogram element to
                                                ** deallocate */
    OPH_HISTOGRAM       *nexthistp;             /* need to save next histogram
                                                ** ptr in case memory manager
                                                ** munches the deallocated
                                                ** previous histogram */
    OPE_BMEQCLS		*eqcmap;		/* map of joining equivalence classes */
    OPN_RLS		*prevrls;

    abase = subquery->ops_attrs.opz_base;	/* ptr to base of array of ptrs
                                                ** to joinop attributes */
    if (jeqc >= subquery->ops_eclass.ope_ev)
    {	/* multi-attribute join found */
	eqcmap = subquery->ops_msort.opo_base->opo_stable[jeqc-subquery->ops_eclass.ope_ev]->opo_bmeqcls;
    }
    else
	eqcmap = (OPE_BMEQCLS *)NULL;
    for (histp = trl->opn_histogram; histp; histp = nexthistp)
    {
	OPZ_ATTS               *attrp;		/* ptr to joinop attribute
                                                ** associated with histogram */

	nexthistp = histp->oph_next;		/* save next histogram prior
                                                ** to deallocation */
	attrp = abase->opz_attnums[histp->oph_attribute];
	if ((attrp->opz_equcls == jeqc)
	    ||
	    (	eqcmap
		&&
		BTtest((i4)attrp->opz_equcls, (char *)eqcmap)
	    ))
	{
	    oph_dccmemory( subquery, &attrp->opz_histogram, histp->oph_fcnt);
	    /* can only free the fcnt array if it is on the joining
	    ** eqclass because then we know it was created brand new by
	    ** opn_h2 and does not point to another hist's oph_fcnt. (see
	    ** line "newhp->oph_fcnt=lnp->oph_fcnt" in opn_h2.c 
	    */
	}
	oph_dmemory(subquery, histp);
    }
    /* Locate trl in chain and remove it before deleting. */
    prevrls = subquery->ops_global->ops_estate.opn_sbase->opn_savework[nleaves];
    if (prevrls && prevrls == trl)
	subquery->ops_global->ops_estate.opn_sbase->opn_savework[nleaves] =
			trl->opn_rlnext;
    else if (prevrls)
    {
	for (; prevrls && prevrls->opn_rlnext != trl; prevrls = prevrls->opn_rlnext);
	if (prevrls)
	    prevrls->opn_rlnext = trl->opn_rlnext;	/* skip dropped OPN_RLS */
    }
    opn_drmemory(subquery, trl);
}
