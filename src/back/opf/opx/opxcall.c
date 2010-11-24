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
#define	    OPX_CALL	TRUE
#include    <opxlint.h>

/**
**
**  Name: OPXCALL.C - PROCESS AN INTERRUPT OR EXCEPTION
**
**  Description:
**      This file will contain all the procedures necessary to support the
**      opx_call entry point to the optimizer facility.  The call can be
**      can be an asynchronous interrupt of the OPF, such as a control-C
**      from a user.
**
**  History:    
**      31-dec-85 (seputis)    
**          initial creation
**      15-dec-91 (seputis)
**          make linting fix, used for parameter checking
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 opx_call(
	i4 eventid,
	PTR opscb);
i4 opx_init(
	OPF_CB *opf_cb);

/*{
** Name: opx_call - Process an OPF asynchronous interrupt
**
** External call format:
**	STATUS opx_call((i4)eventid, (PTR)ops_cb);
**
** Description:
**      This entry point should only be called by SCF and it handles
**      an asynchronous interrupt of the OPF, such as a control-C
**      from a user.  The control-C interrupt will only affect the session
**      selected.  A local flag will be set by this routine in the respective
**      session control block and the routine will return.  This
**      session local flag will be monitored periodically by the OPF 
**      and if this flag  is set then an orderly stoppage of that session's 
**      operation will be made.  
**         Interrupts will be disabled when critical data structures
**      are being updated, so that these structures will not be
**      left in an inconsistent state.
**
** Inputs:
**      eventid                      - indication of event
**      ops_cb                       - pointer to OPF session control block
**                                     of session which was interrupted
** Outputs:
** 
** Side Effects:
**	The session control block interrupt flag will be updated to indicate
**      that an interrupt has occurred.
** History:
**	31-dec-85 (seputis)
**          initial creation
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	20-Apr-2004 (jenjo02)
**	    Changed to return STATUS consistent with SCF.
[@history_template@]...
*/

STATUS
opx_call(
	i4		   eventid,
	PTR		   opscb)
{
    ((OPS_CB *)opscb)->ops_interrupt = TRUE;	/* set flag indicating interrupt has
                                        ** occurred */
    return(OK);
}

/*{
** Name: opx_init	- initialize the AIC and PAINE handlers
**
** Description:
**      This routine will inform SCF of the AIC and PAINE handlers
**
** Inputs:
**	opf_cb				    - ptr to caller's control block
**
** Outputs:
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR - if initialization failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      31-mar-87 (seputis)
**          initial creation
**      12-dec-88 (seputis)
**	    return E_DB_OK
[@history_template@]...
*/
DB_STATUS
opx_init(
	OPF_CB		*opf_cb)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_OPF_ID;
    scf_cb.scf_ptr_union.scf_afcn = opx_call;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_nbr_union.scf_amask = SCS_PAINE_MASK;
    scf_status = scf_call(SCS_DECLARE, &scf_cb);
# ifdef E_OP0090_PAINE
    if (DB_FAILURE_MACRO(scf_status))
    {
	opx_rverror( opf_cb, scf_status, E_OP0090_PAINE,
	    scf_cb.scf_error.err_code);
	return(E_DB_ERROR);
    }
# endif
    scf_cb.scf_nbr_union.scf_amask = SCS_AIC_MASK;
    scf_status = scf_call(SCS_DECLARE, &scf_cb);
# ifdef E_OP0091_AIC
    if (DB_FAILURE_MACRO(scf_status))
    {
	opx_rverror( opf_cb, scf_status, E_OP0091_AIC,
	    scf_cb.scf_error.err_code);
	return(E_DB_ERROR);
    }
# endif
    return(E_DB_OK);
}
