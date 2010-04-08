/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<ereq.h>
#include	<equel.h>
#include	<eqstmt.h>
#include	<eqrun.h>
#include	<eqfrs.h>
#include	<fsicnsts.h>

/**
** Name:    osfrs.h -	OSL Parser Set/Inquire FRS Module Interface
**					Definitions.
** Description:
**	This file defines the interface for the OSL parser set/inquire FRS
**	module.  It does so by including the files that define the set/inquire
**	FRS module for EQUEL (above.)
**
** History:
**	Revision 6.0  87/26/04  wong
**	Added 'osfrs_basetype()' as macro.
**
**	Revision 5.1  86/10/08  10:36:03  wong
**	Initial revision.
**
**      27-Jun-95 (tutto01)
**          Redefined the function calls frs_head and frs_error for DESKTOP
**          platforms.  Multiply defined symbols existing between eqgenfrs.c
**          and osfrs.c.
*/

/*
** Name:    osfrs_basetype() -	Return Base FRS Type for DB Datatype.
**
** Input:
**	type	{DB_DT_ID}  DB datatype.
**
** Returns:
**	{DB_DT_ID}  Base FRS type.
**
** History:
**	04/87 (jhw) - Written.
*/
#define osfrs_basetype(type) (oschkstr(type) ? DB_CHR_TYPE : (type < 0 ? -type : type))

/* Function Declarations */

VOID	frs_gen();
# if defined(DESKTOP)
# define frs_head   osfrs_head
# define frs_error  osfrs_error
# endif
VOID	frs_head();
VOID	frs_iqset();
VOID	frs_error();
OSNODE	*osfrs_constant();
ILREF	osfrs_setval();
bool	osfrs_old();

char	*frsck_mode();
