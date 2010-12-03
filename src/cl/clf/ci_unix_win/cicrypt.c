/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<ci.h>
# include	"cilocal.h"

/**
** Name:    CIcrypt.c	- data encryption module
**
** Description:
**          This module contains routines for encryption and
**	decryption.
**
** This file defines:
**	
**	The following routines are intended to be used by RTI system code:
**
**      CIsetkey()	initilize an encryption key schedule for CIencrypt().
**
**	The following routines are intended to be used internally by CI:
**
**	CIencrypt()	Encrypt or decrypt a cipher block of bytes.
**	CIexpand()	map 8 bytes to 64 bytes of bits to pass to CIencrypt().
**	CIshrink()	map 64 bytes of bits to 8 bytes after encryption.
**
** History:
 * Revision 1.1  90/03/09  09:14:05  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:38:37  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:43:30  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:03:15  source
 * sc
 * 
 * Revision 1.2  89/03/15  17:03:10  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:02  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:26:20  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  87/11/17  15:37:51  mikem
**      changed name of local hdr to cilocal.h from ci.h
**      
**      Revision 1.2  87/11/09  15:07:54  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.2  87/07/27  11:11:36  mikem
**      Updated to meet jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-mar-89 (russ)
**	    Adding 5.0 fix found by Tektronix for Greenhill C.
**	    The initialization of L[] and R[] assume the two arrays
**	    are contiguous in memory.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-may-1998 (canor01)
**	    Make CIencrypt() thread-safe by moving static variables onto
**	    the stack.
**	29-jul-1998 (shust01) -- Moved C and D arrays from global to CIsetkey(),
**	    since it is not thread-safe when global.  CIsetkey() is the only 
**	    routine that accesses the arrays and initializes them every time
**	    it is called.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  20-Jul-2004 (lakvi01)
**		SIR 112703, cleaned-up compilation warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      29-Nov-2010 (frima01) SIR 124685
**          Moved declaration of CIencrypt, CIexpand and CIshrink to cilocal.h.
**/


/*
**	Static variable definitions.
*/

/* Initial permutation */
static	char	IP[] = {
	58,50,42,34,26,18,10, 2,
	60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,
	64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,
	59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,
	63,55,47,39,31,23,15, 7,
};

/* Final permutation, FP = IP^(-1) */
static	char	FP[] = {
	40, 8,48,16,56,24,64,32,
	39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,
	37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,
	35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,
	33, 1,41, 9,49,17,57,25,
};

/*
** Permuted-choice 1 from the key bits
** to yield C and D.
** Note that bits 8,16... are left out:
** They are intended for a parity check.
*/
static	char	PC1_C[] = {
	57,49,41,33,25,17, 9,
	 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,
	19,11, 3,60,52,44,36,
};
static	char	PC1_D[] = {
	63,55,47,39,31,23,15,
	 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,
	21,13, 5,28,20,12, 4,
};

/* Sequence of shifts used for the key schedule. */
static	char	shifts[] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
};

/*
** Permuted-choice 2, to pick out the bits from
** the CD array that generate the key schedule.
*/
static	char	PC2_C[] = {
	14,17,11,24, 1, 5,
	 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,
	16, 7,27,20,13, 2,
};
static	char	PC2_D[] = {
	41,52,31,37,47,55,
	30,40,51,45,33,48,
	44,49,39,56,34,53,
	46,42,50,36,29,32,
};

/* The E bit-selection table. */
static	char	e[] = {
	32, 1, 2, 3, 4, 5,
	 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,
	12,13,14,15,16,17,
	16,17,18,19,20,21,
	20,21,22,23,24,25,
	24,25,26,27,28,29,
	28,29,30,31,32, 1,
};

/* The 8 selection functions. */
static	char	S[8][64] = {
	14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	 0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	 4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13,

	15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	 3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	 0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9,

	10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	 1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12,

	 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	 3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14,

	 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	 4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3,

	12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	 9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	 4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13,

	 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	 1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	 6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12,

	13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	 1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	 7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	 2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11,
};

/*
** P is a permutation on the selected combination
** of the current L and key.
*/
static	char	P[] = {
	16, 7,20,21,
	29,12,28,17,
	 1,15,23,26,
	 5,18,31,10,
	 2, 8,24,14,
	32,27, 3, 9,
	19,13,30, 6,
	22,11, 4,25,
};



/*
** Name:	CIexpand	- unpack 8 bytes' bits into 64 bytes.
**
** Description:
**	  This routine unpacks the bits from CRYPT_SIZE bytes into their corresponding
**	position in a BITS_IN_CRYPT_BLOCK byte array.  This is a utility routine to format
**	information for the encryption routine.
**
** Inputs:
**	ate_bytes	the CRYPT_SIZE bytes to be unpacked.
**
** Outputs:
**	b_map		the BITS_IN_CRYPT_BLOCK byte array to put the unpacked
**			bits in.
**
** History:
**	16-sep-85 (ericj) - written.
**	5-may-86 (ericj)
**		Made this a static function.
**	14-apr-95 (emmag)
**	    Desktop compilers insist that register variables
**	    be accompanied by a type name.
*/

VOID
CIexpand(ate_bytes, b_map)
PTR	ate_bytes;
PTR	b_map;
{
	register i4	i, j;
	register i4	c;		/* a char being unpacked */

	for (i = 0; i < BITS_IN_CRYPT_BLOCK;)
	{
		c = *ate_bytes++;
		for (j = 0; j < BITSPERBYTE; i++, j++)
		{
			*b_map++ = (c >> ((BITSPERBYTE - 1) - j)) & 01;
		}
	}
}


/*{
** Name:	CIsetkey	- set up the key schedule
**
** Description:
**	    This routine sets up a key schedule for the
**	block cipher encryption routine.
**
** Inputs:
**	key_str	pointer to KEY_SIZE characters to make the key 
**		schedule from.
**
** Outputs:
**	KS	pointer to the key schedule to be initialized.
**
** History:
**	8-oct-1985 (ericj) -- written.
**	29-jul-1998 (shust01) -- Moved C and D arrays from global to CIsetkey(),
**	    since it is not thread-safe when global.  CIsetkey() is the only 
**	    routine that accesses the arrays and initializes them every time
**	    it is called.
*/

VOID
CIsetkey(key_str, KS)
PTR	key_str;
CI_KS	KS;
{
	register i4	i, j, k;
		 i4	t;
		 char	key[BITS_IN_CRYPT_BLOCK];
/* The C and D arrays used to calculate the key schedule. */
          	char	C[28];
  		char	D[28];


	/* unpack the encryption key into bits */
	CIexpand(key_str, key);

	/*
	** First, generate C and D by permuting
	** the key.  The low order bit of each
	** 8-bit char is not used, so C and D are only 28
	** bits apiece.
	*/
	for (i=0; i<28; i++) {
		C[i] = key[PC1_C[i]-1];
		D[i] = key[PC1_D[i]-1];
	}

	/*
	** To generate Ki, rotate C and D according
	** to schedule and pick up a permutation
	** using PC2.
	*/
	for (i = 0; i < ITER_NUM; i++) {
		/* rotate */
		for (k=0; k<shifts[i]; k++) {
			t = C[0];
			for (j=0; j<28-1; j++)
				C[j] = C[j+1];
			C[27] = t;
			t = D[0];
			for (j=0; j<28-1; j++)
				D[j] = D[j+1];
			D[27] = t;
		}
		/* get Ki. Note C and D are concatenated */
		for (j=0; j<24; j++) {
			KS[i][j] = C[PC2_C[j]-1];
			KS[i][j+24] = D[PC2_D[j]-28-1];
		}
	}
}



/*
** Name:	CIshrink - pack BITS_IN_CRYPT_BLOCK bytes containing a bit into
**			   CRYPT_SIZE bytes.	
**
** Description:
**	   This routines packs an array of BITS_IN_CRYPT_BLOCK bytes
**	containing 0s and 1s into a CRYPT_SIZE number of bytes.
**
** Inputs:
**	b_map		pointer to BITS_IN_CRYPT_BLOCK bytes of bits to
**			be packed.
**
** Outputs:
**	ate_bytes	pointer to CRYPT_SIZE bytes to hold packed bits.
**
** History:
**	16-sep-85 (ericj) - written.
**	14-apr-95 (emmag)
**		Desktop compilers insist that register variables
**		must be accompanied by a type name.
*/

VOID
CIshrink(b_map, ate_bytes)
PTR	b_map;
PTR	ate_bytes;
{
	register i4	i, j;
	register i4	c;		/* a byte being packed up */

	for (i = 0; i < BITS_IN_CRYPT_BLOCK;)
	{
		c = 0;
		for (j = 0; j < BITSPERBYTE; i++, j++)
		{
			c <<= 1;
			c |= *b_map++;
		}
		*ate_bytes++ = c;
	}
	return;
}



/*
**	Static function definitions.
*/


/*
** Name:	CIencrypt	- encrypt or decrypt a block.
**
** Description:
**	    This routine implements a block cipher encryption
**	and decryption routine.
**
** Inputs:
**	KS		pointer to an encryption key schedule which was generated
**			by CIsetkey.
**	decode_flag	FALSE to encode, TRUE to decode.
**
** Outputs:
**	block		pointer to a block of BITS_IN_CRYPT_BLOCK bytes to be encrypted or decrypted.
**			This block serves as both the input and output.
**
** History:
**	8-oct-1985 (ericj) -- written.
*/

VOID
CIencrypt(KS, decode_flag, block)
CI_KS		KS;
bool		decode_flag;
char 		*block;
{
		 i4  	i, ii;
	register i4	t, j, k;

/* The current block, divided into 2 halves. */
      	char	LR[64];
      	char	*L = LR, *R = LR + 32;
      	char	tempL[32];
      	char	f[32];

/* The combination of the key and the input, before selection. */
      	char	preS[48];

	/* First, permute the bits in the input	 */
	for (j=0; j < BITS_IN_CRYPT_BLOCK; j++)
		LR[j] = block[IP[j]-1];

	/* Perform an encryption operation ITER_NUM times. */
	for (ii = 0; ii < ITER_NUM; ii++) {
		 /* encrypting or decrypting? */
		if (decode_flag)
			i = (ITER_NUM - 1) - ii;
		else
			i = ii;

		/* Save the R array, which will be the new L. */
		for (j=0; j<32; j++)
			tempL[j] = R[j];

		/*
		** Expand R to 48 bits using the E selector;
		** exclusive-or with the current key bits.
		*/
		for (j=0; j<48; j++)
			preS[j] = R[e[j]-1] ^ KS[i][j];
		/*
		** The pre-select bits are now considered
		** in 8 groups of 6 bits each.
		** The 8 selection functions map these
		** 6-bit quantities into 4-bit quantities
		** and the results permuted
		** to make an f(R, K).
		** The indexing into the selection functions
		** is peculiar; it could be simplified by
		** rewriting the tables.
		*/
		for (j=0; j<8; j++) {
			t = 6*j;
			k = S[j][(preS[t+0]<<5)+
				(preS[t+1]<<3)+
				(preS[t+2]<<2)+
				(preS[t+3]<<1)+
				(preS[t+4]<<0)+
				(preS[t+5]<<4)];
			t = 4*j;
			f[t+0] = (k>>3)&01;
			f[t+1] = (k>>2)&01;
			f[t+2] = (k>>1)&01;
			f[t+3] = (k>>0)&01;
		}
		/*
		** The new R is L ^ f(R, K).
		** The f here has to be permuted first, though.
		*/
		for (j=0; j<32; j++)
			R[j] = L[j] ^ f[P[j]-1];
		/*
		** Finally, the new L (the original R)
		** is copied back.
		*/
		for (j=0; j<32; j++)
			L[j] = tempL[j];
	}

	/* The output L and R are reversed. */
	for (j=0; j<32; j++) {
		t = L[j];
		L[j] = R[j];
		R[j] = t;
	}
	/*
	** The final output
	** gets the inverse permutation of the very original.
	*/
	for (j=0; j<64; j++)
		block[j] = L[FP[j]-1];
}
