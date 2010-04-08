/*
** Copyright (c) 1985, Ingres Corporation
*/

# ifndef QUE_DEF
# define QUE_DEF 1

/**CL_SPEC
** Name: QU.H - Global definitions for the QU compatibility library.
**
** Specification:
**
** Description:
**      The file contains the type used by QU and the definition of the
**      QU functions.  These are used for manipulating queues.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	19-aug-1997 (teresak)
**	    Moved ifndef QUE_DEF & define QUE_DEF 1 to the beginning of file.
**	    This addresses multiple inclusions of qu.h 
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

