/*
**    Copyright (c) 1991, 2008 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <dl.h>
#include <er.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <si.h>
#include <st.h>
#include <descrip>
#include <limits.h>
#include "dlint.h"

/*
** DL -- the dynamic loading module ("it came from `DY'")
**
**  This module	provides functionality to pull an object into
**  your address space that wasn't there earlier. There	are
**  a number of	base assumptions that we insist	on:
**  1. The object being	pulled in must be a shared library.
**  2. The object must not have unresolved external references.
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
**      16-jul-93 (ed)
**	    added gl.h
**  9-sep-1997 (kinte01)
**    Integrated from Unix CL - change 430676
**    16-jun-1997 (canor01)
**        The "addr" parameter should be a PTR*, not a PTR, to match
**        usage.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**  13-feb-2001 (kinte01)
**	Add DLcreate_loc(), DLprepare_loc(), & DLdelete_loc().
**  13-Mar-2002 (loera01)
**      Re-vamped for Ingres 2.6 on VMS.  DLprepare becomes a wrapper for
**      DLprepare_loc.  Added code for DLcreate, DLcreate_loc, DLdelete,
**      and DLdelete_loc.  This was taken from the Unix implementation.
**      Added support for DLunload: even though it's not possible to unload 
**      an image in VMS, one can still deassign the logical associated with 
**      the file and deallocate the handle.  Eliminated all references to 
**      image descriptor files--these don't apply to VMS.  Took out 
**      "#ifdef's" for non-VMS platforms.  The code for VMS is sufficiently 
**      different that it isn't necessary to reference the Unix code.  
**      DLprepare_loc works a little differently than on Unix, because
**      the underlying LIB$FIND_IMAGE_SYMBOL both maps and loads the shared
**      image in one step.  Thus, if the supplied symbol is found in 
**      DLprepare_loc, it's saved in the returned handle.  If DLbind finds
**      a match between the function it's looking for with the function
**      string stored in its handle from DLprepare_loc, it returns with the 
**      function pointer; otherwise, it does its own LIB$FIND_IMAGE_SYMBOL 
**      via the DLosprepare call, as DLprepare_loc does.
**      It's possible to call DLprepare_loc with the syms argument set to
**      NULL; on VMS, the image will still load, even if it can't map 
**      any function symbols.
**  07-jun-04 (loera01)
**      Added DLload(). 
**  20-nov-07 (Ralph Loen) Bug 119497
**      DLbind() becomes basically a version of DLload() or DL_prepare(),
**      except it uses a location saved in the DL handle.  This is necessary
**      because DLosprepare() now uses a system-level logical for the image,
**      which must be de-assigned as soon as the image is loaded and mapped.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  This CL is prototyped
**	    so no need to keep CL_PROTOTYPED sections.
*/

#define DLMOD_LOGNAME   ERx("DLM")
#define CMDLINSIZ (ARG_MAX)
static i4   lognamcntr=0;

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
static STATUS DLisdir(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4	flagword = (LO_I_TYPE);

    if (LOinfo(locp, &flagword,	&loinf)	!= OK)
	return(DL_DIRNOT_FOUND);
    if ((flagword & LO_I_TYPE) == 0 ||
       (loinf.li_type != LO_IS_DIR))
	return(DL_NOT_DIR);
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
static bool fileexists(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4	flagword = 0;

    return(LOinfo(locp,	&flagword, &loinf) == OK);
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
GLOBALDEF i4 DLdebugset = -1;
static void DLdebugcheck()
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
**  locbuf  A storage area for locp.
**  errp    Set	on any errors.
** Returns:
**  OK	    The	directory was found and	passed simple integrity	checks.
**  other   Some error was detected by a lower-level routine. Either
**	    the	directory specified wasn't found, or it	fails some
**	    basic integrity checks.
** Side	effects:
**  none
*/
static STATUS
DLgetdir(
LOCATION *locp,
char locbuf[],
CL_ERR_DESC *errp)
{
    char *iidldir;
    STATUS ret;
    LOCATION	tmploc;	/* For the couple of times we need a temp var */

    CL_CLEAR_ERR(errp);
    NMgtAt("IIDLDIR", &iidldir);
    if (iidldir	!= NULL	&& iidldir[0] != '\0')
    {
	STcopy(iidldir,	locbuf);
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
	    DLdebug("error constructing	path, ret from NMloc = 0x%x\n",	ret);
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
**  tmpbuf	A storage area for locp.
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
DLconstructobjname(
    char *dlmodname,
    char tmpbuf[],
    LOCATION *tmplocp,
    CL_ERR_DESC	*errp)
{
    LOCATION fileloc;
    char filebuf[MAX_LOC];
    STATUS ret;

    CL_CLEAR_ERR(errp);
    if ((ret = DLgetdir(tmplocp, tmpbuf, errp))	!= OK)
	return(ret);

    STcopy(ERx("lib"), filebuf);
    STcat(filebuf, dlmodname);
    STcat(filebuf, DL_EXT_NAME);
    if ((ret = LOfroms(FILENAME, filebuf, &fileloc)) !=	OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    LOstfile(&fileloc, tmplocp);
    return(OK);
}
static STATUS
DLconstructobjloc(
    char *dlmodname,
    LOCATION *tmplocp,
    CL_ERR_DESC	*errp)
{
    LOCATION fileloc;
    char filebuf[MAX_LOC];
    STATUS ret;

    CL_CLEAR_ERR(errp);
    STcopy(ERx("lib"), filebuf);
    STcat(filebuf, dlmodname);
    STcat(filebuf, DL_EXT_NAME);
    if ((ret = LOfroms(FILENAME, filebuf, &fileloc)) !=	OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }
    LOstfile(&fileloc, tmplocp);
    return(OK);
}


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
    STATUS	ret;
    LOCATION	loc;	/* Directory for DL modules */
    char	locbuf[MAX_LOC];
    char *filename;

    CL_CLEAR_ERR(errp);
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

    return(OK);
}



/*
** Name:    DLprepare
**
** Description:
**	Allow DLbind calls for a module. Might or might	not bind to
**
** Inputs:
**  vers		Not used in VMS.
**  dlmodname		The 8-char (or less) DL	module name
**  syms		List of	symbols	to be used in DLbinds, or NULL
** Outputs:
**  handle		Magic cookie filled in on successful return
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_MOD_NOT_FOUND	DL module name can't be	found.
**  DL_MOD_NOT_READABLE	DL module name can't be	read.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May attach	the DL module during this call,	will likely
**  allocate memory for	the handle.
*/
#ifndef	MAXLINE
#define	MAXLINE	128
#endif

STATUS
DLprepare(char *vers, char *dlmodname, char *syms[], PTR *handle,
    CL_ERR_DESC	*errp)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC+1];
    STATUS	ret;

    CL_CLEAR_ERR(errp);
    if ( (ret = DLgetdir( &loc, locbuf, errp )) != OK )
	return (ret);

    return( DLprepare_loc( vers, dlmodname, syms, &loc, 
			   DL_RELOC_DEFAULT, handle, errp ) );
}

STATUS
DLprepare_loc(char *vers, char *dlmodname, char *syms[], 
              LOCATION *loc, i4  flags, PTR *handle, CL_ERR_DESC *errp)
{
    STATUS	ret;
    char	locbuf[MAX_LOC];
    bool nofile = TRUE;
    DLhandle    *dlhandle = MEreqmem(0,sizeof(DLhandle),FALSE,NULL);
    char        strpid[8];
    char        strcntr[12];

    CL_CLEAR_ERR(errp);
    DLdebugcheck();

    if (loc != NULL)
    {
/*
** See if the shared library file exists.
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
	  
	    nofile = FALSE;
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
/*
** Create a unique logical name that may be checked later by DLbind.
** The name string is formatted as "DLM_strpid_strcntr", where
** strpid is the process ID, and strcntr is a static counter. 
*/ 
    PCunique(strpid);
    CVna(++lognamcntr,strcntr);
    dlhandle->logname = MEreqmem(0,(STlength(DLMOD_LOGNAME)+STlength("_")+
        STlength(strpid)+STlength("_")+STlength(strcntr)+sizeof(char)),
        FALSE,NULL);
    STpolycat(5,DLMOD_LOGNAME,"_",strpid,"_",strcntr,dlhandle->logname);
    CVupper(dlhandle->logname);

    dlhandle->sym = NULL;
    dlhandle->func = NULL;
    dlhandle->location = loc;
/*
** At this point, we should have everything we need.  Now go do it.
*/
    ret	= DLosprepare(loc, syms, errp, dlhandle);
    if ( ret == OK )
    {
       *handle = (PTR)dlhandle;
    }
    return(ret);
}
/*
** Name:    DLload
**
** Description:
**      Allow DLbind calls for a module. Might or might not bind to the
**      symbols provided.
**      This routine is a combination of DLprepare() and DLprepare_loc(),
**      with similar functionality--except that it won't be looking for
**      a file formated as libxxx.exe.  Intead, the user provides
**      the full file name, including the file extension.  The user may 
**      optionally provide a LOCATION pointer if the module to be loaded 
**      doesn't reside in II_SYSTEM:[ingres.library].
**
** Inputs:
**  loc                 The path of the module resolved into a LOCATION.
**  dlmodname           The DL module (file) name.
**  syms                List of symbols to be used in DLbinds, or NULL.
** Outputs:
**  handle              Magic cookie filled in on successful return.
**  errp                A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK                  The location exists, and it's a directory.
**  DL_NOT_IMPLEMENTED  Not implemented on this platform.
**  DL_MOD_NOT_FOUND    DL module name can't be found.
**  DL_MOD_CNT_EXCEEDED Only one (for now) DL module can be bound at a time.
**  DL_VERSION_WRONG    Version was given and checked, but didn't match
**                      the DL module's version string.
**  other               System-specific error status, or file 
** Side effects:
**  See description. May attach the DL module during this call, will likely
**  allocate memory for the handle.
*/

STATUS
DLload(LOCATION *ploc, char *dlmodname, char *syms[], PTR *handle,
    CL_ERR_DESC *errp)
{
    static LOCATION loc; 
    static LOCATION *iloc;
    STATUS      ret;
    LOCATION    txtloc; /* Directory for DL modules */
    char        txtbuf[MAX_LOC];
    char        locbuf[MAX_LOC];
    bool nofile = TRUE;
    DLhandle    *dlhandle = MEreqmem(0,sizeof(DLhandle),FALSE,NULL);
    char        strpid[8];
    char        strcntr[12];

    LOCATION fileloc;

    CL_CLEAR_ERR(errp);
    DLdebugcheck();

    if (ploc == NULL)
    {
        if ((ret = NMloc(LIB, PATH, NULL, &loc)) != OK)
            return (ret);
        iloc = &loc;
    }
    else
        iloc = ploc;

    if (iloc == NULL)
    {
        SETCLERR(errp, DL_MOD_NOT_FOUND, 0);
        return(DL_MOD_NOT_FOUND);
    }

    /*
    ** See if the shared library file exists.
    */
    if ((ret = DLconstructobjloc(dlmodname, iloc, errp)) != OK)
        return ret;

    if ((ret = LOfroms(FILENAME, dlmodname, &fileloc)) !=	OK)
    {
        SETCLERR(errp, ret, 0);
        return(ret);
    }

    LOstfile(&fileloc, iloc);

    if (fileexists(iloc))
    {
        i4		errcode;
        LOINFORMATION	loinfo;
        i4  		flags;

        /* 
        ** If the file exists, but is not readable, say so.
        */
        flags = LO_I_PERMS;
        LOinfo( iloc, &flags, &loinfo );
        if ( (flags & LO_I_PERMS) != 0  &&
             (loinfo.li_perms & LO_P_READ) == 0 )
        {
            SETCLERR(errp, DL_MOD_NOT_READABLE, 0);
            return (DL_MOD_NOT_READABLE);
        }	  
        nofile = FALSE;
    }

    if (nofile)
    {
        char		*filename;

        LOtos(iloc, &filename);
        DLdebug("DL module %s doesn't exist: %s not found\n",
        dlmodname, filename);
        SETCLERR(errp, DL_MOD_NOT_FOUND, 0);
        return(DL_MOD_NOT_FOUND);
    }

    /*
    ** Create a unique logical name that may be checked later by DLbind.
    ** The name string is formatted as "DLM_strpid_strcntr", where
    ** strpid is the process ID, and strcntr is a static counter. 
    */ 
    PCunique(strpid);
    CVna(++lognamcntr,strcntr);
    dlhandle->logname = MEreqmem(0,(STlength(DLMOD_LOGNAME)+STlength("_")+
        STlength(strpid)+STlength("_")+STlength(strcntr)+sizeof(char)),
        FALSE,NULL);
    STpolycat(5,DLMOD_LOGNAME,"_",strpid,"_",strcntr,dlhandle->logname);
    CVupper(dlhandle->logname);

    dlhandle->sym = NULL;
    dlhandle->func = NULL;
    dlhandle->location = iloc;

    /*
    ** At this point, we should have everything we need.  Now go do it.
    */
    ret	= DLosprepare(iloc, syms, errp, dlhandle);
    if ( ret == OK )
    {
       *handle = (PTR)dlhandle;
    }
    return(ret);
}

/*
** Name:    DLbind
**
** Description:
**	Map a function call to a DL module.
**
** Inputs:
**  syms		List of	symbols	to be used in DLbinds, or NULL
** Outputs:
**  addr                Address of mapped function.
**  handle		Magic cookie possibly containing info from 
**                      DLprepare or DLprepare_loc.
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_MOD_NOT_FOUND	DL module name can't be	found.
**  DL_LOOKUP_BAD_HANDLE  Supplied handle is NULL.
**  DL_FUNC_NOT_FOUND   The image has no reference to the symbol.
**  other		System-specific	error status, or file 
** Side	effects:
**  See	description. May attach	the DL module during this call.
*/
STATUS
DLbind(PTR handle, char	*sym, PTR *addr,
    CL_ERR_DESC	*errp)
{
    char *syms[2];
    DLhandle *dlhandle = (DLhandle *)handle;
    LOCATION *loc=dlhandle->location;

    CL_CLEAR_ERR(errp);
    if ( dlhandle == NULL )
    {
        SETCLERR(errp, DL_LOOKUP_BAD_HANDLE, 0);
	return(DL_LOOKUP_BAD_HANDLE);
    }
    syms[0] = sym;
    syms[1] = NULL;
    /*
    ** If the handle already contains the symbol we are looking for, no 
    ** need to go further, as DLprepare or DLprepare_loc has already mapped it.
    */
    if ( syms[0] != NULL && ! STcmp(syms[0],dlhandle->sym))
    {
        *addr = dlhandle->func;
        return(OK);
    }
    else  if  ( DLosprepare(loc, &syms, errp, dlhandle ) != OK )
    {
	SETCLERR(errp, DL_FUNC_NOT_FOUND, 0);
	return(DL_FUNC_NOT_FOUND);
    }
    *addr = dlhandle->func;
    return(OK);
}

/*
** Name:    DLunload
**
** Description:
**  Break the logical ties to a DL module.
**
** Inputs:
**  handle		Magic cookie from DLprepare, MEfree'd during this call.
** Outputs:
**  errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**  OK			The location exists, and it's a	directory.
**  DL_LOOKUP_BAD_HANDLE    Handle is invalid for some reason.
** Side	effects:
**  Will MEfree the handle if possible, and deassign the logical assigned
**  to the module, if possible.
*/
STATUS
DLunload(PTR handle, CL_ERR_DESC *errp)
{
    CL_CLEAR_ERR(errp);
    if ( handle == NULL )
    {
        SETCLERR(errp, DL_LOOKUP_BAD_HANDLE, 0);
	return(DL_LOOKUP_BAD_HANDLE);
    }
    LIB$DELETE_LOGICAL(((DLhandle *)handle)->logname);
    if (((DLhandle *)handle)->logname != NULL)
        MEfree (((DLhandle *)handle)->logname);
    if (((DLhandle *)handle)->sym != NULL)
        MEfree (((DLhandle *)handle)->sym);
    MEfree(handle);
    return OK;
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

    CL_CLEAR_ERR(err);
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

    CL_CLEAR_ERR(err);
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

    STpolycat( 7, linkcmd, " ", DL_LINKFLAGS, " ", parms, " -o ", dlname,
               command );

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

    ret = PCcmdline( NULL, command, PC_WAIT, NULL, err ); 

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


/*
** Name:    DLdelete_loc
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
    LOCATION loc;
    char	locbuf[MAX_LOC+1];
    STATUS	ret;

    CL_CLEAR_ERR(errp);
    if ( (ret = DLgetdir( &loc, locbuf, errp )) != OK )
	return (ret);

    return( DLdelete_loc( dlmodname, &loc, errp ) );
}

STATUS
DLdelete_loc(char *dlmodname, LOCATION *loc, CL_ERR_DESC *errp)
{
    STATUS	ret;
    char	locbuf[MAX_LOC];

    CL_CLEAR_ERR(errp);
    DLdebugcheck();
    if ((ret = DLconstructobjloc(dlmodname, loc, errp)) != OK)
	return(ret);
    if (fileexists(loc) && (ret = LOdelete(loc)) != OK)
    {
	SETCLERR(errp, ret, 0);
	return(ret);
    }

    return(OK);
}
