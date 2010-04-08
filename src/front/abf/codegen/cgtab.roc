/*
** Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<iltypes.h>
# include	<ilops.h>

/**
** Name:	cgtab.c	 -	Intermediate Language Operator Look-up Table.
**
** Description:
**	Contains the IL operator look-up table for the code generator.  (The
**	table definition is in "hdr/iltab.h".  This version includes function
**	references.)  Defines:
**
**	iltab		code generator table.
*/

/*
** Code generator version.
**
**	Note:	Define 'il_func()' to include function references for
**		code generator.
*/

# define il_func(x, y)	(y)

# ifdef DGC_AOS
# define IIILtab IIdgILtab
# endif

# include	<iltab.h>

