/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**
**/

#include <compat.h>

#include <string.h>
#include <lib$routines.h>

#pragma intrinsic (memmove, memset)

int
IIMEcmp (char* str1, char* str2, u_i2 bytcnt)
{
    return (memcmp(str1, str2, bytcnt));
}

void
IIMEcopy (char* src, u_i2 bytcnt, char* dst)
{
    memmove (dst, src, bytcnt);
}

void
MEfill (i4 bytcnt, u_char fill_char, char* src)
{
    memset (src, fill_char, bytcnt);
}

void
IIMEmove (u_i2 srclen, char* src, char pad_char, u_i2 dstlen, char* dst)
{
    lib$movc5 (&srclen, src, &pad_char, &dstlen, dst);
}

