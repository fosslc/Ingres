/*
**    Copyright (c) 2004, Ingres Corporation
*/


#include <compat.h>
#include <si.h>

/*
** SISTUBS.c - 	Functions that emulate normal CL runtime behavior when the
**		FUSEDLL condition is defined.  If FUSEDLL is not defined
**		SIclose, SIgetc, and SIungetc are defined directly to be
**		fclose, getc, amd ungetc respectively.
**
**	Written:
**				
**	18-jan-2001 (crido01)
*/

#ifdef FUSEDLL
STATUS 
SIclose( FILE *fi )
{ 
    return( fclose(fi) );
}
int SIgetc( FILE *fi )
{
    return( getc( fi ) );
} 
int SIungetc( int ch, FILE *fi )
{
    return( ungetc( ch, fi ) );
} 
#endif
