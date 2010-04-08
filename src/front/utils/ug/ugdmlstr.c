/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>

/**
** Name:	ugdmlstr.c -	Front-End DML String Mapping Module.
**
** Description:
**	Contains routines that map between the DB_LANG values for the DMLs
**	and their string representations.  Defines:
**
**	IIUGstrDml()	map string to DML.
**	IIUGdmlStr()	map DML to string.
**
** History:
**	Revision 6.2  89/01  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

static const
	char	_map[3][5] = {
			ERx("none"),
			ERx("QUEL"),
			ERx("SQL")
};

/*{
** Name:	IIUGstrDml() -	Map String to DML.
**
** Description:
**	Returns the DB_LANG value for the DML represented by the input string.
**
** Input:
**	dml	{char *}  The name of the DML.
**
** Returns:
**	{DB_LANG}  The internal DML value, DB_NOLANG on error.
**
** History:
**	89/01 (jhw)  Written.
*/
DB_LANG
IIUGstrDml ( dml )
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
		return DB_NOLANG;
}

/*{
** Name:	IIUGdmlStr() -	Map DML to String.
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
** History:
**	89/01 (jhw)  Written.
**	23-dec-1997 (fucch01)
**	    Had to cast _map as char * so the function type would match
**	    the return value type...
*/
char *
IIUGdmlStr ( dml )
DB_LANG	dml;
{
    return (char *)_map[( dml <= DB_NOLANG || dml > DB_SQL ) ? DB_NOLANG : dml];
}
