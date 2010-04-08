#include <bzarch.h>
# if defined(dr6_us5)
#include <stdio.h>
#include <libelf.h>
#include <string.h>
#include <stdlib.h>
# endif

#include <compat.h>
#include <si.h>
#include <lo.h>
#include <me.h>
#include <st.h>

# ifdef any_hpux
#include <a.out.h>
#include <aouthdr.h>
#include <syms.h>
# endif

# ifdef su4_u42
#include <a.out.h>
#include <stab.h>
# endif

# if defined(sparc_sol) || defined(a64_sol)
#include <dlfcn.h>
#include <sys/machelf.h>
# endif

/*{
**Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: symbol - Symbol table handling routines
**
**  Description:
**     This module manipulates an instore symbol table. The following routines
**     are supplied:-
**
**     DIAGSymbolRead       - Read symbols from an object file
**     DIAGSymbolLookupAddr - Lookup the name corresponding to an address
**     DIAGSymbolLookupName - Lookup address corresponding to name
**
**  History:
**	01-Apr-1996 (prida01)
**	    Add sunOS symbols to iidbms
**	06-Nov-1998 (muhpa01)
**	    Added DIAGSymbol* routines for reading & lookup of object
**	    symbols on HP-UX.
**      20-apr-1999 (hanch04)
**          Don't use for 64 bit OS
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      20-apr-1999 (hanch04)
**          Don't use for 64 bit OS
**	02-jul-2001 (somsa01)
**	    Added include of me.h .
**      20-Nov-2006 (hanal04) Bug 117155
**          Empty DIAGSymbolLookupAddr() causes SIGSEGV because it failed
**          to return NULL to the caller.
**	17-Sep-2007 (bonro01)
**	    Add CS_sol_dump_stack for su4_us5, su9_us5, a64_sol
**	    for both 32bit and 64bit stack tracking.
**	    Remove routines for Solaris.  Symbols are now looked up 
**	    differently.
**      27-Aug-2008 (hanal04) Bug 120823
**          Add stack dump support for i64_hpu.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
}*/

# if defined(any_hpux) && !defined(LP64) && !defined(i64_hpu)
/*
**  Name: SYMBOL table entry
**
**  Description:
**      This structure defines a symbol table entry for HP.
*/

typedef struct IISYMBOL
{
    PTR		name;
    u_i4 	value;
} SYMBOL;

static PTR	string_table = NULL;
static SYMBOL	*symbol_array = NULL;
static i4	symbol_count = 0;

#endif

#if defined(i64_hpu)
/*
**  Name: SYMBOL table entry
**
**  Description:
**      This structure defines a symbol table entry for HP Itanium.
*/

typedef struct IISYMBOL
{
    PTR         name;
    uint64_t    value;
} SYMBOL;

static PTR      string_table = NULL;
static SYMBOL   *symbol_array = NULL;
static i4       symbol_count = 0;

#endif /* i64_hpu */

# if defined(su4_u42) || defined(dr6_us5)
/*{
**  Name: SYMBOL table entry
**
**  Description:
**      This structure defines a symbol table entry. Currently these are simply
**      chained together as a linked list in no particular order but this could
**      be changed if speed becomes a problem
}*/

typedef struct IISYMBOL
{
    struct IISYMBOL *next;
    PTR		name;
    u_i4 	name_offset;
    u_i4 	value;
    uchar 	type;
    i4  size;
} SYMBOL;
#define		MAX_SYMBOL_LIST	100
static PTR	string_table=NULL;
static SYMBOL  	*symbol_list[MAX_SYMBOL_LIST];
static i4     	symbol_list_count=0;
static SYMBOL 	*new_list=NULL;
#endif

# if defined(sparc_sol) || defined(a64_sol)
/*
**  Name: DIAGSymbolLookup - Lookup an address return the name and offset 
**
**  Description:
**      This routine looks up a name withion the symbol table and returns the
**      name of the area it is in along with the offset within that area
**
**  Inputs:
**      long  pc              - Address to lookup
**      PTR   buffer          - Address of buffer to copy symbol
**      i4    size            - Size of buffer
**
**  Outputs:
**      long  *addr           - Where to return address (or NULL)
**      long  *offset         - Where to return offset (or NULL)
**
**  Returns:
**      i4  length            - Length of symbol returned
**
**  History:
**	17-Sep-2007 (bonro01)
**	    Created.
*/

i4
DIAGSymbolLookup( pc, addr, offset, buffer, size )
void	*pc;
unsigned long  *addr;
unsigned long  *offset;
PTR   buffer;
i4    size;
{
	Dl_info	info;
	Sym	*sym;

	*offset = 0;
	*addr = 0;
	if (dladdr1(pc, &info, (void **)&sym, RTLD_DL_SYMENT) == 0)
		return(snprintf(buffer,size, "[0x%p]", pc));

	*offset = (unsigned long)pc - (unsigned long)info.dli_saddr;
	*addr = (unsigned long)info.dli_saddr;
	if (info.dli_sname != NULL) 
	{
		return(snprintf(buffer,size, "%s", info.dli_sname));
	} else {
		return(snprintf(buffer,size, "[0x%p]", pc));
	}
}
# endif

# if defined(any_hpux) && !defined(LP64) && !defined(i64_hpu)
/*
**  Name: DIAGSymbolLookupName - Lookup a name return its address
**
**  Description:
**      This routine looks a name up in the symbol table and returns the 
**      corresponding address (or -1 if not found)
**
**  Inputs:
**      PTR name           - Name to lookup
**
**  Returns:
**      i4  address        - Address (or -1 if not found)
**
**  History:
**	06-Nov-1998 (muhpa01)
**	    Created.
*/

i4
DIAGSymbolLookupName( name )
PTR name;
{
	SYMBOL	*sym = symbol_array;

	while ( sym->name != NULL )
	{
		if ( STcompare( name, sym->name ) == 0 )
			return ( sym->value );
		sym++;
	}
	return ( -1 );
}

/*
**  Name: DIAGSymbolLookupAddr - Lookup an address return the name and offset 
**
**  Description:
**      This routine looks up a name withion the symbol table and returns the
**      name of the area it is in along with the offset within that area
**
**  Inputs:
**      i4  address         - Address to lookup
**
**  Outputs:
**      i4  *offset         - Where to return offset (or NULL)
**
**  Returns:
**      PTR name          - Name of area (or NULL if not found)
**
**  History:
**	06-Nov-1998 (muhpa01)
**	    Created.
*/

PTR
DIAGSymbolLookupAddr( address, offset )
i4  address;
i4  *offset;
{
	SYMBOL	*sym = symbol_array;

	if ( address < sym->value )
		return ( NULL );

	while ( sym->value != NULL )
	{
		if ( address < sym[1].value )
		{
			if ( offset )
				*offset = address - sym->value;
			return ( sym->name );
		}
		else
			sym++;
	}
	return ( NULL );
}
#define got_DIAGSymbolLookupName
#endif

# if defined(i64_hpu)
/*
**  Name: DIAGSymbolLookupName - Lookup a name return its address
**
**  Description:
**      This routine looks a name up in the symbol table and returns the 
**      corresponding address (or -1 if not found)
**
**  Inputs:
**      PTR name           - Name to lookup
**
**  Returns:
**      uint64_t  address        - Address (or -1 if not found)
**
**  History:
**	06-Nov-1998 (muhpa01)
**	    Created.
*/

uint64_t
DIAGSymbolLookupName( name )
PTR name;
{
	SYMBOL	*sym = symbol_array;

	while ( sym->name != NULL )
	{
		if ( STcompare( name, sym->name ) == 0 )
			return ( sym->value );
		sym++;
	}
	return ( -1 );
}

/*
**  Name: DIAGSymbolLookupAddr - Lookup an address return the name and offset 
**
**  Description:
**      This routine looks up a name withion the symbol table and returns the
**      name of the area it is in along with the offset within that area
**
**  Inputs:
**      uint64_t  address         - Address to lookup
**
**  Outputs:
**      uint64_t  *offset         - Where to return offset (or NULL)
**
**  Returns:
**      PTR name          - Name of area (or NULL if not found)
**
**  History:
**	06-Nov-1998 (muhpa01)
**	    Created.
**	17-Sep-2008 (wanfr01)
**          Bug 120915 - return NULL if symbol array does not exist
*/

PTR
DIAGSymbolLookupAddr( address, offset )
uint64_t  address;
uint64_t  *offset;
{
	SYMBOL	*sym = symbol_array;

	if (symbol_array == NULL)
	    return ( NULL );

	if ( address < sym->value )
		return ( NULL );

	while ( sym->value != NULL )
	{
		if ( address < sym[1].value )
		{
			if ( offset )
				*offset = address - sym->value;
			return ( sym->name );
		}
		else
			sym++;
	}
	return ( NULL );
}
#define got_DIAGSymbolLookupName
#endif

# if defined(su4_u42) || defined(dr6_us5)
/*{
**  Name: DIAGSymbolLookupName - Lookup a name return its address
**
**  Description:
**      This routine looks a name up in the symbol table and returns the 
**      corresponding address (or -1 if not found)
**
**  Inputs:
**      PTR name           - Name to lookup
**
**  Returns:
**      i4  address        - Address (or -1 if not found)
**
**  History:
}*/

i4
DIAGSymbolLookupName(name)
PTR name;
{
    SYMBOL 	*pointer;
    i4  	address= -1;
    i4  	count = 0;

    /* and to or for debugging */
    for (count=0;count<symbol_list_count;count++)
    {
	pointer = symbol_list[count];
    	while(pointer!=NULL)
    	{
        	if(!STcompare(name,pointer->name))
            		return(pointer->value);
        	pointer=pointer->next;
    	}
    }

    return(address);
}

/*{
**  Name: DIAGSymbolLookupAddr - Lookup an address return the name and offset 
**
**  Description:
**      This routine looks up a name withion the symbol table and returns the
**      name of the area it is in along with the offset within that area
**
**  Inputs:
**      i4  address         - Address to lookup
**
**  Outputs:
**      i4  *offset         - Where to return offset (or NULL)
**
**  Returns:
**      PTR name          - Name of area (or NULL if not found)
**
**  History:
*/

PTR
DIAGSymbolLookupAddr(address,offset)
i4  address;
i4  *offset;
{
    i4  hack = 4;

    PTR 	name=NULL;
    SYMBOL 	*pointer;
    i4  	my_offset=999999;
    i4  	count;
    for (count = 0; count < symbol_list_count; count++)
    {
	pointer = symbol_list[count];
    	while(my_offset > 0 && pointer!=NULL)
    	{
        	if((address >= pointer->value && 
              	(address < pointer->value + pointer->size)) ||
           	((address == pointer->value) && (pointer->size==0)) ||
           	((address - hack == pointer->value) && (pointer->size==0)) ||
           	((address + hack == pointer->value) && (pointer->size==0)))
        	{
            		if((i4)my_offset > abs(address-pointer->value))
            		{
             			name=pointer->name;
               			my_offset=(address-pointer->value);
            		}
        	}

        	pointer=pointer->next;
    	}

    }
    if(offset!=NULL)
    	*offset=my_offset;
    return(name);
}
    
/*{
**  Name: DIAGSymbolDump - Dump symbol table
**
**  Description:
**      This routine dumps the current contents of the symbol table to the
**      screen
**
**  History:
*/

VOID
DIAGSymbolDump()
{
    SYMBOL *pointer;
    i4     count;

    for (count=0;count<symbol_list_count;count++)
    {
	pointer = symbol_list[count];
    	while(pointer!=NULL)
    	{
        	SIprintf("%s 0x%x %d\n",
			pointer->name,pointer->value,pointer->size);
        	pointer=pointer->next;
    	}
    }
}
#define got_DIAGSymbolLookupName
# endif

#if !defined(got_DIAGSymbolLookupName)
i4
DIAGSymbolLookupName(name)
char *name;
{
}

PTR
DIAGSymbolLookupAddr(address,offset)
i4  address;
i4  *offset;
{
    return (NULL);
}
VOID
DIAGSymbolDump()
{
}
#endif


# ifdef su4_u42
i4  last_text_symbol;
i4  last_data_symbol;
i4  last_bss_symbol;
/*{
**  Name: DIAGSymbolRead - Read symbols
**
**  Description:
**      This routine reads the symbol table entries from an object file and
**      adds them to the in memory symbol table. The object file whose symbols
**      are being read may be a shared object library that is not loaded at
**      the location defined in its symbols - in this case a virtual address
**      offset is added to each symbol value
**
**	When the symbols are read they are sorted into numerical virtual address
**	order. This is used as the size of a symbol. Size of functions etc.
**  Inputs:
**	FILE *fp      -	Object file to read symbols from
**	struct exec *header - obect header;
**      i4  offset    - Address at which symbols will be mapped. Set to 0 unless
**			it is a dynamic libraries.
**
**  Exceptions:
**      Calls DIAGerror() function on error
**
**  History:
**	01-Apr-1996 (prida01)
**	    Improve existing symbol read for sunOS performance. Exclude
**	    Any symbols that arn't needed
*/
DIAGSymbolRead(fp,header,offset)
FILE 	*fp;
struct 	exec *header;
i4  	offset;
{

	struct nlist 	symbols,*sym_ptr,*sym_lowest,*copy_area;
	i4		no_symbols;
	i4		total_symbols;
	i4		sym_start = 0;
	i4		count;
	i4		error;
	i4		sym_offset = N_SYMOFF(*header);
	i4		str_offset = N_STROFF(*header);
	i4		str_table_size;
	PTR		symbol_area;

	/* Move file pointer to the symbol offset */
	if ((error = SIfseek(fp,sym_offset,SI_P_START)) != OK)
	{
		DIAGerror("DIAGSymbolRead:SIfseek failed error %d \n",error);
	}

	/* Set up last possible virtual addresses for symbols */
	last_text_symbol=N_TXTADDR(*header)+ header->a_text+offset;
	last_data_symbol=N_DATADDR(*header)+ header->a_data;
	last_bss_symbol =N_BSSADDR(*header)+ header->a_bss;
	total_symbols = header->a_syms/sizeof(struct nlist);
	symbol_area = (char *)malloc( header->a_syms);
	if ((error =SIread(fp,header->a_syms,&count,symbol_area)) != OK )
	{
		DIAGerror("DIAGSymbolRead: Symbol table not OK %d\n",error);
	}

	/* Read in the size of the string table */
	if ((error = SIfseek(fp,str_offset,SI_P_START)) != OK)
	{
		DIAGerror("DIAGSymbolread: failed error %d \n",error);
	}
	/* Read in the actual string table */
	if ((error = SIread(fp,4,&count,&str_table_size)) != OK)
	{
		DIAGerror("DIAGReadSymbol: Can't read string table\n");
	}
	string_table = (char *)malloc(str_table_size);
	if ((error = SIread(fp,str_table_size,&count,string_table)) != OK)
	{
		DIAGerror("DIAGReadSymbol:String table error %d\n",error);
	}

	/* Get rid of all sdb symbols etc. Makes sorting so much quicker 
	** when we have stuff compiled up debug. Minutes faster
	*/
	sym_lowest = sym_ptr  = (struct nlist *)(symbol_area);
	for (no_symbols =0; no_symbols < total_symbols; no_symbols++)
	{


		/* We don't need these. We just want text symbols */
		if (sym_ptr->n_type >(N_TEXT|N_EXT))
		{
			sym_start++;
			sym_ptr->n_un.n_strx = sym_lowest->n_un.n_strx;
			sym_ptr->n_other = sym_lowest->n_other;
			sym_ptr->n_desc = sym_lowest->n_desc;
			sym_ptr->n_value = sym_lowest->n_value;
			sym_lowest++;

		}
		sym_ptr++;
	}
	/* Sort symbols into numerical order for sizing later */
	for (no_symbols = sym_start; no_symbols < total_symbols; no_symbols++)
	{
		i4 c;
		copy_area = sym_lowest = sym_ptr = (struct nlist *)(symbol_area
		+ (no_symbols  * sizeof(struct nlist)));

	
		for (c=no_symbols;c<total_symbols;c++)
		{
			if ((u_i4)sym_ptr->n_value<(u_i4)sym_lowest->n_value)
			{
			    sym_lowest = sym_ptr;
			}
			sym_ptr++;
		}
		new_symbol(sym_lowest,offset,string_table,str_table_size);
		sym_lowest->n_un.n_strx = copy_area->n_un.n_strx;
		sym_lowest->n_other = copy_area->n_other;
		sym_lowest->n_desc = copy_area->n_desc;
		sym_lowest->n_value = copy_area->n_value;
			
	}

	/* Make symbols point to strings */
	fixup_names();
	free(symbol_area);
}

/*{
** Add a new symbol to the symbol list. Adjust string offset pointer
** to take into account a length field at beginning to string table 
}*/


static i4
new_symbol(symbol,offset,string_table,size)
struct nlist 	*symbol;
i4	offset;
char *string_table;
i4  size;
{
	SYMBOL *new=(SYMBOL *)malloc(sizeof(SYMBOL));
	if(new==NULL)
	{
		SIprintf("new symbol: Out of memory\n");
	}
	new->name_offset=symbol->n_un.n_strx - 4;
	if ((i4) new->name_offset <= size) 
	{
		new->name = string_table + new->name_offset + 1;
		/* Get rid of .o */
		if (STchr( new->name,'.'))
		{
			free(new);
			return(0);
		}
	}
	new->next=new_list;
	new_list=new;
	/* take into account the length at beginning of buffer*/
	new->value=symbol->n_value + offset;
	new->type = symbol->n_type;
	new->size = 0;
	return(0);
}


/*{
** make symbols point to the correct part of the string table 
** We seperate text,bss and data, because I need to know the
** end of each section.

}*/
static i4
fixup_names()
{
	SYMBOL *pointer=new_list;
	i4 	error;
	i4	last_test_size; /* In case we get a .o symbol */
	symbol_list[symbol_list_count];
	while (pointer !=NULL)
	{
		SYMBOL *next=pointer->next;

		switch (pointer->type)
		{
			case  N_TEXT:
			case  N_TEXT|N_EXT :
			    pointer->size = last_text_symbol - pointer->value;
			    last_text_symbol = pointer->value;
			    break;
			case N_DATA:
			case N_DATA|N_EXT:
			    pointer->size = last_data_symbol - pointer->value;
			    last_data_symbol = pointer->value;
			    break;
			case N_BSS:
			case N_BSS|N_EXT:
			    pointer->size = last_bss_symbol - pointer->value;
			    last_bss_symbol = pointer->value;
			    break;
			default : 
			    break;
		}
			/* Insert onto symbol_list */
					     	 
	  	pointer->next=symbol_list[symbol_list_count];
		symbol_list[symbol_list_count]=pointer;

		pointer = next;
	}
	symbol_list_count++;
	if (symbol_list_count > 100)
	{
		DIAGerror("fixup names too many dynamic libraries \n");
	}		
}
#define got_DIAGSymbolRead
# endif

# if defined(i64_hpu) || ( defined(any_hpux) && !defined(LP64) )
/*{
**  Name: DIAGSymbolRead - Read symbols
**
**  Description:
**      This routine reads the symbol table entries from an object file and
**      adds them to the in-memory symbol array, which is sorted by address.
**
**  Inputs:
**	FILE *fp       - Object file to read symbols from
**	SYMENT *header - obect header;
**      i4  offset     - ignored on HP
**
**  Exceptions:
**      Calls DIAGerror() function on error
**
**  History:
**	06-Nov-1998 (muhpa01)
**	    Created.
**	17-Sep-2008 (wanfr01)
**	    Bug 120915 - Drop out of this routine if total_symbols = 0.
**	    Note Itanium builds do not build symbol information into the
**	    dbms.
*/
DIAGSymbolRead( fp, header, offset )
FILE 	*fp;
FILHDR  *header;
i4  	offset;
{

	SYMENT		*sym_ptr, *sym_ptr2, *copy_area;
	i4		no_symbols;
	i4		total_symbols = header->symbol_total;
	i4		count;
	i4		error;
	i4		str_table_size = header->symbol_strings_size;
	PTR		symbol_area;
	STATUS		status = OK;

	/* Bail out if there are no symbols */
        if (header->symbol_total == 0) 
            return;

	/* Move file pointer to the symbol offset */
	if ( ( error = SIfseek( fp, header->symbol_location, SI_P_START ) ) != OK )
		DIAGerror( "DIAGSymbolRead:SIfseek failed error %d \n", error );

	symbol_area = (char *)MEreqmem( NULL, SYMESZ * total_symbols,
				       TRUE, &status );
	if ( status != OK )
		DIAGerror( "DIAGSymbolRead: Out of memory for symbol area\n" );

	if ( ( error = SIread( fp, SYMESZ * total_symbols, &count, symbol_area ) )
	    != OK )
		DIAGerror( "DIAGSymbolRead: Symbol table not OK %d\n", error );

	/* Position to the symbol strings table */
	if ( ( error = SIfseek( fp, header->symbol_strings_location,
	    SI_P_START ) ) != OK )
		DIAGerror( "DIAGSymbolread: failed error %d \n", error );

	string_table = (char *)MEreqmem( NULL, str_table_size, TRUE, &status );
	if ( status != OK )
		DIAGerror( "DIAGSymbolRead: Out of memory for string table\n" );

	/* Read symbol strings table */
	if ( ( error = SIread( fp, str_table_size, &count, string_table ) ) != OK )
		DIAGerror( "DIAGReadSymbol:String table error %d\n", error );

	/* Sift out text symbols that we need. */
	sym_ptr2 = sym_ptr = (SYMENT *)symbol_area;
	for ( no_symbols = 0; no_symbols < total_symbols; no_symbols++ )
	{
		if ( ( ( sym_ptr->symbol_type == ST_CODE ) &&
			( sym_ptr->symbol_scope == SS_UNIVERSAL ) ) ||
			( ( sym_ptr->symbol_type == ST_ENTRY ) &&
			( ( sym_ptr->symbol_scope == SS_UNIVERSAL ) ||
			( sym_ptr->symbol_scope == SS_GLOBAL ) ) ) )
		{
			symbol_count++;
			*sym_ptr2 = *sym_ptr;
			sym_ptr2++;
		}
		sym_ptr++;
	}
	
	/* Allocate space for the final symbol array */
	symbol_array = (SYMBOL *)MEreqmem( NULL,
					  sizeof(SYMBOL) * ( symbol_count + 1 ),
					  TRUE, &status );

	if ( status != OK )
		DIAGerror( "DIAGSymbolRead: Out of memory for symbol array\n" );

	/* Sort symbols by address value & add to symbol array */
	for ( no_symbols = 0; no_symbols < symbol_count; no_symbols++ )
	{
                i4  c;
		copy_area = sym_ptr2 = sym_ptr = (SYMENT *)( symbol_area +
				     ( no_symbols * SYMESZ ) );

		for ( c = no_symbols; c < symbol_count; c++ )
		{
			if ( sym_ptr->symbol_value < sym_ptr2->symbol_value )
			    sym_ptr2 = sym_ptr;
		    	sym_ptr++;
		}
		symbol_array[no_symbols].name = string_table + sym_ptr2->name.n_strx;
		symbol_array[no_symbols].value = sym_ptr2->symbol_value & ~3;
		*sym_ptr2 = *copy_area;
	}
	MEfree( symbol_area );
}
#define got_DIAGSymbolRead
# endif


#if defined(dr6_us5)
/*{
**  Name: DIAGSymbolRead - Read symbols
**
**  Description:
**      This routine reads the symbol table entries from an object file and
**      adds them to the in memory symbol table. The object file whose symbols
**      are being read may be a shared object library that is not loaded at
**      the location defined in its symbols - in this case a virtual address
**      offset is added to each symbol value
**
**      The symbol table is actually built in two stages:-
**      1) The symbol table section is read and a list of SYMBOL structures
**         built chained off new_list
**      2) The string table is read and the name of each SYMBOL in new_list
**         fixed up and the fixed up SYMBOL inserted onto symbol_list
**
**  Inputs:
**      Elf *elf      - Elf descriptor of object file to read
**      i4  offset    - Virtual address offset to use (or 0)
**
**  Exceptions:
**      Calls DIAGerror() function on error
**
**  History:
*/

i4
DIAGSymbolRead(elf,offset)
Elf *elf;
i4  offset;
{
    Elf32_Ehdr *ehdr;
    Elf_Scn *section=NULL;
    Elf_Scn *string_section=NULL;
    Elf_Scn *symbol_section=NULL;
    Elf_Data *data;

    /* Get the elf header so we can find the section names */

    if((ehdr=elf32_getehdr(elf))==NULL)
        DIAGerror("DIAGSymbolRead: Failure to get ehdr\n");

    /* Find string table and symbol table */

    while((section=elf_nextscn(elf,section))!=NULL)
    {
        Elf32_Shdr *shdr;

        if((shdr=elf32_getshdr(section))==NULL)
            DIAGerror("DIAGSymbolRead: Cannot get section header\n");

        if(shdr->sh_type==SHT_STRTAB || shdr->sh_type==SHT_SYMTAB)
        {
            char *name;

            /* Locate the name of the section and check for one we want */

            if((name=elf_strptr(elf,ehdr->e_shstrndx,shdr->sh_name))==NULL)
                DIAGerror("DIAGSymbolRead: Cannot get section name\n");

            if(!STcompare(name,".strtab"))
                string_section=section;
            else if(!STcompare(name,".symtab"))
                symbol_section=section;
        }
    }

    /* There must be one of each */

    if(string_section==NULL || symbol_section==NULL)
        DIAGerror("DIAGSymbolRead: Cannot find string table or symbol table\n");

    /* Plod through the symbol table section remembering what we see */

    data=NULL;
    while((data=elf_getdata(symbol_section,data))!=NULL)
    {
        Elf32_Sym *symbol=(Elf32_Sym *)data->d_buf;

        while(symbol < (Elf32_Sym *)((char *)data->d_buf + data->d_size))
        {
            new_symbol(symbol,offset);
            symbol++;
        }

    }

    /* Now read in the string table and fixup all the names in the new */
    /* symbols we have created                                         */

    data=NULL;
    while((data=elf_getdata(string_section,data))!=NULL)
    {
        fixup_names(data);
    }

    /* The new_list should now be empty - just make sure (you never know) */

    if(new_list!=NULL)
        DIAGerror("DIAGSymbolRead: new_list not empty\n");
}

/*{
**  Name: new_symbol - Internal routine to create a new symbol
**
**  Description:
**      This routine creates a new SYMBOL and adds it to the new_list. The
**      fixup_names routine will later move it from here to the symbol_list
**      once its name has been fixed up (names of symbols are stored as offsets
**      into the string table in an Elf file).
**
**  Inputs:
**      Elf32_Sym *symbol     - Details of symbol to create
**      i4  offset            - Virtual address offset to use
**
**  Returns:
**      i4                    - Error status
**
**  Exceptions:
**      Calls DIAGerror() function on error
**
**  History:
*/

static i4
new_symbol(symbol,offset)
Elf32_Sym 	*symbol;
i4  		offset;
{
    /* Ignore non functions and objects - the rest like -g style line */
    /* numbers may be useful one day but for now ....                 */

    if(ELF32_ST_TYPE(symbol->st_info)==STT_FUNC || 
       ELF32_ST_TYPE(symbol->st_info)==STT_OBJECT)
    {
        SYMBOL *new=(SYMBOL *)malloc(sizeof(SYMBOL));

        if(new==NULL)
            DIAGerror("new_symbol: Out of memory\n");

        new->next=new_list;
        new_list=new;
        new->name_offset=symbol->st_name;
        new->value=symbol->st_value + offset;
        new->size=symbol->st_size;
    }
}

/*{
**  Name: fixup_names - Internal routine to fixup names of symbols
**
**  Description:
**      In Elf files symbol names are stored as offsets into a string table
**      this routine fixes up the new_list symbols to include their correct
**      names and moves them onto symbol_list accordingly.
**
**  Inputs:
**      Elf_Data *data       - Part of the string table
**
**  Returns:
**     i4                    - Error status
**
**  History:
*/

static i4
fixup_names(data)
Elf_Data *data;
{
    SYMBOL *pointer=new_list;
    SYMBOL *last=NULL;
    

    symbol_list_count =1;
    while(pointer!=NULL)
    {
        SYMBOL *next=pointer->next;
        /* i4  name_offset=(i4)pointer->name; */
        if((i4) pointer->name_offset >= data->d_off &&
           (i4) pointer->name_offset < data->d_off + data->d_size)
        {
            /* Fixup name */

            pointer->name=
	    (PTR)STalloc((char *)data->d_buf+pointer->name_offset-data->d_off);

            /* Remove from new_list */
            if(last==NULL)
                new_list=next;
            else
                last->next=next;

            /* Insert onto symbol_list */

            pointer->next=symbol_list[0];
            symbol_list[0]=pointer;
        }
        else
        {
            last=pointer;
        }
            
        pointer=next;

    }

}
#define got_DIAGSymbolRead
# endif 

#if !defined(got_DIAGSymbolRead)
DIAGSymbolRead(fp,header,offset)
FILE *fp;
VOID *header;
i4  offset;
{
}

# endif

