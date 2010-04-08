/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
**	Name:	dctdll.h
**
**	Define data across dll boundary as func calls.
**
**	17-aug-91 (leighb) DeskTop Porting Change: Created
**		Reference data via a function call instead of referencing
**		data across facilities (for DLL's and shared libraries).
**	05-sep-91 (leighb) DeskTop Porting Change:
**		Moved ifdef to this header, unconditionally include
**		it in all modules that reference data across facilities.
**      09-Jan-96 (fanra01)
**              Added precompiler section to include data from DLL on Windows
**              NT.
*/

# ifdef	DLLSHARELIB

#define	clienttable	get_clienttable()
#define	moduletable	get_moduletable()
#define	DD_dev		get_DD_dev()
#define	DD_path		get_DD_path()

# else

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF PTR clienttable;
GLOBALDLLREF PTR moduletable;
GLOBALDLLREF char DD_dev[];
GLOBALDLLREF char DD_path[];
# else              /* NT_GENERRIC && IMPORT_DLL_DATA */
GLOBALREF PTR clienttable; /* hash table for client descriptions (CLIENTDESC) */
GLOBALREF PTR moduletable; /* hash table for module descriptions (MODULEDESC) */
GLOBALREF char DD_dev[];   /* device containing client and module files */
GLOBALREF char DD_path[];  /* path containing client and module files */
# endif             /* NT_GENERRIC && IMPORT_DLL_DATA */

# endif
