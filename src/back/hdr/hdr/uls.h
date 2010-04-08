/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ULS.H - External function protoypes for the ULS module
**
** Description:
**      This file contains the prototype for the ULS name checking module.
**
** History: 
**      01-sep-1992 (rog)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN bool uls_ownname( DD_CAPS *cap_ptr, i4  qmode, DB_LANG qlang,
			      char **quotechar );
