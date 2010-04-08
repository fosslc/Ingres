# include	<compat.h>

# ifdef VMS

/*
+* Filename:	IICOBPACK.C
** Purpose:	Routines to allow Cobol scaled packed decimal variables to
**		interface with our run-time modules as f8's.  It seems that
**		just assign them to f8's causes some truncation by the 
**		compiler.
** Defines:
**		IIpktof8()		- Convert scaled packed to f8.
**		IIf8topk()		- Convert f8 to scaled packed.
**		IIcmptof8()		- Convert scaled COMP to f8.
**		IIf8tocmp()		- Convert f8 to scaled COMP.
**
** Notes:	This routine is VMS dependent about the assumptions it makes
** 		about the internal storage of ceratin types.
**
** 	COBOL Type	VAX Type
**
**	comp (scaled)	i2 or i4	- Work with f8 variable.
**	comp-2		f8		- Is an f8 for use to use.
-*	comp-3		packed		- Use f8 (if scaled or too large).
**
** History:	
**		23-may-1985 	Written. (ncg)
**		02-aug-1989	Shut up ranlib (GordonW)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	COMP_3	char
# define	COMP	char

/*
+* Procedure:	IIpktof8 
** Purpose:	Format Packed Decimal (COMP-3) data into C f8.
** Parameters:	dbl		- f8 *	- COMP-2 variable.
**		pkvar		- char *- Packed variable.
** 		precision	- i4	- Full precision of COMP-3.
**		scale		- i4 	- Scale factor.
** Returns:	None
** Example:	
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
-*		CALL "IIPKTOF8" USING P IIF8 BY VALUE 5 3.
*/

void
IIpktof8( dbl, pkvar, precision, scale )
f8	*dbl;
COMP_3	*pkvar;
i4	precision, scale;
{
    i4	lprec = precision;
    i4	lscale = scale;		/* so not to change original value */

    IIptod( dbl, pkvar, lprec, lscale );
}

/*
+* Procedure:	IIf8topk
** Purpose:	Format C f8 data into Packed Decimal (COMP-3).
** Parameters:	dbl		- f8 *	- COMP-2 variable.
**		pkvar		- char *- Packed variable.
** 		precision	- i4	- Full precision of COMP-3.
**		scale		- i4 	- Scale factor.
** Returns:	None
** Example:	
**		01 P PIC S9(2)V9(3) USAGE COMP-3.
-*		CALL "IIF8TOPK" USING P IIF8 BY VALUE 5 3.
*/

void
IIf8topk( dbl, pkvar, precision, scale )
f8	*dbl;
COMP_3	*pkvar;		
i4	precision, scale;
{
    i4		lprec = precision;
    i4		lscale = scale;		/* so not to change original value */

    IIdtop( *dbl, pkvar, lprec, lscale );
}

/*
+* Procedure:	IIcmptof8
** Purpose:	Format COBOL scaled COMP data to C f8 data.
** Parameters:	dbl		- f8 *	- COMP-2 variable.
**		cmpvar		- char *- COMP variable.
** 		precision	- i4	- Full precision of COMP.
**		scale		- i4 	- Scale factor.
** Returns:	None
** Example:	
**		01 C PIC S9(2)V9(3) USAGE COMP.
-*		CALL "IICMPTOF8" USING C IIF8 BY VALUE 5 3.
*/

void
IIcmptof8( dbl, cmpvar, precision, scale )
f8	*dbl;
COMP	*cmpvar;		
i4	precision, scale;
{
    f8		ldbl;
    i4		lscale = scale;		/* So not to change original value */

    if (precision <= 4)			/* Stored as an i2 */
	ldbl = (f8) (*(i2 *)cmpvar);
    else				/* Stored as an i4 - quadword not sup */
	ldbl = (f8) (*(i4 *)cmpvar);

    while (lscale--)			/* Scale the integer down */
	ldbl /= 10.0;
    *dbl = ldbl;
}


/*
+* Procedure:	IIf8tocmp
** Purpose:	Format C f8 to COBOL scaled COMP data item.
** Parameters:	dbl		- f8 *	- COMP-2 variable.
**		cmpvar		- char *- COMP variable.
** 		precision	- i4	- Full precision of COMP.
**		scale		- i4 	- Scale factor.
** Returns:	None
** Example:	
**		01 C PIC S9(2)V9(3) USAGE COMP.
-*		CALL "IIF8TOCMP" USING C IIF8 BY VALUE 5 3.
*/

void
IIf8tocmp( dbl, cmpvar, precision, scale )
f8	*dbl;
COMP	*cmpvar;		
i4	precision, scale;
{
    f8		ldbl = *dbl;
    i4		lscale = scale;		/* So not to change original value */

    while (lscale--)
	ldbl *= 10.0;

    /* Now round value up just in case */
    if ( ldbl >= 0 )
	ldbl += 0.5;
    else
	ldbl -= 0.5;

    if (precision <= 4)			/* Stored as an i2 */
	*(i2 *)cmpvar = ldbl;
    else
	*(i4 *)cmpvar = ldbl;		/* As an i4 - quadword not supported */
}
# else
static	i4	ranlib_dummy;
# endif /* VMS */
