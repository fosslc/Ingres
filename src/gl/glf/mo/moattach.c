/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <st.h>
# include <cm.h>
# include <si.h>
# include <sp.h>
# include <tr.h>
# include <mo.h>
# include "moint.h"

/**
** Name:	moattach.c	MOattach (instance) related functions.
**
** Description:
**
**	This file defines:
**
**	MO_getinstance	- get instance of a classid, instance.
**	MOattach	- attach instances
**	MOdetach	- detach an attached instance
**	MOon_off	- enable/disable MO attach (for client programs)
**	MO_ipindex	- index method for attached instances.
**	MOidata_index	- standard index method for data of attached objects
**	MOname_index	- standard index for names of attached instances
**
** History:
**
**	15-Jan-92 (daveb)
**		Upgraded to MO.9 spec.
**	27-jan-92 (daveb)
**		upgreaded to mo.10; vastly different here, as there
**		are two trees instead of one.
**	28-Jan-92 (daveb)
**	    Null out node links in SPenq rather than here.
**	31-Jan-92 (daveb)
**	    Now we don't touch the classes.
**	15-jul-92 (daveb)
**	    Go back to single-instance attach/detach, as with
**	    MOcdata_index most bulky static one-offs will be
**	    right in the class definitions anyway, and this
**	    is simpler for the dynamic attach.
**	24-sep-92 (daveb)
**	    documented
**	13-Jan-1993 (daveb)
**	    Attach twin, if any is defined.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	4-Aug-1993 (daveb) 58937
**	    cast PTRs before use.
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p in MOattach().
**	27-jun-95 (emmag)
**	    Cleaned up the MCT changes which weren't ifdef'ed.
**	22-apr-1996 (lawst01)
**	   Windows 3.1 port changes - set BOOL variable correctly.
**	03-jun-1996 (canor01)
**	    Previous change was generic. MOmutex should be held for all
**	    calls to MO_tell_class().
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	02-sep-1999 (somsa01)
**	    In MOdetach(), we were calling MO_tell_class() without holding
**	    the MO mutex.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name:	MO_getinstance	- get instance of a classid, instance.
**
** Description:
**
**	General lookup routine for instance.  If instance is NULL,
**	return first instance of that class.
**
** Re-entrancy:
**	yes, if MO mutex is held.
**
** Inputs:
**	classid		string of classid,
**	instance	string of instance, or NULL.
**
** Outputs:
**	none.
**
** Returns:
**	MO_INSTANCE if found, or NULL.
**
**  History:
**	24-sep-92 (daveb)
**	    documented
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

MO_INSTANCE *
MO_getinstance( char *classid, char *instance )
{
    MO_CLASS	cb;
    MO_INSTANCE	ib;
    MO_INSTANCE	*ip;

    cb.node.key = classid;
    ib.classdef = &cb;
    ib.instance = instance;
    ib.node.key = (PTR)&ib;

    if( instance != NULL )	/* find exact match */
    {
	ip = (MO_INSTANCE *)SPlookup( &ib.node, MO_instances );
    }
    else			/* find first of that class */
    {
	/*
	** This relies on the comparison function returning -1
	** when classids are equal, and the input instance is NULL.
	** After the enq, ib is the lowest block of the class; the
	** next one is the old first.
	*/
	(VOID) SPenq( &ib.node, MO_instances );
	ip = (MO_INSTANCE *)SPnext( &ib.node, MO_instances );
	SPdelete( &ib.node, MO_instances );

	if( ip != NULL && !STequal( (char *)ip->classdef->node.key, classid ) )
	    ip = NULL;
    }

# ifdef xDEBUG
    SIprintf("getinstance %s:%s -> %s:%s\n",
	     classid ? classid : "<nil>",
	     instance ? instance : "<nil>",
	     ip ? ip->classdef->node.key : "<nil>",
	     ip ? ip->instance : "<nil>" );
# endif

    return( ip );
}


/*{
**  Name:	MOattach - attach instances
**
**  Description:
**
**	If MOon_off has been called with MO_DISABLE, returns OK without
**	doing anything.
**
**	Attachs an instance of a classid.  If a classid has a
**	corresponding object-id (as determined by looking up the classid
**	in the MO_META_OBJID class), it is attached under the oid as
**	well.
**
**	If the flags field contains MO_INSTANCE_VAR, then the instance
**	string will be saved in allocated space, otherwise only a
**	pointer will be kept.  The idata may be passed by the
**	MOidata_index method to the get and set methods for the class.
**	This pointer will usually identify the actual object in
**	question.
**
**	Allocates memory for the object using MEreqmem.
**
**	If the call succeeds, it must also call the monitors for the
**	class with the MO_ATTACH event, the instance id, and a NULL
**	value.
**
**  Inputs:
**	 flags
**		either 0 or MO_INSTANCE_VAR if the instance string is not in
**		stable storage.
**	 classid
**		the classid of this class.
**	 instance
**		being attached.
**	 idata
**		the value to associate with the instance, picked out by the
**		MOidata_index method and handed to get and set methods.
**  Returns:
**	 OK
**		is the object was attached successfully.
**	 MO_NO_CLASSID
**		if the classid is not defined.
**	 MO_ALREADY_ATTACHED
**		the object has already been attached.
**	 MO_NO_MEMORY
**		if the MO_MEM_LIMIT would be exceeded.
**	 other
**		system specific error status, possibly indicating allocation
**		failure.
**
**    History:
**	15-jul-92 (daveb)
**	    Go back to single-instance attach/detach, as with
**	    MOcdata_index most bulky static one-offs will be
**	    right in the class definitions anyway, and this
**	    is simpler for the dynamic attach.
**	13-Jan-1993 (daveb)
**	    Attach twin, if any is defined.
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p.
**	13-Apr-2010 (bonro01) Bug 123570
**	    A hang can occur if MOattach calls MO_alloc which needs to
**	    call MEget_pages to allocate more pages. This results in a
**	    call to MOclassdef which also locks the MO_mutex.
**	    To eliminate this problem the MO_mutex is released before calling
**	    MO_alloc and re-acquired afterwards.
*/

STATUS
MOattach( i4  flags, char *classid, char *instance, PTR idata )
{
    MO_INSTANCE	*ip;
    MO_CLASS	*cp;
    STATUS	stat;

    stat = OK;

    if( MO_disabled )
	return( stat );

    MO_once();
    stat = MO_mutex();
    if( stat == OK )		/* got mutex */
    {
	do
	{
	    if( stat != OK )
		break;

	    MO_nattach++;

# ifdef xDEBUG
	    SIprintf("attach %s:%s object %p\n",
		     classid,
		     instance,
		     idata );
# endif

	    /* check for special error cases */

	    ip = MO_getinstance( classid, instance );
	    if( ip != NULL )
	    {
		stat = MO_ALREADY_ATTACHED;
		break;
	    }

	    /* Get class, error if missing */

	    if( OK != MO_getclass( classid, &cp ) )
	    {
		stat = MO_NO_CLASSID;
		break;
	    }

	    /* Release mutex to prevent deadlock in MO_alloc call */
	    (VOID) MO_unmutex();
	    ip = (MO_INSTANCE *)MO_alloc( sizeof(*ip), &stat );
            stat = MO_mutex();
	    if( NULL == ip )
		break;

	    ip->classdef = cp;
	    ip->iflags = flags;
	    if( (flags & MO_INSTANCE_VAR) == 0 )
	    {
		ip->instance = instance;
	    }
	    else
	    {
		ip->instance = MO_defstring( instance, &stat );
		if( ip->instance == NULL )
		{
		    MO_free( (PTR)ip, sizeof(*ip) );
		    break;
		}
	    }
	    ip->idata = idata;
	    ip->node.key = (PTR)ip;

	    (VOID) SPenq( &ip->node, MO_instances );

	} while( FALSE );

	(VOID) MO_unmutex();

	/* Attach twin if needed */

	if( stat == OK && cp->twin != NULL && !CMdigit( (char *)cp->node.key ) )
	    stat = MOattach( flags, cp->twin->node.key, instance, idata );
    }

# if xDEBUG
    if( stat != OK )
	TRdisplay("attach (%s:%s) failed status %d\n",
		  classid, instance, stat );
# endif

    if( stat == OK )
    {
	if ( (stat = MO_mutex()) == OK)
	{
	    (VOID) MO_tell_class( cp, instance, (char *)NULL, MO_ATTACH );
            MO_unmutex ();
	}
    }

    return( stat );
}


/*{
**  Name:	MOdetach - detach an attached instance
**
**  Description:
**
**	Detaches an attached instance.  Subsequent attempts to get or
**	set it will fail, and an attempts to attach it will succeed.
**
**	Frees memory allocated by MOattach using MEfree.
**
**	If the call succeeds, it must also call the monitors for the
**	class with the MO_DETACH event, the instance id, and a NULL
**	value.
**
**  Inputs:
**	 classid
**		the classid of the object.
**	 instance
**		the instance of the object.
**  Outputs:
**	 none
**  Returns:
**	 OK
**		if the classid was detached.
**	 MO_NO_INSTANCE
**		if classid is not attached.
**	 MO_NO_DETACH
**		if the object was attached as MO_PERMANENT.
**
**    History:
**	15-jul-92 (daveb)
**	    Go back to single-instance attach/detach, as with
**	    MOcdata_index most bulky static one-offs will be
**	    right in the class definitions anyway, and this
**	    is simpler for the dynamic attach.
**	02-sep-1999 (somsa01)
**	    We were calling MO_tell_class AFTER releasing the MO mutex.
*/
STATUS
MOdetach( char *classid, char *instance )
{
    MO_INSTANCE	*ip;
    MO_CLASS	*cp;
    STATUS stat = OK;
    STATUS mutex_stat = OK;

    MO_once();
    mutex_stat = MO_mutex();
    do
    {
	if( mutex_stat != OK )		/* got mutex */
	    break;

	MO_ndetach++;

	ip = MO_getinstance( classid, instance );
	if( NULL == ip )
	{
	    stat = MO_NO_INSTANCE;
	    break;
	}

	cp = ip->classdef;
	if( cp->perms & MO_PERMANENT )
	{
	    stat = MO_NO_DETACH;
	    break;
	}

	/*
	** We have detachable instance, do it
	*/

	/* detach twin, if any, giving up it it won't go away */

	if( CMalpha( classid ) && NULL != cp->twin &&
	   (stat = MOdetach( cp->twin->node.key, ip->instance)) != OK )
	    break;

	/* delete saved instance string */

	if( (ip->iflags & MO_INSTANCE_VAR) != 0 )
	    MO_delstring( ip->instance );

	/* and finally delete this node */

	SPdelete( &ip->node, MO_instances );
	MO_free( (PTR)ip, sizeof(*ip) );

    } while( FALSE );

    if( mutex_stat == OK )
	(VOID) MO_unmutex();
    else
	stat = mutex_stat;

    if( stat == OK )
    {
	if ( (stat = MO_mutex()) == OK)
	{
	    (VOID) MO_tell_class( cp, instance, (char *)NULL, MO_DETACH );
            MO_unmutex ();
	}
    }

    return( stat );
}


/*{
**  Name:	MOon_off - enable/disable MO attach (for client programs)
**
**  Description:
**
**
**	Turns MOattach off while having calls to it return OK.  This is
**	intended to be used by client programs that do not want the
**	expense of MO operations, but which are constructed from
**	libraries that would otherwise define MO objects.  All requests
**	return the old state; MO_INQUIRE returns the old state without
**	changing it.
**
**	By default, the state is enabled.
**
**	Queries with MOget and MOgetnext on objects attached when MO was
**	enabled will always work.
**
**  Inputs:
**	 operation
**		one of MO_ENABLE. MO_DISABLE, or MO_INQUIRE.
**  Outputs:
**	 old_state
**		the previous state, either MO_ENABLED or MO_DISABLED.
**  Returns:
**	 OK
**		or other error status.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOon_off( i4  operation, i4  *old_state )
{
    STATUS stat = OK;

    stat = MO_mutex();
    if( stat != OK )
	return( stat );

    *old_state = MO_disabled;
    if( operation != MO_INQUIRE )
	MO_disabled = (operation) ? TRUE : FALSE;

    (VOID) MO_unmutex();

    return( stat );
}



/*{
** Name:	MO_ipindex	- index method for attached instances.
**
** Description:
**
**	An index method for looking into the attached instance tree.
**
**	Next locates the next instance of the same class, and does
**	not march into subsequent classes.
**
** Re-entrancy:
**	yes, if MO mutex is held.
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT.
**	cdata		the classid string.
**	linstance	length of instance buffer.
**	instance	the instance string
**
** Outputs:
**	instance	filled in with new instance on getnext.
**	instdata	stuffed with saved instance data.
**
** Returns:
**	OK
**	MO_BAD_MSG
**	MO_NO_INSTANCE
**	MO_NO_NEXT
**	MO_INSTANCE_TRUNCATED
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

STATUS
MO_ipindex(i4 msg,
	   PTR cdata,
	   i4  linstance,
	   char *instance,
	   PTR *instdata )
{
    STATUS stat = MO_NO_INSTANCE;
    MO_INSTANCE *ip;

    ip = MO_getinstance( (char *)cdata, instance );

    switch( msg )
    {
    case MO_GET:

	if( ip != NULL )
	    stat = OK;
	break;

    case MO_GETNEXT:

	if( ip != NULL )
	{
	    ip = (MO_INSTANCE *) SPnext( &ip->node, MO_instances );
	    if( ip != NULL &&
	       !STequal( (char *)ip->classdef->node.key, (char *)cdata ) )
		ip = NULL;
	}
	else			/* didn't find directly */
	{
	    if( *instance == EOS  )
		ip = MO_getinstance( (char *)cdata, (char *)NULL );
	    else
		break;
	}

	if( ip == NULL )
	    stat = MO_NO_NEXT;
	else
	    stat = MOstrout( MO_INSTANCE_TRUNCATED,
			     ip->instance, linstance, instance );
	break;

    default:

	stat = MO_BAD_MSG;
	break;
    }

    *instdata = (PTR)ip;

    return( stat );
}


/*{
**  Name: MOidata_index - standard index method for data of attached objects
**
**  Description:
**
**	Index for instance data belonging to objects known through
**	MOattach calls, for handing the user object pointer to the
**	get/set/method.  This is the normally used index method cdata of
**	a classid, from
**
**	The input cdata is assumed to be a pointer to a string holding
**	the classid in question, usually by defining the class with
**	MO_CDATA_CLASSID or MO_CDATA_INDEX.  If msg is MO_GET, determine
**	whether the requested instance exists.  If it does, fill in
**	instdata with the idata pointer provided in the attach call for
**	use by a get or set method, and return OK.  If the requested
**	instance does not exist, return MO_NO_INSTANCE.
**
**	If the msg is MO_GETNEXT, see if the requested instance exists;
**	if not return MO_NO_INSTANCE.  Then locate the next instance in
**	the class.  If there is no successor instance in the class,
**	return MO_NO_NEXT.  If there is a sucessor, replace the input
**	instance with the one found.  Fill in instdata with the idata
**	supplied when the instance was attached, for use by the get/set
**	methods defined for the class.  If the new instance won't fit in
**	the given buffer (as determined by linstance ), return
**	MO_INSTANCE_TRUNCATED, otherwise return OK.
**
**  Inputs:
**	 msg
**		either MO_GET or MO_GETNEXT
**	 cdata
**		class specific data from the class definition, assumed to be a
**		pointer to the classid string.
**	 linstance
**		the length of the instance buffer
**	 instance
**		the instance in question.
**	 instdata
**		a pointer to a place to put instance-specific data.
**
**  Outputs:
**	 instdata
**		filled in with appropriate idata for the attached instance.
**  Returns:
**	 OK
**		if the operation succeeded.
**	 MO_NO_INSTANCE
**		if the requested instance does not exist.
**	 MO_NO_NEXT
**		if there is no instance following the specified one in the
**		class.
**	 MO_INSTANCE_TRUNCATED
**		if the output instance of a GET_NEXT would not fit in the
**		provided buffer.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOidata_index(i4 msg,
	      PTR cdata,
	      i4  linstance,
	      char *instance,
	      PTR *instdata )
{
    MO_INSTANCE *ip;
    PTR myidata;
    STATUS stat;

    stat = MO_ipindex( msg, cdata, linstance, instance, &myidata );
    ip = (MO_INSTANCE *)myidata;

    if( stat == OK || stat == MO_INSTANCE_TRUNCATED )
	*instdata = ip->idata;

    return( stat );
}


/*{
**  Name:	MOname_index - standard index for names of attached instances
**
**  Description:
**
**	Method for producing ``index'' objects whose value is the
**	instance.  This is almost like MOidata_index, except that the
**	instdata is filled with a pointer to the instance string (for
**	use with MOstrget) instead of the idata associated with the
**	instance.
**
**	The cdata is assumed to point to the classid string.  If msg is
**	MO_GET, determine whether the requested instance exists.  If it
**	does, fill in instdata with a pointer to the instance string and
**	return OK.  If the requested instance does not exist, return
**	MO_NO_INSTANCE.
**
**	If the msg is MO_GETNEXT, see if the requested instance exists;
**	if not return MO_NO_INSTANCE.  Then locate the next instance in
**	the class.  If there is no successor instance in the class,
**	return MO_NO_NEXT.  If there is a sucessor, replace the input
**	instance with the one found.  Fill in instdata with a pointer to
**	the instance string.  If the new instance won't fit in the given
**	buffer (as determined by linstance ), return
**	MO_INSTANCE_TRUNCATED, otherwise return OK.
**
**  Inputs:
**	 msg
**		either MO_GET or MO_GETNEXT
**	 cdata
**		class specific data pointing to the class string.
**	 linstance
**		the length of the instance buffer
**	 instance
**		the instance in question
**	 instdata
**		place to put a pointer to the instance string.
**  Outputs:
**	 instdata
**		filled in with a pointer to the instance string.
**  Returns:
**	 OK
**		if the operation succeeded.
**	 MO_NO_INSTANCE
**		if the requested instance does not exist.
**	 MO_NO_NEXT
**		if there is no instance following the specified one in the
**		class.
**	 MO_INSTANCE_TRUNCATED
**		if the output instance of a GET_NEXT would not fit in the
**		provided buffer.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOname_index(i4 msg,
	     PTR cdata,
	     i4  linstance,
	     char *instance,
	     PTR *instdata )
{
    MO_INSTANCE *ip;
    PTR myidata;
    STATUS stat;

    stat = MO_ipindex( msg, cdata, linstance, instance, &myidata );
    ip = (MO_INSTANCE *)myidata;

    if( stat == OK || stat == MO_INSTANCE_TRUNCATED )
	*instdata = (PTR)ip->instance;

    return( stat );
}

/* end of moattach.c */
