/*
**Copyright (c) 2004 Ingres Corporation
*/
#ifndef DLINT_H

/*
** dlint.h -- Internal header file that DL internal routines might need.
**
** History:
**	25-jul-91 (jab)
**		Written
**	28-jan-92 (jab)
**		Modified (heavily) for CL-spec conformance.
**	16-mar-92 (jab)
**		Changed SunOS library name to ".so.2.0" because Sun said that they'd
**		STRONGLY suggest something like that.
**	4-Mar-92 (daveb/jab)
**		Correct prototype for DLparsedesc.
**	8-mar-92 (thomasm)
**		Add Rs/6000 to Sun entry to bump up library rev #
**	17-aug-92 (peterk)
**		Added inclusion of dlfcn.h file for su4_us5.
**	20-aug-1992 (jab)
**	     Added support for the working RS/6000 dynamic linking support(!).
**	26-aug-92 (vijay)
**		Add m88 includes and such. Make library revision number 2.0
**		eventhough this is the first w4gl port on this machine to make
**		it look consistent with other ports.
**      07-sep-92 (mwc)
**              Add DL extension for ICL Platforms.
**              Also, set shared object include header file.
**	20-nov-92 (pghosh)
**		Added nc5_us5 specific stuff to undefine DL_OLD_LOOKUP_FUNC 
**		and to include dlfcn.h.
**	15-mar-93 (www)
**		Added DL_EXT_NAME for su4_us5.
**      10-feb-1994 (ajc)
**          Add hp8_bls, which runs on HP 9000 HP-UX/BLS (a B1 secure) OS
**      09-feb-1995 (chech02)
**          Add rs4_us5 for AIX 4.1.
**	10-nov-1995 (murf)
**		Added sui_us5 to all areas specifically defining su4_us5.
**		The purpose to port solaris on intel.
**	17-oct-1996 (canor01)
**	    Remove 8-character filename limit on DL modules.
**      28-may-1997 (canor01)
**          Don't look for old extensions with su4_us5.
**	29-may-1997 (canor01)
**	    Clean up prototypes.
**	13-aug-1997 (canor01)
**	    Add 'flags' parameter to DLosprepare().
**	07-apr-1999 (walro03)
**	    Add DL_EXT_NAME for Sequent (sqs_ptx).
**	10-apr-1999 (popri01)
**	    For Unixware 7 (usl_us5), the shared library suffix (SLSFX) is "1.so".
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_uv1, dra_us5, nc5_us5.
**	13-Aug-1999 (schte01)
**	    For axp_osf, the shared library suffix (SLSFX) is "1.so".
**	23-aug-1999 (muhpa01)
**	    The shared library suffix for HP is ".1.sl"
**	26-oct-1999 (somsa01)
**	    The shared library suffix for HP is ".sl.2.0".
**	10-Dec-1999 (vande02)
**	    The shared library suffix for DG/UX is ".so.2.0".
**	10-dec-1999 (mosjo01)
**	    Treat sqs_ptx same as solaris (conventional) platforms (so.2.0).
**	10-Dec-1999 (hweho01)
**	    The shared library suffix for axp_osf is ".so.2.0".
**	13-Dec-1999 (hweho01)
**	    Add DL_EXT_NAME for AIX 64-bit platform (ris-u64).
**	02-May-2000 (ahaal01)
**	    The shared library suffix for AIX (rs4_us5) is ".a.2.0".
**	22-May-2000 (hweho01)
**	    The shared library suffix for ris_u64 is ".a.2.0".
**	14-Jul-2000 (hanje04)
**	    The shared library suffix for int_lnx is ".so.2.0".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	15-aug-2000 (hanje04)
**	    The shared library suffix for ibm_lnx is ".so.2.0".
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Sep-2000 (hanje04)
**	    The shared library suffix for axp_lnx is ".so.2.0".
**      28-Feb-2001 (musro02)
**          The shared library suffix for nc4_us5 is ".so.2.0".
**       1-mar-2001 (mosjo01)
**          Oops, should have paired sqs_ptx with su4_us5 not su4_u42.
**	27-Feb-2001 (wansh01)
**	    The shared library suffix for sgi_us5 is ".so.2.0".
**	18-Apr-2001 (bonro01)
**	    The shared library suffix for rmx_us5 is ".so.2.0".
**      11-june-2001 (legru01)
**          The shared library suffix for sos_us5 is ".so.2.0".
**      01-Mar-2001 (linke01)
**          modified the shared library suffix for usl_us5
**	15-Jun-2001 (hanje04)
**	    Tidyed up #ifdef to add shared lib suffixes. It was causing 
**	    compiler errors on Solaris.
**      22-Jun-2001 (linke01)
**          modified the shared library suffix for usl_us5         
**      28-Jun-2001 (linke01)
**          change made on 22-Jun-2001 by linke01 was not propagated 
**          correctly while submitted to piccolo. re-submite the change 
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**	03-Dec-2001 (hanje04)
**	    Added shared library suffix for IA64 Linux (i64_lnx)
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**      15-Mar-2005 (bonro01)
**          Added support for Solaris AMD64 a64_sol.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	20-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols.
*/

#define	DLINT_H


#if defined(VMS)
# define	DL_EXT_NAME	".exe"
#endif	/* VMS */

# if defined(su4_u42)
#  define	DL_EXT_NAME	".so.2.0"
#  define	DL_OLD_EXT_NAME	".so.1.0"
#endif	/* old Sun */

# if defined(any_aix)
#  define	DL_EXT_NAME	".a.2.0"
#  define	DL_OLD_EXT_NAME	".a.1.0"
# endif /* aix */

# if defined(m88_us5) || defined(sparc_sol) || defined(sui_us5) || \
     defined(dgi_us5) || defined(axp_osf) || defined(nc4_us5) || \
     defined(sqs_ptx) || defined(sgi_us5) || defined(rmx_us5) || \
     defined(sos_us5) || defined(usl_us5) || defined(LNX) || \
     defined(a64_sol)
#  define	DL_EXT_NAME	".so.2.0"
# endif /* motorola || solaris */

# if defined(any_hpux) || defined(hp3_us5) || defined(hp8_bls)
#  define	DL_EXT_NAME	".sl.2.0"
# endif	/* su4_u42 */

# if defined(dr6_us5)
#  define       DL_EXT_NAME     ".so"
# endif /* ICL */

# if defined(OSX)
#  define       DL_EXT_NAME     "2.0.dylib"
# endif /* ICL */

# ifndef DL_EXT_NAME
#  define	DL_EXT_NAME	".so"
# endif

#define	DL_TXT_NAME	".dsc"

#if defined(hp3_us5)
#define	DL_LOOKUP_FUNC	"_IIdllookupfunc"
#define	DL_OLD_LOOKUP_FUNC	"_IIU3rpaResolveProcAddr"
#else
#define	DL_LOOKUP_FUNC	"IIdllookupfunc"
#define	DL_OLD_LOOKUP_FUNC	"IIU3rpaResolveProcAddr"
#endif
#define	DLVERSNAME	"IIdlversion"

/* motorola and ncr does not have the older versions of w4gl */
#if defined(m88_us5)
# undef DL_OLD_LOOKUP_FUNC
#endif

#if defined(hp3_us5) || defined(any_hpux) || defined(hp8_bls)
#include "/usr/include/dl.h"
#include "/usr/include/shl.h"
#define	HPUXWORKAROUND
#endif

#if defined(su4_u42) || defined (m88_us5)
#include "/usr/include/dlfcn.h"
#endif

#if defined(dr6_us5) ||  defined(sparc_sol) || defined(usl_us5) || \
    defined(sui_us5) ||  defined(sos_us5)
#include <dlfcn.h>
#endif

#if defined(VMS)
#ifndef SETCLERR
#define SETCLERR(s, i, c) { (s)->error = i; }
#endif 
#endif

GLOBALREF int DLdebugset;
#define	DLdebug	if(DLdebugset == 1) SIprintf

#define	DL_valid	0x1234
#define	DL_invalid	0x4321
struct DLint {
	/* Integrity check:
	** "dl_check" is always set to one of DL_valid/DL_invalid
	*/
	i4	dl_check;
	bool dl_isoldversion;	/* W4GL05-vintage DL? */
	PTR	dl_cookie;/* OS cookie necessary for lookups/binds */
	/* Address of lookup function within DL module */
	STATUS	(*dl_lookupfcn)();
	char	dl_storedmodname[LO_FILENAME_MAX];
	char	dl_storedversionstr[BUFSIZ/2];
	char	dl_storeddatecreated[BUFSIZ/2];
	char	dl_exename[BUFSIZ/2];
	char	*dl_libs[BUFSIZ];
	i4		dl_lib_cnt;
	char	dl_liblist[BUFSIZ];
	char	*dl_funcs[BUFSIZ];
	i4		dl_func_cnt;
	char	dl_functionlist[BUFSIZ];
};

STATUS DLosprepare(
#ifdef CL_PROTOTYPED
	char *vers, 
	LOCATION *locp, 
	char *syms[],
	struct DLint *localareap, 
	i4 flags,
	CL_ERR_DESC *errp
#endif /* CL_PROTOTYPED */
);
STATUS DLosunload(
#ifdef CL_PROTOTYPED
	PTR cookie, 
	CL_ERR_DESC *errp
#endif	/* CL_PROTOTYPED */
);
STATUS DLparsedesc(
#ifdef CL_PROTOTYPED
	LOCATION *locp, 
	char *exename,
	char *dlmodname, 
	char *vers, 
	char *creationdate,
	char *functionbuf, 
	char *functions[], 
	i4 *func_cnt,
	char *libbuf, 
	char *libs[], 
	i4 *lib_cnt, 
	CL_ERR_DESC *errp
#endif	/* CL_PROTOTYPED */
);
#endif	/* DLINT_H */
