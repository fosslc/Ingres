/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include 	<si.h>
#include 	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>

/**
** Name:    feafeerr.c -	Print ADF/AFE Error Message Routine.
**
** Description:
**	This file contains and defines:
**
**	FEafeerr	ouput an afe error message.
**
** History:
**	Written 1/26/87 (dpr)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

i4	aferrtostr();
VOID 	FEmsg();

/*{
** Name:    FEafeerr() -	Output an afe error message contained in
**				an ADF_CB.
**
** Description:
**	Given an ADF_CB, this routine extracts the ADF, AFE, or FMT 
**	error number and error message contained in the ADF_CB. It then
**	prints the error number and message using the standard frontend
**	error reporting conventions. This routine should be called to
**	report ADF, AFE, or FMT errors.
**
** Inputs:
**	cb		    pointer to an ADF_CB
**	  .adf_errcb	      the ADF_ERROR structure  
**	    .ad_errcode		afe error code for the error
**	    .ad_emsglen		the length, in bytes, of the error message
**	    .ad_errmsgp		pointer to the formatted error message
**	
** History:
**	Written 1/26/87 (dpr)
**	29-08-1988 (elein)
**		Changed call to IIUGepp to use stderr instead of stdout
*/
VOID
FEafeerr(cb)
ADF_CB	*cb;
{
    i4		rval;			/* holder of return value */
    i4		length;
    char	buf[ER_MAX_LEN+1];

    length = ER_MAX_LEN+1;

    rval = afe_errtostr(cb, buf, &length); 	/* just to be kosher */

    IIUGeppErrorPrettyPrint(stdout, buf, FALSE);
}
