/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <ci.h>
#include <gc.h>
#include <mu.h>
#include <qu.h>
#include <st.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcu.h>

/**
**
**  Name: gcnencry.c
**
**  Description:
**      Contains the following functions:
**
**          gcn_decrypt - Encrypt the password
**
**
**	These two routines are the encrypt and decrypt routines for the
**	Name Service stored passwords. Note that these routines should not
**	be part of of the GCA Library and are only available in NETU itself
**	when it encrypts and any Server that needs to decrypt the password.
**
**  History:    $Log-for RCS$
**	    
**      09-Sep-87   (lin)
**          Initial creation.
**	05-Nov-88   (jorge)
**	    Attempted to add comments. Need to check up in size of arrays.
**	07-Nov-88   (jorge)
**	    Cleaned up and made robust.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	26-Feb-97 (gordy)
**	    Moved gcn_encrypt() to gcu for use outside Name Server.
**	    Need to keep gcn_decrypt() here as it should only be 
**	    accessible to Servers.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	31-mar-2000 (mcgem01)
**	    Jasmine uses the gcn_encrypt function when browsing Ingres;
**	    add a wrapper to gcu_encode to provide this functionality.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 15-Jul-07 (gordy)
**	    Moved gcn_decrypt() into server only code.
**/

STATUS
gcn_encrypt( char *userid, char *passwd, char *etxtbuff )
{
	return(gcu_encode (userid, passwd, etxtbuff));
}
