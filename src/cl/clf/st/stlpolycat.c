/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved.
*/

# include    <compat.h>
# include    <gl.h>
# include    <systypes.h>
# include    <stdarg.h>
# include    <st.h>

/*
**    STlpolycat - Multiple string concatenate with length check.
**
**    Concatenate N null-terminated strings into space supplied by the user
**    and null terminate.  Return pointer.
**    Differs from STpolycat in that the caller also supplies the size
**    of the result area (size NOT to include the trailing null slot).
**    Use STlpolycat for safe concatenation that prevents stack smash,
**    variable overrun, etc.
**
** Usage:
**	char *ptr = STlpolycat(N, len, c1, c2, ... cN, &result[0]);
**	i4 N;  -- Number of input strings to concatenate
**	i4 len;  -- Length of result not including trailing null
**	         ie len == sizeof(result) - 1.
**	char *cn;  -- Input strings
**	char result[];  -- Where to put result
**	Returns &result[0], ie pointer to result
**
** History:
**	14-Jun-2004 (schka24)
**	    Copy from STpolycat for safe string handling
**	17-Sep-2008 (smeke01) b120911
**	    Should test for EOS of substring BEFORE decrementing
**	    remaining length count of destination string.
*/

/*VARARGS2*/
char *
STlpolycat( i4  n, i4 len, char *c1, char *c2, ... )
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
    while (*c1 != '\0' && --len >= 0)
        *resultstr++ = *c1++;
    if ( N > 1 )
    {
        while (*c2 != '\0' && --len >= 0)
            *resultstr++ = *c2++;
    }
    N -= 2;

    if ( N > 0 )
    {
        va_start( args, c2 );
        while ( N-- )
        {
            charstr = va_arg( args, char * );
            while (*charstr != '\0' && --len >= 0)
                *resultstr++ = *charstr++;
        }
        va_end( args );
    }

    *resultstr = '\0';
    
    return (result);
}
