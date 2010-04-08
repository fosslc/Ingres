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
**  Name: moget.c -- get operation for MO.
**
**  Description:
**
**	This module provides a symbol table that can be used by cl and
**	mainline clients to register objects of varying types as management
**	objects.  These objects can be get/set by classid, and may be
**	scanned.
**
**  Intended uses:
**
**	this is intended to be used by all server code that wishes to
**	export management/monitoring objects.
**
**  Interface:
**
**    The file defines the following visible functions.
**
**	MOget		get data from the object.
**	MO_get		get data from the object, MO mutex already held.
**	
**  History:
**	26-oct-91 (daveb)
**		created from old mi.c
**	15-jan-92 (daveb)
**		updated to mo 0.9.
**	22-jan-92 (daveb)
**		updated to mo.10
**	29-jan-92 (daveb)
**		broke out of mogetset.c
**	31-Jan-92 (daveb)
**		methodize heavily.
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Apr-04 (gordy)
**	    Allow access if MO_ANY_READ attribute is set.
**	    Fixed error return.
*/


/*{
**  Name:	MOget - get data associated with an instance.
**
**  Description:
**
**	This public interface locks the MO mutex before doing anything.
**
**	Return the string value of the the requested instance.  Returns
**	MO_NO_INSTANCE if the object doesn't exist.  If the object
**	exists but is not readable, returns MO_NO_READ.
**
**	Existance of the object is determined by calling the index
**	method for the class with the MO_GET method.  If this returns
**	OK, then the instance value it provided is passed to the get
**	method for the class, which fills in the buffer supplied to
**	MOget.
**	
**	If the call succeeds, it must also call the monitor for the
**	class with the MO_GET event, the instance id, and the current
**	value.
**  Inputs:
**	 valid_perms
**		the roles currently in effect, to be and-ed with object
**		permissions to determine access rights.
**	 classid
**		the classid in question.
**	 instance
**		the instance in question.
**	 lsbufp
**		pointer to length of the string buffer handed in, to be used
**		for output if needed.
**  Outputs:
**	 lsbufp
**		filled in with the length of the string written to sbuf.
**	 sbuf
**		contains the retrieved string value.
**	 perms
**		the permissions for the classid.
**  Returns:
**	 OK
**		if the retrieval returned data.
**	 MO_NO_INSTANCE
**		if class is not attached, or if the object permissions are
**		non-zero but do not include the valid_perms bits.
**	 MO_NO_READ
**		if the class doesn't have any read permissions.
**	 MO_VALUE_TRUNCATED
**		if the value wouldn't fit in the provided sbuf.
**	 "other error status"
**		returned by a method.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOget(i4 valperms,
      char *classid,
      char *instance,
      i4  *lsbufp,
      char *sbuf,
      i4  *operms )
{
    STATUS stat;
    MO_once();
    stat = MO_mutex();
    if( stat == OK )
    {
	stat = MO_get( valperms, classid, instance, lsbufp, sbuf, operms );
	(VOID) MO_unmutex();
    }
    return( stat );
}



/*{
**  Name:	MO_get - get data associated with an instance.
**
**  Description:
**
**	This internal interface assumes the MO mutex is held.
**
**	Return the string value of the the requested instance.  Returns
**	MO_NO_INSTANCE if the object doesn't exist.  If the object
**	exists but is not readable, returns MO_NO_READ.
**
**	Existance of the object is determined by calling the index
**	method for the class with the MO_GET method.  If this returns
**	OK, then the instance value it provided is passed to the get
**	method for the class, which fills in the buffer supplied to
**	MOget.
**	
**	If the call succeeds, it must also call the monitor for the
**	class with the MO_GET event, the instance id, and the current
**	value.
**  Inputs:
**	 valid_perms
**		the roles currently in effect, to be and-ed with object
**		permissions to determine access rights.
**	 classid
**		the classid in question.
**	 instance
**		the instance in question.
**	 lsbufp
**		pointer to length of the string buffer handed in, to be
**		used for output if needed.
**  Outputs:
**	 lsbufp
**		filled in with the length of the string written to sbuf.
**	 sbuf
**		contains the retrieved string value.
**	 perms
**		the permissions for the classid.
**  Returns:
**	 OK
**		if the retrieval returned data.
**	 MO_NO_INSTANCE
**		if class is not attached, or if the object permissions are
**		non-zero but do not include the valid_perms bits.
**	 MO_NO_READ
**		if the class doesn't have any read permissions.
**	 MO_VALUE_TRUNCATED
**		if the value wouldn't fit in the provided sbuf.
**	 "other error status"
**		returned by a method.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	 6-Apr-04 (gordy)
**	    Allow access if MO_ANY_READ attribute is set.
**	    Fixed error return.
*/

STATUS
MO_get(i4 valperms,
      char *classid,
      char *instance,
      i4  *lsbufp,
      char *sbuf,
      i4  *operms )
{
    STATUS stat;
    MO_CLASS *cp;
    PTR idata;

    MO_nget++;

    stat = OK;
    *sbuf = EOS;

    /* get class and instance data */

    if( OK == (stat = MO_getclass( classid, &cp ) ) )
    {
	if( (cp->perms & MO_ANY_READ) != 0  ||
	    (valperms & MO_READ & cp->perms) != 0 )
	    stat = (*cp->idx)( MO_GET, cp->cdata, 0, instance, &idata );
	else
	    stat = MO_NO_READ;
    }

    if( stat == OK )
       stat = (*cp->get)( cp->offset, cp->size, idata, *lsbufp, sbuf  );

    if( stat == OK || stat == MO_VALUE_TRUNCATED )
    {
	*operms = cp->perms;
        (VOID) MO_tell_class( cp, instance, sbuf, MO_GET );
    }
    return( stat );
}


