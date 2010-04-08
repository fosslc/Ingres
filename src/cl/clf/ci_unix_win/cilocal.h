/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	CI.h	- Define local cipher constants.
**
** Description:
**	  This file contains local constants used by the compatiblility
**	library module, CI.  This module is used for encryption and
**	decryption, mapping binary byte streames to and from printable
**	strings,  and interpreting authorization strings for Ingres
**	Distribution keys.
**
** History:
 * Revision 1.1  90/03/09  09:14:05  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:38:42  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:43:33  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:03:17  source
 * sc
 * 
 * Revision 1.2  89/04/11  12:10:34  source
 * sc
 * 
 * Revision 1.2  89/03/20  12:09:06  pete
 * change CI_EROFFSET to use E_CL_MASK (as on VMS).
 * 
 * Revision 1.1  88/08/05  13:26:21  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/09  15:07:40  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/27  11:11:11  mikem
**      Updated to meet jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      07-mar-91 (rudyw)
**          Comment out text following #endif
**	25-Jan-1994 (jpk)
**		get rid of CPU serial number check.
**	8 April 1994 (jpk)
**		Change II_AUTHORIZATION to II_AUTH_STRING.
**	06-may-1998 (canor01)
**	    Add ca_license_check return codes.
**	27-may-1998 (canor01)
**	    Add CI_LICENSE_ERR as a generic return code with no parameters.
**      29-dec-1998 (canor01)
**          Add CI_SITEID and CI_BADSITEID.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**/

/* Define offset for CI errors into the CLerror file */
# define	CI_EROFFSET		(E_CL_MASK  + E_CI_MASK)

/*    Define CIcapability error return codes */
# define	CI_NOSTR		1 + CI_EROFFSET
# define	CI_TOOLITTLE		2 + CI_EROFFSET
# define	CI_TOOBIG		3 + CI_EROFFSET
# define	CI_BADCHKSUM		4 + CI_EROFFSET
# define	CI_BADEXPDATE		5 + CI_EROFFSET
# define	CI_BADSERNUM		6 + CI_EROFFSET
# define	CI_BADCPUMODEL		7 + CI_EROFFSET
# define	CI_BADERROR		8 + CI_EROFFSET
# define	CI_BADKEY		9 + CI_EROFFSET
# define	CI_NOCLUSTER		10 + CI_EROFFSET
# define	CI_NOCAP		20 + CI_EROFFSET

# define	CI_NO_LICENSE		CI_EROFFSET + 0x50
# define	CI_WG_COUNT		CI_EROFFSET + 0x51
# define	CI_EXPIRED		CI_EROFFSET + 0x52
# define	CI_WILL_EXPIRE		CI_EROFFSET + 0x53
# define	CI_CANT_OPEN		CI_EROFFSET + 0x54
# define	CI_CORR_FILE		CI_EROFFSET + 0x55
# define	CI_CANT_READ		CI_EROFFSET + 0x56
# define	CI_MAC_SERIAL		CI_EROFFSET + 0x57
# define	CI_MACHINETYPE		CI_EROFFSET + 0x58
# define	CI_TERMINATE		CI_EROFFSET + 0x59
# define	CI_LICENSE_ERR		CI_EROFFSET + 0x60
# define        CI_SITEID               CI_EROFFSET + 0x61
# define        CI_BADSITEID            CI_EROFFSET + 0x62


/* Define the cipher block size */
# define	CRYPT_SIZE	8

/* Define the number of bits in a cipher block */
# define	BITS_IN_CRYPT_BLOCK	(CRYPT_SIZE * BITSPERBYTE)

/* Define the number of seconds in a day */
# define	ADAY		(24 * 60 * 60)
