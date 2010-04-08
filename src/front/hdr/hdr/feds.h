/*-------------------------- feds.h --------------------------------*/
/* static char Sccsid[] = "@(#)feds.h	1.2	1/29/85"; */


/*
**	feds.h
**
**	Copyright (c) 2004 Ingres Corporation	
**	All rights reserved.
*/

/**
** Name:	feds.h - FrontEnd DS header file
**
** Description:
**	Header for FrontEnd DS related declarations.
**
** History:
**	Initial version somewhere at the dawn of time.
**	10/13/92 (dkh) - Updated to define DSTARRAY so that the use
**			 of ArrayOf can be removed.  The alpha C
**			 compiler is confused by ArrayOf.  We
**			 assume that clients include ds.h ahead
**			 of this header file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<fedscnst.h>

/*
**  A descriptor for an array of DSTEMPLATE pointers.
*/
typedef struct
{
	i4		size;		/* number of DSTEMPLATEs */
	DSTEMPLATE	**array;	/* array of DSTEMPLATE pointers */
} DSTARRAY;


/*---------------------------- +++ ---------------------------------*/
