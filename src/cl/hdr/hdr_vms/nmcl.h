/*
** Copyright (c) 1985, Ingres Corporation
*/

/**CL_SPEC
** Name: NM.H - Global definitions for the NM compatibility library.
**
** Specification:
**
** Description:
**      The file contains the type used by NM and the definition of the
**      NM functions.  These are used for handling INGRES names.
**
** History:
**      01-oct-85 (jennifer)
**          Updated to coding standard for 6.0.
**	03-aug-87 (greg)
**	    Update for new symbols, obsolete old ones.
**	27-Jan-88 (ericj)
**	    Update to support sort locations.
**	08-feb-89 (EdHsu)
**	    Updated to support online backup.
**	30-oct-92 (jrb)
**	    Changed ING_SORTDIR to ING_WORKDIR for ML Sort project.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	22-Mar-2010 (hanje04)
**	    SIR 123296
**	    Define ADMIN & LOG for VMS as they are used as 'writable' locations
**	    for LSB builds. Change should have no net effect for VMS.
**/

/*
**  Forward and/or External function references.
*/


/* 
** Defined Constants
*/

/* NM return status codes. */

#define                 NM_OK           0

/*
** Symbols used by NMgtAt().
*/
# define	ING_CKDIR	"II_CHECKPOINT"
# define	ING_DBDIR	"II_DATABASE"
# define	ING_JNLDIR 	"II_JOURNAL"
# define	ING_WORKDIR	"II_WORK"
# define	ING_DMPDIR	"II_DUMP"
/*
** These are not currently used but maybe they make sense
*/
# define	ING_EDIT	"ING_EDIT"
# define	ING_PRINT	"ING_PRINT"
/*
** Am too lazy to check if these are used now.
*/
# define	USER_NAME	"USER"
# define	USER_PORT	""
# define	TERM_TYPE	"TERM"

/*
** Symbols used by NMloc().
*/

# define	BIN		0
# define	DBTMPLT		1
# define	FILES		3
# define	UTILITY		2
# define	LIB		4
# define	SUBDIR		5
# define	TEMP		6
# define	ADMIN		7
# define	LOG		8
