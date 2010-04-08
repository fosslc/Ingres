#if defined(__vms)
#ifndef __NEW_STARLET
#define __NEW_STARLET
#endif

#include <ssdef>
#include <lib$routines>

/*
** Define the bitmask of return status codes from the OTS$ routines which
** emulate the VAX instructions. They return the same bits as in the VAX PSL
*/

typedef struct _VAXPSL
{
    unsigned vaxpsl$v_c         : 1;    // 0 carry
    unsigned vaxpsl$v_v         : 1;    // 1 overflow
    unsigned vaxpsl$v_z         : 1;    // 2 zero
    unsigned vaxpsl$v_n         : 1;    // 3 negative
    unsigned vaxpsl$v_fill_4    : 1;    // 4
    unsigned vaxpsl$v_iv        : 1;    // 5
    unsigned vaxpsl$v_fu        : 1;    // 6
    unsigned vaxpsl$v_fill_7    : 1;    // 7
    unsigned vaxpsl$v_fill_8_31 : 24;   // 8-31
} VAXPSL;

extern VAXPSL ots$cvtsp();
extern VAXPSL ots$cvtps();
extern VAXPSL ots$ashp();

/*
** IINTOP -- convert leading numeric separate to packed decimal
**
**       IIntop (str, slen, pack, plen)
**
**       Parameters:
**               str  -- pointer to leading numeric separate string
**               slen -- length of string
**               pack -- pointer to buffer to hold packed decimal string
**               plen -- length of packed decimal string
**       Returns:
**               pack
**
*/

void   IIntop (char* str, int slen, char* pack, int plen)
{
    VAXPSL sts = ots$cvtsp(&slen, str, &plen, pack);
//  if (sts.vaxpsl$v_v) lib$signal(SS$_DECOVF);
}

/*
** IIPTON -- convert packed decimal to leading numeric separate
**           IIptod (pack, plwn, str, slen)
**
**         Parameters:
**                 pack -- pointer to buffer to hold packed decimal string
**                 plen -- length of packed decimal string
**                 str  -- pointer to leading numeric separate string
**                 slen -- length of string
**         Returns:
**                str
**
*/

void IIpton (char* pack, int plen, char* str, int slen)
{
    VAXPSL sts = ots$cvtps (&plen, pack, &slen, str);
//  if (sts.vaxpsl$v_v) lib$signal(SS$_DECOVF);
}

/*
**
**  IIpscale -- scale a packed decimal number by power of ten
**
**       IIpscale (srcp, srcl, dstp, dstl,scale, round)
**
**       Parameters:
**               srcp  -- packed decimal source string           0
**               srcl  -- source string length                   4
**               dstp  -- packed decimal destination string      8
**               dstl  -- destination string length             12
**               scale -- powers of ten to scale by             16
**               round -- rounding factor for negative scaling  20
**
*/

void IIpscale (char* srcp, int srcl, char* dstp, int dstl, int scale, int round)
{
    VAXPSL sts = ots$ashp (&scale, &srcl, srcp, &round, &dstl, dstp);
//  if (sts.vaxpsl$v_v) lib$signal(SS$_DECOVF);
}


#ifdef TEST_ME
int main (int argc, char* argv[])
{
    return (SS$_NORMAL);
}

#endif


#endif /* __vms */
