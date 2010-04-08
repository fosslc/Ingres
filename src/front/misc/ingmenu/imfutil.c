/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uf.h>
#include	<ug.h>
#include	"erim.h"

/**
** Name:	imfutil.c -	IngMenu Forms Initialization.
**
**	Defines:
**
**	iiimAddForm()
**
** History:
**	Revision 6.2  89/03  wong
**	Added form name parameter and renamed from 'swaddforms()'.
**
**	Revision 6.0  87/09/01  peter
**	Add indexed form capabilities.
**
**	Revision 3.0  84/04 stevel
**	Initial revision.
**/

VOID
iiimAddForm ( form )
char	*form;
{
	LOCATION	*IIUFlcfLocateForm();

	if ( IIUFgtfGetForm(IIUFlcfLocateForm(), form) != OK )
	{
		IIUGerr(E_IM0004_Cannot_find_form, UG_ERR_FATAL,
			1, (PTR) form
		);
		/*NOT REACHED*/
	}
}
