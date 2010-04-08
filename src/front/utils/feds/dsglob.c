# include	<compat.h>
# include	<lo.h>
# include	<ds.h>
# include	<ex.h>
# include	"dsmapout.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
** DS global definitions.  Extracted from DSload to save linked modules if
** somebody uses DSstore but not DSload.
**
** NOTE:
**	IIFSbase is a char * rather than a PTR because we do arithmetic
**	with it.
**  History:
**      07-Dec-95 (fanra01)
**          Initialised globals for global data project.
**
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


GLOBALDEF i4		IIFStabsize = 0;
GLOBALDEF char		*IIFSbase = NULL;
GLOBALDEF FILE		*IIFSfp = NULL;
GLOBALDEF DSHASH	**IIFShashtab ZERO_FILL;
GLOBALDEF DSTEMPLATE	**IIFStemptab ZERO_FILL;	/* template table */
GLOBALDEF STATUS	IIFSretstat = 0;
