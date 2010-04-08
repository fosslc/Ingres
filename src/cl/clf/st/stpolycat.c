/*
** Copyright (c) 1983, 2004 Ingres Corporation
** All Rights Reserved.
*/

# include    <compat.h>
# include    <gl.h>
# include    <systypes.h>
# include    <stdarg.h>
# include    <st.h>

/*
**    STpolycat - Multiple string concatenate.
**
**    Concatenate N null-terminated strings into space supplied by the user
**    and null terminate.  Return pointer.
**
**    History:
**        2-15-83   - Written (jen)
**        7/8/87      - daveb  Rewrite using varargs, remove all
**                restrictions.
**        21-mar-91 (seng)
**            Add <systypes.h> to clear <types.h> on AIX 3.1
**        08-feb-93 (pearl)
**            Add #include of st.h.
**	08-oct-1998 (canor01)
**	    Update to use stdarg.h instead of varargs.h.
**        
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*VARARGS2*/
char *
STpolycat( i4  n, char *c1, char *c2, ... )
{
    register i4     N = n;
    register char    *charstr;
    register char    *resultstr;
    char        *result;
    va_list        args;
    
    /* Find the result string */

    if ( N == 0 )
        return c1;
    if ( N == 1 )
        resultstr = c2;
    N -= 2; /* start counting after c2 */

    if ( N >= 0 )
    {
        va_start( args, c2 );
        while ( N-- )
            resultstr = va_arg( args, char * );
        resultstr = va_arg( args, char * );
        va_end( args );    
    }

    result = resultstr;

    /* Do the concatenation */

    N = n;
    while (*c1 != '\0')
        *resultstr++ = *c1++;
    if ( N > 1 )
    {
        while (*c2 != '\0')
            *resultstr++ = *c2++;
    }
    N -= 2;

    if ( N > 0 )
    {
        va_start( args, c2 );
        while ( N-- )
        {
            charstr = va_arg( args, char * );
            while (*charstr != '\0')
                *resultstr++ = *charstr++;
        }
        va_end( args );
    }

    *resultstr = '\0';
    
    return (result);
}
