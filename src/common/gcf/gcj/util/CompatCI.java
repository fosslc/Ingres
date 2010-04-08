/*
** Copyright (c) 1999, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: CompatCI.java
**
** Description:
**	Implements the encryption class corresponding to the
**	Compatibility Library module CI.
**
**  Classes
**
**	CompatCI
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Moved to util package.
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Renamed to remove specific product reference.  
*/


/*
** Name: CompatCI
**
** Description:
**	Class which implements the encryption mechanism used in
**	the Compatibility Library module CI.
**
**  Constants
**
**	CRYPT_SIZE	Block size used for keys and encryption.
**	KS_SIZE		Size, in bytes, of a key schedule.
**
**  Public Methods
**
**	setkey		Generate key schedule from encryption key.
**	encode		Encrypt a byte array.
**
**  Private Methods
**
**	expand		Expand bytes to single bit per byte.
**	shrink		Shrink bit per byte to bytes.
**	encrypt		Encrypt a single block.
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Renamed to remove specific product reference.
*/

public class
CompatCI
{
    /*
    ** Constants defined by CI.
    */
    public  static final int	CRYPT_SIZE = 8;

    private static final int	BITSPERBYTE = 8;
    private static final int	ITER_NUM = 8;
    private static final int	BITS_IN_CRYPT_BLOCK = 
				(CRYPT_SIZE * BITSPERBYTE);

    /*
    ** Constants used in CI but not formally defined.
    */
    private static final int	KS_BITS = 48;

    public  static final int	KS_SIZE = (ITER_NUM * KS_BITS);

    /* Initial permutation */
    private static final byte	IP[] = 
    {
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
    private static final byte	FP[] = 
    {
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
    private static final byte	PC1_C[] = 
    {
        57,49,41,33,25,17, 9,
         1,58,50,42,34,26,18,
        10, 2,59,51,43,35,27,
        19,11, 3,60,52,44,36,
    };

    private static final byte	PC1_D[] = 
    {
        63,55,47,39,31,23,15,
         7,62,54,46,38,30,22,
        14, 6,61,53,45,37,29,
        21,13, 5,28,20,12, 4,
    };

    /* Sequence of shifts used for the key schedule. */
    private static final byte	shifts[] = 
    {
        1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
    };

    /*
    ** Permuted-choice 2, to pick out the bits from
    ** the CD array that generate the key schedule.
    */
    private static final byte	PC2_C[] = 
    {
        14,17,11,24, 1, 5,
         3,28,15, 6,21,10,
        23,19,12, 4,26, 8,
        16, 7,27,20,13, 2,
    };

    private static final byte	PC2_D[] = 
    {
        41,52,31,37,47,55,
        30,40,51,45,33,48,
        44,49,39,56,34,53,
        46,42,50,36,29,32,
    };

    /* The E bit-selection table. */
    private static final byte	e[] = {
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
    private static final byte	S[][] = 
    {
	{   14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	     0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	     4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	    15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13, },
    
	{   15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	     3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	     0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	    13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9, },

	{   10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	    13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	    13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	     1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12, },

	{    7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	    13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	    10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	     3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14, },

	{    2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	    14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	     4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	    11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3, },

	{   12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	    10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	     9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	     4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13, },

	{    4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	    13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	     1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	     6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12, },

	{   13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	     1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	     7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	     2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11, },
    };

    /*
    ** P is a permutation on the selected combination
    ** of the current L and key.
    */
    private static final byte P[] = 
    {
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
    ** These buffers are used by setkey()
    ** (synchronization required).
    */
    private static byte	    key[] = new byte[ BITS_IN_CRYPT_BLOCK ];
    private static byte	    C[] = new byte[ 28 ];
    private static byte	    D[] = new byte[ 28 ];

    /*
    ** These buffers are used by encrypt()
    ** (synchronization required).
    */
    private static byte	    LR[] = new byte[ 64 ];
    private static byte	    tempL[] = new byte[ 32 ];
    private static byte	    f[] = new byte[ 32 ];
    private static byte	    preS[] = new byte[ 48 ];

    /*
    ** This buffer is used by encode()
    ** (synchronization required).
    */
    private static byte	    bit_map[] = new byte[ CRYPT_SIZE * 
						  BITSPERBYTE ];

/*
** Name: setkey	(see CIsetkey())
**
** Description:
**	Builds a key schedule from a key string.
**	Only the first CRYPT_SIZE bytes of key_str
**	are used.
**
** Input:
**	key_str	    Key string (CRYPT_SIZE bytes).
**
** Output:
**	KS	    Key schedule (KS_SIZE bytes).
**
** Returns:
**	void
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
*/

public static void
setkey( byte key_str[], byte KS[] )
{
    int	    i, j, k;

    /*
    ** We use pre-allocated temporary buffers,
    ** so they must be locked to be thread safe.
    */
    synchronized( key )
    {
	/* unpack the encryption key into bits */
	expand( key_str, 0, key );

	/*
	** First, generate C and D by permuting
	** the key.  The low order bit of each
	** 8-bit char is not used, so C and D 
	** are only 28 bits apiece.
	*/
	for( i = 0; i < C.length; i++) 
	{
	    C[ i ] = key[ PC1_C[ i ] - 1 ];
	    D[ i ] = key[ PC1_D[ i ] - 1 ];
	}

	/*
	** To generate Ki, rotate C and D according
	** to schedule and pick up a permutation
	** using PC2.
	*/
	for ( i = 0; i < ITER_NUM; i++ ) 
	{
	    /* rotate */
	    for( k = 0; k < shifts[ i ]; k++ ) 
	    {
		byte t = C[ 0 ];

		for( j = 0; j < (C.length - 1); j++ )  
		    C[ j ] = C[ j + 1 ];

		C[ C.length - 1 ] = t;
		t = D[ 0 ];

		for( j = 0; j < (D.length - 1); j++ )
		    D[ j ] = D[ j + 1 ];

		D[ D.length - 1 ] = t;
	    }

	    /* get Ki. Note C and D are concatenated */
	    for( j = 0; j < (KS_BITS / 2); j++) 
	    {
		KS[ (i * KS_BITS) + j ] = C[ PC2_C[ j ] - 1 ];
		KS[ (i * KS_BITS) + j + (KS_BITS / 2) ] = 
				D[ PC2_D[ j ] - D.length - 1 ];
	    }
	}
    } // synchronized

    return;
} // setkey

  
/*
** Name: encode	(see CIencode())
**
** Description:
**	Encodes a binary buffer using a block cipher and a
**	key schedule built by setkey().  The block to be
**	encoded should be a multiple of CRYPT_SIZE bytes
**	in length.  The output buffer may be the same as 
**	the input buffer.
**
** Input:
**	plain_text	Buffer containing bytes to encode.
**	p_offset	Position of bytes to encode.
**	size		Number of bytes to encode.
**	ks		Key Schedule.
**	c_offset	Position for encoded bytes.
**
** Output:
**	cipher_text	Buffer to receive encoded bytes.
**
** Returns:
**	void
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
*/

public static void
encode( byte plain_text[], int p_offset, int size, byte ks[],
        byte cipher_text[], int c_offset )
{
    int	end = p_offset + size;

    synchronized( bit_map )
    {
        while( p_offset < end )	// while there is text to encode
        {
	    /*
	    ** unpack 8 bytes of plain text bits to 64 byte
	    ** representation, update pointer to next byte to
	    ** be encrypted.
	    */
	    p_offset = expand( plain_text, p_offset, bit_map );

	    encrypt( ks, false, bit_map );  // encrypt the bit map

	    /*
	    ** pack the bits back into a encrypted 8 byte
	    ** representaion and update pointer to next empty
	    ** byte in cipher text array.
	    */
	    c_offset = shrink( bit_map, cipher_text, c_offset );
        }
    } // synchronized

    return;
} // encode


/*
** Name: expand	(see CIexpand())
**
** Description:
**	Unpacks CRYPT_SIZE bytes into BITS_IN_CRYPT_BLOCK bytes.  
**	The input buffer must be at least (offset + CRYPT_SIZE)
**	bytes in length while the output buffer must be at least
**	BITS_IN_CRYPT_BLOCK bytes in length.
**
** Input:
**	ate_bytes   Bytes to be unpacked (starting at offset).
**	offset	    Position of bytes to be unpacked.
**
** Output:
**	b_map	    Unpacked bytes (starting at postion 0).
**
** Returns:
**	int	    New offset in input buffer.
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
*/

private static int
expand( byte ate_bytes[], int offset, byte b_map[] )
{
    int	out = 0;

    for( int i = 0; i < CRYPT_SIZE; i++ )
    {
	byte c = ate_bytes[ offset++ ];
        
	for( int j = 0; j < BITSPERBYTE; j++ )
        {
	    b_map[ out++ ] = 
		(byte)((c >> ((BITSPERBYTE - 1) - j)) & 0x01);
        }
    }

    return( offset );
} // expand


/*
** Name: shrink	(see CIshrink())
**
** Description:
**	Packs BITS_IN_CRYPT_BLOCK bytes into CRYPT_SIZE bytes.  
**	The input buffer must be at least BITS_IN_CRYPT_BLOCK 
**	bytes in length while the output buffer must be at least
**	(offset + CRYPT_SIZE) bytes in length.
**
** Input:
**	b_map	    Bytes to be packed (starting at postion 0).
**	offset	    Position for packed bytes.
**
** Output:
**	ate_bytes   Packed bytes (starting at offset).
**
** Returns:
**	int	    New offset in output buffer.
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
*/

private static int
shrink( byte b_map[], byte ate_bytes[], int offset )
{
    int	in = 0;

    for( int i = 0; i < CRYPT_SIZE; i++ )
    {
	byte c = 0;
	
	for( int j = 0; j < BITSPERBYTE; j++ )
            c = (byte)((c << 1) | b_map[ in++ ]);

        ate_bytes[ offset++ ] = c;
    }
    
    return( offset );
} // shrink


/*
** Name: encrypt    (see CIencrypt())
**
** Description:
**	Encrypts or decrypts a block of BITS_IN_CRYPT_BLOCK
**	bytes using a Key schedule built by setkey().
**
** Input:
**	KS		Key schedule (KS_SIZE bytes).
**	decode_flag	True for decoding, false for encoding.
**	block		Bytes to be encoded/decoded.
**
** Output:
**	block		Encoded/decoded bytes.
**
** Returns:
**	void
**
** History:
**	14-Jul-99 (gordy)
**	    Created.
*/

private static void
encrypt( byte KS[], boolean decode_flag, byte block[] )
{
    int	i, ii, j, k;

    synchronized( LR )
    {
	/* First, permute the bits in the input  */
	for( j = 0; j < BITS_IN_CRYPT_BLOCK; j++ )
	    LR[ j ] = block[ IP[ j ] - 1 ];

        /* Perform an encryption operation ITER_NUM times. */
	for( ii = 0; ii < ITER_NUM; ii++ ) 
	{
	    /* encrypting or decrypting? */
	    if ( decode_flag )
		i = (ITER_NUM - 1) - ii;
	    else
		i = ii;

	    /* Save the R array, which will be the new L. */
	    for( j = 0; j < (LR.length / 2); j++ )
		tempL[ j ] = LR[ (LR.length / 2) + j ];

	    /*
	    ** Expand R to 48 bits using the E selector;
	    ** exclusive-or with the current key bits.
	    */
	    for( j = 0; j < preS.length; j++ )
		preS[ j ] = (byte)(LR[ (LR.length / 2) + e[j] - 1 ] ^ 
				   KS[ (i * KS_BITS) + j ]);

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
	    for( j = 0; j < S.length; j++ ) 
	    {
		int t = 6 * j;

		k = S[ j ][ (preS[ t + 0 ] << 5) +
			    (preS[ t + 1 ] << 3) +
			    (preS[ t + 2 ] << 2) +
			    (preS[ t + 3 ] << 1) +
			    (preS[ t + 4 ] << 0) +
			    (preS[ t + 5 ] << 4) ];
		
		t = 4 * j;
		f[ t + 0 ] = (byte)((k >> 3) & 0x01);
		f[ t + 1 ] = (byte)((k >> 2) & 0x01);
		f[ t + 2 ] = (byte)((k >> 1) & 0x01);
		f[ t + 3 ] = (byte)((k >> 0) & 0x01);
	    }

	    /*
	    ** The new R is L ^ f(R, K).
	    ** The f here has to be permuted first, though.
	    */
	    for( j = 0; j < (LR.length / 2); j++ )
		LR[ (LR.length / 2) + j ] = (byte)(LR[ 0 + j ] ^ 
						   f[ P[j] - 1 ]);

	    /*
	    ** Finally, the new L (the original R)
	    ** is copied back.
	    */
	    for( j = 0; j < (LR.length / 2); j++ )
		LR[ 0 + j ] = tempL[ j ];
        }

        /* The output L and R are reversed. */
        for( j = 0; j < (LR.length / 2); j++ ) 
	{
	    byte t = LR[ 0 + j ];
	    LR[ 0 + j ] = LR[ (LR.length / 2) + j ];
	    LR[ (LR.length / 2) + j ] = t;
        }

	/*
	** The final output gets the inverse permutation of the original.
	*/
	for( j = 0; j < BITS_IN_CRYPT_BLOCK; j++ )
	    block[ j ] = LR[ FP[ j ] - 1 ];
    
    } // synchronized

    return;
} // encrypt


} // class CompatCI
