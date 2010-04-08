/*
**Copyright (c) 1986, 2004 Ingres Corporation
** All rights reserved.
*/

#include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	"il.h"

/**
** Name:	iltab.c	 -	Intermediate Language Operator Look-up Table.
**
** Description:
**	Contains the IL operator look-up table for the OSL translator.  (The
**	table definition is in "hdr/iltab.h".  This version includes function
**	references.)  Defines:
**
**	iltab		IL interpreter table.
**
** History:
**	xx-xxx-86 (wong)
**		First written.
**
**	19-feb-87 (agh)
**		Added 2nd argument to il_func.
**		
*/

/*
** OSL Interpreter Version.
**
**	Note:  Define 'il_func()' to include function references for
**	OSL interpreter.
*/

# define il_func(x, y)	(x)

# include	<iltab.h>
