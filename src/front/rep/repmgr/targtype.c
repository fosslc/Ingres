/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <er.h>
# include <targtype.h>
# include "errm.h"

/**
** Name:	targtype.c - Replicator target types
**
** Description:
**	Defines
**		RMtarg_get_descr	- get target type description
**
** History:
**	14-jan-97 (joea)
**		Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	RMtarg_get_descr - get target type description
**
** Description:
**	Returns a target type description.
**
** Inputs:
**	target_type	- target type
**
** Outputs:
**	none
**
** Returns:
**	A pointer to a target type description string.
**/
char *
RMtarg_get_descr(
i2	target_type)
{
	switch ((i4)target_type)
	{
	case TARG_FULL_PEER:
		return (ERget(F_RM00A6_Full_peer));

	case TARG_PROT_READ:
		return (ERget(F_RM00A7_Prot_read));
		
	case TARG_UNPROT_READ:
		return (ERget(F_RM00A8_Unprot_read));
	}
				
	return (ERget(F_RM0074_Unknown_type));
}
