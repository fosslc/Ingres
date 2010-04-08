/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef _HSHCRC_HDR
# define _HSHCRC_HDR 1


/* 
** Name:	HSHCRC.c	- Define CRC-32 Hash function extern
**
** Specification:
**      This routine computes a hash function for a variable length byte
**      stream.  A AUTODIN-II CRC polynomial is computed for the byte
**      stream.
**
** Description:
**	Contains HSH CRC-32 function extern
**
** History:
**	11-Jan-1999 (shero03)
**	    initial creation from dm1h_newhash in dm1h.c
**	07-Jun-1999 (shero03)
**	    Add HSH_char - a hash especially for character data
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

void HSH_CRC32(		/* Calculate a CRC-32		        */
	char    *buf, 		/* Pointer to the data to hash		*/
	i4     lenbuf,   	/* Length of buf			*/
        u_i4 *hash_value	/* Pointer to the hash value		*/
);

u_i4 HSH_char(		/* Calculate a hash for character data  */
	char    *buf, 		/* Pointer to the data to hash		*/
	i4      lenbuf   	/* Length of buf			*/
);


# endif /* !_HSHCRC_HDR */
