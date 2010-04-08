/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<er.h>
#include	<uf.h>

/**
** Name:    catinit.c -	Object Module Catalog Initializtion.
**
** Description:
**	Contains a routine that initializes the catalog forms for
**	the object module.  Defines:
**
**	OOcatinit()	initialize catalog forms.
**
** History:
**	Revision 6.0  87/11  wong
**	Initial revision.
**/

/*{
** Name:    OOcatinit() -	Initialize Catalog Forms.
**
** Description:
**	Loads the forms for the catalog module of the objects module
**	from the forms index file.  The forms are "iicatalog", "iidetail",
**	and "iisave".
**
** History:
**	11/87 (jhw) -- Written.
*/

STATUS
OOcatinit ()
{
    register LOCATION	*ffile;

    ffile = IIUFlcfLocateForm();

    return (	IIUFgtfGetForm(ffile, ERx("iicatalog")) == OK &&
		IIUFgtfGetForm(ffile, ERx("iidetail")) == OK &&
		IIUFgtfGetForm(ffile, ERx("iisave")) == OK )
	? OK : FAIL;
}
