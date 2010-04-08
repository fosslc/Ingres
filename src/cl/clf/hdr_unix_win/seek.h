/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SEEK.H - Codes for lseek(2) system call
**
** Description:
**      This files has definitions for things related to resource
**	usage measurement, private to the CL.
**
**
** History:
 * Revision 1.3  90/05/21  15:12:11  source
 * sc
 * 
 * Revision 1.2  90/05/17  18:36:22  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:07  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:41:33  source
 * sc
 * 
 * Revision 1.1  89/05/26  16:07:53  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:11:05  source
 * sc
 * 
 * Revision 1.1  89/03/07  00:38:02  source
 * sc
 * 
**      07-oct-88 (daveb)
**          SV doesn't define these in file.h, so define them here.
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Force error if <clconfig.h> has not been not included;
**		SysV doesn't define these in <file.h>; if <unistd.h>
**		exists use that, otherwise define them here.
**	21-may-90 (rog)
**	    Some <unistd.h>'s don't define SEEK_*.
**	20-feb-97 (muhpa01)
**	    Removed declaration for lseek() for hp8_us5 to fix "Inconsistent
**	    parameter list declaration" error.
**	12-Nov-1997 (allmi01)
**	    Bypassed definition of lseek() for sgi_us5 also. lseek is defined
**	    in unistd.h and this definition is in conflict with the system one. 
**      03-Nov-1997 (hanch04)
**	    Removed declaration for lseek() for su4_us5 to fix "Inconsistent
**	    parameter list declaration" error.
**      30-Jan-2001 (wansh01)
**	    Removed declaration for lseek() for dgi_us5 to fix "Inconsistent
**	    parameter list declaration" error.
**	23-Sep-2003 (hanje04)
**	    Do not declare lseek() for Linux
**	10-Dec-2004 (bonro01)
**	    lseek() is already defined in a64_sol
**	20-Apr-2005 (hanje04)
**	    Remove prototype of lseek(). It's not good practice
**	    to prototype sytem functions and most platforms
**	    exclude it anyway.
**/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before seek.h"
# endif

/*
** L_SET, LINCR and L_XTND can be derived from <unistd.h>, if it exists;
** otherwise define them here.
*/
# ifdef xCL_069_UNISTD_H_EXISTS
# include <unistd.h>
# if !defined(L_SET) && defined(SEEK_SET)
# define L_SET SEEK_SET
# endif /* L_SET */
# if !defined(L_INCR) && defined(SEEK_CUR)
# define L_INCR SEEK_CUR
# endif /* L_INCR */
# if !defined(L_XTND) && defined(SEEK_END)
# define L_XTND SEEK_END
# endif /* L_XTND */
# endif /* xCL_069_UNISTD_H_EXISTS */

# ifndef L_SET

# define	L_SET	0	/* absolute offset */
# define	L_INCR	1	/* relative to current offset */ 
# define	L_XTND	2	/* relative to end of file */

# endif

