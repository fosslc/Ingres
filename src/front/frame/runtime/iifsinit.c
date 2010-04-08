/*	iifsinit.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 
# include	<me.h>
# include	<er.h>

/**
** Name:	iifsinit.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIfsinit()
**
**	Private (static) routines defined:
**
** History:
**	6-may-1983   -	Extracted from John's IIfrminit.c (ncg)
**	5-jan-1987   -  Changed calls to IIbadmem. (prs)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	08/11/91 (dkh) - Added changes so that all memory allocated
**			 for a dynamically created form is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    gcc 4.3 update prototypes.
**/


/*{
** Name:	IIfsinit	-	Alloc and initialize FRS struct
**
** Description:
**	Allocate the data structure that hold FRS variables and then
**	call IIfrsvars to initialize it's contents.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	Ptr to the initialized FRSDATA structure.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

FRSDATA	*
IIfsinit(u_i4 tag)
{
	FRSDATA	*frs_ptr;

	if ((frs_ptr = (FRSDATA *)MEreqmem(tag, (u_i4)sizeof(FRSDATA),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIfsinit"));
	}

	/* no user variable addresses yet */
	IIfrsvars(frs_ptr);
	return (frs_ptr);
}
