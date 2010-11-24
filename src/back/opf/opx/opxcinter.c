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
#define        OPX_CINTER      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPXCINTER.C - check for interrupt
**
**  Description:
**      routine to check for user interrupt
**
**  History:    
**      14-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
OPN_STATUS opx_cinter(
	OPS_SUBQUERY *subquery);

/*{
** Name: opx_cinter	- check for interrupt
**
** Description:
**      This routine will check for the asynchronous abort and  
**      terminate the optimization and return with an error code indicating 
**      a soft abort.
**
** Inputs:
**      global                          ptr to global state variable
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    generates an internal exception if a soft abort is detected
**
** Side Effects:
**	    may not return
**
** History:
**	14-jun-86 (seputis)
**          initial creation from check_interrupt
**	19-nov-99 (inkdo01)
**	    Support opn_exit which doesn't EXsignal (to avoid kernel overhead).
**	28-jul-04 (hayke02)
**	    Call opn_exit() with a FALSE longjump.
[@history_line@]...
*/
OPN_STATUS
opx_cinter(
	OPS_SUBQUERY	    *subquery)
{
    
    OPS_CB          *scb;               /* ptr to session control block for
                                        ** currently active user */
    scb = subquery->ops_global->ops_cb;
    if (scb->ops_interrupt)
    {
	scb->ops_interrupt = FALSE; /* reset interrupt flag */
	if (scb->ops_check 
            && 
            opt_strace( scb, OPT_F046_CONTROLC)
	    )
	{
	    opn_exit(subquery, FALSE);	/* trace flag to allow QEP to be
					** printed */
	    return(OPN_SIGEXIT);
	}
	else
	    opx_error(E_OP0003_ASYNCABORT); /* generate an error indicating
					    ** an asynchronous abort was
                                            ** detected */
    }
    return(OPN_SIGOK);
}
