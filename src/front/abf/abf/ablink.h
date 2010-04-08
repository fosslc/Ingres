/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ablink.h - Defines for linking ABF applications
**
** Description:
**	Define structure ABLNKARGS.
**
** History:
**	3/30/90 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*}
** Name:	ABLNKARGS - arguments for linking ABF application
**
** Description:
**	A union of the arguments for a test link with the arguments for an
**	image link.
**
** History:
**	3/30/90 (Mike S) Initial version
*/
typedef struct
{
	APPL 	*app;		/* Application to link */
	i4	link_type;	/* Image or test */
# define ABLNK_TEST 	0
# define ABLNK_IMAGE 	1

	/*
	**	The following is only needed for a test link. 
	*/
	ABLNKTST *tstimg;		/* Test build info */

	/*
	**	The following are only needed for an image link
	*/
	char *executable;	/* Executable name */
	i4 options;		/* Link options */
} 
ABLNKARGS;
