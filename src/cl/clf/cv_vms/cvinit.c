/*
**Copyright (c) 2010 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<cmcl.h>
#include	<cv.h>

/*
**  Name: CVINIT.C - Functions used to initialize/manipulate the CV function pointers
**
**  Description:
**      This file contains the following external routines:
**    
**      CVinit()             -  Modify CV function pointers based on character set
**
**  History:
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Created
*/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */

void
CVinit() 
{
        if (!(CMGETDBL))
        {
            CV_fvp.IICVal = CVal_SB;
            CV_fvp.IICVal8 = CVal8_SB;
            CV_fvp.IICVlower = CVlower_SB;
            CV_fvp.IICVupper = CVupper_SB;
        }
}
