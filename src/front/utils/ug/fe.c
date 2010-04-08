/*
**  Copyright (c) 1985, 2008 Ingres Corporation
**  All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
LIBOBJECT = fe.c 
**
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"feloc.h"
#ifdef	FE_ALLOC_DBG
#include	<si.h>
#endif

/**
** Name:	fe.c -	Front-End Utility Module Global Data Definitions File.
**
** Description:
**	Contains the global data definitions for the FE memeory allocation
**	module.
**
** History:
**	Revision 6.0  87/09/16  boba
**	Changed memory allocation to use 'FEreqmem()'.
**
**	Revision  4.0  85/01/15  ron
**	1/15/1985 (rlk) - First written.
**
**	7/93 (Mike S) 
**	    We now let ME (in the GL) allocate tags for us.
**	    Remove obsolete globals iiUGtags and iiUGtotTags.  We can't
**	    count on getting the first tag ME allocates, so we init iiUGcurTag
**	    to 0 and keep track in iiUGfirstTag
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-feb-2001 (somsa01)
**	    Changed type of FE_allocsize to SIZE_TYPE.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile. Added LIBOBJECT hint which creates a rule
**	    that copies specified object file to II_SYSTEM/ingres/lib
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/* Tag-ID associated with current tag region. */

GLOBALDEF TAGID	iiUGcurTag = 0;

/* First tag used by FE allocator */
GLOBALDEF TAGID iiUGfirstTag;

/* Allocation size (default is ALLOCSIZE.) */

GLOBALDEF SIZE_TYPE	FE_allocsize = 0;

/* Empty string. */

GLOBALDEF const char	iiUGempty[1] = {EOS};

/* Debug only globals */

#ifdef	FE_ALLOC_DBG
GLOBALDEF bool	FE_firstcall = TRUE;	/* Debug only - True if first call to
						'iiugReqmem()' */
GLOBALDEF FILE	*FE_dbgfile = NULL;	/* Debug file pointer */
GLOBALDEF char	*FE_dbgname = "iiugReqMem.log";	/* Debug file name */
GLOBALDEF i4	FE_totblk = 0;		/* Total # blocks allocated from ME */
GLOBALDEF i4	FE_totbyt = 0;		/* Total # bytes allocated from ME */
#endif

