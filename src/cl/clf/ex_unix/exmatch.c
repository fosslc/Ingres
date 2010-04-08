/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<stdarg.h>
# include	<ex.h>
# include	"exi.h"

/*
** EXmatch
**	xx-xxx-xx (xxxxxx)
**		Created.
**	09-apr-91 (seng)
**		Added #include <systypes.h> because <varargs.h> includes 
**		<sys/types.h> on the RS/6000
**	31-jan-92 (bonobo)
**	    	Replaced double-question marks; ANSI compiler thinks they
**	    	are trigraph sequences.
**	31-Dec-2003 (hanje04)
**		GCC compiler on Linux no longer supports older va_start
**		macro which only takes one argument. Re-implement function
**		to use 2 arg va_start.
*/

/*VARARGS*/
i4
EXmatch( EX target, ... )


{
    va_list ap;
    register i4  arg_count;
    register i4  index;
    register i4  ret_val = 0;

    va_start( ap, target );
    arg_count = va_arg( ap, i4 );
    for ( index = 1; index <= arg_count; index++ )
	if ( target == va_arg( ap, EX ) )
	{
	    ret_val = index;
	    break;
	}
    va_end( ap );
    return( ret_val );
}
