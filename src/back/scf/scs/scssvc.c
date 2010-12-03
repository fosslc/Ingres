/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <erglf.h>
#include    <gl.h>
#include    <er.h>
#include    <cs.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <sl.h>
#include    <cv.h>
#include    <tr.h>
#include    <sp.h>
#include    <mo.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>

#include    <dmccb.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>
#include    <psfparse.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <sc0e.h>
#include    <scc.h>
#include    <sce.h>
#include    <scs.h>
#include    <scd.h>
#include    <scfcontrol.h>
#include    "scsint.h"

/**
**
**  Name: SCSSVC.C - SCF Sequencer Services file
**
**  Description:
**      This file contains the entry points for the services provided 
**      to SCF users by the sequencer module.  These services are primarily 
**      concerned with Asynchronous Ingres Call and Priority Asynchronous 
**      INgres Events (a.k.a. AICs and PAINEs).  Actually, PAINES will be 
**      primarily the responsibility of the dispatcher; for code compactness 
**      reasons, though, they will be dealt with here. 
** 
**      Asynchronous events within an ingres session typically refer 
**      to some out of band communication from the user program to the 
**      DBMS (e.g. a user interrupt or ENDRETRIEVE call).  However, in 
**      the future it may be desirable to allow some other asynchronous 
**      events, so the method provided has been generalized to some 
**      extent. 
** 
**      All asynchronous functions (or AFCNs) are expected to be declared 
**      as follows. 
** 
**	    VOID
**	    <afcn_name>(event_id, whatever_cb)
**	    i4		event_id;
**	    WHATEVER_CB		*whatever_cb; 
** 
**      where at call time, event_id will be an event identifier
**      specifying the event which occured and whatever_cb will be 
**      the address of the session control block for which the event 
**      took place.
**
**          scs_declare() - declare an aic/paine
**          scs_clear() - delete an aic/paine
**          scs_enable() - enable aic delivery
**	    scs_disable() - disable aic delivery
**	    scs_attn()	-   the aic/paine delivered to scf, which calls all
**				others.
**	    scs_format() - Send "format <session_id>" output to gca session
**			   (Used by iimonitor)
**	    scs_avformat() - Send "format <session_id>" output to the errlog
**			   (Used by various exception handlers)
**	    scs_facility() - Return information about the session's current facility.
**
**
**  History:
**      20-Jan-86 (fred)    
**          Created on Jupiter
**	22-Mar-89 (fred)
**	    Added use of sc_proxy_scb for performing work on behalf of other,
**	    non-running sessions.
**	10-Dec-90 (neil)
**	    Added CS_NOTIFY_EVENT handling in scs_attn for alerters.
**      02-feb-9191 (jennifer)
**          Fix bug B32476, treat assoc fail like control c.
**	10-May-1991 (fred)
**	    Changed interrupt handling -- see scs_i_interpret().
**	12-jul-1991 (ralph)
**	    Fix bug 37827 -- process -SCS_INTERRUPT.
**          Fix bug 37774 -- If GCA status other than OK, TIMEOUT, or PURGED,
**          process as interrupt.  Fix for bug 32476 assumed only ASSOC FAILs
**          should be treated as interrupts.
**	18-aug-1991 (ralph)
**	    scs_attn() - Interrupt facilities when CS_REMOVE_EVENT
**	13-jul-1992 (rog)
**	    Moved scs_format into this file from scsmntr.c and added
**	    scs_iformat and scs_avformat.  Included ddb.h for Sybil.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	30-dec-92 (robf)
**	    Add SXF to list of facilities.
**	    Print full terminal name (should be 16, not 8), to match that
**	    in SAL, also move to its own line since too long to fit.
**	    Display Monitor session info where applicable (like
**	    terminal name)
**	12-Mar-1993 (daveb)
**	    Add include <st.h>
**      09-apr-1993 (schang)
**          add gwf interrupt support
**	29-Jun-1993 (daveb)
**	    include <tr.h>, correctly cast arg to CSget_scb().
**	2-Jul-1993 (daveb)
**	    prototyped, removed func externs in headers.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	14-Sep-1993 (daveb)
**	    Honor pending drops in scs_i_interpret.
**	09-mar-94 (swm)
**	    Bug #60425
**	    CS_SID was displayed with %x rather than %p, %x truncates
**	    on 64-bit platforms.
**      20-Apr-1994 (daveb)
**          Change scs_iformat to show/decode new client information
**  	    in the gw_parms.
**      14-jun-95 (lawst01)
**          Added SCF_misc_sem semaphore to protect critical sections. (MCT)
**	11-sep-95 (wonst02)
**	    Add scs_facility() for info (to cssampler) re: the session's facility. 
**	03-jun-1996 (canor01)
**	    Remove SCF_misc_sem.
**	02-Jun-1998 (jenjo02)
**	    Remove test of scb_cscb.cscb_comm_size < 256; internal threads,
**	    factotum threads may not have a scb_cscb allocated, so the 
**	    results of this check are unpredictable at best.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      24-Sep-1999 (hanch04)
**          Call ERmsg_hdr to format error message header.
**      17-nov-1999 (mosjo01) SIR 97564
**          Reposition cs.h before ex.h avoids compiler errors with __jmpbuf
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scs_dbadduser(), scs_dbdeluser(), scs_declare(),
**	    scs_clear(), scs_disable(), scs_enable(),
**	    scs_atfactot().
**	09-Aug-2002 (bolke01)
**	    Enabled the display of the full query in iimonitor in blocks of 939
**	    bytes.  This resolves bug (108487) sub-part of 84396.
**      27-Apr-2003 (hanal04) Bug 112209 INGSRV2807
**          Prevent SEGVs when attempting to dump large query buffers that
**          are no longer present.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new forms of sc0ePut(), uleFormat().
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype static scs_gwinfo_subget()
**/

# define USER_STRING	ERx("user=")
# define HOST_STRING	ERx("host=")
# define PID_STRING	ERx("pid=")
# define TTY_STRING	ERx("tty=")
# define CONN_STRING	ERx("conn=")
# define ER_HDR_OFFSET	61
/* Offset for header = 48 bytes ( MHDR ) + 12 bytles ("   Query:   ") +  1 */

/*
**  Forward and/or External function references.
*/

/* (none) */


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF SC_MAIN_CB          *Sc_main_cb;

static STATUS scs_gwinfo_subget( char *substr,
				 SCD_SCB *scb,
				 i4  lbuf,
				 char *buf );

/*{
** Name: scs_declare	- declare an asynchronous event function
**
** Description:
**      This function allow the user to specify an aic or paine handler
**      to SCF.  Upon receipt of any asynchronous event for the session, 
**      SCF will deliver the aic to the facility (via the handler) as
**	specified below.  By default, SCF will deliver aic's only if the
**      facility in question is running.  E.g. if we are executing a query 
**      and a user interrupt arrives, an aic will be delivered to 
**      QEF (presuming it requested one) but not to the parser 
**      (same presumption).  However, if the user specifies the 
**      SCS_ALWAYS_MASK, then the aic will be delivered regardless 
**      of the facilities activation.  This capability is expected
**      to be used only by DMF;  it is provided because DMF is expected
**      to terminate some long operations in the event of an interrupt
**      (e.g. a sort) but is rarely directly activated by the sequencer.
**
**      All asynchronous functions (or AFCNs) are expected to be declared 
**      as follows. 
** 
**	    VOID
**	    <afcn_name>(event_id, whatever_cb)
**	    i4		event_id;
**	    WHATEVER_CB		*whatever_cb; 
** 
**      where at call time, event_id will be an event identifier
**      specifying the event which occurred and whatever_cb will be 
**      the address of the session control block for which the event 
**      took place.
**
** Inputs:
**      SCS_DECLARE                     op code to scf
**      scf_cb
**          .scf_facility		facility identifier of caller
**					    (defined in DBMS.h)
**          .scf_nbr_union.scf_amask    operation code extensions
**		SCS_AIC_MASK            address provided is an AIC
**		SCS_PAINE_MASK          address provided is a PAINE
**		SCS_ALWAYS_MASK         function should be called regardless
**					    of facility activation (DMF ONLY)
**          .scf_afcn                   address of AIC/PAINE function as described
**					    above
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or ...
**		    E_SC000E_AFCN_ADDR	afcn is not a valid address
**		    E_SC0010_NO_PERM	facility is not authorized to use the
**					    SCS_ALWAYS_MASK function
**	Returns:
**	    E_DB_{OK,ERROR, FATAL}
**	Exceptions:
**
**
** Side Effects:
**	    Not a side effect, but a comment.  This function is provided here
**	    because it is the sequencer who will be delivering user level
**	    AIC's.  This function can be performed only by the sequencer
**	    since the dispatcher has no idea what user facility is active.
**	    PAINE's will be delivered by the dispatcher, but are included
**	    at this level to group similar functions for the user.
**
** History:
**	20-Jan-86 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_line@]...
**	6-jul-93 (shailaja)
**	    Fixed prototype incompatibilities.
*/
DB_STATUS
scs_declare(SCF_CB *scf_cb, SCD_SCB *scb )
{
    CLRDBERR(&scf_cb->scf_error);

    if (scf_cb->scf_ptr_union.scf_afcn == 0)
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC000E_AFCN_ADDR);
	return(E_DB_ERROR);
    }

    if (scf_cb->scf_nbr_union.scf_amask &
	    ~(SCS_AIC_MASK | SCS_PAINE_MASK | SCS_ALWAYS_MASK))
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC0019_BAD_FUNCTION_CODE);
	return(E_DB_ERROR);
    }

    if (scf_cb->scf_nbr_union.scf_amask & SCS_AIC_MASK)
    {
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_addr =
	    scf_cb->scf_ptr_union.scf_afcn;
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_mode =
	    scf_cb->scf_nbr_union.scf_amask &
		    (SCS_AIC_MASK | SCS_ALWAYS_MASK);
    }
    if (scf_cb->scf_nbr_union.scf_amask & SCS_PAINE_MASK)
    {
	Sc_main_cb->sc_plist.sc_afcn[scf_cb->scf_facility].afcn_addr =
	    scf_cb->scf_ptr_union.scf_afcn;
	Sc_main_cb->sc_plist.sc_afcn[scf_cb->scf_facility].afcn_mode =
	    scf_cb->scf_nbr_union.scf_amask &
		    (SCS_PAINE_MASK);
    }
    return(E_DB_OK);
}

/*{
** Name: scs_clear	- clear the AIC/PAINE address
**
** Description:
**      This function is the opposite of the declare function 
**      above.  It removes the AIC/PAINE from eligibility for  
**      delivery.
**
** Inputs:
**      SCS_CLEAR                       op code to scf_call
**      scf_cb
**          .scf_facility               facility to clear addr for
**          .scf_nbr_union.scf_amask                  
**              SCS_AIC_MASK            clear the AIC address
**              SCS_PAINE_MASK          clear the PAINE address
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK
**	Returns:
**	    E_DB_{OK, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Jan-86 (fred)
**          Created on jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
DB_STATUS
scs_clear(SCF_CB *scf_cb, SCD_SCB *scb )
{
    CLRDBERR(&scf_cb->scf_error);

    if (scf_cb->scf_nbr_union.scf_amask &
	    ~(SCS_AIC_MASK | SCS_PAINE_MASK))
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC0019_BAD_FUNCTION_CODE);
	return(E_DB_ERROR);
    }

    if (scf_cb->scf_nbr_union.scf_amask & SCS_AIC_MASK)
    {
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_addr = 0;
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_mode = 0;
    }
    if (scf_cb->scf_nbr_union.scf_amask & SCS_AIC_MASK)
    {
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_addr = 0;
	Sc_main_cb->sc_alist.sc_afcn[scf_cb->scf_facility].afcn_mode = 0;
    }
    return(E_DB_OK);
}

/*{
** Name: scs_disable	- disable AIC delivery
**
** Description:
**      This function temporarily disables AIC delivery for the entire 
**      session.  SCS_ENABLE's and SCS_DISABLE's do not nest. 
** 
**      In the event that a facility disables AIC's, all AIC's for 
**      that facility will be queued and delivered when AIC's are 
**      enabled again.
**
** Inputs:
**      SCS_DISABLE                     op code to scf_call()
**      scf_cb                          no fields used for input
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or...
**                  E_SC0011_NESTED_DISABLE attempt to disable when
**					    already disabled.
**
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    Note:  PAINE's are never disabled (otherwise they wouldn't be a pain)
**
** History:
**	20-Jan-86 (fred)
**          Created on Jupiter
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb().
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
DB_STATUS
scs_disable(SCF_CB *scf_cb, SCD_SCB *scb )
{
    if (scb->scb_sscb.sscb_cfac >= 0)
    {
	scb->scb_sscb.sscb_cfac = -scb->scb_sscb.sscb_cfac;
	CLRDBERR(&scf_cb->scf_error);
	return(E_DB_OK);
    }
    else
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC0011_NESTED_DISABLE);
	return(E_DB_WARN);
    }
}

/*{
** Name: scs_enable	- enable AIC delivery
**
** Description:
**      This function allows AIC delivery to commence.  If any AIC's 
**      have been queued since disabling, they are delivered now.
**
** Inputs:
**      SCS_ENABLE                      op code to scf_call()
**      scf_cb                          no fields used for input
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or...
**                  E_SC0012_NOT_DISABLED not particularly a problem, but
**					    included for completeness and to
**					    cause tracking down since it is
**					    sloppy
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    All queued AIC's are delivered now.
**
** History:
**	20-Jan-86 (fred)
**          Created for jupiter.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb().
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_line@]...
*/
DB_STATUS
scs_enable(SCF_CB *scf_cb, SCD_SCB *scb )
{
    DB_STATUS		status;


    if (scb->scb_sscb.sscb_cfac < 0)
    {
	CLRDBERR(&scf_cb->scf_error);
	status = E_DB_OK;
    }
    else
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC002A_NOT_DISABLED);
	status = E_DB_WARN;
    }
    scb->scb_sscb.sscb_cfac = abs(scb->scb_sscb.sscb_cfac);
    if (scb->scb_sscb.sscb_interrupt == SCS_PENDING_INTR)
    {
	scs_attn(scb->scb_sscb.sscb_eid, scb);
    }
    return(status);
}

/*{
** Name: scs_attn - Inform SCF that an attention request has arrived
**
** Description:
**      This routine is called asynchronously (by CLF\CS) to inform SCF 
**      that an attention request has been delivered.  If the attention 
**      request is for a particular session, then the scb parameter will 
**      be filled with the scb of that session;  otherwise, this parameter 
**      will be zero.  In any case, the eid parameter will contain the 
**      identification of the event type.
**
** Inputs:
**      eid                             Event ID -- from <cs.h>
**      scb                             Session control block if the eid
**                                      is for a session.
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    The event is noted, if applicable.  Other facilities' AIC and PAINE
**	    handlers are called as required.
**
** History:
**      24-feb-1987 (fred)
**          Created.
**	10-dec-1990 (neil)
**	    Added CS_NOTIFY_EVENT handling for alerters.
**	12-jul-1991 (ralph)
**	    Fix bug 37827 -- process -SCS_INTERRUPT.
**      18-aug-1991 (ralph)
**	    scs_attn() - Interrupt facilities when CS_REMOVE_EVENT
**      09-apr-1993 (schang)
**          add gwf interrupt support
**	2-Jul-1993 (daveb)
**	    prototyped.
**	25-jun-98 (stephenb)
**	    Add code to support CS_CTHREAD_EVENT
**	24-Apr-2003 (jenjo02)
**	    Don't set SCS_FAPENDING if already force-aborting,
**	    SIR 110141.
**	03-Dec-2003 (jenjo02)
**	    If no fac_scb, don't make the call. This is a normal
**	    condition for some special threads, like dmrAgents.
**	20-Apr-2004 (jenjo02)
**	    For Logfull Commit during force abort, let DMF
**	    decide if it alone will handle the abort or
**	    if all facilities should get involved.
**	4-May-2004 (schka24)
**	    Don't set sscb_interrupt for CUT thread attn, sequencer
**	    thinks sscb_interrupt means a *user* interrupt.
**	10-May-2004 (jenjo02)
**	    Added CS_KILL_EVENT to kill a query (SIR 110141).
**	11-Sep-2006 (jonj)
**	    Return FAIL when DMF indicates it'll handle CS_RCVR_EVENT
**	    ON LOGFULL COMMIT protocol. FAIL tells CSattn to do nothing
**	    else with the CS_SCB state, just keep going as if nothing
**	    happened.
[@history_template@]...
*/
STATUS
scs_attn(i4 eid,
	 SCD_SCB *scb )
{
    DB_STATUS		status;
    i4                  fac;
    SCD_ALIST		*list;
    void		*fac_scb;
    
    list = &Sc_main_cb->sc_alist;
    if (eid & CS_SERVER_EVENT)
    {
	list = &Sc_main_cb->sc_plist;
    }
    else if (eid & CS_NOTIFY_EVENT)
    {
	/* This is an alert interrupt, simply return */
	return (OK);
    }
    else if (!scb)
    {
	sc0ePut(NULL, E_SC0229_INVALID_ATTN_CALL, NULL, 0);
	return(FAIL);
    }
    else if (scb->scb_sscb.sscb_cfac < 0)
    {
	/*
	** Then AIC's are disabled.  We mark that one came in,
	** and forward them in the future
	*/

	scb->scb_sscb.sscb_eid = eid;
	scb->scb_sscb.sscb_interrupt = SCS_PENDING_INTR;
    }
    else
    {
	if (eid == CS_ATTN_EVENT)
	{
	    status = scs_i_interpret(scb);
	    if (status)
		return(status);
	    scb->scb_sscb.sscb_nointerrupts++;
	}
	else if (eid == CS_CTHREAD_EVENT)
	{
	    /* Nothing.  Do NOT set sscb_interrupt, as sequencer will
	    ** get it confused with a user interrupt.  Pass the CUT
	    ** attention on to the facilities, though.
	    */
	}
	else if (eid == CS_RCVR_EVENT)
	{
	    /*
	    ** ForceAbort: First call DMF's handler.
	    ** It'll return FAIL if the session is
	    ** in LOGFULL COMMIT
	    ** state in which case we don't invoke
	    ** the other FACs handlers, and let DMF
	    ** alone handle it.
	    */
	    if ( list->sc_afcn[DB_DMF_ID].afcn_addr &&
		 scb->scb_sscb.sscb_dmscb )
	    {
		if ( (*(list->sc_afcn[DB_DMF_ID].afcn_addr))
			(eid, scb->scb_sscb.sscb_dmscb) )
		{
		    /* Only DMF will handle ForceAbort */

		    /* "FAIL" tells CSattn to ignore this interrupt */
		    return(FAIL);
		}
	    }
	    /* Everyone gets involved */
	    scb->scb_sscb.sscb_force_abort = SCS_FAPENDING;
	}
	else if (eid == CS_SHUTDOWN_EVENT)
	{
	    /*
	    ** Server is being shutdown - mark state so no error will
	    ** be signalled when server task threads are terminated.
	    */
	    Sc_main_cb->sc_state = SC_SHUTDOWN;
	}

	if ((scb->scb_sscb.sscb_interrupt == SCS_INTERRUPT)
	    || (scb->scb_sscb.sscb_interrupt == -SCS_INTERRUPT)
	    || (scb->scb_sscb.sscb_force_abort == SCS_FAPENDING)
	    || (eid == CS_REMOVE_EVENT)
	    || (eid == CS_CTHREAD_EVENT)
	    || (eid == CS_KILL_EVENT))
	{
	    for (fac = DB_MIN_FACILITY;
		    fac <= DB_MAX_FACILITY;
		    fac++)
	    {
		if ((list->sc_afcn[fac].afcn_mode &
					    (SCS_PAINE_MASK | SCS_ALWAYS_MASK))
			|| (scb && (scb->scb_sscb.sscb_cfac == fac) &&
				(list->sc_afcn[fac].afcn_addr)))
		{
		    fac_scb = scb;
		    if (scb)
		    {
			switch (fac)
			{
			    case DB_ADF_ID:
				fac_scb = scb->scb_sscb.sscb_adscb;
				break;
			    case DB_DMF_ID:
				fac_scb = scb->scb_sscb.sscb_dmscb;
				break;
			    case DB_OPF_ID:
				fac_scb = scb->scb_sscb.sscb_opscb;
				break;
			    case DB_PSF_ID:
				fac_scb = scb->scb_sscb.sscb_psscb;
				break;
			    case DB_RDF_ID:
				fac_scb = scb->scb_sscb.sscb_rdscb;
				break;
			    case DB_QEF_ID:
				fac_scb = scb->scb_sscb.sscb_qescb;
				break;
			    case DB_QSF_ID:
				fac_scb = scb->scb_sscb.sscb_qsscb;
				break;
			    case DB_SCF_ID:
				fac_scb = scb->scb_sscb.sscb_scscb;
				break;
			    case DB_GWF_ID:    /* schang : */
				fac_scb = scb->scb_sscb.sscb_gwscb;
				break;
			    default:
				continue;
			}
		    }
		    (VOID)(*(list->sc_afcn[fac].afcn_addr))(eid, fac_scb);
		}
	    }
	}
    }
    return(OK);
}

/*{
** Name: scs_i_interpret()	- Interpret interrupt block
**
** Description:
**      This routine examines the interrupt block, placing the appropriate 
**      action type into the interrupt indicator in the scb.
**
** Inputs:
**      scb                             Session control block.
**
** Outputs:
**      None directly.                  scb->scb_sscb.sscb_interrupt is
**                                      set appropriately.
**	Returns:
**	    E_DB_OK if worked fine
**	    GCA error iff applicable
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Sep-1987 (fred)
**          Created.
**      02-feb-9191 (jennifer)
**          Fix bug B32476, treat assoc fail like control c.
**      10-may-1991 (fred)
**	    Altered interrupt notification.  Now, do not change the
**	    scb_sscb.sscb_state field.  Setting sscb_interrupt is sufficient.
**	25-jul-1991 (ralph)
**	    Fix bug 37774 -- If GCA status other than OK, TIMEOUT, or PURGED,
**	    process as interrupt.  Fix for bug 32476 assumed only ASSOC FAILs
**	    should be treated as interrupts.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	14-Sep-1993 (daveb)
**	    Honor pending drops here.  
[@history_template@]...
*/
DB_STATUS
scs_i_interpret(SCD_SCB *scb )
{
    GCA_AT_DATA	    *buf;
    
    /* If we got here because a drop was requested, do it now,
       set interrupted, and flag session as dropped */
    
    if( scb->scb_sscb.sscb_flags & SCS_DROP_PENDING )
    {
	scb->scb_sscb.sscb_interrupt = -SCS_INTERRUPT;
	return( E_DB_OK );
    }
    /* if already dropped, then there is nothing to do. */

    if( scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA )
	return( E_DB_OK );

    if ((scb->scb_cscb.cscb_gce.gca_status != E_GC0000_OK) &&
	(scb->scb_cscb.cscb_gce.gca_status != E_GC0020_TIME_OUT) &&
	(scb->scb_cscb.cscb_gce.gca_status != E_GC0027_RQST_PURGED))
    {
	/* Assume all others are processed as interrupt */
	scb->scb_sscb.sscb_rsptype = GCA_ACK;
	scb->scb_sscb.sscb_interrupt = -SCS_INTERRUPT;
	return(E_DB_OK);
    }
    scb->scb_sscb.sscb_rsptype = GCA_ACK;
    buf = (GCA_AT_DATA *) scb->scb_cscb.cscb_gce.gca_data_area;
    if (buf->gca_itype)
	scb->scb_sscb.sscb_interrupt = -SCS_ENDRET;
    else
	scb->scb_sscb.sscb_interrupt = -SCS_INTERRUPT;
    return(E_DB_OK);
}

/*
** History:
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
STATUS
scs_fhandler( EX_ARGS *ex_args )
{
    char    msg[EX_MAX_SYS_REP];

    char	server_id[64];
    CS_SCB      *scb;

    CSget_svrid( server_id );
    CSget_scb( &scb );
    ERmsg_hdr( server_id, scb->cs_self, msg);

    if (!(EXsys_report(ex_args, msg)))
	TRdisplay(
	      "Unexpected exception 0x%x while trying to dump current session",
	      ex_args->exarg_num);
    return(EXDECLARE);
}

/*
** Name: scs_format	- Format a session control block
**
** Description:
**      This routine formats the provided scb into the location 
**      passed in.  If the memory provided is not large enough, 
**      an error is returned.
**
** Inputs:
**      scb                             The scb to format
**	stuff				Unused
**	powerful			Indicates that the caller is powerful
**					(I.e. is a super user).
**	supress_sid			Indicates that the caller already
**					printed the session id.
**
** Outputs:
**      *buf is filled
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-feb-1992 (rog)
**	    Created so that we don't have to change the CL.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
STATUS
scs_format(SCD_SCB *scb,
            char *stuff,
            i4  powerful,
            i4  suppress_sid )

{
	return(scs_iformat(scb, powerful, suppress_sid, 0));
}

/*
** Name: scs_avformat	- Show info on the current thread during an AV
**
** Description:
**	This routine calls scs_iformat to show what thread was running
**	when the server got an AV.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-feb-1992 (rog)
**	    Created.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb().
**	02-Jul-1993 (daveb)
**	    prototyped.
**	07-jul-1993 (rog)
**	    Changed the message from saying a fatal error occurred to just
**	    saying that an error occurred.
[@history_template@]...
*/
STATUS
scs_avformat(void)
{
	STATUS		ret_val;
	EX_CONTEXT	ex;

    if (EXdeclare(scs_fhandler, &ex) != OK)
    {
	ret_val = FAIL;
    }
    else
    {
	SCD_SCB		*scb;
	i4		err_code;
	char 		*msg_buffer;
	i4		msg_length;

	msg_buffer = "An error occurred in the following session:";
	msg_length = STlength(msg_buffer);

	CSget_scb((CS_SCB **)&scb);
	uleFormat(NULL, 0, NULL, ULE_MESSAGE, NULL, msg_buffer, msg_length,0,&err_code,0);
	ret_val = scs_iformat(scb, 1, 0, 1);
    }

    EXdelete();
    return(ret_val);
}

i4
scs_facility(SCD_SCB *scb)

{
    if (scb->scb_sscb.sscb_stype!=SCS_SMONITOR)
	return scb->scb_sscb.sscb_cfac;
    else
	return -1;
}

/*
** Name: scs_iformat	- Format a session control block
**
** Description:
**      This routine formats the provided scb into the location 
**      passed in.  If the memory provided is not large enough, 
**      an error is returned.
**
** Inputs:
**      scb                             The scb to format
**	powerful			Indicates that the caller is powerful
**					(I.e. is a super user).
**	supress_sid			Indicates that the caller already
**					printed the session id.
**	err_output			If true, then the output should go to
**					the errlog and II_DBMS_LOG; otherwise,
**					it goes to SCC_TRACE.
**
** Outputs:
**      *buf is filled
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-mar-1987 (fred)
**          Created.
**	15-jan-91 (ralph)
**	    Correct TRdisplay for session group id
**	21-may-91 (andre)
**	    Effective user, group, and role ids are stored in ics_eusername,
**	    ics_egrpid, and ics_eaplid, respectively.  Values of user, group,
**	    and role identifiers which were in effect at session startup are
**	    stored in ics_susername, ics_sgrpid, and ics_saplid.
**	27-feb-1992 (rog)
**	    Renamed from scs_format so we can use this function for both
**	    iimonitor and AV tracing, cleaned up the argument list, and
**	    change the function so that the out can go to various places.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**
**	    As a part of desupporting SET GROUP/ROLE, we are getting rid of
**	    ics_saplid and ics_sgrpid
**
**	    user name as session startup (the Initial user name) will be stored
**	    in ics_iusername (used to be ics_susername).  ics_susername is now
**	    used for the session auth id which can be changed by SET SESSION
**	    AUTHORIZATION
**	2-Jul-1993 (daveb)
**	    prototyped.  removed unused viar 'size_so_far'.
**	9-nov-93 (robf)
**          Display session description. Comes after activity, before
**	    query.
**	09-mar-94 (swm)
**	    Bug #60425
**	    CS_SID was displayed with %x rather than %p, %x truncates
**	    on 64-bit platforms.
**	02-Jun-1998 (jenjo02)
**	    Remove test of scb_cscb.cscb_comm_size < 256; internal threads,
**	    factotum threads may not have a scb_cscb allocated, so the 
**	    results of this check are unpredictable at best and prevented
**	    the formatting of GCA-less sessions.
**	    Added a test for SCS_SSAMPLER thread types. In an OS-threaded
**	    server, this thread's SCB has no SCF component, leading to
**	    unpredictable and fatal SCF-references in here.
**	15-Nov-1999 (jenjo02)
**	    Append ":thread_id" to session id when running with OS threads.
**      25-Sep-2002 (hanal04) Bug 108741 INGSRV 1883
**          Ensure all %s values passed to TRformat are cast correctly
**          in order to prevent SIGSEGVs in do_format().
**      27-Apr-2003 (hanal04) Bug 112209 INGSRV2807
**          When dumping large query text buffers we must check to
**          see that the buffer is still there.
**	31-oct-2006 (dougi)
**	    Display ics_lqbuf if ics_l_qbuf is 0.
**	26-Feb-2009 (thaju02)
**	    Modified prior change to display both current query and 
**	    last query for iimonitor show sessions formatted. (B121890)
*/
STATUS
scs_iformat(SCD_SCB *scb,
	    i4  powerful,
	    i4  suppress_sid,
	    i4  err_output )

{
    i4		print_mask = 0;
    char		buf[4*ER_MAX_LEN];
    char    	    	user[ GL_MAXNAME * 2 + 1 ];
    char    	    	host[ GL_MAXNAME * 2 + 1 ];
    char    	    	tty[ GL_MAXNAME * 2 + 1 ];
    char    	    	pid[ GL_MAXNAME * 2 + 1 ];
    char    	    	conn[ GL_MAXNAME * 4 + 1 ];

    /* Ignore the sampler thread - it has no SCF component */
    if (scb->cs_scb.cs_thread_type == SCS_SSAMPLER)
	return(FAIL);

    print_mask |= (err_output) ? SCE_FORMAT_ERRLOG : SCE_FORMAT_GCA;

    if (!suppress_sid)
    {
#ifdef OS_THREADS_USED
	if ( CS_is_mt() )
	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		">>>>>Session %p:%d<<<<<",
		scb->cs_scb.cs_self,
		*(i4*)&scb->cs_scb.cs_thread_id);
	else
#endif /* OS_THREADS_USED */
	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
	    ">>>>>Session %p<<<<<",
	    scb->cs_scb.cs_self);
    }

    if (scb->scb_sscb.sscb_stype!=SCS_SMONITOR)
    {
	/*
	** Monitor session doesn't connect to a specific database
	*/
        TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    DB Name: %#s%s (Owned by: %#s )",
    	    DB_DB_MAXNAME, (char *) &scb->scb_sscb.sscb_ics.ics_dbname,
    	    (scb->scb_sscb.sscb_ics.ics_lock_mode & DMC_L_EXCLUSIVE ?
    	        "(exclusive)" : ""),
    	    DB_OWN_MAXNAME, (char *) &scb->scb_sscb.sscb_ics.ics_dbowner);
    }
    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	"    User: %#s (%#s) ",
	DB_OWN_MAXNAME, (char *) scb->scb_sscb.sscb_ics.ics_eusername,
    	DB_OWN_MAXNAME, &scb->scb_sscb.sscb_ics.ics_rusername);
    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	"    User Name at Session Startup: %#s",
    	DB_OWN_MAXNAME, (char *) &scb->scb_sscb.sscb_ics.ics_iusername);
    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	"    Terminal: %16s",
    	(char *) &scb->scb_sscb.sscb_ics.ics_terminal);

    if (scb->scb_sscb.sscb_stype!=SCS_SMONITOR)
    {
        TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
        	"    Group Id: %#s",
        	DB_OWN_MAXNAME, (char *) &scb->scb_sscb.sscb_ics.ics_egrpid);
        TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
        	"    Role Id: %#s",
        	DB_OWN_MAXNAME, (char *) &scb->scb_sscb.sscb_ics.ics_eaplid);
        TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
            "    Application Code: %x            Current Facility: %w %<(%x)",
    	    scb->scb_sscb.sscb_ics.ics_appl_code,
        "<None>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF",
    	    scb->scb_sscb.sscb_cfac);
    }
    if (scb->scb_sscb.sscb_ics.ics_l_act1 ||
    	scb->scb_sscb.sscb_ics.ics_l_act2)
    {
        if (scb->scb_sscb.sscb_ics.ics_l_act1)
        {
    	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    Activity: %t",
    	    scb->scb_sscb.sscb_ics.ics_l_act1,
    	    scb->scb_sscb.sscb_ics.ics_act1);
        }
        else if (scb->scb_sscb.sscb_cfac == DB_DMF_ID)
        {
    	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    Activity: (Aborting)");
        }
        else if (scb->scb_sscb.sscb_cfac == DB_OPF_ID)
        {
    	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    Activity: (Optimizing)");
        }
        if (scb->scb_sscb.sscb_ics.ics_l_act2)
        {
    	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		    "    Act. Detail: %t",
		    scb->scb_sscb.sscb_ics.ics_l_act2,
		    scb->scb_sscb.sscb_ics.ics_act2);
        }
    

    }
    else if (scb->scb_sscb.sscb_stype==SCS_SMONITOR)
    	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    Activity: (Monitor Session)");

    if( scb->scb_sscb.sscb_ics.ics_gw_parms != NULL )
    {
	scs_a_user_get( 0, 0, (PTR)scb, sizeof( user ), user );
	scs_a_host_get( 0, 0, (PTR)scb, sizeof( host ), host );
	scs_a_tty_get( 0, 0, (PTR)scb, sizeof( tty ), tty );
	scs_a_pid_get( 0, 0, (PTR)scb, sizeof( pid ), pid );
	scs_a_conn_get( 0, 0, (PTR)scb, sizeof( conn ), conn );
	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		 "    Client user: %s\n    Client host: %s", user, host );
	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		 "    Client tty: %s\n    Client pid: %s", tty, pid );
	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		 "    Client connection target: %s", conn );
	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		 "    Client information: %s",
		 (char *) scb->scb_sscb.sscb_ics.ics_gw_parms );
    }
          
    /* Session description. This is only available to privileged sessions,
    ** since it could be a comms channel on a secure system.
    ** Same for Query:
    */
    if (powerful && scb->scb_sscb.sscb_stype!=SCS_SMONITOR)
    {
	    TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    	    "    Description: %t",
    		scb->scb_sscb.sscb_ics.ics_l_desc ,
		(scb->scb_sscb.sscb_ics.ics_l_desc ?
		    scb->scb_sscb.sscb_ics.ics_description : ""));
	/*
	** Monitor sessions do not have a query
	*/
        if(!scb->scb_sscb.sscb_troot ||
	   (scb->scb_sscb.sscb_ics.ics_l_qbuf < ER_MAX_LEN-ER_HDR_OFFSET))
	{
        	TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
    		"    Query: %t\n", 
		(scb->scb_sscb.sscb_ics.ics_l_qbuf ? 
		    scb->scb_sscb.sscb_ics.ics_l_qbuf : 0), 
		(scb->scb_sscb.sscb_ics.ics_l_qbuf ? 
		    scb->scb_sscb.sscb_ics.ics_qbuf : ""));

		TRformat(sc0e_tput, &print_mask, buf, sizeof(buf) - 1,
		"    Last Query: %t\n", 
		(scb->scb_sscb.sscb_ics.ics_l_lqbuf ? 
		    scb->scb_sscb.sscb_ics.ics_l_lqbuf : 0), 
		(scb->scb_sscb.sscb_ics.ics_l_lqbuf ? 
		    scb->scb_sscb.sscb_ics.ics_lqbuf : ""));
	}
	else
	{
		i2 qlen = 0, oLen = 0;
		i2 first_pass = TRUE;
		PTR qPtr;

		qPtr = (PTR) ((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrytext;
		for (qlen = ((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrysize; qlen > 0 ;qlen-=oLen)
		{
		   if (qlen > ER_MAX_LEN-ER_HDR_OFFSET)
		      oLen = ER_MAX_LEN-ER_HDR_OFFSET;
		    else 
		       oLen = qlen;
		    if(first_pass)
		    {
        	       TRformat(sc0e_tput, &print_mask, buf, qlen -1,
    		      		    "    Query:%t\n",
		      		    oLen,
    		      		    qPtr);
		       first_pass = FALSE;
		    }
		    else
		    {
        	       TRformat(sc0e_tput, &print_mask, buf, qlen -1,
    		      		    "%t\n",
		      		    oLen,
    		      		    qPtr);
		    }
		    qPtr += oLen ;
		}
	}

    }
    return(OK);
}


/*
** Name:	scs_gwinfo_subget
**
** Description:
**  	grab the value part of the gw string of the form
**          
** 	    var1=value2,var2='value2',var3='value3',var4=value4
**
**  	the input substr is of the form "varN=".  We strip out any quotes
**  	and copy the value to the user buffer.
**
**  	if there are more than one entry for a var, then take
**  	the first one.
**
** Inputs:
**	substr	    substring of the variable name
**  	scb 	    scb with the gw_parm string to scan.
**  	lbuf	    length of output buffer
**
** Outputs:
**	buf 	    buffer to put extracted value string, less quotes.
**  	    	    if substr not found, set to empty string.
**
** Returns:
**	OK  	    	    if got string OK, or no string found.
**  	MO_VALUE_TRUNCATED  if value got truncated to lbuf.
**
** History:
**      22-Apr-1994 (daveb)
**          documented, changed to "first wins"
*/
STATUS
scs_gwinfo_subget( char *substr, SCD_SCB *scb, i4  lbuf, char *buf )
{
    STATUS  cl_stat = OK;
    i4  sublen;
    i4  outlen;
    i4  inquote = FALSE;
    char *str;
    char *loc;

    *buf = EOS;
    if( (str = scb->scb_sscb.sscb_ics.ics_gw_parms) != NULL )
    {
	sublen = STlength( substr );

	/* for each potential match, check for full match */

	for( ; *str && (loc = STchr( str, *substr)) != NULL ; str++ )
	{
	    /* if this is the substring, get the good stuff. */
	    if( !STncmp( loc, substr, sublen ) )
	    {
		loc += sublen;
		if( *loc == '\'' )
		{
		    loc++;
		    inquote = TRUE;
		}
		str = loc;
		
		while( *loc && (inquote && *loc != '\'') ||
		      (!inquote && *loc != ','))
		    loc++;
		outlen = loc - str;
		if( outlen > (lbuf - 1) )
		{
		    outlen = lbuf - 1;
		    cl_stat = MO_VALUE_TRUNCATED;
		}
		STncpy( buf, str, outlen );
		buf[ outlen ] = '\0';
		break;
	    }
	}
    }
    return( cl_stat );
}


/*{
** Name:	scs_a_user_get - get client user from gw_info_string
**
** Description:
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		to gw_info pointer.
**	objsize		ignored
**	object		point to a SCD_SCB.
**	luserbuf	length of userbuf.
**
** Outputs:
**	userbuf		filled with data from gw_info
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
scs_a_user_get(i4 offset,
	       i4  objsize,
	       PTR object,
	       i4  luserbuf,
	       char *userbuf)
{
    return( scs_gwinfo_subget( USER_STRING, (SCD_SCB *)object,
			      luserbuf, userbuf ) );
}

STATUS
scs_a_host_get(i4 offset,
	       i4  objsize,
	       PTR object,
	       i4  luserbuf,
	       char *userbuf)
{
    return( scs_gwinfo_subget( HOST_STRING, (SCD_SCB *)object,
			      luserbuf, userbuf ) );
}

STATUS
scs_a_tty_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    return( scs_gwinfo_subget( TTY_STRING, (SCD_SCB *)object,
			      luserbuf, userbuf ) );
}

STATUS
scs_a_pid_get(i4 offset,
	      i4  objsize,
	      PTR object,
	      i4  luserbuf,
	      char *userbuf)
{
    return( scs_gwinfo_subget( PID_STRING, (SCD_SCB *)object,
			      luserbuf, userbuf ) );
}

STATUS
scs_a_conn_get(i4 offset,
	       i4  objsize,
	       PTR object,
	       i4  luserbuf,
	       char *userbuf)
{
    return( scs_gwinfo_subget( CONN_STRING, (SCD_SCB *)object,
			      luserbuf, userbuf ) );
}



/*{
**
**  Name: scs_chk_priv - validate a given user's ownership of a given privilege
**
**  Description:
**	Calls PM to determine the user's allotted privileges, then
**	checks whether the specific required privilege is among them.
**
**  Inputs:
**	usr  -	user to validate
**	priv -	name of required privilege
**
**  Returns:
**	TRUE/FALSE	- user has the privilege
**	
**  History:
**      28-Oct-92 (brucek)
**          Created.
**	26-Jul-1993 (daveb)
**	    swiped from gca_chk_priv.
**       9-Dec-1993 (daveb)
**          Changed to meet current LRC approved PM vars.
**       2-Mar-1994 (daveb) 58423
**          make public so it can be used in scs_initate for monitor
**  	    session validation.  Limit possible length to DB_MAXNAME in case it
**  	    isn't null terminated
**      21-Apr-1994 (daveb)
**          Renamed/moved from scd to scs_chk_priv for VMS shared
**          library reasons.
**/

bool
scs_chk_priv( char *user_name, char *priv_name )
{
	char	pmsym[128], userbuf[DB_OWN_MAXNAME+1], *value, *valueptr ;
	char	*strbuf = 0;
	int	priv_len;

        /* 
        ** privileges entries are of the form
        **   ii.<host>.*.privilege.<user>:   SERVER_CONTROL,NET_ADMIN,...
	**
	** Currently known privs are:  SERVER_CONTROL,NET_ADMIN,
	**  	    MONITOR,TRUSTED
        */

	STncpy( userbuf, user_name, DB_OWN_MAXNAME );
	userbuf[ DB_OWN_MAXNAME ] = '\0';
	STtrmwhite( userbuf );
	STprintf(pmsym, "$.$.privileges.user.%s", userbuf);
	
	/* check to see if entry for given user */
	/* Assumes PMinit() and PMload() have already been called */

	if( PMget(pmsym, &value) != OK )
	    return FALSE;
	
	valueptr = value ;
	priv_len = STlength(priv_name) ;

	/*
	** STindex the PM value string and compare each individual string
	** with priv_name
	*/
	while ( *valueptr != EOS && (strbuf=STchr(valueptr, ',' )))
	{
	    if (!STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	    /* skip blank characters after the ','*/	
	    valueptr = STskipblank(strbuf+1, 10); 
	}

	/* we're now at the last or only (or NULL) word in the string */
	if ( *valueptr != EOS  && 
              !STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	return FALSE;
}

