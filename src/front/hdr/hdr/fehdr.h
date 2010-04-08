/*
**	fehdr.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fehdr.h - Header file constants that Front-Ends need.
**
** Description:
**	This file contains constant definition that all front ends need.
**
** History:
**	static	char	Sccsid[] = "@(#)fehdr.h	1.1	1/24/85";
**	02/20/87 (dkh) - Added definition of FE_MAXNAME.
**	03/06/87 (dkh) - Renamed BUFSIZ to FE_BUFSIZ to eliminate
**			 conflict with similar declaration in "si.h".
**	03/11/87 (dkh) - Deleted FE_BUFSIZE as no one is using it.
**	03/21/87 (bobm)  Delete EXTYPUSER
**/

/*
**  Needed by some front-ends.  Please notify front-end
**  group if this number is going to CHANGE!
*/

# define	PG_SIZE		2048

/*
**  New front end constant that defines the maximum name
**  length of front end objects (e.g., form/field names).
**  This is needed since front end object names may
**  need to be bigger on certain environments (such as DG).
**  That's why FE_MAXNAME, instead of DB_MAXNAME, should be used. (dkh)
*/
# define	FE_MAXNAME	32

/*
**  Maximum number of characters which will be returned in an FRS prompt
**  response, no matter how wide the screen is.  In practice, this is wider
**  than any screens we know of thus far.  Useful in allocating response
**  buffers.  Does not count the terminating EOS.
*/
# define	FE_PROMPTSIZE	200
