/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DIMO.H - DI MO related prototypes and defines.
**
** Description:
**	Prototypes for the external DImo routines:
**		DImo_init_slv_mem()
**		DImo_attach();
** History:
**      26-jul-93 (mikem)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS DImo_init_slv_mem(
    i4    num_slaves, 
    CSSL_CB *di_slave);

FUNC_EXTERN STATUS DImo_attach(void);
