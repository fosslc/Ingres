/*
** Copyright (c) 2010 Ingres Corporation
*/

/*
**
** Name: ADURIJNDAEL.H - Rijndael encryption and decryption.
**
** This is the header companion to adurijndael.c, q.v. for further info.
**
**  History:
**	14-dec-2009 (toumi01) SIR 122403
**	    Created.
*/

#ifndef H__RIJNDAEL
#define H__RIJNDAEL

i4 adu_rijndaelSetupEncrypt(u_i4 *rk, const unsigned char *key,
  i4 keybits);
i4 adu_rijndaelSetupDecrypt(u_i4 *rk, const unsigned char *key,
  i4 keybits);
void adu_rijndaelEncrypt(const u_i4 *rk, i4 nrounds,
  const unsigned char plaintext[16], unsigned char ciphertext[16]);
void adu_rijndaelDecrypt(const u_i4 *rk, i4 nrounds,
  const unsigned char ciphertext[16], unsigned char plaintext[16]);

#define KEYLENGTH(keybits) ((keybits)/8)
#define RKLENGTH(keybits)  ((keybits)/8+28)
#define NROUNDS(keybits)   ((keybits)/32+6)

#endif
