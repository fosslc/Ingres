/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cv.h>
# include <me.h>
# include <si.h>
# include <st.h>
# include <sp.h>
# include <mo.h>

# include "moint.h"

/**
** Name:	mostr.c	-- string cache for MO
**
** Description:
**	This file defines:
**
**	MO_str_classes[]	- classes for string cache management
**	MO_showstrings		- debug SIprintf string cache.
**	MO_string		- lookup possibly non-present string.
**	MO_defstring		- Get pointer to saved string, bumping ref count.
**	MO_delstring		- delete ref to a string, cleaning cache.
**	MO_strindex		- index method for string cache management.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	29-mar-93 (swm)
**	    Changed buf parameter to buf[0] in CL_OFFSETOF macro invocation.
**	    This eliminates complaints from the DEC Alpha AXP OSF C compiler.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's in MO_showstrings().
**	22-apr-1996 (lawst01)
**	    Windows 3.1 port changes - added cast to function arguments.
**      24-sep-96 (mcgem01)
**          Global data moved to modata.c
**      30-jul-97 (teresak)
**          Modified MO_INDEX_METHOD to be FUNC_EXTERN instead of GLOBALREF. 
**	    GLOBALREF has a specific meaning to VMS and was generating compiler 
**	    warnings used for use in this context.
**      21-jan-1999 (canor01)
**          Remove erglf.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**/

/* # define xDEBUG */

/* forward refs */

FUNC_EXTERN MO_INDEX_METHOD MO_strindex;

GLOBALREF i4 MO_max_str_bytes;
GLOBALREF i4 MO_str_defines;
GLOBALREF i4 MO_str_deletes;
GLOBALREF i4 MO_str_bytes;
GLOBALREF i4 MO_str_freed;


static char index_class[] = MO_STRING_CLASS;

GLOBALREF MO_CLASS_DEF MO_str_classes[];


/*{
** Name:	MO_showstrings	- debug SIprintf string cache.
**
** Description:
**	Prints the contents of the string cache using SIprintf.
**	Not called in normal code.
**
** Re-entrancy:
**	MO mutex should be held.
**
** Inputs:
**	none
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	<manual entries>
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x's to %p's.
*/
VOID
MO_showstrings(void)
{
# ifdef xDEBUG

    MO_STRING *stp;
    SPBLK *sp;

    for( sp = SPfhead( MO_strings ); sp != NULL;
	sp = SPnext( sp, MO_strings ) )
    {
	stp = (MO_STRING *)sp;
	SIprintf("stp %p, ", stp );
	SIprintf("sp %p\n", sp);
	SIprintf("stp %p->node.key ", stp );
	SIprintf("%x ", stp->node.key );
	SIprintf("'%s' refs ", stp->node.key );
	SIprintf("%d\n", stp->refs );
	SIflush( stdout );
    }
# endif
}


/*{
** Name:	MO_string	- lookup possibly non-present string.
**
** Description:
**
**	Lookup the string block for a possibly non-present string
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	s		the string to find.
**
** Outputs:
**	none.
**
** Returns:
**	MO_STRING, or NULL if it isn't present.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

MO_STRING *
MO_string( char *s )
{
    SPBLK sb;
    sb.key = (PTR)s;
    return( (MO_STRING *)SPlookup( &sb, MO_strings ) );
}


/*{
** Name:	MO_defstring	- Get pointer to saved string, bumping ref count.
**
** Description:
**	Defines a new reference to a string, and hands back a pointer to
**	a safe copy of it.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	s		string to save.
**
** Outputs:
**	stat		potential error status, if return is NULL.
**			Might be MO_NO_STRING_SPACE or another allocation
**			failure.
**
** Returns:
**	pointer to the saved string, or NULL, in which case stat will have
**	the details.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

char *
MO_defstring( char *s, STATUS *stat )
{
    SPBLK sb;
    MO_STRING *stp;
    i4  len;
    i4  blen;

    sb.key = (PTR)s;
    stp = (MO_STRING *)SPlookup( &sb, MO_strings );
    if( stp != NULL )
    {
	stp->refs++;
	*stat = OK;
    }
    else
    {
	/* add in length of "ref" field */
	len = (i4)STlength( s ) + 1;
	blen = len + sizeof(SPBLK) + sizeof(i4);
	*stat = MO_NO_STRING_SPACE;
	if( (MO_str_bytes + blen) <= MO_max_str_bytes &&
	   ( NULL != (stp = (MO_STRING *)MO_alloc( blen, stat ) ) ) )
	{
	    MO_str_defines++;
	    MEcopy( s, len, stp->buf );
	    stp->refs = 1;
	    stp->node.key = (PTR)stp->buf;
	    (VOID) SPenq( &stp->node, MO_strings );
	    MO_str_bytes += blen;
	    *stat = OK;
	}
    }
    return( stp ? stp->buf : NULL );
}



/*{
** Name:	MO_delstring	- delete ref to a string, cleaning cache.
**
** Description:
**	Delete a deference to a string in the cache.  If there are no more
**	new references, recover the space in the cache devoted to the string.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	s		string to remove reference to.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/
VOID
MO_delstring( char *s )
{
    MO_STRING *stp;
    i4  blen;

    stp = MO_string( s );
    if( stp != NULL && --(stp->refs) == 0 )
    {
	MO_str_deletes++;
	blen = (i4)STlength( stp->buf ) + 1 + sizeof(SPBLK) + sizeof(i4);
	MO_str_bytes -= blen;
	MO_str_freed += blen;
	SPdelete( (SPBLK *)stp, MO_strings );
	MO_free( (PTR)stp, blen );
    }
}


/*----------------------------------------------------------------
**
** Methods for self-management
*/

/*{
** Name:	MO_strindex	-- index method for string cache management.
**
** Description:
**	A standard index method for use with the string cache.  The output
**	instance pointer is to the MO_STRING block.  Instances are the
**	actual string value.
**
** Re-entrancy:
**	yes, if MO mutex is held
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT
**	cdata		ignored.
**	linstance	length of instance buffer.
**	instance	to lookup
**	instdata	to stuff.
**
** Outputs:
**	instance	the instance string, on getnext.
**	instdata	filled with the MO_STRING location.
**
** Returns:
**	OK
**	MO_BAD_MSG
**	MO_NO_INSTANCE
**	MO_NO_NEXT
**	MO_INSTANCE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_strindex(i4 msg,
	    PTR cdata,
	    i4  linstance,
	    char *instance,
	    PTR *instdata )
{
    STATUS stat = MO_NO_INSTANCE;
    MO_STRING *stp = NULL;

    if( *instance != EOS )
	stp = MO_string( instance );

    switch( msg )
    {
    case MO_GET:

	if( stp != NULL )
	{
	    *instdata = (PTR)stp;
	    stat = OK;
	}
	break;

    case MO_GETNEXT:

	if( stp != NULL )	/* have this instance */
	    stp = (MO_STRING *)SPfnext( &stp->node );
	else if( *instance == EOS ) /* no this try first. */
	    stp = (MO_STRING *)SPfhead( MO_strings );
	else
	    break;		/* no instance */

	if( stp == NULL )	/* fnext or fhead got NULL */
	{
	    stat = MO_NO_NEXT;
	}
	else			/* got the next instance */
	{
	    *instdata = (PTR)stp;
	    stat = MOstrout( MO_INSTANCE_TRUNCATED,
			    stp->buf, linstance, instance );
	}
	break;

    default:
	stat = MO_BAD_MSG;
	break;
    }

    return( stat );
}

/* end of mostr.c */
