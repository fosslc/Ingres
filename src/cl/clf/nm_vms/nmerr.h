/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name: NM.H - Global definitions for the NM compatibility library.
**
** Description:
**      The file contains the local types used by NM.
**      These are used for name manipulation.
**
** History:
**      03-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**/

/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

/* NM return status codes. */

# define	 NM_PWDOPN	(E_CL_MASK + E_NM_MASK + 0x01)
/*  NMpathIng: Unable to open passwd file. */
# define	 NM_PWDFMT	(E_CL_MASK + E_NM_MASK + 0x02)
/*  NMpathIng: Bad passwd file format. */
# define	 NM_INGUSR	(E_CL_MASK + E_NM_MASK + 0x03)
/*  NMpathIng: There is no INGRES user in the passwd file. */
# define	 NM_INGPTH	(E_CL_MASK + E_NM_MASK + 0x04)
/*  NM[sg]tIngAt: No path returned for INGRES user. */
# define	 NM_STOPN	(E_CL_MASK + E_NM_MASK + 0x05)
/*  NM[sg]tIngAt: Unable to open symbol table. */
# define	 NM_STPLC	(E_CL_MASK + E_NM_MASK + 0x06)
/*  NMstIngAt: Unable to position to record in symbol table. */
# define	 NM_STREP	(E_CL_MASK + E_NM_MASK + 0x07)
/*  NMstIngAt: Unable to replace record in symbol table. */
# define	 NM_STAPP	(E_CL_MASK + E_NM_MASK + 0x08)
/*  NMstIngAt: Unable to append symbol to symbol table. */
# define	NM_LOC		(E_CL_MASK + E_NM_MASK + 0x09)
/*  NM_loc: first argument must be 'f' or 't' */
# define        NM_BAD_PWD      (E_CL_MASK + E_NM_MASK + 0x0A)
/*  NMsetpart: Part name too long */
# define        NM_PRTLNG	(E_CL_MASK + E_NM_MASK + 0x0b)
/*  NMsetpart: Illegal part name format */
# define        NM_PRTFMT	(E_CL_MASK + E_NM_MASK + 0x0c)
/*  NMsetpart: Part name is a null string */
# define	NM_PRTNUL	(E_CL_MASK + E_NM_MASK + 0x0d)
/*  NMsetpart: Part name already set */
# define	NM_PRTALRDY	(E_CL_MASK + E_NM_MASK + 0x0e)
