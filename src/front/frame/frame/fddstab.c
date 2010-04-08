/*
**	static	char	Sccsid[] = "@(#)fddstab.c	30.1	2/4/85";
*/

/*
** fddstab.c
**
**	Template table used by DS routines on FRAME structure.
**	These routines are called in the process of writing/reading
**	a compiled form to/from the database.
**	This file is similar to vifred/vfdstab.c.
**
**	Copyright (c) 2004 Ingres Corporation
**
**	History:
**		Written.  1/16/85 (agh)
**	06/20/87 (dkh) - Deleted unncessary references.
**	09/05/90 (dkh) - Added IIFDgfdGetFdDstab() to avoid exporting
**			 the reference to FdDStab.
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**      24-sep-96 (hanch04)
**          Global data moved to framdata.c
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ds.h>
# include	<feds.h>

GLOBALREF DSTEMPLATE	DSframe,
			DSfield,
			DSregfld,
			DSpregfld,
			DStblfld,
			DSptblfld,
			DSfldhdr,
			DSfldtype,
			DSfldval,
			DSfldcol,
			DStrim,
			DSi4,
			DSf8,
			DSchar,
			DSstring;

GLOBALREF DSTEMPLATE *DSTab[] ; 
GLOBALREF DSTARRAY FdDStab ;


/*{
** Name:	IIFDgfdGetFdDstab - Return handle to FdDStab.
**
** Description:
**	This routine simply returns the address of FdDStab to the
**	caller so we don't have to export the address of FdDStab
**	directly.
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		ptr	Pointer to FdDStab.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/05/90 (dkh) - Initial version.
*/
DSTARRAY *
IIFDgfdGetFdDstab()
{
	return(&FdDStab);
}
