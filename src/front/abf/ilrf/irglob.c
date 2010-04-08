/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include "irstd.h"

/**
** Name:	irglob.c - ILRF global declarations
**
*/

GLOBALDEF QUEUE M_list = { 0 };	/* list of frames in memory */
GLOBALDEF QUEUE F_list = { 0 };	/* list of frames in temp file */
