/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: GCACLI.H - GCA CL internal header file.
**
** History:
**	13-jan-89 (seiwald)
**	    written.
**	06-Jun-90 (seiwald)
**	    Made GCalloc() return the pointer to allocated memory.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern PTR	(*GCalloc)();   /* Memory allocation routine */
extern VOID	(*GCfree)();    /* Memory deallocation routine */
extern i4	gc_trace; 	/* Tracing Flag */

# define GCTRACE(n) if( gc_trace >= n )(void)TRdisplay
