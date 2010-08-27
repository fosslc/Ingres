/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <hshcrc.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmftrace.h>
#include    <adurijndael.h>
#include    <dmfcrypt.h>

GLOBALREF	DMC_CRYPT	*Dmc_crypt;

/**
**
**  Name: DM0E.C - DMF encryption support
**
**  Description:
**      This module implements encryption and decryption for DMF.
**
**          dm1e_aes_encrypt
**          dm1e_aes_decrypt
**
**  History:
**      14 feb-2010 (toumi01) SIR 122403
**	    Created.
**	25-May-2010 (kschendel)
**	    Add missing MH include.
**/

/*{
** Name: DM1E_AES_DECRYPT
**
** Description:
**	Implements transparent decryption for column encryption.
**
** Inputs:
**      r			Pointer to DMP_RCB
**      rac			Pointer to DMP_ROWACCESS
**      erec			Pointer to encrypted text rec buffer
**      prec			Pointer to plain text rec buffer
**	work			Pointer to work buffer
**	DB_ERROR		*dberr
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates prec with decrypted version of erec
**
** History:
**      14-feb-2010 (toumi01) SIR 122403
**	    Created.
**	27-Jul-2010 (toumi01) BUG 124133
**	    Store shm encryption keys by dbid/relid, not just relid! Doh!
**	04-Aug-2010 (miket) SIR 122403
**	    Correct function documentation.
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	    Change rcb_enckey_slot base from 0 to 1 for sanity checking.
[@history_template@]...
*/
DB_STATUS
dm1e_aes_decrypt(DMP_RCB *r, DMP_ROWACCESS *rac, char *erec, char *prec,
	char *work, DB_ERROR *dberr)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    STATUS	s = E_DB_OK;
    i4		*err_code = &dberr->err_code;
    u_i4	rk[RKLENGTH(AES_256_BITS)];
    i4		nrounds, keybits, keybytes;
    i4		att, blk, cbc;
    u_i4	crc;
    i4		crclen;
    i4		error;
    char	*pe, *pw, *p;	/* working pointers */
    char	*pprev;		/* previous AES block for CBC mode */
    u_char	key[AES_256_BYTES];
    DMC_CRYPT_KEY *cp;
    bool	aes_trace = DMZ_REC_MACRO(6);

    CLRDBERR(dberr);

    if (Dmc_crypt == NULL)
    {
	i4 slots = 0;
	uleFormat( NULL, E_DM0179_DMC_CRYPT_SLOTS_EXHAUSTED,
	    (CL_ERR_DESC *)NULL,
	    ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 1,
	    sizeof(slots), &slots);
	SETDBERR(dberr, 0, E_DM0174_ENCRYPT_LOCKED);
	return (E_DB_ERROR);
    }

    if (!(t->tcb_rel.relencflags & TCB_ENCRYPTED))
	t = t->tcb_parent_tcb_ptr;	/* for secondary indices */
    cp = (DMC_CRYPT_KEY *)((PTR)Dmc_crypt + sizeof(DMC_CRYPT));
    cp += r->rcb_enckey_slot-1;		/* slot is 1-based */
    MEcopy((PTR)cp->key,sizeof(key),key);	/* cache it locally */
    if ( cp->status == DMC_CRYPT_ACTIVE &&
	 cp->db_id == t->tcb_dcb_ptr->dcb_id &&
	 cp->db_tab_base == t->tcb_rel.reltid.db_tab_base)
	; /* it is the best of all possible worlds */
    else
    {
	/* encryption not unlocked using MODIFY tbl ENCRYPT WITH PASSPHRASE= */
	SETDBERR(dberr, 0, E_DM0174_ENCRYPT_LOCKED);
	return (E_DB_ERROR);
    }

    if (t->tcb_rel.relencflags & TCB_AES128)
    {
	keybits = AES_128_BITS;
	keybytes = AES_128_BYTES;
    }
    else
    if (t->tcb_rel.relencflags & TCB_AES192)
    {
	keybits = AES_192_BITS;
	keybytes = AES_192_BYTES;
    }
    else
    if (t->tcb_rel.relencflags & TCB_AES256)
    {
	keybits = AES_256_BITS;
	keybytes = AES_256_BYTES;
    }
    else
    {
	/* internal error */
	SETDBERR(dberr, 0, E_DM0175_ENCRYPT_FLAG_ERROR);
	return (E_DB_ERROR);
    }

    nrounds = adu_rijndaelSetupDecrypt(rk, key, keybits);

    for (att = 0; att < rac->att_count; att++)
    {
	if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT)
	{
	    if (aes_trace)
		TRdisplay("\tAES %d-bit decrypt blocks:\n",keybits);
	    /* (1) decrypt the data block by block */
	    pe = erec;
	    pw = work;
	    pprev = NULL;
	    for ( blk = 0 ; blk < rac->att_ptrs[att]->encwid
			/ AES_BLOCK ; blk++)
	    {
		if (aes_trace)
		    TRdisplay("\t    %< %4.4{%x%}%2< >%16.1{%c%}<\n", pe, 0);
		adu_rijndaelDecrypt(rk, nrounds, pe, pw);
		/* CBC cipher mode for blocks after the 1st one */
		if (pprev)
		    for ( cbc = 0 ; cbc < AES_BLOCK ; cbc++ )
			pw[cbc] ^= pprev[cbc];
		pprev = pe;
		pe += AES_BLOCK;
		pw += AES_BLOCK;
	    }
	    /* (2) check the hash */
	    if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_CRC)
	    {
		crc = -1;
		crclen = (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_SALT)
		    ? rac->att_ptrs[att]->length + AES_BLOCK 
		    : rac->att_ptrs[att]->length;
		HSH_CRC32(work, crclen, &crc);
		if (MEcmp((PTR)&crc, (PTR)work + crclen, sizeof(crc)) != 0)
		{
		    /* internal error */
		    SETDBERR(dberr, 0, E_DM0176_ENCRYPT_CRC_ERROR);
		    TRdisplay("\tDecryption bad CRC: %x does not match ",crc);
		    MEcopy((PTR)work + crclen, sizeof(crc), (PTR)&crc);
		    TRdisplay("%x:\n",crc);
		    p = work;
		    for ( blk = 0, p = work ;
			blk < rac->att_ptrs[att]->encwid / AES_BLOCK ;
			blk++, p += AES_BLOCK )
			TRdisplay("\t    %< %4.4{%x%}%2< >%16.1{%c%}<\n", p, 0);
		    return (E_DB_ERROR);
		}
	    }
	    /* (3) desalinate data */
	    if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_SALT)
		pw = work + AES_BLOCK;
	    else
		pw = work;
	    /* (4) extract the data */
	    MEcopy(pw, rac->att_ptrs[att]->length, prec);
	    erec += rac->att_ptrs[att]->encwid;
	    prec += rac->att_ptrs[att]->length;
	}
	else
	{
	    MEcopy(erec,rac->att_ptrs[att]->length,prec);
	    erec += rac->att_ptrs[att]->length;
	    prec += rac->att_ptrs[att]->length;
	}
    }

    return(s);
}

/*{
** Name: DM1E_AES_ENCRYPT
**
** Description:
**	Implements transparent encryption for column encryption.
**
** Inputs:
**      r			Pointer to DMP_RCB
**      rac			Pointer to DMP_ROWACCESS
**      prec			Pointer to plain text rec buffer
**      erec			Pointer to encrypted text rec buffer
**	DB_ERROR		*dberr
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates erec with encrypted version of prec
**
** History:
**      14-feb-2010 (toumi01) SIR 122403
**	    Created.
**	27-Jul-2010 (toumi01) BUG 124133
**	    Store shm encryption keys by dbid/relid, not just relid! Doh!
**	04-Aug-2010 (miket) SIR 122403
**	    Correct function documentation.
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	    Change rcb_enckey_slot base from 0 to 1 for sanity checking.
[@history_template@]...
*/
DB_STATUS
dm1e_aes_encrypt(DMP_RCB *r, DMP_ROWACCESS *rac, char *prec, char *erec,
	DB_ERROR *dberr)
{
    DMP_TCB	*t = r->rcb_tcb_ptr;
    STATUS	s = E_DB_OK;
    i4		*err_code = &dberr->err_code;
    u_i4	rk[RKLENGTH(AES_256_BITS)];
    i4		nrounds, keybits, keybytes;
    i4		att, blk, cbc;
    u_i4	rand_i4;
    u_i4	crc;
    i4		crclen;
    i4		error;
    char	*p;		/* working pointer */
    char	*pprevblk;	/* previous AES block for CBC mode */
    char	eout[AES_BLOCK];	/* working encrypted output */
    u_char	key[AES_256_BYTES];
    DMC_CRYPT_KEY *cp;
    bool	aes_trace = DMZ_REC_MACRO(5);

    CLRDBERR(dberr);

    if (Dmc_crypt == NULL)
    {
	i4 slots = 0;
	uleFormat( NULL, E_DM0179_DMC_CRYPT_SLOTS_EXHAUSTED,
	    (CL_ERR_DESC *)NULL,
	    ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 1,
	    sizeof(slots), &slots);
	SETDBERR(dberr, 0, E_DM0174_ENCRYPT_LOCKED);
	return (E_DB_ERROR);
    }

    if (!(t->tcb_rel.relencflags & TCB_ENCRYPTED))
	t = t->tcb_parent_tcb_ptr;	/* for secondary indices */
    cp = (DMC_CRYPT_KEY *)((PTR)Dmc_crypt + sizeof(DMC_CRYPT));
    cp += r->rcb_enckey_slot-1;		/* slot is 1-based */
    MEcopy((PTR)cp->key,sizeof(key),key);	/* cache it locally */
    if ( cp->status == DMC_CRYPT_ACTIVE &&
	 cp->db_id == t->tcb_dcb_ptr->dcb_id &&
	 cp->db_tab_base == t->tcb_rel.reltid.db_tab_base)
	; /* it is the best of all possible worlds */
    else
    {
	/* encryption not unlocked using MODIFY tbl ENCRYPT WITH PASSPHRASE= */
	SETDBERR(dberr, 0, E_DM0174_ENCRYPT_LOCKED);
	return (E_DB_ERROR);
    }

    if (t->tcb_rel.relencflags & TCB_AES128)
    {
	keybits = AES_128_BITS;
	keybytes = AES_128_BYTES;
    }
    else
    if (t->tcb_rel.relencflags & TCB_AES192)
    {
	keybits = AES_192_BITS;
	keybytes = AES_192_BYTES;
    }
    else
    if (t->tcb_rel.relencflags & TCB_AES256)
    {
	keybits = AES_256_BITS;
	keybytes = AES_256_BYTES;
    }
    else
    {
	/* internal error */
	SETDBERR(dberr, 0, E_DM0175_ENCRYPT_FLAG_ERROR);
	return (E_DB_ERROR);
    }

    nrounds = adu_rijndaelSetupEncrypt(rk, key, keybits);

    for (att = 0; att < rac->att_count; att++)
    {
	if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT)
	{
	    p = erec;
	    /* (0) clear the encrypted data area */
	    MEfill((u_i2)rac->att_ptrs[att]->encwid, 0, p);
	    /* (1) generate SALT */
	    if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_SALT)
	    {
		for (blk = 0 ; blk < (AES_BLOCK / sizeof(u_i4)) ; blk++)
		{
		    rand_i4 = MHrand2();
		    MEcopy(&rand_i4,sizeof(u_i4),p);
		    p += sizeof(u_i4);
		}
	    }
	    /* (2) move the data itself */
	    MEcopy(prec, rac->att_ptrs[att]->length, p);
	    p += rac->att_ptrs[att]->length;
	    /* (3) fill in the hash */
	    if (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_CRC)
	    {
		crc = -1;
		crclen = (rac->att_ptrs[att]->encflags & ATT_ENCRYPT_SALT)
		    ? rac->att_ptrs[att]->length + AES_BLOCK 
		    : rac->att_ptrs[att]->length;
		HSH_CRC32(erec, crclen, &crc);
		MEcopy((PTR)&crc, sizeof(crc), p);
	    }
	    /* (4) encrypt the data block by block */
	    p = erec;
	    pprevblk = NULL;
	    if (aes_trace)
		TRdisplay("\tAES %d-bit encrypt blocks:\n",keybits);
	    for ( blk = 0 ; blk < rac->att_ptrs[att]->encwid
			/ AES_BLOCK ; blk++ )
	    {
		/* CBC cipher mode */
		if (pprevblk)
		    for ( cbc = 0 ; cbc < AES_BLOCK ; cbc++ )
			p[cbc] ^= pprevblk[cbc];
		adu_rijndaelEncrypt(rk, nrounds, p, eout);
		if (aes_trace)
		    TRdisplay("\t    %< %4.4{%x%}%2< >%16.1{%c%}<\n", eout, 0);
		MEcopy(eout, AES_BLOCK, p);
		pprevblk = p;
		p += AES_BLOCK;
	    }
	    prec += rac->att_ptrs[att]->length;
	    erec += rac->att_ptrs[att]->encwid;
	}
	else
	{
	    MEcopy(prec,rac->att_ptrs[att]->length,erec);
	    prec += rac->att_ptrs[att]->length;
	    erec += rac->att_ptrs[att]->length;
	}
    }

    return(s);
}
