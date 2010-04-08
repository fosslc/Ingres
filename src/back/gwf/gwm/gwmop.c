/*
**Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <cs.h>
# include <cv.h>
# include <er.h>
# include <ex.h>
# include <cm.h>
# include <me.h>
# include <sp.h>
# include <st.h>
# include <tm.h>
# include <tr.h>
# include <erglf.h>
# include <mo.h>

# include <gca.h>
# include <scf.h>
# include <adf.h>
# include <dmf.h>
# include <dmrcb.h>
# include <dmtcb.h>
# include <ulf.h>
# include <ulm.h>

# include <gwf.h>
# include <gwfint.h>

# include "gwmint.h"

/*
** Name: gwmop.c	- IMA routines for GM_OPBLKs.
**
** Description:
**
**	GM_OPBLKs are structures that the GWM gateway uses to package
**	up queries and responses that may be handled internally
**	by calls to MO or externally with GCM messages.
**
**	OPBLKs have the following format
**
**	op	GM_GET, GM_GETNEXT, GM_SET, GM_GETRESP, GM_SETRESP
**	status	unspecified (for low levels)
**	used	bytes used in buffer
**	place	where this op is directed, or from.
**	buf	data, as described below.
**
**	OPBLK buffer formats
**
**	    All strings are trimmed and EOS terminated.
**
**	    GM_SET
**
**		{ <classid> EOS <instance> EOS <value> EOS }
**		repeated as many times as will fit.
**
**	    GM_GET | GM_GETNEXT
**
**		{ <classid> EOS <instance> EOS }
**		repeated as many times as will fit.
**
**	    GM_GETRESP
**
**		{ <classid> EOS <instance> EOS <value> EOS perms_nat }
**		repeated as often as may fit.
**
**	    GM_SETRESP
**
**		{ <classid> EOS <instance> EOS }
**
** Defines:
**
**		GM_op		- simple operation with OPBLKs
**		GM_lop		- simple local operation with opblks.
**		GM_rop		- single remote operation, caching results.
**		GM_keyfind	- locate a key in a GM_GETRESP OPBLK
**		GM_pt_and_trim	- copy string, trimming down.
**		GM_i_opblk	- initialize an opblk
**		GM_pt_to_opblk	- general put to an opblk.
**		GM_str_to_opblk - put untrimmed string to opblk.
**		GM_trim_pt_to_opblk - copy trimmed string to opblk.
**		GM_key_compare	- compare two GKEYs
**		GM_tup_put	- put something to tuple, w/len checking.
** 		GM_scanresp	- pick MO object out of opblk.
**		GM_ad_out_col	- copy element to tuple
**		GM_val_dump	- TRdisplay adjacend EOS terminated strings.
**		GM_opblk_dump	- TRdisplay an opblk, for debugging.
**		GM_str_to_uns	- convert string to unsigned.
**
** History:
**	30-Jan-92 (daveb)
**	    extracted from other places to centralize an abstraction.
**	1-Jul-92 (daveb)
**	    Don't set key elements in st_key if inkey_len is 0.  They
**	    may be null pointers.
**	17-sep-92 (daveb)
**	    Split a bunch of stuff out to gwmutil.c; only stuff dealing
**	    with GM_OPBLKS is here.
**	29-sep-92 (daveb)
**	    GM_ad_out_col takes srclen argument for handling
**	    non-terminated buffers.
**	30-sep-92 (daveb)
**	    Handing of NULL attributes in GM_ad_out_col was wrong,
**	    and wouldn't compile in some other places because
**	    DMT_F_NULL is going away.  Ifdef it out for fixing later.
**	1-oct-92 (daveb)
**	    Don't raise exceptions on integer overflow; code above treats
**	    as query fatal.  Instead, force to max value of the object.
**	    This may need to be revisited.
**	9-oct-92 (daveb)
**	    documented better; split GM_op into GM_lop and GM_rop.
**	11-Nov-1992 (daveb)
**	    change interface to GM_op to take RSB, not opblks.
**	    use my_guises() for GM_SET too.
**	13-Nov-1992 (daveb)
**	    simply GM_rop, GM_keyfind using new GM_scanresp to scan
**	    GETRESP block Use correct terminating GM_str_to_opblk instead
**	    of wrong GM_pt_to_opblk.
**	14-Nov-1992 (daveb)
**	    In GM_op, take 0 length place as this server too.
**	18-Nov-1992 (daveb)
**	    If we get CV_OVERFLOW, do unsigned conversion w/o questions.
**	    Otherwise log a better error - usually CV_SYNTAX from passing
**	    a string object to an integer column.
**	19-Nov-1992 (daveb)
**	    include <gca.h> now that we're using GCA_FS_MAX_DATA for the
**	    OPBLK buffer size.  Include other CL headers for func externs.
**	    Make read-ahead work:  10-100x performance win on local scans.
**	20-Nov-1992 (daveb)
**	    Keyfind takes key strings directly now.  Use cursor for
**	    locating cache hit.
**	2-Feb-1993 (daveb)
**	    improve comments.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	19-Aug-1993 (daveb)
**	    Unexpected attribute types are register errors, and should
**	    be decoded to useful strings.  (Was segv-ing on bad column
**	    type anyway.)
**      21-Jan-1994 (daveb) 59125
**          add some tracing
**      24-jan-1994 (gmanning)
**         Change unsigned i4  to u_i4 in order to compile on NT.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in GM_opblk_dump().
**	31-may-1995 (reijo01)
**      Fixed potential bug. The length of the data to move to the
**      tuple buffer was being increased to match the dlen. It should
**      be decreased if it is larger then the dlen.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/

/* forwards */

DB_STATUS
GM_lop( i4  op, GM_RSB *rsb,
      i4  *lsbufp, char *sbuf, i4  *perms, STATUS *cl_stat );

DB_STATUS
GM_rop( i4  op, GM_RSB *rsb,
      i4  *lsbufp, char *sbuf, i4  *perms, STATUS *cl_stat );


static char * GM_attr_type( i4  type );

static void GM_str_to_uns(char *a, u_i8 *u);


/*{
** Name:	GM_op	- simple operation with OPBLKs
**
** Description:
**
**  do single operation for cur_key, and the value in buf directly.
**  Uses OPBLKs handed in for space to do remote operations.
**
**  Called by GOget, GOset, but not by the GX gateway.
**
**  Does NOT log CL errors (might be MO_NO_NEXT).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		GM_GET, GM_GETNEXT, GM_SET
**	rsb.
**	    cur_key	the key in question.
**	    request	the query opblk, uninitialized (used as space).
**	    response	the response opblk, perhaps conatining cached
**			results from a previous call.
**	lsbufp		pointer to length of buffer for output
**	sbuf		buffer of input or for output.
**	perms		pointer to perms location for output.
**	cl_stat		pointer to cl_status for output.
**
** Outputs:
**	rsb.request	may be written.
**	rsb.response	may contain results worth saving (read-ahead).
**	sbuf		value returned on get ops.
**	perms		perms returned on get ops.
**	cl_stat		maybe MO_NO_NEXT if E_DB_ERROR return.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    Documented and broke into two parts, GM_lop() and GM_rop();
**	11-Nov-1992 (daveb)
**	    change args to take rsb instead of opblks and key.
**	14-Nov-1992 (daveb)
**	    Take 0 length place as this server too.
*/
DB_STATUS
GM_op( i4  op, GM_RSB *rsb,
      i4  *lsbufp, char *sbuf, i4  *perms, STATUS *cl_stat )
{
    DB_STATUS db_stat = E_DB_OK;

    GM_GKEY *key = &rsb->cur_key;
    
    if( key->place.len == 0 ||
       !STcompare( key->place.str, GM_my_server() ) ||
       !STcompare( key->place.str, GM_my_vnode() ) )
    {
	db_stat = GM_lop( op, rsb, lsbufp, sbuf, perms, cl_stat );
    }
    else			/* remote */
    {
	db_stat = GM_rop( op, rsb, lsbufp, sbuf, perms, cl_stat );
    }

    return( db_stat );
}


/*{
** Name:	GM_lop	- simple local operation with opblks.
**
** Description:
**
**  Do single operation for cur_key, and the value in buf directly.
**  Leaves opblks alone and use MO to work on the immediate object
**  identified in the key.
**
**  Does NOT log CL errors (might be MO_NO_NEXT).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		GM_GET, GM_GETNEXT, GM_SET
**	rsb.cur_key	the key in question.
**	rsb.request	the query opblk, uninitialized (used as space).
**	rsb.response	the response opblk, perhaps conatining cached
**			results from a previous call.
**	lsbufp		pointer to length of buffer for output
**	sbuf		buffer of input or for output.
**	perms		pointer to perms location for output.
**	cl_stat		pointer to cl_status for output.
**
** Outputs:
**	rsb.request	may be written.
**	rsb.response	may contain results worth saving (read-ahead).
**	sbuf		value returned on get ops.
**	perms		perms returned on get ops.
**	cl_stat		maybe MO_NO_NEXT if E_DB_ERROR return.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    broke out from GM_op().
**	11-Nov-1992 (daveb)
**	    use my_guises() for GM_SET too.
**	    change args to take rsb instead of opblks.
**	24-Nov-1992 (daveb)
**	    take guises from the RSB rather than calculating them all the time.
*/

DB_STATUS
GM_lop( i4  op, GM_RSB *rsb,
      i4  *lsbufp, char *sbuf, i4  *perms, STATUS *cl_stat )
{
    DB_STATUS db_stat = E_DB_OK;
    
    GM_GKEY *key = &rsb->cur_key;
    
    if( op == GM_GET )
    {
	*cl_stat = MOget( rsb->guises, key->classid.str, key->instance.str,
			 lsbufp, sbuf, perms );
    }
    else if( op == GM_GETNEXT )
    {
	*cl_stat = MOgetnext( rsb->guises, sizeof( key->classid.str ),
			     sizeof( key->instance.str ),
			     key->classid.str,
			     key->instance.str,
			     lsbufp, sbuf, perms );

	key->classid.len = STlength( key->classid.str );
	key->instance.len = STlength( key->instance.str );
    }
    else if( op == GM_SET )
    {
	*cl_stat = MOset( rsb->guises,
			 key->classid.str, key->instance.str, sbuf );
    }
    
    if( *cl_stat != OK )
	db_stat = E_DB_ERROR;

    return( db_stat );
}


/*{
** Name:	GM_rop	- single remote operation, caching results.
**
** Description:
**
**  Do single operation for cur_key, and the value in buf directly.
**  Uses the OPBLKs provided to rig up a message for GCM, and to
**  store results.  GM_GET and GM_GETNEXT queries try to get
**  as many subsequent objects as fit, in hopes of making scans
**  go faster.  If the object requested is alre.ady in the response
**  buffer, then get it from there instead of making a new remote query.
**
**  It might be good to give caches time-to-live fields, and to do
**  them in more global fashion.  Global handling is touchy because 
**  different session may have different guises.  TTL might not be
**  needed; we expect caches to vanish on their own pretty quickly.
**
**  Does NOT log CL errors (might be MO_NO_NEXT).
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		GM_GET, GM_GETNEXT, GM_SET
**	key		the key in question.
**	request		the query opblk, uninitialized (used as space).
**	response	the response opblk, perhaps conatining cached
**			results from a previous call.
**	lsbufp		pointer to length of buffer for output
**	sbuf		buffer of input or for output.
**	perms		pointer to perms location for output.
**	cl_stat		pointer to cl_status for output.
**
** Outputs:
**	request		may be written.
**	response	may contain results worth saving (read-ahead).
**	sbuf		value returned on get ops.
**	perms		perms returned on get ops.
**	cl_stat		maybe MO_NO_NEXT if E_DB_ERROR return.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	8-oct-92 (daveb)
**	    broke out from GM_op().
**	11-Nov-1992 (daveb)
**	    change args to take rsb instead of opblks and key.
**	13-Nov-1992 (daveb)
**	    Simply using new GM_scanresp to scan GETRESP block
**	    Use correct terminating GM_str_to_opblk instead of
**	    wrong GM_pt_to_opblk.
**	19-Nov-1992 (daveb)
**	    Make read ahead work.
*/

DB_STATUS
GM_rop( i4  op, GM_RSB *rsb,
      i4  *lsbufp, char *sbuf, i4  *perms, STATUS *cl_stat )
{
    DB_STATUS	db_stat = E_DB_OK;
    GM_GKEY	*key = &rsb->cur_key;
    GM_OPBLK	*request = &rsb->request;
    GM_OPBLK	*response = &rsb->response;
    char	*lastloc = &response->buf[ response->used ];
    char	*loc;
    char	*classid;
    char	*instance;
    char	*value;
    i4		element;

    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_op_rop_calls );

    if( op == GM_SET )
    {
	GM_i_opblk( op, key->place.str, request );
	if( (db_stat = GM_str_to_opblk( request,
				      key->classid.len,
				      key->classid.str )) == E_DB_OK &&
	   (db_stat = GM_str_to_opblk( request,
				     key->instance.len,
				     key->instance.str )) == E_DB_OK &&
	   (db_stat = GM_str_to_opblk( request,
				     STlength(sbuf), sbuf )) == E_DB_OK )
	{
	    db_stat = GM_gcm_msg( rsb, cl_stat );
	}
    }
    else if( op == GM_GET || op == GM_GETNEXT )
    {
	loc = NULL;
	
	/* check cached results from last response */
	
	if( response->status == OK &&
	   response->op == GM_GETRESP &&
	   STequal( request->place, response->place ) &&
	   GM_globals.gwm_gcm_usecache != 0 )
	{
	    loc = GM_keyfind( key->classid.str, key->instance.str,
			     response, &element );
	    if( loc != NULL )
	    {
		/* scan this one... */
		GM_scanresp( &loc, &classid, &instance, &value, perms );
		if( loc >= lastloc )
		    loc = NULL;
		else if( request->op == GM_GETNEXT )
		{
		    element++;
		    GM_scanresp( &loc, &classid, &instance, &value, perms );
		}

		/* if this was the last, we need more */
		if( loc >= lastloc )
		    loc = NULL;
		else
		    GM_incr( &GM_globals.gwm_stat_sem,
			    &GM_globals.gwm_op_rop_hits );
	    }
	}
	
	/* if not found, then get new data from place */
	
	if( loc == NULL )
	{
	    GM_incr( &GM_globals.gwm_stat_sem,
		    &GM_globals.gwm_op_rop_misses );

	    GM_i_opblk( op, key->place.str, request );
	    if( (db_stat = GM_str_to_opblk( request,
					  key->classid.len,
					  key->classid.str )) == E_DB_OK &&
	       (db_stat = GM_str_to_opblk( request,
					 key->instance.len,
					 key->instance.str )) == E_DB_OK )
	    {
		db_stat = GM_gcm_msg( rsb, cl_stat );
	    }
	    
	    /* if request went OK, answer is first thing in buf */
	    
	    if( db_stat == E_DB_OK && response->op == GM_GETRESP )
	    {
		loc = response->buf;
		GM_scanresp( &loc, &classid, &instance, &value, perms );
	    }
	}

	/* now we've got it, or we're dead. */

	if( loc == NULL )
	{
	    if( *cl_stat != MO_NO_NEXT &&
	       *cl_stat != MO_NO_CLASSID &&
	       *cl_stat != MO_NO_INSTANCE )
	    {
		/* "GWM: Internal error:  unexpected error in GM_rop:" */
		GM_error( E_GW81C1_ROP_ERROR );
		GM_error( *cl_stat );
	    }
	    db_stat = E_DB_ERROR;
	}
	else		/* answer is scanned */
	{
	    if( op == GM_GETNEXT )	/* update cur_key in key */
	    {
		STcopy( classid, key->classid.str );
		key->classid.len = STlength( key->classid.str );

		STcopy( instance, key->instance.str );
		key->instance.len = STlength( key->instance.str );
	    }

	    /* set value (perms is already set) */
	    *cl_stat = MOstrout( MO_VALUE_TRUNCATED, value, *lsbufp, sbuf );
	}
    }

    if( db_stat != E_DB_OK || *cl_stat != OK )
	GM_breakpoint();

    return( db_stat );
}


/*{
** Name:	GM_keyfind	- locate a key in a GM_GETRESP OPBLK
**
** Description:
**	Scan a GETRESP OPBLK for the specified key, and return it's
**	location in the buffer, or NULL if not found.  This is used
**	to locate read-ahead objects in GM_rop.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	kclassid	the classid to find.
**	kinstance	the instance.
**	response	to search
**	    .cursor		the start location.
**	    .curs_element	the start element number.
**
** Outputs:
**	element		the number of the match returned.
**
** Returns:
**	location in buf of the key object, or NULL.
**
** History:
**	8-oct-92 (daveb)
**	    documented.
**	13-Nov-1992 (daveb)
**	    simplify using net GM_scanresp.
**	19-Nov-1992 (daveb)
**	    pass keys directly, use cursor to root search.
*/

char *
GM_keyfind( char *kclassid, char *kinstance, GM_OPBLK *response, i4  *element )
{
    char *loc = response->cursor;
    char *lastloc = &response->buf[ response->used ]; 
    char *ret_loc = NULL;
    char *classid;
    char *instance;
    char *value;
    i4  perms;
    i4	i;
    
    for( i = response->curs_element;
	i < response->elements && loc < lastloc; i++ )
    {
	GM_scanresp( &loc, &classid, &instance, &value, &perms );
	if( STequal( kclassid, classid ) && STequal( kinstance, instance ) )
	{
	    *element = i;
	    ret_loc = classid;
	    break;
	}
    }
    return( ret_loc );
}


/*{
** Name:	GM_pt_and_trim	- copy string, trimming down.
**
** Description:
**
** Copy string to limited dest, zapping trailing blanks.
** Return error if it won't fit.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	str		source string
**	space		space in the out buffer.
**
** Outputs:
**	bufp		pointer to the buffer pointer, moved
**			to next available place in the buffer
**
** Returns:
**	E_DB_OK		if OK
**	E_DB_ERROR	if no more room in the buffer.
**
** History:
**	8-oct-92 (daveb)
**	    documented.
*/

DB_STATUS
GM_pt_and_trim( char *str, i4  space, char **bufp )
{
    char *p;
    char *bp = *bufp;
    DB_STATUS db_stat = E_DB_ERROR;

    for( p = str; *p; p++ )
	continue;
    while( --p >= str && CMwhite( p ) )
	continue;
    if( (++p - str) < space )
    {
	while( str < p )
	    *bp++ = *str++;
	*bp++ = EOS;
	*bufp = bp;
	db_stat = E_DB_OK;
    }
    return( db_stat );
}


/* ----------------------------------------------------------------
**
** general OPBLK related
*/

/*{
** Name:	GM_i_opblk	- initialize an opblk
**
** Description:
**
**	Set up the opblk with the specified operation and place,
**	with an empty buffer.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		GM_GET|GM_GETNEXT|GM_SET|GM_GETRESP|GM_SETRESP
**	place		string of the place to address opblk to.
**
** Outputs:
**	gwm_opblk	the opblk is initialized.
**
** Returns:
**	none.	
**
** History:
**	8-oct-92 (daveb)
**	    documented.
**	17-Nov-1992 (daveb)
**	    initialize more stuff.
**	19-Nov-1992 (daveb)
**	    setup cursor, for cache location.
*/

VOID
GM_i_opblk( i4  op, char *place, GM_OPBLK *opblk )
{
    opblk->op = op;
    opblk->status = OK;
    opblk->elements = 0;
    opblk->err_element = -1;
    opblk->curs_element = 0;
    opblk->cursor = opblk->buf;
    opblk->used = 0;
    opblk->buf[0] = EOS;
    STcopy( place, opblk->place );
}


/*{
** Name:	GM_pt_to_opblk	- general put to an opblk.
**
** Description:
**	Put something to an opblk buffer, giving error on out of room.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	block		the opblk to write.
**	len		the length of the object to put.
**	str		the object to put, exactly len bytes,
**			EOS is ignored.
**
** Outputs:
**	block		written.
**
** Returns:
**	E_DB_OK		if it fit.
**	E_DB_ERROR	if it didn't.
**
** History:
**	8-oct-92 (daveb)
**	    documented.
*/


DB_STATUS
GM_pt_to_opblk( GM_OPBLK *block, i4  len, char *str )
{
    if( len > (sizeof(block->buf) - block->used ) )
	return( E_DB_ERROR );
    MEcopy( str, len + 1, &block->buf[ block->used ] );
    block->used += len;
    return( E_DB_OK );
}



/*{
** Name:	GM_str_to_opblk - put untrimmed string to opblk.
**
** Description:
**
**	put null term string, untrimmed, to block.  len is STlength(str)
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	block		the block to write.
**	len		the length of the string.
**	str		the string to put.
**
** Outputs:
**	block		is written.
**
** Returns:
**	E_DB_OK		if it fit.
**	E_DB_ERROR	if it didn't.
**
** History:
**	8-oct-92 (daveb)
**	    documented.
*/

DB_STATUS
GM_str_to_opblk( GM_OPBLK *block, i4  len, char *str )
{
    char *dest = &block->buf[ block->used ];

    /* account for EOS */
    len++;
    if( len > (sizeof(block->buf) - block->used ) )
	return( E_DB_ERROR );

    MEcopy( (PTR)str, len, dest );
    block->used += len;
    return( E_DB_OK );
}



/*{
** Name:	GM_trim_pt_to_opblk - copy trimmed string to opblk.
**
** Description:
**
*** trim string and put it to an GM_OPBLK, checking for out of room.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	block		the block to write.
**	len		length of the string, untrimmed.
**	str		the string to write.
**
** Outputs:
**	block		is written.
**
** Returns:
**	E_DB_OK		if it fit.
**	E_DB_ERROR	if it didn't
**
** History:
**	8-oct-92 (daveb)
**	    documented.
*/
DB_STATUS
GM_trim_pt_to_opblk( GM_OPBLK *block, i4  len, char *str )
{
    DB_STATUS db_stat;
    char *p;

    p = &block->buf[ block->used ];
    db_stat = GM_pt_and_trim( str, sizeof(block->buf) - block->used, &p );
    block->used = p - block->buf;
	   
    return( db_stat );
}


/*{
** Name:	GM_key_compare	- compare two GKEYs
**
** Description:
**	Do lexical comparison of two GKEYS
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	this		one key
**	that		another
**
** Outputs:
**	none
**
** Returns:
**	<0, 0 or >0, depending on ordering of the two keys.
**
** History:
**	16-sep-92 (DaveB)
**		Only compare classid, instance if that has some
**		length; otherwise say thay are equal.  This lets
**		Scans work where this is {thing.xxxxx} and that
**		is for { thing. } with no instance.
**	9-oct-92 (daveb)
**	    documented.
*/

i4
GM_key_compare( GM_GKEY *this, GM_GKEY *that )
{
    i4  ret_val;

    ret_val = STcompare( this->place.str, that->place.str );

    if( ret_val == 0 && that->classid.len != 0 )
	ret_val = STcompare( this->classid.str, that->classid.str );

    if( ret_val == 0 && that->instance.len != 0 )
	ret_val = STcompare( this->instance.str, that->instance.str );

    return( ret_val );
}


/*{
** Name:	GM_tup_put	put something to tuple, w/len checking.
**
** Description:
**	Puts the sized source object to a length specified destination.
**	blank padding the dest at the end.  Checks for enough space,
**	a updating client location of the used space.
**
**	Returns error if there isn't
**	enough room left, with nothing written to the buffer.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	slen		length of the source.
**	src		the source object.
**	dlen		the required dest length.
**	tuplen		maximum total size of the dest.
**	tuple		the beginning of the tuple.
**	tup_used	pointer to index into tuple of where to write.
**
** Outputs:
**	tuple		buffer is written
**	tup_used	updated to next available.
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR	if no room.
**
** History:
**	1-Jul-92 (daveb)
**	    Take address of tuple[ *tup_used ] for MEmove.
**	17-jul-92 (daveb)
**	    Truncate slen to dlen, don't error on slen too big, but
**	    dlen too big, which is a consistency error.  Hope this fixes
**	    problems where data is wider than the declared column; want
**	    to just truncate it with no error.
**	31-may-1995 (reijo01)
**      Fixed potential bug. The length of the data to move to the
**      tuple buffer was being increased to match the dlen. It should
**      be decreased if it is larger then the dlen.
*/

DB_STATUS
GM_tup_put( i4  slen, PTR src, i4  dlen,
	    i4  tuplen, char *tuple, i4  *tup_used )
{
    DB_STATUS db_stat = E_DB_OK;

    if( dlen > (tuplen - *tup_used) )
    {
	db_stat= E_DB_ERROR;
    }
    else
    {
	if( slen > dlen )
	    slen = dlen;
	MEmove( slen, src, ' ', dlen, &tuple[ *tup_used ] );
	*tup_used += dlen;
    }
    return( db_stat );
}



/*{
** Name:	GM_scanresp - pick MO object out of opblk.
**
** Description:
**	Given pointer to the GETRESP opblk buffer, load pointers to
**	the parts of the MO object it contains.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	srcp		buffer pointer, updated.
**	classid		pointer to stuff.
**	instance	pointer to stuff.
**	value		pointer to stuff.
**	perms		pointer to stuff with perms value
**
** Outputs:
**	srcp		updated past this element.
**	classid		filled with ptr to classid.
**	instance	filled with ptr to instance.
**	value		filled with ptr to value.
**	perms		filled with perms.
**
** Returns:
**	void.
**
** History:
**	13-Nov-1992 (daveb)
**	    created.
*/

void
GM_scanresp( char **srcp,
	     char **classid, char **instance, char **value, i4  *perms )
{
    char *p = *srcp;

    *classid = p;
    while( *p++ )
	;
    *instance = p;
    while( *p++ )
	;
    *value = p;
    while( *p++ )
	;
    MECOPY_CONST_MACRO( (PTR)p, sizeof(*perms), (PTR)perms );
    p += sizeof( *perms );
    *srcp = p;
}





/*{
**
** Name:  GM_ad_out_col - copy element to tuple
**
** Inputs:
**
**    src	pointer to next string to put into the tuple.
**    srclen	length of source if non-terminated string, otherwise 0.
**    col	column in the output to produce, starting from 0.
**    gwm_rsb	open instance
**	.ncols	number of columns in the table
**	.gwm_att_list	attributes of the tuple
**	        .gwat_att
**			.att_name
**			.att_number	order in table
**			.att_offset	offset in tuple
**			.att_type	type (DB_xxx_TYPE)
**			.att_width	width in tuple
**			.att_prec	precision, if applicable.
**			.att_flags	DMT_F_*
**			.att_key_seq_number
**		.gwat_xatt
**			.data_address	where it is
**			.data_in_size	xattlen.
**
**	tuple
**	tup_used;
**
**    Outputs
**	.tuple		written.
**	.tup_cols	updated.
**	.tup_used	updated.
**	.tup_perms	updated.
**
** History:
**	whenever (daveb)
**		Created.
**	1-jul-92 (daveb).
**		Find attributes in the right place, give right args to
**		GM_tup_put in one case, add default output type error,
**	16-jul-92 (daveb)
**		Handle DB_CHA_TYPE too.  Truncate len to dlen before
**		writing the length descriptor.
**	17-jul-92 (daveb)
**		On varchar, check for dlen - 2 to account for descriptor
**		space.  For varchar(8), we're handed a 10 byte buffer,
**		and we'd better only write 8 chars.
**	4-aug-92 (daveb)
**		Log CL errors.
**	29-sep-92 (daveb)
**		Take srclen argument for handling non-terminated buffers.
**	30-sep-92 (daveb)
**	    Handing of NULL attributes in GM_ad_out_col was wrong,
**	    and wouldn't compile in some other places because
**	    DMT_F_NULL is going away.  Ifdef it out for fixing later.
**	1-oct-92 (daveb)
**	    Don't raise exceptions on integer overflow; code above treats
**	    as query fatal.  Instead, force to max value of the object.
**	    This may need to be revisited.
**	18-Nov-1992 (daveb)
**	    If we get CV_OVERFLOW, do unsigned conversion w/o questions.
**	    Otherwise log a better error - usually CV_SYNTAX from passing
**	    a string object to an integer column.
**	19-Aug-1993 (daveb)
**	    Unexpected attribute types are register errors, and should
**	    be decoded to useful strings.  (Was segv-ing on bad column
**	    type anyway.)  Treat CV conversion as register error too,
**	    not internal.
**      21-Jan-1994 (daveb) 59125
**          add some tracing.
**      24-jan-1994 (gmanning)
**          Change unsigned i4  to u_i4 in order to compile on NT.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Added handling of 8-byte integers.
*/
DB_STATUS
GM_ad_out_col( char *src,
	      i4  srclen,
	      i4  col,
	      GM_RSB *gwm_rsb,
	      i4  tup_len, 
	      char *tuple,
	      i4  *tup_used )
{
    DB_STATUS		db_stat = E_DB_OK;
    STATUS		cl_stat = OK;
    DMT_ATT_ENTRY	*dmt_att;
    GM_ATT_ENTRY	*gwm_att;
    i8			ival;
    i2			len;
    i8			val_i8;
    i4			val_i4;
    i2			val_i2;
    i1			val_i1;

    PTR			lsrc;
    i4			lsrclen;
    i4			dlen;

    if( GM_globals.gwm_trace_gets )
	TRdisplay("  adding '%s' to output row\n", src );

    do
    {
	gwm_att = &gwm_rsb->gwm_gwm_atts[ col ];
	dmt_att = gwm_att->gma_dmt_att;
	if( dmt_att == NULL )
	{
	    db_stat = E_DB_ERROR;
	    break;
	}

	/* Now put the column */
	
	dlen = dmt_att->att_width;
	switch( dmt_att->att_type )
	{
	case DB_INT_TYPE:	/* 30 'i' */
	    if (dlen != 8 && dlen !=4 && dlen != 2 && dlen != 1)
	    {
		/* If it's not an integer display of 1, 2, 4, or 8 bytes width, then error */
		/* "GWM: Internal error:  converting string %0c to
		    decimal from object %1c for column %2c" */

		GM_3error( (GWX_RCB *)0, E_GW81C2_CVAL_ERR,
		    GM_ERR_LOG|GM_ERR_USER,
		    0, (PTR)src,
		    GM_dbslen( gwm_att->gma_col_classid ),
		    (PTR)gwm_att->gma_col_classid,
		    GM_dbslen( dmt_att->att_name.db_att_name ),
		    (PTR)&dmt_att->att_name );
		GM_error( cl_stat );
		db_stat = E_DB_ERROR;
		break;
	    }
	    if ( (cl_stat = CVal8( src, &ival )) != OK )
	    {
		if( cl_stat == CV_OVERFLOW )
		{
		    GM_str_to_uns( src, (u_i8 *)&ival );
		}
		else
		{
		    /* "GWM: Internal error:  converting string %0c to
		        decimal from object %1c for column %2c" */

		    GM_3error( (GWX_RCB *)0, E_GW81C2_CVAL_ERR,
			      GM_ERR_LOG|GM_ERR_USER,
			      0, (PTR)src,
			      GM_dbslen( gwm_att->gma_col_classid ),
			      (PTR)gwm_att->gma_col_classid,
			      GM_dbslen( dmt_att->att_name.db_att_name ),
			      (PTR)&dmt_att->att_name );
		    GM_error( cl_stat );
		    db_stat = E_DB_ERROR;
		    break;
		}
	    }
	    switch (dlen)
	    {
		case 8:
		{
		    if( ival > MAXI8 )
			ival = MAXI8;
		    else if( ival < MINI8 )
			ival = MINI8;
		    val_i8 = ival;
		    lsrc = (PTR) &val_i8;
		    lsrclen = sizeof(val_i8);
		    break;
		}
		case 4:
	    {
		    if( ival > MAXI4 )
			ival = MAXI4;
		    else if( ival < MINI4 )
			ival = MINI4;
		val_i4 = ival;
		lsrc = (PTR) &val_i4;
		lsrclen = sizeof(val_i4);
		    break;
	    }
		case 2:
	    {
		if( ival > MAXI2 )
		    ival = MAXI2;
		else if( ival < MINI2 )
		    ival = MINI2;
		val_i2 = (i2)ival;
		lsrc = (PTR) &val_i2;
		lsrclen = sizeof(val_i2);
		    break;
	    }
		case 1:
	    {
		if( ival > MAXI1 )
		    ival = MAXI1;
		else if ( ival < MINI1 )
		    ival = MINI1;
		val_i1 = ival;
		lsrc = (PTR) &val_i1;
		lsrclen = sizeof(val_i1);
		    break;
		}
	    }

	    db_stat = GM_tup_put( lsrclen, lsrc, dlen, tup_len,
				  tuple, tup_used );
	    break;

	case DB_CHA_TYPE:	/* 20 */

	    if( srclen )
		len = srclen;
	    else
		len = STlength( src );
	    if( len > dlen )
		len = dlen;

	    /* put what there is first... */
	    db_stat = GM_tup_put( len, src, len, tup_len, tuple, tup_used );
	    if( db_stat == E_DB_OK )
	    {
		/* consistency check */
		if( (dlen - len) > (tup_len - *tup_used) )
		{
		    db_stat = E_DB_ERROR;
		    break;
		}

		/* then blank pad what is left */
		while( len++ < dlen )
		    tuple[ (*tup_used)++ ] = ' ';
	    }
	    break;

	case DB_VCH_TYPE:	/* 21 */

	    dlen -= 2;		/* reserve descriptor space */
	    if( srclen )
		len = srclen;
	    else
		len = STlength( src );
	    if( len > dlen )
		len = dlen;

	    /* put the length descriptor */
	    db_stat = GM_tup_put( sizeof(len), (char *)&len, 2,
			      tup_len, tuple, tup_used );

	    /* then the data of that length */
	    if( db_stat == E_DB_OK )
		db_stat = GM_tup_put( len, src, dlen,
				  tup_len, tuple, tup_used );
	    break;

	default:

	    /* "GWM error:  unsupported attribute type %0c
	       for column %1c" */

	    cl_stat = E_GW81C3_UNEXPECTED_ATTR_TYPE;
	    GM_2error( (GWX_RCB *)0, cl_stat,
		      GM_ERR_USER|GM_ERR_LOG,
		      0, (PTR)GM_attr_type( dmt_att->att_type ),
		      GM_dbslen( dmt_att->att_name.db_att_name ),
		      (PTR)&dmt_att->att_name );
	    db_stat = E_DB_ERROR;
	    break;
	}
	if( db_stat != E_DB_OK )
	    break;

    } while( FALSE );

    return( db_stat );
}


/*{
** Name:	GM_attr_type	-- decode dmt atttribute to a string.
**
** Description:
**	given a dmt attribute, return a string for an error message.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	type	the attribute type
**
** Outputs:
**	none.
**
** Returns:
**	pointer to a character string.
**
** History:
**	19-Aug-1993 (daveb)
**	    created
**	05-Oct-2006 (gupsh01)
**	    DB_DTE_TYPE is now ingresdate.
*/

static char *
GM_attr_type( i4  type )
{
    char *t;
    switch( type )
    {
    case DB_NODT:		t = ERx("no datatype"); break;
    case DB_INT_TYPE:		t = ERx("integer"); break;
    case DB_FLT_TYPE:		t = ERx("float"); break;
    case DB_CHR_TYPE:		t = ERx("chr"); break;
    case DB_TXT_TYPE:		t = ERx("text"); break;
    case DB_DTE_TYPE:		t = ERx("ingresdate"); break;
    case DB_MNY_TYPE:		t = ERx("money"); break;
    case DB_DEC_TYPE:		t = ERx("decimal"); break;
    case DB_LOGKEY_TYPE:	t = ERx("logical key"); break;
    case DB_TABKEY_TYPE:	t = ERx("table key"); break;
    case DB_BIT_TYPE:		t = ERx("bit"); break;
    case DB_VBIT_TYPE:		t = ERx("bit varying"); break;
    case DB_LBIT_TYPE:		t = ERx("long bit"); break;
    case DB_CPN_TYPE:		t = ERx("coupon"); break;
    case DB_CHA_TYPE:		t = ERx("char"); break;
    case DB_VCH_TYPE:		t = ERx("varchar"); break;
    case DB_LVCH_TYPE:		t = ERx("long varchar"); break;
    case DB_BYTE_TYPE:		t = ERx("byte"); break;
    case DB_VBYTE_TYPE:		t = ERx("byte varying"); break;
    case DB_LBYTE_TYPE:		t = ERx("long byte"); break;
    default:			t = "unknown type"; break;
    }
    return( t );
}



/*----------------------------------------------------------------
**
** Debug routines
*/

/*{
** Name:	GM_val_dump	- TRdisplay adjacend EOS terminated strings.
**
** Description:
**	Subroutine for dumping opblk contents as strings.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	n		number of strings to dump
**	buf		buffer containing the strings
**	index		ptr to offset into buffer of the first string.
**
** Outputs:
**	index		updated past strings written.
**
** Returns:
**	none.
**
** History:
**	9-oct-92 (daveb)
**	    documented.
**	12-Nov-1992 (daveb)
**	    take \n out of here so we can do getresp right.
**	20-Nov-1992 (daveb)
**	    print element number, tab elements.
*/

static void
GM_val_dump( i4  rownum, i4  n, char *buf, i4  *indexp )
{
    char *where;

    TRdisplay("\t%d\t", rownum );
    while( n-- )
    {
	where = &buf[ *indexp ];
	TRdisplay( ERx("%s\t"), where ? *where ?
		  where : ERx("<empty>") : ERx("<NULL>") );
	*indexp += STlength( where ) + 1;
    }
}


/*{
** Name:	GM_opblk_type - string of opblock operation code
**
** Description:
**	Debug routine to decode the operation code.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	op		the opcode.
**
** Outputs:
**	none.
**
** Returns:
**	string of the op code.
**
** History:
**	2-Feb-1993 (daveb)
**	    documented.
*/

char *
GM_opblk_type( i4  op )
{
    switch( op )
    {
    case GM_BADOP:
	return(ERx("GM_BADOP (0)"));
    case GM_GET:
    	return(ERx("GM_GET"));
    case GM_GETNEXT:
	return(ERx("GM_GETNEXT"));
    case GM_SET:
	return(ERx("GM_SET"));
    case GM_GETRESP:
	return(ERx("GM_GETRESP"));
    case GM_SETRESP:
	return(ERx("GM_SETRESP"));
    default:
	return(ERx("(unknown op)"));
    }
}

/*{
** Name:	GM_opblk_dump	- TRdisplay an opblk, for debugging.
**
** Description:
**	Called from a debugger; dumps to server log with TRdisplay.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	opblk		what to dump.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	9-oct-92 (daveb)
**	    documented.
**	12-Nov-1992 (daveb)
**	    decode getresp correctly.
**	16-Nov-1992 (daveb)
**	    decode op and status, show err_element.
**	20-Nov-1992 (daveb)
**	    decode cursor, don't print if pointers if it looks nuts.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/

void
GM_opblk_dump( GM_OPBLK *opblk )
{
    i4  i;
    i4  perms;
    i4  e;

    /* print stuff that is safe */

    TRdisplay( ERx("opblk %x:\n\top %d (%s)\n\tstatus %d (%s)\n"),
	      opblk, opblk->op, GM_opblk_type( opblk->op ),
	      opblk->status,
	      opblk->status == OK ? ERx("OK") : ERget( opblk->status ));

    TRdisplay( ERx("\telements %d, err_element %d, curs_element %d, used %d\n"),
	      opblk->elements,
	      opblk->err_element,
	      opblk->curs_element,
	      opblk->used );

    /* stop before printing wild pointers */

    if( opblk->elements > 100			||
       opblk->used > sizeof(opblk->buf)		||
       opblk->cursor < opblk->buf		||
       opblk->cursor > &opblk->buf[ sizeof(opblk->buf) ] )
    {
	TRdisplay(ERx("\t[insane values in opblk]\n"));
	return;
    }

    TRdisplay( ERx("\tplace %s, cursor %x %s\n"),
	      opblk->place,
	      opblk->cursor, opblk->cursor );

    switch( opblk->op )
    {
    case GM_GET:
    case GM_GETNEXT:

	TRdisplay(ERx("\tGET/NEXT block, {classid,  instance}\n"));
	for( e = i = 0; i < opblk->used && e < opblk->elements; e++ )
	    GM_val_dump( e, 2, opblk->buf, &i );
	TRdisplay(ERx("\n"));
	break;

    case GM_SET:

	TRdisplay(ERx("\tSET block, {classid, instance, value}\n"));
	for( e = i = 0; i < opblk->used && e < opblk->elements; e++ )
	    GM_val_dump( e, 3, opblk->buf, &i );
	TRdisplay(ERx("\n"));
	break;

    case GM_GETRESP:

	TRdisplay(ERx("\tGETRESP block, {classid, instance, value, perms}\n"));
	for( e = i = 0; i < opblk->used && e < opblk->elements; e++ )
	{
	    GM_val_dump( e, 3, opblk->buf, &i );
	    MEcopy( (PTR)&opblk->buf[ i ], sizeof(perms), (PTR)&perms );
	    i += sizeof(perms);
	    TRdisplay(ERx("%d\n"), perms );
	}
	break;

    case GM_SETRESP:

	TRdisplay(ERx("\tSETRESP block, {classid, instance, status}\n"));
	for( e = i = 0; i < opblk->used && e < opblk->elements; e++ )
	    GM_val_dump( e, 3, opblk->buf, &i );
	TRdisplay(ERx("\n"));
	break;

    default:
	TRdisplay(ERx("\tBad op:  as memory dump:\n"));
	GM_dumpmem( (PTR)opblk, 128 );
	break;
    }
}


/*{
** Name: GM_str_to_uns	- string to unsigned conversion.
**
** Description:
**	Convert a string to an unsigned long.  Curiously, we don't
**	have one of these in the CL.  No overflow checking is done.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	a	string to convert
**
** Outputs:
**	u	pointer to an unsigned i4 to stuff.
**
** Returns:
**	none
**
** History:
**	18-Nov-1992 (daveb)
**	    swiped from MO_str_to_uns.  This should be in the GL/CL.
**      09-Feb-2010 (smeke01) b123226, b113797 
**          Amended for handling of 8-byte integers.
*/

static void
GM_str_to_uns(char *a, u_i8 *u)
{
    register u_i8	x; /* holds the integer being formed */

    /* skip leading blanks */

    while ( *a == ' ' )
	a++;

    /* at this point everything had better be numeric */

    for (x = 0; *a <= '9' && *a >= '0'; a++)
	x = x * 10 + (*a - '0');

    *u = x;
}

