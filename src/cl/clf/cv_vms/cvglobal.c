/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>

/**
** Name: CVGLOBAL.C - Global variables for CV.
**
** Description:
**	Allocate storage for global variables in cv module.
**
**      13-Jan-2010 (wanfr01) Bug 123139
**          Created
**/

GLOBALDEF CV_FUNCTIONS CV_fvp = { CVal_DB, CVal8_DB, CVlower_DB, CVupper_DB };


