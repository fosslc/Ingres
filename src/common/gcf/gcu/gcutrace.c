/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <gcu.h>

/**
** Name: gcutrace.c
**
** Description:
**	Tracing functions.
**
** History:
**	24-Feb-00 (gordy)
**	    Created.
*/


/*
** Name: gcu_lookup
**
** Description:
**	Searches a list for a matching ID and returns the
**	associated name.  If the ID is not found, the ID
**	is formatted for display in a local static buffer.
**	The final entry in the list must have a NULL name.
**
** Input:
**	list	List containing ID/name pairs.
**	id	ID to lookup.
**
** Output:
**	None.
**
** Returns:
**	char *	Name associated with ID.
**
** History:
**	24-Feb-00 (gordy)
**	    Created.
*/

char *
gcu_lookup( GCULIST *list, i4 id )
{
    static char	buffs[ 4 ][ 32 ];
    static u_i2	rot = 0;
    char	*buff;

    for( ; list->name; list++ )
	if ( id == list->id )  break;

    if ( list->name )
	buff =  list->name;
    else
    {
    	buff = buffs[ rot = (rot + 1) % 4 ];
	STprintf( buff, "[0x%x]", id );
    }

    return( buff );
}


/*
** Name: gcu_hexdump
**
** Description:
**	Traces a buffer in hex and ascii.
**
** Input:
**	buff		Data buffer.
**	len		Data length.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** History:
**	24-Feb-00 (gordy)
**	    Created.
*/

VOID					 
gcu_hexdump( char *buff, i4 len )
{
    char	*hexchar = "0123456789ABCDEF";
    char	hex[ 49 ];
    char	asc[ 17 ]; 
    u_i2	i;

    for( i = 0; len-- > 0; buff++ )
    {
	hex[ 3 * i ] = hexchar[ (*buff >> 4) & 0x0f ];
	hex[ 3 * i + 1 ] = hexchar[ *buff & 0x0f ];
	hex[ 3 * i + 2 ] =  ' ';
	asc[ i++ ] = (CMprint( buff )  &&  ! (*buff & 0x80)) ? *buff : '.' ;

	if ( i == 16  ||  ! len )
	{
	    hex[ 3 * i ] = asc[ i ] = '\0';
	    TRdisplay( "%48s    %s\n", hex, asc );
	    i = 0;
	}
    }

    return;
}

