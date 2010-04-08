/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<eraf.h>
#include	<fe.h>

/**
** Name:	afdmlstr.c -	Front-End DML String Mapping Module.
**
** Description:
**	Contains routines that map between the DB_LANG values for the DMLs
**	and their string representations.  Defines:
**
**	IIAFstrDml()	map string to DML.
**	IIAFdmlStr()	map DML to string.
**
** History:
**	Revision 8.0  89/08  wong
**	Moved to AFE from UG and added error reporting and control block
**	parameter.
**
**	Revision 6.2  89/01  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
**/

static char	*_map[3] = {
			ERx("none"),
			ERx("QUEL"),
			ERx("SQL")
};

/*{
** Name:	IIAFstrDml() -	Map String to DML.
**
** Description:
**	Returns the DB_LANG value for the DML represented by the input string.
**
** Input:
**	cb	{ADF_CB *}  The ADF control block.
**	dml	{char *}  The name of the DML.
**
** Returns:
**	{DB_LANG}  The internal DML value, DB_NOLANG on error.
**
** Errors:
**	E_AF2000_BadDMLStr	The name specified for the query language was
**				not recognized.
**
** History:
**	89/01 (jhw)  Written.
**	89/08 (jhw)  Added error message and ADF_CB parameter.
*/
DB_LANG
IIAFstrDml ( cb, dml )
ADF_CB	*cb;
char	*dml;
{
	char	buf[16];

	_VOID_ STlcopy( dml, buf, (u_i4)sizeof(buf)-1 );
	CVupper(buf);

	if ( STequal(buf, _map[DB_SQL]) )
		return DB_SQL;
	else if ( STequal(buf, _map[DB_QUEL]) )
		return DB_QUEL;
	else
	{
		_VOID_ afe_error(cb, E_AF2000_BadDMLStr, 1, dml);
		return DB_NOLANG;
	}
}

/*{
** Name:	IIAFdmlStr() -	Map DML to String.
**
** Description:
**	Return string representation for the DB_LANG DML value.
**
** Input:
**	dml	{DB_LANG}  Value for DML.
**
** Returns:
**	{char *}  Reference to string representation, "none" on error.
**
** Errors:
**	E_AD1007_BAD_QLANG	(Internal) Illegal query language type.
**
** History:
**	89/01 (jhw)  Written.
**	89/08 (jhw)  Added error message.
*/
char *
IIAFdmlStr ( dml )
DB_LANG	dml;
{
	if ( dml <= DB_NOLANG || dml > DB_SQL )
	{
		i4	type = dml;

		IIUGerr(E_AD1007_BAD_QLANG, 0, 1, (PTR)&type);
	}
	return _map[( dml <= DB_NOLANG || dml > DB_SQL ) ? DB_NOLANG : dml];
}
