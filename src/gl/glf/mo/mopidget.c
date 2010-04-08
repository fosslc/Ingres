/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <sp.h>
# include <mo.h>
# include <pc.h>

/**
** Name:	mopidget.c	- MOpidget
**
** Description:
**	This file defines:
**
**	MOpidget - standard get method for the process ID of the calling
**		   process
**
** History:
**	09-Sep-96 (johna)
**	    Created, copied from daveb's scs_pid_get, as many different
**	    callers need access to this routine.
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-Feb-2010 (smeke01) b123226, b113797 
**	    MOlongout/MOulongout now take i8/u_i8 parameter.
**/


/*{
**  Name:	MOpidget - standard get method for the process ID of 
**		the calling process
**
**  Description:
**
**	Return the PID of the calling process.
**
**  Inputs:
**      offset          ignored.
**      objsize         ignored
**      object          ignored.
**      luserbuf        length of userbuf.
**  Outputs:
**	 userbuf	filled with the PID
**		
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**  History:
**      09-Sep-96 (johna)
**          Created, copied from daveb's scs_pid_get, as many different
**          callers need access to this routine.
*/

STATUS 
MOpidget( i4  offset, i4  objsize, PTR object, i4  luserbuf, char *userbuf )
{
	PID pid;

	PCpid( &pid );

	return( MOulongout( MO_VALUE_TRUNCATED,
		(u_i8)pid, luserbuf, userbuf ) );
}

