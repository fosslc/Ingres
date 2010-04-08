
/*
**	Copyright (c) 2004 Ingres Corporation
*/

#ifdef VMS      
#include    <compat.h>
#include    <cv.h>
#include    <er.h>
#include    <si.h>
#include    <me.h>
#include    <pc.h>

#include    <ssdef.h>

/**
**
**  Name: IILGKMEM.C - Attempt to allocate shared memory required by
**	the LG/LK system.
**
**  Description:
**  	Try and allocate shared memory required by the LG/LK system.
**	See "main"
**
**	USAGE for main program:
**		iilgkmem byte_count
**
**          main() 			- the main program.
**
**  History:
**      18-jan-94 (joplin)
**          created.
**	02-feb-94 (joplin)
**	    IFDEF-ed for VMS since this currently only works for VMS 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
** Allows NS&E to build one executable for IIDBMS, DMFACP, DMFRCP, ...
*/
# ifdef II_DMF_MERGE
# define    MAIN(argc, argv)    iilkgmem_libmain(argc, argv)
# else
# define    MAIN(argc, argv)    main(argc, argv)
# endif

/*
**      mkming hints
MODE =          SETUID PARTIAL_LD

NEEDLIBS =      SCFLIB DMFLIB ADFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB

OWNER =         INGUSER

PROGRAM =       iilgkmem
*/

FUNC_EXTERN VOID LG_size();    		/* return size of LG object */
FUNC_EXTERN VOID LK_size();    		/* return size of LK object */
static STATUS	_iilgkmem(i4 argc,char *argv[]);


/*{
** Name: main()	- main routine of iilgkmem
**
** Description:
**	Main routine of iilgkmem
**
**	This program is called by the installation scripts to determine
**	if the shared memory required by the LG/LK can be allocated.
**	This is necessary because on VMS, there is no way to tell if
**	GBLPAGFIL has enough free backing pages for a global memory
**	request until it is actually attempted.  If the request fails
**	then the error_code returned can be examined.
**
**	USAGE for main program:
**		iilgkmem byte_count
**
** Inputs:
**      argc                         	number of arguments.
**      argv                            number of bytes of shared memory
**
** Outputs:
**	none.
**
**	Returns:
**	    exits to the OS with either OK or VMS failure status.
**
** History:
**      18-jan-94 (joplin)
**          created.
*/

VOID
# ifdef	II_DMF_MERGE
MAIN(argc, argv)
# else
main(argc, argv)
# endif
i4                  argc;
char		    *argv[];
{
    MEadvise(ME_INGRES_ALLOC);
    PCexit( _iilgkmem(argc, argv) );
}

static STATUS
_iilgkmem(
i4                  argc,
char		    *argv[])
{
    i4 flag;
    i4      pages;
    i4      bytes;
    char    *key = ERx("IILGKMEM.TMP");
    PTR     memory;
    i4      alloc_pages;
    STATUS  stat;
    CL_SYS_ERR err_code;
    i4 os_error;

    if (argc != 2)
    {
	SIprintf("USAGE: iilgkmem byte_count\n");
	return(FAIL);
    }
    else
    {
	if ( CVan(argv[1], &bytes) != OK || bytes <= 0)
	{
	    SIprintf("USAGE: iilgkmem byte_count\n");
	    return(FAIL);
	}
    }

    /* convert the number of bytes to a number of pages */
    pages = (bytes + ME_MPAGESIZE - 1) / ME_MPAGESIZE;

    /* Flags used the by the LG/LK system to allocate shared memory */
    flag = ME_MSHARED_MASK | ME_IO_MASK | 
	ME_CREATE_MASK | ME_NOTPERM_MASK | ME_MZERO_MASK;
	
    /* Attempt the memory allocation and save the results */
    stat = MEget_pages(flag, pages, key, &memory, &alloc_pages, &err_code);
    os_error = err_code.error;
    
    /* deallocate the shared memory */
    MEsmdestroy(key, &err_code); 

    /*
    ** Now check the result of the memory allocation requiest, 
    ** the configuration and startup scripts on VMS use the output 
    ** of these checks, so make sure these get changed if any changes
    ** are made here.
    */
    if (stat == OK)
	SIprintf( ERx("SUCCESS allocating %d bytes of global memory\n"),bytes);
    else if (os_error == SS$_EXGBLPAGFIL)
	SIprintf( ERx("GBLPAGFIL value in SYSGEN is insufficient\n") );
    else if (os_error == SS$_GPTFULL)
	SIprintf( ERx("GBLPAGES value in SYSGEN is insufficient\n") );
    else if (os_error == SS$_GSDFULL)
	SIprintf( ERx("GBLSECTIONS value in SYSGEN is insufficient\n") );
    else
	SIprintf( ERx("%d\n"), os_error );

    return(OK);
}
#endif
