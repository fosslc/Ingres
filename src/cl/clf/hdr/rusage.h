/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/

/**
** Name: RUSAGE.H - CL private resource measurement
**
** Description:
**      This files has definitions for things related to resource
**	usage measurement, private to the CL.
**
**
** History:
 * Revision 1.4  90/11/19  09:16:05  source
 * sc
 * 
 * Revision 1.3  90/10/08  16:05:13  source
 * sc
 * 
 * Revision 1.2  90/05/17  18:36:19  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:15:07  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:41:29  source
 * sc
 * 
 * Revision 1.2  90/02/01  14:20:27  source
 * sc
 * 
 * Revision 1.1  89/05/26  16:07:49  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:11:02  source
 * sc
 * 
 * Revision 1.3  89/03/21  17:46:30  source
 * sc
 * 
 * Revision 1.2  89/03/07  15:16:10  source
 * sc
 * 
 * Revision 1.1  89/03/07  00:37:59  source
 * sc
 * 
 * Revision 1.2  88/10/17  17:33:41  source
 * sc
 * 
 * Revision 1.1  88/10/12  15:53:21  source
 * sc
 * 
**      07-oct-88 (daveb)
**          Split out from tm/tmlocal.h, because rusage stuff is 
**	    also used in cs.
**	17-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Include time header files for dr3_us4, dr4_us5, sco_us5, 
**		odt_us5, x3bx_us5, sqs_us5 as well as a default case;
**		Include <sys/resource.h> if xCL_034_GETRLIMIT_EXISTS is
**		defined.
**	13-Nov-90 (jkb)
**	    Include sys/resource.h for sqs_ptx to define PROC_PRIOAGE to
**	    allow process aging to be turned off
**	21-mar-91 (seng)
**	    Include both <sys/time.h> and <time.h> for AIX 3.1
**      06-Jun-91 (hhsiung)
**          Include both <sys/time.h> and <time.h> for bu2 or bu3,
**          and remove old bu2 related code to include time.h only.
**	20-jun-95 (emmag)
**	    Desktop porting changes.
**      08-jul-91 (szeng)
**          Solved the LK_RETRY name confilction between lk.h and
**          DECstation.
**	20-jun-95 (emmag)
**	    Desktop porting changes.
**      14-jul-95 (morayf)
**	    Added SCO sos_us5 to those needing time header file.
**	11-oct-95 (tutto01)
**	    Added NT_ALPHA in addition to NT_INTEL.
**	04-Dec-1997 (allmi01)
**	    Added sgi specific undef's before the expansion of sys/resource.h.
**	10-may-1999 (walro03)
**	    Remove obsolete version string bu2_us5, bu3_us5, dr3_us5, dr4_us5,
**	    odt_us5, sqs_u42, sqs_us5, vax_ulx, x3bx_us5.
**      14-may-1999 (hweho01)
**	    Added ris_u64 to those needing time header file.
**	07-feb-2001 (somsa01)
**	    Added porting changes for i64_win.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	01-dec-2008 (joea)
**	    Do not #include <sys/param.h> on VMS.    
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	23-Nov-2010 (kschendel)
**	    Drop a couple more obsolete ports.
**/


# ifndef CLCONFIG_H_INCLUDED
error "You didn't include clconfig.h before including rusage.h"
# endif

# if defined(any_aix)
# include    <sys/time.h>
# include    <time.h>
# define GOT_TIMEVAL
# endif	/* aix */

# if !defined(GOT_TIMEVAL)	/* default */
#   if defined(xCL_015_SYS_TIME_H_EXISTS) && !defined (DESKTOP)  
#     include <sys/time.h>
#   else
#     include     <time.h>
#   endif	/* xCL_015_SYS_TIME_H_EXISTS */
# endif /* !defined(GOT_TIMEVAL) */

# if defined(xCL_003_GETRUSAGE_EXISTS) || defined(xCL_034_GETRLIMIT_EXISTS) 

# if defined(sgi_us5)
# undef _ABI_SOURCE
# endif

# include	<sys/resource.h>		/* header for timing info */

# if defined(sgi_us5)
# define _ABI_SOURCE
# endif

# endif

/* always need these */

# if defined(NT_GENERIC)
# include 	<time.h>
# else  /* NT_GENERIC */
# include	<sys/times.h>			/* for struct tms */
# endif /* NT_GENERIC */


# define	MIN_PER_HR	60
# define	SEC_PER_MIN	60
# define	MICRO_PER_MILLI	1000
# define	MICRO_PER_SEC	1000000
# define	MILLI_PER_SEC	1000

