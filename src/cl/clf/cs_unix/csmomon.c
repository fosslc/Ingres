/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <systypes.h>
# include <sp.h>
# include <pc.h>
# include <me.h>
# include <cs.h>
# include <mo.h>

# include <clconfig.h>
# include <fdset.h>
# include <csinternal.h>
# include <csev.h>
# include <cssminfo.h>
# include "cslocal.h"

# include "csmgmt.h"

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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**    11-aug-93 (ed)
**      add missing include
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**      01-sep-1993 (smc)
**              Changed casts of sid to CS_SID.
**      31-dec-1993 (andys)
**	    In CS_debug_set, add third argument to CS_dump_stack even though 
**          code is currently ifdef'd out.
**	19-sep-1995 (nick)
**	    And yet another parameter to CS_dump_stack().
**      18-feb-1997 (hanch04)
**          Merge Ingres-threaded and OS-threaded servers.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


static void CS_str_to_uns(char *a, u_i4 *u);

MO_SET_METHOD CS_breakpoint_set;
MO_SET_METHOD CS_stopcond_set;
MO_SET_METHOD CS_stopserver_set;
MO_SET_METHOD CS_crashserver_set;
MO_SET_METHOD CS_shutserver_set;
MO_SET_METHOD CS_rm_session_set;
MO_SET_METHOD CS_suspend_session_set;
MO_SET_METHOD CS_resume_session_set;
MO_SET_METHOD CS_debug_set;

GLOBALDEF MO_CLASS_DEF CS_mon_classes[] =
{
  { 0, "exp.clf.unix.cs.mon.breakpoint", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_breakpoint_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.stopcond", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_stopcond_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.stopserver", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_stopserver_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.crashserver", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_crashserver_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.shutserver", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_shutserver_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.rm_session", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_rm_session_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.suspend_session", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_suspend_session_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.resume_session", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_resume_session_set,
	0, MOcdata_index },

  { 0, "exp.clf.unix.cs.mon.debug", 0,
	MO_READ|MO_SERVER_WRITE, 0, 0,
	MOzeroget, CS_debug_set,
	0, MOcdata_index },

  { 0 }
};


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
*/

STATUS
CS_mod_session( char *uns_decimal_session, CS_SCB **scbp )
{
    u_i4 scb_as_ulong;
    CS_SID an_sid;
    CS_SCB *an_scb;
    CS_SCB *scan_scb;
    STATUS stat;
    CS_SCB *this_scb;
    
    CSget_scb(&this_scb);

    CS_str_to_uns( uns_decimal_session, &scb_as_ulong );
    an_sid = (CS_SID)scb_as_ulong;
    
    an_scb = CS_find_scb(an_sid);

    if (an_scb == NULL)
    {
	/* FIXME -- real error status */
	/* "Invalid session id" */

	stat = MO_NO_INSTANCE;
    }
    else if ((MEcmp(an_scb->cs_username, this_scb->cs_username,
		    sizeof(an_scb->cs_username))))
    {
	/* FIXME -- better error */
	/* "Superuser or owner status required to modify sessions" */

	stat = MO_NO_WRITE;
    }
    else
    {
	*scbp = an_scb;
	stat = OK;
    }
    return( stat );
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
CS_is_internal( CS_SCB *an_scb )
{
    STATUS stat = OK;

    if ((an_scb == (CS_SCB *)&Cs_idle_scb) || (an_scb == (CS_SCB *)&Cs_admin_scb))
    {
	/* FIXME -- better error */
	/* "Session %x is an internal task and cannot be modified", */

	stat = MO_NO_WRITE;
    }
    return( stat );
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
CS_breakpoint_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    return( CS_breakpoint() );
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
CS_stopcond_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS stat;
    i4  count;

    stat = CSterminate(CS_COND_CLOSE, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    if ( stat )
    {
	/* FIXME ERsend? */
	/* "Error stopping server, %d. sessions remaining", count */
    }
    return( stat );
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
CS_stopserver_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS stat;
    i4  count;
    
    stat = CSterminate(CS_KILL, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    if( stat )
    {
	/* FIXME ERsend? */
	/* "Server will stop. %d. sessions aborted", count */
    }
    return( stat );
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
CS_crashserver_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    i4  count;
    STATUS stat;
    stat = CSterminate(CS_CRASH, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    return( stat );
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
CS_shutserver_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    STATUS stat;
    i4  count;

    stat = CSterminate(CS_CLOSE, &count);
    if( E_CS0003_NOT_QUIESCENT == stat )
	stat = OK;
    if ( stat )
    {
	/* FIXME ERsend? */
	/* "Server will stop. %d. sessions remaining" */
    }
    return( stat );
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
**	    typo - CS_mod_scb should have been CS_mod_session.
*/

STATUS
CS_rm_session_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    CS_SCB *an_scb;
    STATUS stat;
    
    stat = CS_mod_session( sbuf, &an_scb );
    if( stat == OK && (stat = CS_is_internal( an_scb )) == OK )
    {
	CSremove(an_scb->cs_self);
	CSattn(CS_REMOVE_EVENT, an_scb->cs_self);
    }
    return( stat );
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
CS_suspend_session_set( i4  offset, i4  lsbuf, char *sbuf,
		       i4  size, PTR object  )
{
    CS_SCB *an_scb;
    STATUS stat;

    stat = CS_mod_session( sbuf, &an_scb );
    if( stat == OK && (stat = CS_is_internal( an_scb )) == OK )
	an_scb->cs_state = CS_UWAIT;

    return( stat );
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
CS_resume_session_set( i4  offset, i4  lsbuf, char *sbuf,
		      i4  size, PTR object  )
{
    CS_SCB *an_scb;
    STATUS stat;

    stat = CS_mod_session( sbuf, &an_scb );
    if( stat == OK && (stat = CS_is_internal( an_scb )) == OK )
    {
	if (an_scb->cs_state != CS_UWAIT)
	{
	    /* FIXME -- better status */
	    /* "Session %x was not suspended", */
	    stat = FAIL;
	}
	else
	{
	    an_scb->cs_state = CS_COMPUTABLE;
	}
    }
    return( stat );
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
**      31-dec-1993 (andys)
**	    In CS_debug_set, add third argument to CS_dump_stack even though 
**          code is currently ifdef'd out.
**	19-sep-1995 (nick)
**	    Add another parameter to CS_dump_stack(). ( still ifdef'd out )
*/
STATUS
CS_debug_set( i4  offset, i4  lsbuf, char *sbuf, i4  size, PTR object  )
{
    CS_SCB *an_scb;
    STATUS stat;

    stat = CS_mod_session( sbuf, &an_scb );
    if( stat == OK )
    {
	/* FIXME -- what do do here? */
# if 0
	char	buffer[1048];
	CS_fmt_scb(an_scb, sizeof(buffer), buffer);
	CS_dump_stack(an_scb, 0, output_fcn, FALSE);
# endif
    }
    return( stat );
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
CS_str_to_uns(char *a, u_i4 *u)
{
    register u_i4	x; /* holds the integer being formed */
    
    /* skip leading blanks */

    while ( *a == ' ' )
	a++;
    
    /* at this point everything had better be numeric */

    for (x = 0; *a <= '9' && *a >= '0'; a++)
	x = x * 10 + (*a - '0');
    
    *u = x;
}


