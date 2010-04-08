/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apige2ss.h
**
** Description:
**	Definitions for processing of generic error codes and SQLSTATE values.
**
** History:
**      02-nov-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

# ifndef __APIGE2SS_H__
# define __APIGE2SS_H__


/*
** Global functions
*/

II_EXTERN char	*IIapi_gen2IntSQLState( II_LONG generic, II_LONG local );

# endif /* __APIGE2SS_H__ */
