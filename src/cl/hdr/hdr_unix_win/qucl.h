/*
** Copyright (c) 2004 Ingres Corporation
*/

# ifndef QUE_DEF
# define QUE_DEF 1

/**
** Name: QU.H - Global definitions for the QU compatibility library.
**
** Description:
**      The file contains the type used by QU and the definition of the
**      QU functions.  These are used for manipulating queues.
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
** Forward and/or External typedefs/struct references.
*/
typedef struct _QUEUE QUEUE;

/*
**  Forward and/or External function references.
*/


/* 
** Defined Constants
*/

/* QU return status codes. */

#define                 QU_OK           0


struct _QUEUE
{
	struct	_QUEUE		*q_next;
	struct	_QUEUE		*q_prev;
};
# endif

