/*
** Copyright (c) 1989, Ingres Corporation
*/

/**
** Name: LOPARSE.H - Definitions used by LOparse and its callers
**
** Description:
**      The file contains the structures used to call LOparse.
**
** History:
**	19-apr-89 (Mike S)
**	    Created
**/

/*
**  Forward and/or External function references.
*/

/* 
** Defined Constants
*/

/* Constants */

/* Structures */

/* Structure used by LOparse and its callers */
struct _LOpstruct
{
  	struct FAB pfab;			/* File Access Block */
	struct NAM pnam;                        /* NAMe block */
	char 	   pesa[MAX_LOC+1];             /* Extended String Area */
};

typedef struct _LOpstruct LOpstruct;
