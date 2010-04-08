/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
**
**  Name: moset.c -- MOset operation for MO.
**
**  Description:
**
**	This module provides a symbol table that can be used by CL and
**	mainline clients to register objects of varying types as management
**	objects.  These objects can be get/set by classid, and may be
**	scanned.
**
**  Intended Uses:
**
**	This is intended to be used by all server code that wishes to
**	export management/monitoring objects.
**
**  Interface:
**
**    The file defines the following visible functions.
**
**	MOset		set value of an object.
**
**  History:
**	26-Oct-91 (daveb)
**		Created from old mi.c
**	15-Jan-92 (daveb)
**		Updated to MO 0.9.
**	22-jan-92 (daveb)
**		Updated to MO.10
**	29-Jan-92 (daveb)
**		broke out of mogetset.c
**	31-Jan-92 (daveb)
**		really methodize it.
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Apr-04 (gordy)
**	    Permit access if MO_ANY_WRITE attribute is set.
*/


/*{
**  Name:	MOset - set value associated with an instance
**
**  Description:
**
**	Attempts to set the value associated with the instance.
**
**	If the call succeeds, it must also call the monitors for the
**	class with the MO_SET event, the instance id, and the new value.
**  Inputs:
**	 valid_perms
**		the roles currently in effect, to be and-ed with object
**		permissions to determine access rights.
**	 class
**		the class whose value should be altered.
**	 instance
**		the instance in question.
**	 val
**		pointer to the string containing the new value.
**  Outputs:
**	 none
**  Returns:
**	 OK
**		if the value was set
**	 MO_NO_INSTANCE
**		if the class is not attached
**	 MO_NO_WRITE
**		if the class may not be changed.
**	 other
**		error status returned by a method.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	 6-Apr-04 (gordy)
**	    Permit access if MO_ANY_WRITE attribute is set.
**	24-Oct-05 (toumi01) BUG 115449
**	    Don't hold onto MO mutex if class' processing will try
**	    to take the mutex, and would thus deadlock on ourself.
*/
STATUS
MOset(i4 valperms,
      char *classid,
      char *instance,
      char *val )
{
    STATUS stat = OK;
    MO_CLASS *cp;
    PTR idata;

    MO_once();
    stat = MO_mutex();
    if( stat != OK )
	return( stat );
    MO_nset++;

    /* get class and instance data */

    if( OK == (stat = MO_getclass( classid, &cp ) ) )
    {
	if( (cp->perms & MO_ANY_WRITE) != 0  ||  
	    (valperms & MO_WRITE & cp->perms) != 0 )
	    stat = (*cp->idx)( MO_GET, cp->cdata, 0, instance, &idata );
	else
	    stat = MO_NO_WRITE;
    }

    if( stat == OK )
    {
	if (cp->cflags & MO_NO_MUTEX)
	    (VOID) MO_unmutex();
	stat = (*cp->set)( cp->offset, 0, val, cp->size, idata );
	if (cp->cflags & MO_NO_MUTEX)
	{
	    stat = MO_mutex();
	    if( stat != OK )
		return( stat );
	}
    }

    if( stat == OK )
	(VOID) MO_tell_class( cp, instance, val, MO_SET );

    (VOID) MO_unmutex();
    return( stat );
}

