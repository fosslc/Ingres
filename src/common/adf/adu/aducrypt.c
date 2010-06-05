/*
** Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <cm.h>
#include    <me.h>
#include    <iicommon.h>
#include    <ulf.h>
#include    <adf.h>
#include    <adfint.h>
#include    <adurijndael.h>

/**
**  ADUCRYPT.C -- encryption functions
**
**  The routines included in this file are:
**
**	adu_aesdecrypt() -- AES decrypt encrypted vbyte to plaintext vbyte
**	adu_aesencrypt() -- AES encrypt plaintext vbyte to encrypted vbyte
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:    
**      14-dec-2009 (toumi01) -- created for SIR 122403
**/

/*  Defines of other constants  */

#define	KEYBITS		AES_128_BITS	/* choice for this function */


/*{
** Name: aes_decrypt() - decrypt a varbyte field with 128-bit AES using the
**			 supplied pass phrase
**
** Description:
**	This routine decrypts varbyte data using 128-bit AES. The decrypted
**	value is returned as varbyte, and includes the original data's
**	length field. In all cases, given the nature of AES encryption and
**	the fact that the input encrypted value includes a length, the
**	decrypted result will be at least 2 bytes shorter than the input.
**	In addition, there might have been AES block padding in the encrypted
**	version; any excess is zero-filled.
**
**	The pass phrase is turned into a key by looping over a 128-bit key
**	field adding in all of the user-supplied value. 
**
**	The CBC cipher mode is used for multi-block encryptions.
**
**	If after decryption the length value fails sanity check, it is set
**	to zero. The normal case in which this would happen is when an
**	incorrect pass phrase is supplied.
**
** Inputs:
**	adf_scb		Ptr to ADF session control block.
**	dv1		Ptr to ciphertext DB_DATA_VALUE.
**	dv2		Ptr to password   DB_DATA_VALUE.
**	rdv		Ptr to plaintext  DB_DATA_VALUE.
**
** Outputs:
**	rdv
**	    .db_data	decrypted result
**
**	Returns:
**
**	    E_DB_OK			completed successfully
**	    E_AD1014_BAD_VALUE_FOR_DT	invalid dv1 dv2 or rdv
**
**  History:
**      14-dec-2009 (toumi01) -- created
*/

DB_STATUS
adu_aesdecrypt(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv1,
DB_DATA_VALUE     *dv2,
DB_DATA_VALUE     *rdv)
{
    register i4	i, j;
    register char       *p;
    char                *p1,
			*p2,
			*psave,
			*pend,
			*pprevblk;
    DB_STATUS           db_stat;
    i4			size1,
			size2,
			sizer,
			nrounds;
    unsigned char key[KEYLENGTH(KEYBITS)];
    u_i4 rk[RKLENGTH(KEYBITS)];

    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &p1)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_lenaddr(adf_scb, dv2, &size2, &p2)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_3straddr(adf_scb, rdv, &psave)) != E_DB_OK)
	return (db_stat);
    psave = rdv->db_data;

    /* 
    ** prep the key buffer from the password (interate over the key
    ** buffer adding in user input)
    */
    p = key;
    pend = p + sizeof(key);
    MEfill((u_i2)sizeof(key), 0, (PTR)&key);
    for ( i = 0; i < size2 ; i++ )
    {
	*p += p2[i];
	p++;
	if (p == pend)
	    p = key;
    }

    /*
    ** call adu_rijndaelSetupDecrypt to prepare for decryption
    */
    nrounds = adu_rijndaelSetupDecrypt(rk, key, KEYBITS);

    /*
    ** decrypt AES blocks
    */
    pprevblk = NULL;
    i = size1/AES_BLOCK;
    for ( ; i > 0; i-- )
    {
	adu_rijndaelDecrypt(rk, nrounds, p1, psave);
	/* CBC cipher mode */
	if (pprevblk)
	    for ( j = 0 ; j < AES_BLOCK ; j++ )
		psave[j] ^= pprevblk[j];
	pprevblk = p1;
	p1 += AES_BLOCK;
	psave += AES_BLOCK;
    }

    /* if decrypted data fails sanity check return nothing */
    sizer = (u_i2)((DB_TEXT_STRING *) rdv->db_data)->db_t_count;
    if (sizer > rdv->db_length - sizeof(u_i2))
	((DB_TEXT_STRING *) rdv->db_data)->db_t_count = 0;
	
    return (E_DB_OK);
}


/*{
** Name: aes_encrypt() - encrypt a varbyte field with 128-bit AES using the
**			 supplied pass phrase
**
** Description:
**	This routine encrypts varbyte data using 128-bit AES. The encrypted
**	value is returned as varbyte. All of the input data is encrypted,
**	including the u_i2 length.
**
**	The pass phrase is turned into a key by looping over a 128-bit key
**	field adding in all of the user-supplied value. 
**
**	The CBC cipher mode is used for multi-block encryptions.
**
**	The length of the returned value is the full (varbyte length-included)
**	length of the plaintext operand, padded to 16-byte block size for AES,
**	plus the u_i2 size length of the result varbyte.
**	Example: aes_encrypt('abcdefghijklmno','mypass') returns a 34-byte
**	value. The 15 input varbyte characters, with length, equals 17. This
**	is rounded up for AES block encryption to 32. That 32-byte value
**	with prepended varbyte length is returned, for a total of 34 bytes.
**
** Inputs:
**	adf_scb		Ptr to ADF session control block.
**	dv1		Ptr to plaintext  DB_DATA_VALUE.
**	dv2		Ptr to password   DB_DATA_VALUE.
**	rdv		Ptr to ciphertext DB_DATA_VALUE.
**
** Outputs:
**	rdv
**	    .db_data	encrypted result
**
**	Returns:
**
**	    E_DB_OK			completed successfully
**	    E_AD1014_BAD_VALUE_FOR_DT	invalid dv1 dv2 or rdv
**
**  History:
**      14-dec-2009 (toumi01) -- created
*/

DB_STATUS
adu_aesencrypt(
ADF_CB            *adf_scb,
DB_DATA_VALUE     *dv1,
DB_DATA_VALUE     *dv2,
DB_DATA_VALUE     *rdv)
{
    register i4	i, j;
    register char       *p;
    char                *p1,
			*p2,
			*psave,
			*pend,
			*pprevblk;
    DB_STATUS           db_stat;
    i4			size1,
			size2,
			nrounds;
    unsigned char key[KEYLENGTH(KEYBITS)];
    u_i4 rk[RKLENGTH(KEYBITS)];
    unsigned char residue[AES_BLOCK];

    /* check and set up the parameters */
    if ((db_stat = adu_lenaddr(adf_scb, dv1, &size1, &p1)) != E_DB_OK)
	return (db_stat);
    /* we check dv1 with the above call, but our actual operand for
    ** encryption is the entire DB_DATA value including length */
    p1 = dv1->db_data;
    size1 = dv1->db_length;
    if ((db_stat = adu_lenaddr(adf_scb, dv2, &size2, &p2)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_3straddr(adf_scb, rdv, &psave)) != E_DB_OK)
	return (db_stat);

    /* 
    ** prep the key buffer from the password (interate over the key
    ** buffer adding in user input)
    */
    p = key;
    pend = p + sizeof(key);
    MEfill((u_i2)sizeof(key), 0, (PTR)&key);
    for ( i = 0; i < size2 ; i++ )
    {
	*p += p2[i];
	p++;
	if (p == pend)
	    p = key;
    }

    /*
    ** call adu_rijndaelSetupEncrypt to prepare for encryption
    */
    nrounds = adu_rijndaelSetupEncrypt(rk, key, KEYBITS);

    /*
    ** encrypt full AES blocks
    */
    pprevblk = NULL;
    i = size1/AES_BLOCK;
    for ( ; i > 0; i-- )
    {
	/* CBC cipher mode */
	if (pprevblk)
	    for ( j = 0 ; j < AES_BLOCK ; j++ )
		p1[j] ^= pprevblk[j];
	adu_rijndaelEncrypt(rk, nrounds, p1, psave);
	pprevblk = psave;
	p1 += AES_BLOCK;
	psave += AES_BLOCK;
        size1 -= AES_BLOCK;
    }

    /*
    ** encrypt residual text
    */
    if (size1 > 0)
    {
	MEfill((u_i2)sizeof(residue), 0, (PTR)&residue);
	MEcopy(p1, size1, residue);
	/* CBC cipher mode */
	if (pprevblk)
	    for ( j = 0 ; j < AES_BLOCK ; j++ )
		residue[j] ^= pprevblk[j];
	adu_rijndaelEncrypt(rk, nrounds, residue, psave);
    }
    /*
    ** result length preordained for ADI_O1AES
    */
    ((DB_TEXT_STRING *) rdv->db_data)->db_t_count =
	rdv->db_length - sizeof(u_i2);

    return (E_DB_OK);
}
