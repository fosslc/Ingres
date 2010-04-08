/*
** Copyright (c) 1994 Ingres Corporation
*/

#include	<sacl.h>

/*
** Name: SA.H - defines SA function externs
**
** Description:
**	contains SA function externs
**
** History:
**	7-jan-94 (stephenb)
**	    Initial creation.
**	11-feb-94 (stephenb)
**	    Updated in line with current spec.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN bool 
SAsupports_osaudit();

FUNC_EXTERN STATUS
SAopen( 
    char		*aud_desc, 
    i4		flags, 
    PTR			*aud_trail_d, 
    CL_ERR_DESC		*err_code);

FUNC_EXTERN STATUS
SAclose(PTR		*aud_trail_d,
	CL_ERR_DESC	*err_code);

FUNC_EXTERN STATUS
SAwrite(PTR		*aud_trail_d,
	SA_AUD_REC	*aud_rec,
	CL_ERR_DESC	*err_code);

FUNC_EXTERN STATUS
SAflush(PTR		*aud_trail_d,
	CL_ERR_DESC	*err_code);
