/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** clnfile.h -- CL header to define CL_NFILE()
**
** History:
**	09-may-89 (daveb)
**	    This used to be in si.h, where it didn't belong.  
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add special cases for dr6_us5 and x3bx_us5, both of which 
**		have _NFILE incorrectly defined in <stdio.h>.
**	25-mar-91 (kirke)
**	    Added CL_NFILE() for hp8_us5.  Note that this only applies to
**	    HP-UX 8.0 and above.  Remove this definition if compiling for
**	    earlier releases of HP-UX.
**	02-jul-91 (johnr)
**	    Also need CL_NFILE() for hp3_us3.
**	07-jul-92 (swm)
**	    Bug #39301. Pickup kernel configured file descriptor limit for
**	    DRS6000 and DRS3000.
**	08-feb-93 (sweeney)
**	    usl_us5 also uses ulimit() to get max open files count.
**	26-jul-93 (mikem)
**	    Export the prototype for iiCL_get_fd_table_size().;
**	10-feb-1994 (ajc)
**	    Added hp8_bls specific entries based on hp8*
**	20-mar-1995 (smiba01)
**	    Added dr6_ues entries based on dr6_us5
**	1-may-95 (wadag01)
**	    Aligned with 6.4/05 (odt.sco/02):
**	    "20-nov-91 (rudyw)
**           Add odt_us5 to section defining CL_NFILE() in terms of ulimit().
**           The default keys on _NFILE which is present but not correct on SCO"
**	14-jul-1995 (morayf)
**	    Make sos_us5 like usl_us5 for CL_NFILE() with ulimit(UL_GDESLIM,0L)
**	    as we have the constant in (sys/)ulimit.h.
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dr6_uv1, dra_us5, odt_us5,
**	    x3bx_us5.
**      03-jul-99 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# ifndef CLCONFIG_H_INCLUDED
	# error "didn't include clconfig.h before clnfile.h"
# endif

# ifndef CL_NFILE
# ifdef xCL_004_GETDTABLESIZE_EXISTS
# 	define CL_NFILE()	getdtablesize()
# else
# 	include <si.h>
#	ifdef _NFILE
#		define CL_NFILE()	_NFILE
#	endif
#	ifdef gld_u42
#		include <systypes.h>
#		include <sys/param.h>
#		define CL_NFILE()	NOFILE
#	endif
#       if defined(dr6_us5) || defined(usl_us5) || defined(sos_us5)
#		include <ulimit.h>
#               ifdef CL_NFILE
#                       undef CL_NFILE
#               endif
#               define CL_NFILE()       ((int)ulimit(UL_GDESLIM, 0L))
#       endif /* dr6_us5 usl_us5 */
#	if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls) || \
	   defined(rmx_us5) || defined(rux_us5)
#		ifdef CL_NFILE
#			undef CL_NFILE
#		endif
#		include <unistd.h>
#		define CL_NFILE()	((int)sysconf(_SC_OPEN_MAX))
#	endif /* hp3_us5, hpux, hp8_bls or rmx_us5 */
# endif /* getdtablesize */
# endif /* CL_NFILE */

# ifndef CL_NFILE
	# error "Can't make a definition of CL_NFILE!"
# endif


/*
**  Forward and/or External function references.
*/
 
FUNC_EXTERN i4 iiCL_get_fd_table_size(void);
