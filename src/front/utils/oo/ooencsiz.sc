/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
exec sql include	sqlda;

/**
** Name:	ooencsiz.sc -	Return Encodings Catalog Size Routine.
**
** Description:
**	Contains the routine that returns the size of the encodings catalog
**	for a system.  Defines:
**
**	IIOOencodedSize()	return encodings catalog size.
**
** History:
**	Revision 6.4  90/01  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIOOencodedSize()	Return Encodings Catalog Size.
**
** Description:
**	Returns the column size of the data column in the Front-End encodings
**	catalog, "ii_encodings".  It does so by using a PREPARE to query the
**	DBMS.
**
**	The size of the data column may differ depending on the system, in
**	particular, B1 INGRES is smaller.
**
** Returns:
**	{nat}  The size of the data column.
**
** History:
**	01/90 (jhw) -- Written.
*/

static i4	_size = 0;

i4
IIOOencodedSize()
{
	if ( _size == 0 )
	{
		IISQLDA_TYPE(oo_, ooda, 10);

		ooda.sqln = 10;
		exec sql prepare iioo_size into :&ooda using names
			from 'select encode_estring from ii_encodings';
		if (ooda.sqld == 1)
		{
			_size = ooda.sqlvar[0].sqllen;
		}
	}

	return _size;
}
