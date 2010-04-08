/*
** Copyright (c) 1984, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<lo.h> 
#include	<nm.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:	fetopen.c -	Opens a Temporary Variable Length Binary File.
**
** Description:
**	Contains a routine to open a variable length binary file in the
**	temporary directory.  Defines:
**
**	FEt_open_bin()	open a temporary variable length binary file.
**
** History:
**	Revision 6.2  89/05  wong
**	Corrected to get TEMP path before making a unique name.  Bug #14810.
**
**	Revision 6.0  87/08/20  peterk
**	Updated for 6.0 CL changes.
**
**	Revision 2.0  84/05/25  nadene
**	Initial revision.
**
**	4/21/92 (seg)
**	    Fixed incorrect declaration of paramter.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**/

/*
** Name:	FEt_open_bin() -	Open a Temporary Variable Length File.
**
** Inputs:
**	mode	{char *}  mode to open file, legal modes are "w", "r", "a".
**	     name   -  	name of file
**	     suffix -	suffix (possibly NULL) for file
**	     loc    -   location of file
**	     fptr   -   file pointer associated with opened file
**
** Returns:
**	{STATUS}  OK - file open successful
**		  FAIL - couldn't open temp file
**
** Written: 5/25/84 (nml)
**	8/20/87 (peterk) - changed to work right with changes to 'NMloc()'.
**	05/89 (jhw) -- Corrected to get TEMP path first.  Bug #14810.
**	4/21/92 (seg)
**	    Fixed incorrect declaration of paramter.
*/

STATUS
FEt_open_bin ( mode, fname, suffix, t_loc, fptr )
char		*mode;
char		*fname;
char		*suffix;
LOCATION	*t_loc;
FILE		**fptr;
{
	STATUS	status;

	if ( (status = NMloc(TEMP, PATH, (char *)NULL, t_loc)) != OK  ||
			(status = LOuniq(fname, suffix, t_loc)) != OK  ||
		(status = SIfopen(t_loc, mode, SI_VAR, 0, fptr)) != OK )
	{ /* error! */
		return status;
	}
	return OK;
}
