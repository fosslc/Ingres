/*
**Copyright (c) 2004, 2007 Ingres Corporation
** All Rights Reserved.
**
**
** History:
**	1/92 (jab)
**	    Created
**	3/16/92 (jab)
**	    At J. Wong's suggestion, put in a little more debugging for
**	    the Sun platform.
**	20-aug-1992 (jab)
**	    Added support for the working RS/6000 dynamic linking support(!).
**	21-aug-1992 (jab)
**	    Correcting a flag to "load" in the RS/6000 stuff.
**	21-aug-1992 (jab)
**	    Minor bits of cleanup on HP platform: output error messages
**	    when they're needed, since no FE does it [yet]. This is a tough
**	    choice: go outside of the CL standards in order grant a timely
**	    Tech Support request ("please have it tell us when it fails what
**	    was wrong!"), or to try to make the FE report it correctly.
**	    Prudence, indeed, will dictate that given the choice between
**	    doing nothing or adding a "perror" in a couple of places, that
**	    this will provide less calls to Tech Support.
**	26-Aug-92 (vijay)
**	    Add motorola defines which are similar to Sun. Use 
**	    mode RTLD_LAZY for dlopen.
**	07-sep-92 (mwc)
**	    Added low level loading calls for ICL DRS machines.
**	    This now follows su4_u42 method.
**	30-oct-92 (jab)
**	    Adding support for W4GL05-created routines on the HP3,
**	    which just wasn't there.
**	20-nov-92 (pghosh)
**	    Added nc5_us5 specific dlopen() and dlclose() which is simillar
**	    to sun one.
**	11-sep-92 (johnst)
**	    Added a dra_us5 entry to use dlopen() mode RTLD_NOW instead of
**	    the normal RTLD_LAZY, to fix dynamic linker bug when using
**	    position independent code to build user 3gl shared objects.
**	    At (jab)'s suggestion, this change was implemented by defining
**	    a new symbol RTLD_ARG based on the porting client, and using
**	    RTLD_ARG as the dlopen() mode argument.
**	02-Feb-93 (Farzin Barazandeh)
**	    Added dg8_us5 for DL_LOADING_WORKS.
**	12-mar-93 (www)
**	    Added su4_us5.
**	26-apr-93 (jab)
**	    Pulling out the DG change that was inappropriately added (never
**	    approved and against the design of this file). You shouldn't need
**	    to call the OS-specific routine DURING the symbol resolution itself,
**	    because of performance problems doing so, and because it's not
**	    compatible with the other platforms.
**  	31-aug-93 (jab)
**	    Integrating the following changes from the 'ingres' line:
**	4-Mar-92 (daveb)
**	    Cast localareap to PTR in call to DLunload.  Hey, this
**	    file doesn't have a proper file header!
**	01-feb-93 (sweeney)
**		USL destiny is another SVR4 port - same as dra_us5.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	06-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in DLosprepare().
**	03-mar-94 (ajc)
**	    Added hp8_bls specific code based on hp8* and cleaned up comments.
**      12-Oct-94 (canor01)
**          corrected casting for loadbind() and unload() for ris_us5 (AIX 4.1)
**      09-feb-95 (chech02)
**          Added rs4_us5 for AIX 4.1.
**	19-apr-95 (canor01)
**	    added <errno.h>
**	22-jun-95 (allmi01)
**	    Added dgi_us5 support for DG-UX on Intel platforms following dg8_us5.
**
**	10-nov-1995 (murf)
**		Added sui_us5 to all areas specifically defined with su4_us5.
**		Reason, to port Solaris for Intel.
**	03-oct-1996 (canor01)
**	    If the user wishes to bypass the date and version checking and
**	    doesn't include a lookup function, use the system lookup function.
**	21-oct-1996 (canor01)
**	    If dlopen() fails (Solaris), fill CL_ERR_DESC structure with
**	    additional information gathered from the system.
**	13-aug-1997 (canor01)
**	    Allow flags to pass symbol relocation preferences:
**	    DL_RELOC_IMMEDIATE or DL_RELOC_DEFERRED.
**	31-Mar-98 (gordy)
**	    Fixed HP-UX call to shl_findsym which requires a handle pointer.
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_uv1, dra_us5, enc_us5, nc5_us5.
**	21-jun-1999 (muhpa01)
**	    Added DYNAMIC_PATH flag for shl_findsym() on HP.  Also, added
**	    cast to DLusertab[i].procptr to quiet compiler warning.
**	27-Jul-1999 (schte01)
**	    Added axp_osf to list of platforms using dlopen.
**	10-Dec-1999 (mosjo01)
**	    Added sqs_ptx to list of platforms using dlopen, etc.
**      13-Dec-1999 (hweho01)
**          Added support for AIX 64-bit platform (ris_u64).
**	22-May-2000 (hweho01)
**	    Added ris_u64 to list of platforms using dlopen, etc.
**	29-Jun-2000 (ahaal01)
**	    Added rs4_us5 to list of platforms using dlopen, etc.
**	14-Jul-2000 (hanje04)
**	    Added int_lnx to list of platforms using dl.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	15-aug-2000 (hanje04)
**	    Added ibm_lnx to list of platforms using dl.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Sep-2000 (hanje04)
**	    Added axp_lnx to list of platforms using dl.
**      28-Feb-2001 (musro02)
**         Added nc4_us5 to list of platforms using dl.
**	27-Feb-2001 (wansh01)
**	    Added sgi_us5 to list of platforms using dl.
**	18-Apr-2001 (bonro01)
**	    Added rmx_us5 to list of platforms using dlopen
**      11-june-2001 (legru01)
**          Added sos_us5 to list of platforms using dlopen   
**      13-Mar-2001 (linke01)
**           include <dlfcn.h> for usl_us5 
**      22-Jun-2001 (linke01)
**          Added usl_us5 to list of platforms using dlopen
**      28-Jun-2001 (linke01)
**         when submitted change to piccolo, it was propagated incorrectly,
**         re-submit the change. (this is to modify change#451092).
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
** 	03-Dec-2001 (hanje04)
**	    Added support for IA64 Linux (i64_lnx)
**	08-aug-2002 (somsa01)
**	    Changes for HP-UX 64-bit. HP is going the SVR4 route. Thus,
**	    for 64-bit dynamic loads we look at LD_LIBRARY_PATH first, before
**	    SHLIB_PATH. Also, we can now use the dl family of OS functions
**	    for manipulating shared libraries.
**      08-Oct-2002 (hanje04)
**          As part of AMD x86-64 Linux port, replace individual Linux
**          defines with generic LNX define where apropriate.
**      15-Mar-2005 (bonro01)
**          Added support for Solaris AMD64 a64_sol.
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Based on initial changes by utlia01 and monda07.
**	    Removed defn of dlerror() in favour of system definition in
**	    dlfcn.h
**	22-Apr-2005 (bonro01)
**	    Fix DL support for Solaris a64_sol.
**      11-Jun-2007 (Ralph Loen) Bug 118482
**          For HP platforms, in DLosprepare(), retain the module handle from 
**          shl_load() if the "lookupfunc" is not found.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	25-Aug-2009 (frima01) Bug 122138
**	    Modified last change, added include dlfcn.h for i64_hpu too.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
*/
#include <compat.h>
#include <gl.h>
#ifdef UNIX
#include <systypes.h>
#include <clconfig.h>
#endif
#if defined(VMS)
# include "sys$library:descrip.h"
# include "sys$library:ssdef.h"
# include "sys$library:lnmdef.h"
# include "sys$library:libdef.h"
#endif
#include <errno.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <lo.h>
#include <nm.h>
#include <si.h>
#include <st.h>

#include <dl.h>
#include	"dlint.h"

#if defined(axp_osf) || defined(any_aix) || defined(i64_hpu) || \
    defined(sgi_us5) || defined(usl_us5) || defined(hp2_us5) || \
    defined(LNX) || defined(a64_sol) || defined(OSX)
#include <dlfcn.h>
#endif



/* Determine the correct dlopen() mode option to be used. */
#	define  RTLD_DEF_ARG	RTLD_LAZY     /* deferred */
#	define  RTLD_IMM_ARG	RTLD_NOW      /* immediate */
#	define  RTLD_ARG	RTLD_DEF_ARG  /* default */

#if defined(VMS)
/* Itemlist structure for system service calls */
typedef struct itmlist
{
	i2 buflen;
	i2 itmcode;
	i4 bufadr;
	i4 retadr;
} ITEMLIST;

#define DLMOD_LOGNAME	ERx("II_DL_LOADMODULE")
#define PROCLNT		ERx("LNM$PROCESS_TABLE")
#define DEFAULTDIR	ERx("SYS$DISK:[]")

/*
** Condition handler that handles everything
*/
static
i4 handle_all()
{
	return SS$_CONTINUE;
}
#endif

#ifdef DL_OLD_LOOKUP_FUNC
static struct { char *procname; long *procptr; } *DLusertab = NULL;
static long *DLprocnum = NULL;
long *II3GLsyms = NULL;
#if defined(hpb_us5)
#define	prependunderscore(x)	(x)
#endif	/* hpb */

static char *(*oldlookupfcn)() = NULL;

static STATUS hp3retrofitlookup(cookiep, name, locp)
void *cookiep;
char *name;
char **locp;
{
	char *addr;
	if (oldlookupfcn == NULL)
	{
		return(FAIL);
	}
	addr = (VOID *)(*oldlookupfcn)(name);
	if (addr == NULL)
		return(FAIL);
	*locp = addr;
	return(OK);
}

static STATUS retrofitlookup(cookiep, name, locp)
void *cookiep;
char *name;
char **locp;
{
	char *addr;
	if (oldlookupfcn == NULL)
	{
#if defined(hpb_us5)
/*
** Things to consider for retrofit code for HP8/HP3:
**		1. On W4GL05, underscores weren't automagically prepended
**		   to the name. Don't "fix" it for the user expecting W4GL05 behavior.
**		2. On W4GL05 for certain ports, the data structures were in the
**		   dynamic library but the lookup routine wasn't. Luckily, the
**		   names of the data structures didn't change, so we can use this
**		   cop-out for either case (func there, func not there) by looking
**		   up the old names and using them.
**		3. We use a binary search for the older file because it's what
**		   W4GL05 used.
*/
		if (DLusertab != NULL && DLprocnum != NULL && *locp != NULL)
		{
			int l = 0, u = *DLprocnum - 1;
			while (l <= u)
			{
				int i = (l+u)/2;	/* Get midpoint */
				int ret = strcmp(name, DLusertab[i].procname);
				if (ret == 0)
				{
					*locp = (PTR)DLusertab[i].procptr;
					return(OK);
				}
				else if (ret < 0)
					u = i - 1;
				else
					l = i + 1;
			}
		}
/* Otherwise, return "FAIL" - either the search failed, or there
** weren't reasonable-enough pointers given as input.
*/
#endif	/* hp8 || hp3 */
		return(FAIL);
	}
	addr = (*oldlookupfcn)(name);
	if (addr == NULL)
		return(FAIL);
	*locp = addr;
	return(OK);
}
#endif	/* ifdef DL_OLD_LOOKUP_FUNC */
static STATUS
iiDLsymfunc( void *handle, char *symname, void **retaddr )
{
#if defined(sparc_sol) || defined(usl_us5) || defined(axp_osf) || \
    defined(any_aix) || defined(sgi_us5) || \
    defined(hp2_us5) || defined(i64_hpu) || \
    defined(a64_sol) || defined(LNX) || defined(OSX)

    if ( (*retaddr = dlsym( handle, symname )) == NULL )
	return (FAIL);
    return (OK);

# endif /* sol, etc */
#	if defined(hp8_us5) && !defined(hp2_us5)

	if ( shl_findsym( (shl_t *)&handle, symname, TYPE_UNDEFINED, retaddr) == 0 )
	    return ( OK );
	else
	    return ( FAIL );
	    
# endif
}
/*
** Name:	DLosprepare - OS-specific portion of DLprepare
**
** Description:
**	Given location, which we KNOW exists, load into memory using
**	OS-specific DL mechanims.
**	1. If a version string was specified, assume that the user wants
**	   to check versioning and look for DL_VERS_NAME. If it's not there
**	   or isn't identical to what the user wanted, return error.
**	2. On any error, the library shouldn't be left loaded if there's
**	   any way around it.
**
** Inputs:
**	vers		Version string to be checked, if any
**	locp		Location of library/module to be attached to.
**	syms		DLSYMBOL structs specifying names to be found in
**			attached file;
**	localareap	An internal structure containing any OS-specific
**			information [that we might need] for the OSbind.
**	flags		preferences for symbol relocation:
**			    DL_RELOC_IMMEDIATE
**			    DL_RELOC_DEFERRED
**			    DL_RELOC_DEFAULT
** Outputs:
**	handle		An OS-specific cookie that might be need for
**			later. On hp8_us5, the library handle.
**	localareap	An internal structure containing any OS-specific
**			information [that we might need] for the OSbind.
**			"dl_cookie" is filled in during this call.
**	errp		CL_ERR_DESC that will be filled in on error.
** Returns:
**	OK if found some file with that name was found, non-OK otherwise.
** Side effects:
**	None.
*/
/* ARGSUSED */
STATUS
DLosprepare(
	char *vers,
	LOCATION *locp,
	char *syms[],
	struct DLint *localareap,
	i4 flags,
	CL_ERR_DESC *errp )
{
	char *shlibname;
	PTR cookie = NULL;
	bool haslookup = FALSE;
	STATUS (*lookupfcn)() = NULL;
	int mode;

	LOtos(locp, &shlibname);
/*
** Step 1: Load the library. Note that "shlibname" is the ASCII
** version of that name, e.g. "/usr/lib/xxx.sl" or "dra1:[xx.yy]thing.lib;2".
*/
#if	!DL_LOADING_WORKS
	SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
	return(DL_NOT_IMPLEMENTED);
#else	/* got it over with */

	DLdebug("DL: attempting to load \"%s\"\n", shlibname);
#if defined(sparc_sol) || defined(usl_us5) || defined(axp_osf) || \
    defined(any_aix) || \
    defined(sgi_us5) || defined(hp2_us5) || defined(i64_hpu) || \
    defined(a64_sol) || defined(LNX) || defined(OSX)
	{
		/*
		** The second argument to "dlopen" must always be a '1' (LAZY),
		** according to Sun documentation.
		*/
		if ( flags & DL_RELOC_IMMEDIATE )
		    mode = RTLD_IMM_ARG;
		else if ( flags & DL_RELOC_DEFERRED )
		    mode = RTLD_DEF_ARG;
		else
		    mode = RTLD_ARG;
		cookie = localareap->dl_cookie = dlopen(shlibname, mode);
		if (cookie == NULL)
		{
			if (DLdebugset == 1)
			{
				DLdebug("%s: %s\n", shlibname, dlerror());
			}
			SETCLERR(errp,  DL_OSLOAD_FAIL, 0);
            		STcopy( dlerror(), errp->moreinfo[0].data.string );
            		errp->moreinfo[0].size = 
			    STlength(errp->moreinfo[0].data.string);
			return(DL_OSLOAD_FAIL);
		}
#if defined(DL_OLD_LOOKUP_FUNC)
/*
** Let's look hard at the logic here: if we're interested in finding
** an old lookup function, then...
**
**	1. Try to find it, if it's there.
**     a.If so, use it (which will disable some of the internal
**       version checking and the debug printf that prints out
**       the date-created and so on, which weren't stored back then.)
**     b. If not, and this was an older-style filename, return an error.
**     c. Otherwise try to find the new lookup function to use.
**
** Note that this is Sun-specific right now. We can easily duplicate
** this functionality for other platforms, but Sun (and maybe VMS) are
** the only ones that might have older versions "to live with" that use
** the older names.
*/
		if ((oldlookupfcn = (char *(*) ())dlsym(cookie,
				DL_OLD_LOOKUP_FUNC)) != NULL)
		{
			lookupfcn = retrofitlookup;
			haslookup = TRUE;
		}
		else
		{
		    if (localareap->dl_isoldversion)
		    {
			    _VOID_ dlclose(cookie);
			    cookie = NULL;
			    SETCLERR(errp, DL_BAD_LOOKUPFCN, 0);
			    return(DL_BAD_LOOKUPFCN);
		    }
#endif	/* otherwise, only find the new function. */
		    if ((lookupfcn = 
			    (STATUS (*) ())dlsym(cookie, DL_LOOKUP_FUNC)) == NULL)
		    {
			lookupfcn = iiDLsymfunc;
		    }
		    else
		    {
			haslookup = TRUE;
		    }
#if defined(DL_OLD_LOOKUP_FUNC)
	   }	/* end if-then-else */
#endif	/* if defined(DL_OLD_LOOKUP_FUNC) */
	} /* the block of the su4_u42 code */
#endif /* sol etc */
#	if defined(hpb_us5)
	{
		shl_t ret;
		long val;
		int i;

		for (i = 0; i < localareap->dl_lib_cnt; i++)
		{
			if (shl_load(localareap->dl_libs[i],
				(BIND_IMMEDIATE|DYNAMIC_PATH),
				(long) 0) == (shl_t) NULL)
			{
				if (DLdebugset == 1)
				{
					perror(localareap->dl_libs[i]);
				}
			}
			else
				DLdebug("Preloaded %s\n", localareap->dl_libs[i]);
		}
	
		cookie = localareap->dl_cookie = (PTR)shl_load(shlibname,
                    (BIND_IMMEDIATE|DYNAMIC_PATH), (long)0);
                if (cookie == (PTR)NULL)
		{
/*
** The following was added at the request of Tech Support: "please
** print out the OS-specific error message, as much information as
** you can, when the 3GL library can't be loaded."
**
** For now, that means 'perror' on HP-UX. Ultimately, it means
** encoding substantially more information in the return from DL.
*/
			perror(shlibname);
			SETCLERR(errp, DL_OSLOAD_FAIL, 0);
			return(DL_OSLOAD_FAIL);
		}
#		ifdef HPUXWORKAROUND
/*
** The following works around an HP-UX 8.0 bug in which some shared
** library (aka "dynamic loading") mechanisms just flat-out don't work
** with really big executable files as input to the symbol-resolving
** mechanism. If the cookie is NULL, it'll look through all bound
** libraries, but if we specify it, we get a core dump. Go figure.
** This has been reported to HP.
** 	-jab 6/91
*/
		ret = (shl_t)NULL;
#		endif	/* HPUXWORKAROUND */
		if (shl_findsym((shl_t *)&ret, DL_LOOKUP_FUNC,
				(short)TYPE_UNDEFINED, &val) == -1)
		{
			DLdebug("Lookup %s isn't in DL module %s\n",
				DL_LOOKUP_FUNC, shlibname);
			if (shl_findsym((shl_t *)&ret, prependunderscore("II_U3GLUser3glTab"),
				(short)TYPE_UNDEFINED, &DLusertab) != -1 &&
			    shl_findsym((shl_t *)&ret, prependunderscore("II_U3GLProcNum"),
				(short)TYPE_UNDEFINED, &DLprocnum) != -1)
			{
				DLdebug("Using retrofit lookup function in DL module %s\n",
					shlibname);
				lookupfcn = retrofitlookup;
				oldlookupfcn = NULL;
			}
		}
		else
		{
			lookupfcn = (STATUS (*)())val;
			cookie = (PTR)ret;
			haslookup = TRUE;
		}
		if ( lookupfcn == NULL )
		{
		    lookupfcn = iiDLsymfunc;
		    haslookup = FALSE;
		}
	}
#endif	/* hpb_us5*/

#if defined(VMS)
	/*  We need to do the following:
	**	1. Convert what we were given to a full pathname.
	**  	2. Define a user-level logical name to point to the full
	**	   pathname.
	**	3. Make a condition handler to trap errors from
	**	   LIB$FIND_IMAGE_SYMBOL.
	**	4. Call LIB$FIND_IMAGE_SYMBOL to map the lookup function.
	*/
        {
		char fullshlibname[MAX_LOC+1];
		$DESCRIPTOR(d_fullshlibname, fullshlibname);
		$DESCRIPTOR(d_shlibname, shlibname);
		$DESCRIPTOR(d_default, DEFAULTDIR);
		$DESCRIPTOR(d_logname, DLMOD_LOGNAME);
	    	$DESCRIPTOR(d_proclnt, PROCLNT);
		char lookupname[33];
		$DESCRIPTOR(d_lookupfcn, lookupname);
		i4 status;
		i4 context = 0;
		ITEMLIST itemlist[2];
	
		/* 1. First, expand the library to a full pathname */
		d_shlibname.dsc$w_length = STlength(shlibname);		
		status = LIB$FIND_FILE(&d_shlibname, &d_fullshlibname,
				       &context, &d_default);
		(VOID)LIB$FIND_FILE_END(&context);

		if ((status&1) != 1)
			return DL_MOD_NOT_FOUND;
		fullshlibname[MAX_LOC] = EOS;
		STtrmwhite(fullshlibname);
		d_fullshlibname.dsc$w_length = STlength(fullshlibname);		
				
		/* 2. Now, make the logical name */
		itemlist[0].buflen = d_fullshlibname.dsc$w_length;
		itemlist[0].bufadr = d_fullshlibname.dsc$a_pointer;
		itemlist[0].retadr = d_fullshlibname.dsc$a_pointer;
		itemlist[0].itmcode = LNM$_STRING;
		itemlist[1].buflen = itemlist[1].itmcode = 0;

		status = SYS$CRELNM(0, &d_proclnt, &d_logname, 0, itemlist);

		/* This shouldn't fail, ever */
		if ((status&1) != 1)
			return DL_OSLOAD_FAIL;

		/* 3. Condition handler */
		LIB$ESTABLISH(handle_all);		

		/* 4. Map it */
		STcopy(DL_LOOKUP_FUNC, lookupname);
		CVupper(lookupname);
		d_lookupfcn.dsc$w_length = STlength(lookupname);
		status = LIB$FIND_IMAGE_SYMBOL(
				&d_logname, &d_lookupfcn, &lookupfcn);
		if ((status&1) != 1)
		{
			/* Try the old name */
			STcopy(DL_OLD_LOOKUP_FUNC, lookupname);
			CVupper(lookupname);
			d_lookupfcn.dsc$w_length = STlength(lookupname);
			status = LIB$FIND_IMAGE_SYMBOL(
				&d_logname, &d_lookupfcn, &oldlookupfcn);
			
			if ((status&1) == 1)
			{
				lookupfcn = retrofitlookup;
			}
		}

		if ((status&1) != 1)
		{
			if (status == LIB$_KEYNOTFOU)			
				return DL_BAD_LOOKUPFCN;
			else
				return DL_OSLOAD_FAIL;
		}
		haslookup = TRUE;
		LIB$REVERT();
	}
#endif 

/*
** Step 2: The library's loaded, and the following variables are set:
**		cookie    - whatever OS-specific thing the system call returned.
**			    Note that "*handle" is the same thing for
**			    non-HP platforms.
**		shlibname - the name of what we grabbed.
**		lookupfcn - the address of a lookup function in the module.
** Now, to retrieve symbols and find the information we're looking for.
**
** First, start by validating the lookup function itself. On the HP8
** it's necessary to deal with older 3GL libraries that won't contain the
** lookup function or these two "special" symbols. (All others pay cash.)
*/
#ifdef DL_OLD_LOOKUP_FUNC
	if (lookupfcn != retrofitlookup
	)
#endif
	{
	    char *dlvers, *dldate;

	    if ( haslookup )
	    {
		if ((*lookupfcn)(cookie, DL_IDENT_NAME, &dlvers) != OK ||
			(*lookupfcn)(cookie, DL_DATE_NAME, &dldate) != OK)
		{
			DLdebug("Warning no name/version found\n");
		}
		else
		{
			DLdebug("Loaded %s:\n\tVers:\t\"%s\"\n\tDate:\t\"%s\"\n",
				shlibname,dlvers, dldate);
		}
	    }
	}
	localareap->dl_cookie = cookie;
	localareap->dl_lookupfcn = lookupfcn;
	localareap->dl_check = DL_valid;

/*
** Now, the version string, then the requested symbols themselves.
** Check the version string, if requested. If you wanted to check, but
** it wasn't in the loaded file, it's an error.
*/
 if (vers != NULL &&
#ifdef DL_OLD_LOOKUP_FUNC
	 lookupfcn != retrofitlookup &&
#endif
	 *vers != EOS)
	{
		long addr;
		if ((*lookupfcn)(cookie, DLVERSNAME, &addr) != OK ||
			STcompare((char *)addr, vers) != OK)
		{
			_VOID_ DLunload((PTR)localareap, (CL_ERR_DESC *)NULL);
			/* This will be a no-op on some machines anyhow */
			SETCLERR(errp, DL_VERSION_WRONG, 0);
			return(DL_VERSION_WRONG);
		}
	}
	DLdebug("Successfully loaded %s\n", shlibname);
	return(OK);
#endif	/* !DL_LOADING_WORKS */
}

/*
** Name:	DLosunload
**
** Description:
** 		Do the OS-specific portions of a DLunload.
**
** Inputs:
**	cookie		Magic cookie from DLosprepare. If NULL, we quietly return
**				a success (to get around an HP-UX bug).
** Outputs:
**	errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**	OK		The location exists, and it's a directory.
**	DL_NOT_IMPLEMENTED	Not implemented on this platform.
**	DL_OSUNLOAD_FAILED	The operating system call to unload <whatever> failed.
**	other		System-specific error status, or file 
** Side effects:
**	See description. If possible, will detach the DL module.
*/
STATUS
DLosunload(cookie,errp)
PTR cookie;
CL_ERR_DESC *errp;
{
#if defined(hpb_us5)
	if (cookie == NULL)	/* Silently accept NULL cookies. Hp-UX bug workaround*/
		return(OK);
	if (shl_unload((shl_t)cookie) == -1)
	{
		extern int errno;
		if (errp)
		{
			SETCLERR(errp, DL_OSUNLOAD_FAIL, 0);
		}
		return(DL_OSUNLOAD_FAIL);
	}
#endif	/* hpb */
#if defined(sparc_sol) || defined(usl_us5) || defined(axp_osf) || \
    defined(any_aix) || defined(sgi_us5) || \
    defined(hp2_us5) || defined(i64_hpu) || \
    defined(a64_sol) || defined(LNX) || defined(OSX)
	if (dlclose(cookie) != 0)
	{	
		if (errp)
		{
			SETCLERR(errp, DL_OSUNLOAD_FAIL, 0);
		}
		return(DL_OSUNLOAD_FAIL);
	}
#endif	/* sun */

	return(OK);
}

