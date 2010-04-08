/*
**	newfrmc.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	newfrm.c - Allocate a new FRAME structure.
**
** Description:
**	Routines that allocate new FRAME structures and subcomponents
**	of FRAME structures are defined here.
**	- FDnewfrm - Allocates the FRAME structure.
**	- FDfralloc - Allocate subcomponents of the FRAME structure.
**
** History:
**	JEN - 1 Nov 1981
**	12-20-84 (rlk) - Replaced ME*alloc calls with FE*alloc calls.
**	02/05/87 (dkh) - Added support for ADTs.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	02/20/90 (dkh) - Added initialization of FRAME structure member
**			 "frversion" to FRMVERSION in FDnewfrm().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	NEWFRM.c  -  Allocate space for a new frame
**	
**	Copyright (c) 2004 Ingres Corporation
**
**	These routines allocate space for a frame structs and windows.
**	
**	Fdnewfrm()  -  Allocate the frame struct
**	Arguments:  name  -  name of frame to allocate
**		    frid  -  frame identification number
**	
**	Fdfralloc()  -	Allocate pointers and windows for frame
**	Arguments:  frm	 -  frame pointer
**	
**	
*/


# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<si.h>
# include	<er.h>



/*{
** Name:	FDnewfrm - Allocate a FRAME structure.
**
** Description:
**	Allocate a new FRAME structure and copy the passed in
**	name into the "frname" member of the structure.
**
** Inputs:
**	name	Name to assign for the allocated FRAME structure.
**
** Outputs:
**	Returns:
**		ptr	Pointer to allocated FRAME structure.
**		NULL	If allocation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Some implementations may "syserr" if allocation was
**	not possible.
**
** History:
**	02/05/87 (dkh) - Added support for ADTs.
*/
FRAME *
FDnewfrm(name)				/* FDNEWFRM: */
reg char	*name;			/* name of the frame to create */
{
	reg FRAME	*frm;			/* frame pointer */

	/* allocate frame struct */
	if ((frm = (FRAME *)FEreqmem((u_i4)0, (u_i4)(sizeof(FRAME)), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("FDnewfrm"));
		return(NULL);
	}
	STcopy(name, frm->frname);
	frm->frversion = FRMVERSION;
	return(frm);
}




/*{
** Name:	FDfralloc - Allocate subcomponents for FRAME structure.
**
** Description:
**	Allocate the field pointer and trim pointer arrays for a
**	FRAME structure.  It is assumed that appropriate field number
**	and trim numbers have been filled in before calling this
**	routine.  May be called by VIFRED/QBF for default form building,
**	etc.
**
** Inputs:
**	frm	Pointer to FRAME structure for subcomponent allocation.
**
** Outputs:
**	Returns:
**		TRUE	If allocation was successful.
**		FALSE	If allocation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Some implementations may "syserr" if allocation failed.
**
** History:
**	02/05/87 (dkh) - Added support for ADTs.
*/
i4
FDfralloc(frm)				/* FDFRALLOC: */
reg FRAME	*frm;			/* pointer to frame */
{
	i4	status;

	frm->frfld = (FIELD **)FEreqmem((u_i4)0,
	    (u_i4)(frm->frfldno * sizeof(FIELD *)), TRUE, (STATUS *)&status);
	/* allocate array of field pointers */
	if(frm->frfldno != 0 && status != OK)
	{
		IIUGbmaBadMemoryAllocation(ERx("FDfralloc-1"));
		return(FALSE);
	}

	frm->frnsfld = (FIELD **)FEreqmem((u_i4)0,
	    (u_i4)(frm->frnsno * sizeof(FIELD *)), TRUE, (STATUS *)&status);
	/* allocate array of non-seq field pointers */
	if(frm->frnsno != 0 && status != OK)
	{
		IIUGbmaBadMemoryAllocation(ERx("FDfralloc-2"));
		return(FALSE);
	}

	frm->frtrim = (TRIM **)FEreqmem((u_i4)0,
	    (u_i4)(frm->frtrimno * sizeof(TRIM *)), TRUE, (STATUS *)&status);
	/* allocate array of non-seq field pointers */
	if(frm->frtrimno != 0 && status != OK)
	{
		IIUGbmaBadMemoryAllocation(ERx("FDfralloc-3"));
		return(FALSE);
	}

	return (TRUE);
}
