/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/


# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <cv.h>
# include <me.h>
# include <sp.h>
# include <st.h>
# include <tm.h>
# include <erglf.h>
# include <mo.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <dmucb.h>
# include <ulf.h>
# include <ulm.h>
# include <gca.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"
# include "gwmplace.h"

/**
** Name:	gwmpmib.c	- place MIB for GWM.
**
** Description:
**	This file defines the objects that instantiate
**	MIB objects for the PLACES kept and queried from
**	within the GWM gateway.  It does not, at present,
**	provide access to the per-user vnode domains.
**
**	GM_pmib_init()	- add the place mib classes.
**
** History:
**	2-oct-92 (daveb)
**	    created
**	16-Nov-1992 (daveb)
**	    many changes to make work.
**	12-Jan-1993 (daveb)
**	    Make ttl and some maxes writable, make all writables need SERVER guise.
**	3-Feb-1993 (daveb)
**	    Improve comments, add servers.state object.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-sep-1996 (canor01)
**	    Moved global data definitions to gwmdata.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	24-Aug-10 (gordy)
**	    Fixed display of MIB registration bits.
**/

/* forwards */

static STATUS GM_subpindex(i4 msg, PTR cdata, i4  linstance,
			   char *instance,PTR *instdata );

/* the mib for PLACES */

MO_INDEX_METHOD GM_pindex;
MO_INDEX_METHOD GM_vindex;
MO_INDEX_METHOD GM_sindex;
MO_INDEX_METHOD GM_cindex;

MO_GET_METHOD GM_conn_get;
MO_GET_METHOD GM_cplace_get;
MO_GET_METHOD GM_srv_get;
MO_GET_METHOD GM_sflags_get;

MO_SET_METHOD GM_adddomset;
MO_SET_METHOD GM_deldomset;

GLOBALREF MO_CLASS_DEF GM_pmib_classes[];

/*{
** Name:	GM_pmib_init	- initialize global place MIB
**
** Description:
**	Defines the classes for the place (GCN) mib.  Should
**	only need to call this once.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Returns:
**	OK
**
** History:
**	9-oct-92 (daveb)
**	    documented.
*/
STATUS
GM_pmib_init(void)
{
    return( MOclassdef( MAXI2, GM_pmib_classes ) );
}


/*{
** Name:	GM_pindex	- index method for places
**
** Description:
**
**	An index method for looking into the internal place tree.
**	This grabs the gwm_places_sem.
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
**	instdata	stuffed with saved instance data, a GM_PLACE_BLK
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
**	2-oct-92 (daveb)
**	    documented
*/

STATUS 
GM_pindex(i4 msg,
	   PTR cdata,
	   i4  linstance,
	   char *instance, 
	   PTR *instdata )
{
    STATUS cl_stat;
    
    if( (cl_stat = GM_gt_sem( &GM_globals.gwm_places_sem )) == OK )
    {
	cl_stat = GM_subpindex( msg, cdata, linstance, instance, instdata );
	GM_release_sem( &GM_globals.gwm_places_sem );
    }
    return( cl_stat );
}



/*{
** Name:	GM_subpindex	- subroutine index method for places
**
** Description:
**
**	An index method for looking into the internal place tree,
**	called from other functions that grab the gwm_places_sem.
**
** Re-entrancy:
**	yes, if gwm_places_sem mutex is held.
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT.
**	cdata		the classid string.
**	linstance	length of instance buffer.
**	instance	the instance string
**
** Outputs:
**	instance	filled in with new instance on getnext.
**	instdata	stuffed with saved instance data, a GM_PLACE_BLK
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
**	2-oct-92 (daveb)
**	    documented
*/

static STATUS 
GM_subpindex(i4 msg,
	   PTR cdata,
	   i4  linstance,
	   char *instance, 
	   PTR *instdata )
{
    STATUS cl_stat = MO_NO_INSTANCE;
    GM_PLACE_BLK	*pb;
    SPBLK		lookup;
    
    lookup.key = instance;
    pb = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places );

    switch( msg )
    {
    case MO_GET:

	if( pb != NULL )
	    cl_stat = OK;
	break;

    case MO_GETNEXT:

	if( pb != NULL )
	{
	    pb = (GM_PLACE_BLK *) SPfnext( &pb->place_blk );
	}
	else			/* didn't find directly */
	{
	    if( *instance == EOS  )
		pb = (GM_PLACE_BLK *)SPfhead( &GM_globals.gwm_places );
	    else
		break;
	}

	if( pb == NULL )
	    cl_stat = MO_NO_NEXT;
	else
	    cl_stat = MOstrout( MO_INSTANCE_TRUNCATED,
			     pb->place_key, linstance, instance );
	break;

    default:

	cl_stat = MO_BAD_MSG;
	break;
    }

    *instdata = (PTR)pb;

    return( cl_stat );
}



/*{
** Name: GM_dc_conn_instance - decompose connection instance.
**
** Description:
**	Breaks a connection instance string into the component
**	server-instance string, and the numeric connection id,
**	assuming the instance is in the form:
**
**		<server-instance>.<conn-key>
**
**	and <conn-key> is a string of decimal digits.
**
**	Destroys the input instance.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	instance	the instance to decompose.
**
** Outputs:
**	instance	dot replaced with EOS, ending server-instance.
**	conn_key	filled in with numeric conversion of value
**			past the dot.
**
** Returns:
**	none.
**
** History:
**	6-oct-92 (daveb)
**	    created.
**	14-sep-93 (swm)
**	    bug #62664
**          Call CVaptr() rather than CVal() for conn_key to avoid
**	    i4/pointer truncation error.
*/

void
GM_dc_conn_instance( char *instance, PTR *conn_key )
{
    char *p;

    for( p = instance; *p != EOS && *p != '.'; p++ )
	continue;

    if( *p != EOS )
    {
	*p = EOS;
	p++;
    }
    CVaptr( p, conn_key );
}



/*{
** Name: GM_mk_conn_instance - make instance string from srvr & conn key.
**
** Description:
**	Generate an instance string from a server instance and a
**	connection key in the form:
**
**		<server-instance>.<conn-key>
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	srvr		server instance string
**	conn_key	connection block key.
**	linstance	length of output buffer
**
** Outputs:
**	instance	output buffer filled in.
**
** Returns:
**	MO_INSTANCE_TRUNCATED	if instance isn't big enough.
**
** History:
**	6-oct-92 (daveb)
**	    created.
**	14-sep-93 (swm)
**        call MOptrout() rather than MOlongout() for conn_key to
**        avoid i4/pointer truncation error.
**	    
*/

STATUS
GM_mk_conn_instance( PTR srvr_key,
		    PTR conn_key,
		    i4  linstance,
		    char *instance )
{
    i4		len;
    STATUS	cl_stat;

    len = STlength( (char *)srvr_key );
    cl_stat = MOstrout( MO_INSTANCE_TRUNCATED, (char *)srvr_key,
		       linstance, instance );
    if( cl_stat == OK )
    {
	if( len >= linstance - 1 )
	{
	    cl_stat = MO_INSTANCE_TRUNCATED;
	}
	else
	{
	    instance[ len ] = '.';
	    cl_stat = MOptrout( MO_INSTANCE_TRUNCATED, conn_key,
			    linstance - len - 1, &instance[ len + 1 ] );
	}
    }
    return( cl_stat );
}


/*{
** Name:	GM_gt_conn	- locate connection given server and con_key
**
** Description:
**	Lookup a connection block, given the server instance and the
**	conn_key.  If the connection is found, returns holding the
**	semaphore for the place.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	instance	the server instance value.
**	conn_key	the connection key.
**
** Outputs:
**	ppb		ptr to a GM_PLACE_BLK * to fill in.
**	pcb		ptr to a GM_CONN_BLK * to fill in.
**
** Returns:
**	OK		connection found.
**	MO_NO_INSTANCE	not found.
**
** History:
**	6-oct-92 (daveb)
**	    created.
*/
STATUS
GM_gt_conn( char *instance,
	   PTR conn_key,
	   GM_PLACE_BLK **ppb,
	   GM_CONN_BLK **pcb )
{
    GM_SRVR_BLK		*sb;
    GM_CONN_BLK		*cb;
    GM_PLACE_BLK	*pb;
    SPBLK		lookup;

    /* Assert: gwm_place_sem is already held */

    do
    {
	lookup.key = (PTR)instance;
	pb = (GM_PLACE_BLK *)SPlookup( &lookup, &GM_globals.gwm_places );
	if( pb == NULL || (pb)->place_type != GM_PLACE_SRVR )
	    break;
    
	sb = &pb->place_srvr;
	if( OK != GM_gt_sem( &pb->place_sem ))
	    break;
	    
	lookup.key = conn_key;
	cb = (GM_CONN_BLK *)SPlookup( &lookup,
				     &pb->place_srvr.srvr_conns );

	if( cb == NULL )
	    GM_release_sem( &pb->place_sem );
	
    } while ( FALSE );

    *ppb = pb;
    *pcb = cb;
    return( cb != NULL ? OK : MO_NO_INSTANCE );
}



/*{
** Name:	GM_cindex	- index method for connections
**
** Description:
**
**	An index method for looking into the internal place tree,
**	returning only info on places that are servers.  A GET[NEXT]
**	on a VNODE is an error.
**
**	This grabs the gwm_places_sem.
**
**	A connection instance is of the form
**
**		<server-instance>.<connection-timestamp>
**
** Re-entrancy:
**	yes, relying on gwm_places and place_sem to be acquired.
**
** Inputs:
**	msg		MO_GET or MO_GETNEXT.
**	cdata		the classid string.
**	linstance	length of instance buffer.
**	instance	the instance string
**
** Outputs:
**	instance	filled in with new instance on getnext.
**	instdata	stuffed with saved instance data, a GM_PLACE_BLK
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
**	2-oct-92 (daveb)
**	    created
**	18-Nov-1992 (daveb)
**	    Set got_srvr and use it!
*/

STATUS 
GM_cindex(i4 msg,
	  PTR cdata,
	  i4  linstance,
	  char *instance, 
	  PTR *instdata )
{
    STATUS		cl_stat = MO_NO_INSTANCE;
    GM_PLACE_BLK	*pb;
    GM_CONN_BLK		*cb;
    GM_SRVR_BLK		*sb;
    PTR			conn_key;
    i4			got_place = FALSE;
    i4			got_srvr = FALSE;

    GM_dc_conn_instance( instance, &conn_key );

    do
    {
	if( GM_gt_sem( &GM_globals.gwm_places_sem ) != OK )
	    break;
	else
	    got_place = TRUE;
	
	switch( msg )
	{
	    
	case MO_GET:
	    
	    cl_stat = GM_gt_conn( instance, conn_key, &pb, &cb );
	    if( cl_stat == OK )
	    {
		/* assert: pb->place_sem is now held */
		got_srvr = TRUE;
		sb = &pb->place_srvr;
	    }
	    break;
	    
	case MO_GETNEXT:
	    
	    /* empty instance means find first connection */

	    if( *instance == EOS )
	    {
		pb = NULL;
		cb = NULL;
	    }
	    else		/* next after known connection */
	    {
		/* if input instance doesn't exist, NO_INSTANCE */

		cl_stat = GM_gt_conn( instance, conn_key, &pb, &cb );
		if( cl_stat != OK )
		    break;

		got_srvr = TRUE;
		sb = &pb->place_srvr;
	    }

	    do			/* search for next connection */
	    {
		if( got_srvr )
		{
		    GM_release_sem( &pb->place_sem );
		    got_srvr = FALSE;
		}

		/* if we have cb, try next on this server */

		if( cb != NULL )
		    cb = (GM_CONN_BLK *)SPfnext( &cb->conn_blk );
		    
		/* if still null, try next server */
		if( cb == NULL )
		{
		    /* may be null on first call to find first. */

		    if( pb == NULL )
		    {
			pb = (GM_PLACE_BLK *)SPfhead( &GM_globals.gwm_places );
		    }
		    else	/* not initial trip */
		    {
			sb = &pb->place_srvr;
			pb = (GM_PLACE_BLK *)SPfnext( &pb->place_blk );
		    }

		    /* if this is a server, grab sem and then the
		       head of the connection tree. */
		    
		    if( pb != NULL && pb->place_type == GM_PLACE_SRVR )
		    {
			sb = &pb->place_srvr;
			if( GM_gt_sem( &pb->place_sem ) != OK )
			{
			    pb = NULL;
			}
			else
			{
			    got_srvr = TRUE;
			    cb = (GM_CONN_BLK *)SPfhead( &sb->srvr_conns );
			}
		    }
		}

	    } while( cb == NULL && pb != NULL );
	    
	    if( cb == NULL )
	    {
		cl_stat = MO_NO_NEXT;
	    }
	    else
	    {
		cl_stat = GM_mk_conn_instance( pb->place_blk.key,
					      cb->conn_blk.key,
					      linstance, instance );
	    }

	    break;

	default:
	    
	    cl_stat = MO_BAD_MSG;
	    break;
	}
    } while ( FALSE );

    if( cl_stat == OK )
	*instdata = (PTR)cb;

    if( got_place )
	GM_release_sem( &GM_globals.gwm_places_sem );
    if( got_srvr )
	GM_release_sem( &pb->place_sem );

    return( cl_stat );
}


/*{
**  Name:	GM_conn_get - get instance from a connection block.
**
**  Description:
**
**	Convert the integer at the object location to a character
**	string.  The offset is treated as a byte offset to the input
**	object .  They are added together to get the object location.
**	
**	The output userbuf , has a maximum length of luserbuf that will
**	not be exceeded.  If the output is bigger than the buffer, it
**	will be chopped off and MO_VALUE_TRUNCATED returned.
**
**	The objsize comes from the class definition, and is the length
**	of the integer, one of sizeof(i1), sizeof(i2), or
**	sizeof(i4).
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**	23-sep-92 (daveb)
**	    documented
*/

STATUS 
GM_conn_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS cl_stat = OK;

    GM_CONN_BLK *cb = (GM_CONN_BLK *)((char *)object + offset);
    GM_PLACE_BLK *pb = cb->conn_place;

    cl_stat = GM_mk_conn_instance( pb->place_blk.key,
				  cb->conn_blk.key,
				  lsbuf,
				  sbuf );
    return( cl_stat );
}


/*{
** Name:	<func name>	- <short comment>
**
** Description:
**
**  MO get method.  Given pointer to the connect block
**  with the place pointer, return a nice decoded version of the place.
**
**  With this version, a too-long place is truncated with no error.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		byte offset to apply to the object (should be 0)
**	size		ignored
**	object		pointer to the GM_CONN_BLK.
**	lsbuf		length of the output buffer.
**
** Outputs:
**	sbuf		the buffer to fill.
**
** Returns:
**	OK.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/
STATUS 
GM_cplace_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS cl_stat = OK;

    GM_CONN_BLK *cb = (GM_CONN_BLK *)((char *)object + offset);
    GM_PLACE_BLK *pb = cb->conn_place;

    MOstrout( MO_VALUE_TRUNCATED, pb->place_blk.key, lsbuf, sbuf );

    return( cl_stat );
}


/*{
** Name:	GM_srv_get -- get current server.
**
** Description:
**
**  MO get method.  Return the current server.
**
**  With this version, a too-long place is truncated with no error.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	offset		ignored.
**	size		ignored
**	object		ignored.
**	lsbuf		length of the output buffer.
**
** Outputs:
**	sbuf		the buffer to fill.
**
** Returns:
**	OK.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/

STATUS 
GM_srv_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    STATUS cl_stat = OK;

    MOstrout( MO_VALUE_TRUNCATED, GM_my_server(), lsbuf, sbuf );

    return( cl_stat );
}




/*{
**  Name:	GM_sflags_get - get string of server flags settings
**
**  Description:
**
**	Convert the integer at the object location to a string representing
**	it as flags from a GCA register.
**  Inputs:
**	 offset
**		value from the class definition, taken as byte offset to the
**		object pointer where the data is located.
**	 objsize
**		the size of the integer, from the class definition.
**	 object
**		a pointer to the integer to convert.
**	 luserbuf
**		the length of the user buffer.
**	 userbuf
**		the buffer to use for output.
**  Outputs:
**	 userbuf
**		contains the string of the value of the instance.
**  Returns:
**	 OK
**		if the operation succeseded
**	 MO_VALUE_TRUNCATED
**		the output buffer was too small, and the string was truncated.
**	 other
**		conversion-specific failure status as appropriate.
**  History:
**      10-May-1994 (daveb)
**          created.
**      23-Apr-1996 (rajus01)
**          Added GCB (Protocol Bridge) to TRformat.
**	24-Aug-10 (gordy)
**	    GCB bit was added in wrong position.  It is defined in a
**	    part of the MIB mask which was being discarded.  Fixed the
**	    mask/shift to access all current bits, moved GCB to the
**	    correct location and added NMSVR (Name Server Registry)
**	    and GCD bits.
*/

STATUS 
GM_sflags_get( i4  offset, i4  size, PTR object, i4  lsbuf, char *sbuf )
{
    char buf[ 100 ];
    i4 flags = *(i4*)((char *)object + offset);

    /* FIXME wired to GCA values */

    MEfill( sizeof(buf), 0, buf );
    TRformat(NULL, 0, buf, sizeof(buf), "%v",
	    "NMSVR,GCB,GCD,UNUSED,TRAP,INSTALL,ACP,RCP,INGRES,GCC,GCN", (flags & GCA_RG_MIB_MASK) >> 20 );

    return (MOstrout( MO_VALUE_TRUNCATED, buf, lsbuf, sbuf ));
}
