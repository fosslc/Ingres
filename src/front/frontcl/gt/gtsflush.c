/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include <si.h>

/**
** Name:	gtsflush.c -	Graphics System Alpha Terminal Flush
**
** Description:
**	This module handles flush of standard out and standard error
**	from GT lib calls.  It defines:
**
**	GTsflush()	flush standard out and standard error.
**
** History:
**		Revision 40.102  86/04/16  21:15:40  wong
**		Added flush of standard error.
**		
**		Revision 40.1  86/02/21  13:00:12  bobm
**		Initial revision.
**/

/*	SIflush cannot be used in files that include gks.h
*/

VOID
GTsflush()
{
	SIflush(stderr);	/* flush standard error first,
				**	since it should be unbuffered.
				*/
	SIflush(stdout);
}
