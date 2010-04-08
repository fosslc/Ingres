/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ITdrloc.h	- header file for drawing char tbl
**
** Description:
**	This file declares the drawing character table
**	and the external variables of default drawing
**	characters.
**
** History:
**	30-jan-1987 (yamamoto)
**		first written
**	04/14/90 (dkh) -  Removed extern of lseflg.
**	24-sep-96 (mcgem01)
**	    externs changed to GLOBALREF
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-nov-2009 (wanfr01) b122875
**	    moved some constants to itline.h
**/

/*
**	declaration for character pointer that hold the 
**	default value of drchtbl[].drchar and drchtbl[].prchar
*/

GLOBALREF char	*CHLD;
GLOBALREF char	*CHLE;
GLOBALREF char	*CHLS;
GLOBALREF char	*CHQA;
GLOBALREF char	*CHQB;
GLOBALREF char	*CHQC;
GLOBALREF char	*CHQD;
GLOBALREF char	*CHQE;
GLOBALREF char	*CHQF;
GLOBALREF char	*CHQG;
GLOBALREF char	*CHQH;
GLOBALREF char	*CHQI;
GLOBALREF char	*CHQJ;
GLOBALREF char	*CHQK;

/*
**	flags used by ITlndraw()
*/

GLOBALREF i4	dblflg;
GLOBALREF i4	dbpflg;
