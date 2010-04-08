/*
**	Copyright (c) 1992, 1998 Ingres Corporation
**	All rights reserved.
*/
# include	<compat.h>
# include	<lo.h>
# include	<dl.h>
# include	<me.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	<pe.h>

/**
** Name:    dl.c -	Library routines to interface to dynamic linking
**                  mechanism.
**
** Description:
**
**      The DL module is intended for use in the Windows4GL code to support
**      calling customer-written C procedures.  It might be used for other
**      mechanisms such as User-Define Types (UDT), or the ABF interpreter.
**
**      The CL allows only dynamic loading of C callable functions.
**
**          The following interfaces are provided:
**
**          DLbind      Ensure a function can be called.
**          DLcreate    Create a persistent DL module for later binding.
**          DLdelete    Remove a persistent DL module.
**          DLnameavail See if a DL module name is available for use.
**          DLprepare   Allow a DLbind calls for a module.
**          DLunload    Get rid of a prepared or bound module.
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
** 	10-21-92 rionc
**	    fixes to make 3gl work right
** 	02-02-94 valerier
**	    DLprepare was changed so that environment variable II_LIBU3GL
**	    takes precedence over argument dlmodname
** 	03-15-94 valerier  function DLprepare
**	    Crashing bug fixed... Check that DllStr is not NULL before
**	    dereferencing it.
**	29-apr-94 (leighb)
**	    Add Win32s thunking.
**	12-may-94 (brett)
**	    Remove define for W32SUT_32, it's now in the CFLAGS.
**	13-may-94 (brett)
**	    This file was not changed in a way to conditionally
**	    compile Win32s.  Made a best guess to add ifdef's
**	    for WIN32S.
**	16-may-94 (valerier)
**	    Put define for W32SUT_32 at top of file due to compile
**	    error.  Upon investigation of this approach, this is
**	    within the coding standards.
**	25-jul-94 (leighb)
**	    Saved 16-bit to 32-bit callback function pointer in CL DLL;
**	    Made PCsleep() callable form 16-bit land;
**	    These 2 changes are part of Win32s spawn with wait.
**	26-jul-94 (leighb)
**	    Moved PCsleep() to 16-bit land, so removed PCSLEEP from table.
**	02-aug-94 (leighb)
**	    Moved Win32s callback thunking routines to embed\libq\iicallbk.c
**	01-sep-94 (fraser)
**	    Added call to IIDBIsupSavepfnUTProc to pass pfnUTProc
**	    to the qlib DLL.  A dummy version of IIDBISavepfnUTProc
**	    is linked into the LIBQ DLL to avoid linking problems.
**	7-oct-94 (fraser)
**	    DBI change.  Added definition of THUNK_16BIT_DLL.
**	    The definition and related changes are added at the
**	    end of the file under the section headed "Initialize
**	    WIN32S" so that all the Win32s stuff will be kept
**	    together.
**	02-nov-94 (leighb)
**	    Moved Win32s thunking to ...\cl\clf\ss\iiutload.c so that it
**	    would be in CL32_1.DLL.
**	15-nov-94 (leighb)
**	    Added FreeLibrary to Thunk layer; call the 16-bit version
**	    when freeing a 16-bit library.
**	01-dec-94 (leighb)
**	    Call 16-bit FreeLibrary twice for 16-bit DLLs:  needed because
**	    the 32-bit attempt to load a 16-bit DLL inc's the useage count
**	    even tho it fails.
**	10-dec-94 (fraser)
**	    Added prototype for IIwin32s() because it was
**	    undefined.  (IIwin32s is defined in libq: iiutload.c.)		
**	22-may-95 (leighb)
**	    Replace "IIwin32s()" with "IIgetOSVersion()".
**	24-may-95 (leighb)
**	    Added Win95 generic thunking.
**	08-nov-95 (chan) b71127
**	    DLprepare: if -m flag (dlmodname) is specified on the
**	    command line, it should override II_LIBU3GL.
**	10-oct-1996 (canor01)
**	    Strip to bare bones needed for the Windows NT server.  Allow
**	    the module name to be either a filename (xxxx.DLL) or an
**	    environment variable.  This allows maximum portability between
**	    Windows and Unix.
**	25-oct-1996 (canor01)
**	    Add DLcreate(), DLcreate_loc() and DLprepare_loc()
**	    (for initial use with Jasmine).
**	14-nov-1996 (mcgem01)
**	    Removed erclf.h which isn't actually needed and
**	    won't be created until we have ercomp.exe.
**	24-mar-1997 (canor01)
**	    Added DLdelete_loc() to parallel DLcreate_loc() and
**	    DLprepare_loc().  Reformatted comments to match coding
**	    standard so they can be viewed in common editors.
**	    Restored error return in DLprepare() if no module can
**	    be found.
**	07-may-1997 (canor01)
**	    Add DLconstructloc().
**	15-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	02-jun-97 (mcgem01)
**	    Increase BUFSIZE to MAX_CMDLINE.
**      04-jun-1997 (canor01)
**          Modified DLunload() to just unload the currently referenced module.
**          Cleaned up some formatting to match coding spec.
**	16-jun-1997 (canor01)
**	    Clean up compiler warnings.
**	18-jun-1997 (canor01)
**	    Link can not take extremely long command line, so write
**	    it to a response file.  Also, if there are environment variables
**	    in the command, the response file will not expand them, so do
**	    the substitution ourselves.
**	13-aug-1997 (canor01)
**	    Added 'flags' parameter to DLprepare_loc().  This will allow
**	    caller to state preference for symbol resolution (immediate
**	    or deferred).  And ignore this value for NT.
**	08-sep-1997 (canor01)
**	    Add 'append' flag to DLcreate_loc() to allow linker messages to
**	    be appended to the output file.
**	30-dec-1997 (canor01)
**	    Add miscparms parameter to DLcreate_loc() to allow specifying
**	    additional information (such as C/C++).
**	10-feb-1998 (canor01)
**	    Do not reuse the passed in location for storing the DEF filename.
**	02-mar-1998 (litom50)
**	    Make a copy of parameter #1 of expandenv() before calling it
**	    because expandenv changes the memory parameter #1 points to and
**	    therefore is not MT-safe in some cases.
**	13-aug-1998 (canor01)
**	    If location passed to DLprepare_loc is NULL, use the default OS
**	    search for the module.
**	13-nov-1998 (canor01)
**	    Try to return a more useful error if LoadLibrary fails while doing
**	    a DLbind().
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      20-apr-2004 (loera01)
**          Added new DLload() routine for compatibility with Unix.  On
**          Windows, DLload() is just a wrapper for DLprepare_loc().
*/


typedef struct _DLLDESC DLLDESC;
typedef struct _DLLDESC
{
	HANDLE		DllHandle;           /* module handle if the DLL is
					                 ** loaded, NULL otherwise */
	DLLDESC     *Next;               /* next element pointer */
        DLLDESC     *Prev;
	char		DllName[_MAX_PATH];  /* pathname for the DLL module */
	bool		Win32s;		         /* TRUE -> 16 bit DLL, need to thunk */
};


DLLDESC	*DllList = NULL;
STATICDEF	char  nameOfDll[_MAX_PATH];

VOID    iDLcleanup(VOID);
static void expandenv( char *input, char *output );

/* forward declarations */
static STATUS DLconstructname(
    char *dlmodname,
    char tmpbuf[],
    LOCATION *tmplocp,
    char *ext,
    CL_ERR_DESC *errp);
static STATUS DLconstructxloc(
    char *dlmodname,
    LOCATION *tmplocp,
    char *ext,
    CL_ERR_DESC *errp);
static STATUS DLgetdir(
    LOCATION *locp,
    char locbuf[],
    CL_ERR_DESC *errp);
static STATUS DLisdir(LOCATION *locp);
static bool fileexists(LOCATION *locp);

/**
** Name:    DLbind  - Bind a function and return its location
**
** Description:
**      DLbind takes a handle returned by DLprepare, ensures the requested
**      function is loaded and executable, and returns its location.
**
**      Functions not present on the lists passed to DLcreate and DLprepare
**      might not be found, but they might pass through.  Clients may attempt
**      to bind functions not on the lists, and they may be found, but they
**      should not rely on this behavior.
**
** Inputs:
**          sym             EOS terminated function name.
**
**          handle          This is the handle returned by a successful call
**                          to DLprepare, and not invalidated by a call to
**                          DLunload.
** Outputs:
**          addr            Upon successful return, this is set to the address
**                          of the function whose name was given in sym.
**
**          err             Pointer to variable used to return system-specific
**                          errors.
**
** Returns:
**          OK
**          DL_NOT_IMPLEMENTED
**          DL_FUNC_NOT_FOUND
**          DL_MOD_NOT_FOUND
**	    DL_VERSION_WRONG
**
** Side Effects:
**
** Implementation Note:
**      The handle given to this routine is the pointer to the DLL
**      descriptor (DLLDESC) linked list.  This routine searches for
**      the requested function from those DLL.  If a DLL is not yet loaded,
**      load it now.
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
**		26-apr-94 (leighb)
**			Added Win32s processing.
**		30-aug-94 (leighb)
**			If LoadLibrary() fails, check for error == ERROR_BAD_EXE_FORMAT;
**			if so, this is a 16-bit DLL - use Win32s thunking.
**		21-sept-95 (leighb)
**			Changed 'GLOBALREF's to 'extern's so that these will not have
**			Thread-Local-Storage.  THESE MUST BE externS!
**		23-may-95 (leighb)
**			Return DL_VERSION_WRONG for 16-bit DLL's on Win95.
**      03-jun-1997 (canor01)
**          Return DL_FUNC_NOT_FOUND when symbol not found in DLL.
**	16-jun-1997 (canor01)
**          The "addr" parameter should be a (PTR *).
*/

STATUS
DLbind(PTR handle, char *sym, PTR *addr, CL_ERR_DESC *err)
{
    DLLDESC *ModDesc;
    HANDLE   hDllTmp;
    FARPROC  pfnTmp = NULL;
    STATUS   retval = DL_MOD_NOT_FOUND;
    DWORD    dwLastError;

    /*  rionc
    **  check each possible DLL module for the desired
    **  function.  If not found in one, check the rest anyway.
    **  We may even find some DLL's are missing, but continue
    **  trying all of them.
    */

    *addr = NULL; /* in case we don't find anything */

    ModDesc = (DLLDESC *)handle;	/* the DLL list */
	
    while (ModDesc)
    {
        if (ModDesc->DllHandle == NULL) /* load the module if not yet loaded */
        {
            SetLastError(0);
            hDllTmp = LoadLibrary(ModDesc->DllName); 
            if (hDllTmp)
            {
		ModDesc->DllHandle = hDllTmp;
		ModDesc->Win32s = FALSE;
            }
            else
            {
                retval = DL_OSLOAD_FAIL;
                dwLastError = GetLastError();
                switch( dwLastError )
                {
                case ERROR_MOD_NOT_FOUND:
                    retval = DL_MOD_NOT_FOUND;
                    break;
                case ERROR_NOACCESS:
                    retval = DL_MEM_ACCESS;
                    break;
                }
            }
        }
        
	if (ModDesc->DllHandle)	/* if we found a dll */
	{
	    if (ModDesc->Win32s == FALSE)
	    {
		pfnTmp = GetProcAddress( ModDesc->DllHandle, sym );
  	    }
	    if (pfnTmp)
	    {
		*addr = (char *)pfnTmp;
		retval = OK;
		break;
	    }
            else
            {
                dwLastError = GetLastError();
            }
	}
        ModDesc = ModDesc->Next;
    } /* while there are DLL's to check */

    if ( pfnTmp != NULL )
        retval = OK;

    return(retval);
}


/**
** Name:    DLcreate  - Create module for DLprepare/DLbind dynamic loading
**
** Description:
**      Create a module which can be passed to DLprepare.  The user provides
**      a set of object-file names, library names, and a system-specific
**      string against which to resolve symbols.
**
**      DLcreate might link all of the object files given, resolving first
**      against the symbol table of the executable file and then looking to
**      the libraries for any unresolved symbols, or it might leave a
**      permanent identification of the part for later use by DLprepare
**      and DLbind.
**
**      The list of libraries contains system specific library designation,
**      e.g. "-lm" and "VAXCRTL:".  The list of libraries may be empty, and
**      the name of the executable file may be given as NULL, which means
**      "don't resolve against any executable file's symbol table." If no
**      objects or libraries are supplied, nothing will be in the output
**      module.
**
**      If the user provides a version string, it must be encoded into the
**      output module for use by DLprepare.
**
**      The users provides a list of function names which will be made
**      available to DLprepare for symbol table lookups.  The list may be
**      empty, in which case no functions are guaranteed to be in the output
**      module.  DLcreate must ensure that any functions in the list will be
**      seen by a subsequent DLprepare and DLbind.  The places the error
**      checking burden on the program creating the module rather than on
**      those attemping to load it later.
**
**      The operation of DLcreate is undefined if there are conflicts
**      between the symbols defined by the exename and any of the input
**      libraries.  DLcreate might return an error, or it might create a
**      module selecting one of the definitions at random.
**
** Inputs:
**          exename         If non-null (and *exename != EOS), a C string
**                          identifying the executable the output module
**                          will be used with, in a way that will allow
**                          symbol resolution if that is required.
**
**          vers            If non-null (and *vers != EOS), a pointer to
**                          a version string to be embedded in the output
**                          module.
**
**          dlmodname       A pointer to a C string containing the name of
**                          a DL module to create, using the format described
**                          above.
**          in_objs         An array of C string, with NULL entry as its last
**                          element, containing system-specific
**                          specification for names of input object files.
**
**          in_libs         An array of C string, with NULL entry as its last
**                          element, containing system-specific
**                          specifications for names of input libraries.
**
**          exp_fncs        An array of C string, with NULL entry as its last
**                          element, containing the function names that can
**                          be found using DLprepare and/or DLbind.
** Outputs:
**          err             Pointer to variable used to return system-specific
**                          errors.
** Returns:
**          OK
**          DL_NOT_IMPLEMENTED
**
** Side Effects:
**          none
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
**      03-jun-1997 (canor01)
**          Allow the DLLs to be built with debug information if it
**          exists in the individual obj files.
**	13-jun-1997 (canor01)
**	    Allow specifying the file to recieve the output from
**	    the link.
**	24-jun-1998 (zhoqi02)
**	    Changed "LINK" to "LINK.EXE" to solve the CompileProcedure
**	    error when using SAMBA, SMB (Problem #39).

*/
STATUS
DLcreate(char *exename, char *vers, char *dlmodname, 
        char *in_objs[], char *in_libs[], char *exp_fncs[], 
        CL_ERR_DESC *err)
{
    LOCATION    loc;
    char        locbuf[MAX_LOC+1];
    STATUS      ret;

    if ( (ret = DLgetdir( &loc, locbuf, err )) != OK )
        return (ret);

    return ( DLcreate_loc( exename, vers, dlmodname, in_objs, in_libs,
                           exp_fncs, &loc, "", NULL, FALSE, NULL, err ) );
}

STATUS
DLcreate_loc(
            char        *exename,
            char        *vers,
            char        *dlmodname,
            char        *in_objs[],
            char        *in_libs[],
            char        *exp_fncs[],
            LOCATION    *dlloc,
            char        *parms,
	    LOCATION    *errloc,
	    bool	append,
	    char	*miscparms,
            CL_ERR_DESC *err
)
{
    char        command[ MAX_CMDLINE ];
    char        cmd1[ MAX_CMDLINE ];
    char        cmd2[ MAX_CMDLINE ];
    i4          namecnt;
    LOCATION    curloc;
    char        curloc_buf[ MAX_LOC+1 ];
    STATUS      ret;
    char        dlname[ MAX_LOC+1 ];
    LOCATION	defloc;
    char	defloc_buf[ MAX_LOC+1 ];
    char	defname[ MAX_LOC+1 ];
    FILE	*deffile;
    LOCATION    rsploc;
    char	rsploc_buf[ MAX_LOC+1 ];
    char        rspname[ MAX_LOC+1 ];
    FILE        *rspfile;
    char	*rspptr;
    i4          byteswritten;
    char	modstring[ MAX_LOC+1 ];
    bool	cplusplus = FALSE;
    char	*linkcmd;

    STprintf( dlname, "%s.DLL", dlmodname );

    cmd1[0] = EOS;

    if ( parms == NULL )
	parms = "";

    if ( miscparms != NULL && *miscparms != EOS )
    {
        if ( STstrindex( miscparms, "C++", 0, TRUE ) != NULL )
            cplusplus = TRUE;
    }
 
    if ( cplusplus )
        linkcmd = DL_LINKCMD_CPP;

    LOcopy( dlloc, rsploc_buf, &rsploc );
    STprintf( rspname, "%s.RSP", dlmodname );
    LOfstfile( rspname, &rsploc );
    LOtos( &rsploc, &rspptr );
    SIfopen( &rsploc, "w", SI_TXT, 1, &rspfile );

    STprintf( command, 
	      "LINK.EXE /DEBUG /DEBUGTYPE:both /NOLOGO /DLL /OUT:%s %s @%s", 
              dlname, parms, rspptr );

    /*
    ** If the user has specified a list of functions to
    ** be exported, build a .DEF file to define them as
    ** exported to the linker.  (Otherwise, assume all functions
    ** to be exported are marked for export on the compile.)
    */
    if ( exp_fncs != NULL )
    {
        LOcopy( dlloc, defloc_buf, &defloc );
        STprintf( defname, "%s.DEF", dlmodname );
        LOfstfile( defname, &defloc );
        SIfopen( &defloc, "w", SI_TXT, 1, &deffile );
	SIfprintf( deffile, "EXPORTS\n" );
        for ( namecnt = 0; exp_fncs[namecnt] != NULL; namecnt++ )
	{
	     SIfprintf( deffile, "    %s\n", exp_fncs[namecnt] );
	}
	SIclose( deffile );
	STcat( command, " /DEF:" );
	STcat( command, dlmodname );
	STcat( command, ".DEF" );
    }
    
    /* build list of object files */
    if ( in_objs != NULL )
    {
        for ( namecnt = 0; in_objs[namecnt] != NULL; namecnt++ )
        {
            STcopy( in_objs[namecnt], cmd2 );
            expandenv( cmd2, modstring );
            STcat( cmd1, " " );
            STcat( cmd1, modstring );
        }
    }
    /* additional libraries? */
    if ( in_libs != NULL )
    {
        for ( namecnt = 0; in_libs[namecnt] != NULL; namecnt++ )
        {
            STcopy( in_libs[namecnt], cmd2 );
	    expandenv( cmd2, modstring );
            STcat( cmd1, " " );
            STcat( cmd1,  modstring );
        }
    }

    SIwrite( STlength( cmd1 ), cmd1, &byteswritten, rspfile );
    SIclose( rspfile );

    /* send output to dlmodname.ERR if errloc is not set */
    if ( errloc == NULL )
    {
        STcat( command, " >" );
        STcat( command, dlmodname );
        STcat( command, ".ERR" );
    }

    /* change to correct directory */
    LOgt( curloc_buf, &curloc );
    LOchange( dlloc );

    ret = PCdocmdline( NULL, command, PC_WAIT, append, errloc, err );

    LOchange( &curloc );

    if ( ret )
        return (ret);

    LOcopy( dlloc, curloc_buf, &curloc );

    if ( (ret = LOfstfile( dlname, &curloc )) != OK )
        return (ret);
    if ( (ret = PEworld( "+r+w+x", &curloc )) != OK )
        return (ret);

    return (OK);
}


/**
** Name:    DLdelete  - Delete the persistent version of a DL module
**
** Description:
**      Remove a DL module that has been created by DLcreate.
**
** Inputs:
**          dlmodname       The name of a DL module.
**
** Outputs:
**          err             Pointer to variable used to return system-specific
**                          errors.
**
** Returns:
**          OK
**          DL_NOT_IMPLEMENTED
**
** Side Effects:
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
**	24-mar-1997 (canor01)
**	    Implemented for Windows NT.  Added DLdelete_loc() to parallel
**	    DLcreate_loc() and DLprepare_loc().
**  03-jun-1997 (canor01)
**      When modules are being loaded one at a time and not from a
**      list of names, return the single node, not the head of the list.
*/
STATUS
DLdelete(char *dlmodname, CL_ERR_DESC *err)
{
    LOCATION    loc;
    char        locbuf[MAX_LOC+1];
    STATUS      ret;

    if ( (ret = DLgetdir( &loc, locbuf, err )) != OK )
        return (ret);

    return( DLdelete_loc( dlmodname, &loc, err ) );
}

STATUS
DLdelete_loc(char *dlmodname, LOCATION *loc, CL_ERR_DESC *err)
{
    STATUS	ret;

    /*
    ** We check if the name references a file
    */
    if ((ret = DLconstructxloc( dlmodname, loc, ".DLL", err )) != OK)
        return ( ret );

    if ( fileexists( loc ) && (ret = LOdelete( loc )) != OK )
    {
        SETCLERR( err, ret, 0 );
        return ( ret );
    }

    return (OK);
}


/**
** Name:    DLnameavail  - Check if the given DL module exist.
**
** Description:
**      DLnameavail checks that the object name can be used to create a
**      DL module using DLcreate.  Although it mainly checks that the name
**      is unused by any existing DL module, it may return more information
**      in the error returns.
**
** Inputs:
**          dlmodname       The name of a DL module.
**
** Outputs:
**          err             Pointer to variable used to return system-specific
**                          errors.
**
** Returns:
**          OK
**          DL_NOT_IMPLEMENTED
**          DL_MODULE_PRESENT
**
** Side Effects:
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
*/
STATUS
DLnameavail(char *dlmodname, CL_ERR_DESC *err)
{

    return(DL_NOT_IMPLEMENTED);
}


/**
** Name:    DLprepare  - Allow module to be bound to address space for
**                       execution.
**
** Description:
**      DLprepare allows the given module to be bound to the address space
**      of the invoking process, making the assumption that the module has
**      been prepared by DLcreate or a process that imitates it.  The module
**      may or may not actually be bound at this time.  Items will be bound
**      when calls to DLbind succeed.
**
**      Althogh no performance requirements are given in this specification,
**      DLprepare must be fast enough that a user-level program can bring
**      user-provided code into its address space quickly.
**
**      An error might be returned for any of the following reasons:
**          1.  Insufficient address space;
**          2.  Insufficient file descriptors, if used in implementation;
**          3.  A necessary file was the wrong 'file type' or lacked the
**              requisite permissions.
**
**      If a version string is given and is non-null, it's assumed that
**      there is an internal version string stored in the loaded module,
**      or if the two strings don't exactly match, an error is returned
**      and the module is not loaded.
**
**      A list of functions is given, which give information to DLprepare
**      telling it which functions will potentially be bound later, in
**      case it needs to save information about those functions.  Functions
**      not handed in to DLprepare may not be visible at the time a
**      DLbind is attempted.
**
** Inputs:
**          vers            If non-null (and *vers != EOS), a pointer to
**                          a version string to be checked against one
**                          embeded in the loaded module.
**
**          dlmodname       The name of a DL module.  This is used to
**                          construct the name of the DL module that is
**                          loaded into the address space and nust be the
**                          same string as the one passed to DLcreate to
**                          create the DL module.
**
**          syms            An array of C-string with a NULL pointer as the
**                          last element in the array.  This lists function
**                          name as you would write them in a C program.
**                          DLprepare can use this as an initial list of
**                          functions the user's interested in locating in
**                          the DL module.  DLbin is allowed to restrict
**                          function lookups to this list.
** Outputs:
**          handle          A generic pointer used for management of DL
**                          support routines.
**
**          err             Pointer to variable used to return system-specific
**                          errors.
**
** Returns:
**          OK
**          DL_MOD_NOT_FOUND
**          DL_MODULE_COUNT_EXCEEDED
**          DL_NOT_IMPLEMENTED
**          DL_VERSION_WRONG
**
** Implementation Note:
**      For OS/2 PM and Windows, the "dlmodname" is an environment variable
**      specifying a string of full DLL pathnames.  These pathnames are
**      separated by semicolon.  Functions are searched from these DLL
**      in the order as listed in the string, from left to right.
**
**      If "dlmodname" is not specified, the environment variable II_LIBU3GL
**      is used as the default.
**
**      DLprepare simply breaks up the string of DLL pathnames into the
**      linked list of internal DLL descriptor.  The actual action of
**      loading and locating functions is defered until DLbind is called.
**
** Side Effects:
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
**		11-mar-94 (rion)
**			return OK if no DLL modules are specified.
**	24-mar-1997 (canor01)
**	    Restored error return if no module can be found.
*/
STATUS
DLprepare(char *vers, char *dlmodname, char *syms[], PTR *handle,
        CL_ERR_DESC *err)
{
    LOCATION    loc;
    char        locbuf[MAX_LOC+1];
    STATUS      ret;

    if ( (ret = DLgetdir( &loc, locbuf, err )) != OK )
        return (ret);

    return( DLprepare_loc( vers, dlmodname, syms, &loc, 
    			   DL_RELOC_DEFAULT, handle, err ) );
}

STATUS
DLload(LOCATION *ploc, char *dlmodname, char *syms[], PTR *handle,
    CL_ERR_DESC *errp)
{
    LOCATION loc, *iloc = &loc;
    STATUS      ret;
    LOCATION    txtloc; /* Directory for DL modules */
    char        txtbuf[MAX_LOC];
    char        locbuf[MAX_LOC];

    if (ploc == NULL)
    {
        if ((ret = NMloc(BIN, PATH, NULL, iloc)) != OK)
            return (ret);
    }
    else
        iloc = ploc;

    return( DLprepare_loc( NULL, dlmodname, syms, iloc, 
    			   DL_RELOC_DEFAULT, handle, errp ) );
}
STATUS
DLprepare_loc(char *vers, char *dlmodname, char *syms[],
              LOCATION *loc, i4  flags, PTR *handle, CL_ERR_DESC   *errp)
{
    DLLDESC *ModDesc, *listPtr;
    char    *DllStr, *EndPtr;
    STATUS  status;
    i4      len;
    STATUS      ret;
    bool nofile = TRUE;
    bool parsename = TRUE;

    DllStr = NULL;

    /* if we are given a literal name, use it instead of trying to parse it */
    len = STlength(dlmodname);
    if ( len > 4 &&
         STbcompare( ".exe", 4, dlmodname + (len-4), 4, TRUE) == 0 ||
         STbcompare( ".dll", 4, dlmodname + (len-4), 4, TRUE) == 0 )
    {
        parsename = FALSE;
    }
         

  if ( loc == NULL )
  {
    LOCATION tmploc;
    char     locbuf[MAX_LOC+1] = "";
    LOCATION fnameloc;
    char     fnamebuf[MAX_LOC+1] = "";

    if ( parsename )
    {
        DLgetdir(&tmploc, locbuf, errp);
        if ((ret = DLconstructxloc(dlmodname, &tmploc, ".DLL", errp)) != OK)
            return ret;
    }
    else
    {
        LOfroms(FILENAME, dlmodname, &tmploc);
    }
    LOfroms(FILENAME, fnamebuf, &fnameloc);
    LOgtfile(&tmploc, &fnameloc);
    LOtos( &fnameloc, &DllStr );
    nofile = FALSE;
  }
  else
  {
    /*
    ** We check if the name references a file
    */
    if ((ret = DLconstructxloc(dlmodname, loc, ".DLL", errp)) != OK)
        return ret;

    if (fileexists(loc))
    {
        nofile = FALSE;
        LOtos( loc, &DllStr );
    }
  }

    /* not a file, get the environment variable */
    if ( nofile )
    {
    /* get the string specifying the DLL list */

	if (dlmodname && *dlmodname)
		NMgtAt(dlmodname, &DllStr);

	if (DllStr == NULL)
		return OK;
    }

    /* get rid of leading white characters and ';' */

    while (*DllStr && (*DllStr <= ' ' || *DllStr == ';'))
        DllStr++;

    if (*DllStr == '\0')
    {
	return ( DL_MOD_NOT_FOUND );
    }

    /* break the string into individual elements */
    do
    {
        EndPtr = strchr(DllStr, ';');

        if (EndPtr)
            len = EndPtr - DllStr;
        else
            len = strlen(DllStr);

        if (!len)
            continue;

        if (len >= _MAX_PATH)
            len = _MAX_PATH - 1;

        ModDesc = (DLLDESC *)MEreqmem(0, sizeof(DLLDESC), TRUE, &status);

        if (status)
        {
            return(status);
        }

        memcpy(ModDesc->DllName, DllStr, len);
 		ModDesc->DllName[len] = EOS;	/* 17-feb-94 ...puree */
        ModDesc->DllHandle = NULL;

        DllStr = EndPtr + 1;

        /* append the new element to the DLL descriptor list */

		listPtr = DllList;

        if (listPtr == NULL)
        {
            DllList = ModDesc;
        }
        else
        {
            while (listPtr->Next)
				listPtr = listPtr->Next;

            listPtr->Next = ModDesc;
            ModDesc->Prev = listPtr;
        }
    } while (EndPtr);
    
    if ( nofile )
        *handle = (PTR)DllList;
    else
        *handle = (PTR)ModDesc;

    /* set up function pointer for cleaning up */
    PCatexit(iDLcleanup);  
    return(OK);
}


/**
** Name:    DLunload  - Unload module
**
** Description:
**      Destroy any bound between address space and dynamically loaded
**      module that DLprepare and DLbind may have previously established.
**      After this returns, the handle is no longer valid for use by
**      DLbind.
**
** Inputs:
**          handle          A handle previously returned from DLprepare.
**
** Outputs:
**          err             Pointer to variable used to return system-specific
**                          errors.
**
** Returns:
**          OK
**          DL_NOT_IMPLEMENTED
**
** Side Effects:
**
** History:
**      08/11/92 puree
**          written for OS/2 v2.0
**      09/92    valerier
**          ported to Windows NT
**      04-jun-1997 (canor01)
**          DLunload() will just unload the currently referenced
**          module. iDLcleanup() will still be called at program
**          exit.  Remove from module descriptor list.
*/
STATUS
DLunload(PTR handle, CL_ERR_DESC *err)
{
    DLLDESC *ModDesc;

    ModDesc = (DLLDESC *)handle;
    if ( ModDesc->DllHandle != NULL )
    {
        if ( ModDesc->Win32s == FALSE )
        {
            FreeLibrary( ModDesc->DllHandle );
        }
    }
    ModDesc->DllHandle = NULL;

    /* remove from list */
    if ( ModDesc->Next != NULL )
        ModDesc->Next->Prev = ModDesc->Prev;
    if ( ModDesc->Prev != NULL )
        ModDesc->Prev->Next = ModDesc->Next;

    /* move head of list if that's where we are */
    if ( ModDesc == DllList )
        DllList = ModDesc->Next;

    MEfree( (PTR)ModDesc );

    return(OK);
}


VOID
iDLcleanup(VOID)
{
    DLLDESC *ModDesc, *Next;
    

    for (ModDesc = DllList; ModDesc; ModDesc = Next)
    {
        if (ModDesc->DllHandle != NULL)
        {
			if (ModDesc->Win32s == FALSE)
			{
	            FreeLibrary(ModDesc->DllHandle);
	        }
        }
        Next = ModDesc->Next;
        MEfree((PTR)ModDesc);
    }
    DllList = NULL;  /* don't free it again if called again by atexit */

    return;
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
*/
STATUS
DLconstructloc( LOCATION *inloc,
                char     *buffer,
                LOCATION *outloc,
                CL_ERR_DESC *errp )
{
    char        namebuf[MAX_LOC+1];
    char        *nameptr;
    LOCATION    nameloc;
    STATUS      ret;
 
    /* copy the filename to a temporary buffer */
    LOtos( inloc, &nameptr );
    STcopy( nameptr, namebuf );
 
    /* add the file extension to the buffer */
    STcat( namebuf, ".DLL" );
 
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
**      A local routine to constuct the names of the shared lib/object
**      and any other files, given the DL module name and directory.
** Inputs:
**  dlmodname   Name of the DL module, 8 chars or less.
**  ext         Extension of the file, e.g. ".so" or ".dsc" or whatever
** Outputs:
**  tmplocp     The output location that's been filled in.
**  tmpbuf      A storage area for locp.
**  errp        Set on any errors.
** Returns:
**  OK          The directory was found and passed simple integrity checks.
**  other       Some error was detected by a lower-level routine. Either
**              the directory specified wasn't found, or it fails some
**              basic integrity checks.
** Side effects:
**  none
*/
static STATUS 
DLconstructname(dlmodname, tmpbuf, tmplocp, ext, errp)
    char *dlmodname;
    char tmpbuf[];
    LOCATION *tmplocp;
    char *ext;
    CL_ERR_DESC *errp;
{
    STATUS ret;

    if ((ret = DLgetdir(tmplocp, tmpbuf, errp)) != OK)
        return(ret);

    return ( DLconstructxloc(dlmodname, tmplocp, ext, errp) );
}

static STATUS DLconstructxloc(dlmodname, tmplocp, ext, errp)
    char *dlmodname;
    LOCATION *tmplocp;
    char *ext;
    CL_ERR_DESC *errp;
{
    LOCATION fileloc;
    char filebuf[MAX_LOC];
    STATUS ret;

    STcopy(dlmodname, filebuf);    
    STcat(filebuf, ext);
    if ((ret = LOfroms(FILENAME, filebuf, &fileloc)) != OK)
    {
        SETCLERR(errp, ret, 0);
        return(ret);
    }
    LOstfile(&fileloc, tmplocp);
    return(OK);
}

/*
** Name:    DLgetdir
**
** Description:
**      A local routine to fill in a location with the user-provided
**      directory "$IIDLDIR", or $II_SYSTEM/ingres/files/dynamic as
**      a default. Checks to make sure that the requested directory
**      exists [and is a directory].
** Inputs:
**  none
** Outputs:
**  locp    The output location that's been filled in.
**  locbuf  A storage area for locp.
**  errp    Set on any errors.
** Returns:
**  OK      The directory was found and passed simple integrity checks.
**  other   Some error was detected by a lower-level routine. Either
**          the directory specified wasn't found, or it fails some
**          basic integrity checks.
** Side effects:
**  none
*/
static STATUS DLgetdir(locp, locbuf, errp)
LOCATION *locp;
char locbuf[];
CL_ERR_DESC *errp;
{
    char *iidldir;
    STATUS ret;
    LOCATION    tmploc; /* For the couple of times we need a temp var */

    NMgtAt("IIDLDIR", &iidldir);
    if (iidldir != NULL && iidldir[0] != '\0')
    {
        STcopy(iidldir, locbuf);
        if ((ret = LOfroms(PATH, locbuf, locp)) != OK)
        {
            SETCLERR(errp, ret, 0);
            return(ret);
        }
    }
    else
    {
        if ((ret = NMloc(FILES, PATH, "dynamic", &tmploc)) != OK)
        {
            SETCLERR(errp, ret, 0);
            return(ret);
        }
        LOcopy(&tmploc, locbuf, locp);
    }
    /*
    **  At this point, "locp" points to the location where DL modules
    **  reside.
    */
    ret = DLisdir(locp);
    if (ret != OK)
    {
        char *locs;
        (void) LOtos(locp, &locs);
        SETCLERR(errp, ret, 0);
        return(ret);
    }
    return(OK);
}
/*
** Name:    DLisdir
**
** Description:
**      A local routine to check that the given location exists and is
**      a directory.
**
** Inputs:
**  locp    Pointer to LOCATION to be checked out.
** Outputs:
**  none
** Returns:
**  OK      The location exists, and it's a directory.
**  DL_DIRNOT_FOUND File/dir not found
**  DL_NOT_DIR  File exists, isn't a directory.
**  other       System-specific error status, or file
** Side effects:
**  None
*/
static STATUS DLisdir(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4  flagword = (LO_I_TYPE);

    if (LOinfo(locp, &flagword, &loinf) != OK)
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
**      A local routine to check that the given location exists.
**      This is the old LOexists, really.
**
** Inputs:
**  locp        Pointer to LOCATION to be checked out.
** Outputs:
**  none
** Returns:
**  TRUE        The location exists.
**  FALSE       The location doesn't exist.
** Side effects:
**  None
*/
static bool fileexists(locp)
LOCATION *locp;
{
    LOINFORMATION loinf;
    i4  flagword = 0;

    return(LOinfo(locp, &flagword, &loinf) == OK);
}

/*
** expandenv - expand environment variables in a string
**
** Description:
**	expands environment variables in a string
**
** Inputs:
**	input	string that may contain environment variables
**
** Outputs:
**	output	string with environment variables replaced
**	        this must point to a buffer of a large enough size.
**
** Returns:
**	none
**
** History:
**	18-jun-1997 (canor01)
**	    Created.
**
*/
static void
expandenv( char *input, char *output )
{
    char *firstpercent, *lastpercent;
    char *envptr;
    char *cp;

    cp = input;
    *output = EOS;
    firstpercent = input;

    
 while( cp )
 {
    firstpercent = STindex( cp, "%", 0 );
    if ( firstpercent != NULL )
    {
	lastpercent = STindex( firstpercent+1, "%", 0 );
        if ( lastpercent != NULL )
	{
	    *lastpercent = EOS;
	    envptr = getenv( firstpercent+1 );
	    if ( envptr != NULL )
	    {
		*firstpercent = EOS;
		STcat( output, cp );
		STcat( output, envptr );
		*firstpercent = '%';
	    }
	    else
	    {
		STcat( output, cp );
		STcat( output, "%" );
	    }
	    *lastpercent = '%';
	    cp = lastpercent+1;
	}
	else
	{
	    STcat( output, cp );
	    cp = NULL;
	}
    }
    else
    {
	STcat( output, cp );
	cp = NULL;
    }
 }
		
}
