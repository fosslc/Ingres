/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<uf.h>
#include	<ug.h>

/**
** Name:	abstatus.c -	Report ABF Build Status Routine.
**
** Description:
**	Contains a routine used to display status messages for the various
**	steps required to build an ABF application.  Defines:
**
**	iiabStatusMsg()	print a build status message.
**
** History:
**	Revision 6.2  89/02  wong
**	Modified to be passed message ID and renamed from 'abstamsg()'.
**
**	Revision 2.0  82/10/14  joe
**	Initial revision.
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
*/

/*{
** Name:	iiabStatusMsg() -	Print a Build Status Message.
**
** Description:
**	Print a status message according to the parameters.
**
** Input:
**	msgid	{ER_MSGID}  The message ID to be output.
**	name	{char *}  The name of the object being built.
**
** History:
**	Written 10/14/82 (jrc)
**	02/89 (jhw)   Changed to be passed message ID and renamed.
*/
VOID
iiabStatusMsg ( msgid, name )
ER_MSGID	 msgid;
char		*name;
{
	char buf[240];

	IIUFrtmRestoreTerminalMode(IIUFNORMAL);
	IIUGfmt( buf, 240, ERget(msgid), 1, name);
	SIfprintf(stderr, ERx("%s\r\n"), buf);
	SIflush(stderr);
}
