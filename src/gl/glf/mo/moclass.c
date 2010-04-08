/*
** Copyright (c) 1987, 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <st.h>
# include <cm.h>
# include <sp.h>
# include <mu.h>
# include <mo.h>
# include "moint.h"

/**
** Name: moclass.c	- class functions for MO.
**
** Description:
**	This file defines:
**
**	MO_getclass	- locate class by name
**	MO_nxtclass	- get next classid, given current.
**	MOclassdef	- define a class
**	MOcdata_index	- standard index method for single instance class
**	MO_showclass	- SIprintf info about class.
**
** History:
**	29-Jan-92 (daveb)
**		created.
**	31-Jan-92 (daveb)
**		pulled out methods (mometa.c), leaving this
**		with class only functions.
**	24-sep-92 (daveb)
**	    documented
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p in MO_getclass().
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 6-Apr-04 (gordy)
**	    Include test for MO_ANY_{READ,WRITE} attributes when
**	    testing for valid access methods.
*/

# ifdef xDEBUG
void MO_showclasses( void );
# endif /* xDEBUG */


/*{
** Name: MO_getclass	- locate class by name
**
** Description:
**	given the name, get the MO_CLASS block.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	classid		string of the class to find.
**
** Outputs:
**	cpp		stuffed with a pointer to the block.
**
** Returns:
**	OK		if found and *cpp is valid.
**	MO_NO_CLASSID	if not found.
**
** History:
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p.
**	<manual entries>
*/
STATUS
MO_getclass( char *classid, MO_CLASS **cpp )
{
    MO_CLASS *cp;
    MO_CLASS cb;
    STATUS ret_val = OK;

    cb.node.key = classid;
    if( (cp = (MO_CLASS *)SPlookup( &cb.node, MO_classes )) == NULL )
	ret_val = MO_NO_CLASSID;
    else
	*cpp = cp;

# ifdef xDEBUG
    SIprintf("looking for class %s, got %p (%s)\n",
	     classid, cp, cp ? cp->node.key : "<nil>" );
    MO_showclasses();
# endif

    return( ret_val );
}


/*{
** Name: MO_nxtclass	-- get next classid, given current.
**
** Description:
**	Given a current classid, locate the next.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	classid		the current classid.
**
** Outputs:
**	cpp		stuffed with next MO_CLASS.
**
** Returns:
**	OK
**	MO_NO_CLASS	if current classid is not found.
**	MO_NO_NEXT	there are no more.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_nxtclass( char *classid, MO_CLASS **cp )
{
    MO_CLASS cb;
    STATUS ret_val = OK;

    cb.node.key = classid;
    if( *classid == EOS )	/* get first */
    {
	*cp = (MO_CLASS *)SPfhead( MO_classes );
	if( *cp == NULL )
	    ret_val = MO_NO_NEXT;
    }
    else			/* need exact match */
    {
	/* SPnext tends to drive the classes tree into a left-chain */

	*cp = (MO_CLASS *)SPlookup( &cb.node, MO_classes );
	if( *cp == NULL )
	    ret_val = MO_NO_CLASSID;
	else if( (*cp = (MO_CLASS *)SPfnext( &(*cp)->node ) ) == NULL )
	    ret_val = MO_NO_NEXT;
    }
    return( ret_val );
}


/*{
**  Name: MOclassdef - define a class
**
**  Description:
**
**	If MOon_off has been called with MO_DISABLE, returns OK
**	without doing anything.
**
**	Interprets up to nelem elements in the classes array as
**	definitions of classes.  The array may be terminated by a
**	member having a NULL classid value.
**
**	For each class, if the flags contain MO_CLASSID_VAR or
**	MO_INDEX_VAR, then the string will be saved in allocated
**	space, otherwise only a pointer will be kept.  The object size
**	is subsequently passed along to get and set methods, and is
**	the same for all instances.  The perms field will be AND-ed
**	with valid permissions given to the MOget and MOset functions
**	to provide some security.  The offset field will be passed
**	uninterpreted to the get and set methods for their use.  The
**	get band set fields define the functions that will be used to
**	convert back and forth between internal representations and
**	character form.  The cdata field contains data that will be
**	passed uninterpreted to the idx index method. The cdata allows
**	the index method to know what class it is being used for.  In
**	particular, the MO provided MOname_index and MOidata_index
**	methods require that this be the name of a classid.  This is
**	usually either the actual classid, by specifying
**	MO_CDATA_CLASSID, or the index class, by specifying
**	MO_CDATA_INDEX.
**
**	Returns OK if the class was defined sucessfully.  If the class
**	is already defined, MOattach will return OK if the current
**	definition is the same as the old one.  If it is not, returns
**	MO_INCONSISTENT_CLASS, and the original definition remains
**	unaffected.
**
**	Allocates memory for definition using MEreqmem.
**  Inputs:
**	 nelem
**		the maximum number of elements in the table to define
**	 classes
**		A pointer to an array of class definitions, terminated with one
**		having a null classid field.
**  Outputs:
**	 none
**  Returns:
**	 OK
**		is the object was attached successfully.
**	 MO_NO_MEMORY
**		if the MO_MEM_LIMIT would be exceeded.
**	 MO_NULL_METHOD
**		A null get or set method was provided, but the permissions said
**		get or set was allowed.
**	 other
**		system specific error status, possibly indicating allocation
**		failure.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	13-jun-2002 (devjo01)
**	    b108015 - potential for hang caused by recursive call
**	    to MOclassdef triggered by ME facility registering its
**	    class defs if a memory allocation needed by MOclassdef
** 	    happens to trigger the first MEget_page call on an
**	    OS_THREADS_USED platform.
**	 6-Apr-04 (gordy)
**	    Include test for MO_ANY_{READ,WRITE} attributes when
**	    testing for valid access methods.
*/
STATUS
MOclassdef( i4  nelem, MO_CLASS_DEF *cdp )
{
    static	i4		 recursion = 0;
    static	MU_SID		 whoisin;
    static	MO_CLASS_DEF	*deferred_cdp = NULL;
    static	i4		 deferred_nelem;

    MO_CLASS	*cp;
    MO_CLASS	*tcp;		/* twin in meta data */
    STATUS	stat;
    i4		lbuf;
    char	buf[ MO_MAX_CLASSID ];
    i4		perms;
    bool	cleanup_cp = FALSE;
    MU_SID	whoami;

    stat = OK;

    if( MO_disabled )
	return( stat );

    whoami = MUget_sid();

    if ( recursion && whoami == whoisin )
    {
	/*
	** Currently b108015 is the only way known by which we can
	** self deadlock in MO_mutex, but this additional logic
	** should protect against all one level single instance
	** recursions within MOclassdef.
	**
	** Only one thread can be within the protected portion,
	** so we should be able to save the recursed MO class def
	** in a simple static variable for later processing
	** by the non-recursive MOclassdef call.
	*/
	deferred_cdp = cdp;
	deferred_nelem = nelem;
	return( stat );
    }

    MO_once();
    stat = MO_mutex();
    if( stat != OK )
	return( stat );

    whoisin = whoami;
    recursion = 1;

    while ( cdp != NULL )
    {
	/* out-dent 1 step */

    for( ; nelem-- && cdp->classid != NULL ; cdp++ )
    {
	MO_nclassdef++;

	/* Get class, installing if needed. */

	if( OK == MO_getclass( cdp->classid, &cp ) )
	{
	    /* It's hard to tell if we have a consistent class
	       definition because of saved strings, and the
	       MO_INDEX_CLASSID, MO_CDATA_xxx stuff.  Assume
	       it's OK */
	}
	else			/* need to install new class */
	{
	    if(((cdp->perms & (MO_READ|MO_ANY_READ)) && cdp->get == NULL) ||
	       ((cdp->perms & (MO_WRITE|MO_ANY_WRITE)) && cdp->set == NULL ))
	    {
		stat = MO_NULL_METHOD;
		break;
	    }

	    cp = (MO_CLASS *) MO_alloc( sizeof(*cp), &stat );
	    if( NULL == cp )
		break;

	    /* Handle classid twin.  If it exists, we need to copy
	       relevant stuff, otherwise we just NULL it out */

	    lbuf = sizeof(buf);
	    stat = MO_get( ~0, MO_META_OID_CLASS, cdp->classid,
			  &lbuf, buf, &perms );
	    if( stat == OK )
	    {
		(VOID) MO_getclass( buf, &tcp );	/* "can't fail" */
		cp->twin = tcp;
		cp->monitor = tcp->monitor;
	    }
	    else
	    {
		stat = OK;
		cp->twin = NULL;
		cp->monitor = NULL;
	    }

	    /* set up the simple stuff in the block */

	    cp->cflags = cdp->flags;
	    cp->size = cdp->size;
	    cp->perms = cdp->perms;
	    cp->offset = cdp->offset;
	    cp->get = cdp->get;
	    cp->set = cdp->set;
	    cp->idx = cdp->idx ;

	    /* still need to figure these out; force to known state here */

	    cp->node.key = NULL;
	    cp->index = NULL;
	    cp->cdata = NULL;

	    /* Save the classid, either directly or saved */

	    if( (cp->cflags & MO_CLASSID_VAR) == 0 )
	    {
		cp->node.key = cdp->classid;
	    }
	    else		/* need to save it */
	    {
		cp->node.key = MO_defstring( cdp->classid, &stat );
		if( cp->node.key == NULL )
		{
		    cleanup_cp = TRUE;
		    break;
		}
	    }

	    /* save the index, from whatever source, maybe saving
	       a user supplied string. */

	    if( (cp->cflags & MO_INDEX_CLASSID) != 0 )
	    {
		cp->cflags &= ~MO_INDEX_VAR;
		cp->index = cp->node.key;
	    }
	    else if( (cp->cflags & MO_INDEX_VAR) == 0 )
	    {
		cp->index = cdp->index;
	    }
	    else
	    {
		cp->index = MO_defstring( cdp->index, &stat );
		if( cp->index == NULL )
		{
		    cleanup_cp = TRUE;
		    break;
		}
	    }

	    /* get the cdata for the index function, maybe
	       saving a user supplied string. */

	    if( (cp->cflags & MO_CDATA_CLASSID) != 0 )
	    {
		cp->cflags &= ~MO_CDATA_VAR;
		cp->cdata = cp->node.key;
	    }
	    else if( (cp->cflags & MO_CDATA_INDEX) != 0 )
	    {
		cp->cflags &= ~MO_CDATA_VAR;
		cp->cdata = cp->index;
	    }
	    else if( (cp->cflags & MO_CDATA_VAR) == 0 )
	    {
		cp->cdata = cdp->cdata;
	    }
	    else		/* is CDATA_VAR */
	    {
		cp->cdata = (PTR) MO_defstring( (char *)cdp->cdata, &stat );
		if( cp->cdata == NULL )
		{
		    cleanup_cp = TRUE;
		    break;
		}
	    }
	    (VOID) SPenq( &cp->node, MO_classes );
	}
    }

    if( cleanup_cp )
    {
	if( cp->cflags & MO_CLASSID_VAR && cp->node.key != NULL )
	    MO_delstring( cp->node.key );
	if( cp->cflags & MO_INDEX_VAR && cp->index != NULL )
	    MO_delstring( cp->index );
	if( cp->cflags & MO_CDATA_VAR && cp->cdata != NULL )
	    MO_delstring( cp->cdata );
	MO_free( (PTR)cp, sizeof(*cp) );
    }

	/* Resume normal indentation. */
	if ( stat != OK )
	    break;

	cdp = deferred_cdp;
	nelem = deferred_nelem;
	deferred_cdp = NULL;
    } /* end-while */

    recursion = 0;

# ifdef xDEBUG
    if( stat != OK )
	SIprintf("classdef %s stat %d\n", cdp->classid, stat );
# endif

    (VOID) MO_unmutex();
    return( stat );
}


/*{
**  Name: MOcdata_index - standard index method for single instance class
**
**  Description:
**
**	Index for using the cdata of the class definition as the data
**	belonging to instance "0" of the class.  MO_GETNEXT returns
**	instance "0" if the input instance is the empty string.  If
**	the instance is not the string "0" , returns MO_NO_INSTANCE.
**	If the msg is MO_GET, uses the input cdata value as the output
**	instdata.
**
**  Inputs:
**	 msg
**		either MO_GET or MO_GETNEXT
**	 cdata
**		class specific data that will be used as the output
**		instdata for instance "0" on an MO_GET.
**	 linstance
**		the length of the instance buffer
**	 instance
**		the instance in question.
**	 instdata
**		a pointer to a place to put instance-specific data.
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
**  History:
**	23-sep-92 (daveb)
**	    documented
*/
STATUS
MOcdata_index(i4 msg,
	      PTR cdata,
	      i4  linstance,
	      char *instance,
	      PTR *instdata )
{
    STATUS stat = OK;

    switch( msg )
    {
    case MO_GET:
	if( STequal( instance, "0" ) )
	    *instdata = cdata;
	else
	    stat = MO_NO_INSTANCE;
	break;

    case MO_GETNEXT:
	if( *instance == EOS )
	{
	    *instdata = cdata;
	    stat = MOstrout(MO_INSTANCE_TRUNCATED, "0", linstance, instance);
	}
	else if( STequal( instance, "0" ) )
	{
	    stat = MO_NO_NEXT;
	}
	else
	{
	    stat = MO_NO_INSTANCE;
	}
	break;

    default:
	stat = MO_BAD_MSG;
	break;
    }

    return( stat );
}

