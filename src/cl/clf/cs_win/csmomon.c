/*
** Copyright (C) 1987, 2004 Ingres Corporation
** All Rights Reserved.
** This is an unpublished work containing confidential and proprietary
** information of Ingres Corporation.  Use, disclosure, reproduction,
** or transfer of this work without the express written consent of
** Ingres Corporation is prohibited.
*/

# include <compat.h>
# include <sys\types.h>
# include <sp.h>
# include <pc.h>
# include <cs.h>
# include <mo.h>
# include <me.h>
# include <pc.h>

/* # include <clconfig.h> */
# include <csinternal.h>
/* # include <csev.h> */
/* # include <cssminfo.h> */
# include "cslocal.h"

# include "csmgmt.h"

GLOBALREF CS_SCB           Cs_main_scb;

VOID CS_breakpoint();

/**
** Name:	csmomon.c	- active monitor methods.
**
** Description:
**
**	Active parts of iimonitor, as MO methods.
**
**	This file defines:
**
**	CS_mod_session		- may we modify this session?
**	CS_is_internal		- is this a special internal thread?
**	CS_breakpoint_set	- set function for CS_breakpoint
**	CS_stopcond_set		- set function for CS_COND_CLOSE.
**	CS_stopserver_set	- set function for CS_KILL.
**	CS_crashserver_set	- set function for CS_CRASH
**	CS_shutserver_set	- set function for CS_CLOSE
**	CS_rm_session_set	- MO object to remove a session
**	CS_suspend_session_set	- try to stop a session
**	CS_resume_session_set	- try to resume a stopped session
**	CS_debug_set		- set function debugging a thread
**
** History:
**	29-Oct-1992 (daveb)
**	    Documented.
**	11-Jan-1993 (daveb)
**	    Make objects readable with MOzeroget.  Fix typo
**	    for CS_mod_scb -> CS_mod_session.
**	12-Jan-1993 (daveb)
**	    typo in rm_session class def - no index method was given!  SEGV city.
**	    Filter out normal "error" status return from CSterminate.
**      12-Dec-95 (fanra01)
**          Extracted data for DLLs on windows NT.
**      10-Jun-97 (fanra01)
**          Modified CS_mod_session to exhaust all avenues when looking for
**          an scb before returning MO_NO_INSTANCE.
**          In CS_rm_session_set modified calling CSremove and CSattn with the
**          sid and not the pointer to scb.
**          This allows a session to be terminated via ima.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	22-jul-2004 (somsa01)
**	    Removed unnecessary include of erglf.h.
**      20-Jul-2004 (lakvi01)
**              SIR#112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

static void     CS_str_to_uns(char *a, u_i4 * u);

MO_SET_METHOD   CS_breakpoint_set;
MO_SET_METHOD   CS_stopcond_set;
MO_SET_METHOD   CS_stopserver_set;
MO_SET_METHOD   CS_crashserver_set;
MO_SET_METHOD   CS_shutserver_set;
MO_SET_METHOD   CS_rm_session_set;
MO_SET_METHOD   CS_suspend_session_set;
MO_SET_METHOD   CS_resume_session_set;
MO_SET_METHOD   CS_debug_set;


/*{
** Name:	CS_mod_session	\- may we modify this session?
**
** Description:
**	Get an SCB pointer from a string instance, and tell
**	us whether the use executing is powerful enough to
**	do session modifications.  If he is the user owning
**	the sesssion he can, or if he is super-user.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	uns_decimal_session	the instance string
**
** Outputs:
**	scbp			SCB point stuffed with value.
**
** Returns:
**	OK		if user can modify the session
**	MO_NO_INSTANCE	if it's a bogus session id.
**	MO_NO_WRITE	if the user may not modify the session.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**      10-Jun-97 (fanra01)
**          Modified the condition for a scb not found.  Ensure that we try to
**          get an scb using the an_scb as a SID before returning
**          MO_NO_INSTANCE.
*/

STATUS
CS_mod_session(char *uns_decimal_session, CS_SCB ** scbp)
{
	u_i4            scb_as_ulong;
	CS_SCB         *an_scb;
	CS_SCB         *scan_scb;
	STATUS          stat;
	CS_SCB         *this_scb;
        bool           blFound = FALSE;

	CSget_scb(&this_scb);
	CS_str_to_uns(uns_decimal_session, &scb_as_ulong);
	an_scb = (CS_SCB *) scb_as_ulong;

	for (scan_scb = Cs_srv_block.cs_known_list->cs_next;
	     scan_scb && scan_scb != Cs_srv_block.cs_known_list;
	     scan_scb = scan_scb->cs_next) {
		if (scan_scb == an_scb)
                {
                        blFound = TRUE;
			break;
                }
	}

	if ((!blFound) && ((an_scb = CS_find_scb(an_scb)) == 0)) {
		/* FIXME -- real error status */
		/* "Invalid session id" */

		stat = MO_NO_INSTANCE;
	} else if ((MEcmp(an_scb->cs_username, this_scb->cs_username,
			  sizeof(an_scb->cs_username)))) {
		/* FIXME -- better error */
		/* "Superuser or owner status required to modify sessions" */

		stat = MO_NO_WRITE;
	} else {
		*scbp = an_scb;
		stat = OK;
	}
	return (stat);
}



/*{
** Name:	CS_is_internal	\- is this a special internal thread?
**
** Description:
**	Tells if the thread is the idle job or the admin task.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	an_scb		the scb in question
**
** Outputs:
**	none.
**
** Returns:
**	OK		if it isn't
**	MO_NO_WRITE	if it's special.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/
STATUS
CS_is_internal(CS_SCB * an_scb)
{
	STATUS          stat = OK;

	if (an_scb == &Cs_main_scb) {
		/* FIXME -- better error */
		/* "Session %x is an internal task and cannot be modified", */

		stat = MO_NO_WRITE;
	}
	return (stat);
}

/* ---------------------------------------------------------------- */




/*{
** Name:	CS_breakpoint_set	- set function for CS_breakpoint
**
** Description:
**	Calls the CS_breakpoint() function on demand; args ifnored.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored
**
** Outputs:
**	none
**
** Returns:
**	CS_breakpoint return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**
*/

STATUS
CS_breakpoint_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
	CS_breakpoint();
	return (OK);
}





/*{
** Name:	CS_stopcond_set	- set function for CS_breakpoint
**
** Description:
**	Calls CSterminate( CS_COND_CLOSE,... ) function on demand
**	to nicely stop the server. Args ignored.  Should only be
**	executed by a privileged user! (Protect the MO object).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored
**
** Outputs:
**	none
**
** Returns:
**	CS_terminate return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	12-Jan-1993 (daveb)
**	    Filter out normal "error" status return from CSterminate.
*/

STATUS
CS_stopcond_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
	STATUS          stat;
	i4              count;

	stat = CSterminate(CS_COND_CLOSE, &count);
	if (E_CS0003_NOT_QUIESCENT == stat)
		stat = OK;
	if (stat) {
		/* FIXME ERsend? */
		/* "Error stopping server, %d. sessions remaining", count */
	}
	return (stat);
}




/*{
** Name:	CS_stopserver_set	- set function for CS_KILL.
**
** Description:
**	Calls CSterminate( CS_KILL, ... ) on demand to stop a server
**	quickly.  Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	12-Jan-1993 (daveb)
**	    Filter out normal "error" status return from CSterminate.
*/

STATUS
CS_stopserver_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
	STATUS          stat;
	i4              count;

	stat = CSterminate(CS_KILL, &count);
	if (E_CS0003_NOT_QUIESCENT == stat)
		stat = OK;
	if (stat) {
		/* FIXME ERsend? */
		/* "Server will stop. %d. sessions aborted", count */
	}
	return (stat);
}




/*{
** Name:	CS_crashserver_set	- set function for CS_CRASH
**
** Description:
**	Calls CSterminate( CS_CRASH, ...) on demand  to stop the
**	server dead in it's tracks.  Args ignored.  Should only
**	be called by a powerful user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	12-Jan-1993 (daveb)
**	    Filter out normal "error" status return from CSterminate.
*/

STATUS
CS_crashserver_set(i4 offset, i4 lsbuf, char *sbuf, i4 size, PTR object)
{
	i4              count;
	STATUS          stat;
	stat = CSterminate(CS_CRASH, &count);
	if (E_CS0003_NOT_QUIESCENT == stat)
		stat = OK;
	return (stat);
}




/*{
** Name:	CS_shutserver_set	- set function for CS_CLOSE
**
** Description:
**	Calls CSterminate( CS_CLOSE, ...) function on demand to stop the
**	server nicely.  Args ignored.  Should only be called by a powerful
**	user.  Protect the MO object!
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		ignored
**
** Outputs:
**	none
**
** Returns:
**	CSterminate error return value.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	12-Jan-1993 (daveb)
**	    Filter out normal "error" status return from CSterminate.
*/

STATUS
CS_shutserver_set(i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
	STATUS          stat;
	i4              count;

	stat = CSterminate(CS_CLOSE, &count);
	if (E_CS0003_NOT_QUIESCENT == stat)
		stat = OK;
	if (stat) {
		/* FIXME ERsend? */
		/* "Server will stop. %d. sessions remaining" */
	}
	return (stat);
}





/*{
** Name:	CSrm_session_set	- MO object to remove a session
**
** Description:
**	Calls the CSremove for a session on demand.  The object
**	is the SCB of the session, other args are ignored.   This
**	enforces the "super user or owner" permissions.  The MO
**	object may be unprotected.
**
**	Removing sessions is dangerous, and may crash the server.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		the SCB in question
**
** Outputs:
**	none
**
** Returns:
**	OK		tried to remove it.
**	MO_NO_INSTANCE	bogus SCB
**	MO_NO_WRITE	no permission to remove it.
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
**	11-Jan-1993 (daveb)
**	    typo - CS_mod_scb should have bee CS_mod_session.
**      10-Jun-97 (fanra01)
**          Modified calling CSremove and CSattn with the sid and not the
**          pointer to scb.
*/

STATUS
CS_rm_session_set(i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
	CS_SCB         *an_scb;
	STATUS          stat;

	stat = CS_mod_session(sbuf, &an_scb);
	if (stat == OK && (stat = CS_is_internal(an_scb)) == OK) {
		CSremove((i4 ) an_scb->cs_self);
		CSattn(CS_REMOVE_EVENT, (i4 ) an_scb->cs_self);
	}
	return (stat);
}



/*{
** Name:	CS_suspend_session_set - try to stop a session
**
** Description:
**	MO function to try to stop a session.  Enforces
**	the "owner or superuser" permissions, and doesn't let
**	you stop the idle or admin threads.  All other threads
**	may be stopped, even "critical" higher level threads.
**
**	Stopping threads is dangerous and may hang the server.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		the SCB in question
**
** Outputs:
**	none
**
** Returns:
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
CS_suspend_session_set(i4  offset, i4  lsbuf, char *sbuf,
		       i4  size, PTR object)
{
	CS_SCB         *an_scb;
	STATUS          stat;

	stat = CS_mod_session(sbuf, &an_scb);
	if (stat == OK && (stat = CS_is_internal(an_scb)) == OK)
		an_scb->cs_state = CS_UWAIT;

	return (stat);
}




/*{
** Name:	CS_resume_session_set	- try to resume a stopped session
**
** Description:
**	Tries to resume a session stopped by a previous
**	CS_suspend_session_set (or iimonitor) call.  May
**	only be done by the owner of the thread or a "powerful"
**	user.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		the SCB in question
**
** Outputs:
**	none
**
** Returns:
**	OK		if tried to resume it.
**	MO_NO_INSTANCE	if bogus SCB.
**	MO_NO_WRITE	if you don't have permission.
**	FAIL		if it wasn't suspended
**
** History:
**	28-Oct-1992 (daveb)
**	    documented
*/

STATUS
CS_resume_session_set(i4  offset, i4  lsbuf, char *sbuf,
		      i4  size, PTR object)
{
	CS_SCB         *an_scb;
	STATUS          stat;

	stat = CS_mod_session(sbuf, &an_scb);
	if (stat == OK && (stat = CS_is_internal(an_scb)) == OK) {
		if (an_scb->cs_state != CS_UWAIT) {
			/* FIXME -- better status */
			/* "Session %x was not suspended", */
			stat = FAIL;
		} else {
			an_scb->cs_state = CS_COMPUTABLE;
		}
	}
	return (stat);
}




/*{
** Name:	CS_debug_set	- set function debugging a thread
**
** Description:
**	Dumps debug information about the thread.  May only
**	be called by the owner or a super user.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	lsbuf		ignored
**	sbuf		ignored
**	size		ignored
**	object		the SCB in question
**
** Outputs:
**	none
**
** Side Effects:
**	Debug information about the thread is dumped
**	to the server log.
**
** Returns:
**	OK		dump attempted.
**	MO_NO_INSTANCE	bogus SCB
**	MO_NO_WRITE	no permission to dump it.
**
** History:
**	28-Oct-1992 (daveb)
**	    documentd.
*/
STATUS
CS_debug_set(i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object)
{
	CS_SCB         *an_scb;
	STATUS          stat;

	stat = CS_mod_session(sbuf, &an_scb);
	if (stat == OK) {
		/* FIXME -- what do do here? */
# if 0
		char            buffer[1048];
		CS_fmt_scb(an_scb, sizeof(buffer), buffer);
		CS_dump_stack(an_scb, output_fcn);
# endif
	}
	return (stat);
}


/*{
** Name: CS_str_to_uns	- string to unsigned conversion.
**
** Description:
**	Convert a string to an unsigned long.  Curiously, we don't
**	have one of these in the CL.  No overflow checking is done.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	a	string to convert
**
** Outputs:
**	u	pointer to an unsigned i4 to stuff.
**
** Returns:
**	none
**
** History:
**	24-sep-92 (daveb)
**	    documented.
*/

static void
CS_str_to_uns(char *a, u_i4 * u)
{
	register u_i4 x;	/* holds the integer being formed */

	/* skip leading blanks */

	while (*a == ' ')
		a++;

	/* at this point everything had better be numeric */

	for (x = 0; *a <= '9' && *a >= '0'; a++)
		x = x * 10 + (*a - '0');

	*u = x;
}
