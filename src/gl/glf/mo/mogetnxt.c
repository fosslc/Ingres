/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <st.h>
# include <cv.h>
# include <sp.h>
# include <mo.h>
# include "moint.h"

/**
**  Name: mogetnxt.c -- getnext operation for MO.
**
**  Description:
**
**	This module provides a symbol table that can be used by CL and
**	mainline clients to register objects of varying types as management
**	objects.  These objects can be get/set by classid, and may be
**	scanned.
**
**  Intended uses:
**
**	This is intended to be used by all server code that wishes to
**	export management/monitoring objects.
**
**  Interface:
**
**    The file defines the following visible functions.
**
**	MOgetnext		get data from the object.
**	MO_getnext		get data from the object, MO mutex held
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
**	30-Nov-1992 (daveb)
**	    Fix incorrect error code- return MO_NO_CLASSID, not
**	    MO_NO_INSTANCE.
**	5-May-1993 (daveb)
**	    Rename file mogetnxt.c instead of mogetnext.c, to meet
**	    msdos/coding standard conventions.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>*
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-aug-2001 (somsa01)
**	   Cleaned up compiler warnings.
**	 6-Apr-04 (gordy)
**	    Permit access if MO_ANY_READ attribute is set.
*/


/*
** Name:  MOgetnext - get next object in class, instance order.
**
**	This public interface grabs the MO mutex before doing anything.
**
**	Given an object, attempts to return the next one.  Ordering is
**	as by STcompare of { classid, instance } of objects including
**	the input valid_perms bits.
**
**	As a special case, if the classid and instance are both a 0
**	length strings, then return the first of any instances in the
**	class space that match its valid_perms.
**
**	If the input instance does not exist or doesn't include
**	valid_perms, returns MO_NO_INSTANCE, and the output class is
**	unknown.
**
** Inputs:
**	valid_perms
**	the roles currently in effect, to be and-ed with object permissions to
**	determine access rights.
**
**	lclassid	length of the buffer for the classid.
**
**	classid		the classid in question.
**
**	linstance	length of the buffer for the instance.
**
**	instance	the instance in question.
**
**	lsbufp		pointer to the length of the output string buffer.
**
** Outputs:
**
**	classid		filled in with the new classid.
**
**	instance	filled in with the new instance.
**
**	lsbufp		if *sval is NULL, contains the actual length
**			of the value in sbuf.
**
**	sbuf		if sval is NULL, a copy of the output string.
**			If it wasn't long enough, returns
**			MO_TRUNCATED.
**
**	perms		the permissions for the returned instance.
**
** Returns:
**
**	OK		if the retrieval returned data.
**
**	MO_TRUNCATED 	output buffer for class, instance, or sbuf was
**			too small, and one or more was truncated.
**
**	MO_NO_INSTANCE	if the input class/instance is not attached,
**			or it's permissions do not include the
**			valid_perms bits.
**
**	MO_NO_NEXT	no value following input class.
**
**	other error status, returned by a method.
**
*/

STATUS
MOgetnext(i4 valperms,
	  i4  lclassid,
	  i4  linstance,
	  char *classid,
	  char *instance,
	  i4  *lsbufp,
	  char *sbuf,
	  i4  *operms )
{
    STATUS stat = OK;

    MO_once();
    stat = MO_mutex();
    if( stat == OK )
    {
	stat = MO_getnext(valperms, lclassid, linstance,
			 classid, instance,
			 lsbufp, sbuf, operms );

	(VOID) MO_unmutex();
    }
    return( stat );
}


/*{
**  Name:	MO_getnext - get next object in class, instance order.
**
**  Description:
**
**	This private interface assumes the MO mutex is held.
**
**	Given an object, attempts to return the next one that is
**	readable.  Ordering is as by STcompare of { classid, instance }
**	of objects including the input valid_perms bits.
**
**	As a special case, if the classid and instance are both a 0
**	length strings, then return the first of any instances in the
**	class space that match its valid_perms.
**
**	If the input instance does not exist or doesn't include
**	valid_perms, returns MO_NO_INSTANCE, and the output class is
**	untouched.  If there is no readable successor to the input
**	object, returns MO_NO_NEXT.
**
**	If the classid of the returned object won't fit in the provided
**	buffer, returns MO_CLASSID_TRUNCATED.  If the instance of the
**	returned object won't fit in the provided buffer, returns
**	MO_INSTANCE_TRUNCATED.  If the value returned won't fit in the
**	supplied buffer, returns MO_VALUE_TRUNCATED.  Any of these
**	truncation errors is a serious programming error, which should
**	abort processing as it may be impossible to continue a get next
**	scan from that point.  Client buffers should always be sized big
**	enough to avoid truncation, so truncation is a serious
**	programming error.  MOgetnext will not work as expected when
**	handed a truncated classid or instance for a subsequent call.
**
**	Existance of the input object is determined by calling the idx
**	method for the class with the MO_GETNEXT message.
**
**	If the call succeeds, it must also call the monitors for the
**	class with the MO_GET event, the instance id, and the current
**	value.
**  Inputs:
**	 valid_perms
**		the roles currently in effect, to be and-ed with object
**		permissions to determine access rights.
**	 lclassid
**		length of the buffer for the classid.
**	 classid
**		the classid in question.
**	 linstance
**		length of the buffer for the instance.
**	 instance
**		the instance in question.
**	 lsbufp
**		pointer to the length of the output string buffer.
**  Outputs:
**	 classid
**		filled in with the new classid.
**	 instance
**		filled in with the new instance.
**	 lsbufp
**		if *sval is NULL, contains the actual length of the value in
**		sbuf.
**	 sbuf
**		if sval is NULL, a copy of the output string.  If it
**		wasn't long enough, returns MO_VALUE_TRUNCATED.
**	 perms
**		the permissions for the returned instance.
**  Returns:
**	 OK
**		if the retrieval returned data.
**	 MO_CLASSID_TRUNCATED
**		buffer for classod was too small, and output was truncated.
**	 MO_INSTANCE_TRUNCATED
**		buffer for instance was too small and output was truncated.
**	 MO_VALUE_TRUNCATED
**		buffer for value was too small, and output was truncated.
**	 MO_NO_CLASSID
**		the input class did not exist, or it's permissions
**		did not include readable valid_perms bits
**	 MO_NO_INSTANCE
**		the input instance did not exist, or it's permissions
**		didnot include readable valid_perms bits
**	 MO_NO_NEXT
**		ther was no readable object following the input one.
**	 other
**		error status, returned by a method.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	30-Nov-1992 (daveb)
**	    Fix incorrect error code- return MO_NO_CLASSID, not MO_NO_INSTANCE.
**	 6-Apr-04 (gordy)
**	    Permit access if MO_ANY_READ attribute is set.
*/

STATUS
MO_getnext(i4 valperms,
	   i4  lclassid,
	   i4  linstance,
	   char *classid,
	   char *instance,
	   i4  *lsbufp,
	   char *sbuf,
	   i4  *operms )
{
    STATUS stat = OK;
    STATUS xstat = OK;

    MO_INSTANCE *ip;
    MO_CLASS *cp;		/* this class */
    PTR idata;

    MO_ngetnext++;

# ifdef xDEBUG
    SIprintf("getnext entry: %s:%s\n", classid, instance );
# endif

    ip = NULL;
    cp = NULL;
    *sbuf = EOS;

    if( *instance != EOS )
    {
	ip = MO_getinstance( classid, instance );
	if( ip != NULL )
	    cp = ip->classdef;
	else if( OK != MO_getclass( classid, &cp ) )
	    stat = MO_NO_CLASSID;
    }
    else if( *classid != EOS && OK != MO_getclass( classid, &cp ) )
    {
	stat = MO_NO_CLASSID;
    }

    /*
    ** Now do the real work:  Given the starting point implied
    ** by ip and cp, find the next visible instance.
    */
    if( stat == OK )
    {
	if( cp == NULL )	/* start with first class */
	{
	    cp = (MO_CLASS *) SPfhead( MO_classes );
	    ip = NULL;
	}

	/*
	 ** Loop over classes looking for the next valid instance.
	 **
	 ** On exit, if cp != NULL, it's the class of the found instance;
	 **	if NULL, then we never found a good instance.
	 */
	for( ; cp != NULL ; cp = (MO_CLASS *)SPnext( &cp->node, MO_classes ) )
	{
	    if( (cp->perms & MO_ANY_READ) != 0  ||
		(valperms & MO_READ & cp->perms) != 0 )
	    {
		stat = (*cp->idx)( MO_GETNEXT, cp->cdata,
			      linstance, instance, &idata );
		if( stat == OK )
		{
		    stat = (*cp->get)( cp->offset,
				      cp->size, idata,
				      *lsbufp, sbuf  );
		    if( stat == OK )
			break;
		}
	    }
	    /* Now caller's ip and instance are bogus; set to search state */

	    ip = NULL;
	    *instance = EOS;
	}

	if( cp == NULL )
	    stat = MO_NO_NEXT;

	if( stat == OK || MO_IS_TRUNCATED( stat ) )
	    xstat = MOstrout( MO_CLASSID_TRUNCATED,
			      cp->node.key, lclassid, classid );

	if( stat == OK )
	    stat = xstat;
    }

    *lsbufp = (i4)STlength( sbuf );

    if( stat == OK || stat == MO_VALUE_TRUNCATED )
    {
	*operms = cp->perms;
	(VOID) MO_tell_class( cp, instance, sbuf, MO_GET );
    }

# ifdef xDEBUG
    SIprintf("getnext exit: (%d) %s:%s - %s\n",
	     stat, classid, instance, sbuf );
# endif
    return( stat );
}

