#include <bzarch.h>

#include <compat.h>
#include <clconfig.h>
#include <si.h>
#include <lo.h>
#include <me.h>
#include <st.h>
#include <ex.h>
#include <exinternal.h>

# ifdef any_hpux
#include <a.out.h>
#include <aouthdr.h>
#include <syms.h>
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
**	17-Nov-2010 (kschendel) SIR 124685
**	    Drop long-obsolete SunOS 4, dr6.
**	    Prototype fixes.
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
DIAGSymbolLookup( void *pc, unsigned long *addr, unsigned long *offset,
	PTR buffer, i4 size )
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
DIAGSymbolLookupName( char *name )
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
DIAGSymbolLookupAddr( i4 address, i4 *offset )
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
DIAGSymbolLookupName( char *name )
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
DIAGSymbolLookupAddr( uint64_t address, uint64_t *offset )
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

#if !defined(got_DIAGSymbolLookupName)
i4
DIAGSymbolLookupName(char *name)
{
    return (0);
}

PTR
DIAGSymbolLookupAddr(i4 address,i4 *offset)
{
    return (NULL);
}
#endif


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
**	FILHDR *header - obect header;
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
void
DIAGSymbolRead( void *fparg, void *hdrarg, i4 offset )
{

	FILE		*fp = (FILE *) fparg;
	FILHDR		*header = (FILHDR *) hdrarg;
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



#if !defined(got_DIAGSymbolRead)
void
DIAGSymbolRead(void *fp,void *header,i4 offset)
{
}

# endif

