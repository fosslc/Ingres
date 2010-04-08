/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	abfcompl.h -	ABF Compilation Interface.
**
** Description:
**	Contains the interface definitions for the ABF compilation module.
**
** History:
**	Revision 6.3  89/09  wong
**	Initial revision.
**	14-aug-91 (blaise)
**		Added ABF_NOCOMPILE, set if imageapp is issued with -nocompile.
**/

/*
** Name:	ABF_COMPILE -	ABF Compilation Options.
**
** Description:
**	Bit mask definitions for ABF compilation options.
*/
#define ABF_COMPILE	0	/* Default:  no force, no object-code, etc. */

#define ABF_FORCE	1	/* force compilation */
#define ABF_OBJECTCODE	2	/* compile object-code */
#define ABF_OBJONLY	4	/* require object-code, but no source-code */
#define ABF_NOERRDISP	8	/* do not do the error display */
#define ABF_ERRIMMED	16	/* display errors immediately */
#define ABF_CALLDEPS	32	/* load call depenedencies after successful compile */
#define ABF_NOCOMPILE	64	/* don't compile, just link object code */
