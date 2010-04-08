#include <compat.h>
#include <si.h>
#include <lo.h>
#include <nm.h>

# ifdef su4_u42
#include <stdio.h>
#include <a.out.h>
#include  <link.h>
# endif

# ifdef any_hpux
#include <stdio.h>
#include <a.out.h>
#include <filehdr.h>
# endif

# if defined(sparc_sol) || defined(dr6_us5)
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
*/

# ifdef su4_u42
/*{
**  Name: DIAGObjectRead - Read details from an object file
**
**  Description:
**      This routine locates the symbol table and list of loadable areas from
**      an object file and remembers them for future use. (most of the work
**      is done by the symbol and map routines this routine simply provides
**      the required driving code). Uses the a.out format.
**
**      The list of loadable areas is in the program header, a pointer to this
**      is obtained via the ELF library.
**
**  Inputs:
**      LOC *loc    	- Object file location
**      i4  offset      - Address it is loaded to (for shared libraries)
**
**  Side effects:
**      Keeps a file descriptor to the object file to allow areas of store
**      to be read in as required
**
**  Exceptions:
**      Calls DIAGerror() function with errors
**
**  History:
**	01-Apr-1996 (prida01)
**	    Fix to remove macro for addresses
}*/
i4
DIAGObjectRead(loc,offset)
LOCATION *loc;
i4 	 offset;
{

	struct exec 			object;
	FILE				*fp;
	i4				count;
	char				path[255];
	char				fullname[255];
	struct link_dynamic 		*dyn_link;


	/* Open the object file random access */
	if (SIfopen(loc,"r",SI_RACC,sizeof(struct exec),&fp) != OK)
	{
		DIAGerror("DIAGObjectRead: Failed to open file \"%s\"\n",
			loc->fname);
	}
	/* Read the object file header got to start somewhere */
	if (SIread(fp,sizeof(struct exec),&count,&object) != OK)
	{
		DIAGerror("DIAGObjectRead: Failed to read object header\n"); 
	}

	/* read the symbols from the object file */
	DIAGSymbolRead(fp,&object,0);

	/* DO dynamic stuff */
	if (!object.a_dynamic)
	{
		return;
	}

	/* We look for a symbol called */
	dyn_link = (struct link_dynamic *)DIAGSymbolLookupName("_DYNAMIC");

	if ((i4)dyn_link == -1)
	{
		dyn_link = (struct link_dynamic *)N_DATADDR(object);
		if ((i4)dyn_link == -1)
			return;
	}
	
	/* This is for older versions of binaries can't test haven't got them */
	/* It's a just in case branch */
	if (dyn_link->ld_version < 2)
	{
		struct link_dynamic_1 	*link_dyn;
		struct link_map 	*map;
		link_dyn = dyn_link->ld_un.ld_1;

		map = link_dyn->ld_loaded;
		while (map)
		{
			if (LOfroms(PATH&FILENAME,(char *)map->lm_name,
					&loc) != OK)
			{
			 	DIAGerror("DIAGObjectRead: Failed to locate \"%s\"\n"
					 ,(char *)map->lm_name);
			 }
			 if (SIfopen(&loc,"r",SI_RACC,sizeof(struct exec),&fp) 
									!= OK)
		 	 {
			 	DIAGerror("DIAGObjectRead: Failed to open file \"%s\"\n"
					 ,loc->fname);
			 }
			 if (SIread(fp,sizeof(struct exec),&count,&object) 
									!= OK)
			 {
				 DIAGerror("DIAGObjectRead: Failed to read dynamic lib\n");
														 }
		  	DIAGSymbolRead(fp,&object,map->lm_addr);
			map = map->lm_next;
		}
	}
	else
	{
		/* This is the branch of code we will use */
		struct link_dynamic_2   *link_dyn;
		struct link_map 	*map;
		link_dyn = dyn_link->ld_un.ld_2;

		map = link_dyn->ld_loaded;
		/* For each loaded library read in it's symbols and relocate them */
		/* offset the address that library is loaded at */
		while (map)
		{
			if (LOfroms(PATH&FILENAME,(char *)map->lm_name,&loc) != OK)
			{
				DIAGerror("DIAGObjectRead: Failed to locate \"%s\"\n"
				,(char *)map->lm_name);
			}
			if (SIfopen(&loc,"r",SI_RACC,sizeof(struct exec),&fp) != OK)
			{
				DIAGerror("DIAGObjectRead: Failed to open file \"%s\"\n",loc->fname);
			}
			if (SIread(fp,sizeof(struct exec),&count,&object) != OK)
			{
				DIAGerror("DIAGObjectRead: Failed to read dynamic lib\n"); 
			}

			DIAGSymbolRead(fp,&object,map->lm_addr);
			map = map->lm_next;
		}
	}

}
#define got_DIAGObjectRead
# endif

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
DIAGObjectRead( loc, offset )
LOCATION *loc;
i4 	 offset;
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

# if defined(dr6_us5)

/*{
**  Name: DIAGObjectRead - Read details from an object file
**
**  Description:
**      This routine locates the symbol table and list of loadable areas from
**      an object file and remembers them for future use. (most of the work
**      is done by the symbol and map routines this routine simply provides
**      the required driving code)
**
**      The list of loadable areas is in the program header, a pointer to this
**      is obtained via the ELF library.
**
**  Inputs:
**      LOCATION *loc    - Object filename
**      i4  offset        - Address it is loaded to (for shared libraries)
**
**  Side effects:
**      Keeps a file descriptor to the object file to allow areas of store
**      to be read in as required
**
**  Exceptions:
**      Calls DIAGerror() function with errors
**
**  History:
**	14-Mch-1996 (prida01)
**	    Accepts location as a parameter instead of just filename.
}*/
i4
DIAGObjectRead(loc,offset)
LOCATION *loc;
i4   offset;
{
    i4  fdes;
    Elf *elf;
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
    i4  c;
    PTR		filename;


    filename = loc->path;
    /* Check we are compatible with the ELF library */
    if(elf_version(EV_CURRENT)==EV_NONE)
        DIAGerror("DIAGObjectRead: Elf library out of date\n");

    /* Open the file and get an ELF descriptor */
    if((fdes=open(filename,O_RDONLY))== -1)
    {
    	DIAGerror("DIAGObjectRead: Fail to open file \"%s\"\n",filename);
	TRdisplay("DIAGObjectRead: Fail to open file \"%s\"\n",filename);
    }

    if((elf=elf_begin(fdes,ELF_C_READ,(Elf *)NULL))==NULL)
    {
        DIAGerror("DIAGObjectRead: elf_begin failed\n");
        TRdisplay("DIAGObjectRead: elf_begin failed\n");
    }

    /* Read in the symbols */

    DIAGSymbolRead(elf,offset);

    elf_end(elf);
}
#define got_DIAGObjectRead
# endif

#if !defined(got_DIAGObjectRead)
i4
DIAGObjectRead(loc,offset)
PTR loc;
i4  offset;
{
}

# endif 
