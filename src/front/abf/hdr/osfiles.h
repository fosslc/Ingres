/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    osfiles.h -		OSL Parser File Declarations File.
**
** Description:
**	Contains the declarations for the OSL parser file variables.
**	This includes the names of the files as well as the file variables.
**
** History:
**	Revsion 6.2  89/01  billc
**	Replaced 'osOfilename' for OSL/CodeGen merge.
**	Added 'osLfilename'.  03/89  wong.
**
**	Revision 6.0  87/07  wong
**	Added 'osErrfile' and remove 'osOfilename'.  07/87  wong.
**	Removed 'osRfilname' and 'osPfilename'.  03/87 wong.
**	Moved from "osglobs.h".  02/87 wong.
**/

/*
** FILES
*/
GLOBALREF char	*osIfilname;
GLOBALREF char	*osOfilname;
GLOBALREF char	*osLfilname;
GLOBALREF FILE	*osIfile;
GLOBALREF FILE	*osOfile;
GLOBALREF FILE	*osLisfile;
GLOBALREF FILE	*osErrfile;
