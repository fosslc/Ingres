/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef CDDS_H_INCLUDED
# define CDDS_H_INCLUDED
# include <compat.h>

/**
** Name:	cdds.h - CDDS information for RepServer
**
** Description:
**	CDDS information for RepServer.
**
** History:
**	16-dec-96 (joea)
**		Created based on cdds.sh in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.  Rename functions.
**/

/*
** Name:	RS_CDDS - CDDS information
**
** Description:
**	This structure holds information about a CDDS.
*/
typedef struct
{
	i2	cdds_no;
	i2	collision_mode;
	i2	error_mode;
} RS_CDDS;


FUNC_EXTERN STATUS RScdds_cache_init(void);
FUNC_EXTERN STATUS RScdds_lookup(i2 cdds_no, RS_CDDS **cdds_ptr);

# endif
