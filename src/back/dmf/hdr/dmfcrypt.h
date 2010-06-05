/*
**Copyright (c) 2010 Ingres Corporation
*/

/**
** Name: DMFCRYPT.H - Definitions for DMF encryption.
**
** Description:
**	Function and data definitions for DMF encryption.
**
** History:
**	14-feb-2010 (toumi01) SIR 122403
**	    Created.
**/

#define DMC_CRYPT_MEM		"Cryptshmem.mem"

/*
** Name: DMC_CRYPT - encryption global control block
**
** Description:
**	contains enabled keys for encryption
**
** History:
**	13-apr-2010 (toumi01) SIR 122403
**	    Created.
*/
typedef struct _DMC_CRYPT
{
    CS_SEMAPHORE	crypt_sem;	/* for protection */
    i4			seg_size;	/* number of key entries - total */
    i4			seg_active;	/* number of key entries - active */
} DMC_CRYPT;

/*
** Name: DMC_CRYPT_KEY
**
** Description:
**	contains individual enabled key entries
**
** History:
**	13-apr-2010 (toumi01) SIR 122403
**	    Created.
*/
typedef struct _DMC_CRYPT_KEY
{
    i4		status;
#define		DMC_CRYPT_INACTIVE	0L
#define		DMC_CRYPT_ACTIVE	1L
    i4		db_tab_base;		/* encryption is on base tables */
    u_char	key[AES_256_BYTES];
} DMC_CRYPT_KEY;


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS	dm1e_aes_decrypt(
				DMP_RCB		*r,
				DMP_ROWACCESS	*rac,
				char            *erec,
				char            *prec,
				char            *work,
				DB_ERROR	*dberr);


FUNC_EXTERN DB_STATUS	dm1e_aes_encrypt(
				DMP_RCB		*r,
				DMP_ROWACCESS	*rac,
				char            *prec,
				char            *erec,
				DB_ERROR	*dberr);