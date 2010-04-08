/*
**  Copyright (c) 2004 Ingres Corporation
**  All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"feloc.h"

/**
** Name:	fereq.c -	Front End Memory Allocation Routines.
**
** Description:
**	Contains the general FE memory allocation routines.  Defines:
**
**	FEreqmem()		allocate small blocks of memory.
**	FEreqlng()		allocate large blocks of memory.
**
** History:
**	Revision 6.0  87-jul-06  bruceb
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-feb-2001 (somsa01)
**	    Changed type of sizes to SIZE_TYPE.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF TAGID	iiUGcurTag;
GLOBALREF TAGID	iiUGfirstTag;

/*{
** Name:	FEreqmem() -	Allocate a (Small) Block of Tagged Memory.
**
** Description:
** 	Returns a pointer to a block of dynamically allocated memory that can
**	hold 'size' bytes upto a maximum of 'u_i4'.  This block of memory is
**	tagged either with the tag-ID passed in to this routine, or if that tag
**	is zero, with the current tag block.  (See 'FEbegintag()' and
**	'FEendtag()'.)
**
*	This routine is used in conjunction with 'FEbegintag()', 'FEendtag()',
**	and 'FEfree()'.
**
** Inputs:
**	tag	{u_i4}  Tag-ID of the memory region.
**	size	{u_i4}  Number of bytes to allocate.
**	clear	{bool}  Indicate whether storage is to be cleared.
**
** Outputs:
**	status	{STATUS *}  OK or FAIL depending on whether allocation
**			succeeded or failed.  Not set if passed in as
**			a NULL.
**
** Returns:
**	{PTR}	Pointer to block of memory or NULL on error.
**
** History:
**	06-jul-87 (bab)	Modified using FE[tc]alloc as a base.
**    	7/93 (Mike S) Check for iiUGcurTag being 0
*/

PTR
FEreqmem ( u_i4 tag, SIZE_TYPE size, bool clear, STATUS *status )
{
	PTR		block;
	STATUS		istatus;

       if (iiUGcurTag == 0)
       {
           /* Initialize at the first tag */
           iiUGcurTag = iiUGfirstTag = FEgettag();
       }

	if ( tag == 0 )			/* if no specific tag is desired */
		tag = iiUGcurTag;	/* then use the current tag */

	block = iiugReqMem(tag, size, clear, &istatus);
	
	if ( status != NULL )
		*status = istatus;

	return ( istatus != OK ) ? (PTR)NULL : block;
}

/*{
** Name:	FEreqlng() -	Allocate a (Large) Block of Tagged Memory.
**
** Description:
** 	Returns a pointer to a block of dynamically allocated memory that can
**	hold 'size' bytes upto a (large) maximum of 'u_i4'.  This routine
**	differs from 'FEreqmem()' in this respect since 'FEreqmem()' can only
**	allocate upto a maximum of 'u_i4' bytes.  This block of memory is
**	tagged either with the tag-ID passed in to this routine, or if that
**	tag is zero, with the current tag block.  (See 'FEbegintag()' and
**	'FEendtag()'.)
**
**	This routine is used in conjunction with 'FEbegintag()', 'FEendtag()',
**	and 'FEfree()'.
**
** Inputs:
**	tag	{u_i4}  Tag-ID of the memory region.
**	size	{u_i4}  Number of bytes to allocate.
**	clear	{bool}  Indicate whether storage is to be cleared.
**
** Outputs:
**	status	{STATUS *}  OK or FAIL depending on whether allocation
**			succeeded or failed.  Not set if passed in as
**			a NULL.
**
** Returns:
**	{PTR}	Pointer to block of memory or NULL on error.
**
** History:
**	06-jul-87 (bab)	Modified using FE[tc]alloc as a base.
*/

PTR
FEreqlng ( u_i4 tag, SIZE_TYPE size, bool clear, STATUS *status )
{
	PTR		block;
	STATUS		istatus;

	if ( tag == 0 )			/* if no specific tag is desired */
		tag = iiUGcurTag;	/* then use the current tag */

	block = iiugReqMem(tag, size, clear, &istatus);
	
	if ( status != NULL )
		*status = istatus;

	return ( istatus != OK ) ? (PTR)NULL : block;
}
