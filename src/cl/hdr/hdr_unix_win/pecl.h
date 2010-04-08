/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: PE.H - Global definitions for the PE compatibility library.
**
** Description:
**      The file contains the type used by PE and the definition of the
**      PE functions.  These are used for permission handling.
**
** History: 
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**/

/*
**  Forward and/or External function references.
*/


/* 
** Defined Constants
*/

/* PE return status codes. */

#define                 PE_OK           0
