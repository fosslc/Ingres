/*
** Copyright (c) 1989, 2000 Ingres Corporation
*/

/**
** Name: TC.H - Global definitions for the SI compatibility library.
**
** Description:
**      The file contains the type used by TC and the definition of the
**      TC functions.  These are used for COMM-files I/O.
**
** History:
**      22-Apr-1989 (Eduardo)
**          Created it.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**/

/* 
** Forward and/or External typedef/struct references.
*/
typedef struct _TCFILE TCFILE;


/*
**  Forward and/or External function references.
*/


/* 
** Defined Constants
*/

/* TC special characters */

#define	    TC_EOF	    -1	    /* end of file */
#define	    TC_TIMEDOUT	    (i4)-2 /* time out on TCgetc */
#define	    TC_EOQ	    034	    /* end of query */
#define	    TC_EQR	    035	    /* end of query results */
#define	    TC_BOS	    036	    /* beginning of session */

/* The following symbols are machine dependent */

#define	    _COMMFILES	    10		/* max. no. of opened COMM-files */

/* special status */

#define	    TC_BAD_SYNTAX   10


/*
** Type Definitions.
*/

/* FILE control struct. */

typedef	struct _TCFILE
{
	char	*_buf;
	char	*_pos;
	char	*_end;
	short	_id;
	char	_flag;
};


#define	    _TCREAD	    01
#define	    _TCWRT	    02
#define	    _TCOPEN	    04
#define	    _TCEOF	    020

