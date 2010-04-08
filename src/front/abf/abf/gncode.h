/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	gnspace.h -	ABF Name Generation Definitions.
**
** Description:
**	This header file defines encodes to be used to interact with GN,
**	ie., encodes for the numbered name spaces.
**
** History:
**	Revision 6.2  89/09  wong
**	Added AGN_OFILE for object files.  JupBug #8106.
**
**	Revision 6.0  87/05  bobm
**	Initial revision.
*/

/* NOTE!  The procedure, frame, and form symbols name space definitions are used
** to generate global symbols for an application image and for compatability of
** user's object-code cannot be changed!  The file and local symbol name space
** definitions, however, can be changed.
*/
#define AGN_OPROC	1	/* OSL procedure symbols */
#define AGN_OFRAME	2	/* OSL frame symbols */
#define AGN_FORM	3	/* form symbols */
#define AGN_SFILE	4	/* source files */
#define AGN_OFILE	5	/* object files */
#define AGN_LOCAL	6	/* local symbols in abextract */
#define AGN_REC		7	/* record definitions in abextract */

/*
** for AGN_LOCAL, we don't generate prefix string at runtime -
** number in string should match AGN_LOCAL encode.
*/
#define AGN_LPFX ERx("II06")

/*
** lengths to use for generating linkable symbol names
*/
#define AGN_SULEN 7	/* unique in this many */
#define AGN_STLEN 18	/* maximum length */
