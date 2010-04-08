/*
** Copyright (c) 2004 Ingres Corporation
*/
# ifndef TARGTYPE_H_INCLUDED
# define TARGTYPE_H_INCLUDED
# include <compat.h>

/**
** Name:	targtype.h - Replicator target types
**
** Description:
**	Contains defines for the Replicator target types.
**
** History:
**	14-jan-97 (joea)
**		Created.
**/

# define TARG_FULL_PEER		1
# define TARG_PROT_READ		2
# define TARG_UNPROT_READ	3

# define VALID_TARGET_TYPE(t) \
		((t) >= TARG_FULL_PEER && (t) <= TARG_UNPROT_READ)


FUNC_EXTERN char *RMtarg_get_descr(i2 target_type);

# endif
