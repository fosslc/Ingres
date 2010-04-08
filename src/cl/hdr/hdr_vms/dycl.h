/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name: DY.H - Global definitions for the DY compatibility library.
**
** Description:
**      The file contains the type used by DY and the definition of the
**      DY functions.  These are used for dynamic linking.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS  DYinit();           /* Initialize for link. */
FUNC_EXTERN STATUS  DYload();           /* Dynamically load. */

/* 
** Defined Constants
*/

/* DY return status codes. */

#define                 DY_OK           0
# define		DY_NOSTAB	(E_CL_MASK + E_DY_MASK + 0x01)
# define		DY_NOEXE	(E_CL_MASK + E_DY_MASK + 0x02)
# define		DY_NOMEM	(E_CL_MASK + E_DY_MASK + 0x03)
# define		DY_NOREAD	(E_CL_MASK + E_DY_MASK + 0x04)
# define		DY_NOFILE	(E_CL_MASK + E_DY_MASK + 0x05)

/*
** Defines external types needed for dynamic linking.
*/

typedef struct
{
	char	*sym_name;	/* Symbols name */
	i4	*sym_value;	/* Its address */
} SYMTAB;

