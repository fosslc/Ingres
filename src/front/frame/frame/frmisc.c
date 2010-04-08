# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	<cm.h>

/*
**	frmisc.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	frmisc.c - Miscellaneous routines.
**
** Description:
**	Miscellaneous routines used by the forms system are located here.
**	- FDfind - Find an element in an array of elements.
**
** History:
**	2/26/85	(bab) moved FDGFName() and FDGCName() into nmretrv.c
**	02/15/87 (dkh) - Added header.
**	09/11/87 (dkh) - Code cleanup.
**	11-sep-87 (bab)
**		Changed type of 'arr' parameter from char** to nat**;
**		'cp' variable likewise.  (DG considers both struct*'s
**		and nat*'s to be word pointers, whereas char*'s are
**		byte pointers; arr is really some type of a struct**.)
**	12/23/87 (dkh) - Performance changes.
**	02/26/88 (dkh) - Fixed to pass correct arg to CMcmpcase.
**	12/31/88 (dkh) - Perf changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN	i4	IIFDcncCreateNameChecksum();


/*{
** Name:	FDfind - Find an element in an array of elements.
**
** Description:
**	This is a routine which is used to find an element in an
**	array of elements.  The array must be an array of pointers.
**	The user must pass a routine which grabs a name given the pointer.
**
** Inputs:
**	arr	Array of elements to look in.
**	name	Name of element to look for in array.
**	number	Number of elements in the array.
**	routine	Routine to extract the name of the element.
**
** Outputs:
**	Returns:
**		ptr	Pointer to element that was found.  NULL if
**			element not found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
FIELD *
FDfind(arr, name, number, routine)
reg i4		*arr[];
reg char	*name;
reg i4		number;
reg FLDHDR	*(*routine)();
{
	reg i4		i;
	reg i4		**cp;
	char		buf[FE_MAXNAME + 1];
	i4		checksum;
	FLDHDR		*hdr;

	STcopy(name, buf);
	CVlower(buf);

	checksum = IIFDcncCreateNameChecksum(buf);

	/*
	**  Field names should have been lowercased already.
	**  If users muck around in the catalogs by hand, too bad.
	*/
	for (i = 0, cp = arr; i < number; i++, cp++)
	{
		hdr = (*routine)(*cp);
		if (checksum != hdr->fhdfont)
		{
			continue;
		}
		if (STcompare(buf, hdr->fhdname) == 0)
		{
			return ((FIELD *)*cp);
		}
	}
	return (NULL);
}
