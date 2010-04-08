/*
**	Copyright (c) 2008 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<si.h>

/**
** Name:	sifused.c -	Default SI implementations under FUSEDLL build
**
** Description:
**	This file contains the definitions of routines that are normally
**	#define'ed directly to CRT implementations.
**
**	When building with FUSEDLL defined, those mappings do not apply, and 
**	we need real (albeit default) implementations for these functions, so
**	that we can obtain a complete si.lib suitable for linking the Ingres
**	build tools (such as ercomp, eqc, esqlc).
**
**	Under a FUSEDLL build, OpenROAD links with the w4si clone, instead of
**	this si.  But both the si and w4si libraries remain complete and plug-
**	compatible with each other.
**
** History:
**	14-oct-2004 (dansa02)
**	    Created for flat-linked OpenROAD build process.
**	27-oct-2008 (zhayu01) SIR 120921
**	    Change the file name from siduumy.c to sifused.c.
*/

#ifdef SIclose
#undef SIclose
#endif

STATUS
SIclose( FILE *fi )
{
    return fclose( fi );
}

#ifdef SIgetc
#undef SIgetc
#endif

/*
** SIgetc
*/
int SIgetc( FILE *stream )
{
    return getc( stream );
}

#ifdef SIungetc
#undef SIungetc
#endif

/*
** SIungetc
*/
int SIungetc( int ch, FILE *stream )
{
    return ungetc( ch, stream );
}
