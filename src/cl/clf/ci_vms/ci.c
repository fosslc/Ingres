/*
**    Copyright (c) 1985, 2003 Ingres Corporation
*/

# include	<compat.h>
# include	<ci.h>
# include	<gl.h>
# include	"cilocal.h"
# include	<me.h>
# include	<st.h>
# include	<ci.h>
# include	<tm.h>
# include	<bt.h>
# include	<er.h>
# include	<cv.h>
# include       <ex.h>

/**
** Name:    CI.c    - This module is used for the coding and decoding of 
**		information.
**
** Description:
**        This module was originally written for encoding and decoding
**	authorization strings in the Ingres Distribution Keys project.
**	  The encryption and decryption algorithms are intended to be general
**	purpose enough to be used by the rest of the system where encoding
**	and decoding routines are required.
**	  This file defines:
**
**	  The following routines are intended to be used by RTI system code:
**
**	CIchksum()	A function that computes the checksum of a binary
**			byte stream.
**      CIdecode()	This routine decodes cipher text to plain text using
**			a key string.
**      CIencode()      This routine encodes plain text to cipher text
**			using a key string.
**	CItobin()	This routine maps a string of characters to a block of
**			binary bytes.  The string of characters must have been
**			originally created using CItotext().
**	CItotext()	This routine maps binary data to a printable alphabet 
**			consisting of upper case letters excluding 'I' and 'O' and
**			digits excluding '0' and '1'.
**
** History:    $Log - for RCS$
**	10-sep-86 (ericj)
**		written.
**	13-mar-86 (ericj)
**		Fix call to CVal() in CIcapability.
**	14-mar-86 (ericj)
**		Temporarily turn off cluster checking in CIcapability routine.
**      <automatically entered by code control>
**	19-sep-89 (jrb)
**	    Added check in CImk_capability_black for special authorization
**	    string which is now invalid.  This was done because the
**	    authorization string was omnipotent and got inadvertantly published
**	    in the I&O guide as an example.
**	6-nov-89 (bobm)
**	    OEM typedef is obsolete.  fix so it won't compile if defined.
**	    return FAIL for CI_PART_RUNTIME and CI_FULL_RUNTIME if CI_NO_CHK
**	6-nov-89 (seputis)
**	    added BT.H
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	25-Jan-1994 (jpk)
**	    get rid of CPU serial number (which had degenerated to
**	    VAX processor model number) in return for 32 more capability bits.
**	29-March-1994 (jpk)
**	    Fix bug 60494: get rid of special case -- runtime licensing
**	    for strings set to disable checking anyway.
**	8 April 1994 (jpk)
**		Change II_AUTHORIZATION to II_AUTH_STRING.
**      08-Aug-1996 (rosga02)
**          Added nat as the return type to CIcapability()
**    	09-Sep-1997 (kinte01)
**	  Remove check for auth string printed in IO guide - Unix change 428688
**	  07-feb-1997 (canor01)
**          Remove special authorization string that was mistakenly
**          distributed in the I&O guide, since it is no longer valid
**          with this new major release and subsequent DIST_KEY.
**          and CImk_capability_block().  
**	10-aug-1998 (kinte01)
**	  Add new CA-license routines
**      04-sep-1998 (kinte01)
**	  Don't report license success to the errlog.  Also check for
**	  a null license string. (unix change 437510)
**	  Don't print empty license messages to the log.  Also, fix
**	  problem whereby we were truncating the messages prematurely.
**	  Unix change (437531)
**	12-jan-1998 (kinte01)
**	  Add new licensing component code for TNG version. Cross integrate
**	  Unix change 439680.
**	11-sep-1998 (kinte01)
**	  Add new bit for CI_ICE (unix change 437663)
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**      18-Oct-2000 (horda03)
**        Added function CIvalidate. Direct copy from UNIX.
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**      04-Dec-2000 (horda03)
**        Add exception handler around call to CA_LIC_CHECK to handle any
**        signals raised in the license checking code.
**        (103403)
**	26-jul-2002 (mcgem01)
**	    Add a licensing component code 2SLS for the spatial object
**	    library.
**	22-nov-2002(mcgem01)
**	    Added a licensing component code of 2AOT for Advantage OpenROAD
**	    Transformation Runtime.
**	13-may-2003 (abbjo03)
**	    Partial cross-integration of Unix CL change of 1-nov-2001, namely,
**	    per new licensing policy, CIcapability will always return OK, even
**	    if LIC_TERMINATE returned.
**	11-Jun-2004 (hanch04)
**	    Removed reference to CI for the open source release.
**	29-Nov-2010 (frima01) SIR 124685
**	    Removed forward declarations - they reside in ci.h.
*/

/* # defines */
# define	BIT_CNT		5
#define CI_i4swap(x) ((((x) & 0x000000ff) << 24) | (((x) >> 24) & 0x000000ff) \
				| (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))
# define CI_i2swap(x) ((((x) & 0x00ff) << 8) | (((x) >> 8) & 0x00ff))
# define	CCITT_INIT	0x0ffff
# define	CCITT_MASK	0102010

/* statics */
/*
**   Define a bit mask which will be used by CItotext and CItobin to clear
** all bits in the transformation register, other than the bits that haven't
** yet been processed.
*/
static	i4	bit_mask[] = {
		0, 01, 03, 07, 017
};


/*
** Name:	CIchksum	- Compute a check sum for a stream of bytes.
**
** Description:
**	   This procedure computes a non-unique checksum for a stream of bytes.
**	Actually this routine produces a CCITT cyclic redundancy code.
**
** Inputs:
**	block	pointer to a stream of binary bytes whose checksum is to be
**		computed.
**	size	number of bytes in the binary byte stream.
**
** Outputs:
**	Returns:
**		a BITFLD, which is the computed checksum.
**
** History:
**	19-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made parameters, size and inicrc, i4s as requested by
**		CL committee.
*/

BITFLD
CIchksum(input, size, inicrc)
u_char	*input;
i4	size;
i4	inicrc;
{
			i4	i;
	register	BITFLD	crc;

	crc = inicrc;
	while (size--)
	{
		crc ^= *input++;
		for (i = 8; i; i--)
		{
			if (crc & 01)
			{
				crc >>= 1;
				crc ^= CCITT_MASK;
			}
			else {
				crc >>= 1;
			}
		}
	}
	return(crc);
}



/*{
** Name:	CIdecode - Decode cipher text to plain text using key.
**
** Description:
**	  This is a general purpose encryption routine designed to decrypt
**	binary cipher text into binary plain text using a block cipher and key
**	schedule to perform the transformation.  IMPORTANT NOTE, This is a block
**	cipher that operates on CRYPT_SIZE bytes of data at a time and expects
**	the size of the cipher text to be evenly divisible by CRYPT_SIZE.
**
** Inputs:
**	cipher_text	pointer to binary cipher text to be decoded.  This is
**			interpreted as a bit stream.
**	size		size in bytes of cipher text to be decoded.  This size
**			should be evenly divisible by CRYPT_SIZE.
**	ks		key schedule used to transform cipher text back to plain
**			text.  Must be the same key that was used to originally
**			encrypt the cipher text.
** Outputs:
**	plain_text	pointer to byte array to hold decoded cipher text.
**			Must be at least as big as cipher text being decoded.
**	
** History:
**	10-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made size parameter a i4 as requested by CL committee.
*/
VOID
CIdecode(cipher_text, size, ks, plain_text)
PTR		cipher_text;
i4		size;
CI_KS		ks;
PTR		plain_text;
{
	/* Define an array to hold CRYPT_SIZE bytes worth of unpacked bits */
	char	bit_map[BITS_IN_CRYPT_BLOCK];

	/* for length of cipher text */
	while (size > 0)
	{
		/*
		** expand 8 cipher text bytes to 64 bit representation,
		** update pointer to next byte to be deciphered, and
		** update count of bytes left to be decoded
		*/
		CIexpand(cipher_text, bit_map);
		cipher_text += 8;
		size -= 8;

		/* decrypt an 8 byte block of cipher text */
		CIencrypt(ks, TRUE, bit_map);

		/*
		** pack 64 decoded bits back to 8 byte representation and
		** update pointer to next plain text position to be filled.
		*/
		CIshrink(bit_map, plain_text);
		plain_text += 8;
	}
	return;
}



/*{
** Name:	CIencode	- encode plain text to cipher text.
**
** Description:
**	  This routine encodes binary plain text to a binary cipher text using
**	a block cipher and key string to perform the transformation.  IMPORTANT
**	NOTE, this routine uses a block cipher that operates on CRYPT_SIZE bytes of data
**	at a time.  Thus, the size of the cipher text being operated on should
**	be padded so that it is evenly divisible by CRYPT_SIZE.
**
** Inputs:
**	plain_text	pointer to binary plain text to be encripted.
**	size		number of bytes in plain text to be encripted.  This
**			should be evenly divisible by CRYPT_SIZE.
**	ks		key schedule to be used in the encryption.
**
** Outputs:
**	cipher_text	pointer to byte array which will hold the encoded
**			binary text.  This array must be at least as big
**			as the cipher text being encrypted.
**
** History:
**	10-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made size parameter a i4 as requested by CL committee.
*/
VOID
CIencode(plain_text, size, ks, cipher_text)
PTR		plain_text;
i4		size;
CI_KS		ks;
PTR		cipher_text;
{
	/* Define an array to hold CRYPT_SIZE bytes worth of unpacked bits */
	char	bit_map[CRYPT_SIZE * BITSPERBYTE];

	/* while there's still plain text to be encoded */
	while (size > 0)
	{
		/*
		** unpack 8 bytes of plain text bits to 64 byte
		** representation, update pointer to next byte to
		** be encrypted, and count of bytes left to be
		** encrypted.
		*/
		CIexpand(plain_text, bit_map);
		plain_text += 8;
		size -= 8;

		/* encrypt the bit map */
		CIencrypt(ks, FALSE, bit_map);
		/*
		** pack the bits back into a encrypted 8 byte
		** representaion and update pointer to next empty
		** byte in cipher text array.
		*/
		CIshrink(bit_map, cipher_text);
		cipher_text += 8;
	}
	return;
}


/*{
** Name:	CItobin	- map printable characters to binary data.
**
** Description:
**	    This routine maps a printable string defined over the alphabet
**	of upper case letters excluding 'I' and 'O' and digits excluding '0'
**	and '1' to a sequence of binary bytes.  Note, this routine is 
**	intended to be used only with text strings that were originally
**	generated using CItobin.
**
** Inputs:
**	textbuf	pointer to printable characters to be mapped to binary bytes.
**
** Outputs:
**	size	pointer to i4 for returning resulting binary block size.
**	block	pointer to sequence of bytes to hold mapped binary bytes.
**		  IMPORTANT NOTE, there must be at least
**		CI_TOBIN_SIZE_MACRO(STlength(textbuf)) number of bytes available.
**		If this argument is going to be passed to CIdecode or CIencode, its
**		size in bytes should be evenly divisible by CRYPT_SIZE.
**
** History:
**	16-sep-85 (ericj) - written.
**	05-jan-86 (ericj) - modified to map from a different printable alphabet as
**			generated by CItotext.
*/

VOID
CItobin(textbuf, size, block)
PTR	textbuf;
i4	*size;
u_char	*block;
{
	register	t_reg = 0;		/* transformation register */
	register	c;		/* character being upmapped */
	i4		i;
	i4		bit_cnt = 0;	/* # of bits in t_reg to be mapped. */

	*size = 0;
	/* while there's still text to be mapped to binary data */
	while (*textbuf) {
		/* get up to 4 printable chars to map to three binary bytes */
		while (bit_cnt < 3 * BITSPERBYTE)
		{
			if (*textbuf)
			{
				c = *textbuf++;
				if (c > 'O')
					c--;
				if (c > 'I')
					c--;
				if (c > '9')
					c -= 7;
				c -= '2';
				t_reg <<= BIT_CNT;
				t_reg |= (c & 037);
				bit_cnt += BIT_CNT;
			}
			else
			{
				break;
			}
		}
		/* write out as many integral bytes as there are in t_reg */
		for (i = BITSPERBYTE; bit_cnt - i >= 0; i += BITSPERBYTE)
		{
			*block++ = (t_reg >> (bit_cnt - i)) & 0377;
			(*size)++;
		}
		bit_cnt -= (i - BITSPERBYTE);
		t_reg &= bit_mask[bit_cnt];
	}
	return;
}



/*{
** Name:	CItotext	- map binary bytes to printable alphabet.
**
** Description:
**	    This routine maps a sequence of binary bytes to the characters
**	in a printable alphabet.  The printable alphabet consists of upper
**	case letters excluding 'O' and 'I' and digits excluding '0' and '1'.
**
** Inputs:
**	block	pointer to sequence of binary bytes to be mapped.
**	size	the number of binary bytes to be mapped.
**
** Outputs:
**	textbuf	pointer of output array to hold mapped printable characters.
**		IMPORTANT NOTE, this array's size must be greater than or equal
**		to CI_TOTEXT_SIZE_MACRO(size).  
**
** History: 
**	16-sep-85 (ericj) - written.
**	05-jan-86 (ericj) - modified to map to a new printable alphabet that
**			consists of characters that are easily distinguished on
**			our laser printer.
*/
VOID
CItotext(block, size, textbuf)
u_char	*block;
i4	size;
PTR	textbuf;
{
	register	t_reg = 0;		/* transformation register */
	register	c;		/* a printable character */
	i4		i;
	i4		bit_cnt = 0;	/* # of bits to transform */

	/* while there are still bytes to transform or bits to be processed */
	while (size > 0 || bit_cnt)
	{
		/* fill transformation register with up to three bytes */
		for (i = 3; i && bit_cnt < 3 * BITSPERBYTE; size--, i--)
		{
			if (size > 0)
			{
				t_reg <<= BITSPERBYTE;
				t_reg = ((*block++) & 0377) | t_reg;
				bit_cnt += BITSPERBYTE;
			}
			else {
				if (bit_cnt % BIT_CNT)
				{
					t_reg <<= (BIT_CNT - (bit_cnt % BIT_CNT));
					bit_cnt += (BIT_CNT - (bit_cnt % BIT_CNT));
				}
				break;
			}
		}
		/* map as many integral BIT_CNT units in t_reg to printable characters */
		for (i = BIT_CNT; bit_cnt - i >= 0; i += BIT_CNT)
		{
			/* map BIT_CNT bits to a printable character */
			c = (t_reg >> (bit_cnt - i)) & 037;
			c += '2';
			if (c > '9')
				c += 7;
			if (c >= 'I')
				c ++;
			if (c >= 'O')
				c++;
			*textbuf++ = c;
		}
		/*
		** count how many bits weren't processed and clear the bits
		** that were.
		*/
		bit_cnt -= (i - BIT_CNT);	
		t_reg &= bit_mask[bit_cnt];
	}
	*textbuf = '\0';
}
