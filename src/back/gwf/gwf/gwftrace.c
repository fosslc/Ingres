/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <tr.h>
#include    <st.h>
#include    <me.h>
#include    <cs.h>
#include    <lk.h>
#include    <lg.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <gwf.h>
#include    <gwfint.h>
#include    <gwftrace.h>
/*
**
**  Name: GWFTRACE.C - GWF trace entry point.
**
**  Description:
**
**	Function prototypes defined in GWFTRACE.H.
**
**      This routine takes a GWF trace request and processes it.
**
**          gwf_trace - The GWF trace entry point.
**
**  History:
**      14-sep-92 (robf)
**          Created for C2/Security Audit gateway
**	16-nov-92 (robf)
**	    Remove call to gwsxa_stats to allow RMS to compile.
**	    Needs proper exit for tracing
**	17-nov-92 (robf)
**	    Moved gwf_session_id from here into gwfses.c at schang's
**	    suggestions (its really session-level rather than tracing)
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-sep-93 (swm)
**	    Moved cs.h include to be above lg.h since the new CS_SID type
**	    is needed in lg.h.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made gwf_display() portable. The old gwf_display()
**	    took an array of variable-sized arguments. It could
**	    not be re-written as a varargs function as this
**	    practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function,
**	    TRformat, directly.
**	    The gwf_display() function has now been deleted, but
**	    for flexibilty gwf_display has been #defined to
**	    TRformat to accomodate future change.
**	    All calls to gwf_display() changed to pass additional
**	    arguments required by TRformat:
**	        gwf_display(gwf_scctalk,0,trbuf,sizeof(trbuf) - 1,...)
**	    this emulates the behaviour of the old gwf_display()
**	    except that the trbuf must be supplied.
**	    Note that the size of the buffer passed to TRformat
**	    has 1 subtracted from it to fool TRformat into NOT
**	    discarding a trailing '\n'.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**/
GLOBALREF GW_FACILITY *Gwf_facility;
FUNC_EXTERN	DB_STATUS	scf_call();
static    	bool          	gwf_tracing_on;

/*{
** Name: gwf_trace	- GWF trace entry point.
**
** Description:
**      This routine takes a standard trace control block as input, and
**	sets or clears the given trace point.  The trace points are either
**	specific to a session or server wide.
**
** Inputs:
**      debug_cb                        A standard debug control block.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1992 (robf)
**          Created  for C2/Security Audit gateway
*/
DB_STATUS
gwf_trace(
DB_DEBUG_CB  *debug_cb
)
{
	FUNC_EXTERN VOID gwsxa_stats();
	DB_DEBUG_CB *d = debug_cb;
	GW_FACILITY *gwf=Gwf_facility;
	i4	    bitno = d->db_trace_point;
	GW_SESSION  *session;
	DB_STATUS   status;
	i4	    *vector  ;

	status=E_DB_OK;
	for(;;)
	{
		/*
		**	Check if per session or global
		**
		**	Global are between 50 and 99 
		**
		*/
		if(bitno>=50 && bitno <=99)
			vector= Gwf_facility->gwf_trace;
		else
		{
			/*
			**	Get session id
			*/
			status=gwf_session_id(&session);
			if( status!=E_DB_OK || session==NULL)
			{
				TRdisplay("GWF_TRACE: Error getting GWF session id to set trace point\n");
				break;
			}
			vector=session->gws_trace;
			if (vector==NULL)
			{
				TRdisplay("GWF: NULL session  trace vector\n");
				break;
			}
		}
		/*
		**	Set the trace points
		*/
		if (d->db_trswitch == DB_TR_ON)
		{
			TRdisplay("Setting GWF trace point %d\n",bitno);
			GWF_SET_MACRO(vector, bitno);
		}
		else 
		{
			TRdisplay("Resetting GWF trace point %d\n",bitno);
			GWF_CLR_MACRO(vector, bitno);
		}
		break;
	}
	gwf_tracing_on=TRUE;
	return (status);    
}

/*
**
**  Name: gwf_scc_trace - Send a trace to the user via SCF
**
**
**  History:
**      24-sep-92 (robf)
**          Created for C2/Security Audit gateway, cloned from dmf tracing
**/
VOID
gwf_scc_trace( char *msg_buffer)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)STlength(msg_buffer);
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d while attempting to send info from GWF\n",
	    scf_cb.scf_error.err_code);
    }
}

/*{
** Name: gwf_scctalk	- send output line to front end
**
** Description:
**      Send a formatted output line to the front end. A NL character
**	is added to the end of the message. Buffer to which a pointer
**	is received should have on extra byte for `\n'.
**
** Inputs:
**	arg				place holder
**	msg_len				length of message
**      msg_buffer                      ptr to message
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	24-sep-92 (robf)
**		Created
*/
i4
gwf_scctalk(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer)
{
    char	 *p;

    if (msg_length == 0)
	return;

    /* Truncate message if necessary */
    if (msg_length > GWF_MAX_MSGLEN)
	msg_length = GWF_MAX_MSGLEN;

    /* If last char is null, replace with NL, else add NL */
    if (*(p = (msg_buffer + msg_length - 1)) == '\0')
	*p = '\n';
    else
    {
	*(++p) = '\n';
	msg_length++;
    }
    /*
    **	Sendt output to the user
    */
    gwf_scc_trace(msg_buffer);
}

/*{
** Name: gwf_chk_sess_trace	- Checks if a session trace flag is set.
**
** Description:
**      Checks to see if a session trace flag is set.
**
** Inputs:
**	None
**
** Outputs:
**	Returns:
**	    bool
**
** History:
**	06-oct-92 (robf)
**		Created
*/
bool  
gwf_chk_sess_trace(bitno)
i4 bitno;
{
	DB_STATUS status;
	GW_SESSION  *session;
	/*
	**	Only check if there is tracing
	**	This saves the call to SCF if no tracing is needed
	*/
	if(gwf_tracing_on==FALSE)
		return FALSE;
	/*
	**	Get session id
	*/
	status=gwf_session_id(&session);
	
	if( status!=E_DB_OK)
	{
		return FALSE;
	}
	/*
	**	Check if trace flag is set
	*/
	if(GWF_MACRO(session->gws_trace,bitno) !=0 )
		return TRUE;
	else
		return FALSE;
}
