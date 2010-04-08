/*
**  (C) Copyright 1993,2003 Ingres Corporation
*/

#include <compat.h>
#include <idmsucry.h>

/*
//  Module Name: ENCRYPT.C
//
//  Description: Password encrytion routines
//
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//  12/15/1993  Dave Ross          Cloned from PWCRYPT.C, minor changes:
//                                 1. don't #include PLATFORM.H
//                                 2. MEDIUM model prototype
//  02/17/1999  Dave Thole         Ported to UNIX
//  06/06/2000  Dave Thole         Correct password encryption
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
*/

/*
 *---------------------------------------------------------------------------
 * MODULE NAME: ENCRYPT.C
 * DESCRIPTION: pwcrypt (username, password) encrypts or decrypts the
 *              32-character password. The 32-character username is used as a
 *              key seed. The encryption algorithm is bistable, such that to
 *              encrypt twice results in the original clear password, and to
 *              encrypt a third time results in the same encryption as the
 *              first time. Thus, to decrypt an encrypted password, just call
 *              pwcrypt again.
 *
 *              pwcrypt returns its result by overwriting the original
 *              password.
 *
 *---------------------------------------------------------------------------
 *                  M O D I F I C A T I O N   H I S T O R Y
 *---------------------------------------------------------------------------
 * PROGRAMMER.:     Bharat Kallianpurkar
 * DATE START.:     08/15/91      DATE END:     08/15/91
 * DESCRIPTION:     VAX PORT 2nd Pass.
 *---------------------------------------------------------------------------
 */


static const int smoke[] =
{
 99,170,3,4,33,222,44,51,13,76,81,79,44,43,250,1,
 7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 187,153,123,177,165,249,255,88,231,54,67,22,221,33,32,78,91,93,107,21,
 7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 55,43,89,231,108,198,196,0,4,5,44,55,67,80,31,43,65,181,104,134,142,131,
 233,243,191,3,13,6,81,235,82,203,211,111,110,99,33,1,2,4,77,144,176,125,
 63,154,155,187,194,153,203,113,117,173,184,166,102,22,190,191,3,6,66,161,
 67,7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 187,153,123,177,165,249,255,88,231,54,67,22,221,33,32,78,91,93,107,21,
 7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 165,55,43,89,231,108,198,196,0,4,5,44,55,67,80,31,43,65,181,104,134,142,131,
 233,243,191,3,13,6,81,235,82,203,211,111,110,99,33,1,2,4,77,144,176,125,
 63,154,155,187,194,153,203,113,117,173,184,166,102,22,190,191,3,6,66,161,
 7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 187,153,123,177,165,249,255,88,231,54,67,22,221,33,32,78,91,93,107,21,
 101,103,102,7,180,160,199,65,90,21,27,84,56,72,93,87,247,147,101,2,33,77,121,
 55,43,89,231,108,198,196,0,4,5,44,55,67,80,31,43,65,181,104,134,142,131,
 233,243,191,3,13,6,81,235,82,203,211,111,110,99,33,1,2,4,77,144,176,125,
 63,154,155,187,194,153,203,113,117,173,184,166,102,22,190,191,3,6,66,161,
 13,171,132,198,2,67,62,79,77,35,82,251,97,68,109,119,179,169,104,124,213,
};

int pwcrypt (const char * key, char * pw)       /* change from PWCRYPT */
{
    int i, ismoke;
    uchar * ukey = (uchar*)key;          /* avoid char arithmetic problems */

    for (i = ismoke = 0; i<32; i++)
        ismoke += ukey[i];
    ismoke %= 256;

    for (i=0; i<32; i++)
        pw[i] ^= smoke[ismoke+i];
        return 0;
}
