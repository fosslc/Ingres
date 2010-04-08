
/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"gnspace.h"
# include	"gnint.h"

/**
** Name:	gnglob.c - GN global variables
**	
*/

GLOBALDEF SPACE *List = NULL;	/* list of known spaces ("World of Ptavvs",
				"Neutron Star", ... - sorry) */

GLOBALDEF i4  Clen;	/* current comparison length for hash table keys */
GLOBALDEF i4  Hlen;	/* significance length for hashing of table keys */

GLOBALDEF bool Igncase;	/* ignore case in hash table comparison */
