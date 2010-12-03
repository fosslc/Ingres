#include <compat.h>
#include <clconfig.h>
#include <si.h>
#include <lo.h>
#include <nm.h>
#include <ex.h>
#include <exinternal.h>

# ifdef any_hpux
#include <stdio.h>
#include <a.out.h>
#include <filehdr.h>
# endif

# if defined(sparc_sol)
#include <stdio.h>
#include <libelf.h>
#include <fcntl.h>
# endif
/*
**Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: object - Object file analysis
**
**  Description:
**      This module contains the object file specific logic for locating symbol
**      table entries and loadable areas
**
**      The following routine is supplied:-
**
**      DIAGObjectRead    - Read details of an object file
**
**  History:
**	14-Mch-1996 (prida01)
**	    Make object file accept location as parameter instead of
**	    file name for portability.
**	04-dec-1996 (canor01)
**	    Remove unneeded include of map.h.
**	06-Nov-1998 (muhpa01)
**	    Created DIAGObjectRead() in support of stack tracing on HP-UX.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Sep-2007 (bonro01)
**	    Add CS_sol_dump_stack for su4_us5, su9_us5, a64_sol for
**	    both 32bit and 64bit stack tracing.
**	    Remove Solaris routines because symbols are now looked up
**	    differently.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Drop long-obsolete SunOS 4, dr6.
*/


# ifdef any_hpux
/*{
**  Name: DIAGObjectRead - Read details from an object file
**
**  Description:
**      This routine locates the symbol table and list of loadable areas from
**      an object file and remembers them for future use. (most of the work
**      is done by the symbol and map routines this routine simply provides
**      the required driving code). Uses the a.out format.
**
**  Inputs:
**      LOC *loc    	- Object file location
**      i4  offset      - ignored on HP
**
**  Side effects:
**      Keeps a file descriptor to the object file to allow areas of store
**      to be read in as required
**
**  Exceptions:
**      Calls DIAGerror() function with errors
**
**  History:
**	06-Nov-1998 (muhpa01)
**		Created.
}*/
i4
DIAGObjectRead( LOCATION *loc, i4 offset )
{
	FILHDR	header;
	FILE	*fp;
	i4	count;

	/* Open the object file random access */
	if ( SIfopen( loc, "r", SI_RACC, FILHSZ, &fp ) != OK )
		DIAGerror( "DIAGObjectRead: Failed to open file \"%s\"\n",
			  loc->fname );

	/* Read the object file header */
	if ( SIread( fp, FILHSZ, &count, (PTR)&header ) != OK )
		DIAGerror( "DIAGObjectRead: Failed to read object header\n" ); 

	/* read the symbols from the object file */
	DIAGSymbolRead( fp, &header, 0 );
	SIclose( fp );
}
#define got_DIAGObjectRead
# endif /* hpux */


#if !defined(got_DIAGObjectRead)
i4
DIAGObjectRead(LOCATION *loc,i4 offset)
{
    return (0);
}

# endif 
