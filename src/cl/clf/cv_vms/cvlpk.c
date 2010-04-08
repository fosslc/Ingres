/*
**    Copyright (c) 1987, 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <cv.h>
#include    <mh.h>
#include    <me.h>
#include    <cm.h>

/* forward declarations */
FUNC_EXTERN STATUS CV0decerr();

/*{
 ** Name: CVlpk() - Convert long integer to packed decimal.
 **
 ** Description:
 **      Converts a four byte integer into a packed decimal value of the
 **      requested precision and scale.
 **
 ** Inputs:
 **      num                             The four byte integer.
 **      prec                            Precision for result:  1 <= prec <= 31.
 **      scale                           Scale for result:  0 <= scale <= prec.
 **
 ** Outputs:
 **      pk                              The resulting packed decimal value is
 **                                      placed here.
 **
 **      Returns:
 **          OK                          Success.
 **          CV_OVERFLOW                 Too many non-fractional digits;
 **                                      i.e. more than (prec - scale) of them.
 **
 **      Exceptions:
 **          none
 **
 ** Side Effects:
 **          none
 **
 ** History:
 **      10-may-88 (jrb)
 **          Initial creation and coding.
 **      24-jul-89 (jrb)
 **          Now zeroes result on overflow.
 **      30-dec-99 (kinte01)
 **          Clean up compiler warnings using ANSI C Compiler
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	8-Jun-2004 (schka24)
**	    Use newer/safer i8 typedef instead of longlong.
 */
 
 STATUS
 CVlpk(num, prec, scale, pk)
 i4          num;
 i4         prec;
 i4         scale;
 PTR         pk;
 {
     i4         pr = scale + 1;         /* no. of nybbles from the left */
     u_char      *p;                     /* points into pk */
 
     /* zero out entire decimal number */
     MEfill((u_i2)(prec/2+1), (u_char)0, pk);
 
     /* insert sign code into pk and make num negative if it isn't already   */
     /* (we convert everything to negative instead of positive because MINI4 */
     /* has no positive counterpart)                                         */
     if (num < 0)
     {
         *((u_char *)pk + prec/2) = MH_PK_MINUS;
     }
     else
     {
         *((u_char *)pk + prec/2) = MH_PK_PLUS;
         num = -num;
     }
 
     /* set p to byte containing rightmost nybble of integral part of pk */
     p = (u_char *)pk + prec/2 - pr/2;
 
     /* insert digits from right to left into pk until full or num is 0 */
     while (num < 0 && pr <= prec)
     {
         if (pr++ & 1)
             *p-- |= (u_char)((-(num % 10) << 4) & 0xf0);
         else
             *p = (u_char)(-(num % 10));
         
         num /= 10;
     }
 
     /* if there's anything left, it was too big to fit */
     if (num < 0)
         return(CV0decerr(CV_OVERFLOW, pk, prec));
 
     return(OK);
}


/*{
** Name: CVl8pk() - Convert 64-bit integer to packed decimal.
**
** Description:
**      Converts a eight byte integer into a packed decimal value of the
**      requested precision and scale.
**
** Inputs:
**      num                             The eight byte integer.
**      prec                            Precision for result:  1 <= prec <= 31.
**      scale                           Scale for result:  0 <= scale <= prec.
**
** Outputs:
**      pk                              The resulting packed decimal value is
**                                      placed here.
**
**      Returns:
**          OK                          Success.
**          CV_OVERFLOW                 Too many non-fractional digits;
**                                      i.e. more than (prec - scale) of them.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      23-dec-03 (inkdo01)
**	    Cloned from CVlpk().
*/
STATUS
CVl8pk(
i8		num,
i4		prec,
i4		scale,
PTR		pk)
{
     i4         pr = scale + 1;         /* no. of nybbles from the left */
     u_char      *p;                     /* points into pk */
 
     /* zero out entire decimal number */
     MEfill((u_i2)(prec/2+1), (u_char)0, pk);
 
     /* insert sign code into pk and make num negative if it isn't already   */
     /* (we convert everything to negative instead of positive because MINI4 */
     /* has no positive counterpart)                                         */
     if (num < 0)
     {
         *((u_char *)pk + prec/2) = MH_PK_MINUS;
     }
     else
     {
         *((u_char *)pk + prec/2) = MH_PK_PLUS;
         num = -num;
     }
 
     /* set p to byte containing rightmost nybble of integral part of pk */
     p = (u_char *)pk + prec/2 - pr/2;
 
     /* insert digits from right to left into pk until full or num is 0 */
     while (num < 0 && pr <= prec)
     {
         if (pr++ & 1)
             *p-- |= (u_char)((-(num % 10) << 4) & 0xf0);
         else
             *p = (u_char)(-(num % 10));
         
         num /= 10;
     }
 
     /* if there's anything left, it was too big to fit */
     if (num < 0)
         return(CV0decerr(CV_OVERFLOW, pk, prec));
 
     return(OK);
}
