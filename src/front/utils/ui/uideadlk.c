/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>


/*{
** Name:	IIUIdeadlock() 
**
** Description: returns TRUE if the error number passed in is a deadlock
**	error.
**
** 	useage-
**	for (i= 0; i < MAX_RETRIES; i++)
**	{
**		IIUIbeginXaction();
**			.
**			.
**			.
**		
**
**		EXEC SQL some database statement;
**		EXEC SQL INQUIRE_INGRES (:errno = errorno);
**		if (errno != 0)
**		{
**			IIUIabortXaction(); 
**			if (IIUIdeadlock(errno))
**				resume;
**			else
**				break;
**		}
**			.
**			.
**			.
**			.
**	}
**		
**
** Inputs:
**	i4	error_num;
**
** Returns:
**	{bool}  TRUE		// error_num is deadlock error
**		FALSE		// error_num is not deadlock error
**
** History:
**	13-apr-1989 (danielt) written
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


bool
IIUIdeadlock ( error_num )
i4	error_num;
{
	return (bool)
	    ( error_num == 4700 || error_num == 4706 || error_num == 49900 );
}
