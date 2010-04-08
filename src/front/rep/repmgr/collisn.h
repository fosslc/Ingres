/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef COLLISN_H_INCLUDED
# define COLLISN_H_INCLUDED

# include <compat.h>

/**
** Name:	collisn.h - collision structures
**
** History:
**	16-dec-96 (joea)
**		Created based on structures in tbldef.sh in replicator library.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

# define TX_INSERT	1
# define TX_UPDATE	2
# define TX_DELETE	3


typedef struct
{
	i2	sourcedb;
	i4	transaction_id;
	i4	sequence_no;
	i4	table_no;
} COLLISION;

typedef struct _collisions_found
{
	i2	local_db;
	i2	remote_db;
	i4	type;		/* INSERT, UPDATE, DELETE */
	bool	resolved;
	COLLISION	db1;
	COLLISION	db2;
} COLLISION_FOUND;

FUNC_EXTERN STATUS      queue_collision(i2 localdb, char *filename);
# endif
