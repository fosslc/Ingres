/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <gc.h>
#include <mu.h>
#include <st.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcu.h>

/*
**
** Name: gcucode.c
**
** Description:
**	GCF utility functions for encryption.
**
**          gcu_encode		Encrypt a string.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
*/

/*
** CI requires string lengths to be a multiple of
** a base block size.  The CL spec defines this to
** be CRYPT_SIZE, but this is not defined in CL
** header files.  CI defines the value locally
** and also uses hard coded values.
*/
# define CRYPT_SIZE		8

# define GCN_PASSWD_END		'0'



/*
** Name: gcu_encode
**
** Description:
**	This routine encrypts a string.  Uses the inverse algorithm
**	of gcn_decrypt() so that passwords encrypted using this
**	routine are acceptable for Name Server usage.
**
**	An output buffer length of (input_len + 8) * 2 is required.
**	This length is larger than actually needed but is a simplified
**	calculation of the encryption length.
**
** Inputs:
**	key		Encryption key.
**	str		String to encrypt.
**
** Outputs:
**	buff		Encrypted string.
**
** Returns:
**	STATUS		OK or FAIL.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted from GCN.
**	21-Jul-09 (gordy)
**	    Declare default sized buffer.  Use dynamic storage if
**	    actual lengths exceed default.
*/

STATUS
gcu_encode( char *key, char *str, char *buff )
{
    CI_KS	ksch;
    char	kbuff[ CRYPT_SIZE + 1 ];
    char	tbuff[ 128 ];
    char	*tmp;
    i4		len;

    /*
    ** Validate input.
    */
    if ( ! key  ||  ! buff )  return( FAIL );

    if ( ! str  ||  ! *str )
    {
    	*buff = EOS;
	return( OK );
    }

    /* 
    ** Coerce key to length CRYPT_SIZE.
    */
    for ( len = 0; len < CRYPT_SIZE; len++ )  kbuff[len] = *key ? *key++ : ' ';
    kbuff[ CRYPT_SIZE ] = EOS;
    CIsetkey( kbuff, ksch );

    /*
    ** Input is appended with a delimiter, padded to 
    ** a multiple of CRYPT_SIZE and NULL terminated.
    ** Ensure temp buffer is large enough.
    */
    len = STlength( str );

    tmp = ((len + CRYPT_SIZE + 1) < sizeof( tbuff ))
          ? tbuff : (char *)MEreqmem( 0, len + CRYPT_SIZE + 2, FALSE, NULL );

    if ( ! tmp )  return( FAIL );

    /*
    ** Coerce encryption string to multiple of CRYPT_SIZE.
    ** Add trailing marker for strings with trailing SPACE.
    ** Pad string with spaces to modulo CRYPT_SIZE length.
    */
    MEcopy( (PTR)str, len, (PTR)tmp );
    tmp[ len++ ] = GCN_PASSWD_END;
	
    while( len % CRYPT_SIZE )  tmp[ len++ ] = ' ';
    tmp[ len ] = EOS;

    /*
    ** Now encrypt the string.
    */
    CIencode( tmp, len, ksch, tmp );
    CItotext( (u_char *)tmp, len, buff );

    if ( tmp != tbuff )  MEfree( (PTR)tmp );
    return( OK );
}
