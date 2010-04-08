/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<erar.h>
#include	<ui.h>

/**
** Name:	abrtskey.sc -	ABF Run-Time Surrogate Key Function.
**
** Description:
**	Contains the routine used to implement the surrogate key system
**	function.  This function returns a surrogate key.  Define:
**
**	IIARsurrogate_key()	return surrogate key for table.
**
** History:
**	Revision 8.0  89/06  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Need ui.h to satisfy gcc 4.3.
**/

/*{
** Name:	IIARsurrogate_key() -	Return Surrogate Key for Table.
**
** Description:
**	Returns a surrogate key for a table.  This key is an integer in the
**	range of 2^31 - 1 to -2^31.  The zero key is the error key and will
**	be returned on error or when no more keys can be allocated.
**
**	This function calls a database procedure if the DBMS is INGRES.
**	Otherwise, it executes the appropriate queries directly.
**
**	The UPDATE detects that the row exists, that another key can be
**	allocated, and whether the table exists.  If this fails, then
**	either the table does not exist or no more keys can be allocated.
**	If the row for the table does not exist, but the table does exist,
**	then the row will be added to the surrogate key table.
**
**	This procedure does not account for tables that exist, but which
**	do not have INSERT or UPDATE priveleges granted.
**
** Input:
**	table_name	{char *}  The name of the table.
**
** Returns:
**	{i4}	0 on error, otherwise, the key number.
**
** History:
**	06/89 (jhw) -- Written.
**	07-oct-1993 (mgw)
**		Added <fe.h> include because of new TAGID dependency on it
**		in <ug.h>
*/

#define E_US0845_2117	0x845

i4
IIARsurrogate_key ( table_name, test )
EXEC SQL BEGIN DECLARE SECTION;
char	*table_name;
EXEC SQL END DECLARE SECTION;
bool	test;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	key;
	EXEC SQL END DECLARE SECTION;

#ifndef DEBUG
#define test	TRUE
#endif

	iiuicw1_CatWriteOn();
	if ( test && IIUIdcn_ingres() )
	{ /* database procedure */
		exec sql execute procedure ii_surrogate_key
			( table_name = :table_name ) into :key;
		if ( key == 0 && FEinqerr() == OK )
		{ /* not found or key wrapped */
			exec sql select surrogate_key
					into :key from ii_surrogate_key
				where table_name in
					(select table_name from iitables
						where table_name = :table_name
					);
			if ( FEinqerr() == OK && FEinqrows() == 0 )
			{ /* definitely not found */
				IIUGerr( E_US0845_2117, UG_ERR_ERROR,
						1, (PTR)table_name
				);
			}
			return 0;
		}
	}
	else
	{ /* in-line */
		exec sql repeated
			update ii_surrogate_key
				set surrogate_key = surrogate_key + 1
				where surrogate_key <> 0 and table_name in
					(select table_name from iitables
						where table_name = :table_name
					);
		if ( FEinqerr() != OK )
			return 0;
		else if ( FEinqrows() == 0 )
		{ /* check for table existence */
			EXEC SQL BEGIN DECLARE SECTION;
			i4	_exists;
			EXEC SQL END DECLARE SECTION;

			exec sql select count(*) into :_exists from iitables
				where table_name = :table_name;
			if ( FEinqerr() != OK || _exists == 0 )
			{ /* table does not exist */
				IIUGerr( E_US0845_2117, UG_ERR_ERROR,
						1, (PTR)table_name
				);
				return 0;
			}
			exec sql select surrogate_key
					into :key from ii_surrogate_key
				where table_name = :table_name;
			if ( FEinqerr() != OK ||
					( FEinqrows() != 0 && key == 0 ) )
			{ /* no more keys */
				return 0;
			}
			/* Add row for table */
			exec sql insert into ii_surrogate_key
					values ( :table_name, 1 );
			if ( FEinqerr() != OK || FEinqrows() == 0 )
				return 0;
		}
		exec sql repeated select surrogate_key
					into :key from ii_surrogate_key
				where table_name = :table_name;
		if ( FEinqerr() != OK || FEinqrows() == 0 )
			return 0;
	}
	iiuicw0_CatWriteOff();
	return key;
}
