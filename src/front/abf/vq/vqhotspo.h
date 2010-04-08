

/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vqhotspo.h -	hot spot definitions
**
** Description:
**	This file contains definitions for maintenance of 
**	an array of hotspots. Special trim is created, and
**	then if the user button clicks on the hotspots 
**	the form system send back the number of the trim element;
**
** History:
**	01/13/90 - (tom) created
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/



/*}
** Name:	HOTSPOT - hot spot structure
**
** Description:
**	an array of these keeps track of hotspot info
**
** History:
**	03/31/89 (tom) - created
*/
typedef struct
{
	i2	num;		/* trim number */
	i2	row;		/* AFD screen row of frame */ 
	i2	col;		/* AFD screen col */
	i2	down;		/* flag to say if we go down */
} HOTSPOT; 

#define FF_MAXHOTSPOTS 150

GLOBALREF HOTSPOT IIVQhsaHotSpotArray[FF_MAXHOTSPOTS];
GLOBALREF i4  IIVQhsiHotSpotIdx;


