/*
** Copyright (c) 1991, Ingres Corporation
*/

#ifndef		SEM_INCLUDED
#define		SEM_INCLUDED	1

/**CL_SPEC
** Name: SEM.H - Miscellaneous semaphores for the CL
**
** Description:
**      This file contains define's and datatype definitions for semaphores
**	for use in various CL modules that need them (e.g. TR).  These mutex
**	semaphores are guaranteed to be initialized by the DLL startup routine.
**
** History: $Log-for RCS$
**	 23 April 1991 (seg)
**	    Created
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
[@history_template@]...
**/

/*
**  Forward and/or External typedef/struct references.
*/
#define  SEM_MUTEX_COUNT 4

#define TR_SEM		0	/* used by tr */
#define GCC_SEM		1	/* used by gcc protocol drivers */
#define ME_TAG_SEM	2	/* used by me */
#define ME_REQ_SEM	3	/* used by me */

FUNC_EXTERN VOID SEMrequest(i4);
FUNC_EXTERN VOID SEMrelease(i4);

#endif /* SEM_INCLUDED */
