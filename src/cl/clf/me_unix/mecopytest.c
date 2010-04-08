/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** mecopytest.c -- test to make sure MECOPY macros are working right.
**
** History:
**	08-may-89 (daveb)
**		Created.
**	10-may-89 (daveb)
**		Add checks to make sure that the source is left along,
**		and also that handing the wrong type pointer to the
**		macros works correctly.
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	06-jan-1998 (fucch01)
**	    casted 1st param in calls to MEcopy w/ (PTR) for picky IRIX
**	    (sgi_us5) compiler...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-mar-2002 (somsa01)
**	    Added MING hints for successful 64-bit compilation.
*/

/*
PROGRAM = mecopytest

NEEDLIBS = COMPATLIB
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>
# include <si.h>

char source[] = { "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" };

# define SIZE	sizeof(source)

char vsource[ SIZE ];
char csource[ SIZE ];
char fsource[ SIZE ];

char destvar[ SIZE + 1 ];
char destconst[ SIZE + 1 ];
char destfunc[ SIZE + 1 ];

int err = 0;

main(
	VOID)
{
	int i;
	char *from;

	char *vfrom;
	char *cfrom;
	char *ffrom;

	int *vints = (int *)source;
	int *cints = (int *)source;
	int *fints = (int *)source;

	STcopy( source, vsource );
	STcopy( source, csource );
	STcopy( source, fsource );

	/* check straight char stuff */
	for( i = 0; i < SIZE ; i++ )
	{
		/* clear dests */
		MEfill( SIZE, 0, destconst );
		MEfill( SIZE, 0, destvar );
		MEfill( SIZE, 0, destfunc );

		/* moving trailing part of string to front of dest */

		from = &source[ SIZE - 1] - i;
		vfrom = &vsource[ SIZE - 1] - i;
		cfrom = &csource[ SIZE - 1] - i;
		ffrom = &fsource[ SIZE - 1] - i;

		MECOPY_CONST_MACRO( cfrom, i, destconst );
		check( from, destconst, i , "const" );

		MECOPY_VAR_MACRO( vfrom, i, destvar );
		check( from, destvar, i , "var" );

		MEcopy( ffrom, i, destfunc );
		check( from, destfunc, i , "func" );
	}

	/* check handling when given non-char ptr arg */

	for( i = 0; i < SIZE ; i++ )
	{
		/* clear dests */
		MEfill( SIZE, 0, destconst );
		MEfill( SIZE, 0, destvar );
		MEfill( SIZE, 0, destfunc );

		MECOPY_CONST_MACRO((PTR) cints, i, destconst );
		check( source, destconst, i , "const" );

		MECOPY_VAR_MACRO((PTR) vints, i, destvar );
		check( source, destvar, i , "var" );

		MEcopy((PTR) fints, i, destfunc );
		check( source, destfunc, i , "func" );
	}

	SIprintf( err ? "Failures!!!\n" : "MEcopy & MECOPY_MACROS OK\n" );
	PCexit( err );
}

check(
	char *a,
	char *b,
	i4 len,
	char *name)
{
    if( MEcmp( a, b, (u_i2)len ) )
    {
	SIprintf("%s failed at length %d\n", name, len );
	err++;
    }
    if( b[len] )
    {
	SIprintf("%s wrote past end for length %d\n", name, len);
	err++;
    }
    else
    {
	/* terminated string, so show it */
    	SIprintf("%-10s %2d: >%s<\n", name, len, b );
    }
}
