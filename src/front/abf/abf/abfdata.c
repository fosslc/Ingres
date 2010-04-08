/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h> 
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>

/**
** Name:    abfdata.c -	ABF Global Data Definitions File.
**
** Description:
**	Contains the definitions for the global data of ABF.
**
** History:
**	08/16/91 (emerson)
**		Added IIabUserName and IIabConnect (for bug 37878).
**	25-Aug-2009 (kschendel) 121804
**	    Get rid of abf header includes, none are needed here.
**/

/*
** FLAGS
*/
GLOBALDEF bool	abFnwarn = TRUE;	/* warnings on system functions */
GLOBALDEF bool	IIabAbf = FALSE;	/* -v option */
GLOBALDEF bool	IIabWopt = FALSE;	/* -w option */
GLOBALDEF bool	IIabVision = FALSE;	/* -v option */
GLOBALDEF bool	IIabOpen = FALSE;	/* +wopen option */
GLOBALDEF bool	IIabC50 = FALSE;	/* -5.0 option */

GLOBALDEF char	*IIabGroupID	= ERx("");	/* Group ID */
GLOBALDEF char	*IIabRoleID	= ERx("");	/* Role ID and password */
GLOBALDEF char	*IIabPassword	= ERx("");	/* User Password */
GLOBALDEF char	*IIabUserName	= ERx("");	/* User Name (-u flag) */
GLOBALDEF char	*IIabConnect	= ERx("");	/* WITH clause for database
						** connect (+c flag) */

/* executable name for screen titles */
GLOBALDEF char	*IIabExename = "ABF";
