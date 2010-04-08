/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abfdbcat.h>
#include	"iamint.h"

/**
** Name:	iamglob.c - global variables
**
** Description:
**	AOM global variables - there are globaldef's in some other
**	source files as well, to avoid unneeded linkages.
*/

GLOBALDEF bool Xact = TRUE;		/* transaction state */
GLOBALDEF i4  Cvers = ACAT_VERSION;	/* version stamp */
GLOBALDEF bool Timestamp = TRUE;	/* handle timestamps */
GLOBALDEF char *Catowner = NULL;	/* alternate owner */
