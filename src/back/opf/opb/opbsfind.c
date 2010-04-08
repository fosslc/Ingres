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

#define             OPB_SFIND           TRUE
#include    <opxlint.h>
/**
**
**  Name: OPBSFIND.C - find if equivalence class is sargable in boolean factor
**
**  Description:
**      Routine which finds if equivalence class is sargable in the boolean
**      factor.
**
**  History:    
**      21-may-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opb_sfind	- test if eqclass exists in sargable clause in boolfact
**
** Description:
**      See if eqclass exists in a sargable clause in this boolfact.  Thus,
**      the boolean factor must be of the form (a op const1) OR (b op const2)...
**      i.e. at least one attribute must be compared against a constant.  
**      This routine will check this fact.
**      FIXME - this routine can be replaced by a map attached to each boolean
**      factor of the sargable clauses in the boolean factor.  This is probably
**      not very useful since the keyinfo list do not get very long.
**
** Inputs:
**      bp                              ptr to boolean factor element to be
**                                      analyzed
**      eqcls                           equivalence class which must be
**                                      sargable inside the boolean factor
**
** Outputs:
**	Returns:
**	    TRUE - if the equivalence is sargable inside the boolean factor "bp"
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-may-86 (seputis)
{@history_line@}...
*/
bool
opb_sfind(
	OPB_BOOLFACT       *bp,
	OPE_IEQCLS         eqcls)
{
    OPB_BFKEYINFO       *bfkeyp;    /* used to traverse the list of keys
                                    ** associated with the boolean factor */

    /* FIXME - eliminate this search by setting flag in bp structure */
    for (bfkeyp = bp->opb_keys; bfkeyp; bfkeyp = bfkeyp->opb_next)
    {
	if ((bfkeyp->opb_eqc == eqcls) && bfkeyp->opb_keylist)
	    return (TRUE);
    }
    return (FALSE);
}
