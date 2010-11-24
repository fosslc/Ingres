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
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <scf.h>
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
#include    <ex.h>
#define             OPS_GETCB           TRUE
#include    <opxlint.h>
/**
**
**  Name: OPSGETCB.C - get session control block from SCF
**
**  Description:
**      Routine to get the session control block from SCF
**
**  History:    
**      3-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	10-Jan-2001 (jenjo02)
**	    Use GET_OPS_CB macro instead of scf_call(SCU_INFORMATION)
**	    to fetch session's OPS_CB*.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPS_CB *ops_getcb(void);

/*{
** Name: ops_getcb	- routine to get session control block from SCF
**
** Description:
**      In some cases, such as an exception handler, it is not possible to 
**      to pass the session control block as a parameter.  This routine will 
**      call SCF to obtain the session control block.
**
** Inputs:
** Outputs:
**	Returns:
**	    ptr to session control block
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	10-Jan-2001 (jenjo02)
**	    Use GET_OPS_CB macro instead of scf_call(SCU_INFORMATION)
**	    to fetch session's OPS_CB*.
[@history_line@]...
*/
OPS_CB *
ops_getcb()
{
    CS_SID	sid;

    CSget_sid(&sid);
    return(GET_OPS_CB(sid));
}
