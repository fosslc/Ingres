/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#ifdef	FE_ALLOC_DBG
#include	<si.h>
#include	<er.h>
#endif
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"feloc.h"

/**
** Name:	fefree.c -	Front-End Utility Memory Free Module.
**
** Description:
**	Contains the memory free routine for the FE memory allocation module.
**	Defines:
**
**	FEfree()	free tagged allocation blocks.
**
** History:
**	Revision 3.0  84/10/31  rlk
**	Initial revision.
**/

/*{
** Name:	FEfree() -	Free Memory Previously Allocated by FE calls.
**
** Descriptions
** 	Frees a region of memory tagged with the specified tag.  This memory
**	block must have been initially allocated using one of the FE memory
**	allocation routines.
**
** Inpuat:
**	tag	{TAGID}  The tag ID of the region to be freed.
**
** Returns:
**	{STATUS}  OK or FAIL	
**
** Assumptions:
**	The region of memory must have been allocated using an FE routine.
**
** History:
**	10/31/84 (rlk) - First Written
**	1/15/85 (rlk) - Modified to use a different allocation scheme.
**	27-oct-1988 (jhw) -- Check that tag is allocated before trying
**		to free.  Clearing the 'iiUGtags[index]' will trash memory
**		if it hasn't been.  Jupiter bug #3817.
**	18-apr-1993 (emerson)
**		Issue the MEtfree even if no iiugReqMem requests have been
**		issued for the tag (i.e. there's no slot for the tag in the
**		iiUGtags array, or it's NULL).  This corrects a problem
**		where if a tag was acquired using FEgettag and it was used
**		solely for ME requests, then an IIUGtagFree on the tag
**		did not free the memory (although it did free the tag).
**      11-may-1993 (rogerl)
**              Although if nothing is freed, return OK instead of ME_NO_TFREE.
**              (51840)
**      18-may-1993 (rdrane)
**              Return OK if the we have a valid tag, even if there was
**              no memory associated with it.  Otherwise, the client can
**              get confused if a tag was acquired using FEgettag() and it
**              was used solely for ME requests (there are too many clients
**              to fix any other way, which is why the 18-apr-1993 change was
**              effected).  This is essentially rogerl's fix, but we make
**              sure that we always free a valid tag.
**	7/93 (Mike S) Make this a cover over IIUGfreetagblock, which
**		is in fedoreq.c.  
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
*/

STATUS
FEfree ( TAGID tag )
{
	return IIUGfreetagblock(tag);
}

