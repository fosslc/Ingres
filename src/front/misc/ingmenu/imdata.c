/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*{
** Name:	imdata.c	- Global declarations for INGMENU.
**
** Description:
**	GLOBALDEFS for INGMENU.
**
** History:
**	02-sep-1987 (peter)
**		Written.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"imconst.h"


GLOBALDEF	char	*im_dbname = ERx("");	     /* database name */
GLOBALDEF	bool	im_empty_catalogs = FALSE;
GLOBALDEF	char	*im_xpipe = ERx(""); /*
					** Pipe names, with -X flag.
					** NOTE:  These must be set
					** to an empty string or
					** a value from the command
					** line.  It must not be
					** NULL in the call to FEingres
					*/
GLOBALDEF	char	*im_uflag = ERx(""); /*
					** Username, with -u flag.
					** NOTE:  These must be set
					** to an empty string or
					** a value from the command
					** line.  It must not be
					** NULL in the call to FEingres
					*/
GLOBALDEF	bool	testingSW = FALSE;
GLOBALDEF	bool	SWsettest = FALSE;
