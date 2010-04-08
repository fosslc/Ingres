/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<itline.h>
# include	"itdrloc.h"
# include	<er.h>

/**
** Name:	ITdrloc.c	- definitions for drawing char tbl
**
** Description:
**	This file define the drawing character table and
**	the default value of the characters
**
** History:
**	30-jan-1987 (yamamoto) - first written
**	08/14/87 (dkh) - ER changes.
**	04/14/90 (dkh) - Removed lseflg declaration and made it
**			 a local variable for ITlndraw().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/*
** drawing character table
*/

GLOBALDEF DRLINECHAR	drchtbl[NDRINX];

/*
** default drawing character
*/

GLOBALDEF char	*CHLD = ERx("");	/* init terminal to drawing lines */
GLOBALDEF char	*CHLE = ERx("");/* interpret subseq. chars as regular chars  */
GLOBALDEF char	*CHLS = ERx("");/* interpret subseq. chars for drawing chars */
GLOBALDEF char	*CHQA = ERx("+");	/* lower-right corner of a box	*/
GLOBALDEF char	*CHQB = ERx("+");	/* upper-right corner of a box	*/
GLOBALDEF char	*CHQC = ERx("+");	/* upper-left corner of a box	*/
GLOBALDEF char	*CHQD = ERx("+");	/* lower-left corner of a box	*/
GLOBALDEF char	*CHQE = ERx("+");	/* crossing lines		*/
GLOBALDEF char	*CHQF = ERx("-");	/* horizontal line		*/
GLOBALDEF char	*CHQG = ERx("+");	/* left 	'T'		*/
GLOBALDEF char	*CHQH = ERx("+");	/* right 	'T'		*/
GLOBALDEF char	*CHQI = ERx("+");	/* bottom 	'T'		*/
GLOBALDEF char	*CHQJ = ERx("+");	/* top 		'T'		*/
GLOBALDEF char	*CHQK = ERx("|"); 	/* vertical bar			*/

/*
** flags used by IT functions
*/

GLOBALDEF i4	dblflg = 0;
GLOBALDEF i4	dbpflg = 0;
