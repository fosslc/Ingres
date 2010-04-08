/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"iamstd.h"

/**
** Name:	iamtname.c - table names
**
** Description:
**	Catalog name variables.	 Only set up for catalogs with names > 12,
**	so we can HACKFOR50 shorter names.
** History :
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPINTERPLIBDATA
**	    in the Jamfile.
*/

/*
**
LIBRARY = IMPINTERPLIBDATA
**
*/

GLOBALDEF char *Depcat = ERx("ii_abfdependencies");
GLOBALDEF char *Aocat = ERx("ii_abfobjects");
GLOBALDEF char *Longcat = ERx("ii_longremarks");
