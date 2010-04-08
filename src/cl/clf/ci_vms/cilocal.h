/*
**	Copyright (c) 1985, 2001 Ingres Corporation
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
**	<automatically entered by code control>
**      4-oct-92 (ed) - change illegal endif directive to comment
**	25-Jan-1994 (jpk) get rid of CPU serial number checking
**	8 April 1994 (jpk)
**		Change II_AUTHORIZATION to II_AUTH_STRING.
**	10-aug-1998 (kinte01)
**		Add ca_license_check return codes.
**	09-jan-2001 (kinte01)
**		Add CI_SITEID and CI_BADSITEID.
**	11-Jun-2004 (hanch04)
**	    Removed reference to CI for the open source release.
**/

/* Define offset for CI errors into the CLerror file */
# define	CI_EROFFSET		(E_CL_MASK + E_CI_MASK)

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

# define        CI_NO_LICENSE           CI_EROFFSET + 0x50
# define        CI_WG_COUNT             CI_EROFFSET + 0x51
# define        CI_EXPIRED              CI_EROFFSET + 0x52
# define        CI_WILL_EXPIRE          CI_EROFFSET + 0x53
# define        CI_CANT_OPEN            CI_EROFFSET + 0x54
# define        CI_CORR_FILE            CI_EROFFSET + 0x55
# define        CI_CANT_READ            CI_EROFFSET + 0x56
# define        CI_MAC_SERIAL           CI_EROFFSET + 0x57
# define        CI_MACHINETYPE          CI_EROFFSET + 0x58
# define        CI_TERMINATE            CI_EROFFSET + 0x59
# define        CI_LICENSE_ERR          CI_EROFFSET + 0x60
# define        CI_SITEID               CI_EROFFSET + 0x61
# define        CI_BADSITEID            CI_EROFFSET + 0x62


/* Define the cipher block size */
# define	CRYPT_SIZE	8

/* Define the number of bits in a cipher block */
# define	BITS_IN_CRYPT_BLOCK	(CRYPT_SIZE * BITSPERBYTE)

/* Define the number of seconds in a day */
# define	ADAY		(24 * 60 * 60)




