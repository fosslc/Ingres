/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	sca.h -- externs for sca functions
**
** Description:
**	FUNC_EXTERNS for sca.
**
**	2-Jul-1993 (daveb)
**	    created.
**/

FUNC_EXTERN DB_STATUS sca_add_datatypes(SCD_SCB *scb,
					PTR adf_svcb,
					i4 adf_size,
					i4 deallocate_flag,
					DB_ERROR *error,
					PTR *new_svcb,
					i4 *new_size );

FUNC_EXTERN i4  sca_check(i4 major_id, i4 minor_id);

# ifdef MAYBE_UNUSED
FUNC_EXTERN VOID sca_obtain_id( i4 *major_id, i4 *minor_id );
# endif


