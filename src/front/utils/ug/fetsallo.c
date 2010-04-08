/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	"feloc.h"	/* includes <me.h> */

/**
** Name:	fetsallo.c -	Allocate String in Memory.
**
** Description:
**	Contains a routine that allocates a string in the current block of
**	tagged memory.  Defines:
**
**	FEtsalloc()	allocate string in memory.
**
** History:
**	Revision 6.2  89/05/02  gordonw
**	Replace 'MEcopy()' with MECOPY macro call.
**
**	Revision 6.0  1987-aug-20  bruceb
**	Modified to use 'iiugReqMem()'.
**
**	Revision 3.0  1985-jan-16  ron
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	FEtsalloc() -	Allocate String in Tagged Memory.
**
** Description:
**	Allocates and returns a reference to a block of memory containing the
**	passed in string.  This block of memory has been tagged with the tag-ID
**	specified by the input parameter.
**
**	The FE's memory management scheme is totally different from that used
**	in the ME routines.  Routines such as 'MEsize()' are not compatable
**	and 'FEfree()' must be called to free the storage.
**
**	This routine is used in conjunction with 'FEbegintag()', 'FEendtag()',
**	and 'FEfree()'.
**
** Input:
**	tag	{TAGID}  The tag for the memory region.
**	string	{char *}  An EOS-terminated string.
**
** Returns:
**	{char *}  The allocated string.  (NULL is returned on error.)
**
** History:
**	1/16/85 (rlk) - First Written
**	20-aug-87 (bab)	Modified to use 'iiugReqMem()'.
**	7/93 (Mike S) Use FEreqlng
**	10-dec-93 (rudyw)
**		The length of the input string may be 'nat' byte size, but
**		the copy macro cast the length to a 'u_i2' causing a problem
**		any time the length was greater than 64K. Added a loop to 
**		break up the copy process into pieces when needed. Bug 43439.
**      08-Dec-95 (fanra01)
**          Changed all externs to GLOBALREFs
*/

char *
FEtsalloc ( u_i2 tag, char *string )
{
	register char	*bptr;
	register char	*sptr;
	char		*blk;
	u_i4		len;
	u_i4		cplen;
	STATUS		stat;

	GLOBALREF char	iiUGempty[1];

	if ( string == NULL )
		return NULL;

	if ( *string == EOS )
		return iiUGempty;

	len = STlength(string) + 1;	/* ... + EOS */
		
	if ( (blk = (char *)FEreqlng((u_i4)tag, (u_i4)len, FALSE,
					&stat)) != NULL )
	{
		/*
		** Memory has been successfully allocated and the copy to
		** it will be done in pieces smaller than the limit of a
		** 'u_i2' to accomodate the underlying MEcopy routine.
		*/
		for ( sptr=string, bptr=blk; len>0;
				 len=len-cplen, sptr+=cplen, bptr+=cplen)
		{
			cplen = min(len,64000);
			MECOPY_VAR_MACRO( sptr, (u_i2)cplen, bptr );
		}
	}

	return blk;
}
