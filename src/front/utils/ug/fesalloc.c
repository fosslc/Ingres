/*
**  Copyright (c) 2004 Ingres Corporation
**  All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"feloc.h"	/* includes <me.h> */

/**
** Name:	fesalloc.c -	Allocate String in Memory.
**
** Description:
**	Contains a routine that allocates a string in the current block of
**	tagged memory.  Defines:
**
**	FEsalloc()	allocate string in memory.
**
** History:
**	Revision 6.0  1987-aug-20  bruceb
**	Modified to use 'iiugReqMem()'.
**
**	Revision 3.0  1984-dec-21  ron
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-feb-2001 (somsa01)
**	    Corrected compiler warnings.
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	FEsalloc() -	Allocate String in Tagged Memory.
**
** Description:
**	Allocates and returns a reference to a block of memory containing the
**	passed in string.  This block of memory has been tagged with the tag-ID
**	associated with the current tag block.  (See 'FEbegintag()' and
**	'FEendtag()'.)
**
**	This routine is used in conjunction with 'FEbegintag()', 'FEendtag()',
**	and 'FEfree()'.
**
** Input:
**	string	{char *}  An EOS-terminated string.
**
** Returns:
**	{char *}  The allocated string.  (NULL is returned on error.)
**
** History:
**	12/21/84 (rlk) - First Written
**	1/15/1985 (rlk) - Modified to use FEdoalloc.
**	20-aug-87 (bab)	Modified to use 'iiugReqMem()'.
**	10/1/87 (daveb) -- remove erronerous declarations of CL 
**		routines, conflicting with correct decls. in CLentry.h
**	7/93 (Mike S) Use FEreqmem
**      08-Dec-95 (fanra01)
**          Changed all externs to GLOBALREFs
*/

char *
FEsalloc ( char *string )
{
	register char	*blk;
	SIZE_TYPE	len;
	STATUS		stat;

	GLOBALREF char	iiUGempty[1];

	if ( string == NULL )
		return NULL;

	if ( *string == EOS )
		return iiUGempty;

	len = STlength(string) + 1;	/* ... + EOS */

	if ( (blk = (char *)FEreqmem(0, len, FALSE, &stat)) != NULL )
	{
		MEcopy( string, (u_i2)len, blk );
	}

	return blk;
}
