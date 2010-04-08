/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcu.h>

/*
** Name: gcuget.c - manipulate buffers and sundry tasks
**
** Description:
**	This file contains the routines for composing send buffers
**	and dissecting receive buffers, and a few utility routines.
**
**	gcu_get_str	Get string in GCN_VAL format from buffer
**	gcu_put_str	Put string into buffer in GCN_VAL format
**	gcu_get_nat	Get integer from buffer
**	gcu_put_nat	Put integer into buffer
**	gcu_words	custom STgetwords()
**      gcu_alloc_str   allocate and copy str into buffer
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-Jul-2001 (wansh01) 
**          Added gcu_alloc_str() 
**	01-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
*/


/*
** Name: gcu_get_str
**
** Description:
**	Returns a pointer to a string which has been encoded 
**	in GCN_VAL format.
**
** Input:
**	buff		Current location in buffer.
**
** Output:
**	str		Start of string.
**
** Returns:
**	i4		Amount of buffer used by string.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
*/

i4
gcu_get_str( char *buff, char **str )
{
    i4	len;

    I4ASSIGN_MACRO( *buff, len );

    if ( len )
	*str = buff + sizeof( len );
    else
	*str = "";

    return( sizeof( len ) + len );
}

/*
** Name: gcu_alloc_str() - allocate buffer and copy var string from input.
**
** Description:
**      allocate memory with the size of input buff size.  
**      Copies the string from input buff into the allocated ptr, 
**      null terminating ptr.
**      assign ptr to output str.  
**
** Returns:
**      Amount of buffer used.
**
** History:
**      19-Jul-2001 (wansh01) 
**         created
*/
i4
gcu_alloc_str( char *buff,  char **str)
{
    i4	     len;
    STATUS   status;
    char     *ptr;

    I4ASSIGN_MACRO( *buff, len );

 
        if ((ptr = (char *)MEreqmem(0,len+1, FALSE, &status)) == NULL)
	   return 0;

	MEcopy( buff + sizeof( len ), len, ptr );

	if( !len || buff[ len - 1 ] )
	   ptr[ len ] = '\0';

        *str = ptr; 

	return sizeof( len ) + len;
}


/*
** Name: gcu_put_str
**
** Description:
**	Encodes a string into a buffer in GCN_VAL format.
**
** Input:
**	buff		Current location in buffer.
**	str		String, may be NULL or empty.
**
** Output:
**	None.
**
** Returns:
**	i4		Amount of buffer used by string.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
*/

i4
gcu_put_str( char *buff, char *str )
{
    i4	len;

    if ( ! str || ! *str )
    {
	len = 0;
	I4ASSIGN_MACRO( len, *buff );
    }
    else
    {
	len = (i4)STlength( str ) + 1;
	I4ASSIGN_MACRO( len, *buff );
	MEcopy( str, len, buff + sizeof( len ) );
    }

    return( sizeof( len ) + len );
}


/*
** Name: gcu_get_int
**
** Description:
**	Extracts an integer from a buffer.
**
** Input:
**	buff		Current location in buffer.
**
** Output:
**	value		Integer value.
**
** Returns:
**	i4		Amount of buffer used by integer.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
*/

i4
gcu_get_int( char *buff, i4  *value )
{
    i4	i4val;

    I4ASSIGN_MACRO( *buff, i4val );
    *value = (i4)i4val;

    return( sizeof( i4val ) );
}


/*
** Name: gcu_put_int
**
** Description:
**	Encodes an integer in a buffer.
**
** Input:
**	buff		Current location in buffer.
**	value		Integer value.
**
** Output:
**	None.
**
** Returns:
**	i4		Amount of buffer used by integer.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
*/

i4
gcu_put_int( char *buff, i4  value )
{
    i4	i4val = value;

    I4ASSIGN_MACRO( i4val, *buff );

    return( sizeof( i4val ) );
}


/*
** Name: gcu_words
**
** Description: 
** 	Breaks buffer into null terminated strings at a given delimiter.
**
**	If output buffer not provided, original buffer will be modified 
**	with null terminators for each word.  Otherwise, output buffer
**	must be at least as large as input buffer.
**
** Input:
**	buff		Buffer.
**	delim		Delimiter separating words.
**	size		Maximum number of entries in output words array.
**
** Output:
**	dest		Buffer for resulting words, may be NULL.
**	words		Array of extracted words.
**
** Returns:
**	i4		Number of words.
**
** History
**	26-Feb-97 (gordy)
**	    Extracted from GCN for general usage.
*/

i4
gcu_words( char *buff, char *dest, char **words, i4  delim, i4  size )
{
    char **pv = words;

    if ( ! buff  ||  ! *buff )  return( 0 );

    if ( dest )
    {
	STcopy( buff, dest );
	buff = dest;
    }

    while( size-- )
    {
	*pv++ = buff;

	while( *buff != delim )
	    if ( ! *buff++ )  return( (i4)(pv - words) );

	*buff++ = 0;
    }

    return( (i4)(pv - words) );
}

