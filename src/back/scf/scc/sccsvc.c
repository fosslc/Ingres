/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h> 
#include    <gl.h> 
#include    <tm.h>
#include    <me.h>
#include    <sl.h> 
#include    <pc.h>
#include    <cs.h> 
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h> 
#include    <dbdbms.h> 
#include    <erusf.h>
#include    <ulf.h> 
#include    <scf.h> 
#include    <qsf.h> 
#include    <adf.h> 
#include    <gca.h>
#include    <ddb.h>

#include    <generr.h>
#include    <sqlstate.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <ulm.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sxf.h>
#include    <sc0a.h>
#include    <sc0e.h>
#ifdef VMS
#include    <ssdef.h>
#include    <starlet.h>
#include    <astjacket.h>
#endif

/**
**
**  Name: SCCSVC.C - SCF Communications services
**
**  Description:
**      This file contains the service routines provided by the communications 
**      module of the scf.  For phase I of jupiter, this will be restricted to 
**      provision of a method for sending error back to the frontend. 
** 
**      In the future, the routines necessary for distributed database support 
**      will also be found here.  At such time as they are known, they will 
**      be provided. 
**
**
**          scc_error() - send an error message to the frontend
**
**
**  History:
**      20-Jan-86 (fred)    
**          Created on Jupiter.
**	22-Mar-89 (fred)
**	    Made use of new sc_proxy_scb field.  This field is used when SCF
**	    wishes to work on behalf of another session, one of its own choosing.
**	24-mar-89 (paul)
**	    Add support for user defined error messages.
**	24-apr-89 (neil)
**	    Fixed formatting for user defined error messages.
**	04-may-89 (neil)
**	    Backed out change of 24-mar and applied new operation code
**	    SCC_RSERROR for RAISE ERROR.
**	19-may-89 (jrb)
**	    Added support for generic errors to scc_error.
**	30-nov-89 (fred)
**	    Added support for sending incomplete messages -- as part of large
**	    datatype support.
**	19-Jan-90 (anton)
**	    Prevent threads from continueing once writes fail
**      06-feb-90 (ralph)
**          Check consistency of message pointers in scc_send and scc_dispose.
**	10-dec-90 (neil)
**	    Alerters: Managing read/writes while there are alerts, linting
**	    and alert completion routines.
**	10-may-1991 (fred)
**	    Improved interrupt handling (see scc_recv() and scc_gcomplete()).
**	8-Aug-91 (daveb)
**	    B38056:  CSresume threads without checking SCF states if
**		they don't look like they are SCF threads.  Otherwise
**		we lose resume's and the system hangs.  The case in mind
**		is i/o processing in the CL admin thread, which does not
**		have the fields checked when we think it's an SCF thread.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              02-nov-89 (georgeg)
**                  Modified scc_error to bump sscb_noerror only if scc_error()
**		    is called with opcode SCC_ERROR.
**              08-nov-89 (georgeg)
**                  If the protocol level of the partner is GCA_PROTOCOL_LEVEL_2
**		    or lower report error the 6.2 way with no genetic error.
**              29-nov-89 (georgeg)
**                  if the session has no FE GCA don't que the message for 
**		    output.
**              10-may-1991 (fred)
**                  Improved interrupt handling (see scc_recv() and 
**		    scc_gcomplete()).
**          End of STAR comments.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**      13-jul-92 (rog)
**          Included er.h because of a change to scs.h.
**	10-nov-92 (robf)
**	    Make sure SXF audit data is flushed prior to returning anything
**	    to the user. 
**      10-mar-93 (mikem)
**          bug #47624
**          Disable CSresume() calls from scc_gcomplete() while session is
**          running down in scs_terminate() as indicated by the
**          sccb_terminating field.  Allowing CSresume()'s during this time
**          leads to early wakeups from CSsuspend()'s called by scs_terminate
**          (like DI disk I/O operations).
**	12-Mar-1993 (daveb)
**	    Add include <me.h>
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.  Move FUNC_EXTERNs
**	    into <scc.h>.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-1993 (andys)
**	    Check the status set by gca_receive in scc_recv. Make sure
**	    we return FAIL if it is not what we want. [bug 42284]
**	    [was 16-sep-92 (andys), 05-oct-92 (andys)]
**	 6-sep-93 (swm)
**	    Cast first parameter of CSresume() to CS_SID and change scb
**	    cast to CS_SID in if-statement, to match revised CL interface
**	    specification.
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	20-Sep-1993 (daveb)
**	    Don't log expected/OK INV_ASSOC error.  Also, adjust sc0e_put
**	    calls to match prototype.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in scc_gcomplete().
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	    This results in owner within sc0m_allocate having to be PTR.
**      31-Jan-1994 (fpang)
**	    In scc_send(), don't issue E_SC023B_INCONSISTENT_MSG_QUEUE if gca 
**	    has already been dropped.
**	    Fixes B5807, Spurious and random E_SC023B_INCONSISTENT_MSG_QUEUE
**	    errors in errlog.log.
**      17-oct-1994 (canor01)
**          fix another cast for DB_SCF_ID to PTR.
**      26-jun-1995 (canor01)
**          Pass the SCB to IIGCa_call(), not the SID. These are different
**	    on NT.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	03-jun-1996 (canor01)
**	    Generic fix for SCB/SID problem.  scb->cs_self is acceptible
**	    for SIDs that are thread ids and for those that are SCB pointers.
**	31-jul-1996 (prida01)
**	    We do not check the scg_mask in outgoing messages if there are no 
**	    messages. Caused a looping server.
**	06-nov-1996 (cohmi01)
**	    scc_fa_notify - Add message block management for RAAT. Include
**	    erusf.h to obtain #define for err 4706.        Bug 78797.
**	19-aug-1996 (canor01)
**	    When passing the SCB to IIGCa_call(), eliminate redundant call
**	    to CS_find_scb().
**	30-Mar-1998 (jenjo02)
**	    Removed SCS_NOGCA flag from sscb_flags, utilizing (1 << DB_GCF_ID)
**	    mask in sscb_need_facilities instead.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**      04-Jul_2000 (horda03)
**          VMS only. Only applicable while Ingres built with /NOMEMBER_ALIGNMENT.
**          If an AST fires for a mailbox which this code is checking the GCA_STATUS
**          then a corrupt value of the status may be determined; the AST must arrive
**          whilst the code is accessing the variable; because of the /NOMEMBER
**          compiler option, the variable may straddle two quadwords, so two memory
**          access operations are required. This introduces a window where the code
**          can determine one part of the value, the AST updates the variable and then
**          the code determines theother part of the variable. Temporary solution is
**          to disable ASTs while accessing the GCA_STATUS variable. (101951/INGSRV1206).
**      12-Jul-2000 (horda03)
**          Removed unwanted header files that break non-VMS builds.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scc_relay(), scc_error(), scc_trace().
**      28-jan-2004 (horda03) Bug 110771/INGSRV2482
**          Compiler optimisation reorders the setting of scs_astate to SCS_ASDEFAULT
**          and last_error to gca_status in scc_send(). It is critical that the
**          scs_astate is updated before the gca_status value is read, as if the
**          outstanding send completes with sscb_astate == SCS_ASIGNORE, the session
**          will not be resumed. This causes the DBMS session to hang, waiting for a
**          CSresume().
**      31-aug-2004 (sheco02)
**          X-integrate change 467133 to main.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new sc0ePut call formats.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
**      20-Apr-2009 (horda03) Bug 121826
**          Don't bother calling CSsuspend() when sending a message if the session is
**          not able to call CSresume() (see scc_gcomplete) and the message has already
**          been delivered.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**/

/*
**  Forward and/or External function/variable references.
*/

GLOBALREF SC_MAIN_CB *Sc_main_cb;        /* scf main cb */

static VOID scc_end_message( SCD_SCB *scb ); /* Complete incomplete message */

/*{
** Name: scc_error	- send an error to a user
**
** Description:
**      This function provides a way to send errors to the frontend 
**      program.  It is provided to allow successful operation of all 
**      facilities to be viewed as a function call interface, while errors 
**      are odd events.  This function may do buffering of the error; 
**      it may also cause the sending facility to be suspended while 
**      the error (or some set of them) is sent.
**
** Inputs:
**      operation			SCC_ERROR, SCC_MESSAGE, SCC_WARNING or
**					SCC_RSERROR
**		                           (op code to scf_call())
**      scf_cb
**	    .scf_nbr_union.scf_local_error    local error number for this error
**	    .scf_aux_union.scf_sqlstate	      SQLSTATE status code for error
**          .scf_len_union.scf_blength	      length of error message to be sent
**          .scf_ptr_union.scf_buffer	      pointer to text of error message
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or ...
**                  E_SC000C_BUFFER_LENGTH  (0 < scf_blength <= DB_ERR_SIZE) != TRUE
**		    E_SC000D_BUFFER_ADDR    error buffer not addressable
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Jan-86 (fred)
**          Created on Jupiter.
**	14-May-86 (fred)
**          added error number (scf_enumber)
**	11-may-1988 (fred)
**	    Added operation parameter to allow sending of MESSAGEs, WARNINGs, or
**	    ERRORs.
**	24-mar-89 (paul)
**	    Add support for user defined error messages.
**	24-apr-89 (neil)
**	    Fixed formatting for user defined error messages.
**	04-may-89 (neil)
**	    New operation SCC_RSERROR for RAISE ERROR statements.
**	    Fixed TRdisplay call (arg passing order).
**	19-may-89 (jrb)
**	    Added support for generic errors and removed xDEBUG from around
**	    sc902 checking code.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              08-nov-89 (georgeg)
**                  If the protocol level of the partner is GCA_PROTOCOL_LEVEL_2
**		    or lower report error the 6.2 way with no genetic error 
**		    (ingres error in gca_id_error and gca_local_error=0) else 
**		    if the if we are higher that GCA_PROTOCOL_LEVEL_2 report 
**		    genneric errors.
**              29-nov-89 (georgeg)
**                  if the session has no FE GCA don't que the message for 
**		    output.
**              30-may-1990 (georgeg)
**                  declared operation as a i4.
**          End of STAR comments.
**	25-oct-92 (andre)
**	    if communicating using protocol level GCA_PROTOCOL_LEVEL_60 or
**	    above, send encoded (using ss_encode()) sqlstate in gca_id_error
**	    instead of generic error;
**	    for protocol level > GCA_PROTOCOL_LEVEL_2 but less than 
**	    GCA_PROTOCOL_LEVEL_60, a mapping function must be used to determine
**	    generic error message based on sqlstate (and possibly the dbms
**	    error)
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	30-jun-1993 (shailaja)
**	    Fixed prototype incompatibilities in CSget_scb.
**	2-Jul-1993 (daveb)
**	    give operation a type, prototype.  Also use proper block
**	    indirection for allocation.
**	20-Jan-1994 (fpang)
**	    In scc_send(), after sending an incomplete message, if the 
**	    continuation message if found, move it to the front of the queue 
**	    so that it can be deleted in order.
**	    Fixes B57764, TM help in direct connect causes Star to SEGV.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      04-Jul-2000 (horda03)
**          Disable ASTs (on VMS) while accessing the GCA_STATUS variable.
**       6-Mar-2006 (hanal04) Bug 115745
**          A DBP looping over MESSAGE statements causes the DBMS server
**          to consume all avaialble memory. The ebuf allocations are not
**          deallocated until the "EXECUTE" terminates and scc_send() is called
**          to send/deallocate all of the queue messages.
**          The solution is to send/deallocate all SCC_MESSAGE (DBP generated
**          messages) immediately.
**      jan 2010 (stephenb)
**          Batch processing changes.
*/
DB_STATUS
scc_error( i4  operation, SCF_CB * scf_cb, SCD_SCB *scb )
{
    DB_STATUS		status;
    SCC_GCMSG		*ebuf;
    GCA_ER_DATA		*emsg;
    i4		eseverity;
    PTR			block;
    GCA_PARMLIST        gca_parms;
    i4			error;
    i4			gca_indicator = 0;
    STATUS		rv;

#ifdef VMS
    int ast_enabled;
#endif

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    if (    (scf_cb->scf_len_union.scf_blength <= 0)
	||  (scf_cb->scf_len_union.scf_blength > DB_ERR_SIZE)
       )
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC000C_BUFFER_LENGTH);
	status = E_DB_ERROR;
    }
    else if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
    {
	if (!scf_cb->scf_ptr_union.scf_buffer)
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC000D_BUFFER_ADDR);
	    status = E_DB_ERROR;
	}
	else
	{
	    scb->scb_sscb.sscb_noerrors++;

	    if (Sc_main_cb->sc_gca_cb)	/* if not running standalone */
	    {
		GCA_DATA_VALUE	*gca_data_value;
		
		status = sc0m_allocate(0,
				(i4)(sizeof(SCC_GCMSG)
				    + sizeof(GCA_ER_DATA)
				    + scf_cb->scf_len_union.scf_blength),
				SCS_MEM,
				(PTR) DB_SCF_ID,
				SCG_TAG,
				&block);
		ebuf = (SCC_GCMSG *)block;
		if (status)
		{
		    sc0ePut(NULL, status, NULL, 0);
		    SETDBERR(&scf_cb->scf_error, 0, status);
		    return(E_DB_ERROR);
		}
				    
		ebuf->scg_mtype = GCA_ERROR;
		ebuf->scg_mask = 0;
		ebuf->scg_msize = sizeof(GCA_ER_DATA) - 
					sizeof(GCA_DATA_VALUE)
					    + sizeof(i4)
					    + sizeof(i4)
					    + sizeof(i4)
			+ scf_cb->scf_len_union.scf_blength;
		ebuf->scg_marea = (PTR)((char *) ebuf + sizeof(SCC_GCMSG));
		emsg = (GCA_ER_DATA *)(ebuf->scg_marea);
		emsg->gca_l_e_element = 1;


		switch (operation)
		{
		  case SCC_MESSAGE:
		    eseverity = GCA_ERMESSAGE;
		    break;
		  case SCC_WARNING:
		    eseverity = GCA_ERWARNING;
		    break;
		  case SCC_RSERROR:		    /* User RAISE ERROR message */
		    eseverity = GCA_ERDEFAULT | GCA_ERFORMATTED;
		    break;
		  case SCC_ERROR:
		  default:
		    eseverity = GCA_ERDEFAULT;
		    break;
		}

		/*
		** - If the client has a partner protocol level of 2 or less,
		**   then he is not expecting generic errors, so we send errors
		**   the old way (local error in the gca_id_error slot and 0 in
		**   the gca_local_error slot).
		** - If the patner protocol level is in (2,60) we send generic
		**   error in the gca_id_error slot and local error in the
		**   gca_local_error slot.
		** - Finally, if the protocol level >= 60, we send local error
		**   in gca_local_error and encoded sqlstate in gca_id_error
		**
		** Note: cscb_version will be correctly set ONLY IF we are going
		** through the name server.  Direct connections (by using the
		** II_DBMS_SERVER logical) will cause cscb_version to be 0.
		*/
		if (scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2)
		{
		    MEcopy((PTR)&scf_cb->scf_nbr_union.scf_local_error,
			    sizeof(scf_cb->scf_nbr_union.scf_local_error),
			    (PTR)&emsg->gca_e_element[0].gca_id_error);
		    emsg->gca_e_element[0].gca_local_error = 0;
		}
		else if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
		{
		    i4	    generic_error;

		    generic_error = ss_2_ge(
			scf_cb->scf_aux_union.scf_sqlstate.db_sqlstate,
			scf_cb->scf_nbr_union.scf_local_error);

		    MEcopy((PTR)&generic_error, sizeof(generic_error),
			    (PTR)&emsg->gca_e_element[0].gca_id_error);
		    MEcopy((PTR)&scf_cb->scf_nbr_union.scf_local_error,
			    sizeof(scf_cb->scf_nbr_union.scf_local_error),
			    (PTR)&emsg->gca_e_element[0].gca_local_error);
		}
		else	/* GCA_PROTOCOL_LEVEL >= GCA_PROTOCOL_LEVEL_60 */
		{
		    i4	    ss;

		    /* encode sqlstate as a i4 */
		    ss = ss_encode(scf_cb->scf_aux_union.scf_sqlstate.db_sqlstate);

		    MEcopy((PTR) &ss, sizeof(ss),
			(PTR) &emsg->gca_e_element[0].gca_id_error);
		    MEcopy((PTR)&scf_cb->scf_nbr_union.scf_local_error,
			    sizeof(scf_cb->scf_nbr_union.scf_local_error),
			    (PTR)&emsg->gca_e_element[0].gca_local_error);
		}

		emsg->gca_e_element[0].gca_id_server =
		    (i4) Sc_main_cb->sc_pid;

		emsg->gca_e_element[0].gca_severity = eseverity;

		emsg->gca_e_element[0].gca_server_type = 0;
		emsg->gca_e_element[0].gca_l_error_parm = 1;
		gca_data_value = emsg->gca_e_element[0].gca_error_parm;
		
		gca_data_value->gca_type = DB_CHA_TYPE;
		gca_data_value->gca_l_value = scf_cb->scf_len_union.scf_blength;
		MEcopy((PTR)scf_cb->scf_ptr_union.scf_buffer,
		    scf_cb->scf_len_union.scf_blength,
		    (PTR)gca_data_value->gca_value);
     
		if(operation == SCC_MESSAGE)
		{
		    /* Send message and deallocate now. A looping DBP can
		    ** accumulate a lot of messages before the "EXECUTE" allows us
		    ** to send and deallocate messages in scc_send() 
		    */
		    MEfill(sizeof(gca_parms), 0, &gca_parms);
		    gca_parms.gca_sd_parms.gca_buffer = ebuf->scg_marea;
		    gca_parms.gca_sd_parms.gca_msg_length = 
				     (ebuf->scg_msize ? ebuf->scg_msize : 1);
		    gca_parms.gca_sd_parms.gca_message_type = ebuf->scg_mtype;
		    gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
		    gca_parms.gca_sd_parms.gca_association_id = scb->scb_cscb.cscb_assoc_id;
		    gca_parms.gca_sd_parms.gca_modifiers = 0;
		    gca_parms.gca_sd_parms.gca_status = E_DB_OK;
		    gca_indicator |= GCA_ASYNC_FLAG;
		    if (scb->scb_cscb.cscb_batch_count > 0)
			gca_parms.gca_sd_parms.gca_modifiers = GCA_NOT_EOG;
     
                    status=IIGCa_call(GCA_SEND,
                                      &gca_parms,
                                      (i4)gca_indicator,
                                      (PTR)scb,
                                      (i4)-1,
                                      &error);

                    if (status)
                    {
                       status = error;
                       if((scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA) == 0)
                          sc0e_0_put(status, 0);
                    }

                    /* Only invoke the CSsuspend() if the message hasn't been delivered,
                    ** or if the delivery of the message will cause a CSresume.
                    */
                    if ( (gca_parms.gca_sd_parms.gca_status == E_GCFFFF_IN_PROCESS) ||
                         ( (scb->scb_sscb.sscb_force_abort != SCS_FORCE_ABORT) &&
                           (scb->scb_sscb.sscb_interrupt <= 0 ) &&
                           (scb->scb_sscb.sscb_terminating == 0) &&
                           (scb->scb_sscb.sscb_astate != SCS_ASIGNORE) ) )
                    {
                        do
                        {
                           rv=CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK |
                                        CS_TIMEOUT_MASK, (i4)2, 0);
                        }
                        while ((rv == E_DB_OK) &&
                               (gca_parms.gca_sd_parms.gca_status ==
                                      E_GCFFFF_IN_PROCESS));
                    }
                    else
                       rv = E_DB_OK;

		    status = sc0m_deallocate(0, (PTR *)&ebuf);
		    if (status)
		    {
			sc0ePut(NULL, status, NULL, 0);
			return(E_DB_ERROR);
		    }

                    if (rv == E_CS0008_INTERRUPTED)
                    {
                       scf_cb->scf_error.err_code = rv;
    
                       return(E_DB_ERROR);
                    }
		}
		else
		{
		    if(scb->scb_sscb.sscb_flags & SCS_INSERT_OPTIM &&
			    operation == SCC_ERROR)
		    {
			SCC_GCMSG	*message;
			SCC_GCMSG	*next_message;
			/* 
			** we got an error while running an insert to copy optimization.
			** The assumption is that an error in a copy that causes a user
			** error to be sent to the front-end will also terminate the copy
			** (thus making all previous rows evaporate). What we will do
			** in this case is remove all previous responses from the message
			** queue and insert the error. When the code drops back in to
			** the sequencer this error will be flagged as batch terminating
			** which will cause us to eat the rest of the input for the current
			** batch. The user will ultimately be sent one error message for
			** the current copy optimization and must assume that the rest of
			** the batch (i.e. the entire copy) failed.
			*/
			message=scb->scb_cscb.cscb_mnext.scg_next;
			while (message != (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext)
			{
			    
			    if (message->scg_mtype == GCA_RESPONSE)
			    {
				/* remove from queue */
				message->scg_next->scg_prev = message->scg_prev;
				message->scg_prev->scg_next = message->scg_next;
			    }
			    next_message = message->scg_next;
			    /* and deallocate it (because it will never be sent ) */
			    if (message->scg_mtype == GCA_RESPONSE && 
				    (message->scg_mask & SCG_QDONE_DEALLOC_MASK))
				(void)sc0m_deallocate(0, (PTR *)&message);
			    message = next_message;
			}
			/* just in case */
			scb->scb_cscb.cscb_eog_error = TRUE;
		    }
		    ebuf->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
		    ebuf->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = ebuf;
		    scb->scb_cscb.cscb_mnext.scg_prev = ebuf;
		}
	    }
	    
	    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_LOG_ERROR, NULL, NULL))
	    {
		char    *ops;

		switch (operation)
		{
		  case SCC_ERROR:
		  default:
		    ops = "SCC_ERROR";
		    break;
		  case SCC_RSERROR:	
		    ops = "SCC_RSERROR";
		    break;
		  case SCC_MESSAGE:
		    ops = "SCC_MESSAGE";
		    break;
		  case SCC_WARNING:
		    ops = "SCC_WARNING";
		    break;
		}
		
		TRdisplay("%s:SQLSTATE: %5s.%<(%x)/local_error: %d.%<(%x): %t\n",
		    ops,
		    scf_cb->scf_aux_union.scf_sqlstate.db_sqlstate,
		    scf_cb->scf_nbr_union.scf_local_error,
		    scf_cb->scf_len_union.scf_blength,
		    scf_cb->scf_ptr_union.scf_buffer);
	    }
	}
    }
    return(status);
}

/*{
** Name: scc_trace	- send a trace to the user
**
** Description:
**      This function provides a way to send trace messages to the frontend 
**      program.  It is provided to allow successful operation of all 
**      facilities to be viewed as a function call interface, while errors 
**      are odd events.  This function may do buffering of the error; 
**      it may also cause the sending facility to be suspended while 
**      the error (or some set of them) is sent.
**
** Inputs:
**      SCC_TRACE                       op code to scf_call()
**      scf_cb
**          .scf_len_union.scf_blength  length of trace message to be sent
**          .scf_ptr_union.scf_buffer   pointer to text of trace message
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or ...
**                  E_SC000C_BUFFER_LENGTH  (0 > scf_blength)
**		    E_SC000D_BUFFER_ADDR    error buffer not addressable
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-feb-87 (fred)
**          Created on Jupiter.
**	05-may-89 (jrb)
**	    Added support for SCT_LOG_TRACE tracepoint.  If set this tracepoint
**	    stops trace messages from going to the front end and sends them to
**	    the log.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              29-nov-1989 (georgeg)
**                  if there is no FE to this session do not que the message.
**          End of STAR comments.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.  Use PTR for allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      04-Jul-2000 (horda03)
**          Disable ASTs (on VMS) while accessing the GCA_STATUS variable.
**	01-Jul-2004 (jenjo02)
**	    If session has no GCA (Factotum), route messages to
**	    TRdisplay. Prefix message with session id rather than
**	    "TRACE TO FE:".
*/
DB_STATUS
scc_trace( SCF_CB *scf_cb, SCD_SCB *scb )
{
    DB_STATUS		status;
    SCC_GCMSG		*tbuf;
    PTR			block;
    i4			error;

#ifdef VMS
    int ast_enabled;
#endif

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    if (scf_cb->scf_len_union.scf_blength <= 0)
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC000C_BUFFER_LENGTH);
	status = E_DB_ERROR;
    }
    else if (!scf_cb->scf_ptr_union.scf_buffer)
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC000D_BUFFER_ADDR);
	status = E_DB_ERROR;
    }
    else
    {
	if (Sc_main_cb->sc_gca_cb  &&	/* if not running standalone */
            scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID) &&
	    !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_LOG_TRACE, NULL, NULL))
	{
	    status = sc0m_allocate(0,
			    (i4)(sizeof(SCC_GCMSG)
				+ scf_cb->scf_len_union.scf_blength),
			    SCS_MEM,
			    (PTR) DB_SCF_ID,
			    SCG_TAG,
			    &block);
	    tbuf = (SCC_GCMSG *)block;
	    if (status)
	    {
		sc0ePut(NULL, status, NULL, 0);
		SETDBERR(&scf_cb->scf_error, 0, status);
		return(E_DB_ERROR);
	    }
				
	    tbuf->scg_mtype = GCA_TRACE;
	    tbuf->scg_mask = 0;
	    tbuf->scg_msize = scf_cb->scf_len_union.scf_blength;
	    tbuf->scg_marea = (PTR)((char *) tbuf + sizeof(SCC_GCMSG));
	    MEcopy((PTR)scf_cb->scf_ptr_union.scf_buffer,
		    scf_cb->scf_len_union.scf_blength,
		    (PTR)tbuf->scg_marea);
	    tbuf->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	    tbuf->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = tbuf;
	    scb->scb_cscb.cscb_mnext.scg_prev = tbuf;
	}
	
        if ( !(scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) ||
	    (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_LOG_ERROR, NULL, NULL) ||
	      ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_LOG_TRACE, NULL, NULL)) )
	{
	    TRdisplay("Session %x: %t\n",
		scb->cs_scb.cs_self,
		scf_cb->scf_len_union.scf_blength,
		scf_cb->scf_ptr_union.scf_buffer);
	}
    }
    return(status);
}

/*{
** Name: scc_send   - Send data to partner.
**
** Description:
**      This routine cycles thru the list of messages destined for the 
**      partner, sending (via GCF) the information to the front end. 
**      After sending the last block, the blocks are deallocated.
**
**	If we notice that the force abort flag is set to FACOMPLETE,
**	then we did a force abort while we were sending this set of
**	messages back.  If the list is empty and, then we are too late to
**	sneak a message in.  However, if there is at least one message
**	to send back, then we know that we haven't sent the responce
**	block back (since that's always the last message).  Since that
**	is the case, we will dispose of all remaining messages, insert
**	an error message (that we forced an xact abort) and stick a new
**	responce block on the end.  That'll tell the user and look like
**	it happened during this query.
**
**	If the queue is not empty, then we just return, and let nature
**	take its course.  When we finish reading the next message set
**	from our FE friend, then we will report the error and pretend
**	that it happened during that query.  It doesn't really matter
**	when it happened, as long as the user finds out that their
**	transaction is kaput.
**
**	Various checks are made through this routine against the session
**	alert state.  This is because it is this routine that "notices"
**	there are alerts to send to the client.  Because these alerts are
**	sometimes sent when the user may not be waiting to read them
**	we modify the return state of the routine so that it does not
**	suspend from its caller.  For a detailed description of alert state
**	transition and how they interact with scc_send, please read the
**	introductory comment in sce/scenotify.c.
**
**	Support has been added for the sending of incomplete messages.  When a
**	message que element is marked as incomplete, then the message type is
**	saved in the cscb_incomplete field of the communication portion of the
**	session control block.  Whenever this routine is entered with the
**	cscb_incomplete field non-zero, then the next message sent must have a
**	type which is equal to the cscb_incomplete field (and is, presumably,
**	more of that same message).
**
**      scb                             The scb from which to send blocks.
**	sync				Should GCF perform
**					this request synchronously
**
** Outputs:
**      None
**	Returns:
**	    STATUS -- 'Cause is called by CL (as opposed to DB_STATUS).
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jul-1987 (fred)
**          Created.
**      26-oct-88 (eric)
**          Added support for the cscb_qdone queue. Cscb_qdone is a queue
**          of messages that are to be deallocated when the query is
**          finished.
**      27-jul-88 (fred)
**	    Added support for the write (send) operation to be interruptable.
**	    This was necessary to fix bugs wherein the session being forced was
**	    awaiting a write, which never came.  This happened in STAR when a
**	    distributed thread was doing queries based upon the output of the
**	    another query.  If the queries being done were in the same
**	    installation as the one waiting to complete, the system would
**	    hang until STAR aborted one side, the other, or, more often,
**	    both.
**
**	    This change involves supporting and recognizing that a scc_send()
**	    may be called again with a `last_error' status of GCF_IN_PROCESS.
**	    When this is done, and we are not in a force abort state, then we
**	    simply return as if we had started a new write.  If, though, we are
**	    force abort pending, then we pretend we are done, and return so that
**	    the sequencer can perform the abort.
**      30-nov-89 (fred)
**	    Addes support for the management of incomplete messages.  This is
**	    done for large object support.  See comment in main description body
**	    above.
**	19-Jan-90 (anton)
**	    Prevent thread of continueing once writes fail.
**      06-fep-90 (ralph)
**          Check consistency of chain pointers.
**	10-dec-90 (neil)
**	    Added work/checks for alerters.  For a detailed description of
**	    alert-related processing refer to the large comment in scenotify.c.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              07-aug-1989 (georgeg)
**                  moved changes from jupiter, to handle unknown errors from 
**		    GCF
**              06-feb-1990 (georgeg)
**                  if the last error was E_GCFFFF_IN_PROGRESS return to the 
**		    caller ok.
**          End of STAR comments.
**	10-nov-92 (robf)
**	    Make sure SXF audit data is flushed prior to returning anything
**	    to the user. 
**	2-Jul-1993 (daveb)
**	    prototyped.  Send os_error instead of pointer to OS error to
**	    sc0e_put, as required.
**	29-Jul-1993 (daveb)
**	    Don't log INV_ASSOC_ID errs from dropped connections.
**	28-oct-93 (robf)
**          Don't log invalid queue errs from  dropped connections.
**	20-Jan-1994 (fpang)
**	    After sending an incomplete message, if the continuation message
**	    if found, move it to the front of the queue so that it can be
**	    deleted in order.
**	    Fixes B57764, TM help in direct connect causes Star to SEGV.
**      31-Jan-1994 (fpang)
**	    Don't issue E_SC023B_INCONSISTENT_MSG_QUEUE if gca has already
**	    been dropped.
**	    Fixes B5807, Spurious and random E_SC023B_INCONSISTENT_MSG_QUEUE
**	    errors in errlog.log.
**	31-July-1996 (prida01)
**	    Fix for bug 77613. Server looping endlessly. We do not check
**	    the message scg_mask if there are no messages.
**      28-jan-2004 (horda03) Bug 110771/INGSRV2482
**          Ensure that last_status is updated following the setting of
**          scs_astate to SCS_ASDEFAULT. Moved the assignment of
**          was_in_progress to follow reset of alert state.
**	5-Jul-2007 (kibro01) b118505
**	    Drop out of waiting for input if we get a DROP_PENDING request,
**	    such as from a remove session request from IPM.
**	November 2009 (stephenb)
**	    Set GCA_EOG/GCA_NOT_EOG appropriately for batch processing
**	24-may-2010 (stephenb)
**	    Don't force GCA_EOG for messages other than GCA_RESPONSE and
**	    GCA_ERROR because it causes GCA to flush. We will leave the
**	    value unset (neither GCA_EOG nor GCA_NOT_EOG), this will
**	    effectively ask GCA to decide based on messages type, allowing
**	    it to optimize message sending. This makes for more effecient
**	    communications.
[@history_template@]...
*/
STATUS
scc_send( SCD_SCB *scb, i4  sync )
{
    SCC_GCMSG           *cmsg = scb->scb_cscb.cscb_mnext.scg_next;
    SCC_GCMSG           *cmsg_next;
    DB_STATUS           status;
    i4		last_error = scb->scb_cscb.cscb_gco.gca_status;
    i4		cur_error;
    PTR		free_message = NULL;
    bool		was_in_process;		/* For alert state change */
    bool		queue_was_empty;	/* To process next alert when
						** message queue was really
						** empty.
						*/

    /*
    ** First make sure to reset the alert "ignore" state - if it was set.
    ** Close window where the send completes while we change the state.
    ** Note that with "ignore" set we will not call CSresume (see scenotify.c).
    */
    if (scb->scb_sscb.sscb_astate == SCS_ASIGNORE)
    {
	/* horda03 - Warning on AXM.VMS the compiler may reorder the
        **           assignments od sscb_astate and last_error. It is
        **           critical that the sscb_astate is updated to
        **           SCS_ASDEFAULT before the gca_status value is cheded.
        **           So, if you modify the follow code, you need to verify
        **           that the machine code is correct too!
        */
	SCS_ASTATE_MACRO("scc_send/ignored", scb->scb_sscb, SCS_ASDEFAULT);
	was_in_process = (last_error == E_GCFFFF_IN_PROCESS);
	if ( (last_error = scb->scb_cscb.cscb_gco.gca_status) == E_GCFFFF_IN_PROCESS)	/* Read it again */
	    return (OK);		/* Still reading */
	if (was_in_process)		/* Read completed during state change */
	    CScancelled(0);		/* And continue ... */
    }
    else
    {
	/* Always return to the default state */
	SCS_ASTATE_MACRO("scc_send/clearing", scb->scb_sscb, SCS_ASDEFAULT);
    }

    /* If we are waiting for user input, but have received a DROP_PENDING
    ** notification, we should finish the query a.s.a.p., so drop out here
    ** like with force-abort so the DROP_PENDING can be picked up
    ** (kibro01) b118505
    */
    if (scb->scb_sscb.sscb_flags & SCS_DROP_PENDING)
    {
	return(E_CS001D_NO_MORE_WRITES);
    }

    if (scb->scb_sscb.sscb_force_abort == SCS_FAPENDING)
    {
	return(E_CS001D_NO_MORE_WRITES);
    }
    else if (last_error == E_GCFFFF_IN_PROCESS )
    {
	return(OK);
    }

    /*
    ** This call is necessary to clear up outstanding junk after an interrupt
    ** or forced abort.
    */
    
    if (!sync && scb->scb_sscb.sscb_interrupt)
	CScancelled(0);
   
    scb->scb_cscb.cscb_gco.gca_association_id =
	    scb->scb_cscb.cscb_assoc_id;
    
    if (last_error &&
	    (last_error != E_GC0000_OK) &&
	    (last_error != E_GC0022_NOT_IACK) &&
	    (last_error != E_GC0027_RQST_PURGED))
    {
	sc0ePut(NULL, last_error, &scb->scb_cscb.cscb_gco.gca_os_status, 0);
        return(last_error);
    }
    if (scb->scb_cscb.cscb_new_stream++)
    {
	cmsg_next = cmsg->scg_next;

        /* Ensure we don't have a 1-node loop */
        if (cmsg_next == cmsg)
        {
            /* Flush the message queue */
            scb->scb_cscb.cscb_mnext.scg_next =
                scb->scb_cscb.cscb_mnext.scg_prev =
                    (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext.scg_next;
            sc0ePut(NULL, E_SC023B_INCONSISTENT_MSG_QUEUE, NULL, 0);
            return(E_SC023B_INCONSISTENT_MSG_QUEUE);
        }

	/* Remove the message from the queue */
	cmsg->scg_prev->scg_next = cmsg->scg_next;
	cmsg->scg_next->scg_prev = cmsg->scg_prev;

	/*
        ** Reset the message chain pointers to reference self.
        ** That way, we can more likely detect some kind of inconsistency.
        */
	cmsg->scg_prev = cmsg;
	cmsg->scg_next = cmsg;

	/*
	** When freeing an IACK clear "purge" flag that was (or may have
	** been) set to preserve alerts over interrupts.
	*/
	if (cmsg->scg_mtype == GCA_IACK)
	    scb->scb_sscb.sscb_aflags &= ~SCS_AFPURGE;

	if ((cmsg->scg_mask & SCG_NODEALLOC_MASK) == 0)
	{
	    status = sc0m_deallocate(0, (PTR *)&cmsg);
	    if (status)
	    {
		sc0ePut(NULL, status, NULL, 0);
		return(status);
	    }
	}
	else if ((cmsg->scg_mask & SCG_QDONE_DEALLOC_MASK) != 0)
	{
	    /* place the message onto the scg_qdone queue; */
	    cmsg->scg_next = scb->scb_cscb.cscb_qdone.scg_next;
	    cmsg->scg_prev = scb->scb_cscb.cscb_qdone.scg_next->scg_prev;
	    scb->scb_cscb.cscb_qdone.scg_next->scg_prev = cmsg;
	    scb->scb_cscb.cscb_qdone.scg_next = cmsg;
	}

	cmsg = cmsg_next;

	/* if the list is empty */
	if (cmsg == (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext)
	{
	    if (scb->scb_sscb.sscb_state == SCS_INPUT)
	    {
		/* deallocate all of the messages on the scg_qdone queue; */
		cmsg = (SCC_GCMSG *) scb->scb_cscb.cscb_qdone.scg_next;
		while ((SCC_GCQUE *) cmsg != &scb->scb_cscb.cscb_qdone)
		{
		    cmsg_next = cmsg->scg_next;

		    /* Ensure we don't have a 1-node loop */
		    if (cmsg_next == cmsg)
		    {
			/* Flush the queue */
			scb->scb_cscb.cscb_qdone.scg_next =
			    scb->scb_cscb.cscb_qdone.scg_prev =
				(SCC_GCMSG *)&scb->scb_cscb.cscb_qdone.scg_next;
			sc0ePut(NULL, E_SC023B_INCONSISTENT_MSG_QUEUE, NULL, 0);
			return(E_SC023B_INCONSISTENT_MSG_QUEUE);
		    }

		    cmsg->scg_prev = cmsg;
		    cmsg->scg_next = cmsg;

		    status = sc0m_deallocate(0, (PTR *)&cmsg);
		    if (status)
		    {
			sc0ePut(NULL, status, NULL, 0);
			return(status);
		    }
		    cmsg = cmsg_next;
		}

		/* All the messages on the query done deallocate queue have
		** been deallocated. Lets reinitialize the queue so that we
		** don't try to deallocate the messages again.
		*/
		scb->scb_cscb.cscb_qdone.scg_next = 
			scb->scb_cscb.cscb_qdone.scg_prev = 
				(SCC_GCMSG *)&scb->scb_cscb.cscb_qdone.scg_next;
	    }

	    scb->scb_cscb.cscb_new_stream = 0;

	    if (last_error == E_GC0001_ASSOC_FAIL)
	    {
		return ((STATUS)last_error);
	    }
	    /* If no alerts or (are alerts but an interrupt needs processing) */
	    if (   scb->scb_sscb.sscb_alerts == NULL
		|| scb->scb_sscb.sscb_interrupt != 0)
	    {
		return ((STATUS)E_CS001D_NO_MORE_WRITES);
	    }

	    /*
	    ** Message queue must be empty - hang around for alerts as they
	    ** may come with empty message queues (see scenotify.c)
	    */
	    queue_was_empty = TRUE;
	}
	else
	{
	    queue_was_empty = FALSE;
	} /* If message queue is empty */
    }
    else 
    {
	/*
	** If there's an empty queue we must have got here to process alerts.
	** It is an error if the queue is empty and there are no alerts.
	** (see scenotify.c), and the GCA wasn't dropped (when we can get
	** here with no queue and no alerts)
	*/
	queue_was_empty = (cmsg == (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext);
	if (queue_was_empty && scb->scb_sscb.sscb_alerts == NULL) 
	{
	    if( scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA )
	    {   /* Just return if GCA is already dead */
		return(E_CS001D_NO_MORE_WRITES);
	    }
	    /* How did we get here?  Log an error */
	    scb->scb_cscb.cscb_mnext.scg_next =
		scb->scb_cscb.cscb_mnext.scg_prev =
		    (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext.scg_next;

	    sc0ePut(NULL, E_SC023B_INCONSISTENT_MSG_QUEUE, NULL, 0);
	    return(E_SC023B_INCONSISTENT_MSG_QUEUE);
	}
    } /* If new stream */

    if (last_error == E_GC0001_ASSOC_FAIL)
    {
	status = last_error;
	return((STATUS) status);
    }

    /*
    ** At this point there are either messages to send (!queue_was_empty)
    ** or alerts to send.  queue_was_empty would have been set if we got
    ** here with an empty queue but there are alerts to send.
    */

    if (!queue_was_empty && scb->scb_sscb.sscb_force_abort == SCS_FACOMPLETE)
    {
	/*
	** Then we did a force abort during message processing.
	** If this is the case, then clear out the current set of messages,
	** insert a force abort error, insert a responce block saying that we
	** had an error, and then proceed as if nothing else happened.
	** The remainder of the code will act the same, and all will proceed
	** swimmingly.
	**
	** If this force abort happened while we were doing a write to the FE,
	** then there is a chance that CS still thinks that the next operation
	** has already completed (that is, that the last operation neither
	** completed through CSsuspend() or had a CScancelled() done).  In order
	** to remedy this problem, we unconditionally do a CScancelled() now.
	** There is no problem with doing these so long as there are no
	** outstanding operations.  There should be none at this time.
	*/

	CScancelled(0);
	
	scc_fa_notify(scb);	    /* Put force abort message in the queue. */
	scb->scb_sscb.sscb_force_abort = 0;
	if (scb->scb_cscb.cscb_incomplete)
	{
	    scc_end_message(scb);
	    scb->scb_cscb.cscb_incomplete = 0;
	    /* End any outstanding incomplete messages */
	}
	cmsg = scb->scb_cscb.cscb_mnext.scg_next;

	/* Note that this message has been sent... */
	
	scb->scb_cscb.cscb_new_stream = 1;
    }
    /*
    ** If there are alerts 
    ** and  either the message queue is empty 
    **      or we haven't got a purging message queued (GCA_IACK), 
    ** and message isn't a continuation (gca_end_of_data == FALSE),
    ** then send the alert.
    ** 
    ** Note that we can do this only if we are not in the midst of a message
    ** as indicated by cscb_incomplete.  This can happen when we are sending a
    ** block portion of a blob and an alert arrives in the middle.
    */
    /* bug 77613 if the queue is empty and there are no more messages
    ** we cannot check cmsg->scg_mask because it is pointing to random
    ** data. If this data has the same value as SCG_NOT_EOD_MASK (0x4)
    ** we get into and endless loop in the server.
    */
    else if (	(scb->scb_cscb.cscb_incomplete == 0)
	     &&	scb->scb_sscb.sscb_alerts != NULL
	     && (   queue_was_empty
		 || (scb->scb_sscb.sscb_aflags & SCS_AFPURGE) == 0
		))
		if ((queue_was_empty) ||
		(!queue_was_empty && (cmsg->scg_mask & SCG_NOT_EOD_MASK) == 0))
    {
	/* Insert alert message and adjust current message context */
	status = sce_notify(scb);
	if (status == E_DB_OK)    /* If sce_notify fails it will log an error */
	{
	    cmsg = scb->scb_cscb.cscb_mnext.scg_next;
	    scb->scb_cscb.cscb_new_stream = 1;
	    if (queue_was_empty)    /* We must have been called to send event */
	    {
		SCS_ASTATE_MACRO("scc_send/alerts",scb->scb_sscb,SCS_ASPENDING);
	    }
	}
	else if (queue_was_empty)
	{
	    /*
	    ** sce_notify failed (and logged an error) and we really didn't
	    ** have any more messages.  Treat this as a normal case where
	    ** we've finished sending all our messages (like we do earlier) -
	    ** this allows us to continue with the thread.
	    */
	    return ((STATUS)E_CS001D_NO_MORE_WRITES);
	}
	/*
	** Else, sce_notify failed, but we can still return the original
	** message on front of queue
	*/
    }

    /*
    **	If there was an incomplete message, scan the queue for the next message
    **	to send.  If it is not there, than the queue remains unchanged, and we
    **	return to await the continuation.  If the completion or continuation
    **	(the message could span > 2 blocks) message is present, allow it to be
    **	sent.
    */
    
    while (scb->scb_cscb.cscb_incomplete
	    && (cmsg != (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext)
	    && (scb->scb_cscb.cscb_incomplete != cmsg->scg_mtype))
    {
	switch (cmsg->scg_mtype)
	{
	    case    GCA_RELEASE:
	    case    GCA_DONE:
	    case    GCA_RESPONSE:
	    case    GCA_S_ETRAN:
	    case    GCA_RETPROC:
		/*
		**  I.e. any message which will be the "your turn" message:
		**  these are errors if encountered in this loop.  They mean
		**  that a message was not completed as expected.
		*/
		sc0ePut(NULL, E_SC0255_INCOMPLETE_SEND, NULL, 2,
			 sizeof(cmsg->scg_mtype), (PTR)&cmsg->scg_mtype,
			 sizeof(scb->scb_cscb.cscb_incomplete),
			 (PTR)&scb->scb_cscb.cscb_incomplete,
			 0, (PTR)0, 0, (PTR)0, 0, (PTR)0, 0, (PTR)0);
		scb->scb_cscb.cscb_incomplete = 0;
		scc_end_message(scb);
		cmsg = scb->scb_cscb.cscb_mnext.scg_next;
			/* Start from scratch... */
		break;

	    case    GCA_IACK:
		/* This one's OK */
		scb->scb_cscb.cscb_incomplete = 0;
		break;
		
	    default:
		cmsg = cmsg->scg_next;
		break;
	}
    }
    if (cmsg == (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext)
    {
	/*
	** In this case, then there are no messages to be sent which can be
	** sent.  In this case, simply return and await more messages to be
	** sent.
	*/
	
	scb->scb_cscb.cscb_new_stream = 0;
	return(E_CS001D_NO_MORE_WRITES);
    }

    if (scb->scb_cscb.cscb_incomplete
	    && (cmsg != scb->scb_cscb.cscb_mnext.scg_next)
	    && (cmsg->scg_mtype == scb->scb_cscb.cscb_incomplete))
    {
	/* Move message to front of queue so it gets deleted correctly */
	cmsg->scg_prev->scg_next = cmsg->scg_next;
	cmsg->scg_next->scg_prev = cmsg->scg_prev;
	cmsg->scg_next = scb->scb_cscb.cscb_mnext.scg_next;
	cmsg->scg_prev = scb->scb_cscb.cscb_mnext.scg_next->scg_prev;
	scb->scb_cscb.cscb_mnext.scg_next->scg_prev = cmsg;
	scb->scb_cscb.cscb_mnext.scg_next = cmsg;
    }

    scb->scb_cscb.cscb_new_stream = 1;
    
    scb->scb_cscb.cscb_gco.gca_descriptor = cmsg->scg_mdescriptor;
    scb->scb_cscb.cscb_gco.gca_buffer = cmsg->scg_marea;
    scb->scb_cscb.cscb_gco.gca_msg_length = 
				    (cmsg->scg_msize ? cmsg->scg_msize : 1);
    scb->scb_cscb.cscb_gco.gca_message_type = cmsg->scg_mtype;
    scb->scb_cscb.cscb_gco.gca_modifiers = 0;

    /*
    ** If gca_end_of_data can ever be set to FALSE then make sure never to
    ** insert a different message type (i.e. GCA_EVENT) in between the
    ** incomplete GCA message.
    */

    if ((cmsg->scg_mask & SCG_NOT_EOD_MASK) == 0)
    {
	int 	batch_count = scb->scb_cscb.cscb_batch_count;
	
	scb->scb_cscb.cscb_gco.gca_end_of_data = TRUE;
	scb->scb_cscb.cscb_incomplete = 0;
	if (scb->scb_cscb.cscb_batch_count > 0 || scb->scb_cscb.cscb_in_group)
	{
	    /* don't decrement batch count for errors or RETPROC, there will be
	     * a GCA_RESPONSE immediately following this
	     */
	    if (cmsg->scg_mtype == GCA_RETPROC && scb->scb_cscb.cscb_in_group)
	    {
		scb->scb_cscb.cscb_gco.gca_modifiers = GCA_NOT_EOG;
	    }
	    else if (cmsg->scg_mtype == GCA_ERROR && scb->scb_cscb.cscb_in_group )
	    {
		scb->scb_cscb.cscb_gco.gca_modifiers = GCA_NOT_EOG;
	    }
	    else
	    {
		/* this must either be GCA_RESPONSE (since batch only supports
		 * non row returning queries) or we are not in group
		 * processing
		 */
		if (scb->scb_cscb.cscb_eog_error)
		{
		    /* error forced end of group */
		    scb->scb_cscb.cscb_gco.gca_modifiers = GCA_EOG;
		    scb->scb_cscb.cscb_batch_count = 0;
		}
		else if (scb->scb_cscb.cscb_batch_count > 0)
		    scb->scb_cscb.cscb_gco.gca_modifiers = GCA_NOT_EOG;
		else
		    scb->scb_cscb.cscb_gco.gca_modifiers = GCA_EOG;
		if (scb->scb_cscb.cscb_batch_count)
		    scb->scb_cscb.cscb_batch_count--;
	    }
	}
	else if (cmsg->scg_mtype == GCA_RESPONSE ||
		cmsg->scg_mtype == GCA_ERROR)
	    scb->scb_cscb.cscb_gco.gca_modifiers = GCA_EOG;
	if (batch_count <= 0)
	    scb->scb_cscb.cscb_in_group = FALSE;
    }
    else
    {
	scb->scb_cscb.cscb_gco.gca_end_of_data = FALSE;
	scb->scb_cscb.cscb_incomplete = cmsg->scg_mtype;
    }
    /*
    **	Flush SXF audit data prior to communicating with the user
    */
    status = sc0a_flush_audit(scb, &cur_error);

    if (status != E_DB_OK)
    {
	/*
	**	Write an error
	*/
	sc0ePut(NULL, cur_error, NULL, 0);
	return cur_error;
    }

    status = IIGCa_call(GCA_SEND,
			(GCA_PARMLIST *)&scb->scb_cscb.cscb_gco,
			(sync ? GCA_SYNC_FLAG : 0),
			(PTR)&scb->cs_scb,
			-1,
			&cur_error);

    if (status)
    {
	status = cur_error;
	if( (scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA) == 0 )
	    sc0ePut(NULL, status, NULL, 0);
	if (!sync)
	    CScancelled(0); /* mark as nothing complete */
    }
    else if (scb->scb_sscb.sscb_astate == SCS_ASPENDING)
    {
	/* Pending alert-sends do not suspend on return (see scenotify.c) */
	status = E_CS001D_NO_MORE_WRITES;
    }

    return((STATUS) status);
}

/*{
** Name: scc_recv	- Receive data from FE.
**
** Description:
**      This routine requests the reception of a message from
**      the FE.  It is a simple routine, provided to insulate 
**      the CLF\CS from the ``nastiness'' of ingres code.
**
** Inputs:
**      scb                             Scb to be read from.
**	sync				Should GCF perform
**					this request synchronously
**
** Outputs:
**      none
**	Returns:
**	    STATUS	(since to a CL routine)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jul-1987 (fred)
**          Created.
**      27-jul-88 (fred)
**	    Minor tweek to make scc_recv() return immediately of there is a force
**	    abort pending for this session.
**	14-Aug-1989 (fred)
**          Added code so that read's will always perform CScancelled before
**          processing.  This closes a window which exists after
**          interrupts.
**	10-dec-90 (neil)
**	    Added work/checks for alerters.  For a detailed description of
**	    alert-related processing refer to the large comment in scenotify.c.
**	10-may-1991 (fred)
**	    Improved interrupt handling characteristics (i.e. made them work)
**	    by checking for interrupts and uncompleted reads in a few more
**	    places.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	23-aug-1993 (andys)
**	    Check the status set by gca_receive in scc_recv. Make sure
**	    we return FAIL if it is not what we want. [bug 42284]
**	    [was 16-sep-92 (andys), 05-oct-92 (andys)]
**      04-Jul-2000 (horda03)
**          Disable ASTs (on VMS) while accessing the GCA_STATUS variable.
*/
STATUS
scc_recv( SCD_SCB *scb, i4  sync )
{
    DB_STATUS		    status;
    DB_STATUS		    error;
    GCA_RV_PARMS	    *rparms = &scb->scb_cscb.cscb_gci;
#ifdef VMS
    int			    ast_enabled;
#endif

    /*
    ** First check if we are about to read but we were left in an
    ** "ignore GCA completion" alert state.  This is a serious error.
    */
    if (scb->scb_sscb.sscb_astate == SCS_ASIGNORE)
    {
	sc0ePut(NULL, E_SC0275_IGNORE_ASTATE, NULL, 0);
	return (E_SC0275_IGNORE_ASTATE);
    }
    if (scb->scb_sscb.sscb_interrupt ||
	    (scb->scb_sscb.sscb_force_abort == SCS_FAPENDING))
    {
	return(E_CS001E_SYNC_COMPLETION);
    }
#ifdef VMS
    ast_enabled = sys$setast(0);
#endif

    if (   rparms->gca_status != E_GCFFFF_IN_PROCESS
        && rparms->gca_status != E_SC1000_PROCESSED)
    {
#ifdef VMS
        if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif
	/* GCA finished read but we haven't yet processed */
	return(E_CS001E_SYNC_COMPLETION);
    }
#ifdef VMS
    if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif

    /*
    ** Must be about to read or already in middle of read.
    **
    ** Check for alerts first so that we always make sure to turn on
    ** the "alert interruptable" flag if we are an idle thread (an
    ** already read-pending thread is indicated by E_GCFFFF_IN_PROCESS).
    ** This way, not just the *first* alert that interrupts the
    ** idle thread will turn on "alert interruptable".  For example, if
    ** a read was pending as the thread got an alert, processed it, and
    ** returned here - we make sure to turn "interruptable" back on.
    ** Supress the check when we have already sent an alert to the client
    ** (and it's still "pending") or when we are not trying to read normal
    ** input (ie. COPY processing may have a bunch of read & writes).
    */
    if (   (Sc_main_cb->sc_alert != NULL)		/* If fielding alerts */
	&& (scb->scb_sscb.sscb_astate != SCS_ASPENDING) /* & no pending sends */
	&& (scb->scb_sscb.sscb_state == SCS_INPUT)	/* & normal input     */
       )
    {
	CSp_semaphore(TRUE, &scb->scb_sscb.sscb_asemaphore);
	if (scb->scb_sscb.sscb_alerts != NULL)
	{
	    /* Alerts to process - stuff to do so return asap */
	    CSv_semaphore(&scb->scb_sscb.sscb_asemaphore);
	    return (E_CS001E_SYNC_COMPLETION);
	}
	else
	{
	    /* No alerts - we are now alert-interruptable */
	    SCS_ASTATE_MACRO("scc_recv/idle", scb->scb_sscb, SCS_ASWAITING);
	    CSv_semaphore(&scb->scb_sscb.sscb_asemaphore);
	}
    }

#ifdef VMS
    ast_enabled = sys$setast(0);
#endif

    /* If we are in the middle of a read then continue with it now */
    if (rparms->gca_status == E_GCFFFF_IN_PROCESS)
    {
#ifdef VMS
        if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif
	return(OK);
    }
    else if (rparms->gca_status != E_SC1000_PROCESSED)
    {
#ifdef VMS
        if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif

	return (E_CS001E_SYNC_COMPLETION);
    }
#ifdef VMS
    if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif
    CScancelled(0); /* Close interrupt handling window w/in GCF */
	
    rparms->gca_association_id = scb->scb_cscb.cscb_assoc_id;
    rparms->gca_flow_type_indicator = GCA_NORMAL;
    rparms->gca_buffer = scb->scb_cscb.cscb_inbuffer;
    rparms->gca_b_length = scb->scb_cscb.cscb_comm_size;
    rparms->gca_descriptor = NULL; /* EJLFIX: Fix for copies */
    rparms->gca_modifiers = 0;
    status = IIGCa_call(GCA_RECEIVE,
		    (GCA_PARMLIST *)rparms,
		    (sync ? GCA_SYNC_FLAG : 0),
		    (PTR)&scb->cs_scb,
		    -1,
		    &error);
    if (status)
    {
	sc0ePut(NULL, error, NULL, 0);
	if (!sync)
	    CScancelled(0); /* mark as nothing complete */
    }
#ifdef VMS
    ast_enabled = sys$setast(0);
#endif
    if ((rparms->gca_status != E_GCFFFF_IN_PROCESS) &&
	(rparms->gca_status != E_GC0000_OK) &&
	(rparms->gca_status != E_GC0027_RQST_PURGED))
    {
#ifdef VMS
        if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif
	return FAIL ;
    }

#ifdef VMS
    if (ast_enabled == SS$_WASSET) sys$setast(1);
#endif

    if (scb->scb_sscb.sscb_interrupt ||
	    (scb->scb_sscb.sscb_force_abort == SCS_FAPENDING))
    {
	return(E_CS001E_SYNC_COMPLETION);
    }
    return((STATUS) status);
}

/*{
** Name: scc_dispose	- Delete sent data blocks
**
** Description:
**      This routine deallocates data blocks which have already been sent
**	or are listed to be sent and the server `changes its mind' (as is
**	the case after a user interrupt or force abort interrupt.
**      It is called when a "string" of blocks has been queued, and all are 
**      to be deleted.
**
** Inputs:
**      scb                             The scb in question.
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-Jul-1987 (fred)
**          Created.
**      15-may-89 (eric)
**          This shouldn't toss error or trace messages.
**      06-fep-90 (ralph)
**          Check consistency of chain pointers.
**      10-dec-90 (neil)
**          This shouldn't toss GCA_EVENT messages either.  When, we are in
**	    interrupt mode, however, we should purge ALL messages (instead
**	    of GCA doing this).
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              10-oct-1990 (georgeg)
**                  reinitialize the message queue before returning to caller.
**          End of STAR comments.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@History_template@]...
*/
VOID
scc_dispose( SCD_SCB *scb )
{
    DB_STATUS           status;
    SCC_GCMSG		*del_cmsg;
    SCC_GCMSG		*next_cmsg;
    
    /* Start deleting messages at the beginning of the message queue */
    del_cmsg = scb->scb_cscb.cscb_mnext.scg_next;

    /* while there are more messages to delete */
    while (del_cmsg != (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext)
    {
	/* find the next message that is after the current one that is
	** being deleted
	*/
	next_cmsg = del_cmsg->scg_next;

        /* Ensure we don't have a 1-node loop */
        if (next_cmsg == del_cmsg)
        {
            /* Flush the message queue */
            scb->scb_cscb.cscb_mnext.scg_next =
                scb->scb_cscb.cscb_mnext.scg_prev =
                    (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext.scg_next;
            sc0ePut(NULL, E_SC023B_INCONSISTENT_MSG_QUEUE, NULL, 0);
            break;
        }
	
	/*
	** If we are not in interrupt mode (when GCA purges anyway) and
	** the message isn't an error, tracing or event, then delete it
	*/
	if (scb->scb_sscb.sscb_interrupt == 0 &&
	    del_cmsg->scg_mtype != GCA_ERROR &&
	    del_cmsg->scg_mtype != GCA_TRACE &&
	    del_cmsg->scg_mtype != GCA_EVENT
	   )
	{
	    /* take it off of the queue that it's on */
	    del_cmsg->scg_prev->scg_next = del_cmsg->scg_next;
	    del_cmsg->scg_next->scg_prev = del_cmsg->scg_prev;

            /*
            ** Reset the deleted message chain pointers to reference self.
            ** That way, we can more likely detect some kind of inconsistency.
            */
            del_cmsg->scg_prev = del_cmsg;
            del_cmsg->scg_next = del_cmsg;

	    /* if it's deallocatable, then deallocate it. */
	    if ((del_cmsg->scg_mask & SCG_NODEALLOC_MASK) == 0)
	    {
		status = sc0m_deallocate(0, (PTR *)&del_cmsg);
		if (status)
		    sc0ePut(NULL, status, NULL, 0);
	    }
	}

	del_cmsg = next_cmsg;
    }

    /* If the queue is empty then clear out cscb_new_stream */
    if (scb->scb_cscb.cscb_mnext.scg_next == 
	    (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext
	)
    {
	scb->scb_cscb.cscb_new_stream = 0;
    }
}

/*{
** Name: scc_iprime	- Prime system to receive interrupt notification
**
** Description:
**      This routine calls GCF with the appropriate parameters to set up 
**      for receipt of an expedited message.
**
** Inputs:
**      scb                             Session control block.
**
** Outputs:
**      None
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Jul-1987
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	29-Jul-1993 (daveb)
**	    don't log INV_ASSOC_ID, in case we dropped it by hand.
[@history_template@]...
*/
VOID
scc_iprime( SCD_SCB *scb )
{
    DB_STATUS           status;
    DB_STATUS		error;
    GCA_RV_PARMS	*rparms = &scb->scb_cscb.cscb_gce;

    rparms->gca_association_id = scb->scb_cscb.cscb_assoc_id;
    rparms->gca_flow_type_indicator = GCA_EXPEDITED;
    rparms->gca_buffer = (PTR) scb->scb_cscb.cscb_intr_buffer;
    rparms->gca_b_length = sizeof(scb->scb_cscb.cscb_intr_buffer);
    rparms->gca_descriptor = NULL;
    rparms->gca_modifiers = 0;
    status = IIGCa_call(GCA_RECEIVE,
			(GCA_PARMLIST *)rparms,
			0,
		        (PTR)&scb->cs_scb,
			-1,
			&error);
    if (status && error != E_GC0005_INV_ASSOC_ID )
    {
	sc0ePut(NULL, error, NULL, 0);
    }
}

/*{
** Name: scc_gcomplete	- Inform system that a GCA event has completed.
**
** Description:
**      This routine intercepts the GCA call to inform the system that 
**      GCA processing is complete.  If the session is doing normal 
**      processing of queries, then this call simply makes a direct 
**      CSresume() call.  If, however, the session is aborting an 
**      oldest transaction, the this call is ignored pending the 
**      flushing of the queue.
**
** Inputs:
**      sid                             Session id
**
** Outputs:
**      None
**	Returns:
**	    VOID
**	Exceptions:
**	    None.
**
** Side Effects:
**	    none
**
** History:
**      30-Nov-1987 (fred)
**          Created.
**	10-dec-1990 (neil)
**	    Added extra check for alerters.  For a detailed description of
**	    why we supress CSresume in certain alert states see large comment
**	    in scenotify.c.
**	10-may-1991 (fred)
**	    Changed check for completedness to correspond with scsqncr.c &
**	    scssvc.c fixes.  Fixes read/read deadlock problem.
**	9-Aug-91 (daveb)
**	    Explicitly check that this is an SCD_SCB before trying to
**	    decode it.  Add debug tracing to prove that it is working.
**	    This is the key part to the fix for bug 38056.  Fixed date
**	    on fred's history above (it's a '91 fix, not a '90).
**      10-mar-93 (mikem)
**          bug #47624
**          Disable CSresume() calls from scc_gcomplete() while session is
**          running down in scs_terminate() as indicated by the
**          sccb_terminating field.  Allowing CSresume()'s during this time
**          leads to early wakeups from CSsuspend()'s called by scs_terminate
**          (like DI disk I/O operations).
**	2-Jul-1993 (daveb)
**	    prototyped.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	03-jul-95 (emmag)
**	    Desktop porting changes. CSresume takes a SID, not an SCB pointer, 
**	    except for CS_ADDER_ID.
**	03-jun-1996 (canor01)
**	    Generic fix for SCB/SID problem.  scb->cs_self is acceptible
**	    for SIDs that are thread ids and for those that are SCB pointers.
[@history_template@]...
*/
VOID
scc_gcomplete( SCD_SCB *scb )
{
    /*
    ** In English, 
    **	    if the scb exists and 
    **		this is a new session
    **	    	or this is a CS_SCB, but doesn't look like an SCD_SCB,
    **	    or     we are not force aborting a thread,
    **	       and we are not in an interrupt,
    **	       and we are not ignoring because of an alert state,
    **          then resume the thread.
    */
    if( scb != NULL )
    {
	if ( (CS_SID)scb == CS_ADDER_ID )
	{
	    CSresume((CS_SID)scb);
	}	
	else if ( scb->cs_scb.cs_client_type != SCD_SCB_TYPE )
	{
# ifdef xDEBUG
	    if ( !((scb->scb_sscb.sscb_force_abort != SCS_FORCE_ABORT)
		   && ( scb->scb_sscb.sscb_interrupt <= 0 )
		   && ( scb->scb_sscb.sscb_astate != SCS_ASIGNORE) ))
		TRdisplay("scc_gcomplete:  thread %p type %x would have hung.\n", 
			  scb, scb->cs_scb.cs_client_type );
# endif
	    CSresume(scb->cs_scb.cs_self);
	}
	else if ( (scb->scb_sscb.sscb_force_abort != SCS_FORCE_ABORT)
		  && ( scb->scb_sscb.sscb_interrupt <= 0 )
		  && ( scb->scb_sscb.sscb_terminating == 0)
	          && ( scb->scb_sscb.sscb_astate != SCS_ASIGNORE) )
	{
	    CSresume(scb->cs_scb.cs_self);
	}
	else
	{
# ifdef xDEBUG
	    TRdisplay("scc_gcomplete:  NO RESUME, cs_client_type %x\n",
		    scb->cs_scb.cs_client_type );
# endif
	}
    }
}

/*{
** Name: scc_fa_notify	- Send force_abort error sequence
**
** Description:
**      This routine notifies the connected frontend that a forced abort has
**	occured, and that, more importantly, he/she/it was the victim.  This is
**	done by
**	    - clearing the message stream,
**	    - inserting the error block (# 4706)
**	    - and inserting the appropriate type of responce block (or best
**		    guess).
**
**	Once this is done, the caller is expected to have these messages sent in
**	the usual way.
**
**	It is the caller's responsibility to guarantee that this session is not
**	busy, and that these messages can be deallocated (that is, that there
**	are no outstanding GCF operations on these messages).  This routine
**	assumes that it is free to perform its function.
**	For a RAAT session, the caller is also responsible to ensure that
**	a generic 'sscb_raat_message' area has been allocated.
**
** Inputs:
**      scb                             Session to notify
**
** Outputs:
**      (none)
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Error message and responce block are placed on the outgoing message
**	    queue for this session.
**
** History:
**      27-jul-89 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	06-nov-1996 (cohmi01)
**	    Add message block management for RAAT. Include
**	    erusf.h to obtain #define for err 4706.        Bug 78797.
[@history_template@]...
*/
VOID
scc_fa_notify( SCD_SCB *scb )
{
    SCC_GCMSG		*message;

    scc_dispose(scb);
    if (scb->scb_cscb.cscb_incomplete != 0)
    {
	scc_end_message(scb); /* Insert 0 length message */
	scb->scb_cscb.cscb_incomplete = 0;
    }

    /* Format of message block differs between SQL and RAAT */
    if ( (scb->scb_cscb.cscb_gcp.gca_it_parms.gca_message_type == GCA_QUERY &&
        ((GCA_Q_DATA *)(scb->scb_cscb.cscb_gcp.gca_it_parms.gca_data_area))
           ->gca_language_id == DB_NDMF))
    {
	char 	*outbuf;

	/* Message type indicates caller is a RAAT program */

	message = (SCC_GCMSG *) &scb->scb_sscb.sscb_raat_message->message;
	message->scg_msize = sizeof(i4);  /* size of err_code */
	message->scg_mask = SCG_NODEALLOC_MASK;

	/* message data consists of Force_Abort error code */
	outbuf = (char *)(message->scg_marea);
	*((i4 *)outbuf) = E_US1262_4706;

	/* Queue up message for SCF to return to RAAT program  */
	message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;

	/* Since current RAAT request will be ignored, purge buffer queue */
	scb->scb_sscb.sscb_raat_buffer_used = 0;
    }
    else
    {
	sc0e_uput(E_US1262_4706, NULL, 0);

	/* Now insert a responce block */
	message = &scb->scb_cscb.cscb_rspmsg;
	message->scg_mtype = scb->scb_sscb.sscb_rsptype;
	message->scg_next = 
	    (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = 
	    scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
	    message;
	scb->scb_cscb.cscb_mnext.scg_prev = 
	    message;
	message->scg_mask = SCG_NODEALLOC_MASK;
	if (message->scg_mtype == GCA_DONE)
	{
	    scb->scb_sscb.sscb_rsptype = GCA_REFUSE;
	}
	message->scg_msize = sizeof(GCA_RE_DATA);
    }
}

/*{
** Name: scc_relay	- relay an LDB message to client
**
** Description:
**      This function provides a way to relay message from
**	LDB to client. It is used mainly for GCA_ERROR and GCA_TRACE.
**
** Inputs:
**      scf_cb
**          .scf_ptr_union.scf_buffer   pointer to text of error message
**
** Outputs:
**      scf_cb
**          .error
**              .err_code               E_SC_OK or ...
**		    E_SC000D_BUFFER_ADDR    error buffer not addressable
**	Returns:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Jan-89 (alexh)
**          Created.
**	29-nov-1989 (georgeg)
**	    if there is no FE to this session do not que the message.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.  Use proper PTR for allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/
DB_STATUS
scc_relay( SCF_CB *scf_cb, SCD_SCB *scb )
{
    DB_STATUS		status;
    SCC_GCMSG		*ebuf;
    GCA_RV_PARMS	*relay_msg;
    PTR			block;

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    if (!scf_cb->scf_ptr_union.scf_buffer)
    {
	SETDBERR(&scf_cb->scf_error, 0, E_SC000D_BUFFER_ADDR);
	status = E_DB_ERROR;
    }
    else if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
    {

	relay_msg = (GCA_RV_PARMS *)scf_cb->scf_ptr_union.scf_buffer;

	/* allocate buffer for the relay message */
	status = sc0m_allocate(0,
			       (i4)(sizeof(SCC_GCMSG) + 
			       scf_cb->scf_len_union.scf_blength),
			       SCS_MEM,
			       (PTR) DB_SCF_ID,
			       SCG_TAG,
			       &block);
	ebuf = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0ePut(NULL, status, NULL, 0);
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    return(E_DB_ERROR);
	}
				
	ebuf->scg_mtype = relay_msg->gca_message_type;
	ebuf->scg_mask = 0;
	ebuf->scg_msize = relay_msg->gca_d_length;
	ebuf->scg_marea = (PTR)((char *) ebuf + sizeof(SCC_GCMSG));

	/* copy the message */
	MEcopy(relay_msg->gca_buffer,
	       scf_cb->scf_len_union.scf_blength,
	       ebuf->scg_marea);

	/* put the message in the queue */
	ebuf->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	ebuf->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = ebuf;
	scb->scb_cscb.cscb_mnext.scg_prev = ebuf;
    }
    return(status);
}

/*{
** Name: scc_end_message	- Complete an incomplete message
**
** Description:
**      This routine sends a zero length message of the type specified in the
**	cscb_incomplete field of the session control block.  This is used when
**	the DBMS encounters an error which prevents the completion of the
**	sending of some tuple data.  This is most likely to occur during
**	large datatype processing. 
**
** Inputs:
**      scb                             Session Control Block
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-Dec-1989 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
static VOID
scc_end_message( SCD_SCB *scb )
{
    DB_STATUS           status;
    SCC_GCMSG           *message;
    DB_STATUS		error;
    PTR			block;

    status = sc0m_allocate(0,
		(i4)(sizeof(SCC_GCMSG)),
		SCS_MEM,
		(PTR) DB_SCF_ID,
		SCG_TAG,
		&block);
    message = (SCC_GCMSG *)block;
    if (status)
    {
	sc0ePut(NULL, status, NULL, 0);
	return;
    }
    message->scg_mask = 0;
    message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
    message->scg_next = (SCC_GCMSG *) scb->scb_cscb.cscb_mnext.scg_next;
    message->scg_prev = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    scb->scb_cscb.cscb_mnext.scg_next->scg_prev = message;
    scb->scb_cscb.cscb_mnext.scg_next = message;

    message->scg_mtype = scb->scb_cscb.cscb_incomplete;
    message->scg_msize = 0;
}

/*{
** Name: scc_prompt	- send a prompt messge to a user
**
** Description:
**      This function provides a way to send prompts to the frontend
**      program.  Note that this doesn't actually send the prompt, it
**	simply allocates the GCA message and puts it on the message
**	queue. The caller is responsible for enforcing the prompt 
**	send/receieve protocol.
**
** Inputs:
**	scf_cb	- SCB
**
**	mesg	 - Prompt message string
**
**	mesglen  - Length of mesg
**
**	replylen - Maximum reply len
**
**	noecho   - True if prompt is NOECHO
**
**      password - True if prompt is for user password
**
**	timeout	 - Timeout value (0 if notimeout) 
**
** Outputs:
**	    E_DB_{OK, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	5-nov-93  (robf)
**          Created 
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      04-Jul-2000 (horda03)
**          Disable ASTs (on VMS) while accessing the GCA_STATUS variable.
*/
DB_STATUS
scc_prompt(
SCD_SCB *scb, 
char 	*mesg, 
i4  	mesglen, 
i4   	replylen,
bool 	noecho,
bool 	password,
i4	timeout
)
{
    DB_STATUS           ret_val;
    DB_STATUS		status;
    SCC_GCMSG		*pbuf;
    GCA_PROMPT_DATA	*pdata;
    i4		eseverity;
    PTR			block;

#ifdef VMS
    int ast_enabled;
#endif

    if ( (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) == 0 ||
        !(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_PROMPT) ||
        scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
    {
	/*
	** Can't prompt if no GCA or partner doesn't handle prompts
	*/
	ret_val = E_DB_ERROR;
    }
    else
    {

	if (Sc_main_cb->sc_gca_cb)	/* if not running standalone */
	{
	    GCA_DATA_VALUE	*gca_data_value;
	    
	    status = sc0m_allocate(0,
			    (i4)(sizeof(SCC_GCMSG)
				+ sizeof(GCA_PROMPT_DATA)
				+ mesglen + 32),
			    SCS_MEM,
			    (PTR) DB_SCF_ID,
			    SCG_TAG,
			    &block);
	    pbuf = (SCC_GCMSG *)block;
	    if (status)
	    {
		sc0ePut(NULL, status, NULL, 0);
		return(E_DB_ERROR);
	    }
				
	    pbuf->scg_mtype = GCA_PROMPT;
	    pbuf->scg_mask = 0;
	    pbuf->scg_msize = sizeof(GCA_PROMPT_DATA) -
				    sizeof(GCA_DATA_VALUE)
					+ sizeof(i4)
					+ sizeof(i4)
					+ sizeof(i4)
				    + mesglen;
	    pbuf->scg_marea = (PTR)((char *) pbuf + sizeof(SCC_GCMSG));
	    pdata = (GCA_PROMPT_DATA *)(pbuf->scg_marea);
	    pdata->gca_l_prmesg = 1;
	    pdata->gca_prflags=0;
	    if(noecho)
		pdata->gca_prflags|= GCA_PR_NOECHO;
	    if(password)
		pdata->gca_prflags|= GCA_PR_PASSWORD;

	    if(timeout>0)
	    {
		pdata->gca_prflags|= GCA_PR_TIMEOUT;
		pdata->gca_prtimeout= timeout;
	    }
	    else
		pdata->gca_prtimeout= 0;

	    pdata->gca_prmaxlen = replylen;

	    gca_data_value = &pdata->gca_prmesg[0];
	    gca_data_value->gca_type = DB_CHA_TYPE;
	    gca_data_value->gca_precision = 0;
	    gca_data_value->gca_l_value = mesglen;
	    MEcopy((PTR)mesg, mesglen, (PTR)gca_data_value->gca_value);
	    pbuf->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	    pbuf->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;

	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = pbuf;
	    scb->scb_cscb.cscb_mnext.scg_prev = pbuf;
	}
	ret_val = E_DB_OK;
    }
    return(ret_val);
}
