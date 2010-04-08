/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ilrffrm.h -	Interpreted Frame Object Run-time Facility
**				Frame Description File.
** Description:
**	Contains the structure definition for ILRF frame description.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Deleted member "current";
**		added members dbd, num_dbd, stacksize, static_data.
**
**	Revision 6.0  87/03  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*}
** Name:    FRMINFO -	ILRF Frame Description Structure.
**
** Description:
**	The structure that defines an interpreted frame object to the ILRF
**	module.
*/
typedef struct
{
	FDESC		*symtab;
	IL		*il;
	FRMALIGN	*align;
	IL_DB_VDESC	*dbd;
	i4		num_dbd;
	i4		stacksize;
	PTR		static_data;

} FRMINFO;
