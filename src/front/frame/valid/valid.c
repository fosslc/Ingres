/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h> 

/*
**	VALID.c  -  Variables used for validation
**	
**	This file contains all of the global variables used
**	by the validation portion of the frame driver.
**	
**	History:  1 Aug 1981 - written   (JEN)
**		 14 Nov 1985 - Deleted GLOBALDEF FIELD *vl_curfld; (dkh)
**
**	Changed all old initializations since they cause compiler 
**	syntax errs. - nml 
**
**	06/23/89 (dkh) - Changes for derived field support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/* static char	Sccsid[] = "%W%	%G%"; */


GLOBALDEF FRAME	*vl_curfrm = NULL;		/* current frame being parsed */
GLOBALDEF FLDHDR	*vl_curhdr = NULL;
GLOBALDEF FLDTYPE *vl_curtype = NULL;
GLOBALDEF FLDVAL  *vl_curval = NULL;
GLOBALDEF bool	vl_syntax = FALSE;

GLOBALDEF char	Pchar[2] = {0};			/* char I/O stack */
GLOBALDEF i4	Pctr = 0;			/* character pointer */

GLOBALDEF char	*Lastok = NULL;		/* last token on error */
GLOBALDEF i2		Opcode = 0;		/* op code */
GLOBALDEF bool		Newline = 0;		/* new line */
