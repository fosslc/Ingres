/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <abclass.h>
# include       <metafrm.h>

/**
** Name:	vqrenesc.c	-  Adjust escape code when menuitem text changes
**
** Description:
**	This file defines:
**
**	IIVQrmeRenameMenuEscapes	Adjust escape code for changed menu text
**
** History:
**	8/3/89 (Mike S)		Intiial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN STATUS	IIAMmfMetaFree();
FUNC_EXTERN STATUS	IIAMmpMetaPopulate();
FUNC_EXTERN STATUS	IIAMmuMetaUpdate();
/* static's */


/*{
** Name: IIVQrmeRenameMenuEscapes	Adjust escape code for changed menu text
**
** Description:
**	Escape code is attached to a menuitem via the menuitem text.
**	When this text changes, we have to change the MFESC strucutre as well.
**	
**
** Inputs:
**	metaframe	METAFRAME *	Current metaframe
**	oldname		char *		Old menu text
**	newframe	char *		New menu text
**
** Outputs:
**	none
**
**	Returns:
**		STATUS		OK unless something failed
**				FAIL otherwise
**
**	Exceptions:
**		none
**
** Side Effects:
**	The memory for the escape code structures is freed.  They were
**	probably not in memory in the first place.
**
** History:
**	8/3/89	(Mike S)	Initial version
*/
STATUS
IIVQrmeRenameMenuEscapes(metaframe, oldname, newname)
METAFRAME	*metaframe;
char		*oldname;
char		*newname;
{
	i4	changes = 0;	/* Number of changes made */
	STATUS	status = OK;	/* Assume success */
	i4	i;

	/* Load the escape sequences into memory, if need be */
	if ((metaframe->popmask & MF_P_ESC) == 0)
		if (IIAMmpMetaPopulate(metaframe, MF_P_ESC) != OK)
			return FAIL;

	/* Change the menuitem text for all matching escape code */
	for (i = 0; i < metaframe->numescs; i++)
	{
		register MFESC 	*escptr = metaframe->escs[i];
					/* Escape code pointer */
		register i4	type = escptr->type;
					/* Escape code type */

		if ((type == ESC_MENU_START) || (type == ESC_MENU_END))
			if (STcompare(escptr->item, oldname) == 0)
			{
				escptr->item = newname;
				changes++;
			}
	}

	/* If we made any changes, save it to the database */
	if (changes > 0)
		status = IIAMmuMetaUpdate(metaframe, MF_P_ESC);

	/* Free the memory */
	_VOID_ IIAMmfMetaFree(metaframe, MF_P_ESC);

	return (status);
}
