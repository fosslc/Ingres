/*
** Copyright (c) 1991, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<er.h>
# include	<iicommon.h>
#include	<fe.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilrffrm.h>
/* ILRF Block definition. */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgout.h"


/**
** Name:	cginit.c -	Code Generator Data Initialization Module.
**
** Description:
**	Performs initializations for the code generator's global variables.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved iicgFormVar here from cgstrs.roc.
**
**	Revision 6.3/03/01
**	01/11/91 (emerson)
**		Remove 32K limit on number of source statements:
**		Change iiCGlineNo from i4  to i4.
**
**	Revision 6.3  89/12  wong
**	Added 'iiCGlineNo' and 'iiCGqryIndex'.  JupBug #7899.
**
**	Revision 6.0  87/06  arthur
**	Initial revision.
**      24-sep-96 (hanch04)
**              Global data moved to cginit.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      19-dec-2008 (joea)
**          Replace GLOBALDEF const by GLOBALCONSTDEF.
**/

GLOBALDEF CG_GLOBS	IICGgloGlobs ZERO_FILL;	/* globals from command line */

GLOBALDEF IRBLK		IICGirbIrblk ZERO_FILL;	/* IRBLK for the ILRF session */

GLOBALDEF i4	IICGotlTotLen = 0;	/* length of the output line so far */

/* Array of tabs for indenting */
GLOBALDEF char	IICGoiaIndArray[] =	"\0\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

GLOBALDEF bool	IICGdonFrmDone = FALSE;	/* finished with all IL for frame? */

GLOBALDEF i4	iiCGlineNo = 0;		/* current line number */
GLOBALDEF i4		iiCGqryID = 0;		/* query ID (index) */

GLOBALDEF char	iicgFormVar[CGBUFSIZE] = "IICform"; /* name of variable
						    ** containing form name */

/*
**	cgostruc.c
*/

/*
** Name:        _IICGcontrol -  C Control Statement Names.
*/
GLOBALCONSTDEF char _IICGif[]       = ERx("if");
GLOBALCONSTDEF char _IICGelseif[]   = ERx("else if");
GLOBALCONSTDEF char _IICGfor[]      = ERx("for");
GLOBALCONSTDEF char _IICGwhile[]    = ERx("while");
