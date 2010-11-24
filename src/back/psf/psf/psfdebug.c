/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSFDEBUG.C - Functions for debugging PSF.
**
**  Description:
**      This file contains the standard entry point for debugging PSF.
**
**          psf_debug	 - Set and clear trace flags
**	    psf_relay	 - Relay a message buffer directly to psf_scctrace
**	    psf_scctrace - Output routine called by psf_display, psf_relay.
**
**
**  History:    $Log-for RCS$
**      17-apr-86 (jeff)    
**          written
**	22-feb-88 (stec)
**	    Fix `set printqry' problem.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	29-jun-93 (andre)
**	    #included TR.H
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-oct-93 (rblumer)
**	   In order to allow session trace points that takes a value,
**	   changed ult_set_macro call to use firstval and secondval
**	   instead of hard-coding zeros for the values.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Made psf_display() portable. The old psf_display() took a variable
**	    number of variable-sized arguments. It could not be re-written as
**	    a varargs function as this practice is outlawed in main line code.
**	    The problem was solved by invoking a varargs function, TRformat,
**	    directly.
**	    The psf_display() function has now been deleted, but for flexibilty
**	    psf_display has been #defined to TRformat to accomodate future
**	    change.
**	    All calls to psf_display() changed to pass additional arguments
**	    required by TRformat:
**	        psf_display(psf_scctrace, 0, trbuf, sizeof(trbuf) - 1, ...)
**	    this emulates the behaviour of the old psf_display() except that
**	    the trbuf must be supplied.
**	    Added psf_relay() to cater for an instance of adu_2prvalue() which
**	    requires a function pointer to an output routine.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	10-Jan-2001 (jenjo02)
**	    Replaced SCU_INFORMATION with psf_sesscb() call.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psf_debug(
	DB_DEBUG_CB *debug_cb);
void psf_relay(
	char *msg_buffer);
i4 psf_scctrace(
	PTR arg1,
	i4 msg_length,
	char *msg_buffer);

/*{
** Name: psf_debug	- Standard entry point for debugging PSF.
**
** Description:
**      This function is the standard entry point to PSF for setting and
**	clearing tracepoints.
**
** Inputs:
**      debug_cb                        Pointer to a DB_DEBUG_CB
**        .db_trswitch                    What operation to perform
**	    DB_TR_NOCHANGE		    None
**	    DB_TR_ON			    Turn on a tracepoint
**	    DB_TR_OFF			    Turn off a tracepoint
**	  .db_trace_point		  The number of the tracepoint to be
**					  effected
**	  .db_vals[2]			  Optional values, to be interpreted
**					  differently for each tracepoint
**	  .db_value_count		  The number of values specified in
**					  the above array
**
** Outputs:
**      None
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_WARN			Operation completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-apr-86 (jeff)
**          written
**	13-feb-90 (andre)
**	    set scf_stype to SCU_EXCLUSIVE before calling scu_swait.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	   removed declaration of scf_call()
**	08-oct-93 (rblumer)
**	   In order to allow session trace points that take a value,
**	   changed ult_set_macro call to use firstval and secondval
**	   instead of hard-coding zeros for the values.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**      21-Feb-2007 (hanal04) Bug 117736
**          Added trace point PS503 to dump PSF's ULM memory usage to the
**          DBMS log.
*/
DB_STATUS
psf_debug(
	DB_DEBUG_CB        *debug_cb)
{
    i4                  flag;
    i4		firstval;
    i4		secondval;
    PSS_SESBLK		*sess_cb;
    extern PSF_SERVBLK	*Psf_srvblk;

    /* Get the session control block */
    sess_cb = psf_sesscb();

    /* Flags 0 - PSF_TBITS-1 are for the server; all others are for sessions */
    flag = debug_cb->db_trace_point;
    if (flag >= PSF_TBITS)
    {
	flag = flag - PSF_TBITS;
	if (flag >= PSS_TBITS)
	    return (E_DB_ERROR);
    }

    /* There can be UP TO two values, but maybe they weren't given */
    if (debug_cb->db_value_count > 0)
	firstval = debug_cb->db_vals[0];
    else
	firstval = 0L;

    if (debug_cb->db_value_count > 1)
	secondval = debug_cb->db_vals[1];
    else
	secondval = 0L;

    /*
    ** Three possible actions: Turn on flag, turn it off, or do nothing.
    */
    switch (debug_cb->db_trswitch)
    {
    case DB_TR_ON:
	/* First PSF_TBITS flags belong to server, others to session */
	if (debug_cb->db_trace_point < PSF_TBITS)
	{
	    CSp_semaphore(1, &Psf_srvblk->psf_sem); /* exclusive */
	    ult_set_macro(&Psf_srvblk->psf_trace, flag, firstval, secondval);
	    CSv_semaphore(&Psf_srvblk->psf_sem);
	}
	else
	{
	    /* Do nothing if couln't get session control block */
	    if (sess_cb != (PSS_SESBLK *) NULL)
	    {
		if(flag == PSS_ULM_DUMP_POOL)
		{
		    ULM_RCB	ulm_rcb;
		    char	buf[512];
		    SCF_CB	scf_cb;

		    ulm_rcb.ulm_poolid = Psf_srvblk->psf_poolid;
		    ulm_rcb.ulm_facility = DB_PSF_ID;
		    _VOID_ ulm_mappool(&ulm_rcb);
		    ulm_print_pool(&ulm_rcb);
		    STprintf(buf, "ULM Memory Pool Map and ULM Memory Print Pool for PSF has been \nwritten to the DBMS log file.");
		    
		    scf_cb.scf_length = sizeof(scf_cb);
		    scf_cb.scf_type = SCF_CB_TYPE;
		    scf_cb.scf_facility = DB_PSF_ID;
		    scf_cb.scf_session = DB_NOSESSION;
		    scf_cb.scf_nbr_union.scf_local_error = 0;
		    scf_cb.scf_len_union.scf_blength = STlength(buf);
		    scf_cb.scf_ptr_union.scf_buffer = buf;
		    _VOID_ scf_call(SCC_TRACE, &scf_cb);
		}
		else
		{
		    ult_set_macro(&sess_cb->pss_trace, flag, firstval, 
				secondval);

		    /* Yacc debugging requires a special call */
		    if (flag == PSS_YTRACE)
		        psl_trace((PTR) sess_cb->pss_yacc, TRUE);
		}
	    }
	}
	break;

    case DB_TR_OFF:
	/* First PSF_TBITS flags belong to server, others to session */
	if (debug_cb->db_trace_point < PSF_TBITS)
	{
	    CSp_semaphore(1, &Psf_srvblk->psf_sem); /* exclusive */
	    ult_clear_macro(&Psf_srvblk->psf_trace, flag);
	    CSv_semaphore(&Psf_srvblk->psf_sem);
	}
	else
	{
	    /* Do nothing if couldn't get session control block */
	    if (sess_cb != (PSS_SESBLK *) NULL)
	    {
		ult_clear_macro(&sess_cb->pss_trace, flag);
		/* Yacc debugging requires a special call */
		if (flag == PSS_YTRACE)
		    psl_trace((PTR) sess_cb->pss_yacc, FALSE);
	    }
	}
	break;

    case DB_TR_NOCHANGE:
	/* Do nothing */
	break;

    default:
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: psf_relay	- relay a message buffer to psf_scctrace
**
** Description:
**	Relay a formatted output line to psf_scctrace.
**
** Inputs:
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
**	11-oct-93 (swm)
**	    Bug #56448
**	    Created
[@history_template@]...
*/
VOID
psf_relay(
	char	*msg_buffer)
{
	psf_scctrace((i4)0, (i4)STlength(msg_buffer), msg_buffer);
}

/*{
** Name: psf_scctrace	- send output line to front end
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
**      3-aug-87 (daved)
**          initial creation
**	12-jan-87 (stec)
**	    Added code inserting NL into the buffer.
**	22-feb-88 (stec)
**	    Fix `set printqry' problem. `\n' should not
**	    be preceded by `\0'.
**	21-may-89 (jrb)
**	    renamed scf_enumber to scf_local_error
**	22-may-01 (hayke02)
**	    We now do not add on a newline character ('\n') if we already
**	    have one at the end of the message. This prevents a SIGBUS on
**	    some platforms (i.e. dgi_us5) when we try to add on the nl
**	    to the NULL messages from adu_2prvalue() which do not contain
**	    the extra byte normally needed for the nl. This change fixes
**	    bug 104713.
[@history_template@]...
*/
STATUS
psf_scctrace(
	PTR	    arg1,
	i4	    msg_length,
	char        *msg_buffer)
{
    SCF_CB       scf_cb;
    DB_STATUS    scf_status;
    char	 *p;

    if (msg_length == 0)
	return 0;

    /* Truncate message if necessary */
    if (msg_length > PSF_MAX_TEXT)
	msg_length = PSF_MAX_TEXT;

    /* If last char is null, replace with NL, else add NL */
    p = (msg_buffer + msg_length - 1);
    if (*p != '\n')
    {
	if (*p == '\0')
	    *p = '\n';
	else
	{
	    *(++p) = '\n';
	    msg_length++;
	}
    }

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_PSF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0; /* this is an ingres error number
					** returned to user, use 0 just in case 
					*/
    scf_cb.scf_len_union.scf_blength = (u_i4)msg_length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */

    scf_status = scf_call(SCC_TRACE, &scf_cb);

    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error %d displaying a message to user\n",
	    scf_cb.scf_error.err_code);
    }
    return 0;
}
