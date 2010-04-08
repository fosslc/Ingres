/*
** Copyright (c) 2004 Ingres Corporation 
*/
/*
**  FRAME.c  -  Initialize frame constants
**	
**  This routine initializes the variables associated with the
**  frame driver.
**	
**  History:  JEN  -  13 Nov 1981
**	03/05/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	08/15/90 (dkh) - Fixed bug 21670.
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

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	<si.h>

#ifdef SEP

# include       <tc.h>

GLOBALDEF TCFILE *IIFDcommfile = NULL;   /* pointer to output COMM-file */

#endif /* SEP */

GLOBALDEF i4	frcommno = 0;		/* number of commands */
GLOBALDEF char	fruser[FE_MAXNAME + 1] = {0}; /* the frame INGRES user */
GLOBALDEF char	frdba[FE_MAXNAME + 1] = {0};  /* the frame INGRES dba */
GLOBALDEF char	frusername[FE_MAXNAME + 1] = {0};/* the frame INGRES username */
GLOBALDEF bool	frdebuglist[100] = {0};	/* list of frame debuggers */
GLOBALDEF FRAME	*frcurfrm = NULL;
GLOBALDEF FILE	*fddmpfile = NULL;	/* dump file */
GLOBALDEF char	dmpbuf[MAX_TERM_SIZE + 1];

/*
**  Following ifdef necessary because IBM needs to initialize vars
**  but VMS/UNIX can't initialize unions. (dkh)
*/

# ifdef WSC
GLOBALDEF FRAMEUNION	_framecast = {0};
# else
GLOBALDEF FRAMEUNION	_framecast;	/* see frame.h */
# endif

GLOBALDEF FT_TERMINFO	frterminfo = {0};	/* terminal information */

/*
**  Initialize utility procedure control block.
*/
GLOBALDEF	FRS_UTPR	fdutprocs = {0};
