/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abclass.h>
# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<frame.h>
# include	<oocat.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<st.h>
# include	<cm.h>
# include	<cv.h>
# include	<flddesc.h>

# include	<metafrm.h>

# include	"ervq.h"
# include	"vqloc.h"


/**
** Name:	vqglob.c -	allow editing of globals
**
** Description:
**	This file has routines to allow selecting various 
**	global definitions for edit.
**
**	This file defines:
**
**	IIVQgeGlobalEdit - implement list pick to select global to edit
**
** History:
**	09/29/89 (tom) - created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


FUNC_EXTERN STATUS IIABcaCatalogEdit(); 
FUNC_EXTERN VOID IIVQlocals(); 
FUNC_EXTERN i4  IIFDlpListPick();

# define _vqglob_prmpt	ERget(F_VQ00A4_vqglob_prmpt)
# define _vqglob_opt	ERget(F_VQ00A5_vqglob_opt)
# define _vqloc_glob_prmpt	ERget(F_VQ00A7_vqloc_glob_prmpt)
# define _vqloc_glob_opt	ERget(F_VQ00A6_vqloc_glob_opt)


/*{
** Name:	IIVQgeGlobalEdit	- implement global edit
**
** Description:
**	Present popup allowing user to select global to be edited.
**
** Inputs:
**	APPL	*app;	- application we are working with
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	09/29/89 (tom) - 
*/
VOID
IIVQgeGlobalEdit(app, noload)
APPL	*app;
bool	noload;
{

	i4 type;

	switch (
		IIFDlpListPick(_vqglob_prmpt, _vqglob_opt, 3, 
			-1, -1, _vqglob_prmpt, ERx("vqedglob.hlp"),
			(i4 (*)())NULL, (PTR)NULL)
		)
	{
	case 0:
		type = OC_GLOBAL;
		break;

	case 1:
		type = OC_CONST;
		break;

	case 2:
		type = OC_RECORD;
		break;

	default:
		return;
	}

	IIABcaCatalogEdit( app, type, noload, FALSE);
}


/*{
** Name:	IIVQlgLocGlobEdit	- implement global edit
**
** Description:
**	Present popup allowing user to select either local or global
**	items to be edited.
**
** Inputs:
**	APPL	*app;	- application we are working with
**
** Outputs:
**
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	17-jan-90 (kenl)
**		Created.
*/
VOID
IIVQlgLocGlobEdit(app, noload, apobj)
APPL		*app;
bool		noload;
OO_OBJECT       *apobj;
{

	i4 type;

	switch (
		IIFDlpListPick(_vqloc_glob_prmpt, _vqloc_glob_opt, 4, 
			-1, -1, _vqloc_glob_prmpt, ERx("vqedlogl.hlp"),
			(i4 (*)())NULL, (PTR)NULL)
		)
	{
	case 0:
		IIVQlocals(apobj);
		return;

	case 1:
		type = OC_GLOBAL;
		break;

	case 2:
		type = OC_CONST;
		break;

	case 3:
		type = OC_RECORD;
		break;

	default:
		return;
	}

	IIABcaCatalogEdit( app, type, noload, FALSE);
}
