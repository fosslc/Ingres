/*
**Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <systypes.h>
#include <clconfig.h>
#include <cm.h>
#include <er.h>
#include <cs.h>
#include <lo.h>
#include <nm.h>
#include <si.h>
#include <st.h>
#include <pc.h>
#include <pe.h>

#include <dl.h>
#include    "dlint.h"
#include <errno.h>

# include       <sys/param.h>

# ifdef xCL_070_LIMITS_H_EXISTS
# include <limits.h>
# endif /* xCL_070_LIMITS_H_EXISTS */

/*
** DL -- the dynamic loading module ("it came from `DY'")
**
**  This module	provides functionality to pull an object into
**  your address space that wasn't there earlier. There	are
**  a number of	base assumptions that we insist	on:
**  1. The object being	pulled in must be created in a manner
**     consistent with what 'DLprepare'	does. For HP-UX, this means
**     its "ld -b" output and has a function "IIdllookupfunc" that
**     provides	name->address translation; for the Sun it should
**     be a shared object from an "ld -o file.so" run.
**     will mean "uses ld -b" on an hp8_us5, but this will expand.
**  2. The object isn't	any ol'	relocatable object. If there are
**     unresolved references to	external variables/common blocks,
**     the OS mechanism	should have provisions to resolve them.
**  3. The symbols "IIdlversion" and "IIdldatecreated" must be in
**     all symbol tables. That way, we can do a	"trial lookup" in DLload
**     to make sure that the lookup function works before returning a
**     success in DLload.
**
**  If this module's not implemented on	a system, then a non-fatal
**  error (DL_NOT_IMPLEMENTED) is returned if you invoke any of	the
**  routines herein.
**
** Side-effects:
**  There are static variables in this procedure that are set the first
**  time through and not reset.	They're	set based on environment variables,
**  which shouldn't fluctuate during the life of the program. Also, DL
**  is free to allocate	memory that may	stay around for	a long,	long time.
**
**
** History:
**  13-may-91 (jab)
**	Written
**  31-may-91 (jab)
**	Bug fixes abound. Changed interface....
**  25-jul-91 (jab)
**	Changed	interface to deal with getting rid of some HP-UX-isms.
**	Should work for	Encore and HP 9000/xxx machine now.
**  16-sep-91 (jefff)
**	Added hooks for	en8_us5
**  4-oct-91 (jab)
**	Pulled out search path stuff, it'll disappear from the new DL
**	mechanisms anyhow. Also, fixed bug #39967, in which FORTRAN and
**	other shared libs can now be "preloaded" to resolve the	3GL
**	functions against if necessary.
**  15-dec-91 (jab)
**	Changed	"fileexists" so	that it's not in the global name space.
**  28-jan-92 (jab)
**	Updating to CL specs and checking into 6.4 codeline for	W4GL II.
**  4/92 (Mike S)
**	Correct typo; check old library if it exists.
**  8-jun-93 (ed)
**	replaced CL_PROTOTYPED for CL_HAS_PROTOS
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-apr-95 (canor01)
**	    added <errno.h>
**	03-jun-1996 (canor01)
**	    Removed MCT semaphores since code is not used by threaded servers.
**	03-oct-1996 (canor01)
**	    Dllookupfcn() should be passed the handle, not a pointer to the
**	    handle.
**	17-oct-1996 (canor01)
**	    Implement DLcreate().  Add DLcreate_loc() and DLprepare_loc().
**	21-oct-1996 (canor01)
**	    Add user-defined parameters to DLcreate_loc().  Add additional
**	    return of DL_MOD_NOT_READABLE when file exists but not readable.
**	25-oct-1996 (canor01)
**	    Fixed typo.
**    06-jan-1997 (canor01)
**          Increase size of command buffer from BUFSIZ to ARG_MAX (largest
**          argument size for spawned command).  Replace STprintf() with
**          STpolycat() to get past the BUFSIZ limit.
**      08-jan-1997 (canor01)
**          Above change has potential to overflow stack.  Get dynamic
**          memory instead.
**	29-jan-1997 (canor01)
**          If xCL_070_LIMITS_H_EXISTS is defined, include
**          <limits.h> and define CMDLINSIZ as ARG_MAX.
**	29-jan-1997 (hanch04)
**	    added param.h for other than DESKTOP
**	20-mar-1997 (canor01)
**	    Add DLdelete_loc() to parallel DLcreate_loc().
**	07-may-1997 (canor01)
**	    Add DLconstructloc().
**	28-may-1997 (canor01)
**	    Use previously obtained directory information when searching
**	    for files with old extensions.
**	29-may-1997 (canor01)
**	    Bringing changes from NT version of DLconstructloc() lost the 
**	    "lib" prefix used by Unix module names.
**	04-jun-1997 (canor01)
**	    Modified DLunload() to just unload the currently referenced module.
**	    Cleaned up some formatting to match coding spec.
**	26-Nov-1997 (allmi01)
**	    Added sgi_us5 (Silicon Grpahics) to platforms which set NCARGS.
**	13-aug-1997 (canor01)
**	    Added 'flags' parameter to DLprepare_loc().  This will allow
**	    caller to state preference for symbol resolution (immediate
**	    or deferred).
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      14-Jan-1999 (fanra01)
**          Add check for a NULL location in DLprepare_loc.  This will allow
**          the default OS paths to be scanned for the library.
**      03-jul-1999 (podni01)
**          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-apr-2004 (loera01)
**          Added new DLload() routine.
**      20-dec-2004 (Ralph.Loen@ca.com) Bug 113660
**          In DLload(), if the platform is 64-bit and employs lp64 
**          subdirectories, add the lp64 subdirectory to the search path.
** 	28-Apr-2005 (hanje04)
**	    Fix up function prototypes to quiet compiler warnings.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
*/

/* External variables */

# ifdef ARG_MAX
#     define CMDLINSIZ    (ARG_MAX)
# else
# if defined(sgi_us5)
#     define NCARGS           51200 /* NB - this is maximum value of tunable */
# elif defined(NT_GENERIC)
#     define NCARGS           1024  /* NB - this is maximum value of tunable */
# endif
#     define CMDLINSIZ    (NCARGS > 40000 ? 40000 : NCARGS)
# endif /* ARG_MAX */

/* Prototypes */
static STATUS 
DLconstructxloc( char *dlmodname, LOCATION *tmplocp, char *ext,
			CL_ERR_DESC	*errp );

/*
** Name:    DLisdir
**
** Description:
**	A local	routine	to check that the given	location exists	and is
**	a directory.
**
** Inputs:
**  locp    Pointer to LOCATION	to be checked out.
** Outputs:
**  none
** Returns:
**  OK	    The	location exists, and it's a directory.
**  DL_DIRNOT_FOUND File/dir not found
**  DL_NOT_DIR	File exists, isn't a directory.
**  other	System-specific	error status, or file 
** Side	effects:
**  None
*/
static STATUS
DLisdir(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4	flagword = (LO_I_TYPE);

    if (LOinfo(locp, &flagword,	&loinf)	!= OK)
	return(DL_DIRNOT_FOUND);
    if ((flagword & LO_I_TYPE) == 0 ||
       (loinf.li_type != LO_IS_DIR))
	return(DL_NOT_DIR);
    /* FIXME since user can override with IIDLDIR, it would be
    ** a good idea to validate that the dir is owned by the installation
    ** owner (ingres) and is only writable by the owner.
    */
    return(OK);
}

/*
** Name:    fileexists
**
** Description:
**	A local	routine	to check that the given	location exists.
**	This is	the old	LOexists, really.
**
** Inputs:
**  locp	Pointer	to LOCATION to be checked out.
** Outputs:
**  none
** Returns:
**  TRUE	The location exists.
**  FALSE	The location doesn't exist.
** Side	effects:
**  None
*/
static bool 
fileexists(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4	flagword = LO_I_PERMS;

    return( LOinfo(locp, &flagword, &loinf) == OK);
}

/*
** Name:    DLdebugcheck
**
** Description:
**	A local	routine	to make	sure that debugging, if	requested,
**	has been turned	on. ALL	user-callable routines should invoke
**	this on	entry.
**
** Inputs:
**  none
** Outputs:
**  none
** Returns:
**  none
** Side	effects:
**  Modifies "DLdebugset" to maintain state of debugging.
*/
GLOBALDEF i4  DLdebugset = -1;
static void 
DLdebugcheck()
{
    char *dbgvar = NULL;

    if (DLdebugset == -1)
    {
	NMgtAt("IIDLDEBUG", &dbgvar);
	if (dbgvar != NULL && dbgvar[0]	!= EOS)
	{
	    DLdebugset = 1;
	}
    }
}

/*
** Name:    DLgetdir
**
** Description:
**	A local	routine	to fill	in a location with the user-provided
**	directory "$IIDLDIR", or $II_SYSTEM/ingres/files/dynamic as
**	a default. Checks to make sure that the	requested directory
**	exists [and is a directory].
** Inputs:
**  none
** Outputs:
**  locp    The	output location	that's been filled in.
**  locbuf  A storage area for locp, size at least MAX_LOC+1.
**  errp    Set	on any errors.
** Returns:
**  OK	    The	directory was found and	passed simple integrity	checks.
**  other   Some error was detected by a lower-level routine. Either
**	    the	directory specified wasn't found, or it	fails some
**	    basic integrity checks.
** Side	effects:
**  none
**
** History:
**	????
**	15-Jun-2004 (schka24)
**	    Avoid locbuf overrun.
*/
static STATUS 
DLgetdir(locp, locbuf, errp)
LOCATION *locp;
char locbuf[];
CL_ERR_DESC *errp;
{
    char *iidldir;
    STATUS ret;
    LOCATION	tmploc;	/* For the couple of times we need a temp var */

    /* (schka24) The only reason I leave this override in is that it's
    ** used for the ICE server, which is not setuid.  And, users can't
    ** affect the env var, only the installation owner.
    */
    NMgtAt("IIDLDIR", &iidldir);
    if (iidldir	!= NULL	&& iidldir[0] != '\0')
    {
	STlcopy(iidldir, locbuf, MAX_LOC-1);
	if ((ret = LOfroms(PATH, locbuf, locp))	!= OK)
	{
	    DLdebug("error constructing	path, $IIDLDIR = \"%s\"\n", iidldir);
	    SETCLERR(errp, ret, 0);
	    return(ret);
	}
    }
    else
    {
        if ((ret = NMloc(FILES,	PATH, "dynamic", &tmploc)) != OK)
	{
	    DLdebug("error constructing	path, ret from NMloc = 0x%x\n",	
		    ret);
	    SETCLERR(errp, ret, 0);
	    return(ret);
	}
	LOcopy(&tmploc,	locbuf,	locp);
    }
/*
**  At this point, "locp" points to the	location where DL modules
**  reside.
*/
    ret	= DLisdir(locp);
    if (ret != OK)
    {
	char *locs;
	(void) LOtos(locp, &locs);
	DLdebug("Error in %s: non-existent or bad directory (ret = 0x%x)\n",
	    locs, ret);
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    return(OK);
}

/*
** Name:    DLconstructloc
**  
** Description:
**      Build a location containing full name (with extension)
**      given a module name.
**       
** Inputs:
**      inloc   pointer to LOCATION containing either module name
**              alone or path plus module name (no extension).   
**      buffer  pointer to a buffer of at least MAX_LOC+1 which  
**              will be used to build the new LOCATION.
**               
** Outputs:      
**      outloc  pointer to LOCATION containing full path plus
**              module name with extension appended.
**      errp    filled in on error.
**       
** Returns:
**      OK      Success.
**      other   Some error was detected by a lower-level routine.
**       
** Side effects:
**      none
**
** History:
**	08-may-1997 (canor01)
**	    Pass pointer to location structure to LOstfile(), even
**	    though Solaris does the right thing with it.
**	29-may-1997 (canor01)
**	    Bringing changes from NT version lost the "lib" prefix
**	    used by Unix module names.
*/
STATUS
DLconstructloc( LOCATION *inloc,
                char     *buffer,
                LOCATION *outloc,
                CL_ERR_DESC *errp )
{
    char        tmpbuf[MAX_LOC+1];
    char        namebuf[MAX_LOC+1];
    char        *nameptr;
    char	*cp;
    LOCATION    nameloc;
    STATUS      ret;
 
    /* copy the filename to a temporary buffer */
    LOtos( inloc, &nameptr );    

    if ( (cp = STrchr( nameptr, '/' )) == NULL )
    {
	STcopy(ERx("lib"), namebuf);
	STcat( namebuf, nameptr );
    }
    else
    {
	STncpy( namebuf, nameptr, cp - nameptr );
	namebuf[ cp - nameptr ] = EOS;
	STcat( namebuf, ERx("/lib") );
	STcat( namebuf, CMnext(cp) );
    }

    /* add the file extension to the buffer */
    STcat( namebuf, DL_EXT_NAME );
     
    /* create a temporary location with the name+extension */
    LOfroms( PATH & FILENAME, namebuf, &nameloc );
     
    /* if not a full path, get the default */
    if ( !LOisfull( inloc ) )
    {    
        if ((ret = DLgetdir( outloc, buffer, errp )) != OK)
            return(ret);
    }
    else 
    {
        /* use the path from the input location */
        LOcopy( inloc, buffer, outloc );
    } 
 
    /* set the output file name */
    LOstfile( &nameloc, outloc );
         
    return ( OK );
}    

/*
** Name:    DLconstructname
**
** Description:
**	A local	routine	to constuct the	names of the shared lib/object
**	and any	other files, given the DL module name and directory.
** Inputs:
**  dlmodname	Name of	the DL module, 8 chars or less.
**  ext		Extension of the file, e.g. ".so" or ".dsc" or whatever
** Outputs:
**  tmplocp	The output location that's been	filled in.
**  tmpbuf	A storage area for locp, size at least MAX_LOC+1.
**  errp	Set on any errors.
** Returns:
**  OK		The directory was found	and passed simple integrity checks.
**  other	Some error was detected	by a lower-level routine. Either
**		the directory specified	wasn't found, or it fails some
**		basic integrity	checks.
** Side	effects:
**  none
*/
static STATUS 
DLconstructname( char *dlmodname, char tmpbuf[], LOCATION *tmplocp,
			char *ext, CL_ERR_DESC	*errp )
{
    STATUS ret;

    if ((ret = DLgetdir(tmplocp, tmpbuf, errp))	!= OK)
	return(ret);

    return ( DLconstructxloc(dlmodname, tmplocp, ext, errp) );
}

static STATUS 
DLconstructxloc( char *dlmodname, LOCATION *tmplocp, char *ext,
			CL_ERR_DESC	*errp )
{
    LOCATION fileloc;
    char filebuf[MAX_LOC];
    STATUS ret;

    STcopy(ERx("lib"), filebuf);
    STcat(filebuf, dlmodname);
    STcat(filebuf, ext);
    if ((ret = LOfroms(FILENAME, filebuf, &fileloc)) !=	OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    LOstfile(&fileloc, tmplocp);
    return(OK);
}
#ifdef DL_OLD_EXT_NAME
#define	DLconstructoldobjname(dlmodname, tmpbuf, tmplocp, errp) \
    DLconstructname(dlmodname, tmpbuf, tmplocp,	DL_OLD_EXT_NAME, errp)
#define DLconstructoldobjloc(dlmodname, tmplocp, errp) \
    DLconstructxloc(dlmodname, tmplocp, DL_OLD_EXT_NAME, errp)
#endif
#define	DLconstructobjname(dlmodname, tmpbuf, tmplocp, errp) \
    DLconstructname(dlmodname, tmpbuf, tmplocp,	DL_EXT_NAME, errp)
#define	DLconstructtxtname(dlmodname, tmpbuf, tmplocp, errp) \
    DLconstructname(dlmodname, tmpbuf, tmplocp,	DL_TXT_NAME, errp)
#define	DLconstructobjloc(dlmodname, tmplocp, errp) \
    DLconstructxloc(dlmodname, tmplocp,	DL_EXT_NAME, errp)
#define	DLconstructtxtloc(dlmodname, tmplocp, errp) \
    DLconstructxloc(dlmodname, tmplocp,	DL_TXT_NAME, errp)

/*
** ------------- end of	nasty common support routines -------------
*/

/*
** Name:    DLnameavail
**
** Description:
**	DLnameavail checks that	the object naem	can be used to create
**	a DL module using DLcreate. Although it	mainly checks that the
**	name is	unused by any existing DL module, it may return	more
**	information in the error returns.
**  
**
** Inputs:
**  dlmodname	The name of a DL module
** Outputs:
**  err		Points to variable used	to return system-specific
**		errors.
** Returns:
**  OK		The name is available for use, that is,	it's not
**		currently in use by another DL.
**  DL_NOT_IMPLEMENTED	if DL isn't implemented	on this	platform.
**  DL_MODULE_PRESENT	A DL-module exists by that name	already.
**  other	System-specific	error status.
** Side	effects:
**  None
*/

STATUS
DLnameavail(char *dlmodname, CL_ERR_DESC *errp)
{
#if !(DL_LOADING_WORKS || DL_UNLOADING_WORKS)
    SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
#else	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
    STATUS	ret;
    LOCATION	loc;	/* Directory for DL modules */
    char	locbuf[MAX_LOC];
    char *filename;

    DLdebugcheck();
    if ((ret = DLconstructobjname(dlmodname, locbuf, &loc, errp)) != OK)
	return(ret);
    if (fileexists(&loc))
    {
	LOtos(&loc, &filename);
	DLdebug("DL: %s	exists,	so %s isn't available as a DL name\n",
	    filename, dlmodname);
	SETCLERR(errp, DL_MODULE_PRESENT, 0);
	return(DL_MODULE_PRESENT);
    }
    if ((ret = DLconstructtxtname(dlmodname, locbuf, &loc, errp)) != OK)
	return(ret);
    if (fileexists(&loc))
    {
	LOtos(&loc, &filename);
	DLdebug("DL: %s	exists,	so %s isn't available as a DL name\n",
	    filename, dlmodname);
	SETCLERR(errp, DL_MODULE_PRESENT, 0);
	return(DL_MODULE_PRESENT);
    }

    return(OK);
#endif	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
}

/*
** Name:    DLdelete
**
** Description:
**	Deletes	the on-disk copy of the	DL module, returning an	error
**	status (or "OK") when appropriate.
**
** Inputs:
**  dlmodname		The 8-char (or less) DL	module name
** Outputs:
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May or may	not free up malloc'ed memory.
*/

STATUS
DLdelete(char *dlmodname, CL_ERR_DESC *errp)
{
#if !(DL_LOADING_WORKS || DL_UNLOADING_WORKS)
    SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
#else	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
    LOCATION loc;
    char	locbuf[MAX_LOC+1];
    STATUS	ret;

    if ( (ret = DLgetdir( &loc, locbuf, errp )) != OK )
	return (ret);

    return( DLdelete_loc( dlmodname, &loc, errp ) );
#endif	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
}

STATUS
DLdelete_loc(char *dlmodname, LOCATION *loc, CL_ERR_DESC *errp)
{
    STATUS	ret;
    char	locbuf[MAX_LOC];

    DLdebugcheck();
    if ((ret = DLconstructobjloc(dlmodname, loc, errp)) != OK)
	return(ret);
    if (fileexists(loc) && (ret = LOdelete(loc)) != OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }

    if ((ret = DLconstructtxtloc(dlmodname, loc, errp)) != OK)
	return(ret);
    if (fileexists(loc) && (ret = LOdelete(loc)) != OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    return(OK);
}

/*
** Name:    DLload
**
** Description:
**	Allow DLbind calls for a module. Might or might	not bind to the
**      symbols provided.
**      This routine is a combination of DLprepare() and DLprepare_loc(),
**      with similar functionality--except that it won't be looking for
**      a file formated as libxxx.sfx.2.0.  Intead, the user provides
**      the full file name, including "lib" and the file extension.
**      The user may optionally provide a LOCATION pointer if the module
**      to be loaded doesn't reside in $II_SYSTEM/ingres/lib.
**
** Inputs:
**  loc                 The path of the module resolved into a LOCATION.
**  dlmodname		The DL module (file) name.
**  syms		List of	symbols	to be used in DLbinds, or NULL.
** Outputs:
**  handle		Magic cookie filled in on successful return.
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  DL_MOD_NOT_FOUND	DL module name can't be	found.
**  DL_MOD_CNT_EXCEEDED	Only one (for now) DL module can be bound at a time.
**  DL_VERSION_WRONG	Version	was given and checked, but didn't match
**			the DL module's	version	string.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May attach	the DL module during this call,	will likely
**  allocate memory for	the handle.
*/

STATUS
DLload(LOCATION *ploc, char *dlmodname, char *syms[], PTR *handle,
    CL_ERR_DESC	*errp)
{
    LOCATION loc, *iloc = &loc;
    STATUS	ret;
    LOCATION	txtloc;	/* Directory for DL modules */
    char	txtbuf[MAX_LOC];
    char	locbuf[MAX_LOC];
    struct DLint *localarea;
    LOCATION fileloc;

    DLdebugcheck();

    if (ploc == NULL)
    {
	if ((ret = NMloc(LIB, PATH, NULL, iloc)) != OK)
            return (ret);
    }
    else
        iloc = ploc;

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
    if ((ret = LOfaddpath(iloc, "lp64", iloc)) != OK) 
            return ret;
#endif  /* ADD_ON64 */

    localarea =	(struct	DLint *)
	MEreqmem(0, sizeof(*localarea),	/* clear = */ TRUE, &ret);
    if (localarea == NULL)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    localarea->dl_check	= DL_invalid;

    if ((ret = LOfroms(FILENAME, dlmodname, &fileloc)) != OK)
    {
         SETCLERR(errp, ret, 0);
         MEfree((PTR)localarea);
         return ret;
    }
    LOstfile(&fileloc, iloc);
        
    if (fileexists(iloc))
    {
	i4		errcode;
	LOINFORMATION	loinfo;
	i4  		flags;

	/* if the file exists, but is not readable, say so */
	flags = LO_I_PERMS;
	LOinfo( iloc, &flags, &loinfo );
	if ( (flags & LO_I_PERMS) != 0  &&
	    (loinfo.li_perms & LO_P_READ) == 0 )
        {
            SETCLERR(errp, DL_MOD_NOT_READABLE, 0);
            MEfree((PTR)localarea);
	    return (DL_MOD_NOT_READABLE);
        }
	  
        localarea->dl_isoldversion = FALSE;
    }
    else
    {
	char		*filename;

	LOtos(iloc, &filename);
	DLdebug("DL module %s doesn't exist: %s not found\n",
	    dlmodname, filename);
	SETCLERR(errp, DL_MOD_NOT_FOUND, 0);
	return(DL_MOD_NOT_FOUND);
    }
    LOcopy( iloc, txtbuf, &txtloc );
    if ((ret = DLconstructtxtloc(dlmodname, &txtloc, errp)) !=	OK)
    {
        MEfree((PTR)localarea);
	return(ret);
    }

    /*
    ** If the file exists, try to parse/use	it.
    */
    if (fileexists(&txtloc))
    {
	localarea->dl_func_cnt = sizeof(localarea->dl_funcs) /
	    sizeof(localarea->dl_funcs[0]);
	localarea->dl_lib_cnt =	sizeof(localarea->dl_libs) /
	    sizeof(localarea->dl_libs[0]);
	if ((ret = DLparsedesc(&txtloc,
		localarea->dl_exename,
		localarea->dl_storedmodname,
		localarea->dl_storedversionstr,
		localarea->dl_storeddatecreated,
		localarea->dl_functionlist, localarea->dl_funcs,
		&localarea->dl_func_cnt,
		localarea->dl_liblist, localarea->dl_libs,
		&localarea->dl_lib_cnt,
		errp)) != OK)
	{
            MEfree((PTR)localarea);
	    return(ret);
	}
	if (localarea->dl_functionlist[0] == EOS)
	{
	    localarea->dl_func_cnt = 0;
	}
	if (localarea->dl_liblist[0] ==	EOS)
	{
	    localarea->dl_lib_cnt = 0;
	}
    }
    else
    {
	localarea->dl_func_cnt = 0;
    }

/*
** At this point, we should have everything we need, including the names
** of any spare	libraries to load, what	symbols	are needed, all	that jazz.
** Now go do it.
*/
    ret	= DLosprepare(NULL, iloc, syms, localarea, DL_RELOC_DEFAULT, errp);
    if (ret == OK)
	*handle	= (PTR)localarea;
    return(ret);
}

/*
** Name:    DLprepare
**
** Description:
**	Allow DLbind calls for a module. Might or might	not bind to
**
** Inputs:
**  vers		Version	string (or NULL) to be checked against
**  dlmodname		The 8-char (or less) DL	module name
**  syms		List of	symbols	to be used in DLbinds, or NULL
** Outputs:
**  handle		Magic cookie filled in on successful return
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  DL_MOD_NOT_FOUND	DL module name can't be	found.
**  DL_MOD_CNT_EXCEEDED	Only one (for now) DL module can be bound at a time.
**  DL_VERSION_WRONG	Version	was given and checked, but didn't match
**			the DL module's	version	string.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May attach	the DL module during this call,	will likely
**  allocate memory for	the handle.
*/
#ifndef	MAXLINE
#define	MAXLINE	128
#endif

# ifdef ONLY_ONE_DL_MODULE_ACTIVE
static int DL_active = 0;
# endif /* ONLY_ONE_DL_MODULE_ACTIVE */

STATUS
DLprepare(char *vers, char *dlmodname, char *syms[], PTR *handle,
    CL_ERR_DESC	*errp)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC+1];
    STATUS	ret;

    if ( (ret = DLgetdir( &loc, locbuf, errp )) != OK )
	return (ret);

    return( DLprepare_loc( vers, dlmodname, syms, &loc, 
			   DL_RELOC_DEFAULT, handle, errp ) );
}

STATUS
DLprepare_loc(char *vers, char *dlmodname, char *syms[], 
              LOCATION *loc, i4  flags, PTR *handle, CL_ERR_DESC *errp)
{
#if !DL_LOADING_WORKS
    SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
#else	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
    STATUS	ret;
    LOCATION	txtloc;	/* Directory for DL modules */
    char	txtbuf[MAX_LOC];
    char	locbuf[MAX_LOC];
    struct DLint *localarea;
    bool nofile = TRUE;

    DLdebugcheck();
    localarea =	(struct	DLint *)
	MEreqmem(0, sizeof(*localarea),	/* clear = */ TRUE, &ret);
    if (localarea == NULL)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    localarea->dl_check	= DL_invalid;

    if (loc != NULL)
    {
	/*
	** We check the new name first, since if both exist we want the new one.
	*/
	if ((ret = DLconstructobjloc(dlmodname, loc, errp)) != OK)
	    return ret;

	if (fileexists(loc))
	{
	    i4		errcode;
	    LOINFORMATION	loinfo;
	    i4  		flags;

	    /* if the file exists, but is not readable, say so */
	    flags = LO_I_PERMS;
	    LOinfo( loc, &flags, &loinfo );
	    if ( (flags & LO_I_PERMS) != 0  &&
		 (loinfo.li_perms & LO_P_READ) == 0 )
	    {
		SETCLERR(errp, DL_MOD_NOT_READABLE, 0);
		return (DL_MOD_NOT_READABLE);
	    }
	  
	    localarea->dl_isoldversion = FALSE;
	    nofile = FALSE;
	}
	else
	{
#if defined(DLconstructoldobjname)
	    if ((ret = DLconstructoldobjloc(dlmodname, loc, errp)) != OK)
		return ret;
	    if (fileexists(loc))
	    {
		localarea->dl_isoldversion = TRUE;
		nofile = FALSE;
	    }
#endif
	}
	if (nofile)
	{
	    char		*filename;

	    LOtos(loc, &filename);
	    DLdebug("DL module %s doesn't exist: %s not found\n",
		dlmodname, filename);
	    SETCLERR(errp, DL_MOD_NOT_FOUND, 0);
	    return(DL_MOD_NOT_FOUND);
	}
    }
# ifdef ONLY_ONE_DL_MODULE_ACTIVE
    if (DL_active > 0)
    {
	DLdebug("Trying	to use a second	DL module without unloading the	first\n");
	SETCLERR(errp, DL_MOD_COUNT_EXCEEDED, 0);
	return(DL_MOD_COUNT_EXCEEDED);
    }
    DL_active++;
# endif /* ONLY_ONE_DL_MODULE_ACTIVE */

    if (loc != NULL)
    {
	LOcopy( loc, txtbuf, &txtloc );
	if ((ret = DLconstructtxtloc(dlmodname, &txtloc, errp)) !=	OK)
	return(ret);
    }
    else
    {
        txtbuf[0] = '\0';
        LOfroms (PATH, txtbuf, &txtloc);
        loc = &txtloc;
	if ((ret = DLconstructobjloc(dlmodname, loc, errp)) != OK)
	    return (ret);
    }

    /*
    ** If the file exists, try to parse/use	it.
    */
    if (fileexists(&txtloc))
    {
	localarea->dl_func_cnt = sizeof(localarea->dl_funcs) /
	    sizeof(localarea->dl_funcs[0]);
	localarea->dl_lib_cnt =	sizeof(localarea->dl_libs) /
	    sizeof(localarea->dl_libs[0]);
	if ((ret = DLparsedesc(&txtloc,
		localarea->dl_exename,
		localarea->dl_storedmodname,
		localarea->dl_storedversionstr,
		localarea->dl_storeddatecreated,
		localarea->dl_functionlist, localarea->dl_funcs,
		&localarea->dl_func_cnt,
		localarea->dl_liblist, localarea->dl_libs,
		&localarea->dl_lib_cnt,
		errp)) != OK)
	{
	    return(ret);
	}
	if (vers && vers[0] != EOS && localarea->dl_storedversionstr[0]	!= EOS
	    && STcompare(vers, localarea->dl_storedversionstr) != 0)
	{
	    SETCLERR(errp, DL_VERSION_WRONG, 0);
	    return(DL_VERSION_WRONG);
	}
	if (localarea->dl_functionlist[0] == EOS)
	{
	    localarea->dl_func_cnt = 0;
	}
	if (localarea->dl_liblist[0] ==	EOS)
	{
	    localarea->dl_lib_cnt = 0;
	}
    }
    else
    {
	localarea->dl_func_cnt = 0;
    };

/*
** At this point, we should have everything we need, including the names
** of any spare	libraries to load, what	symbols	are needed, all	that jazz.
** Now go do it.
*/
    ret	= DLosprepare(vers, loc, syms, localarea, flags, errp);
    if (ret == OK)
	*handle	= (PTR)localarea;
    return(ret);
#endif	/*  !(DL_LOADING_WORKS || DL_UNLOADING_WORKS) */
}

/*
** Name:    DLbind
**
** Description:
**	Allow DLbind calls for a module. Might or might	not bind to
**
** Inputs:
**  vers		Version	string (or NULL) to be checked against
**  dlmodname		The 8-char (or less) DL	module name
**  syms		List of	symbols	to be used in DLbinds, or NULL
** Outputs:
**  handle		Magic cookie filled in on successful return
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  DL_MOD_NOT_FOUND	DL module name can't be	found.
**  DL_MOD_CNT_EXCEEDED	Only one (for now) DL module can be bound at a time.
**  DL_VERSION_WRONG	Version	was given and checked, but didn't match
**			the DL module's	version	string.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May attach	the DL module during this call,	will likely
**  allocate memory for	the handle.
**
** History:
**	03-oct-1996 (canor01)
**	    Dllookupfcn() should be passed the handle, not a pointer to the
**	    handle.
**	16-jun-1997 (canor01)
**	    The "addr" parameter should be a PTR*, not a PTR, to match
**	    usage.
*/
STATUS
DLbind(PTR handle, char	*sym, PTR *addr,
    CL_ERR_DESC	*errp)
{
#if !DL_LOADING_WORKS
    SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
#else	/*  !DL_LOADING_WORKS */
    char *val;/* it's hard-coded into IIlookup this way */
    STATUS	(*lookupfcn)();
    PTR	cookie;

    if (handle == NULL ||
	((struct DLint *)handle)->dl_check != DL_valid ||
	((struct DLint *)handle)->dl_lookupfcn == NULL)
    {
	SETCLERR(errp, DL_LOOKUP_BAD_HANDLE, 0);
	return(DL_LOOKUP_BAD_HANDLE);
    }
    if (((struct DLint *)handle)->dl_func_cnt >	0)
    {
	i4 cnt;

	for (cnt = 0; cnt < ((struct DLint *)handle)->dl_func_cnt; cnt++)
	    if (STcompare(sym, ((struct	DLint *)handle)->dl_funcs[cnt])	== 0)
		break;
	if (cnt	== ((struct DLint *)handle)->dl_func_cnt)
	{
	    SETCLERR(errp, DL_FUNC_NOT_FOUND, 0);
	    return(DL_FUNC_NOT_FOUND);
	}
    }

    lookupfcn =	((struct DLint *)handle)->dl_lookupfcn;
    cookie = ((struct DLint *)handle)->dl_cookie;
    if ((*lookupfcn)((void *)cookie, sym, &val) != OK)
    {
	SETCLERR(errp, DL_FUNC_NOT_FOUND, 0);
	return(DL_FUNC_NOT_FOUND);
    }
    else
    {
	*addr = (PTR)val;
    }
    return(OK);
#endif	/* !DL_LOADING_WORKS */
}


/*
** Name:    DLunload
**
** Description:
**  Break the logical and physical (?) ties to a DL module.
**
** Inputs:
**  handle		Magic cookie from DLprepare, MEfree'd during this call.
** Outputs:
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  DL_LOOKUP_BAD_HANDLE    Handle is invalid for some reason.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. If	possible, will detach the DL module; will MEfree
**  the	handle if possible.
*/
STATUS
DLunload(PTR handle, CL_ERR_DESC *errp)
{
#if !DL_UNLOADING_WORKS
    SETCLERR(errp, DL_NOT_IMPLEMENTED, 0);
#else	/*  !DL_UNLOADING_WORKS	*/
    PTR	cookie;
    STATUS ret;

    if (handle == NULL ||
	((struct DLint *)handle)->dl_check != DL_valid)
    {
	SETCLERR(errp, DL_LOOKUP_BAD_HANDLE, 0);
	return(DL_LOOKUP_BAD_HANDLE);
    }
    ret	= DLosunload(((struct DLint *)handle)->dl_cookie, errp);
    ((struct DLint *)handle)->dl_check = DL_invalid;
# ifdef ONLY_ONE_DL_MODULE_ACTIVE
    DL_active--;
# endif /* ONLY_ONE_DL_MODULE_ACTIVE */
    MEfree(handle);
    return(ret);

#endif	/* !DL_UNLOADING_WORKS */
}

/*
** Name:    DLcreate
**
** Description:
**  Create a module which can be passed to DLprepare.
**
** Inputs:
**  exename	If non-NULL (and *exename != EOS), a C string identifying 
**		the executable the output module will be used with, in a 
**		way that will allow symbol resolution if that is required. 
**  vers	If non-NULL (and *vers != EOS), a pointer to a version string 
**		to be embedded in output module. 
**  dlmodname	A pointer to a C string containing the name of a DL module 
**		to create, using the format described under "Definitions" . 
**  in_objs	An array of C strings, with NULL entry as its last element, 
**		containing system-specific specifications for names of input 
**		object files. This list can specify zero libraries 
**		(in_objs[0] = NULL). 
**  in_libs	An array of C strings, with NULL entry as its last element, 
**		containing system-specific specifications for names of input 
**		libraries. This list can specify zero libraries 
**		(in_libs[0] = NULL). 
**  exp_fcns	An array of C strings, with NULL entry as its last element, 
**		containing the function names that can be found using 
**		DLprepare and/or DLbind . 
** Outputs:
**  err		Pointer to variable used to return system-specific errors. 
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_NOT_IMPLEMENTED	Not implemented	on this	platform.
**  other		System-specific	error status
** Side	effects:
**  None except the obvious.
** History:
**	06-jan-1997 (canor01)
**	    Increase size of command buffer from BUFSIZ to ARG_MAX (largest
**	    argument size for spawned command).  Replace STprintf() with
**	    STpolycat() to get past the BUFSIZ limit.
**      08-jan-1997 (canor01)
**          Above change has potential to overflow stack.  Get dynamic
**          memory instead.
*/
STATUS 
DLcreate(
            char        *exename,
            char        *vers,
            char        *dlmodname,
            char        *in_objs[],
            char        *in_libs[],
            char        *exp_fcns[],
            CL_ERR_DESC *err
)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC+1];
    STATUS	ret;

    if ( (ret = DLgetdir( &loc, locbuf, err )) != OK )
	return (ret);

    return ( DLcreate_loc( exename, vers, dlmodname, in_objs, in_libs,
                           exp_fcns, &loc, "", NULL, FALSE, NULL, err ) );
}

STATUS 
DLcreate_loc(
            char        *exename,
            char        *vers,
            char        *dlmodname,
            char        *in_objs[],
            char        *in_libs[],
            char        *exp_fcns[],
	    LOCATION    *dlloc,
	    char	*parms,
	    LOCATION    *errloc,
	    bool        append,
	    char	*miscparms,
            CL_ERR_DESC *err
)
{
    char	*command = NULL;
    i4		namecnt;
    char	*iidldir;
    LOCATION    curloc;
    char	curloc_buf[ MAX_LOC+1 ];
    STATUS	ret;
    char	dlname[ MAX_LOC+1 ];
    char        *errfile;
    bool	cplusplus = FALSE;
    char	*linkcmd;

    STprintf( dlname, "lib%s%s", dlmodname, DL_EXT_NAME );

    if ( parms == NULL )
	parms = "";

    if ( miscparms != NULL && *miscparms != EOS )
    {
	if ( STstrindex( miscparms, "C++", 0, TRUE ) != NULL )
	    cplusplus = TRUE;
    }

    if ( cplusplus )
	linkcmd = DL_LINKCMD_CPP;
    else
	linkcmd = DL_LINKCMD;

    command = MEreqmem(0, CMDLINSIZ, FALSE, &ret);
    if ( command == NULL )
        return ( ret );
 
    command[0] = EOS;
#ifdef sparc_sol
    STpolycat( 9, linkcmd, " ", DL_LINKFLAGS, " ", parms,
               " -o ", dlname, " -h ", dlname, command );
#else
    STpolycat( 7, linkcmd, " ", DL_LINKFLAGS, " ", parms, " -o ", dlname,
               command );
#endif

    if ( in_objs != NULL )
    {
        for ( namecnt = 0; in_objs[namecnt] != NULL; namecnt++ )
        {
	    STcat( command, " " );
	    STcat( command, in_objs[namecnt] );
        }
    }
    if ( in_libs != NULL )
    {
        for ( namecnt = 0; in_libs[namecnt] != NULL; namecnt++ )
        {
	    STcat( command, " " );
	    STcat( command, in_libs[namecnt] );
        }
    }
#ifdef sparc_sol
    if ( cplusplus )
	{
	    STcat( command, " " );
	    STcat( command, DL_LINKCPPLIB );
	}
#endif

    /* send output to dlmodname.err if errloc is not set */
    if ( errloc == NULL )
    {
        STcat( command, " >" );
        STcat( command, dlmodname );
        STcat( command, ".err" );
    }
	/*
    else
    {
        LOtos( errloc, &errfile );
        STcat( command, " >" );
	STcat( command, errfile );
    }
    */

    /* change to correct directory */
    LOgt( curloc_buf, &curloc );
    LOchange( dlloc );

/*    ret = PCcmdline( NULL, command, PC_WAIT, NULL, err ); */

	 ret = PCdocmdline( NULL, command, PC_WAIT, append, errloc, err );

    MEfree(command);

    LOchange( &curloc );
    if ( ret )
	return (ret);

    if ( (ret = LOfstfile( dlname, dlloc )) != OK )
	return (ret);
    if ( (ret = PEworld( "+r+w+x", dlloc )) != OK )
	return (ret);

    return (OK);
}

