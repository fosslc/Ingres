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
#define             OPZ_FINDATT         TRUE
#include    <opxlint.h>
/**
**
**  Name: OPZFINDATT.C - find if joinop attribute has been created
**
**  Description:
**      Routine to find if joinop attribute exists
**
**  History:    
**      19-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPZ_IATTS opz_findatt(
	OPS_SUBQUERY *subquery,
	OPV_IVARS joinopvar,
	OPZ_DMFATTR dmfattr);

/*{
** Name: opz_findatt	- find an existing joinop attribute
**
** Description:
**      This routine will find an existing JOINOP attribute number given
**      the relation attribute number
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**      joinopvar                       local range table variable number
**      dmfattno                        relation attribute number
**
** Outputs:
**	Returns:
**	    -negative result if joinop attribute could not be found
**          -joinop attribute number if found i.e. an index into
**          the ops_attr array, which will be >= 0.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-86 (seputis)
**          initial creation
**	 6-sep-04 (hayke02)
**	    Skip OPZ_COLLATT atributes. This change fixes problem INGSRV 2940,
**	    bug 112873.
**	 8-nov-05 (hayke02)
**	    Add OPZ_VAREQVAR if OPS_VAREQVAR is on and OPZ_COLLATT is not set.
**	    This ensures that OPZ_VAREQVAR is set for exisiting atts that were
**	    created when OPS_VAREQVAR was not on. This change fixes bug 115420
**	    problem INGSRV 3465.
[@history_line@]...
*/
OPZ_IATTS
opz_findatt(
	OPS_SUBQUERY       *subquery,
	OPV_IVARS          joinopvar,
	OPZ_DMFATTR        dmfattr)
{
    OPZ_IATTS		joinopattr; /* joinop attribute number */
    OPZ_AT              *abase;	    /* ptr to base of joinop attribute table */

    joinopattr = subquery->ops_attrs.opz_av; /* start with index of largest 
				    ** joinop attribute number */
    abase = subquery->ops_attrs.opz_base; /* base of joinop attributes array */
    while (joinopattr--)
    {	/* search the entire joinop attribute array if necessary */
	register OPZ_ATTS      *ap; /* ptr to joinop attribute struct */

	ap = abase->opz_attnums[joinopattr];
	if( ap->opz_varnm == joinopvar && ap->opz_attnm.db_att_id == dmfattr &&
	    !(ap->opz_mask & OPZ_COLLATT))
	{
	    if (subquery->ops_mask2 & OPS_VAREQVAR)
		ap->opz_mask |= OPZ_VAREQVAR;
	    return(joinopattr);	    /* return joinop attribute number >= 0 */
	}
    }
    return(OPZ_NOATTR);		    /* return a negative value if the joinop
				    ** attribute number could not be found
				    */
}
