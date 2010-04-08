/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <st.h>
# include <sp.h>
# include <mo.h>

# include "moint.h"

/**
** Name:	momon.c	- MO monitor related functions
**
** Description:
**
**	This file defines:
**
**	MO_igetmon		- get monitor given character classid.instance
**	MO_mon_index		- index function for monitor lookup
**	MO_mon_id_get		- get method for monitor id.
**	MO_mon_class_get	- get function for monitor classid.
**	MO_nullstr_get		- MOstpget that handles null.
**	MO_getmon		- locate monitor block
**	MO_delmon		- delete a MO_MON_BLOCK
**	MOset_monitor		- set monitor function for a class
**	MO_tell_class		- tell monitor when you have class pointer
**	MOtell_monitor		- call class monitor function for an instance
**
** History:
**	15-Jan-92 (daveb)
**		Upgraded to MO.9 spec.
**	27-Jul-92 (daveb)
**		Allow multiple monitors per classid (as per MO 1.6), and
**		qualify them with regexps.  Regexps don't work yet.
**	24-sep-92 (daveb)
**	    documented
**	4-Dec-1992 (daveb)
**	    Change exp.gl.glf to exp.glf for brevity
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	4-Aug-1993 (daveb) 58937
**	    cast PTRs before use.
**      24-sep-96 (mcgem01)
**          Global data moved to modata.c
**	21-may-1997 (thoda04)
**	    Add function prototype for MO_mon_id_get before referenced.
**      21-jan-1999 (canor01)
**          Remove erglf.h. Rename "class" to "mo_class".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**/


/* forward refs */

MO_MON_BLOCK *MO_getmon( MO_CLASS *cp, PTR mon_data );
STATUS MO_mon_id_get(i4 offset, i4  objsize, PTR object,
                     i4  luserbuf, char *userbuf);

/* Darned few default get methods here, sigh.  That means this is
   fairly tricky stuff. */

GLOBALREF MO_CLASS_DEF MO_mon_classes[];



/*{
** Name:	MO_igetmon	- get monitor given character classid.instance
**
** Description:
**	Given a string of the form "classid.instance", return a pointer
**	to the monitor for the instance.
**
**	Problems:  uses fixed length buffer for the classid.
**
** Re-entrancy:
**	yes, assuming MO mutex is held.
**
** Inputs:
**	instance	string to use as the key.
**
** Outputs:
**	none.
**
** Returns:
**	pointer to an MO_MON_BLOCK, or NULL if none exists.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

MO_MON_BLOCK *
MO_igetmon( char *instance )
{
    char classid[ 80 ];
    MO_CLASS *cp;
    char *p;
    u_i4 mon_data_val;

    for( p = classid; *instance && *instance != '.' ; )
	*p++ = *instance++;
    *p = EOS;
    cp = NULL;
    MO_getclass( classid, &cp );

    if( *instance )
	instance++;

    MO_str_to_uns( instance, &mon_data_val );

    return( MO_getmon( cp, (PTR)mon_data_val ) );
}


/*{
** Name:	MO_mon_index	- index function for monitor lookup
**
** Description:
**	An index method for perusing the monitor tree.
**
** Re-entrancy:
**	yes, assuming MO mutex is held.
**
** Inputs:
**	msg		MO_GET, or MO_GETNEXT.
**	cdata		ignored in this function.
**	linstance	length of the instance buffer.
**	instance	the instance in question.
**
** Outputs:
**	instance	filled in for getnext
**
** Returns:
**	OK
**	MO_NO_INSTANCE
**	MO_NO_NEXT
**	MO_BAD_MSG
**	MO_INSTANCE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_mon_index(i4 msg,
	     PTR cdata,
	     i4  linstance,
	     char *instance,
	     PTR *instdata )
{
    STATUS ret_val = OK;

    MO_MON_BLOCK *mp;

    do				/* one-time through only */
    {
	mp = NULL;
	if( *instance || msg == MO_GET )
	{
	    mp = MO_igetmon( instance );
	    if( mp == NULL )
	    {
		ret_val = MO_NO_INSTANCE;
		break;
	    }
	    else
	    {
		/* FIXME */
	    }
	}
	else
	{
		/* FIXME */
	}

	switch( msg )
	{
	case MO_GET:

	    *instdata = (PTR)mp;
	    break;

	case MO_GETNEXT:

	    mp = (MO_MON_BLOCK *)(mp ?
				  SPfnext( &mp->node ) :
				  SPfhead( MO_monitors ) );

	    if( mp == NULL )
	    {
		ret_val = MO_NO_NEXT;
	    }
	    else
	    {
		*instdata = (PTR)mp;
		ret_val = MO_mon_id_get( 0, 0, (PTR)mp, linstance, instance );
		if( ret_val == MO_VALUE_TRUNCATED )
		    ret_val = MO_INSTANCE_TRUNCATED;
	    }
	    break;

	default:
	    ret_val = MO_BAD_MSG;
	    break;
	}
    } while( FALSE );

    return( ret_val );
}


/*{
** Name:	MO_mon_id_get	-- get method for monitor id.
**
** Description:
**	A get method to make the monitor id visible.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	objsize		ignored
**	object		an MO_MON_BLOCK
**	luserbuf	length of out buf
**
** Outputs:
**	userbuf		out buf
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** Side Effects:
**	May temporarily allocated some memory, freed before return.
**
** History:
**	24-sep-92 (daveb)
**	    documented
**	7-sep-93 (swm)
**	    call MOptrout() rather than MOulongout() for mp->mon_data to
**	    avoid i4/pointer truncation errors.
*/
STATUS
MO_mon_id_get(i4 offset,
	     i4  objsize,
	     PTR object,
	     i4  luserbuf,
	     char *userbuf)
{
    STATUS ret_val = OK;
    MO_MON_BLOCK *mp = (MO_MON_BLOCK *)object;
    char localbuf[ MO_MAX_NUM_BUF * 3 + 2 ];
    char numbuf[ MO_MAX_NUM_BUF ];
    i4  len;
    char *outbuf;

    (void)MOptrout( 0, mp->mon_data, sizeof( numbuf ), numbuf );
    len = (i4)(STlength( mp->mo_class->node.key ) + STlength(numbuf) + 2);
    if(  len <= sizeof( localbuf ) )
	outbuf = localbuf;
    else
	outbuf = MO_alloc( len, &ret_val );

    if( ret_val == OK )
    {
	STprintf( outbuf, "%s.%s", mp->mo_class->node.key, numbuf );
	ret_val = MOstrout( MO_VALUE_TRUNCATED, outbuf, luserbuf, userbuf );
	if( outbuf != localbuf )
	    MO_free( outbuf, len );
    }
# if 0
    if( ret_val != OK )
	/* FIXME */
# endif

    return( ret_val );
}



/*{
** Name:	MO_mon_class_get	 - get function for monitor classid.
**
** Description:
**
**	Get method for monitor class.
**	There's an indirection here or we'd use MOstrpget
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored
**	size		ignored
**	object		MO_MON_BLOCK
**	lsbuf		length of output buffer
**	sbuf		output buffer
**
** Outputs:
**	sbuf		buffer written with classid of the monitor.
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_mon_class_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    MO_MON_BLOCK *mp = (MO_MON_BLOCK *)object;

    return( MOstrout( MO_VALUE_TRUNCATED, mp->mo_class->node.key, lsbuf, sbuf ) );
}



/*{
** Name:	MO_nullstr_get	- MOstpget that handles null.
**
** Description:
**
**	Just like MOstpget, but handles being given a NULL pointer.
**	(Puts "<NULL>" if input object is NULL).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		added to object ptr.
**	size		ignored.
**	object		base object pointer.
**	lsbuf		length of output buffer.
**	sbuf		output buffer
**
** Outputs:
**	sbuf		output buffer
**
** Returns:
**	OK
**	MO_VALUE_TRUNCATED
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_nullstr_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS ret_val = OK;
    char *objp = (char *)object + offset;
    char *cobj = *(char **)objp;

    ret_val = MOstrout( MO_VALUE_TRUNCATED, cobj ? cobj : "<NULL>",
		       lsbuf, sbuf );

# if 0
    if( ret_val != OK )
	MO_breakpoint();
# endif

    return( ret_val );
}


/*{
** Name:	MO_getmon	- locate monitor block
**
** Description:
**	Given the class pointer and mon_data, get the monitor block.
**
** Re-entrancy:
**	yes, assuming MO mutex is held.
**
** Inputs:
**	cp		class pointer
**	mon_data	the mon_data in question
**
** Outputs:
**	none.
**
** Returns:
**	MO_MON_BLOCK or NULL if not found.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

MO_MON_BLOCK *
MO_getmon( MO_CLASS *cp, PTR mon_data )
{
    MO_MON_BLOCK mon;
    MO_MON_BLOCK *mp;

    mon.node.key = (PTR)&mon;
    mon.mo_class = cp;
    mon.mon_data = mon_data;
    mp = (MO_MON_BLOCK *) SPlookup( &mon.node, MO_monitors );
    return( mp );
}


/*{
** Name:	MO_delmon	- delete a MO_MON_BLOCK
**
** Description:
**	Delete the specified block from the monitor tree.
**
** Re-entrancy:
**	yes, assuming MO mutex is held.
**
** Inputs:
**	mp	the monitor to delete.
**
** Outputs:
**	none.
**
** Returns:
**
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

VOID
MO_delmon( MO_MON_BLOCK *mp )
{
    MO_MON_BLOCK *nmp;		/* next monitor */
    MO_CLASS *cp = mp->mo_class;

    /* free the regexp we saved earlier */

    if( mp->qual_regexp != NULL )
	MO_delstring( mp->qual_regexp );

    /* If this is the first monitor for the class, then we need to
       point the class at the next one, because this one is going away.
       If the next one is for a different class, then there are no more
       monitors for this class. */

    if( cp->monitor == mp )
    {
	nmp = (MO_MON_BLOCK *)SPfnext( &mp->node );
	cp->monitor = ( nmp != NULL && nmp->mo_class == cp ) ? nmp : NULL;

	if( cp->twin )
	    cp->twin->monitor = cp->monitor;
    }

    /* slice it out of the tree*/

    SPdelete( &mp->node, MO_monitors );

    /* recover memory for the node */

    MO_free( (PTR)mp, sizeof(*mp) );

    MO_ndel_monitor++;
}


/*{
**  Name:	MOset_monitor - set monitor function for a class
**
**	Arranges to have a monitor function called as a direct result of
**	an MOtell_monitor call or indirectly from other operations.  The
**	monitors for a classid and it's twin are kept identical.  There
**	may be multiple monitors for a classid.  Monitors are installed
**	for both the classid and it's twin.  The monitor is uniquely
**	identified by it's classid and mon_data value.  (The same
**	mon_data may be used for multiple classids.)
**
**	A monitor is deleted by specifying a NULL monitor function for a
**	{classid, mon_data} pair.
**
**	The output parameter old_monitor is filled in with the previous
**	monitor for the {classid, mon_data}.
**
**	When a call to the monitor might be made, the instance will be
**	qualified by the qual_regexp regular expression specified when
**	the monitor was set.  If the instance matches, or the expression
**	was NULL, then the monitor is called.
**
**	The monitor is called with the classid, the classid of the twin,
**	if any, the qualified instance, a value, a message type, and the
**	mon_data value.
**
**	If the class is not currently defined, the call fails with
**	MO_NO_CLASSID.  It may also fail for inability to get memory.
**
**	The new monitor function is not called by the MOset_monitor
**	call.
**  Inputs:
**
**	 classid
**		the classid of the object to be monitored.
**	 mon_data
**		data to be handed to the MO_MONITOR_FUNC for the classid.
**	 qual_regexp
**		the qualifying regular expression string, or NULL.
**	 monitor
**		the function to call for events affecting the instance.
**
**  Outputs:
**	old_monitor
**		any old monitor for the {classid,mon_data} pair.
**
**  Returns:
**	 OK
**		if the monitor was attached.
**	 MO_NO_CLASSID
**		if the classid wasn't defined.
**	 MO_BAD_MONITOR
**		if the {classid, mon_data} doesn't exist and the new monitor is
**		NULL (trying to delete one that doesn't exist).
**	 MO_MEM_LIMIT_EXCEEDED
**		couldn't allocate memory for the monitor.
**  History:
**	23-sep-92 (daveb)
**	    documented
**	4-Aug-1993 (daveb)
**	    cast PTRs before use.
*/

STATUS
MOset_monitor(	char *classid,
		PTR mon_data,
		char *qual_regexp,
		MO_MONITOR_FUNC *monitor,
		MO_MONITOR_FUNC **old_monitor )
{
    MO_MON_BLOCK *mp;		/* our monitor block */
    MO_MON_BLOCK *fmp;		/* first monitor block */
    MO_MON_BLOCK *pmp;		/* previous monitor block */
    MO_CLASS *cp;		/* class in question (not classid) */
    char *saved_qual = NULL;

    STATUS ret_val = OK;
    STATUS mutex_stat;

    mutex_stat = MO_mutex();
    do
    {
	if( mutex_stat != OK )
	    break;

	/* if we'll need it, save any qual_regexp string */

	if( monitor != NULL && qual_regexp != NULL )
	{
	    saved_qual = MO_defstring( qual_regexp, &ret_val );
	    if( saved_qual == NULL )
		break;

	    /* FIXME maybe compile it here too? */
	}

	/* locate the class def, we need it everywhere */

	if( (ret_val = MO_getclass( classid, &cp )) != OK )
	    break;

	/* see if this is a replacement for an existing monitor */

	mp = NULL;
	if( (mp = MO_getmon( cp, mon_data ) ) != NULL )
	{
	    /* yes, replace it with new values as appropriate */

	    *old_monitor = mp->monitor;
	    if( monitor == NULL ) /* delete this monitor */
	    {
		MO_delmon( mp );
	    }
	    else		/* replace this one's values */
	    {
		mp->monitor = monitor;
		mp->mon_data = mon_data;

		/* delete old qual string */
		if( mp->qual_regexp != NULL )
		    MO_delstring( mp->qual_regexp );
		mp->qual_regexp = saved_qual;
	    }
	}
	else if( monitor == NULL ) /* huh!? */
	{
	    ret_val = MO_BAD_MONITOR;
	    break;
	}
	else			/* make a new one and link it in. */
	{
	    *old_monitor = NULL;

	    /* if it's an OID, get the class def instead */

	    if( CMdigit( (char *)cp->node.key ) && cp->twin != NULL )
		cp = cp->twin;

	    /* get a new monitor block */

	    mp = (MO_MON_BLOCK *)MO_alloc( sizeof( *mp ), &ret_val );
	    if( mp == NULL )
		break;

	    /* fill in the new block, and link in to the tree */

	    mp->node.key = (PTR)mp;
	    mp->mo_class = cp;
	    mp->monitor = monitor;
	    mp->mon_data = mon_data;
	    mp->qual_regexp = saved_qual;
	    (void) SPenq( &mp->node, MO_monitors );

	    /* Fill in the class with first monitor for the class.
	       The loop below goes backwards until pmp is null or
	       doesn't have the same class definition.  When it exits,
	       fmp is at the first monitor block for the class */

	    for( fmp = mp;
		(pmp = (MO_MON_BLOCK *)SPfprev( &fmp->node )) != NULL &&
		pmp->mo_class == cp; fmp = pmp )
		continue;

	    fmp->mo_class->monitor = fmp;
	    if( cp->twin )
		cp->twin->monitor = fmp;
	}

    } while ( FALSE );

    if( mutex_stat != OK )
    {
	ret_val = mutex_stat;
    }
    else
    {
	if( ret_val == OK )
	    MO_nset_monitor++;
	else if( saved_qual != NULL )
	    MO_delstring( saved_qual );

	(void) MO_unmutex();
    }

    return( ret_val );
}


/*{
** Name:	MO_tell_class	-- tell monitor when you have class pointer
**
** Description:
**
**	tell all the qualified monitors for the class about
**	the event.  Called with the MO mutex held.
**
**	FIXME:  doesn't currently qualify with REmatch.
**
** Re-entrancy:
**	yes, assuming MO mutex is held.
**
** Inputs:
**	cp		class in question
**	instance	instance in question
**	value		to send
**	msg		to send
**
** Outputs:
**	none.
**
** Returns:
**	OK or return of monitors.
**
** History:
**	24-sep-92 (daveb)
**	    documented
*/

STATUS
MO_tell_class( MO_CLASS *cp, char *instance, char *value, i4  msg )
{
    MO_MON_BLOCK *mp;
    STATUS ret_val = OK;

    for( mp = cp->monitor;
	ret_val == OK && mp != NULL && mp->mo_class == cp;
	mp = (MO_MON_BLOCK *)SPfnext( &mp->node ) )
    {
	/* FIXME:  handle REmatch correctly */

	if( mp->qual_regexp == NULL
	   /* || REmatch( mp->qual_regexp, instance ) */ )
	{
	    MO_ntell_monitor++;

	    ret_val = (*mp->monitor)( cp->node.key,
				     cp->twin ? cp->twin->node.key : NULL,
				     instance,
				     value,
				     mp->mon_data,
				     msg );
	}
    }
    return( ret_val );
}


/*{
**  Name:	MOtell_monitor - call class monitor function for an instance
**
**  Description:
**
**	Consider call any monitor functions for the classid and it's
**	twin with the specified value and message.  The instance
**	provided to the MOtell_monitor call will be qualified by the
**	qua_regexps provided with each monitor instance; only those
**	which match will be called.
**
**	If the class is not defined or no monitor exists, returns OK.
**
**	Returns the status returned by the called monitor functions.  It
**	stops calling monitors when one returns error status.  In most
**	cases this should be ignored, because the caller of
**	MOtell_monitor has no idea who is being called.
**
**  Inputs:
**	 classid
**		the classid whose monitor is to be notified.
**	 instance
**		the instance to give to the monitor.
**	 value
**		the value to give to the monitor.
**	 msg
**		the message type to give to the monitor, one of MO_GET,
**		MO_GETNEXT, MO_SET, MO_SET_MONITOR, or MO_DEL_MONITOR.
**  Outputs:
**	 none.
**  Returns:
**	 OK
**		or other error status.  Usually best ignored.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS
MOtell_monitor( char *classid, char *instance, char *value, i4  msg )
{
    MO_CLASS *cp;
    STATUS ret_val = OK;

    MO_once();
    ret_val = MO_mutex();
    if( ret_val == OK )
    {
	ret_val = MO_getclass( classid, &cp );
	if( ret_val == OK && cp->monitor != NULL )
	    ret_val = MO_tell_class( cp, instance, value, msg );

	(VOID) MO_unmutex();
    }
    return( ret_val );
}

/* end of momon.c */

