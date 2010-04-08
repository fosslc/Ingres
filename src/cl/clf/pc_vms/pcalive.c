/*
**    Copyright (c) 1992, 2008 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <pc.h>

#include    <efndef.h>
#include    <iledef.h>
#include    <iosbdef.h>
#include    <jpidef.h>
#include    <ssdef.h>
#include    <lib$routines.h>
#include    <starlet.h>

/*
** Name: PCis_alive	    - Is this process still alive?
**
** Description:
**	This function can be called to determine whether a given process is
**	alive or not.
**
** Inputs:
**	pid		    Process to check
**
** Outputs:
**	none
**
** Returns:
**	TRUE		    Process appears to be alive. We also return TRUE if
**			    we can't tell whether the process is alive because
**			    a system problem keeps us from finding out.
**	FALSE		    Process is definitely NOT alive
**
** History:
**	3-jun-1992 (bryanp)
**	    Created
**	21-jun-1993 (bryanp)
**	    I'm not sure if this is documented anywhere, but sys$getjpiw seems
**	    to do something really weird when 'pid' is 0. If 'pid' is zero,
**	    the system call assumes that you want to know the state of your
**	    own process (as though you didn't already know this?) and does the
**	    following: (1) sets 'pid' to your own process's process ID, and
**	    (2) returns SS$_NORMAL, and (3) fills in the process state of your
**	    own process. This is not useful, since it causes us to return "true"
**	    when called to ask if process 0 is alive, but process 0 is in fact
**	    NOT alive, and does not exist. To fix this, I've added code to
**	    handle process 0 as a special case (ugh!).
**      16-jul-93 (ed)
**	    added gl.h
**	3-jan-1993 (bryanp)
**	    ALPHA-specific: event flag 0 appears to be "dangerous". We don't
**	    really understand why, but using event flag 0 in the sys$getjpiw
**	    call causes intermittent "hangs" in ALPHA DBMS servers. To work
**	    around this, we use a non-zero event flag which we get from
**	    lib$getef.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	28-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
*/
bool
PCis_alive(PID pid)
{
    i4	    process_state;
    i4	    i;
    ILE3  jpiget[] = {
				    {sizeof(i4), JPI$_STATE, 0, 0},
				    { 0, 0, 0, 0}
	             };
    IOSB    iosb;

    if (pid == 0)
	return (FALSE);

    jpiget[0].ile3$ps_bufaddr = (PTR)&process_state;

    i = sys$getjpiw(EFN$C_ENF, &pid, 0, jpiget, &iosb, 0, 0);
    if (i & 1)
	i = iosb.iosb$w_status;
    if (i == SS$_NONEXPR)
	return (FALSE);

    if (( i & 1) == 0)
    {
	/*
	** A few errors indicate that the target process is alive, but for
	** some reason we can't find out about it. For those errors we return
	** TRUE. The error which definitely indicates that the process is
	** not alive is SS$_NONEXPR "The specified process does not exist".
	**
	** For now, we'll not try to itemize the types of errors, but will just
	** return TRUE for all such errors, assuming that the process still is
	** alive, but we just can't find out any information about it.
	*/
	return (TRUE);
    }
    else
    {
	/*
	** If getjpi succeeds, the target process is definitely alive.
	*/
	return (TRUE);
    }
}
