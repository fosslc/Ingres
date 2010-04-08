/*
**  IIVMSSD.H -  Type descriptor used by VAX/VMS languages
**
**	This descriptor can be used to determine the type of
**	data being passed to LIBQ. At the present time we use it
**	only for describing strings.
**
**	There are three kinds of strings of interest:
**		static strings	-- passed by FORTRAN, COBOL, PASCAL
**		dynamic strings -- passed by BASIC
**		varying strings -- passed by PASCAL V2.0
**  History:
**		26-feb-1985	- Rewritten for Equel Rewrite (ncg)
**
**  Copyright (c) 2004 Ingres Corporation
*/

# define	DSC_S		(char)1		/* Static string type */
# define	DSC_D		(char)2		/* Dynamic string */
# define	DSC_VS		(char)11	/* Varying string */

typedef struct sdscr
{
	u_i2	sd_len;			/* Length of descriptor */
	char	sd_type;		/* Type (unused) */
	char	sd_class;		/* Class of descriptor */
	char	*sd_ptr;		/* Pointer into data */
} SDESC;
