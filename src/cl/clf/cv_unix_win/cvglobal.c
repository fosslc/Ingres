/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <cs.h>
#include    <gl.h>
#include    <me.h>
#include    <cv.h>

/**
** Name: CVGLOBAL.C - Global variables for CV.
**
** Description:
**	Allocate storage for global variables in cv module.
**
LIBRARY = IMPCOMPATLIBDATA
**
** History:
 * Revision 1.1  90/03/09  09:14:23  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:39:57  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:35  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:20  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:21  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
 * Revision 1.1  87/11/10  12:37:26  mikem
 * Initial revision
 * 
**      Revision 1.3  87/07/27  11:20:46  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	08-jun-1995 (canor01)
**	    added thread-local storage for cv_errno in MCT server
**	03-jun-1996 (canor01)
**	    modified thread-local storage for use with operating-system
**	    threads.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Added CV_fvp
**/

/* externs */

GLOBALDEF i4	cv_errno = 0;	/* error status - set by exception 
				** catching routines 
				*/
# ifdef OS_THREADS_USED
GLOBALDEF	ME_TLS_KEY	cv_errno_key;
# endif /* OS_THREADS_USED */

GLOBALDEF CV_FUNCTIONS CV_fvp = { CVal_DB, CVal8_DB, CVlower_DB, CVupper_DB };


