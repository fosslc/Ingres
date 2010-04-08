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
#define        OPN_EQTAMP      TRUE
#include    <bt.h>
#include    <opxlint.h>


/**
**
**  Name: OPN_EQTAMP.C - equivalence class to joinop attribute map
**
**  Description:
**      routines which convert an equivalence class to a joinop attribute 
**      given a histogram list
**
**
**  History:    
**      23-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/


/*{
** Name: opn_eqtamp	- return histogram for attribute with given eqclass
**
** Description:
**	 Search a linked list of OPH_HISTOGRAM structs for the one
**	 with eqclass eqcls and return pointer to the pointer which
**	 points to this struct (so that the struct can be 
**	 easily deleted from the list if desired).
**
**       FIXME - this routine should be replaced by a direct access structure
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      hp                              ptr to beginning of list of histograms
**      eqcls                           equivalence class of requested histogram
**      mustfind                        TRUE means the requested histogram must
**                                      be found or a exception should be
**			                generated
**
** Outputs:
**	Returns:
**	    ptr to ptr to OPH_HISTOGRAM structure (so it can be unlinked)
**          of the histogram associated with equivalence class eqcls
**	Exceptions:
**	    none
**
** Side Effects:
**	    consistency check failure could generate internal exception and
**          abort the optimization
**
** History:
**	23-may-86 (seputis)
**          initial creation
**	28-sep-00 (inkdo01)
**	    Code around composite histograms.
**	16-aug-05 (inkdo01)
**	    Add support of multi-EQC attrs.
**	20-nov-2006 (dougi)
**	    Guard against bad attribute numbers.
**	21-nov-2006 (dougi)
**	    Removed above change in favour of better solution.
**	10-Mar-2009 (kibro01) b121740
**	    Allow a check of whether a histogram is in use for multiple
**	    histograms, so we know not to remove it from the list.  Keep
**	    opn_eqtamp the same, but add a new opn_eqtamp_multi with the 
**	    required extra parameter.
*/
OPH_HISTOGRAM **
opn_eqtamp(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      **hp,
	OPE_IEQCLS         eqcls,
	bool               mustfind)
{
	return (opn_eqtamp_multi(subquery,hp,eqcls,mustfind,NULL));
}

OPH_HISTOGRAM **
opn_eqtamp_multi(
	OPS_SUBQUERY       *subquery,
	OPH_HISTOGRAM      **hp,
	OPE_IEQCLS         eqcls,
	bool               mustfind,
	bool		   *multiple_eqcls)
{
    OPH_HISTOGRAM       **histp;    /* used to traverse the list of histograms
                                    ** looking for one with this equivalence
                                    ** class */
    OPZ_AT              *abase;     /* ptr to base of array of ptrs to joinop
                                    ** attributes */

    abase = subquery->ops_attrs.opz_base; /* get base of array of ptrs to
                                    ** joinop attribute elements
                                    */
    if (multiple_eqcls)
	*multiple_eqcls = FALSE;
    for (histp = hp; *histp; histp = &((*histp)->oph_next))
	if (!((*histp)->oph_mask & OPH_COMPOSITE) &&
	   (abase->opz_attnums[(*histp)->oph_attribute]->opz_equcls == eqcls
	    ||
	    BTtest((i4)eqcls, (char *)&abase->opz_attnums[
			(*histp)->oph_attribute]->opz_eqcmap)
	    ||
	    subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->ope_mask &
								OPE_SPATJ &&
	    BTtest((i4)(*histp)->oph_attribute, 
		(char *)&subquery->ops_eclass.ope_base->ope_eqclist[eqcls]->
		ope_attrmap)))
	{
	    if (multiple_eqcls &&
		BTcount((char *)&abase->opz_attnums[(*histp)->oph_attribute]->
			opz_eqcmap,subquery->ops_eclass.ope_ev) > 1)
		*multiple_eqcls = TRUE;
	    return (histp);
	}

#ifdef    E_OP0482_HISTOGRAM
    if (mustfind)                   /* histogram for equivalence class not
                                    ** found when expected */
	opx_error(E_OP0482_HISTOGRAM);
#endif
    return (NULL);
}
